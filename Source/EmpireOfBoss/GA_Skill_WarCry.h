#pragma once

#include "CoreMinimal.h"
#include "EOB_GameplayAbility.h"
#include "GA_Skill_WarCry.generated.h"

class UGameplayEffect;

/** 战吼：自身 Buff（+攻击+护甲，8 秒，Buff GE 在蓝图默认值里配） */
UCLASS()
class EMPIREOFBOSS_API UGA_Skill_WarCry : public UEOB_GameplayAbility
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	/** Buff GE（蓝图默认值填 GE_Buff_WarCry：8秒，AttackPower+10，Armor+10） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|Skill")
	TSubclassOf<UGameplayEffect> BuffEffectClass;
};
