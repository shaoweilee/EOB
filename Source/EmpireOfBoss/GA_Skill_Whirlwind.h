#pragma once

#include "CoreMinimal.h"
#include "EOB_GameplayAbility.h"
#include "GA_Skill_Whirlwind.generated.h"

/** 旋风斩：360° 全圆 AoE，×1.2，可暴击（前置：猛击） */
UCLASS()
class EMPIREOFBOSS_API UGA_Skill_Whirlwind : public UEOB_GameplayAbility
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|Skill")
	float DamageMultiplier = 1.2f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|Skill")
	float Radius = 300.f;
};
