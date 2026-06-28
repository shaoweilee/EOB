// Copyright Epic Games, Inc. All Rights Reserved.

#include "EmpireOfBossPlayerController.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "EmpireOfBoss.h"
#include "MyGameplayTagsLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EOB_AttributeSet.h"
#include "EOB_HUDWidget.h"
#include "GameplayEffectTypes.h" // 引入 GAS 结构体依赖

AEmpireOfBossPlayerController::AEmpireOfBossPlayerController()
{
	bMoveToMouseCursor = false;

	// configure the controller
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	DefaultMouseCursor = EMouseCursor::Default;
	CachedDestination = FVector::ZeroVector;
	FollowTime = 0.f;
}

void AEmpireOfBossPlayerController::BeginPlay()
{
	Super::BeginPlay();
	MyHero = Cast<AEmpireOfBossCharacter>(GetCharacter());
	if (MyHero)
	{
		MoveComp = MyHero->GetCharacterMovement();
		UABC = MyHero->GetAbilitySystemComponent();
		OriginMaxWalkSpeed = MoveComp->MaxWalkSpeed;
	}


	// 🌟 2. 在末尾无缝加入 UI 订阅管道
	if (UABC && MyHero)
	{
		if (UEOB_AttributeSet* EOB_AS = MyHero->GetEOBAttributeSet())
		{
			// 核心魔法：向 ASC 订阅“当 Health 属性发生任何改变时，立刻呼叫我的 OnPlayerHealthChanged”
			UABC->GetGameplayAttributeValueChangeDelegate(UEOB_AttributeSet::GetHealthAttribute())
			    .AddUObject(this, &AEmpireOfBossPlayerController::OnPlayerHealthChanged);
		}
	}
}

void AEmpireOfBossPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// if (MyHero)
	// {
	// 	MyHero->GetCharacterMovement()->Velocity = FVector(500.f, 0.f, 0.f);
	// }
}

void AEmpireOfBossPlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
		// Set up action bindings
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
		{
			// Setup mouse input events
			EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Started, this,
			                                   &AEmpireOfBossPlayerController::OnInputStarted);
			EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Triggered, this,
			                                   &AEmpireOfBossPlayerController::OnSetDestinationTriggered);
			EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Completed, this,
			                                   &AEmpireOfBossPlayerController::OnSetDestinationReleased);
			EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Canceled, this,
			                                   &AEmpireOfBossPlayerController::OnSetDestinationReleased);
			// rooted
			EnhancedInputComponent->BindAction(RootedAction, ETriggerEvent::Started, this,
			                                   &AEmpireOfBossPlayerController::OnRootKeyStarted);
			EnhancedInputComponent->BindAction(RootedAction, ETriggerEvent::Completed, this,
			                                   &AEmpireOfBossPlayerController::OnRootKeyCompleted);
			EnhancedInputComponent->BindAction(RootedAction, ETriggerEvent::Canceled, this,
			                                   &AEmpireOfBossPlayerController::OnRootKeyCancelled);
		}
		else
		{
			UE_LOG(LogEmpireOfBoss, Error,
			       TEXT(
				       "'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."
			       ), *GetNameSafe(this));
		}
	}
}

void AEmpireOfBossPlayerController::OnInputStarted()
{
	RotateCharacterToCursor();
	StopMovement();
}

void AEmpireOfBossPlayerController::OnSetDestinationTriggered()
{
	// We flag that the input is being pressed
	FollowTime += GetWorld()->GetDeltaSeconds();

	FHitResult Hit;
	bool bHitSuccessful = false;
	bHitSuccessful = GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);

	// If we hit a surface, cache the location
	if (bHitSuccessful)
	{
		CachedDestination = Hit.Location;
	}
	if (IsBlockMove())
	{
		// 按住移动时高频触发此函数，我们实时让角色去锁定和追踪鼠标的最新朝向
		RotateCharacterToCursor();
		return;
	}
	if (MyHero)
	{
		FVector WorldDirection = (CachedDestination - MyHero->GetActorLocation()).GetSafeNormal();
		MyHero->AddMovementInput(WorldDirection, 1.0, false);
	}
}

