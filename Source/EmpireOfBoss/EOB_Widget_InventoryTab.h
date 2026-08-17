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
class UEOB_Widget_InventorySlot;
class UEOB_Widget_PreferenceOption;

/**
 * 背包标签栏上的单个页签，一行 = [Slot_Bag 包裹栏位] + [Button_Tab 页签按钮]：
 *  - Slot_Bag（可选绑定，WBP_InventorySlot 实例）：显示该页装备的背包。
 *    左键抓取/拖拽 = 两个栏位的背包互换位置；右键 = 取消装备包裹。
 *  - Button_Tab + Image_PrefIcon：显示该页"偏好"分类的图标。
 *    左键 = 切换到该页；右键 = 展开/收起"偏好"下拉面板（Panel_Dropdown，替代原来的箭头按钮）。
 * 蓝图里必须包含两个同名控件：Button_Tab、Image_PrefIcon；
 * 可选控件：Slot_Bag（不配就没有包裹栏位功能）、Panel_Dropdown（默认 Collapsed）、VerticalBox_Options。
 * 下拉面板画在页签之外没关系，UMG 默认不裁剪，能正常显示和点击。
 */
UCLASS()
class EMPIREOFBOSS_API UEOB_Widget_InventoryTab : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 初始化：绑定背包组件、所属背包面板、页号（0~11） */
	void InitTab(UEOB_InventoryComponent* Inv, UEOB_Widget_Inventory* OwnerPanel, int32 InTabIndex);

	/** 刷新显示：激活状态、偏好图标、是否当前页、包裹栏位上的背包 */
	void RefreshTab();

	int32 GetTabIndex() const { return TabIndex; }

	/** 本页签左侧的包裹栏位（没配返回 nullptr；背包面板做拖拽落点检测时用） */
	UEOB_Widget_InventorySlot* GetBagSlotWidget() const { return Slot_Bag; }

	/** 收起下拉面板（切页、打开别的页签下拉时由面板统一调用） */
	void CloseDropdown();

	/** 下拉选项被点：把该页偏好设为对应分类（由 EOB_Widget_PreferenceOption 回调） */
	void OnPreferenceOptionChosen(int32 CategoryIndex);

protected:
	virtual void NativeConstruct() override;

	/** 右键单击走这里（Button_Tab 只响应左键，右键冒泡到本控件）= 展开/收起下拉面板 */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** 页签按钮（左键 = 切换页） */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UButton> Button_Tab;

	/** 按钮上的图标（显示当前偏好分类的图标；当前页染金色、未激活染灰色） */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UImage> Image_PrefIcon;

	/** 包裹栏位（可选绑定：放一个 WBP_InventorySlot 实例、命名 Slot_Bag。显示该页装备的背包） */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UEOB_Widget_InventorySlot> Slot_Bag;

	/** 包裹栏位的边长（像素），只影响本实例，不影响 WBP_InventorySlot 皮肤本身 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	float BagSlotSize = 67.f;

	/** 下拉面板（默认设 Collapsed；右键页签时展开）。可以是任意容器控件，画在页签外面也行 */
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

	/** 展开/收起下拉面板 */
	void ToggleDropdown();

private:
	int32 TabIndex = 0;
	TWeakObjectPtr<UEOB_InventoryComponent> RefInventory;
	TWeakObjectPtr<UEOB_Widget_Inventory> RefPanel;
};
