#include "EOB_Widget_PreferenceOption.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "EOB_Widget_InventoryTab.h"

void UEOB_Widget_PreferenceOption::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Option)
	{
		Button_Option->OnClicked.AddDynamic(this, &UEOB_Widget_PreferenceOption::OnOptionClicked);
	}
}

void UEOB_Widget_PreferenceOption::InitOption(UEOB_Widget_InventoryTab* OwnerTab, int32 InCategoryIndex,
                                              UTexture2D* Icon, const FText& Name)
{
	RefTab = OwnerTab;
	CategoryIndex = InCategoryIndex;

	if (Image_Icon)
	{
		if (Icon)
		{
			Image_Icon->SetBrushFromTexture(Icon);
			Image_Icon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else
		{
			// 这个分类没配图标就不显示图标，只显示文字
			Image_Icon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (Text_Name)
	{
		Text_Name->SetText(Name);
	}
}

void UEOB_Widget_PreferenceOption::OnOptionClicked()
{
	if (RefTab.IsValid())
	{
		RefTab->OnPreferenceOptionChosen(CategoryIndex);
	}
}
