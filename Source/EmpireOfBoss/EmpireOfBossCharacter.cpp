// Copyright Epic Games, Inc. All Rights Reserved.

#include "EmpireOfBossCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"

AEmpireOfBossCharacter::AEmpireOfBossCharacter()
{
	// Set size for player capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate character to camera direction
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Create the camera boom component
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));

	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false;

	// Create the camera component
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));

	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// 让主角在物理矩阵中成为独一无二的 HeroType
	// 假设 HeroType 在 DefaultEngine.ini 中被分配为了 ECC_GameTraceChannel1 
	GetCapsuleComponent()->SetCollisionObjectType(ECC_GameTraceChannel1);
}

void AEmpireOfBossCharacter::BeginPlay()
{
	Super::BeginPlay();

	// stub
}

void AEmpireOfBossCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsTryingToAttack && CurrentTarget)
	{
		CheckAttackRangeAndExecute();
	}
}

void AEmpireOfBossCharacter::SetGhostWalkEnabled(bool bEnabled)
{
	UCapsuleComponent* MyCapsule = GetCapsuleComponent();
	if (!MyCapsule) return;

	if (bEnabled)
	{
		// 1. 技能开始：清除可能正在排队的恢复定时器
		GetWorldTimerManager().ClearTimer(GhostWalkSafeTimerHandle);

		// 2. 动态改写碰撞矩阵：将自己对普通怪物(Pawn通道)的响应从 Block 降级为 Overlap
		MyCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		UE_LOG(LogTemp, Log, TEXT("[穿怪系统]: 技能启动，主角进入幽灵虚化状态！"));
	}
	else
	{
		// 3. 技能结束：不直接变回 Block，而是进入安全检查
		TryRestoreSolidCollision();
	}
}

void AEmpireOfBossCharacter::TryRestoreSolidCollision()
{
	UCapsuleComponent* MyCapsule = GetCapsuleComponent();
	if (!MyCapsule) return;

	// 空间物理查询：利用当前主角胶囊体的实际大小，探测原地有没有重叠的怪物(ECC_Pawn)
	TArray<FOverlapResult> Overlaps;
	FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(MyCapsule->GetScaledCapsuleRadius(),
	                                                            MyCapsule->GetScaledCapsuleHalfHeight());
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bIsInsideEnemy = GetWorld()->OverlapMultiByChannel(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ECC_Pawn,
		CapsuleShape,
		Params
	);

	if (bIsInsideEnemy)
	{
		// 💥 卡肉警告：主角依然在怪物身体内部！如果此时强行变 Block，会导致引擎物理抖动或被弹飞
		// 补救措施：维持虚化状态，并开启定时器，10帧(0.1s)后再次尝试恢复，直到角色滑出怪圈
		if (!GetWorldTimerManager().IsTimerActive(GhostWalkSafeTimerHandle))
		{
			GetWorldTimerManager().SetTimer(GhostWalkSafeTimerHandle, this,
			                                &AEmpireOfBossCharacter::TryRestoreSolidCollision, 0.1f, true);
			UE_LOG(LogTemp, Warning, TEXT("[穿怪系统]: 检测到卡肉！主角暂缓恢复实体，正等待滑出怪堆..."));
		}
	}
	else
	{
		// 🎉 安全脱离：周围没有任何怪物与主角胶囊体穿插，可以完美变回硬直碰撞实体
		MyCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		GetWorldTimerManager().ClearTimer(GhostWalkSafeTimerHandle);
		UE_LOG(LogTemp, Log, TEXT("[穿怪系统]: 顺利脱离怪堆！恢复正常物理阻挡。"));
	}
}

void AEmpireOfBossCharacter::CheckAttackRangeAndExecute()
{
	if (!bIsTryingToAttack || !CurrentTarget) return;

	float Distance = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());

	// 如果距离小于 200cm (2米)
	if (Distance <= 200.f)
	{
		// 1. 停止移动
		GetCharacterMovement()->StopMovementImmediately();

		// 2. 转向目标
		FRotator LookAtRot = FRotationMatrix::MakeFromX(CurrentTarget->GetActorLocation() - GetActorLocation()).
			Rotator();
		SetActorRotation(FRotator(0, LookAtRot.Yaw, 0));

		// 3. 执行攻击
		PerformMeleeAttack(); // 播放蒙太奇
		bIsTryingToAttack = false; // 重置状态
	}
	else
	{
		// 继续寻路接近
		UAIBlueprintHelperLibrary::SimpleMoveToActor(GetController(), CurrentTarget);
	}
}

void AEmpireOfBossCharacter::PerformMeleeAttack()
{
	// 确保有配置动画，且当前没有在播攻击动画
	if (AttackMontage && !GetMesh()->GetAnimInstance()->IsAnyMontagePlaying())
	{
		// 只负责一件事：把手举起来，开始挥刀！
		PlayAnimMontage(AttackMontage);
	}
}

void AEmpireOfBossCharacter::ApplyFanDamage()
{
	float AttackRadius = 200.f; // 2米
	float HalfAngle = 45.f; // 扇形张角的一半（即 90 度扇形）

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Shape = FCollisionShape::MakeSphere(AttackRadius);

	// 获取周围所有敌人
	GetWorld()->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity, ECC_Pawn, Shape);

	for (auto& Result : Overlaps)
	{
		AActor* HitActor = Result.GetActor();
		if (HitActor && HitActor != this)
		{
			UE_LOG(LogTemp, Warning, TEXT("---%s---"), *HitActor->GetActorNameOrLabel());
			FVector DirToTarget = (HitActor->GetActorLocation() - GetActorLocation()).GetSafeNormal();
			FVector Forward = GetActorForwardVector();

			// 计算夹角：点积 > cos(弧度)
			float Dot = FVector::DotProduct(Forward, DirToTarget);
			float Angle = FMath::Acos(Dot) * 180.f / PI;

			if (Angle <= HalfAngle)
			{
				// 造成 2-3 点伤害
				float Damage = FMath::RandRange(2.f, 3.f);
				UGameplayStatics::ApplyDamage(HitActor, Damage, GetController(), this, UDamageType::StaticClass());
			}
		}
	}
}
