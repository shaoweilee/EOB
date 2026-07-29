#include "EOB_Widget_Inventory.h"
#include "Components/UniformGridPanel.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "EOB_InventoryComponent.h"
#include "EOB_Widget_InventorySlot.h"
#include "EmpireOfBossCharacter.h"

void UEOB_Widget_Inventory::NativeConstruct()
{
	Super::NativeConstruct();

	// 自动拿到主角的背包组件并订阅变化事件
	if (AEmpireOfBossCharacter* Hero = Cast<AEmpireOfBossCharacter>(GetOwningPlayerPawn()))
	{
		RefInventory = Hero->InventoryComponent;
		if (RefInventory)
		{
			RefInventory->OnInventoryChanged.AddDynamic(this, &UEOB_Widget_Inventory::RefreshUI);
			RefInventory->OnEquipmentChanged.AddDynamic(this, &UEOB_Widget_Inventory::RefreshUI);
		}
	}

	RefreshUI();
}

void UEOB_Widget_Inventory::RefreshUI()
{
	if (!GridPanel_Items || !SlotWidgetClass || !RefInventory) return;

	GridPanel_Items->ClearChildren();

	for (int32 i = 0; i < RefInventory->Items.Num(); ++i)
	{
		UEOB_Widget_InventorySlot* SlotWidget = CreateWidget<UEOB_Widget_InventorySlot>(this, SlotWidgetClass);
		if (!SlotWidget) continue;

		SlotWidget->InitInventorySlot(RefInventory, i);
		SlotWidget->UpdateSlot(RefInventory->Items[i]);
		GridPanel_Items->AddChildToUniformGrid(SlotWidget, i / GridColumns, i % GridColumns);
	}
}
