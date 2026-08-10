#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_Widget_Inventory.generated.h"

class UUniformGridPanel;
class UEOB_InventoryComponent;
class UEOB_Widget_InventorySlot;
class UEOB_Widget_InventoryTab;

/**
 * 背包面板：标签栏（12 个页签）+ 当前页格子（格子数 = 该页背包容量）。
 * 蓝图子类需要两个同名 Uniform Grid Panel：
 *  - GridPanel_Tabs：标签栏容器（12 个页签摆成 2 行 × 6 列）
 *  - GridPanel_Items：物品格子容器
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

	/** 标签栏容器（页签摆成 2 行 × 6 列） */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UUniformGridPanel> GridPanel_Tabs;

	/** 格子控件类（蓝图默认值里选 WBP_InventorySlot） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	TSubclassOf<UEOB_Widget_InventorySlot> SlotWidgetClass;

	/** 页签控件类（蓝图默认值里选 WBP_InventoryTab） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	TSubclassOf<UEOB_Widget_InventoryTab> TabWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	int32 GridColumns = 8;

	/** 当前显示的标签页（0~11），默认第 1 页 */
	UPROPERTY(BlueprintReadOnly, Category = "EOB|UI")
	int32 CurrentTabIndex = 0;

	UPROPERTY()
	TObjectPtr<UEOB_InventoryComponent> RefInventory;
};
