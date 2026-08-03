#include "GA_Skill_Charge.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "TimerManager.h"

void UGA_Skill_Charge::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo,
                                       const FGameplayAbilityActivationInfo ActivationInfo,
                                       const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 1. 面向鼠标 → 路径伤害（窄扇形）
	RotateAvatarToCursor();

	bool bIsCrit = false;
	const float Damage = ComputeSkillDamage(DamageMultiplier, true, bIsCrit);
	ApplyDamageFan(DamageRadius, 25.f, Damage, bIsCrit);

	// 2. 位移：胶囊体扫掠找落脚点（撞墙则停在墙前），然后定时器驱动平滑冲刺
	if (ACharacter* C = Cast<ACharacter>(Avatar))
	{
		const FVector Start = C->GetActorLocation();
		const FVector End = Start + C->GetActorForwardVector() * ChargeDistance;

		FHitResult Hit;
		const UCapsuleComponent* Capsule = C->GetCapsuleComponent();
		FCollisionShape Shape = FCollisionShape::MakeCapsule(
			Capsule ? Capsule->GetScaledCapsuleRadius() : 40.f,
			Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 90.f);
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(C);

		FVector Dest = End;
		if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_WorldStatic, Shape, Params))
		{
			Dest = Hit.Location + C->GetActorForwardVector() * -50.f; // 墙前留一步
		}

		// 🌟 平滑冲刺：停掉当前移动，0.15s 内从起点插值到终点
		C->GetCharacterMovement()->StopMovementImmediately();
		DashStart = Start;
		DashDest = Dest;
		DashElapsed = 0.f;
		GetWorld()->GetTimerManager().SetTimer(DashTimerHandle, this, &UGA_Skill_Charge::DashTick, 0.016f, true);
	}

	UE_LOG(LogTemp, Log, TEXT("[技能] 冲锋！伤害 %.1f%s"), Damage, bIsCrit ? TEXT("（暴击！）") : TEXT(""));
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}

void UGA_Skill_Charge::DashTick()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		GetWorld()->GetTimerManager().ClearTimer(DashTimerHandle);
		return;
	}

	DashElapsed += 0.016f;
	const float Alpha = FMath::Clamp(DashElapsed / DashDuration, 0.f, 1.f);
	const FVector NewLoc = FMath::Lerp(DashStart, DashDest, Alpha);

	// sweep=true：冲刺途中撞到墙或怪物身体会被挡停，不会穿模
	Avatar->SetActorLocation(NewLoc, true);

	if (Alpha >= 1.f)
	{
		GetWorld()->GetTimerManager().ClearTimer(DashTimerHandle);
	}
}
