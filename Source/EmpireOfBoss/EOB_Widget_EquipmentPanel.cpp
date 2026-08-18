#include "EOB_Widget_EquipmentPanel.h"
#include "EOB_InventoryComponent.h"
#include "EOB_Widget_Inventory.h"
#include "EOB_Widget_InventorySlot.h"
#include "EmpireOfBossCharacter.h"

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

	// 登记为"会挡住丢弃判定"的面板：手持物落在本面板的空隙上 = 无反应，不会误丢到地上
	UEOB_Widget_Inventory::GetAllPanelWidgets().AddUnique(this);

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
	UEOB_Widget_Inventory::GetAllPanelWidgets().RemoveAll(
		[this](const TWeakObjectPtr<UUserWidget>& Ptr)
		{
			return !Ptr.IsValid() || Ptr.Get() == this;
		});

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
