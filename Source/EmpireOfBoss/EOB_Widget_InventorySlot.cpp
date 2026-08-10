#include "EOB_Widget_InventorySlot.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "EOB_InventoryComponent.h"
#include "EOB_ItemDefinition.h"
#include "EOB_ItemFunctionLibrary.h"

void UEOB_Widget_InventorySlot::NativeConstruct()
{
	Super::NativeConstruct();

	// C++ 自动绑定左键点击，蓝图不用再连 OnClicked
	if (Button)
	{
		Button->OnClicked.AddDynamic(this, &UEOB_Widget_InventorySlot::OnSlotClicked);
	}
}

void UEOB_Widget_InventorySlot::InitInventorySlot(UEOB_InventoryComponent* Inv, int32 InTabIndex, int32 InSlotInTab)
{
	Mode = EEOBSlotWidgetMode::Inventory;
	RefInventory = Inv;
	TabIndex = InTabIndex;
	SlotInTab = InSlotInTab;
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

	// 左键：背包格留给"抓取/拖拽"（下一步实现）；装备格暂时保留"点击卸下"作为过渡
	if (Mode == EEOBSlotWidgetMode::Equipment)
	{
		RefInventory->UnequipItem(EquipSlot);
	}
}

FReply UEOB_Widget_InventorySlot::NativeOnMouseButtonDown(const FGeometry& InGeometry,
                                                          const FPointerEvent& InMouseEvent)
{
	// 右键单击背包格：装备穿到默认槽位；背包物品装到第一个空栏位
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton
		&& Mode == EEOBSlotWidgetMode::Inventory
		&& RefInventory.IsValid())
	{
		FEOBItemInstance Item;
		if (RefInventory->GetItemAt(TabIndex, SlotInTab, Item) && Item.IsValid())
		{
			if (Item.Definition->Kind == EEOBItemKind::Bag)
			{
				RefInventory->EquipBagFromInventory(TabIndex, SlotInTab);
			}
			else
			{
				RefInventory->EquipFromInventory(TabIndex, SlotInTab);
			}
			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
