// Copyright Epic Games, Inc. All Rights Reserved.

#include "EmpireOfBossPlayerController.h"

#include "AbilitySystemComponent.h"
#include "CPP_Enemy_Base.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "EmpireOfBoss.h"
#include "MyGameplayTagsLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EOB_AttributeSet.h"
#include "EOB_HUDWidget.h"
#include "GameplayEffectTypes.h" // 引入 GAS 结构体依赖
#include "EOB_SkillTreeComponent.h"
#include "EOB_GameplayAbility.h"

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
		OriginMaxWalkSpeed = MoveComp->MaxWalkSpeed;
	}
	GetWorldTimerManager().SetTimerForNextTick(this, &AEmpireOfBossPlayerController::BindUIEvent);
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
			// M2: 背包开合
			EnhancedInputComponent->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this,
			                                   &AEmpireOfBossPlayerController::OnToggleInventory);

			// M3a: 角色面板开合
			EnhancedInputComponent->BindAction(ToggleCharacterAction, ETriggerEvent::Started, this,
			                                   &AEmpireOfBossPlayerController::OnToggleCharacter);

			// M3b: 技能树开合 + 技能快捷键 + 右键当前技能 + Tab 切换
			EnhancedInputComponent->BindAction(ToggleSkillTreeAction, ETriggerEvent::Started, this,
			                                   &AEmpireOfBossPlayerController::OnToggleSkillTree);
			EnhancedInputComponent->BindAction(Skill1Action, ETriggerEvent::Started, this,
			                                   &AEmpireOfBossPlayerController::OnSkill1);
			EnhancedInputComponent->BindAction(Skill2Action, ETriggerEvent::Started, this,
			                                   &AEmpireOfBossPlayerController::OnSkill2);
			EnhancedInputComponent->BindAction(Skill3Action, ETriggerEvent::Started, this,
			                                   &AEmpireOfBossPlayerController::OnSkill3);
			EnhancedInputComponent->BindAction(Skill4Action, ETriggerEvent::Started, this,
			                                   &AEmpireOfBossPlayerController::OnSkill4);
			EnhancedInputComponent->BindAction(CastCurrentSkillAction, ETriggerEvent::Started, this,
			                                   &AEmpireOfBossPlayerController::OnCastCurrentSkill);
			EnhancedInputComponent->BindAction(CycleSkillAction, ETriggerEvent::Started, this,
			                                   &AEmpireOfBossPlayerController::OnCycleSkill);
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
	// 1. 先检测是否点到了敌人
	AActor* EnemyTarget = GetTargetUnderCursor();

	if (EnemyTarget)
	{
		// 标记为攻击意图
		MyHero->CurrentTarget = EnemyTarget;
		MyHero->bIsTryingToAttack = true;
		bPressedOnEnemy = true; // 🌟 本次按压点在敌人身上，松手时不发地面寻路

		// 可选：在这里播放一个点击目标的特效
		return;
	}

	// 🌟 2. 核心拦截：检测是否点到了宝箱等可交互物件
	FHitResult InteractHit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, InteractHit))
	{
		if (InteractHit.GetActor() && InteractHit.GetActor()->ActorHasTag("Interactable"))
		{
			// 点到宝箱了！清空攻击意图
			MyHero->bIsTryingToAttack = false;
			MyHero->CurrentTarget = nullptr;
			bPressedOnEnemy = false; // 🌟 交互组件自己会寻路，但也别走 Released 的地面寻路

			// 🌟 直接 return！彻底放权给宝箱的 InteractableComponent::OnOwnerClicked 去处理它的 SimpleMoveToActor。
			// 这样控制器就不会用普通地面的 SimpleMoveToLocation 去掐断组件的寻路了！
			return;
		}
	}

	// 2. 如果没点到敌人，清空攻击意图，执行原有逻辑
	MyHero->bIsTryingToAttack = false;
	MyHero->CurrentTarget = nullptr;
	bPressedOnEnemy = false; // 🌟 这次按压是点地面

	// 🌟 核心修正：点到树木/墙壁时，立刻让射线走 ECC_GameTraceChannel2 专线击穿它们，落锁到地面！
	FHitResult GroundHit;
	if (GetHitResultUnderCursor(ECC_GameTraceChannel2, true, GroundHit))
	{
		// 强行把 CachedDestination 刷新为击穿树木后的真实地表坐标
		CachedDestination = GroundHit.Location;
		// 🌟 核心手感优化：不要盲目执行 StopMovement() 导致角色发呆！
		// 只要点的是空地，在按下的第一帧，立刻利用 SimpleMoveToLocation 发起一次寻路点火！
		// 这能保证角色在 0.001 秒内就立刻起跑，彻底消灭前 0.2 秒的原地发呆。
		if (!IsBlockMove())
		{
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, CachedDestination);
		}
	}

	RotateCharacterToCursor();
	// StopMovement();
}

