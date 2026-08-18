#include "EOB_InventoryComponent.h"
#include "EOB_ItemDefinition.h"
#include "EOB_AffixTableRow.h"
#include "EOB_PickupBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/DataTable.h"
#include "MyGameplayTagsLibrary.h"

UEOB_InventoryComponent::UEOB_InventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UEOB_InventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// 初始化 12 个背包栏位（默认全部未激活）
	Tabs.Init(FEOBInventoryTab(), NumTabs);

	// 开局赠送背包：依次装进第 1、2……个栏位
	if (StartingBagDefinition)
	{
		for (int32 i = 0; i < StartingBagCount && i < NumTabs; ++i)
		{
			const FEOBItemInstance Bag = CreateItemInstance(StartingBagDefinition, StartingBagRarity);
			EquipBagToTab(Bag, i);
		}
		UE_LOG(LogTemp, Log, TEXT("[背包] 开局赠送 %d 个背包（品质 %s，每个 %d 格）"),
		       StartingBagCount,
		       *UEnum::GetValueAsString(StartingBagRarity),
		       GetBagCapacity(StartingBagRarity));
	}
	else
	{
		UE_LOG(LogTemp, Error,
		       TEXT(
			       "[背包] StartingBagDefinition 未配置！玩家开局没有任何背包，无法拾取物品。请到主角蓝图的 InventoryComponent 默认值里指定一个背包 DA（Kind=背包）。"
		       ));
	}

	// 🛡️ 防御：Live Coding 热重载偶尔会静默冲掉蓝图里配置的引用。
	//    词缀表丢了不会崩，但所有装备会退化成"只有基础属性"，必须有报警。
	if (!AffixTable)
	{
		UE_LOG(LogTemp, Error,
		       TEXT("[装备] InventoryComponent 的词缀表（AffixTable）未配置！所有装备将没有随机词缀。请到主角蓝图的组件默认值里指回 DT_Affixes。"));
	}
}

// ===================== 入包 / 移除 =====================

int32 UEOB_InventoryComponent::AddItem(const FEOBItemInstance& Item)
{
	if (!Item.IsValid()) return INDEX_NONE;

	const int32 Encoded = FindSlotForItem(Item);
	if (Encoded == INDEX_NONE) return INDEX_NONE;

	const int32 Tab = Encoded / SlotEncodingBase;
	const int32 Slot = Encoded % SlotEncodingBase;
	Tabs[Tab].Slots[Slot] = Item;
	OnInventoryChanged.Broadcast();
	return Encoded;
}

bool UEOB_InventoryComponent::RemoveItemAt(int32 TabIndex, int32 SlotInTab)
{
	if (!IsValidTab(TabIndex)) return false;
	FEOBInventoryTab& Tab = Tabs[TabIndex];
	if (!Tab.Slots.IsValidIndex(SlotInTab) || !Tab.Slots[SlotInTab].IsValid()) return false;

	Tab.Slots[SlotInTab] = FEOBItemInstance();
	OnInventoryChanged.Broadcast();
	return true;
}

// ===================== 装备穿脱 =====================

bool UEOB_InventoryComponent::EquipFromInventory(int32 TabIndex, int32 SlotInTab)
{
	if (!IsValidTab(TabIndex)) return false;
	FEOBInventoryTab& Tab = Tabs[TabIndex];
	if (!Tab.Slots.IsValidIndex(SlotInTab) || !Tab.Slots[SlotInTab].IsValid()) return false;

	FEOBItemInstance ItemToEquip = Tab.Slots[SlotInTab];

	// 背包不是装备，不能穿到装备面板上
	if (ItemToEquip.Definition->Kind == EEOBItemKind::Bag)
	{
		UE_LOG(LogTemp, Warning, TEXT("[背包] %s 是背包，请用 EquipBagFromInventory 装进背包栏位。"),
		       *ItemToEquip.Definition->ItemName.ToString());
		return false;
	}

	EEOBEquipSlot TargetSlot = ItemToEquip.Definition->EquipSlot;

	// 戒指自动分配：左空放左，右空放右，都满换左
	if (TargetSlot == EEOBEquipSlot::Ring)
	{
		if (!EquippedItems.Contains(EEOBEquipSlot::RingLeft)) TargetSlot = EEOBEquipSlot::RingLeft;
		else if (!EquippedItems.Contains(EEOBEquipSlot::RingRight)) TargetSlot = EEOBEquipSlot::RingRight;
		else TargetSlot = EEOBEquipSlot::RingLeft;
	}

	// 目标槽已有装备：卸下它，和背包格原位交换
	if (FEOBItemInstance* Existing = EquippedItems.Find(TargetSlot))
	{
		RemoveItemEffects(*Existing);
		Tab.Slots[SlotInTab] = *Existing;
		EquippedItems.Remove(TargetSlot);
	}
	else
	{
		Tab.Slots[SlotInTab] = FEOBItemInstance(); // 清空背包格
	}

	ApplyItemEffects(ItemToEquip);
	EquippedItems.Add(TargetSlot, ItemToEquip);

	OnInventoryChanged.Broadcast();
	OnEquipmentChanged.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("[装备] 已穿上 %s"), *ItemToEquip.Definition->ItemName.ToString());
	return true;
}

