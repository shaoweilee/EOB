#include "GA_Skill_Fireball.h"

#include "EOB_Projectile.h"

void UGA_Skill_Fireball::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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
	if (!Avatar || !ProjectileClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	RotateAvatarToCursor();

	bool bIsCrit = false; // 法术不暴击
	const float Damage = ComputeSkillDamage(DamageMultiplier, false, bIsCrit);

	// 出生点：身前 80cm、高 90cm
	const FVector SpawnLoc = Avatar->GetActorLocation()
		+ Avatar->GetActorForwardVector() * 80.f
		+ FVector(0.f, 0.f, 90.f);

	// 🌟 弹道瞄准光标落点（带俯仰角），不再是水平直飞。
	//    水平直飞 + 地形起伏下坡，视觉上就像"斜着往上飘"；瞄着落点飞就是指哪打哪。
	FRotator SpawnRot = Avatar->GetActorRotation();
	FVector AimPoint;
	if (GetCursorGroundPoint(AimPoint))
	{
		const FVector Dir = AimPoint - SpawnLoc;
		if (!Dir.IsNearlyZero())
		{
			SpawnRot = Dir.Rotation();
		}
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (AEOB_Projectile* Proj = GetWorld()->SpawnActor<AEOB_Projectile>(ProjectileClass, SpawnLoc, SpawnRot, Params))
	{
		Proj->Init(Damage, false, DamageEffectClass, GetAbilitySystemComponentFromActorInfo(), Avatar);
	}

	UE_LOG(LogTemp, Log, TEXT("[技能] 火球！伤害 %.1f"), Damage);
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
