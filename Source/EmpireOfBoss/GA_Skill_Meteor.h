#pragma once

#include "CoreMinimal.h"
#include "EOB_GameplayAbility.h"
#include "GA_Skill_Meteor.generated.h"

/** 陨石：鼠标落点延迟 1 秒砸下，400 范围 ×5.0，不暴击（前置：烈焰风暴） */
UCLASS()
class EMPIREOFBOSS_API UGA_Skill_Meteor : public UEOB_GameplayAbility
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|Skill")
	float DamageMultiplier = 5.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|Skill")
	float Radius = 400.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|Skill")
	float ImpactDelay = 1.f;

private:
	FTimerHandle StrikeTimerHandle;
	FVector PendingStrikePoint = FVector::ZeroVector;
};