bool UEOB_InventoryComponent::UnequipItem(EEOBEquipSlot Slot)
{
	FEOBItemInstance* Existing = EquippedItems.Find(Slot);
	if (!Existing) return false;

	// 和拾取走同一条归位规则：偏好页 → 任意页 → 第一个空位页（只进已激活的页）
	const int32 Encoded = FindSlotForItem(*Existing);
	if (Encoded == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[装备] 背包已满，无法卸下！"));
		return false;
	}

	FEOBItemInstance Item = *Existing;
	RemoveItemEffects(Item);
	EquippedItems.Remove(Slot);
	Tabs[Encoded / SlotEncodingBase].Slots[Encoded % SlotEncodingBase] = Item;

	OnInventoryChanged.Broadcast();
	OnEquipmentChanged.Broadcast();
	return true;
}

// ===================== 装备面板抓取/拖拽原语（手持装备用） =====================

bool UEOB_InventoryComponent::CanEquipToSlot(const UEOB_ItemDefinition* Def, EEOBEquipSlot PanelSlot) const
{
	if (!Def) return false;
	if (Def->Kind != EEOBItemKind::Equipment) return false; // 背包不能穿到装备面板
	return EquipSlotsMatch(Def->EquipSlot, PanelSlot);
}

bool UEOB_InventoryComponent::GetEquippedItem(EEOBEquipSlot Slot, FEOBItemInstance& OutItem) const
{
	if (const FEOBItemInstance* Existing = EquippedItems.Find(Slot))
	{
		OutItem = *Existing;
		return true;
	}
	OutItem = FEOBItemInstance();
	return false;
}

bool UEOB_InventoryComponent::UnequipSlotToInstance(EEOBEquipSlot Slot, FEOBItemInstance& OutItem)
{
	FEOBItemInstance* Existing = EquippedItems.Find(Slot);
	if (!Existing) return false;

	OutItem = *Existing;
	RemoveItemEffects(OutItem); // 属性立即刷新，句柄清空
	EquippedItems.Remove(Slot);

	OnEquipmentChanged.Broadcast(); // 装备面板刷新为空槽；背包没动，不广播 OnInventoryChanged
	UE_LOG(LogTemp, Log, TEXT("[装备] 已拿起 %s（属性已移除，等待放下）"), *OutItem.Definition->ItemName.ToString());
	return true;
}

bool UEOB_InventoryComponent::EquipInstanceToSlot(const FEOBItemInstance& Item, EEOBEquipSlot Slot,
                                                  FEOBItemInstance& OutReplaced)
{
	OutReplaced = FEOBItemInstance();
	if (!CanEquipToSlot(Item.Definition, Slot)) return false;

	// 槽位有货：顶下旧装备，交还给调用方（UI 会把它抓在手里继续跟手）
	if (FEOBItemInstance* Existing = EquippedItems.Find(Slot))
	{
		OutReplaced = *Existing;
		RemoveItemEffects(OutReplaced);
		EquippedItems.Remove(Slot);
	}

	FEOBItemInstance ToApply = Item;
	ApplyItemEffects(ToApply);
	EquippedItems.Add(Slot, ToApply);

	OnEquipmentChanged.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("[装备] 已穿上 %s"), *Item.Definition->ItemName.ToString());
	return true;
}

bool UEOB_InventoryComponent::EquipFromInventoryToSlot(int32 TabIndex, int32 SlotInTab, EEOBEquipSlot PanelSlot)
{
	if (!IsTabActive(TabIndex)) return false;
	FEOBInventoryTab& Tab = Tabs[TabIndex];
	if (!Tab.Slots.IsValidIndex(SlotInTab) || !Tab.Slots[SlotInTab].IsValid()) return false;

	const FEOBItemInstance ItemToEquip = Tab.Slots[SlotInTab];
	if (!CanEquipToSlot(ItemToEquip.Definition, PanelSlot))
	{
		UE_LOG(LogTemp, Warning, TEXT("[装备] %s 不能穿到这个槽位。"), *ItemToEquip.Definition->ItemName.ToString());
		return false;
	}

	// 旧装备回到该背包格（设计文档：原有装备回到物品栏）；空槽则清空该格
	if (FEOBItemInstance* Existing = EquippedItems.Find(PanelSlot))
	{
		FEOBItemInstance Replaced = *Existing;
		RemoveItemEffects(Replaced);
		EquippedItems.Remove(PanelSlot);
		Tab.Slots[SlotInTab] = Replaced;
	}
	else
	{
		Tab.Slots[SlotInTab] = FEOBItemInstance();
	}

	FEOBItemInstance ToApply = ItemToEquip;
	ApplyItemEffects(ToApply);
	EquippedItems.Add(PanelSlot, ToApply);

	OnInventoryChanged.Broadcast();
	OnEquipmentChanged.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("[装备] 已把背包里的 %s 穿到指定槽位"), *ItemToEquip.Definition->ItemName.ToString());
	return true;
}

