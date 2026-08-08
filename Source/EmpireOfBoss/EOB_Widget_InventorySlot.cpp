#include "EOB_Widget_InventorySlot.h"
#include "Components/Button.h"
#include "Components/Image.h"
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
		const FLinearColor RarityColor = UEOB_ItemFunctionLibrary::GetRarityColor(Item.Rarity);

		// 物品图标
		Image_icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Image_icon->SetBrushFromTexture(Item.Definition->Icon);

		// 品质边框 + 中心径向渐变，同染品质色（辉光透明度略降，柔和不抢图标）
		Image_frame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Image_frame->SetColorAndOpacity(RarityColor);

		Image_glow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Image_glow->SetColorAndOpacity(FLinearColor(RarityColor.R, RarityColor.G, RarityColor.B, 0.6f));
	}
	else
	{
		// 空格：图标和辉光隐藏，背景全透明；边框留一丝淡灰方便辨认格子位置
		Image_icon->SetBrushFromTexture(nullptr);
		Image_icon->SetVisibility(ESlateVisibility::Hidden);

		Image_glow->SetVisibility(ESlateVisibility::Hidden);

		Image_frame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Image_frame->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.15f));
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
