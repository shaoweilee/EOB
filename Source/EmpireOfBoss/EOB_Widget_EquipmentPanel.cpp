#include "EOB_Widget_EquipmentPanel.h"
#include "EOB_InventoryComponent.h"
#include "EOB_Widget_Inventory.h"
#include "EOB_Widget_InventorySlot.h"
#include "EmpireOfBossCharacter.h"
#include "InputCoreTypes.h"

void UEOB_Widget_EquipmentPanel::NativeConstruct()
{
	Super::NativeConstruct();

	if (AEmpireOfBossCharacter* Hero = Cast<AEmpireOfBossCharacter>(GetOwningPlayerPawn()))
	{
		RefInventory = Hero->InventoryComponent;
		if (RefInventory)
		{
			RefInventory->OnEquipmentChanged.AddDynamic(this, &UEOB_Widget_EquipmentPanel::RefreshEquip);
			RefInventory->OnInventoryChanged.AddDynamic(this, &UEOB_Widget_EquipmentPanel::RefreshEquip);
		}
	}

	// 登记"面板范围"：手持物落在这个范围内 = 面板空隙 = 无反应，不会误丢到地上。
	// 优先用可选绑定的 PanelBounds（底图/外框）；没绑就用根控件——
	// 根控件若铺满整张设计画布，几何范围就是全屏，丢弃判定会失灵（日志会提醒）。
	UWidget* BoundsWidget = PanelBounds ? ToRawPtr(PanelBounds) : GetRootWidget();
	if (BoundsWidget)
	{
		RegisteredBounds = BoundsWidget;
		UEOB_Widget_Inventory::GetAllPanelBoundsWidgets().AddUnique(BoundsWidget);
		if (!PanelBounds)
		{
			UE_LOG(LogTemp, Warning,
			       TEXT("[装备 UI] WBP_EquipmentPanel 未绑定 PanelBounds：面板空隙判定将使用根控件大小。"
				       "若根控件是全屏，请把包住面板的底图/外框命名为 PanelBounds 并勾选“是变量”。"));
		}
	}

	// 给蓝图上手动摆好的 10 个格子分发各自的槽位身份 + 手势总管。
	// 注意：如果本面板先于背包面板构造，GetInstance() 会拿到空——没关系，
	// 格子在左键按下的瞬间会现场补取（见 EOB_Widget_InventorySlot::NativeOnPreviewMouseButtonDown）。
	UEOB_Widget_Inventory* GestureHub = UEOB_Widget_Inventory::GetInstance();

	Slot_Weapon->InitEquipmentSlot(RefInventory, EEOBEquipSlot::Weapon, GestureHub);
	Slot_Shield->InitEquipmentSlot(RefInventory, EEOBEquipSlot::Shield, GestureHub);
	Slot_Helmet->InitEquipmentSlot(RefInventory, EEOBEquipSlot::Helmet, GestureHub);
	Slot_Chest->InitEquipmentSlot(RefInventory, EEOBEquipSlot::Chest, GestureHub);
	Slot_Gloves->InitEquipmentSlot(RefInventory, EEOBEquipSlot::Gloves, GestureHub);
	Slot_Boots->InitEquipmentSlot(RefInventory, EEOBEquipSlot::Boots, GestureHub);
	Slot_Belt->InitEquipmentSlot(RefInventory, EEOBEquipSlot::Belt, GestureHub);
	Slot_Amulet->InitEquipmentSlot(RefInventory, EEOBEquipSlot::Amulet, GestureHub);
	Slot_RingLeft->InitEquipmentSlot(RefInventory, EEOBEquipSlot::RingLeft, GestureHub);
	Slot_RingRight->InitEquipmentSlot(RefInventory, EEOBEquipSlot::RingRight, GestureHub);

	RefreshEquip();
}

void UEOB_Widget_EquipmentPanel::NativeDestruct()
{
	const UWidget* MyBounds = RegisteredBounds.Get();
	UEOB_Widget_Inventory::GetAllPanelBoundsWidgets().RemoveAll(
		[MyBounds](const TWeakObjectPtr<UWidget>& Ptr)
		{
			return !Ptr.IsValid() || Ptr.Get() == MyBounds;
		});
	RegisteredBounds = nullptr;

	Super::NativeDestruct();
}

void UEOB_Widget_EquipmentPanel::UpdateOneSlot(UEOB_Widget_InventorySlot* Widget, EEOBEquipSlot SlotId) const
{
	if (!Widget) return;

	if (RefInventory)
	{
		if (const FEOBItemInstance* Found = RefInventory->EquippedItems.Find(SlotId))
		{
			Widget->UpdateSlot(*Found);
			return;
		}
	}
	Widget->UpdateSlot(FEOBItemInstance()); // 空槽
}

void UEOB_Widget_EquipmentPanel::RefreshEquip()
{
	UpdateOneSlot(Slot_Weapon, EEOBEquipSlot::Weapon);
	UpdateOneSlot(Slot_Shield, EEOBEquipSlot::Shield);
	UpdateOneSlot(Slot_Helmet, EEOBEquipSlot::Helmet);
	UpdateOneSlot(Slot_Chest, EEOBEquipSlot::Chest);
	UpdateOneSlot(Slot_Gloves, EEOBEquipSlot::Gloves);
	UpdateOneSlot(Slot_Boots, EEOBEquipSlot::Boots);
	UpdateOneSlot(Slot_Belt, EEOBEquipSlot::Belt);
	UpdateOneSlot(Slot_Amulet, EEOBEquipSlot::Amulet);
	UpdateOneSlot(Slot_RingLeft, EEOBEquipSlot::RingLeft);
	UpdateOneSlot(Slot_RingRight, EEOBEquipSlot::RingRight);
}

// ===================== 面板空隙兜底拦截 =====================

FReply UEOB_Widget_EquipmentPanel::NativeOnMouseButtonDown(const FGeometry& InGeometry,
                                                           const FPointerEvent& InMouseEvent)
{
	// 装备格处理过的点击不会冒泡到这里；能到这里说明落点是"面板空隙"：
	// 一律吞掉，防止穿透到游戏世界让角色移动。右键 = 取消手持（装备穿回原始槽位）。
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (UEOB_Widget_Inventory* Hub = UEOB_Widget_Inventory::GetInstance())
		{
			if (Hub->IsHoldingItem())
			{
				Hub->CancelHeldItem();
			}
		}
	}
	return FReply::Handled();
}

FReply UEOB_Widget_EquipmentPanel::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry,
                                                                  const FPointerEvent& InMouseEvent)
{
	// 快速连点的第二次按下走这里，同样吞掉
	return NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
