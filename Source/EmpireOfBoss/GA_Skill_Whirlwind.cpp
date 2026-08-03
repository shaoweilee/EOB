#include "GA_Skill_Whirlwind.h"

void UGA_Skill_Whirlwind::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                          const FGameplayAbilityActorInfo* ActorInfo,
                                          const FGameplayAbilityActivationInfo ActivationInfo,
                                          const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bool bIsCrit = false;
	const float Damage = ComputeSkillDamage(DamageMultiplier, true, bIsCrit);
	ApplyDamageFan(Radius, 180.f, Damage, bIsCrit);

	UE_LOG(LogTemp, Log, TEXT("[技能] 旋风斩！伤害 %.1f%s"), Damage, bIsCrit ? TEXT("（暴击！）") : TEXT(""));
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
