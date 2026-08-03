#include "GA_Skill_WarCry.h"

void UGA_Skill_WarCry::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo,
                                       const FGameplayAbilityActivationInfo ActivationInfo,
                                       const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ApplyBuffToSelf(BuffEffectClass);

	UE_LOG(LogTemp, Log, TEXT("[技能] 战吼！攻击与护甲提升 8 秒"));
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
