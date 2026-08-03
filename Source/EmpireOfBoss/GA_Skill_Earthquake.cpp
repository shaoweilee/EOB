#include "GA_Skill_Earthquake.h"

#include "GameFramework/Character.h"

void UGA_Skill_Earthquake::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

	bool bIsCrit = false;
	const float Damage = ComputeSkillDamage(DamageMultiplier, true, bIsCrit);
	TArray<AActor*> HitActors = ApplyDamageFan(Radius, 180.f, Damage, bIsCrit);

	// 击退：命中的每个敌人沿远离英雄的方向抛出去
	if (Avatar)
	{
		for (AActor* HitActor : HitActors)
		{
			if (ACharacter* EC = Cast<ACharacter>(HitActor))
			{
				FVector Away = EC->GetActorLocation() - Avatar->GetActorLocation();
				Away.Z = 0.f;
				Away = Away.GetSafeNormal();
				EC->LaunchCharacter(Away * KnockbackStrength + FVector(0.f, 0.f, 300.f), true, true);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[技能] 大地震击！伤害 %.1f，击退 %d 个敌人%s"),
	       Damage, HitActors.Num(), bIsCrit ? TEXT("（暴击！）") : TEXT(""));
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
