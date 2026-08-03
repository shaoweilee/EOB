#pragma once

#include "CoreMinimal.h"
#include "EOB_GameplayAbility.h"
#include "GA_Skill_Fireball.generated.h"

class AEOB_Projectile;

/** 火球：朝鼠标方向发射投射物，命中爆炸小 AoE，×2.2，不暴击 */
UCLASS()
class EMPIREOFBOSS_API UGA_Skill_Fireball : public UEOB_GameplayAbility
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|Skill")
	float DamageMultiplier = 2.2f;

	/** 投射物类（技能蓝图默认值里填 BP_EOB_Projectile） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|Skill")
	TSubclassOf<AEOB_Projectile> ProjectileClass;
};
