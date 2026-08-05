#pragma once

#include "CoreMinimal.h"
#include "EOB_GameplayAbility.h"
#include "GA_Skill_Charge.generated.h"

/** 冲锋：向鼠标方向冲刺（上限 600），撞贴到敌人时结算路径伤害 ×1.5，可暴击（前置：战吼） */
UCLASS()
class EMPIREOFBOSS_API UGA_Skill_Charge : public UEOB_GameplayAbility
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
								 const FGameplayAbilityActorInfo* ActorInfo,
								 const FGameplayAbilityActivationInfo ActivationInfo,
								 const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|Skill")
	float DamageMultiplier = 1.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|Skill")
	float ChargeDistance = 600.f;
	/** 判定余量：以主角胶囊体为准向外扩多少厘米开始结算伤害。
	 *  0 = 两个胶囊严格贴上才结算；40 = 还差 40 厘米（视觉上"撞到脸上"）就结算 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|Skill")
	float PathHitRadius = 40.f;
	/** 冲刺时长（秒），越短越"炸" */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|Skill")
	float DashDuration = 0.15f;
};