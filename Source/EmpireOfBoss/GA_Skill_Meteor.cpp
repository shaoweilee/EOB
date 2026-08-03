#include "GA_Skill_Meteor.h"

#include "TimerManager.h"

void UGA_Skill_Meteor::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo,
                                       const FGameplayAbilityActivationInfo ActivationInfo,
                                       const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!GetCursorGroundPoint(PendingStrikePoint))
	{
		if (AActor* Avatar = GetAvatarActorFromActorInfo()) PendingStrikePoint = Avatar->GetActorLocation();
	}

	// 延迟落下（技能实例是 InstancedPerActor，定时器回调安全）
	GetWorld()->GetTimerManager().SetTimer(StrikeTimerHandle, this, &UGA_Skill_Meteor::MeteorStrike,
	                                       ImpactDelay, false);

	UE_LOG(LogTemp, Log, TEXT("[技能] 陨石召唤中……落点 %s"), *PendingStrikePoint.ToString());
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}

void UGA_Skill_Meteor::MeteorStrike()
{
	bool bIsCrit = false;
	const float Damage = ComputeSkillDamage(DamageMultiplier, false, bIsCrit);
	ApplyDamageAtLocation(PendingStrikePoint, Radius, Damage, false);

	UE_LOG(LogTemp, Log, TEXT("[技能] 陨石落地！伤害 %.1f"), Damage);
}