void AEmpireOfBossPlayerController::OnSetDestinationTriggered()
{
	// 🌟 点在敌人身上的按压：长按也不做跟随移动，追击交给 CheckAttackRangeAndExecute
	if (bPressedOnEnemy)
	{
		FollowTime = 0.f;
		return;
	}
	// We flag that the input is being pressed
	FollowTime += GetWorld()->GetDeltaSeconds();

	FHitResult Hit;

	// If we hit a surface, cache the location
	if (GetHitResultUnderCursor(ECC_GameTraceChannel2, true, Hit))
	{
		CachedDestination = Hit.Location;
	}
	if (IsBlockMove())
	{
		// 按住移动时高频触发此函数，我们实时让角色去锁定和追踪鼠标的最新朝向
		RotateCharacterToCursor();
		return;
	}
	if (FollowTime > ShortPressThreshold)
	{
		if (MyHero)
		{
			FVector WorldDirection = (CachedDestination - MyHero->GetActorLocation()).GetSafeNormal();
			MyHero->AddMovementInput(WorldDirection, 1.0, false);
		}
	}
}

void AEmpireOfBossPlayerController::OnSetDestinationReleased()
{
	// 🌟 点在敌人身上的按压：松开不发 SimpleMoveToLocation，否则攻击完角色还会往怪脚上贴
	if (bPressedOnEnemy)
	{
		bPressedOnEnemy = false;
		FollowTime = 0.f;
		return;
	}

	// 如果按住的时间非常短，判定为标准的“暗黑流点击走位”
	if (FollowTime <= ShortPressThreshold)
	{
		if (IsBlockMove())
		{
			FollowTime = 0.f;
			return;
		}
		// 🌟 再次安全兜底检测，确保 CachedDestination 是点击地面时的最新坐标
		FHitResult FloorHit;
		if (GetHitResultUnderCursor(ECC_GameTraceChannel2, true, FloorHit))
		{
			CachedDestination = FloorHit.Location;
		}
		// 此时因为长按的 AddMovementInput 没触发，导航路径不会被打断，角色可以 100% 完美走过去
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, CachedDestination);
		// 生成点击地面的金币/脚印特效
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FXCursor, CachedDestination, FRotator::ZeroRotator,
		                                               FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
	}

	FollowTime = 0.f;
}

void AEmpireOfBossPlayerController::OnRootKeyStarted()
{
	if (MyHero->AbilitySystemComponent)
	{
		MyHero->AbilitySystemComponent->AddLooseGameplayTag(FMyGameplayTags::State_Rooted);
	}
}

void AEmpireOfBossPlayerController::OnRootKeyCompleted()
{
	if (MyHero->AbilitySystemComponent)
	{
		MyHero->AbilitySystemComponent->RemoveLooseGameplayTag(FMyGameplayTags::State_Rooted);
	}
}

void AEmpireOfBossPlayerController::OnRootKeyCancelled()
{
	if (MyHero->AbilitySystemComponent)
	{
		MyHero->AbilitySystemComponent->RemoveLooseGameplayTag(FMyGameplayTags::State_Rooted);
	}
}