bool UEOB_InventoryComponent::SwapEquippedWithInventorySlot(EEOBEquipSlot Slot, int32 TabIndex, int32 SlotInTab,
                                                            const FEOBItemInstance& HeldItem,
                                                            FEOBItemInstance& OutDisplaced)
{
	OutDisplaced = FEOBItemInstance();
	if (!IsTabActive(TabIndex)) return false;
	FEOBInventoryTab& Tab = Tabs[TabIndex];
	if (!Tab.Slots.IsValidIndex(SlotInTab) || !Tab.Slots[SlotInTab].IsValid()) return false;
	if (!HeldItem.IsValid()) return false;

	const FEOBItemInstance GridItem = Tab.Slots[SlotInTab];
	if (!CanEquipToSlot(GridItem.Definition, Slot)) return false; // 背包格装备必须能穿到目标槽位才互换

	Tab.Slots[SlotInTab] = HeldItem; // 手持装备进背包格

	// 槽位若又被别的装备占着，占用者交还给 UI 继续抓在手里（保证不吞装备）
	if (FEOBItemInstance* Existing = EquippedItems.Find(Slot))
	{
		OutDisplaced = *Existing;
		RemoveItemEffects(OutDisplaced);
		EquippedItems.Remove(Slot);
	}

	FEOBItemInstance ToApply = GridItem;
	ApplyItemEffects(ToApply);
	EquippedItems.Add(Slot, ToApply);

	OnInventoryChanged.Broadcast();
	OnEquipmentChanged.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("[装备] 背包格的 %s 已穿上，手持装备放入背包格"), *GridItem.Definition->ItemName.ToString());
	return true;
}

bool UEOB_InventoryComponent::PlaceInstanceIntoSlot(const FEOBItemInstance& Item, int32 TabIndex, int32 SlotInTab)
{
	if (!Item.IsValid()) return false;
	if (!IsTabActive(TabIndex)) return false;
	FEOBInventoryTab& Tab = Tabs[TabIndex];
	if (!Tab.Slots.IsValidIndex(SlotInTab)) return false;
	if (Tab.Slots[SlotInTab].IsValid()) return false; // 只放空格，有货的格子请走别的路径

	Tab.Slots[SlotInTab] = Item;
	OnInventoryChanged.Broadcast();
	return true;
}

bool UEOB_InventoryComponent::PlaceInstanceIntoTab(const FEOBItemInstance& Item, int32 TabIndex)
{
	if (!Item.IsValid()) return false;

	const int32 EmptySlot = FindEmptySlotInTab(TabIndex); // 页未激活也会返回 INDEX_NONE
	if (EmptySlot == INDEX_NONE) return false;

	Tabs[TabIndex].Slots[EmptySlot] = Item;
	OnInventoryChanged.Broadcast();
	return true;
}

AEOB_PickupBase* UEOB_InventoryComponent::DropInstanceToWorld(const FEOBItemInstance& Item)
{
	if (!Item.IsValid()) return nullptr;
	return SpawnDropPickup(Item); // 物品不在任何容器里，生成完即可，无需广播
}

// ===================== 背包（物品）穿脱 =====================

