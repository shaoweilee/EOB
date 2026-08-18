#include "EOB_Widget_InventorySlot.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "EOB_InventoryComponent.h"
#include "EOB_ItemDefinition.h"
#include "EOB_ItemFunctionLibrary.h"
#include "EOB_Widget_Inventory.h"

namespace EOBSlotWidgetRegistry
{
	/** 全局格子注册表：背包/包裹栏位/装备面板的格子都在里面，供面板做"禁止投放"红框检测和落点查找 */
	TArray<TWeakObjectPtr<UEOB_Widget_InventorySlot>> GAllSlotWidgets;
}

TArray<TWeakObjectPtr<UEOB_Widget_InventorySlot>>& UEOB_Widget_InventorySlot::GetAllSlotWidgets()
{
	return EOBSlotWidgetRegistry::GAllSlotWidgets;
}

void UEOB_Widget_InventorySlot::NativeConstruct()
{
	Super::NativeConstruct();

	// 注意：Button 的 OnClicked 不再绑定任何东西——三种模式的左键按下都在预览阶段被吞掉
	// （见 NativeOnPreviewMouseButtonDown），Button 收不到按下、OnClicked 不会触发。
	// "原地点击"语义由背包面板 Tick 在松开时补发，功能不变。

	// 登记进全局列表（网格每帧重建也没关系，弱引用 + 析构注销 + 面板侧顺手清理）
	EOBSlotWidgetRegistry::GAllSlotWidgets.AddUnique(this);
}

void UEOB_Widget_InventorySlot::NativeDestruct()
{
	EOBSlotWidgetRegistry::GAllSlotWidgets.RemoveAll(
		[this](const TWeakObjectPtr<UEOB_Widget_InventorySlot>& Ptr)
		{
			return !Ptr.IsValid() || Ptr.Get() == this;
		});

	Super::NativeDestruct();
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

void UEOB_Widget_InventorySlot::InitEquipmentSlot(UEOB_InventoryComponent* Inv, EEOBEquipSlot InEquipSlot,
                                                  UEOB_Widget_Inventory* OwnerPanel)
{
	Mode = EEOBSlotWidgetMode::Equipment;
	RefInventory = Inv;
	RefPanel = OwnerPanel;
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
	// 缓存下来：红框取消时按它恢复品质色
	LastItem = Item;

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

	// 禁止投放提示优先于一切常规染色：整框染红
	if (bForbidden)
	{
		Image_frame->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Image_frame->SetColorAndOpacity(FLinearColor(1.f, 0.08f, 0.05f, 1.f));
	}
}

void UEOB_Widget_InventorySlot::SetForbiddenHighlight(bool bOn)
{
	if (bForbidden == bOn) return;
	bForbidden = bOn;
	// 重走一遍常规染色：bForbidden=true 时末尾会整框染红；false 时恢复成品质色/淡灰
	UpdateSlot(LastItem);
}

FReply UEOB_Widget_InventorySlot::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry,
                                                                 const FPointerEvent& InMouseEvent)
{
	// 左键按下（三种模式全部）：登记拖拽起点，并【直接吞掉事件】——
	// 不让 Button 收到这次按下，Slate 就不会捕获鼠标，
	// PC->GetMousePosition 在拖拽全程保持实时（跟手图标就靠这个唯一真值源，零坐标换算）。
	// 原地松开的"点击"语义由面板 Tick 在松开时补发 OnSlotGrabClicked，功能不变。
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// 装备面板的格子可能先于背包面板构造，那时拿不到面板引用；按下的瞬间现场补取一次
		if (!RefPanel.IsValid())
		{
			RefPanel = UEOB_Widget_Inventory::GetInstance();
		}

		if (RefPanel.IsValid())
		{
			RefPanel->NotifySlotLeftPressed(this, InMouseEvent.GetScreenSpacePosition());
			return FReply::Handled();
		}
	}

	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UEOB_Widget_InventorySlot::NativeOnMouseButtonDown(const FGeometry& InGeometry,
                                                          const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton && RefInventory.IsValid())
	{
		if (!RefPanel.IsValid())
		{
			RefPanel = UEOB_Widget_Inventory::GetInstance();
		}

		// 手里拿着东西时：任何右键 = 放回原处（取消手持；装备来源 = 穿回原始槽位），优先级最高
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

		// 装备格：空手右键不做事（卸下请用左键抓取），吞掉事件防止冒泡
		if (Mode == EEOBSlotWidgetMode::Equipment)
		{
			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
