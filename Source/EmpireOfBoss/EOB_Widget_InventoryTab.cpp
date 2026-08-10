#include "EOB_Widget_InventoryTab.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ComboBoxString.h"
#include "EOB_InventoryComponent.h"
#include "EOB_Widget_Inventory.h"

namespace EOBTabUI
{
	/** 下拉框选项文字，顺序与 EEOBItemCategory 枚举一一对应 */
	static const FString CategoryNames[] = {
		TEXT("未分类"), TEXT("武器"), TEXT("盾牌"), TEXT("头盔"), TEXT("胸甲"),
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

	if (ComboBox_Preference)
	{
		ComboBox_Preference->ClearOptions();
		for (int32 i = 0; i < EOBTabUI::CategoryCount; ++i)
		{
			ComboBox_Preference->AddOption(EOBTabUI::CategoryNames[i]);
		}
		ComboBox_Preference->OnSelectionChanged.AddDynamic(this, &UEOB_Widget_InventoryTab::OnPreferenceChanged);
	}
}

void UEOB_Widget_InventoryTab::InitTab(UEOB_InventoryComponent* Inv, UEOB_Widget_Inventory* OwnerPanel,
                                       int32 InTabIndex)
{
	RefInventory = Inv;
	RefPanel = OwnerPanel;
	TabIndex = InTabIndex;
	RefreshTab();
}

void UEOB_Widget_InventoryTab::RefreshTab()
{
	if (!RefInventory.IsValid()) return;

	const bool bActive = RefInventory->IsTabActive(TabIndex);
	const EEOBItemCategory Pref = RefInventory->GetTabPreference(TabIndex);
	const int32 PrefIndex = FMath::Clamp(static_cast<int32>(Pref), 0, EOBTabUI::CategoryCount - 1);
	const bool bIsCurrent = RefPanel.IsValid() && RefPanel->GetCurrentTab() == TabIndex;

	// 按钮文字 = 偏好分类名；当前页金色高亮，普通页白色
	if (Text_TabName)
	{
		Text_TabName->SetText(FText::FromString(EOBTabUI::CategoryNames[PrefIndex]));
		Text_TabName->SetColorAndOpacity(FSlateColor(
			bIsCurrent ? FLinearColor(1.f, 0.8f, 0.2f) : FLinearColor::White));
	}

	// 未激活（没装背包）的页：按钮和下拉都禁用，置灰
	if (Button_Tab)
	{
		Button_Tab->SetIsEnabled(bActive);
	}
	if (ComboBox_Preference)
	{
		ComboBox_Preference->SetIsEnabled(bActive);
		// SetSelectedOption 会触发 OnSelectionChanged（Direct 类型），那边已做忽略处理
		ComboBox_Preference->SetSelectedOption(EOBTabUI::CategoryNames[PrefIndex]);
	}
}

void UEOB_Widget_InventoryTab::OnTabClicked()
{
	if (RefPanel.IsValid())
	{
		// SetCurrentTab 内部会拒绝未激活的页
		RefPanel->SetCurrentTab(TabIndex);
	}
}

void UEOB_Widget_InventoryTab::OnPreferenceChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	// 代码里 SetSelectedOption 引发的回调不处理（那是回显，不是玩家选择）
	if (SelectionType == ESelectInfo::Direct || !RefInventory.IsValid()) return;

	for (int32 i = 0; i < EOBTabUI::CategoryCount; ++i)
	{
		if (EOBTabUI::CategoryNames[i] == SelectedItem)
		{
			RefInventory->SetTabPreference(TabIndex, static_cast<EEOBItemCategory>(i));
			UE_LOG(LogTemp, Log, TEXT("[背包 UI] 第 %d 页偏好设为【%s】"), TabIndex + 1, *SelectedItem);
			break;
		}
	}

	RefreshTab(); // 按钮名跟着变
}
