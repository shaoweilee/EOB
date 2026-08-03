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
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "MyGameplayTagsLibrary.h"
#include "EOB_InventoryComponent.h"
#include "EOB_AttributeSet.h"
#include "EOB_LevelComponent.h"

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

	// M2 新增：背包与装备组件
	InventoryComponent = CreateDefaultSubobject<UEOB_InventoryComponent>(TEXT("InventoryComponent"));

	// M3a 新增：经验/升级/属性点组件
	LevelComponent = CreateDefaultSubobject<UEOB_LevelComponent>(TEXT("LevelComponent"));
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
	if (!MyCapsule)
	{
		return;
	}

	// =====================================================================
	// 🌟 核心设计：动态修改碰撞矩阵对敌人（Enemy）通道的响应
	// 
	// 1. 如果 bEnabled 为 true（开启幽灵步）：
	//    将 Capsule 对 Enemy 通道响应设为 ECR_Ignore（忽略），主角能直接穿怪。
	//
	// 2. 如果 bEnabled 为 false（关闭幽灵步）：
	//    将 Capsule 对 Enemy 通道响应设为 ECR_Block（阻挡），主角恢复实体碰撞，不再重叠。
	// =====================================================================

	// 假设 ECC_GameTraceChannel1 是你在项目设置中创建的自定义对象通道 "Enemy"
	// 注意：你需要在引擎 Project Settings -> Collision -> Object Channels 中创建 "Enemy"
	ECollisionChannel EnemyChannel = ECC_GameTraceChannel4;

	// 根据开关状态选择对应的碰撞响应
	ECollisionResponse NewResponse = bEnabled ? ECR_Ignore : ECR_Block;

	// 动态应用响应
	MyCapsule->SetCollisionResponseToChannel(EnemyChannel, NewResponse);

	// 辅助调试日志（开发完毕后可注释掉）
	UE_LOG(LogTemp, Log, TEXT("[%s] 幽灵步状态更新: %s, 对敌人碰撞响应设置为: %s"),
	       *GetName(),
	       bEnabled ? TEXT("开启") : TEXT("关闭"),
	       bEnabled ? TEXT("Ignore (忽略)") : TEXT("Block (阻挡)")
	);
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

	// M1 新增：追击途中目标已被打死（比如被溅射伤害收掉），立刻收手，不再追尸体
	if (UAbilitySystemComponent* TargetAsc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(CurrentTarget))
	{
		if (TargetAsc->HasMatchingGameplayTag(FMyGameplayTags::State_Dead))
		{
			CurrentTarget = nullptr;
			bIsTryingToAttack = false;
			return;
		}
	}

	float Distance = FVector::Dist(GetActorLocation(), CurrentTarget->GetActorLocation());

	// 如果距离小于 180cm
	if (Distance <= 180.f)
	{
		// 1. 停止移动
		GetCharacterMovement()->StopMovementImmediately();

		// 2. 转向目标
		FRotator LookAtRot = FRotationMatrix::MakeFromX(CurrentTarget->GetActorLocation() - GetActorLocation()).
			Rotator();
		SetActorRotation(FRotator(0, LookAtRot.Yaw, 0));

		// 3. 执行攻击
		PerformMeleeAttack(); // 播放蒙太奇
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
				// M2：基础随机伤害 + 攻击力属性（武器和词缀的加成从这里生效）
				const float BonusAttack = AttributeSet ? AttributeSet->GetAttackPower() : 0.f;
				float Damage = FMath::RandRange(2.f, 3.f) + BonusAttack;

				// 💥 M3a 新增：暴击判定（TL2：敏捷提供暴击率，力量提供暴击伤害）
				const float CritChance = AttributeSet ? AttributeSet->GetCritChance() : 0.f;
				const bool bIsCrit = CritChance > 0.f && (FMath::FRand() * 100.f < CritChance);
				if (bIsCrit)
				{
					const float CritMultiplier = AttributeSet ? AttributeSet->GetCritDamage() / 100.f : 1.5f;
					Damage *= CritMultiplier;
				}

				// 1. 安全地获取主角自己和击中怪物的 GAS 组件
				UAbilitySystemComponent* MyAbsc = AbilitySystemComponent;
				// 💡 UAbilitySystemGlobals 可以极其安全地从任何 Actor 身上拔出它的 ASC，不管它是谁！
				UAbilitySystemComponent* TargetAbsc =
					UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor);

				// M1 新增：尸体不再吃伤害（否则还会给尸体弹伤害数字）
				if (TargetAbsc && TargetAbsc->HasMatchingGameplayTag(FMyGameplayTags::State_Dead))
				{
					continue;
				}

				// 确保双方都有 GAS 系统，且你在蓝图里配好了 DamageEffectClass
				if (MyAbsc && TargetAbsc && DamageEffectClass)
				{
					// 2. 创建上下文：记录“这一刀是谁砍的”，方便以后做击杀回血、吸血等机制
					FGameplayEffectContextHandle ContextHandle = MyAbsc->MakeEffectContext();
					ContextHandle.AddInstigator(this, this);

					// 3. 打包伤害请求 (Spec)
					FGameplayEffectSpecHandle SpecHandle = MyAbsc->MakeOutgoingSpec(
						DamageEffectClass, 1.f, ContextHandle);

					if (SpecHandle.IsValid())
					{
						// 🌟 4. 将你 C++ 里随机出来的 2~3 点伤害，塞进 SetByCaller 里！
						SpecHandle.Data.Get()->SetSetByCallerMagnitude(FMyGameplayTags::Data_Damage, Damage);
						// 💥 M3a：把暴击标记也塞进 Spec，目标属性集结算时读出来飘橙色大数字
						SpecHandle.Data.Get()->SetSetByCallerMagnitude(FMyGameplayTags::Data_IsCrit,
						                                               bIsCrit ? 1.f : 0.f);

						// 5. 狠狠地灌进怪物的 GAS 系统！
						MyAbsc->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetAbsc);

						UE_LOG(LogTemp, Warning, TEXT("⚔️ 成功向怪物 [%s] 的伤害池灌入 %.1f 点随机伤害！"), *HitActor->GetName(),
						       Damage);
					}
				}
			}
		}
	}
}
