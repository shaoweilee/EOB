#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_Widget_Inventory.generated.h"

class UEOB_InventoryComponent;
class UUniformGridPanel;
class UVerticalBox;
class UEOB_Widget_InventorySlot;
class UEOB_Widget_InventoryTab;
class UEOB_Widget_HeldItemIcon;
class UTexture2D;

/** 手持物品的来源类型 */
enum class EEOBHeldSourceType : uint8
{
	None, // 空手
	InventorySlot, // 背包格
	BagSlot // 包裹栏位（拿的是整个包）
};

/**
 * 背包主面板：
 * - GridPanel_Items 显示当前背包页的物品
 * - 左右两列页签（VerticalBox_TabsLeft / VerticalBox_TabsRight），共 12 个
 * - 抓取（原地点击）/ 拖拽（按住移动超阈值）/ 拖出面板丢弃，由本类集中处理
 * - 手持物品通过 HeldIcon 图标跟手显示（HitTestInvisible，不挡点击）
 *
 * 抓取：原地点击 = 拿起（东西仍在原格，源格变空 + 图标跟手）；再点目标格 = 放下/交换。
 * 拖拽：按下移动超过 DragThresholdPixels 后松手 = 对落点结算；落在面板外 = 丢到地上（仅背包格来源）。
 * 任何时刻右键 = 取消拿起（东西回源格显示，数据从未动过）。
 */
UCLASS()
class EMPIREOFBOSS_API UEOB_Widget_Inventory : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 切换到某个背包页（未激活的页不可切换） */
	void SetCurrentTab(int32 NewTab);

	/** 关闭所有页签下拉框（可豁免一个——刚打开的那个） */
	void CloseAllTabDropdowns(int32 ExceptTabIndex = INDEX_NONE);

	/** 让所有页签按背包组件当前状态刷新显示 */
	void RefreshTabs();

	/** 当前是否有拿在手上的物品 */
	bool IsHoldingItem() const { return HeldSource != EEOBHeldSourceType::None; }

	/** 格子控件回调：左键按下的瞬间（登记拖拽起点） */
	void NotifySlotLeftPressed(UEOB_Widget_InventorySlot* Slot, const FVector2D& ScreenPos);

	/** 格子控件回调：原地点击（松手时位移未超阈值）= 抓取手势的"拿起/放下" */
	void OnSlotGrabClicked(UEOB_Widget_InventorySlot* Slot);

	/** 取消拿起：手上的东西回源格（数据从未离开过，只需恢复源格显示 + 收图标） */
	void CancelHeldItem();