bool UEOB_InventoryComponent::EquipBagFromInventory(int32 TabIndex, int32 SlotInTab)
{
	if (!IsValidTab(TabIndex)) return false;
	FEOBInventoryTab& SourceTab = Tabs[TabIndex];
	if (!SourceTab.Slots.IsValidIndex(SlotInTab) || !SourceTab.Slots[SlotInTab].IsValid()) return false;

	const FEOBItemInstance Bag = SourceTab.Slots[SlotInTab];
	if (Bag.Definition->Kind != EEOBItemKind::Bag)
	{
		UE_LOG(LogTemp, Warning, TEXT("[背包] %s 不是背包物品。"), *Bag.Definition->ItemName.ToString());
		return false;
	}

	// 找第一个空栏位
	for (int32 Tab = 0; Tab < NumTabs; ++Tab)
	{
		if (!Tabs[Tab].Bag.IsValid())
		{
			SourceTab.Slots[SlotInTab] = FEOBItemInstance(); // 先清空原格，再装包
			EquipBagToTab(Bag, Tab);
			UE_LOG(LogTemp, Log, TEXT("[背包] 已把 %s 装进第 %d 个栏位"),
			       *Bag.Definition->ItemName.ToString(), Tab + 1);
			return true;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[背包] 12 个栏位都装满了背包，装不上了！"));
	return false;
}

bool UEOB_InventoryComponent::EquipBagToTab(const FEOBItemInstance& Bag, int32 TabIndex)
{
	if (!IsValidTab(TabIndex)) return false;
	if (!Bag.IsValid() || !Bag.Definition || Bag.Definition->Kind != EEOBItemKind::Bag) return false;

	FEOBInventoryTab& Tab = Tabs[TabIndex];
	if (Tab.Bag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[背包] 第 %d 个栏位已有背包，请走 SwapBagOnTab 换包。"), TabIndex + 1);
		return false;
	}

	Tab.Bag = Bag;
	Tab.Slots.SetNum(GetBagCapacity(Bag.Rarity));
	OnInventoryChanged.Broadcast();
	return true;
}

bool UEOB_InventoryComponent::SwapBagOnTab(int32 TabIndex, int32 FromTab, int32 FromSlot)
{
	if (!IsValidTab(TabIndex) || !IsValidTab(FromTab)) return false;
	FEOBInventoryTab& Tab = Tabs[TabIndex];
	FEOBInventoryTab& SourceTab = Tabs[FromTab];

	if (!Tab.Bag.IsValid()) return false; // 空栏位直接走 EquipBagToTab
	if (!SourceTab.Slots.IsValidIndex(FromSlot) || !SourceTab.Slots[FromSlot].IsValid()) return false;

	const FEOBItemInstance NewBag = SourceTab.Slots[FromSlot];
	if (NewBag.Definition->Kind != EEOBItemKind::Bag) return false;

	const int32 NewCap = GetBagCapacity(NewBag.Rarity);
	const int32 OldCap = Tab.Slots.Num();
	if (NewCap < OldCap)
	{
		UE_LOG(LogTemp, Warning, TEXT("[背包] 新背包（%d 格）比旧背包（%d 格）小，不能替换！"), NewCap, OldCap);
		return false;
	}

	// 物品原样迁入新包，旧包回到新包原来所在的格子
	const FEOBItemInstance OldBag = Tab.Bag;
	const TArray<FEOBItemInstance> OldItems = Tab.Slots;

	Tab.Bag = NewBag;
	Tab.Slots.SetNum(NewCap);
	for (int32 i = 0; i < OldItems.Num(); ++i)
	{
		Tab.Slots[i] = OldItems[i];
	}

	SourceTab.Slots[FromSlot] = OldBag;

	OnInventoryChanged.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("[背包] 第 %d 个栏位换包：%s（%d 格）→ %s（%d 格），物品已迁入"),
	       TabIndex + 1,
	       *OldBag.Definition->ItemName.ToString(), OldCap,
	       *NewBag.Definition->ItemName.ToString(), NewCap);
	return true;
}

bool UEOB_InventoryComponent::EquipBagFromSlotToTab(int32 FromTab, int32 FromSlot, int32 ToTab)
{
	if (!IsValidTab(FromTab) || !IsValidTab(ToTab)) return false;
	FEOBInventoryTab& SourceTab = Tabs[FromTab];
	if (!SourceTab.Slots.IsValidIndex(FromSlot) || !SourceTab.Slots[FromSlot].IsValid()) return false;

	const FEOBItemInstance Bag = SourceTab.Slots[FromSlot];
	if (Bag.Definition->Kind != EEOBItemKind::Bag)
	{
		UE_LOG(LogTemp, Warning, TEXT("[背包] %s 不是背包物品，不能装进包裹栏位。"),
		       *Bag.Definition->ItemName.ToString());
		return false;
	}

	// 目标栏位已有背包：走换包（容量检查 + 物品迁移 + 旧包回源格）
	if (Tabs[ToTab].Bag.IsValid())
	{
		return SwapBagOnTab(ToTab, FromTab, FromSlot);
	}

	SourceTab.Slots[FromSlot] = FEOBItemInstance(); // 先清空原格，再装包
	EquipBagToTab(Bag, ToTab);
	UE_LOG(LogTemp, Log, TEXT("[背包] 已把 %s 装进第 %d 个栏位"),
	       *Bag.Definition->ItemName.ToString(), ToTab + 1);
	return true;
}

bool UEOB_InventoryComponent::SwapTabs(int32 TabA, int32 TabB)
{
	if (!IsValidTab(TabA) || !IsValidTab(TabB) || TabA == TabB) return false;
	if (!Tabs[TabA].Bag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[背包] 第 %d 个栏位没有背包，没法交换位置。"), TabA + 1);
		return false;
	}

	// 只交换"背包 + 包内物品"；偏好（Preference）是页的属性，留在原地不动
	Swap(Tabs[TabA].Bag, Tabs[TabB].Bag);
	Swap(Tabs[TabA].Slots, Tabs[TabB].Slots);

	OnInventoryChanged.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("[背包] 第 %d 个栏位和第 %d 个栏位的背包互换了位置"), TabA + 1, TabB + 1);
	return true;
}

