#include "GA_Skill_FlameStorm.h"

void UGA_Skill_FlameStorm::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo* ActorInfo,
                                           const FGameplayAbilityActivationInfo ActivationInfo,
                                           const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FVector TargetPoint;
	if (!GetCursorGroundPoint(TargetPoint))
	{
		// 取不到鼠标点就砸自己脚下
		if (AActor* Avatar = GetAvatarActorFromActorInfo()) TargetPoint = Avatar->GetActorLocation();
	}

	bool bIsCrit = false;
	const float Damage = ComputeSkillDamage(DamageMultiplier, false, bIsCrit);
	ApplyDamageAtLocation(TargetPoint, Radius, Damage, false);

	UE_LOG(LogTemp, Log, TEXT("[技能] 烈焰风暴！落点 %s，伤害 %.1f"), *TargetPoint.ToString(), Damage);
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
