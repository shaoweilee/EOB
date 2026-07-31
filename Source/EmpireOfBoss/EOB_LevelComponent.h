#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EOB_LevelComponent.generated.h"

class UGameplayEffect;

/** 四维属性类型（加点按钮用） */
UENUM(BlueprintType)
enum class EEOBStatType : uint8
{
	Strength	UMETA(DisplayName = "力量"),
	Dexterity	UMETA(DisplayName = "敏捷"),
	Focus		UMETA(DisplayName = "专注"),
	Vitality	UMETA(DisplayName = "体力")
};

/**
 * M3a：经验/升级/属性点组件（TL2 规则）
 * 挂在英雄身上：杀怪得经验 → 升级送 5 属性点 + 1 技能点
 */
UCLASS(ClassGroup=(EOB), meta=(BlueprintSpawnableComponent))
class EMPIREOFBOSS_API UEOB_LevelComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEOB_LevelComponent();

	/** 当前等级 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EOB|Level")
	int32 Level = 1;

	/** 当前经验 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EOB|Level")
	float CurrentXP = 0.f;

	/** 可分配属性点（升级 +5） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EOB|Level")
	int32 StatPoints = 0;

	/** 技能点（升级 +1，M3b 技能树用，先攒着） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EOB|Level")
	int32 SkillPoints = 0;

	/** 升级所需经验 = Base * Level^Exponent（默认 100 * 等级^1.5） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Level")
	float BaseXPRequirement = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Level")
	float XPGrowthExponent = 1.5f;

	/** 升到下一级需要多少经验 */
	UFUNCTION(BlueprintPure, Category = "EOB|Level")
	float GetXPToNextLevel() const;

	/** 击杀奖励入口（怪物死亡时呼叫） */
	UFUNCTION(BlueprintCallable, Category = "EOB|Level")
	void AddExperience(float Amount);

	/** 分配 1 点属性点，返回是否成功（点数不足返回 false） */
	UFUNCTION(BlueprintCallable, Category = "EOB|Level")
	bool SpendStatPoint(EEOBStatType Stat);

	// ===================== GE 槽位（在英雄蓝图的组件默认值里配置） =====================

	/** 升级成长 GE：瞬间型，MaxHealth +10 / MaxMana +5 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Level|GE")
	TSubclassOf<UGameplayEffect> LevelUpGrowthGE;

	/** 加点 GE：瞬间型，每个含主属性 + 派生属性 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Level|GE")
	TSubclassOf<UGameplayEffect> StatGE_Strength;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Level|GE")
	TSubclassOf<UGameplayEffect> StatGE_Dexterity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Level|GE")
	TSubclassOf<UGameplayEffect> StatGE_Focus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Level|GE")
	TSubclassOf<UGameplayEffect> StatGE_Vitality;

private:
	/** 循环结算升级（一次吃大量经验可能连升几级） */
	void HandleLevelUps();

	/** 给挂主（英雄）打一个瞬间 GE */
	void ApplyGEToOwner(TSubclassOf<UGameplayEffect> GEClass);

	/** 通知 HUD 刷新经验条和角色面板 */
	void NotifyHUD();
};