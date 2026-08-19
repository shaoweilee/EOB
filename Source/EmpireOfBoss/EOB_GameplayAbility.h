#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "EOB_GameplayAbility.generated.h"

class UGameplayEffect;

/**
 * M3b：主动技能基类。所有技能都继承它。
 * 提供：面向鼠标、鼠标落点、技能伤害公式、扇形/落点伤害、自我 Buff
 * M6b：范围伤害同时打进 MassBattle 实体体系（割草怪），六个技能自动获得打群怪能力
 */
UCLASS()
class EMPIREOFBOSS_API UEOB_GameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UEOB_GameplayAbility();

	/** 伤害 GE（技能蓝图默认值里统一填 GE_Damage，和普攻共用结算管线） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|Skill")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** 鼠标瞄准最大距离（厘米）。防止技能落在十万八千里外的背景山体上 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|Skill")
	float MaxAimDistance = 1500.f;

protected:
	/** 让释放者转向鼠标指向的地面点 */
	void RotateAvatarToCursor() const;

	/** 取鼠标指向的地面坐标（烈焰风暴/陨石的落点），失败返回 false */
	bool GetCursorGroundPoint(FVector& OutPoint) const;

	/**
	 * 技能伤害 = (攻击力 + 2~3随机) × 倍率 × (1 + 技能伤害加成/100)
	 * bCanCrit=true 时按暴击率掷点，暴击 ×暴击伤害/100
	 */
	float ComputeSkillDamage(float DamageMultiplier, bool bCanCrit, bool& bOutIsCrit) const;

	/** 以释放者为圆心的扇形伤害（HalfAngle>=180 即全圆），返回命中的敌人 */
	TArray<AActor*> ApplyDamageFan(float Radius, float HalfAngle, float Damage, bool bIsCrit) const;

	/** 以任意点为圆心的范围伤害（鼠标落点技能用），返回命中的敌人 */
	TArray<AActor*> ApplyDamageAtLocation(FVector Center, float Radius, float Damage, bool bIsCrit) const;

	/** 给自己挂 Buff GE（战吼用） */
	void ApplyBuffToSelf(TSubclassOf<UGameplayEffect> BuffGE) const;

	/**
	 * M6b：把同一份范围伤害同步打进 MassBattle 实体体系（割草怪）。
	 * 暴击已在 EOB 侧结算进 Damage 数值，插件侧暴击率强制为 0，防止双重暴击。
	 * 场上没有 Mass 敌人时自动空操作，开销可忽略。
	 */
	void ApplyDamageToMassEnemies(const FVector& Center, float Radius, float Damage) const;
};
