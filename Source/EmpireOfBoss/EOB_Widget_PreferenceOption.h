#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_Widget_PreferenceOption.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UEOB_Widget_InventoryTab;

/**
 * 背包页签“偏好”下拉面板的单个选项：图标 + 分类名，点击 = 把该页偏好设为此分类。
 * 蓝图里需要同名控件：Button_Option、Image_Icon、Text_Name（均为可选绑定，缺了不报错）。
 */
UCLASS()
class EMPIREOFBOSS_API UEOB_Widget_PreferenceOption : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 初始化：所属页签、分类序号（0~9，与 EEOBItemCategory 一一对应）、图标、分类名 */
	void InitOption(UEOB_Widget_InventoryTab* OwnerTab, int32 InCategoryIndex, UTexture2D* Icon, const FText& Name);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UButton> Button_Option;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UImage> Image_Icon;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UTextBlock> Text_Name;

	UFUNCTION()
	void OnOptionClicked();

private:
	int32 CategoryIndex = 0;
	TWeakObjectPtr<UEOB_Widget_InventoryTab> RefTab;
};
