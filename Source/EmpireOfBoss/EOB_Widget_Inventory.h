#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_Widget_Inventory.generated.h"

class UUniformGridPanel;
class UVerticalBox;
class UTexture2D;
class UEOB_InventoryComponent;
class UEOB_Widget_InventorySlot;
class UEOB_Widget_InventoryTab;
class UEOB_Widget_HeldItemIcon;

/**
 * 背包面板：左右两列页签（各 6 个）+ 当前页格子（格子数 = 该页背包容量）。
 * 蓝图子类需要三个同名容器：
 *  - VerticalBox_TabsLeft：左侧页签栏（第 1~6 页，从上到下）
 *  - VerticalBox_TabsRight：右侧页签栏（第 7~12 页，从上到下）
 *  - GridPanel_Items：物品格子容器
 * 左右页签各用一个皮肤类（TabWidgetClassLeft / TabWidgetClassRight），
 * 两个皮肤的父类都是 EOB_Widget_InventoryTab，只是 UMG 布局镜像。
 *
 * 本面板同时是"抓取/拖拽"的手势状态机：
 *  - 抓取：原地点击格子 = 拿起（源格变空、图标跟手）；再点一格 = 放下
 *  - 拖拽：按住移动超过 DragThresholdPixels 像素 = 拖拽，松开 = 落点结算
 *  - 落点在面板外（背包格来源）= 丢到地上；包裹栏位来源拖到空处 = 取消
 *  - 任何时候右键 = 取消手持、放回原处
 */
UCLASS()
class EMPIREOFBOSS_API UEOB_Widget_Inventory : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void RefreshUI();

	/** 刷新标签栏（激活状态、偏好名、当前页高亮、包裹栏位） */
	UFUNCTION()
	void RefreshTabs();

	/** 切换当前显示的标签页（0~11）；未激活的页拒绝切换 */
	UFUNCTION(BlueprintCallable, Category = "EOB|UI")
	void SetCurrentTab(int32 NewTab);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EOB|UI")
	int32 GetCurrentTab() const { return CurrentTabIndex; }

	/** 收起所有页签的"偏好"下拉面板（ExceptTabIndex 除外，-1 = 全部收起） */
	void CloseAllTabDropdowns(int32 ExceptTabIndex = -1);

	// ===================== 抓取 / 拖拽（格子控件调用） =====================

	/** 格子报告"左键按下了"：登记拖拽起点（位移超阈值才算拖拽） */
	void NotifySlotLeftPressed(UEOB_Widget_InventorySlot* InventorySlot, const FVector2D& ScreenPos);

	/** 格子报告"原地点击了"：手上没东西 = 拿起；手上有东西 = 放下 */
	void OnSlotGrabClicked(UEOB_Widget_InventorySlot* InventorySlot);

	/** 手上是否拿着物品（拿起后东西仍在原格，只是显示为空、图标跟手） */
	bool IsHoldingItem() const { return HeldSource != EEOBHeldSourceType::None; }

	/** 取消手持：图标消失、源格恢复显示（东西从头到尾没动过） */
	void CancelHeldItem();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

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

	/** 左侧页签皮肤类（WBP_InventoryTab_Left） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	TSubclassOf<UEOB_Widget_InventoryTab> TabWidgetClassLeft;

	/** 右侧页签皮肤类（WBP_InventoryTab_Right）。不填则 12 个页签都用左侧皮肤 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	TSubclassOf<UEOB_Widget_InventoryTab> TabWidgetClassRight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	int32 GridColumns = 8;

	/** 物品栏格子的边长（像素）。只作用于物品栏，装备面板格子不受影响 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	float InventorySlotSize = 110.f;

	/** 拖拽判定阈值（像素）：按住左键位移超过它才算拖拽，否则算"原地点击=抓取" */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	float DragThresholdPixels = 8.f;

	/** 手持图标的边长（像素） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	float HeldIconSize = 64.f;

	/** 当前显示的标签页（0~11），默认第 1 页 */
	UPROPERTY(BlueprintReadOnly, Category = "EOB|UI")
	int32 CurrentTabIndex = 0;

	UPROPERTY()
	TObjectPtr<UEOB_InventoryComponent> RefInventory;

private:
	/** 手持物品的来源 */
	enum class EEOBHeldSourceType : uint8
	{
		None, // 没拿东西
		InventorySlot, // 从背包格拿起
		BagSlot // 从包裹栏位拿起（拿的是装备的背包）
	};

	/** 从某格拿起（源格显示为空、手持图标出现）。空格/无背包则什么都不发生 */
	void TryPickUpFromSlot(UEOB_Widget_InventorySlot* InventorySlot);

	/** 把手上的东西放到目标格（按来源 × 目标模式分派到组件的对应函数） */
	void PlaceHeldOnSlot(UEOB_Widget_InventorySlot* TargetSlot);

	/** 拖拽松开的落点结算：落在格子上 = 放下；落在别处 = 丢地上（仅背包格来源）或取消 */
	void ResolveDropAtScreenPosition(const FVector2D& ScreenPos);

	/** 找屏幕坐标下的格子（12 个包裹栏位 + 当前页物品格），找不到返回 nullptr */
	UEOB_Widget_InventorySlot* FindSlotAtScreenPosition(const FVector2D& ScreenPos) const;

	/** 显示手持图标 */
	void BeginHeldIcon(UTexture2D* Icon);

	/** 清空手持状态；bRestoreSourceVisual = 把源格显示还原（取消/放回原处时用） */
	void ClearHeldItem(bool bRestoreSourceVisual);

	EEOBHeldSourceType HeldSource = EEOBHeldSourceType::None;
	int32 HeldTab = INDEX_NONE;
	int32 HeldSlot = INDEX_NONE;

	/** 被拿起后显示为空的源格子（弱引用：刷新重建后会自动失效） */
	TWeakObjectPtr<UEOB_Widget_InventorySlot> HeldGhostSlot;

	/** 拖拽起点登记（按下位置 + 按下的格子） */
	TWeakObjectPtr<UEOB_Widget_InventorySlot> PotentialDragSlot;
	FVector2D PotentialDragPos = FVector2D::ZeroVector;
	bool bDragging = false;

	/** 手持图标（运行时创建的纯 C++ 控件，自带跟随鼠标逻辑） */
	UPROPERTY()
	TObjectPtr<UEOB_Widget_HeldItemIcon> HeldIcon;
};
