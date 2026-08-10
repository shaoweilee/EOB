#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_ItemTypes.h"
#include "EOB_Widget_InventoryTab.generated.h"

class UButton;
class UTextBlock;
class UComboBoxString;
class UEOB_InventoryComponent;
class UEOB_Widget_Inventory;

/**
 * 背包标签栏上的单个页签：
 *  - Button_Tab + Text_TabName：显示该页"偏好"分类名，点击切换到该页（未激活的页禁用）
 *  - ComboBox_Preference：下拉单选 10 个分类，设置该页"偏好"
 * 蓝图里必须包含三个同名控件：Button_Tab、Text_TabName、ComboBox_Preference（BindWidget 自动绑定）
 */
UCLASS()
class EMPIREOFBOSS_API UEOB_Widget_InventoryTab : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 初始化：绑定背包组件、所属背包面板、页号（0~11） */
	void InitTab(UEOB_InventoryComponent* Inv, UEOB_Widget_Inventory* OwnerPanel, int32 InTabIndex);

	/** 刷新显示：激活状态、偏好名、是否当前页 */
	void RefreshTab();

protected:
	virtual void NativeConstruct() override;

	/** 页签按钮（点击 = 切换页） */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UButton> Button_Tab;

	/** 按钮上的文字（显示偏好分类名） */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UTextBlock> Text_TabName;

	/** 偏好下拉单选框（10 个分类） */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UComboBoxString> ComboBox_Preference;

	UFUNCTION()
	void OnTabClicked();

	UFUNCTION()
	void OnPreferenceChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

private:
	int32 TabIndex = 0;
	TWeakObjectPtr<UEOB_InventoryComponent> RefInventory;
	TWeakObjectPtr<UEOB_Widget_Inventory> RefPanel;
};
