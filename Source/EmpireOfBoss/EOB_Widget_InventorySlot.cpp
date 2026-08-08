#include "EOB_Widget_InventorySlot.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "EOB_InventoryComponent.h"
#include "EOB_ItemDefinition.h"
#include "EOB_ItemFunctionLibrary.h"

void UEOB_Widget_InventorySlot::NativeConstruct()
{
	Super::NativeConstruct();

	// C++ 自动绑定点击，蓝图不用再连 OnClicked
	if (Button)
	{
		Button->OnClicked.AddDynamic(this, &UEOB_Widget_InventorySlot::OnSlotClicked);
	}
}

void UEOB_Widget_InventorySlot::InitInventorySlot(UEOB_InventoryComponent* Inv, int32 InSlotIndex)
{
	Mode = EEOBSlotWidgetMode::Inventory;
	RefInventory = Inv;
	SlotIndex = InSlotIndex;
}

void UEOB_Widget_InventorySlot::InitEquipmentSlot(UEOB_InventoryComponent* Inv, EEOBEquipSlot InEquipSlot)
{
	Mode = EEOBSlotWidgetMode::Equipment;
	RefInventory = Inv;
	EquipSlot = InEquipSlot;
}

void UEOB_Widget_InventorySlot::UpdateSlot(const FEOBItemInstance& Item)
{
	if (Item.IsValid() && Item.Definition)
	{
		Image_icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Image_icon->SetBrushFromTexture(Item.Definition->Icon);
		Border_rarity->SetBrushColor(UEOB_ItemFunctionLibrary::GetRarityColor(Item.Rarity));
	}
	else
	{
		// 空格：隐藏图标（无贴图的 Image 会画默认白方块）+ 暗灰边框
		Image_icon->SetBrushFromTexture(nullptr);
		Image_icon->SetVisibility(ESlateVisibility::Hidden);
		Border_rarity->SetBrushColor(FLinearColor(0.15f, 0.15f, 0.15f, 1.f));
	}
}

void UEOB_Widget_InventorySlot::OnSlotClicked()
{
	if (!RefInventory.IsValid()) return;

	if (Mode == EEOBSlotWidgetMode::Inventory)
	{
		RefInventory->EquipFromInventory(SlotIndex);
	}
	else
	{
		RefInventory->UnequipItem(EquipSlot);
	}
}
