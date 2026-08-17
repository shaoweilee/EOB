#include "EOB_Widget_InventoryTab.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "EOB_InventoryComponent.h"
#include "EOB_Widget_Inventory.h"
#include "EOB_Widget_InventorySlot.h"
#include "EOB_Widget_PreferenceOption.h"

namespace EOBTabUI
{
	/** 下拉选项文字，顺序与 EEOBItemCategory 枚举一一对应 */
	static const FString CategoryNames[] = {
		TEXT("任意"), TEXT("武器"), TEXT("盾牌"), TEXT("头盔"), TEXT("胸甲"),
		TEXT("手套"), TEXT("鞋子"), TEXT("腰带"), TEXT("项链"), TEXT("戒指")
	};
	static constexpr int32 CategoryCount = 10;
}

void UEOB_Widget_InventoryTab::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Tab)
	{
		Button_Tab->OnClicked.AddDynamic(this, &UEOB_Widget_InventoryTab::OnTabClicked);
	}

	// 下拉面板默认收起，并往里填 10 行"图标+文字"选项
	if (Panel_Dropdown)
	{
		Panel_Dropdown->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (Panel_Dropdown && VerticalBox_Options && PreferenceOptionClass)
	{
		VerticalBox_Options->ClearChildren();
		for (int32 i = 0; i < EOBTabUI::CategoryCount; ++i)
		{
			UEOB_Widget_PreferenceOption* Option = CreateWidget<UEOB_Widget_PreferenceOption>(
				this, PreferenceOptionClass);
			if (!Option) continue;

			UTexture2D* Icon = CategoryIcons.IsValidIndex(i) ? CategoryIcons[i].Get() : nullptr;
			Option->InitOption(this, i, Icon, FText::FromString(EOBTabUI::CategoryNames[i]));
			VerticalBox_Options->AddChild(Option);
		}
	}
}

void UEOB_Widget_InventoryTab::InitTab(UEOB_InventoryComponent* Inv, UEOB_Widget_Inventory* OwnerPanel,
                                       int32 InTabIndex)
{
	RefInventory = Inv;
	RefPanel = OwnerPanel;
	TabIndex = InTabIndex;

	// 包裹栏位：绑定数据 + 定尺寸（只影响本实例，不影响 WBP_InventorySlot 皮肤）
	if (Slot_Bag)
	{
		Slot_Bag->InitBagSlot(Inv, TabIndex, OwnerPanel);
		Slot_Bag->SetSlotDesiredSize(BagSlotSize);
	}

	RefreshTab();
}

void UEOB_Widget_InventoryTab::RefreshTab()
{
	if (!RefInventory.IsValid()) return;

	const bool bActive = RefInventory->IsTabActive(TabIndex);
	const EEOBItemCategory Pref = RefInventory->GetTabPreference(TabIndex);
	const int32 PrefIndex = FMath::Clamp(static_cast<int32>(Pref), 0, EOBTabUI::CategoryCount - 1);
	const bool bIsCurrent = RefPanel.IsValid() && RefPanel->GetCurrentTab() == TabIndex;

	// 页签图标 = 当前偏好分类的图标；当前页染金色、未激活染灰色、普通页白色
	if (Image_PrefIcon)
	{
		UTexture2D* Icon = CategoryIcons.IsValidIndex(PrefIndex) ? CategoryIcons[PrefIndex].Get() : nullptr;
		if (Icon)
		{
			Image_PrefIcon->SetBrushFromTexture(Icon);
			Image_PrefIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

			FLinearColor Tint = FLinearColor::White;
			if (!bActive)
			{
				Tint = FLinearColor(0.35f, 0.35f, 0.35f);
			}
			else if (bIsCurrent)
			{
				Tint = FLinearColor(1.f, 0.8f, 0.2f);
			}
			Image_PrefIcon->SetColorAndOpacity(Tint);
		}
		else
		{
			Image_PrefIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// 包裹栏位：显示该页装备的背包（没装包就显示空格）
	if (Slot_Bag)
	{
		FEOBItemInstance Bag;
		RefInventory->GetTabBag(TabIndex, Bag);
		Slot_Bag->UpdateSlot(Bag);
	}

	// 未激活（没装背包）的页：页签按钮禁用（包裹栏位保持可点，方便以后接受拖来的背包）
	if (Button_Tab)
	{
		Button_Tab->SetIsEnabled(bActive);
	}
}

FReply UEOB_Widget_InventoryTab::NativeOnMouseButtonDown(const FGeometry& InGeometry,
                                                         const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		// 手里拿着物品时：右键 = 放回原处（取消手持），优先级最高
		if (RefPanel.IsValid() && RefPanel->IsHoldingItem())
		{
			RefPanel->CancelHeldItem();
			return FReply::Handled();
		}

		// 右键页签 = 展开/收起"偏好"下拉面板（替代原来的箭头按钮）；未激活的页不弹
		if (RefInventory.IsValid() && RefInventory->IsTabActive(TabIndex))
		{
			ToggleDropdown();
		}
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UEOB_Widget_InventoryTab::CloseDropdown()
{
	if (Panel_Dropdown)
	{
		Panel_Dropdown->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UEOB_Widget_InventoryTab::OnPreferenceOptionChosen(int32 CategoryIndex)
{
	if (RefInventory.IsValid())
	{
		RefInventory->SetTabPreference(TabIndex, static_cast<EEOBItemCategory>(CategoryIndex));
		const int32 ClampedIndex = FMath::Clamp(CategoryIndex, 0, EOBTabUI::CategoryCount - 1);
		UE_LOG(LogTemp, Log, TEXT("[背包 UI] 第 %d 页偏好设为【%s】"), TabIndex + 1, *EOBTabUI::CategoryNames[ClampedIndex]);
	}

	RefreshTab(); // 页签图标立刻跟着换
	CloseDropdown(); // 选完收起下拉
}

void UEOB_Widget_InventoryTab::OnTabClicked()
{
	if (RefPanel.IsValid())
	{
		// SetCurrentTab 内部会拒绝未激活的页，并顺手收起所有下拉面板
		RefPanel->SetCurrentTab(TabIndex);
	}
}

void UEOB_Widget_InventoryTab::ToggleDropdown()
{
	if (!Panel_Dropdown) return;

	if (Panel_Dropdown->GetVisibility() == ESlateVisibility::Visible)
	{
		CloseDropdown();
		return;
	}

	// 同时只开一个下拉：让面板收起其他页签的
	if (RefPanel.IsValid())
	{
		RefPanel->CloseAllTabDropdowns(TabIndex);
	}
	Panel_Dropdown->SetVisibility(ESlateVisibility::Visible);
}