bool UEOB_InventoryComponent::UnequipBagToSlot(int32 BagTab, int32 ToTab, int32 ToSlot)
{
	if (!IsValidTab(BagTab) || !IsTabActive(ToTab)) return false;
	FEOBInventoryTab& Source = Tabs[BagTab];
	if (!Source.Bag.IsValid()) return false;

	// 只有包内物品全部清空才能卸
	for (const FEOBItemInstance& SlotItem : Source.Slots)
	{
		if (SlotItem.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[背包] 第 %d 个栏位的背包里还有物品，清空后才能卸下！"), BagTab + 1);
			return false;
		}
	}

	FEOBInventoryTab& Target = Tabs[ToTab];
	if (!Target.Slots.IsValidIndex(ToSlot)) return false;
	if (Target.Slots[ToSlot].IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[背包] 目标格子已有物品，卸下的背包只能放到空格。"));
		return false;
	}

	Target.Slots[ToSlot] = Source.Bag;
	Source.Bag = FEOBItemInstance();
	Source.Slots.Empty();

	OnInventoryChanged.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("[背包] 已把第 %d 个栏位的背包卸到第 %d 页第 %d 格"),
	       BagTab + 1, ToTab + 1, ToSlot + 1);
	return true;
}

bool UEOB_InventoryComponent::UnequipBag(int32 TabIndex)
{
	if (!IsValidTab(TabIndex)) return false;
	FEOBInventoryTab& Tab = Tabs[TabIndex];
	if (!Tab.Bag.IsValid()) return false;

	// 只有包内物品全部清空才能卸
	for (const FEOBItemInstance& SlotItem : Tab.Slots)
	{
		if (SlotItem.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[背包] 第 %d 个栏位的背包里还有物品，清空后才能卸下！"), TabIndex + 1);
			return false;
		}
	}

	const FEOBItemInstance Bag = Tab.Bag;
	const int32 Capacity = Tab.Slots.Num();

	// 先摘下，再按归位规则找格；找不到就恢复
	Tab.Bag = FEOBItemInstance();
	Tab.Slots.Empty();

	if (AddItem(Bag) == INDEX_NONE)
	{
		Tab.Bag = Bag;
		Tab.Slots.SetNum(Capacity);
		OnInventoryChanged.Broadcast();
		UE_LOG(LogTemp, Warning, TEXT("[背包] 其他背包都满了，卸下的背包无处可放！"));
		return false;
	}

	OnInventoryChanged.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("[背包] 已卸下第 %d 个栏位的 %s"), TabIndex + 1, *Bag.Definition->ItemName.ToString());
	return true;
}

// ===================== 标签页查询 =====================

void UEOB_InventoryComponent::SetTabPreference(int32 TabIndex, EEOBItemCategory NewPreference)
{
	if (!IsValidTab(TabIndex)) return;
	Tabs[TabIndex].Preference = NewPreference;
	// 偏好变化不改变物品位置，只影响以后的归位；无需广播
}

EEOBItemCategory UEOB_InventoryComponent::GetTabPreference(int32 TabIndex) const
{
	if (!IsValidTab(TabIndex)) return EEOBItemCategory::Uncategorized;
	return Tabs[TabIndex].Preference;
}

bool UEOB_InventoryComponent::IsTabActive(int32 TabIndex) const
{
	return IsValidTab(TabIndex) && Tabs[TabIndex].Bag.IsValid();
}

int32 UEOB_InventoryComponent::GetTabCapacity(int32 TabIndex) const
{
	if (!IsTabActive(TabIndex)) return 0;
	return Tabs[TabIndex].Slots.Num();
}

bool UEOB_InventoryComponent::GetTabBag(int32 TabIndex, FEOBItemInstance& OutBag) const
{
	if (!IsValidTab(TabIndex)) return false;
	OutBag = Tabs[TabIndex].Bag;
	return Tabs[TabIndex].Bag.IsValid();
}