bool AEmpireOfBossPlayerController::IsBlockMove()
{
	return MyHero->AbilitySystemComponent->HasMatchingGameplayTag(FMyGameplayTags::State_Rooted) || MyHero->
		AbilitySystemComponent->HasMatchingGameplayTag(
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
		if (GetHitResultUnderCursor(ECC_GameTraceChannel2, false, HitResult))
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
	float CurrentHealth = MyHero->AttributeSet->GetHealth();
	float MaxHealth = MyHero->AttributeSet->GetMaxHealth();
	float HealthPercent = MaxHealth > 0.f ? (CurrentHealth / MaxHealth) : 0.f;

	if (EOBHUDWidget)
	{
		EOBHUDWidget->VM_UpdateHPVisual(HealthPercent);
	}

	UE_LOG(LogTemp, Log, TEXT("[UI 联动管道]: 检测到玩家血量发生改变！当前最新血量为: %.1f"), Data.NewValue);
}

void AEmpireOfBossPlayerController::OnPlayerManaChanged(const FOnAttributeChangeData& Data)
{
	const float CurrentMana = MyHero->AttributeSet->GetMana();
	const float MaxMana = MyHero->AttributeSet->GetMaxMana();
	const float ManaPercent = MaxMana > 0.f ? (CurrentMana / MaxMana) : 0.f;

	if (EOBHUDWidget)
	{
		EOBHUDWidget->VM_UpdateMPVisual(ManaPercent);
	}
	// UE_LOG(LogTemp, Log, TEXT("[UI 联动管道]: 检测到玩家蓝量发生改变！当前最新蓝量为: %.1f"), Data.NewValue);
}

AActor* AEmpireOfBossPlayerController::GetTargetUnderCursor()
{
	FHitResult HitResult;
	// 假设 ECC_GameTraceChannel2 是你的 Enemy_Trace 通道
	if (GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_GameTraceChannel4), true, HitResult))
	{
		return HitResult.GetActor(); // 只要有返回，百分之百是敌人
	}
	return nullptr;
}

void AEmpireOfBossPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// 🌟 每帧执行敌人悬停检测
	CheckEnemyHoverUnderCursor();
}

void AEmpireOfBossPlayerController::CheckEnemyHoverUnderCursor()
{
	if (!EOBHUDWidget) return;

	if (AActor* CurrentHoveredActor = GetTargetUnderCursor())
	{
		if (LastHoveredEnemy != CurrentHoveredActor)
		{
			LastHoveredEnemy = CurrentHoveredActor;
			EOBHUDWidget->ShowStateBar(ESlateVisibility::Visible);

			// 🌟 核心修复：切悬停目标的瞬间，立刻拉取新敌人的真实血量刷血条
			// 不再显示上一个敌人留下的残留值
			if (ACPP_Enemy_Base* Enemy = Cast<ACPP_Enemy_Base>(CurrentHoveredActor))
			{
				const float MaxHP = Enemy->AttributeSet->GetMaxHealth();
				const float CurrentHP = Enemy->AttributeSet->GetHealth();
				const float Percent = MaxHP > 0.f ? Enemy->AttributeSet->GetHealth() / MaxHP : 0.f;
				FText HPText = FText::FormatNamed(
					FText::FromString(TEXT("{CurrentHP} / {MaxHP}")),
					TEXT("CurrentHP"), FText::AsNumber(CurrentHP), // int32 可以直接传
					TEXT("MaxHP"), FText::AsNumber(MaxHP)
				);
				EOBHUDWidget->BP_UpdateEnemyHP(Percent);
				EOBHUDWidget->BP_UpdateEnemyText(Enemy->EnemyName, HPText);
			}
		}
	}
	else
	{
		if (LastHoveredEnemy.IsValid())
		{
			LastHoveredEnemy = nullptr;
			EOBHUDWidget->ShowStateBar(ESlateVisibility::Collapsed);
		}
	}
}

void AEmpireOfBossPlayerController::OnToggleInventory()
{
	if (EOBHUDWidget)
	{
		EOBHUDWidget->ToggleInventoryPanels();
	}
}

