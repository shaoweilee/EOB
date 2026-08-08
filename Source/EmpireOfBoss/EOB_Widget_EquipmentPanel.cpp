#include "EOB_Widget_EquipmentPanel.h"
#include "EOB_InventoryComponent.h"
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

	// 给蓝图上手动摆好的 10 个格子分发各自的槽位身份（点击 = 卸下对应槽）
	Slot_Weapon->InitEquipmentSlot(RefInventory, EEOBEquipSlot::Weapon);
	Slot_Shield->InitEquipmentSlot(RefInventory, EEOBEquipSlot::Shield);
	Slot_Helmet->InitEquipmentSlot(RefInventory, EEOBEquipSlot::Helmet);
	Slot_Chest->InitEquipmentSlot(RefInventory, EEOBEquipSlot::Chest);
	Slot_Gloves->InitEquipmentSlot(RefInventory, EEOBEquipSlot::Gloves);
	Slot_Boots->InitEquipmentSlot(RefInventory, EEOBEquipSlot::Boots);
	Slot_Belt->InitEquipmentSlot(RefInventory, EEOBEquipSlot::Belt);
	Slot_Amulet->InitEquipmentSlot(RefInventory, EEOBEquipSlot::Amulet);
	Slot_RingLeft->InitEquipmentSlot(RefInventory, EEOBEquipSlot::RingLeft);
	Slot_RingRight->InitEquipmentSlot(RefInventory, EEOBEquipSlot::RingRight);

	RefreshEquip();
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
