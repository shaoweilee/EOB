#include "EOB_Widget_InventorySlot.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "EOB_InventoryComponent.h"
#include "EOB_ItemDefinition.h"
#include "EOB_ItemFunctionLibrary.h"
#include "EOB_Widget_Inventory.h"

void UEOB_Widget_InventorySlot::NativeConstruct()
{
	Super::NativeConstruct();

	// C++ 自动绑定左键点击，蓝图不用再连 OnClicked
	if (Button)
	{
		Button->OnClicked.AddDynamic(this, &UEOB_Widget_InventorySlot::OnSlotClicked);
	}
}

void UEOB_Widget_InventorySlot::InitInventorySlot(UEOB_InventoryComponent* Inv, int32 InTabIndex,
                                                  int32 InSlotInTab, UEOB_Widget_Inventory* OwnerPanel)
{
	Mode = EEOBSlotWidgetMode::Inventory;
	RefInventory = Inv;
	RefPanel = OwnerPanel;
	TabIndex = InTabIndex;
	SlotInTab = InSlotInTab;
}

void UEOB_Widget_InventorySlot::InitEquipmentSlot(UEOB_InventoryComponent* Inv, EEOBEquipSlot InEquipSlot)
{
	Mode = EEOBSlotWidgetMode::Equipment;
	RefInventory = Inv;
	EquipSlot = InEquipSlot;
}

void UEOB_Widget_InventorySlot::InitBagSlot(UEOB_InventoryComponent* Inv, int32 InTabIndex,
                                            UEOB_Widget_Inventory* OwnerPanel)
{
	Mode = EEOBSlotWidgetMode::BagSlot;
	RefInventory = Inv;
	RefPanel = OwnerPanel;
	TabIndex = InTabIndex;
	SlotInTab = INDEX_NONE; // 包裹栏位没有"页内格号"的概念
}

void UEOB_Widget_InventorySlot::SetSlotDesiredSize(float InSize)
{
	// 只对本实例生效：装备面板不调用这个方法，格子大小不受影响
	if (SizeBox_Root)
	{
		SizeBox_Root->SetWidthOverride(InSize);
		SizeBox_Root->SetHeightOverride(InSize);
	}
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

	// 装备格：手上拿着东西时不动作；否则保留"点击卸下"作为过渡（装备面板抓取在下一阶段做）
	if (Mode == EEOBSlotWidgetMode::Equipment)
	{
		if (RefPanel.IsValid() && RefPanel->IsHoldingItem()) return;
		RefInventory->UnequipItem(EquipSlot);
		return;
	}

	// 背包格 / 包裹栏位：原地点击 = 抓取手势的"拿起 / 放下"，统一交给面板的状态机
	if (RefPanel.IsValid())
	{
		RefPanel->OnSlotGrabClicked(this);
	}
}

FReply UEOB_Widget_InventorySlot::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry,
                                                                 const FPointerEvent& InMouseEvent)
{
	// 左键按下：向面板登记"拖拽起点"。面板在 Tick 里检测位移超过阈值就进入拖拽。
	// 注意这里只是登记，不吞事件——原地松开时 Button 的 OnClicked 照常触发（= 抓取）。
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
		&& RefPanel.IsValid()
		&& Mode != EEOBSlotWidgetMode::Equipment)
	{
		RefPanel->NotifySlotLeftPressed(this, InMouseEvent.GetScreenSpacePosition());
	}

	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UEOB_Widget_InventorySlot::NativeOnMouseButtonDown(const FGeometry& InGeometry,
                                                          const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && RefInventory.IsValid())
	{
		// 手里拿着东西时：任何右键 = 放回原处（取消手持），优先级最高
		if (RefPanel.IsValid() && RefPanel->IsHoldingItem())
		{
			RefPanel->CancelHeldItem();
			return FReply::Handled();
		}

		// 右键单击背包格：装备穿到默认槽位；背包物品装到第一个空包裹栏位
		if (Mode == EEOBSlotWidgetMode::Inventory)
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
			}
			return FReply::Handled();
		}

		// 右键单击包裹栏位 = 取消装备包裹（包内还有物品时组件会拒绝）
		if (Mode == EEOBSlotWidgetMode::BagSlot)
		{
			if (RefInventory->IsTabActive(TabIndex))
			{
				RefInventory->UnequipBag(TabIndex);
			}
			// 空栏位也吞掉右键，免得冒泡给页签把"偏好"下拉面弹出来
			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
