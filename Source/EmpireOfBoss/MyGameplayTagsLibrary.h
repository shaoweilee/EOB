#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "MyGameplayTagsLibrary.generated.h"

USTRUCT(BlueprintType)
struct FMyGameplayTags
{
	GENERATED_BODY()

public:
	// 初始化函数保持静态
	static void InitializeNativeTags();

	// =====================================================================
	// 将所有 Tag 变量改为静态 (static)
	// =====================================================================
	static FGameplayTag State_Rooted;
	static FGameplayTag State_Restricted_KnockedBack;
	static FGameplayTag Ability_Type_CCBreak;
	static FGameplayTag Data_Damage;
	// M1 新增：死亡状态标签（攻击逻辑/AI/UI 统一查询尸体用）
	static FGameplayTag State_Dead;
	// M2 新增：装备词缀数值的 SetByCaller 标签
	static FGameplayTag Data_AffixMagnitude;
	// M3a 新增：本次伤害是否暴击的 SetByCaller 标记
	static FGameplayTag Data_IsCrit;
	// M3b 新增：技能冷却标签 + 战吼 Buff 标签
	static FGameplayTag Cooldown_Skill_MeleeSmash;
	static FGameplayTag Cooldown_Skill_Whirlwind;
	static FGameplayTag Cooldown_Skill_Fireball;
	static FGameplayTag Cooldown_Skill_FlameStorm;
	static FGameplayTag Cooldown_Skill_Meteor;
	static FGameplayTag Cooldown_Skill_WarCry;
	static FGameplayTag Cooldown_Skill_Charge;
	static FGameplayTag Cooldown_Skill_Earthquake;
	static FGameplayTag Buff_WarCry;
};

UCLASS()
class EMPIREOFBOSS_API UMyGameplayTagsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 暴露给蓝图的函数，内部也要同步修改去掉 .Get()
	UFUNCTION(BlueprintPure, Category = "GameplayTags")
	static FGameplayTag GetStateRooted() { return FMyGameplayTags::State_Rooted; }
};
