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
	 * 元属性 (Meta Attributes - 伤害结算池)
	 * ===================================================================== */
	UPROPERTY(BlueprintReadOnly, Category = "EOB|Attributes")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UEOB_AttributeSet, IncomingDamage);
};
