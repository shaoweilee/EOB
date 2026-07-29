#include "EOB_HUDWidget.h"
#include "EOB_Widget_Inventory.h"
#include "EOB_Widget_EquipmentPanel.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

void UEOB_HUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 自动创建背包/装备面板，默认隐藏
	if (InventoryPanelClass && !InventoryPanel)
	{
		InventoryPanel = CreateWidget<UEOB_Widget_Inventory>(this, InventoryPanelClass);
		if (InventoryPanel)
		{
			InventoryPanel->AddToViewport();
			InventoryPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (EquipmentPanelClass && !EquipmentPanel)
	{
		EquipmentPanel = CreateWidget<UEOB_Widget_EquipmentPanel>(this, EquipmentPanelClass);
		if (EquipmentPanel)
		{
			EquipmentPanel->AddToViewport();
			EquipmentPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UEOB_HUDWidget::ToggleInventoryPanels()
{
	const auto Toggle = [](UUserWidget* Panel)
	{
		if (!Panel) return;
		const bool bVisible = Panel->GetVisibility() == ESlateVisibility::Visible;
		Panel->SetVisibility(bVisible ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	};

	Toggle(InventoryPanel);
	Toggle(EquipmentPanel);
}