void AEmpireOfBossPlayerController::OnToggleCharacter()
{
	if (EOBHUDWidget)
	{
		EOBHUDWidget->ToggleCharacterPanel();
	}
}

void AEmpireOfBossPlayerController::OnToggleSkillTree()
{
	if (EOBHUDWidget)
	{
		EOBHUDWidget->ToggleSkillTreePanel();
	}
}

void AEmpireOfBossPlayerController::CastSkillAtSlot(int32 SlotIndex)
{
	if (!MyHero || !MyHero->SkillTreeComponent || !MyHero->AbilitySystemComponent) return;

	if (TSubclassOf<UGameplayAbility> AbilityClass =
		MyHero->SkillTreeComponent->GetLearnedAbilityAt(SlotIndex))
	{
		// 直放的同时设为右键当前技能（TL2 手感）
		MyHero->SkillTreeComponent->CurrentSkillSlot = SlotIndex;
		const bool bActivated = MyHero->AbilitySystemComponent->TryActivateAbilityByClass(AbilityClass);
		if (!bActivated)
		{
			UE_LOG(LogTemp, Warning, TEXT("[技能] 释放失败：冷却中或蓝不足"));
		}
	}
}

void AEmpireOfBossPlayerController::OnSkill1() { CastSkillAtSlot(0); }
void AEmpireOfBossPlayerController::OnSkill2() { CastSkillAtSlot(1); }
void AEmpireOfBossPlayerController::OnSkill3() { CastSkillAtSlot(2); }
void AEmpireOfBossPlayerController::OnSkill4() { CastSkillAtSlot(3); }

void AEmpireOfBossPlayerController::OnCastCurrentSkill()
{
	if (!MyHero || !MyHero->SkillTreeComponent) return;
	CastSkillAtSlot(MyHero->SkillTreeComponent->CurrentSkillSlot);
}

void AEmpireOfBossPlayerController::OnCycleSkill()
{
	if (MyHero && MyHero->SkillTreeComponent)
	{
		MyHero->SkillTreeComponent->CycleCurrentSkill();
	}
}

void AEmpireOfBossPlayerController::BindUIEvent()
{
	// 🌟 2. 在末尾无缝加入 UI 订阅管道
	if (MyHero && MyHero->AbilitySystemComponent && MyHero->AttributeSet)
	{
		// 核心魔法：向 ASC 订阅“当 Health 属性发生任何改变时，立刻呼叫我的 OnPlayerHealthChanged”
		MyHero->AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			      UEOB_AttributeSet::GetHealthAttribute())
		      .AddUObject(this, &AEmpireOfBossPlayerController::OnPlayerHealthChanged);
		// 订阅 Mana 变化 → 刷新蓝条
		MyHero->AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			      UEOB_AttributeSet::GetManaAttribute())
		      .AddUObject(this, &AEmpireOfBossPlayerController::OnPlayerManaChanged);
		// maxhealth
		MyHero->AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			      UEOB_AttributeSet::GetMaxHealthAttribute())
		      .AddUObject(this, &AEmpireOfBossPlayerController::OnPlayerHealthChanged);
		// maxmana
		MyHero->AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
			      UEOB_AttributeSet::GetMaxManaAttribute())
		      .AddUObject(this, &AEmpireOfBossPlayerController::OnPlayerManaChanged);

		// 🩺 诊断日志：确认订阅块真的执行了
		UE_LOG(LogTemp, Warning, TEXT("[UI 联动管道] 血量/蓝量订阅已完成！"));
	}
	else
	{
		// 🩺 诊断日志：订阅块被跳过，说明此刻 MyHero/ASC/AttributeSet 还没就绪
		UE_LOG(LogTemp, Error, TEXT("[UI 联动管道] 订阅失败！MyHero=%s ASC=%s AttributeSet=%s"),
		       MyHero ? TEXT("OK") : TEXT("NULL"),
		       (MyHero && MyHero->AbilitySystemComponent) ? TEXT("OK") : TEXT("NULL"),
		       (MyHero && MyHero->AttributeSet) ? TEXT("OK") : TEXT("NULL"));
	}
}
