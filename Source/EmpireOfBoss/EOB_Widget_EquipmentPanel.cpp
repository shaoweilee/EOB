#include "EOB_Widget_EquipmentPanel.h"
#include "Components/PanelWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "EOB_InventoryComponent.h"
#include "EOB_Widget_InventorySlot.h"
#include "EmpireOfBossCharacter.h"

// 10 个槽位的生成顺序（戒指分左右）
static const EEOBEquipSlot GEquipSlotOrder[] = {
	EEOBEquipSlot::Weapon, EEOBEquipSlot::Shield, EEOBEquipSlot::Helmet, EEOBEquipSlot::Chest,
	EEOBEquipSlot::Gloves, EEOBEquipSlot::Boots, EEOBEquipSlot::Belt, EEOBEquipSlot::Amulet,
	EEOBEquipSlot::RingLeft, EEOBEquipSlot::RingRight
};

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

	// 程序自动生成 10 个装备格，蓝图不用手动摆
	if (EquipSlotsBox && SlotWidgetClass)
	{
		EquipSlotsBox->ClearChildren();
		SlotWidgets.Empty();

		for (const EEOBEquipSlot SlotId : GEquipSlotOrder)
		{
			if (UEOB_Widget_InventorySlot* W = CreateWidget<UEOB_Widget_InventorySlot>(this, SlotWidgetClass))
			{
				W->InitEquipmentSlot(RefInventory, SlotId);
				EquipSlotsBox->AddChild(W);
				SlotWidgets.Add(W);
			}
		}
	}

	RefreshEquip();
}

void UEOB_Widget_EquipmentPanel::RefreshEquip()
{
	if (!RefInventory) return;

	for (int32 i = 0; i < SlotWidgets.Num(); ++i)
	{
		if (const FEOBItemInstance* Found = RefInventory->EquippedItems.Find(GEquipSlotOrder[i]))
		{
			SlotWidgets[i]->UpdateSlot(*Found);
		}
		else
		{
			SlotWidgets[i]->UpdateSlot(FEOBItemInstance()); // 空槽
		}
	}
}
