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

	// 出生点：身前 80cm、高 90cm，沿朝向飞
	const FVector SpawnLoc = Avatar->GetActorLocation()
		+ Avatar->GetActorForwardVector() * 80.f
		+ FVector(0.f, 0.f, 90.f);
	const FRotator SpawnRot = Avatar->GetActorRotation();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (AEOB_Projectile* Proj = GetWorld()->SpawnActor<AEOB_Projectile>(ProjectileClass, SpawnLoc, SpawnRot, Params))
	{
		Proj->Init(Damage, false, DamageEffectClass, GetAbilitySystemComponentFromActorInfo(), Avatar);
	}

	UE_LOG(LogTemp, Log, TEXT("[技能] 火球！伤害 %.1f"), Damage);
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