void AEmpireOfBossPlayerController::OnSetDestinationReleased()
{
	// If it was a short press
	if (FollowTime <= ShortPressThreshold)
	{
		if (IsBlockMove())
		{
			FollowTime = 0.f;
			return;
		}
		// We move there 
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, CachedDestination);
		// and spawn some particles
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FXCursor, CachedDestination, FRotator::ZeroRotator,
		                                               FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
	}

	FollowTime = 0.f;
}

void AEmpireOfBossPlayerController::OnRootKeyStarted()
{
	if (UABC)
	{
		UABC->AddLooseGameplayTag(FMyGameplayTags::State_Rooted);
	}
}

void AEmpireOfBossPlayerController::OnRootKeyCompleted()
{
	if (UABC)
	{
		UABC->RemoveLooseGameplayTag(FMyGameplayTags::State_Rooted);
	}
}

void AEmpireOfBossPlayerController::OnRootKeyCancelled()
{
	if (UABC)
	{
		UABC->RemoveLooseGameplayTag(FMyGameplayTags::State_Rooted);
	}
}

bool AEmpireOfBossPlayerController::IsBlockMove()
{
	return UABC->HasMatchingGameplayTag(FMyGameplayTags::State_Rooted) || UABC->HasMatchingGameplayTag(
		FMyGameplayTags::State_Restricted_KnockedBack);
}

void AEmpireOfBossPlayerController::RotateCharacterToCursor()
{
	if (!MyHero || !MoveComp) return;
	FVector TargetWorldLocation = FVector::ZeroVector;
	FVector WorldLocation;
	FVector WorldDirection;
	if (DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		FHitResult HitResult;
		if (GetHitResultUnderCursor(ECC_Visibility, false, HitResult))
		{
			TargetWorldLocation = HitResult.ImpactPoint;
		}
		else
		{
			float PawnZ = MyHero->GetActorLocation().Z;
			if (!FMath::IsNearlyZero(WorldDirection.Z))
			{
				float t = (PawnZ - WorldLocation.Z) / WorldDirection.Z;
				if (t > 0.f) TargetWorldLocation = WorldLocation + (WorldDirection * t);
			}
		}
		FVector PawnLocation = MyHero->GetActorLocation();
		TargetWorldLocation.Z = PawnLocation.Z;
		if (!TargetWorldLocation.IsZero())
		{
			DynamicTargetRotation = (TargetWorldLocation - PawnLocation).Rotation();
			MoveComp->bOrientRotationToMovement = false;
			FTimerManager& TimerManager = GetWorldTimerManager();
			if (!TimerManager.IsTimerActive(SmoothRotateTimerHandle))
			{
				TimerManager.SetTimer(SmoothRotateTimerHandle, [this]()
				{
					if (!MyHero || !MoveComp) return;
					FRotator CurrentRot = MyHero->GetActorRotation();
					FRotator NewRot = FMath::RInterpTo(CurrentRot, DynamicTargetRotation, 0.01f, 25.0f);
					MyHero->SetActorRotation(NewRot);
					if (CurrentRot.Equals(DynamicTargetRotation, 1.0f))
					{
						MoveComp->bOrientRotationToMovement = true;
						GetWorldTimerManager().ClearTimer(SmoothRotateTimerHandle);
					}
				}, 0.01f, true);
			}
		}
	}
}

// 🌟 3. 实现回调函数
void AEmpireOfBossPlayerController::OnPlayerHealthChanged(const FOnAttributeChangeData& Data)
{
	// Data.NewValue 就是扣血/加血后的最新值！
	float CurrentHealth = Data.NewValue;
	float MaxHealth = MyHero->GetEOBAttributeSet()->GetMaxHealth();
	float HealthPercent = MaxHealth > 0.f ? (CurrentHealth / MaxHealth) : 0.f;

	if (EOBHUDWidget)
	{
		EOBHUDWidget->VM_UpdateHPVisual(HealthPercent);
	}

	UE_LOG(LogTemp, Log, TEXT("[UI 联动管道]: 检测到玩家血量发生改变！当前最新血量为: %.1f"), CurrentHealth);
}
