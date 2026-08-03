#include "GA_Skill_Charge.h"

#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

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

	// 2. 位移：胶囊体扫掠找落脚点（撞墙则停在墙前），然后瞬移过去
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
		C->TeleportTo(Dest, C->GetActorRotation());
	}

	UE_LOG(LogTemp, Log, TEXT("[技能] 冲锋！伤害 %.1f%s"), Damage, bIsCrit ? TEXT("（暴击！）") : TEXT(""));
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
