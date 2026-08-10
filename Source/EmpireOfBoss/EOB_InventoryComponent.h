#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EOB_ItemTypes.h"
#include "EOB_InventoryComponent.generated.h"

class UDataTable;
class UAbilitySystemComponent;
class AEOB_PickupBase;

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
 * 一个背包栏位（= 一个标签页）：
 * 装上背包物品才激活；格子数 = 背包容量（由品质决定：白8/绿14/蓝20/金32）
 */
USTRUCT(BlueprintType)
struct FEOBInventoryTab
{
	GENERATED_BODY()

	/** 该栏位装备的背包物品（无效 = 栏位未激活，不能存放物品、不能被选中） */
	UPROPERTY(BlueprintReadOnly, Category = "EOB|Inventory")
	FEOBItemInstance Bag;

	/** 背包内的物品格子（长度 = 背包容量） */
	UPROPERTY(BlueprintReadOnly, Category = "EOB|Inventory")
	TArray<FEOBItemInstance> Slots;

	/** 该标签页的"偏好"分类（只是偏好，不限制存放） */
	UPROPERTY(BlueprintReadOnly, Category = "EOB|Inventory")
	EEOBItemCategory Preference = EEOBItemCategory::Uncategorized;
};

/**
 * 背包与装备组件：挂在主角身上。
 * 背包 = 12 个背包栏位（标签页），每个栏位装备一个背包物品，容量由背包品质决定。
 * 入包按"偏好页 → 未分类页 → 第一个空位页"的规则，只进入已激活（有背包）的页。
 * 装备 = 槽位 → 物品实例 的映射；穿上灌 Infinite GE，卸下按句柄移除。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class EMPIREOFBOSS_API UEOB_InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEOB_InventoryComponent();
	virtual void BeginPlay() override;

	/** 背包栏位（标签页）数量，固定 12 */
	static constexpr int32 NumTabs = 12;

	/** AddItem 返回值的编码：返回值 = 页号 × SlotEncodingBase + 页内格号 */
	static constexpr int32 SlotEncodingBase = 64;

	/** 词缀池（DataTable，行结构 EOBAffixTableRow） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Inventory")
	TObjectPtr<UDataTable> AffixTable;

	/** M3a 补丁：四维词缀的派生加成规则表（在英雄蓝图的组件默认值里配置 8 行） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Inventory")
	TArray<FEOBDerivedAffixRule> DerivedAffixRules;

	/** 开局赠送的背包定义（DA，Kind = 背包；在英雄蓝图的组件默认值里配置） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Inventory")
	TObjectPtr<UEOB_ItemDefinition> StartingBagDefinition;

	/** 开局赠送的背包数量（依次装进第 1、2……个栏位） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Inventory", meta = (ClampMin = "0", ClampMax = "12"))
	int32 StartingBagCount = 2;

	/** 开局赠送背包的品质（蓝 = 20 格） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Inventory")
	EEOBRarity StartingBagRarity = EEOBRarity::Blue;

	/** 丢弃到地上时生成的拾取物类（所有装备/背包共用，外观由物品 DA 的 WorldMesh 决定） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Inventory")
	TSubclassOf<AEOB_PickupBase> DropPickupClass;

	/** 12 个背包栏位（标签页） */
	UPROPERTY(BlueprintReadOnly, Category = "EOB|Inventory")
	TArray<FEOBInventoryTab> Tabs;

	/** 已穿戴装备：键 = 槽位 */
	UPROPERTY(BlueprintReadOnly, Category = "EOB|Inventory")
	TMap<EEOBEquipSlot, FEOBItemInstance> EquippedItems;

	// ===================== 入包 / 移除 =====================

	/** 入包：按归位规则找格（只进已激活的页），返回 页号×64+页内格号；无处可放返回 INDEX_NONE */
	UFUNCTION(BlueprintCallable, Category = "EOB|Inventory")
	int32 AddItem(const FEOBItemInstance& Item);

	UFUNCTION(BlueprintCallable, Category = "EOB|Inventory")
	bool RemoveItemAt(int32 TabIndex, int32 SlotInTab);

	// ===================== 装备穿脱 =====================

	/** 穿上背包里某格的"装备"（右键单击；目标槽有货则原位交换；背包物品请走 EquipBagFromInventory） */
	UFUNCTION(BlueprintCallable, Category = "EOB|Inventory")
	bool EquipFromInventory(int32 TabIndex, int32 SlotInTab);

	/** 卸下某槽位的装备回背包（按归位规则找格；无处可放则失败） */
	UFUNCTION(BlueprintCallable, Category = "EOB|Inventory")
	bool UnequipItem(EEOBEquipSlot Slot);

	// ===================== 背包（物品）穿脱 =====================

	/** 把背包里某格的"背包物品"装备到第一个空栏位（右键单击背包） */
	UFUNCTION(BlueprintCallable, Category = "EOB|Inventory")
	bool EquipBagFromInventory(int32 TabIndex, int32 SlotInTab);

	/** 把背包物品装进指定空栏位（该栏位已有背包请走 SwapBagOnTab） */
	UFUNCTION(BlueprintCallable, Category = "EOB|Inventory")
	bool EquipBagToTab(const FEOBItemInstance& Bag, int32 TabIndex);

	/** 换包：新包容量 >= 旧包才能替换，物品原样迁入新包，旧包回到新包原来所在的格子 */
	UFUNCTION(BlueprintCallable, Category = "EOB|Inventory")
	bool SwapBagOnTab(int32 TabIndex, int32 FromTab, int32 FromSlot);

	/** 卸下某栏位的背包回背包栏（仅当包内物品已清空；无处安放则失败并恢复） */
	UFUNCTION(BlueprintCallable, Category = "EOB|Inventory")
	bool UnequipBag(int32 TabIndex);

	// ===================== 标签页查询 =====================

	UFUNCTION(BlueprintCallable, Category = "EOB|Inventory")
	void SetTabPreference(int32 TabIndex, EEOBItemCategory NewPreference);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EOB|Inventory")
	EEOBItemCategory GetTabPreference(int32 TabIndex) const;

	/** 该栏位是否已激活（装有背包） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EOB|Inventory")
	bool IsTabActive(int32 TabIndex) const;

	/** 该栏位的容量（未激活为 0） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EOB|Inventory")
	int32 GetTabCapacity(int32 TabIndex) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EOB|Inventory")
	bool GetItemAt(int32 TabIndex, int32 SlotInTab, FEOBItemInstance& OutItem) const;

	/** 某页第一个空格的页内格号；页满或未激活返回 INDEX_NONE */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EOB|Inventory")
	int32 FindEmptySlotInTab(int32 TabIndex) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EOB|Inventory")
	bool TabHasEmptySlot(int32 TabIndex) const;

	// ===================== 移动/交换（抓取 & 拖拽用） =====================

	/** 两格交换（目标格为空 = 移动；目标格有货 = 互换） */
	UFUNCTION(BlueprintCallable, Category = "EOB|Inventory")
	bool MoveOrSwapItem(int32 FromTab, int32 FromSlot, int32 ToTab, int32 ToSlot);

	/** 把某格物品移到目标页的第一个空格；目标页满或未激活返回 false */
	UFUNCTION(BlueprintCallable, Category = "EOB|Inventory")
	bool MoveItemToTab(int32 FromTab, int32 FromSlot, int32 ToTab);

	// ===================== 丢弃 =====================

	/** 把某格物品丢到主人脚边的地上（保留品质和词缀），返回生成的拾取物 */
	UFUNCTION(BlueprintCallable, Category = "EOB|Inventory")
	AEOB_PickupBase* DropItemToWorld(int32 TabIndex, int32 SlotInTab);

	// ===================== 实例生成 =====================

	/** 生成物品实例：装备按品质 roll 词缀；背包不 roll 词缀（品质 = 容量） */
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

	/** 归位规则：偏好该分类的已激活页 → "未分类"偏好的已激活页 → 任意有空位的已激活页，返回 页号×64+页内格号 */
	int32 FindSlotForItem(const FEOBItemInstance& Item) const;

	bool IsValidTab(int32 TabIndex) const { return TabIndex >= 0 && TabIndex < NumTabs; }
};