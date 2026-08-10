#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_ItemTypes.h"
#include "EOB_Widget_InventoryTab.generated.h"

class UButton;
class UImage;
class UVerticalBox;
class UEOB_InventoryComponent;
class UEOB_Widget_Inventory;
class UEOB_Widget_PreferenceOption;

/**
 * 背包标签栏上的单个页签：
 *  - Button_Tab + Image_PrefIcon：显示该页"偏好"分类的图标，点击切换到该页（未激活的页禁用置灰）
 *  - Button_Arrow：点开/收起"偏好"下拉面板（Panel_Dropdown，里面是 10 行图标+文字选项）
 * 蓝图里必须包含三个同名控件：Button_Tab、Image_PrefIcon、Button_Arrow；
 * 可选控件（不配就没有下拉功能）：Panel_Dropdown（默认 Collapsed 的下拉面板）、VerticalBox_Options（选项容器）。
 * 下拉面板画在页签 110×56 之外没关系，UMG 默认不裁剪，能正常显示和点击。
 */
UCLASS()
class EMPIREOFBOSS_API UEOB_Widget_InventoryTab : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 初始化：绑定背包组件、所属背包面板、页号（0~11） */
	void InitTab(UEOB_InventoryComponent* Inv, UEOB_Widget_Inventory* OwnerPanel, int32 InTabIndex);

	/** 刷新显示：激活状态、偏好图标、是否当前页 */
	void RefreshTab();

	int32 GetTabIndex() const { return TabIndex; }

	/** 收起下拉面板（切页、打开别的页签下拉时由面板统一调用） */
	void CloseDropdown();

	/** 下拉选项被点：把该页偏好设为对应分类（由 EOB_Widget_PreferenceOption 回调） */
	void OnPreferenceOptionChosen(int32 CategoryIndex);

protected:
	virtual void NativeConstruct() override;

	/** 页签按钮（点击 = 切换页） */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UButton> Button_Tab;

	/** 按钮上的图标（显示当前偏好分类的图标；当前页染金色、未激活染灰色） */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UImage> Image_PrefIcon;

	/** 箭头按钮（点击 = 展开/收起偏好下拉面板） */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UButton> Button_Arrow;

	/** 下拉面板（默认设 Collapsed；点箭头时展开）。可以是任意容器控件，画在页签外面也行 */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UWidget> Panel_Dropdown;

	/** 下拉面板里放选项的垂直框（C++ 运行时往里填 10 行选项） */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UVerticalBox> VerticalBox_Options;

	/** 10 个分类的图标，顺序必须和 EEOBItemCategory 一致：任意/武器/盾牌/头盔/胸甲/手套/鞋子/腰带/项链/戒指 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	TArray<TObjectPtr<UTexture2D>> CategoryIcons;

	/** 下拉选项控件类（蓝图默认值里选 WBP_PreferenceOption） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	TSubclassOf<UEOB_Widget_PreferenceOption> PreferenceOptionClass;

	UFUNCTION()
	void OnTabClicked();

	UFUNCTION()
	void OnArrowClicked();

	/** 展开/收起下拉面板 */
	void ToggleDropdown();

private:
	int32 TabIndex = 0;
	TWeakObjectPtr<UEOB_InventoryComponent> RefInventory;
	TWeakObjectPtr<UEOB_Widget_Inventory> RefPanel;
};