protected:
	/** 左侧页签用的控件蓝图（父类应为 EOB_Widget_InventoryTab，例如 WBP_InventoryTab_Left） */
	UPROPERTY(EditAnywhere, Category = "EOB|Inventory")
	TSubclassOf<UEOB_Widget_InventoryTab> TabWidgetClassLeft;

	/** 右侧页签用的控件蓝图（例如 WBP_InventoryTab_Right）；留空则右侧也用左侧的类 */
	UPROPERTY(EditAnywhere, Category = "EOB|Inventory")
	TSubclassOf<UEOB_Widget_InventoryTab> TabWidgetClassRight;

	/** 物品格子用的控件蓝图（父类应为 EOB_Widget_InventorySlot） */
	UPROPERTY(EditAnywhere, Category = "EOB|Inventory")
	TSubclassOf<UEOB_Widget_InventorySlot> SlotWidgetClass;

	/** 手持图标用的控件蓝图（父类应为 EOB_Widget_HeldItemIcon，例如 WBP_HeldItemIcon）；留空则用纯 C++ 兜底 */
	UPROPERTY(EditAnywhere, Category = "EOB|Inventory")
	TSubclassOf<UEOB_Widget_HeldItemIcon> HeldIconWidgetClass;

	/** 网格每行多少列（决定 GridPanel_Items 的列数） */
	UPROPERTY(EditAnywhere, Category = "EOB|Inventory")
	int32 GridColumns = 8;

	/** 背包格子边长（像素） */
	UPROPERTY(EditAnywhere, Category = "EOB|Inventory")
	float InventorySlotSize = 67.f;

	/** 拖拽阈值（像素）：按下后位移超过它才算拖拽，否则视为抓取手势的原地点击 */
	UPROPERTY(EditAnywhere, Category = "EOB|Inventory")
	float DragThresholdPixels = 8.f;

	/** 手持图标的边长（像素） */
	UPROPERTY(EditAnywhere, Category = "EOB|Inventory")
	float HeldIconSize = 64.f;

	/** 物品网格容器（WBP 里同名 UniformGridPanel，父级建议是画布槽并勾"大小到内容"） */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> GridPanel_Items;

	/** 左侧页签容器（WBP 里同名 VerticalBox，装第 1~6 页） */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VerticalBox_TabsLeft;

	/** 右侧页签容器（WBP 里同名 VerticalBox，装第 7~12 页） */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VerticalBox_TabsRight;

	/** 当前显示的背包页下标 */
	UPROPERTY(BlueprintReadOnly, Category = "EOB|Inventory")
	int32 CurrentTabIndex = 0;

	/** 蓝图事件：物品或装备变化后调用（WBP 里可重载做额外刷新，如包裹总容量显示） */
	UFUNCTION(BlueprintNativeEvent, Category = "EOB|Inventory")
	void RefreshUI();
	virtual void RefreshUI_Implementation();

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** 主角的背包组件 */
	UPROPERTY()
	TObjectPtr<UEOB_InventoryComponent> RefInventory;

	/** 手持物品的来源类型（None = 空手） */
	EEOBHeldSourceType HeldSource = EEOBHeldSourceType::None;

	/** 手持来源：背包格时为所在页，包裹栏位时为页签下标 */
	int32 HeldTab = INDEX_NONE;

	/** 手持来源：背包格的格内下标（包裹栏位来源时恒为 INDEX_NONE） */
	int32 HeldSlot = INDEX_NONE;

	/** 被拿起的源格控件（幽灵格，拿起期间显示为空；取消/放下时恢复或刷新） */
	TWeakObjectPtr<UEOB_Widget_InventorySlot> HeldGhostSlot;

	/** 左键按下瞬间登记的潜在拖拽起点（原地松手=抓取，移动超阈值=拖拽） */
	TWeakObjectPtr<UEOB_Widget_InventorySlot> PotentialDragSlot;

	/** 左键按下瞬间的屏幕坐标 */
	FVector2D PotentialDragPos = FVector2D::ZeroVector;

	/** 当前是否处于"拖拽中"（用于区分原地点击的抓取手势） */
	bool bDragging = false;

	/** 跟手的手持物品图标（NativeConstruct 里创建，视口最高层） */
	UPROPERTY()
	TObjectPtr<UEOB_Widget_HeldItemIcon> HeldIcon;

	/** 尝试从指定格子拿起物品（成功则源格变空 + 图标跟手；数据仍在原格） */
	void TryPickUpFromSlot(UEOB_Widget_InventorySlot* Slot);

	/** 把手上的东西放到目标格子（交换/装备包裹/互换页签等，按来源与目标类型分发） */
	void PlaceHeldOnSlot(UEOB_Widget_InventorySlot* TargetSlot);

	/** 拖拽松手时对落点结算：格子→放下；面板外→丢弃（仅背包格来源）；否则→取消 */
	void ResolveDropAtScreenPosition(const FVector2D& ScreenPos);

	/** 查找屏幕坐标下的格子控件（先查 12 个包裹栏位，再查物品网格） */
	UEOB_Widget_InventorySlot* FindSlotAtScreenPosition(const FVector2D& ScreenPos) const;

	/** 显示跟手图标（拿起时调用） */
	void BeginHeldIcon(UTexture2D* Icon);

	/** 结束手持状态；bRestoreSourceVisual=true 时恢复源格显示（取消/原路放回） */
	void ClearHeldItem(bool bRestoreSourceVisual);
};
