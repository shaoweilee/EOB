#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "EOB_AttributeSet.generated.h" // 🌟 必须与文件名 EOB_AttributeSet 严格一致

// 🌟 GAS 标准魔术宏：自动生成每个属性的 Getter、Setter、Init 和 Property 函数
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class EMPIREOFBOSS_API UEOB_AttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UEOB_AttributeSet();

	// 核心生命周期函数
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	/* =====================================================================
	 * 核心生存属性 (Vital Attributes)
	 * ===================================================================== */
	UPROPERTY(BlueprintReadOnly, Category = "EOB|Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UEOB_AttributeSet, Health);

	UPROPERTY(BlueprintReadOnly, Category = "EOB|Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UEOB_AttributeSet, MaxHealth);

	UPROPERTY(BlueprintReadOnly, Category = "EOB|Attributes")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UEOB_AttributeSet, Mana);

	UPROPERTY(BlueprintReadOnly, Category = "EOB|Attributes")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UEOB_AttributeSet, MaxMana);

	/* =====================================================================
	 * 防御属性 (Defense Attributes)
	 * ===================================================================== */
	UPROPERTY(BlueprintReadOnly, Category = "EOB|Attributes")
	FGameplayAttributeData Armor;
	ATTRIBUTE_ACCESSORS(UEOB_AttributeSet, Armor);

	/* =====================================================================
 * M2 新增：四维主属性 + 攻击力（装备词缀的修改目标）
 * ===================================================================== */
	UPROPERTY(BlueprintReadOnly, Category = "EOB|Attributes")
	FGameplayAttributeData Strength;
	ATTRIBUTE_ACCESSORS(UEOB_AttributeSet, Strength);

	UPROPERTY(BlueprintReadOnly, Category = "EOB|Attributes")
	FGameplayAttributeData Dexterity;
	ATTRIBUTE_ACCESSORS(UEOB_AttributeSet, Dexterity);

	UPROPERTY(BlueprintReadOnly, Category = "EOB|Attributes")
	FGameplayAttributeData Focus;
	ATTRIBUTE_ACCESSORS(UEOB_AttributeSet, Focus);

	UPROPERTY(BlueprintReadOnly, Category = "EOB|Attributes")
	FGameplayAttributeData Vitality;
	ATTRIBUTE_ACCESSORS(UEOB_AttributeSet, Vitality);

	UPROPERTY(BlueprintReadOnly, Category = "EOB|Attributes")
	FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS(UEOB_AttributeSet, AttackPower);

	/* =====================================================================
	 * M3a 新增：战斗派生属性（TL2 还原版）
	 * ===================================================================== */
	/** 暴击率（%），基础 5，敏捷加点提升 */
	UPROPERTY(BlueprintReadOnly, Category = "EOB|Attributes")
	FGameplayAttributeData CritChance;
	ATTRIBUTE_ACCESSORS(UEOB_AttributeSet, CritChance);

	/** 暴击伤害（%），150 = 1.5 倍，力量加点提升 */
	UPROPERTY(BlueprintReadOnly, Category = "EOB|Attributes")
	FGameplayAttributeData CritDamage;
	ATTRIBUTE_ACCESSORS(UEOB_AttributeSet, CritDamage);

	/** 闪避率（%），基础 0，敏捷加点提升 */
	UPROPERTY(BlueprintReadOnly, Category = "EOB|Attributes")
	FGameplayAttributeData DodgeChance;
	ATTRIBUTE_ACCESSORS(UEOB_AttributeSet, DodgeChance);

	/** 技能伤害加成（%），专注加点提升，M3b 技能用 */
	UPROPERTY(BlueprintReadOnly, Category = "EOB|Attributes")
	FGameplayAttributeData SkillDamageBonus;
	ATTRIBUTE_ACCESSORS(UEOB_AttributeSet, SkillDamageBonus);

	/* =====================================================================
	 * 元属性 (Meta Attributes - 伤害结算池)
	 * ===================================================================== */
	UPROPERTY(BlueprintReadOnly, Category = "EOB|Attributes")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UEOB_AttributeSet, IncomingDamage);
};