bool UEOB_InventoryComponent::GetItemAt(int32 TabIndex, int32 SlotInTab, FEOBItemInstance& OutItem) const
{
	if (!IsValidTab(TabIndex)) return false;
	const FEOBInventoryTab& Tab = Tabs[TabIndex];
	if (!Tab.Slots.IsValidIndex(SlotInTab)) return false;
	OutItem = Tab.Slots[SlotInTab];
	return true;
}

int32 UEOB_InventoryComponent::FindEmptySlotInTab(int32 TabIndex) const
{
	if (!IsTabActive(TabIndex)) return INDEX_NONE;

	const TArray<FEOBItemInstance>& Slots = Tabs[TabIndex].Slots;
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		if (!Slots[i].IsValid()) return i;
	}
	return INDEX_NONE;
}

bool UEOB_InventoryComponent::TabHasEmptySlot(int32 TabIndex) const
{
	return FindEmptySlotInTab(TabIndex) != INDEX_NONE;
}

// ===================== 移动/交换 =====================

bool UEOB_InventoryComponent::MoveOrSwapItem(int32 FromTab, int32 FromSlot, int32 ToTab, int32 ToSlot)
{
	if (!IsTabActive(FromTab) || !IsTabActive(ToTab)) return false;
	FEOBInventoryTab& SourceTab = Tabs[FromTab];
	FEOBInventoryTab& TargetTab = Tabs[ToTab];
	if (!SourceTab.Slots.IsValidIndex(FromSlot) || !TargetTab.Slots.IsValidIndex(ToSlot)) return false;
	if (FromTab == ToTab && FromSlot == ToSlot) return false;
	if (!SourceTab.Slots[FromSlot].IsValid()) return false;

	// 目标格有货 = 互换；目标格为空 = 移动（空实例换过去等价于清空原格）
	const FEOBItemInstance Temp = SourceTab.Slots[FromSlot];
	SourceTab.Slots[FromSlot] = TargetTab.Slots[ToSlot];
	TargetTab.Slots[ToSlot] = Temp;

	OnInventoryChanged.Broadcast();
	return true;
}

bool UEOB_InventoryComponent::MoveItemToTab(int32 FromTab, int32 FromSlot, int32 ToTab)
{
	if (!IsTabActive(FromTab) || !IsTabActive(ToTab)) return false;
	FEOBInventoryTab& SourceTab = Tabs[FromTab];
	if (!SourceTab.Slots.IsValidIndex(FromSlot) || !SourceTab.Slots[FromSlot].IsValid()) return false;

	const int32 EmptySlot = FindEmptySlotInTab(ToTab);
	if (EmptySlot == INDEX_NONE) return false;

	Tabs[ToTab].Slots[EmptySlot] = SourceTab.Slots[FromSlot];
	SourceTab.Slots[FromSlot] = FEOBItemInstance();

	OnInventoryChanged.Broadcast();
	return true;
}

// ===================== 丢弃 =====================

AEOB_PickupBase* UEOB_InventoryComponent::DropItemToWorld(int32 TabIndex, int32 SlotInTab)
{
	if (!IsTabActive(TabIndex)) return nullptr;
	FEOBInventoryTab& Tab = Tabs[TabIndex];
	if (!Tab.Slots.IsValidIndex(SlotInTab) || !Tab.Slots[SlotInTab].IsValid()) return nullptr;

	const FEOBItemInstance Item = Tab.Slots[SlotInTab];
	AEOB_PickupBase* Pickup = SpawnDropPickup(Item);
	if (!Pickup) return nullptr;

	RemoveItemAt(TabIndex, SlotInTab); // 内部会广播 OnInventoryChanged
	UE_LOG(LogTemp, Log, TEXT("[背包] 已把 %s 丢到地上"), *Item.Definition->ItemName.ToString());
	return Pickup;
}

// ===================== 实例生成 =====================

