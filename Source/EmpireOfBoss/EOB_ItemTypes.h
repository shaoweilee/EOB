#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayEffectTypes.h"
#include "EOB_ItemTypes.generated.h"

class UGameplayEffect;
class UEOB_ItemDefinition;

/** 装备品质（TL2 配色） */
UENUM(BlueprintType)
enum class EEOBRarity : uint8
{
	White UMETA(DisplayName = "普通(白)"),
	Green UMETA(DisplayName = "魔法(绿)"),
	Blue  UMETA(DisplayName = "稀有(蓝)"),
	Gold  UMETA(DisplayName = "传说(金)")
};

/** 物品种类：装备（穿到装备面板）/ 背包（装进背包栏位 = 标签页） */
UENUM(BlueprintType)
enum class EEOBItemKind : uint8
{
	Equipment UMETA(DisplayName = "装备"),
	Bag       UMETA(DisplayName = "背包")
};

/** 装备槽位（Ring 只用于物品定义，穿戴时自动分配左右） */
UENUM(BlueprintType)
enum class EEOBEquipSlot : uint8
{
	Weapon    UMETA(DisplayName = "武器"),
	Shield    UMETA(DisplayName = "盾牌"),
	Helmet    UMETA(DisplayName = "头盔"),
	Chest     UMETA(DisplayName = "胸甲"),
	Gloves    UMETA(DisplayName = "手套"),
	Boots     UMETA(DisplayName = "鞋子"),
	Belt      UMETA(DisplayName = "腰带"),
	Amulet    UMETA(DisplayName = "项链"),
	Ring      UMETA(DisplayName = "戒指(通用)"),
	RingLeft  UMETA(DisplayName = "戒指(左)"),
	RingRight UMETA(DisplayName = "戒指(右)")
};

/** 物品分类（标签页"偏好"用；Uncategorized 放第一个，作为所有标签页的默认偏好） */
UENUM(BlueprintType)
enum class EEOBItemCategory : uint8
{
	Uncategorized UMETA(DisplayName = "任意"),
	Weapon        UMETA(DisplayName = "武器"),
	Shield        UMETA(DisplayName = "盾牌"),
	Helmet        UMETA(DisplayName = "头盔"),
	Chest         UMETA(DisplayName = "胸甲"),
	Gloves        UMETA(DisplayName = "手套"),
	Boots         UMETA(DisplayName = "鞋子"),
	Belt          UMETA(DisplayName = "腰带"),
	Amulet        UMETA(DisplayName = "项链"),
	Ring          UMETA(DisplayName = "戒指")
};

/** 装备槽位 → 物品分类（戒指的三个槽位都归为"戒指"） */
inline EEOBItemCategory EquipSlotToCategory(EEOBEquipSlot Slot)
{
	switch (Slot)
	{
	case EEOBEquipSlot::Weapon: return EEOBItemCategory::Weapon;
	case EEOBEquipSlot::Shield: return EEOBItemCategory::Shield;
	case EEOBEquipSlot::Helmet: return EEOBItemCategory::Helmet;
	case EEOBEquipSlot::Chest:  return EEOBItemCategory::Chest;
	case EEOBEquipSlot::Gloves: return EEOBItemCategory::Gloves;
	case EEOBEquipSlot::Boots:  return EEOBItemCategory::Boots;
	case EEOBEquipSlot::Belt:   return EEOBItemCategory::Belt;
	case EEOBEquipSlot::Amulet: return EEOBItemCategory::Amulet;
	case EEOBEquipSlot::Ring:
	case EEOBEquipSlot::RingLeft:
	case EEOBEquipSlot::RingRight: return EEOBItemCategory::Ring;
	default: return EEOBItemCategory::Uncategorized;
	}
}

/** 背包容量表：品质 = 格子数（白 8 / 绿 14 / 蓝 20 / 金 32） */
inline int32 GetBagCapacity(EEOBRarity Rarity)
{
	switch (Rarity)
	{
	case EEOBRarity::White: return 8;
	case EEOBRarity::Green: return 14;
	case EEOBRarity::Blue:  return 20;
	case EEOBRarity::Gold:  return 32;
	default: return 8;
	}
}

/** 一条确定数值的词缀（固定词缀和 roll 出来的随机词缀都用它存储） */
USTRUCT(BlueprintType)
struct FEOBAffixValue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Affix")
	FGameplayAttribute Attribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Affix")
	TEnumAsByte<EGameplayModOp::Type> ModifierOp = EGameplayModOp::Additive;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Affix")
	float Value = 0.f;

	/** 对应的 GE 蓝图类（每种属性一个，见配置步骤） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Affix")
	TSubclassOf<UGameplayEffect> GEClass;
};

/** 一件物品的运行时实例 = 定义 + 品质 + 随机词缀结果（背包物品没有词缀，品质决定容量） */
USTRUCT(BlueprintType)
struct FEOBItemInstance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "EOB|Item")
	TObjectPtr<UEOB_ItemDefinition> Definition = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "EOB|Item")
	EEOBRarity Rarity = EEOBRarity::White;

	/** 掉落时 roll 出的随机词缀（固定词缀在 Definition 里，不复制；背包恒为空） */
	UPROPERTY(BlueprintReadOnly, Category = "EOB|Item")
	TArray<FEOBAffixValue> RolledAffixes;

	/** 穿戴期间激活的 GE 句柄，卸下时用来移除（运行时数据） */
	TArray<FActiveGameplayEffectHandle> AppliedEffectHandles;

	bool IsValid() const { return Definition != nullptr; }
};