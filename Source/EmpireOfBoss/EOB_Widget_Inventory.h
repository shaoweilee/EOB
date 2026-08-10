#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_Widget_Inventory.generated.h"

class UUniformGridPanel;
class UVerticalBox;
class UEOB_InventoryComponent;
class UEOB_Widget_InventorySlot;
class UEOB_Widget_InventoryTab;

/**
 * 背包面板：左右两列页签（各 6 个）+ 当前页格子（格子数 = 该页背包容量）。
 * 蓝图子类需要三个同名容器：
 *  - VerticalBox_TabsLeft：左侧页签栏（第 1~6 页，从上到下）
 *  - VerticalBox_TabsRight：右侧页签栏（第 7~12 页，从上到下）
 *  - GridPanel_Items：物品格子容器
 * 左右页签各用一个皮肤类（TabWidgetClassLeft / TabWidgetClassRight），
 * 两个皮肤的父类都是 EOB_Widget_InventoryTab，只是 UMG 布局镜像。
 */
UCLASS()
class EMPIREOFBOSS_API UEOB_Widget_Inventory : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void RefreshUI();

	/** 刷新标签栏（激活状态、偏好名、当前页高亮） */
	UFUNCTION()
	void RefreshTabs();

	/** 切换当前显示的标签页（0~11）；未激活的页拒绝切换 */
	UFUNCTION(BlueprintCallable, Category = "EOB|UI")
	void SetCurrentTab(int32 NewTab);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EOB|UI")
	int32 GetCurrentTab() const { return CurrentTabIndex; }

protected:
	virtual void NativeConstruct() override;

	/** 物品格子容器 */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UUniformGridPanel> GridPanel_Items;

	/** 左侧页签栏容器（第 1~6 页） */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UVerticalBox> VerticalBox_TabsLeft;

	/** 右侧页签栏容器（第 7~12 页） */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UVerticalBox> VerticalBox_TabsRight;

	/** 格子控件类（蓝图默认值里选 WBP_InventorySlot） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	TSubclassOf<UEOB_Widget_InventorySlot> SlotWidgetClass;

	/** 左侧页签皮肤类（WBP_InventoryTab_Left：左文字 + 右箭头） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	TSubclassOf<UEOB_Widget_InventoryTab> TabWidgetClassLeft;

	/** 右侧页签皮肤类（WBP_InventoryTab_Right：左箭头 + 右文字）。不填则 12 个页签都用左侧皮肤 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	TSubclassOf<UEOB_Widget_InventoryTab> TabWidgetClassRight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	int32 GridColumns = 8;

	/** 物品栏格子的边长（像素）。只作用于物品栏，装备面板格子不受影响 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	float InventorySlotSize = 110.f;

	/** 当前显示的标签页（0~11），默认第 1 页 */
	UPROPERTY(BlueprintReadOnly, Category = "EOB|UI")
	int32 CurrentTabIndex = 0;

	UPROPERTY()
	TObjectPtr<UEOB_InventoryComponent> RefInventory;
};