FEOBItemInstance UEOB_InventoryComponent::CreateItemInstance(UEOB_ItemDefinition* Def, EEOBRarity Rarity)
{
	FEOBItemInstance NewItem;
	NewItem.Definition = Def;
	NewItem.Rarity = Rarity;

	if (!Def) return NewItem;

	// 背包不 roll 词缀：品质即容量
	if (Def->Kind == EEOBItemKind::Bag)
	{
		UE_LOG(LogTemp, Log, TEXT("[背包] 生成背包【%s】品质=%s，容量 %d 格"),
		       *Def->ItemName.ToString(),
		       *UEnum::GetValueAsString(Rarity),
		       GetBagCapacity(Rarity));
		return NewItem;
	}

	if (!AffixTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[词缀] 生成【%s】品质=%s，词缀表=❌未配置！"),
		       *Def->ItemName.ToString(),
		       *UEnum::GetValueAsString(Rarity));
		return NewItem;
	}

	// 1. 品质决定词缀数量（TL2 手感：白 0 / 绿 1 / 蓝 2~3 / 金 4~5）
	int32 AffixCount = 0;
	switch (Rarity)
	{
	case EEOBRarity::Green: AffixCount = 1;
		break;
	case EEOBRarity::Blue: AffixCount = FMath::RandRange(2, 3);
		break;
	case EEOBRarity::Gold: AffixCount = FMath::RandRange(4, 5);
		break;
	default: break;
	}
	if (AffixCount <= 0) return NewItem;

	// 2. 按装备槽位过滤词缀池
	static const FString ContextString(TEXT("AffixRoll"));
	TArray<FEOBAffixTableRow*> AllRows;
	AffixTable->GetAllRows<FEOBAffixTableRow>(ContextString, AllRows);

	TArray<const FEOBAffixTableRow*> Candidates;
	for (const FEOBAffixTableRow* Row : AllRows)
	{
		if (Row && Row->GEClass && Row->ApplicableSlots.Contains(Def->EquipSlot))
		{
			Candidates.Add(Row);
		}
	}

	// 3. 加权抽取，抽中即移出候选（同一件装备不出重复词缀）
	for (int32 i = 0; i < AffixCount && Candidates.Num() > 0; ++i)
	{
		float TotalWeight = 0.f;
		for (const FEOBAffixTableRow* C : Candidates) TotalWeight += FMath::Max(0.f, C->Weight);
		if (TotalWeight <= 0.f) break;

		float Roll = FMath::FRand() * TotalWeight;
		int32 Picked = 0;
		for (int32 j = 0; j < Candidates.Num(); ++j)
		{
			Roll -= FMath::Max(0.f, Candidates[j]->Weight);
			if (Roll <= 0.f)
			{
				Picked = j;
				break;
			}
		}

		const FEOBAffixTableRow* Row = Candidates[Picked];

		FEOBAffixValue Affix;
		Affix.Attribute = Row->TargetAttribute;
		Affix.ModifierOp = EGameplayModOp::Additive;
		Affix.Value = FMath::FRandRange(Row->MinValue, Row->MaxValue);
		Affix.GEClass = Row->GEClass;
		NewItem.RolledAffixes.Add(Affix);

		Candidates.RemoveAt(Picked);
	}

	UE_LOG(LogTemp, Log, TEXT("[词缀] 生成【%s】品质=%s，roll 出 %d 条词缀"),
	       *Def->ItemName.ToString(),
	       *UEnum::GetValueAsString(Rarity),
	       NewItem.RolledAffixes.Num());

	return NewItem;
}

EEOBRarity UEOB_InventoryComponent::RollRarity(float WhiteWeight, float GreenWeight, float BlueWeight, float GoldWeight)
{
	const float Total = WhiteWeight + GreenWeight + BlueWeight + GoldWeight;
	if (Total <= 0.f) return EEOBRarity::White;

	float Roll = FMath::FRand() * Total;
	if ((Roll -= WhiteWeight) < 0.f) return EEOBRarity::White;
	if ((Roll -= GreenWeight) < 0.f) return EEOBRarity::Green;
	if ((Roll -= BlueWeight) < 0.f) return EEOBRarity::Blue;
	return EEOBRarity::Gold;
}

// ===================== 内部工具 =====================

void UEOB_InventoryComponent::ApplyItemEffects(FEOBItemInstance& Item)
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC || !Item.IsValid()) return;

	// 固定词缀 + 随机词缀一起灌
	TArray<FEOBAffixValue> AllAffixes = Item.Definition->BaseAffixes;
	AllAffixes.Append(Item.RolledAffixes);

	// 小工具：打一个 GE，数值走 SetByCaller，句柄记到装备上（卸下时统一移除）
	auto ApplyAffixGE = [&](TSubclassOf<UGameplayEffect> GEClass, float Magnitude)
	{
		if (!GEClass) return;

		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(Item.Definition);
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GEClass, 1.f, Context);
		if (!SpecHandle.IsValid()) return;

		// 🌟 把数值塞进 SetByCaller，GE 蓝图里用 Data.AffixMagnitude 读
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(FMyGameplayTags::Data_AffixMagnitude, Magnitude);

		const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		if (Handle.WasSuccessfullyApplied())
		{
			Item.AppliedEffectHandles.Add(Handle);
		}
	};

	UE_LOG(LogTemp, Warning, TEXT("════ [装备] %s（%d 条词缀）════"),
	       *Item.Definition->ItemName.ToString(), AllAffixes.Num());

	for (const FEOBAffixValue& Affix : AllAffixes)
	{
		// 1. 词缀本体
		ApplyAffixGE(Affix.GEClass, Affix.Value);

		UE_LOG(LogTemp, Warning, TEXT("      词缀: %s +%.1f"), *Affix.Attribute.GetName(), Affix.Value);

		// 2. 🌟 M3a 补丁：TL2 还原——词缀加四维时，同步施加派生加成
		for (const FEOBDerivedAffixRule& Rule : DerivedAffixRules)
		{
			if (Rule.SourceAttribute == Affix.Attribute)
			{
				ApplyAffixGE(Rule.DerivedGE, Affix.Value * Rule.Multiplier);
			}
		}
	}
}

