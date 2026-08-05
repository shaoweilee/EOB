#pragma once

#include "CoreMinimal.h"
#include "EOB_GameplayAbility.h"
#include "GA_Skill_FlameStorm.generated.h"

/** 烈焰风暴：鼠标落点持续 AoE，总伤 ×2.5 均摊多跳，不暴击（前置：火球） */
UCLASS()
class EMPIREOFBOSS_API UGA_Skill_FlameStorm : public UEOB_GameplayAbility
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	/** 总伤害倍率（均摊到每一跳：每跳伤害 = 总伤 / 跳数） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|Skill")
	float DamageMultiplier = 2.5f;

	/** 风暴半径 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|Skill")
	float Radius = 300.f;

	/** 风暴持续秒数 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|Skill")
	float Duration = 4.f;

	/** 每跳间隔秒数 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|Skill")
	float TickInterval = 0.5f;
};
