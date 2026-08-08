#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EOB_ItemTypes.h"
#include "EOB_InventoryComponent.generated.h"

class UDataTable;
class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChangedSignature);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentChangedSignature);

class UGameplayEffect;

/**
 * M3a 补丁：四维词缀的派生加成规则
 * 当词缀加的属性 = SourceAttribute 时，额外施加 DerivedGE，
 * 数值 = 词缀数值 × Multiplier（TL2：装备给的属性同样提供派生加成）
 */
USTRUCT(BlueprintType)
struct FEOBDerivedAffixRule
{
	GENERATED_BODY()

	/** 词缀加的是这个属性时触发（如 EOB_AttributeSet.Strength） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Inventory")
	FGameplayAttribute SourceAttribute;

	/** 额外施加的 GE（Infinite，修饰符用 SetByCaller 读 Data.AffixMagnitude） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Inventory")
	TSubclassOf<UGameplayEffect> DerivedGE;

	/** 派生数值 = 词缀数值 × 倍数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Inventory")
	float Multiplier = 1.f;
};

/**
 * 背包与装备组件：挂在主角身上。
 * 背包 = 固定长度数组（空格的 Definition 为 nullptr）
 * 装备 = 槽位 → 物品实例 的映射
 * 穿上：词缀灌 Infinite GE；卸下：按句柄移除
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class EMPIREOFBOSS_API UEOB_InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEOB_InventoryComponent();
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Inventory", meta = (ClampMin = "1"))
	int32 InventorySize = 32;

	/** 词缀池（DataTable，行结构 EOBAffixTableRow） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Inventory")
	TObjectPtr<UDataTable> AffixTable;

	/** M3a 补丁：四维词缀的派生加成规则表（在英雄蓝图的组件默认值里配置 8 行） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Inventory")
	TArray<FEOBDerivedAffixRule> DerivedAffixRules;

	/** 背包格子（固定长度，UI 直接遍历它） */
	UPROPERTY(BlueprintReadOnly, Category = "EOB|Inventory")
	TArray<FEOBItemInstance> Items;

	/** 已穿戴装备：键 = 槽位 */
	UPROPERTY(BlueprintReadOnly, Category = "EOB|Inventory")
	TMap<EEOBEquipSlot, FEOBItemInstance> EquippedItems;

	/** 入包，返回槽位下标；背包满返回 INDEX_NONE */
	UFUNCTION(BlueprintCallable, Category = "EOB|Inventory")
	int32 AddItem(const FEOBItemInstance& Item);

	UFUNCTION(BlueprintCallable, Category = "EOB|Inventory")
	bool RemoveItemAt(int32 SlotIndex);

	/** 穿上背包里某格的装备（目标槽有货则原位交换） */
	UFUNCTION(BlueprintCallable, Category = "EOB|Inventory")
	bool EquipFromInventory(int32 SlotIndex);

	/** 卸下某槽位的装备回背包（背包满则失败） */
	UFUNCTION(BlueprintCallable, Category = "EOB|Inventory")
	bool UnequipItem(EEOBEquipSlot Slot);

	/** 生成装备实例：按品质定词缀数量，从词缀池按槽位过滤加权抽取 */
	UFUNCTION(BlueprintCallable, Category = "EOB|Item")
	FEOBItemInstance CreateItemInstance(UEOB_ItemDefinition* Def, EEOBRarity Rarity);

	/** 按权重掷品质 */
	static EEOBRarity RollRarity(float WhiteWeight, float GreenWeight, float BlueWeight, float GoldWeight);

	/** 背包变化事件（UI 绑定它来刷新格子） */
	UPROPERTY(BlueprintAssignable, Category = "EOB|Inventory")
	FOnInventoryChangedSignature OnInventoryChanged;

	/** 装备变化事件（UI 绑定它来刷新装备面板） */
	UPROPERTY(BlueprintAssignable, Category = "EOB|Inventory")
	FOnEquipmentChangedSignature OnEquipmentChanged;

private:
	void ApplyItemEffects(FEOBItemInstance& Item);
	void RemoveItemEffects(FEOBItemInstance& Item);
	UAbilitySystemComponent* GetOwnerASC() const;
	int32 FindEmptySlot() const;
};