void UEOB_InventoryComponent::RemoveItemEffects(FEOBItemInstance& Item)
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC) return;

	for (const FActiveGameplayEffectHandle& Handle : Item.AppliedEffectHandles)
	{
		ASC->RemoveActiveGameplayEffect(Handle);
	}
	Item.AppliedEffectHandles.Empty();
}

UAbilitySystemComponent* UEOB_InventoryComponent::GetOwnerASC() const
{
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		return ASI->GetAbilitySystemComponent();
	}
	return nullptr;
}

int32 UEOB_InventoryComponent::FindSlotForItem(const FEOBItemInstance& Item) const
{
	if (!Item.IsValid() || !Item.Definition) return INDEX_NONE;

	// 背包物品没有分类归属，按"任意"参与归位
	const EEOBItemCategory Category = (Item.Definition->Kind == EEOBItemKind::Bag)
		                                  ? EEOBItemCategory::Uncategorized
		                                  : EquipSlotToCategory(Item.Definition->EquipSlot);

	// 1. 偏好该分类的已激活页（按页号顺序取第一个有空位的）
	if (Category != EEOBItemCategory::Uncategorized)
	{
		for (int32 Tab = 0; Tab < NumTabs; ++Tab)
		{
			if (IsTabActive(Tab) && Tabs[Tab].Preference == Category)
			{
				const int32 Slot = FindEmptySlotInTab(Tab);
				if (Slot != INDEX_NONE) return Tab * SlotEncodingBase + Slot;
			}
		}
	}

	// 2. "任意"偏好的已激活页
	for (int32 Tab = 0; Tab < NumTabs; ++Tab)
	{
		if (IsTabActive(Tab) && Tabs[Tab].Preference == EEOBItemCategory::Uncategorized)
		{
			const int32 Slot = FindEmptySlotInTab(Tab);
			if (Slot != INDEX_NONE) return Tab * SlotEncodingBase + Slot;
		}
	}

	// 3. 任意有空位的已激活页
	for (int32 Tab = 0; Tab < NumTabs; ++Tab)
	{
		const int32 Slot = FindEmptySlotInTab(Tab);
		if (Slot != INDEX_NONE) return Tab * SlotEncodingBase + Slot;
	}

	return INDEX_NONE;
}

AEOB_PickupBase* UEOB_InventoryComponent::SpawnDropPickup(const FEOBItemInstance& Item)
{
	if (!DropPickupClass)
	{
		UE_LOG(LogTemp, Error,
		       TEXT("[背包] DropPickupClass 未配置！请到主角蓝图的 InventoryComponent 默认值里指定一个装备拾取物类（如 BP_Pickup_Sword）。"));
		return nullptr;
	}
	if (!Item.IsValid()) return nullptr;

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return nullptr;
	UWorld* World = OwnerActor->GetWorld();
	if (!World) return nullptr;

	// 落点：主人面前 80cm，向下射线找地面（和怪物掉落同一通道）
	const FVector TraceStart = OwnerActor->GetActorLocation()
		+ OwnerActor->GetActorForwardVector() * 80.f
		+ FVector(0.f, 0.f, 100.f);
	const FVector TraceEnd = TraceStart - FVector(0.f, 0.f, 600.f);

	FVector SpawnLoc = TraceStart;
	FHitResult Hit;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);
	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_GameTraceChannel2, QueryParams))
	{
		SpawnLoc = Hit.ImpactPoint + FVector(0.f, 0.f, 2.f);
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AEOB_PickupBase* Pickup = World->SpawnActor<AEOB_PickupBase>(
		DropPickupClass, SpawnLoc, FRotator::ZeroRotator, Params);
	if (!Pickup) return nullptr;

	Pickup->SetDroppedItemDefinition(Item.Definition); // 套用 DA 外观（网格/旋转/缩放/落地）
	Pickup->SetPresetInstance(Item); // 记住已掷出的品质 + 词缀，再捡起不重新掷
	return Pickup;
}
