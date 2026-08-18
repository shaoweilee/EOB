#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_ItemTypes.h"
#include "EOB_Widget_Inventory.generated.h"

class UEOB_InventoryComponent;
class UUniformGridPanel;
class UVerticalBox;
class UEOB_Widget_InventorySlot;
class UEOB_Widget_InventoryTab;
class UEOB_Widget_HeldItemIcon;
class UEOB_Widget_HoldClickCatcher;
class UTexture2D;

/** 手持物品的来源类型 */
enum class EEOBHeldSourceType : uint8
{
	None, // 空手
	InventorySlot, // 背包格（懒转移：数据仍在原格，取消零成本）
	BagSlot, // 包裹栏位（拿的是整个包；懒转移）
	EquipmentSlot // 装备槽（真脱下：拿起即移属性，实例由本面板持有）
};

/**
 * 背包主面板 = 全部格子手势的"总管"：
 * - GridPanel_Items 显示当前背包页的物品；左右两列页签共 12 个
 * - 抓取（原地点击）/ 拖拽（按住移动超阈值）/ 页签投放 / 面板外丢弃，统一在本类结算
 * - 手持物品通过 HeldIcon 图标跟手显示（HitTestInvisible，不挡点击）
 * - 手持期间张开全屏透明点击层（ClickCatcher）：点面板外空地 = 丢弃/取消，不会误触角色移动
 * - 手持时每帧刷新"禁止投放"红框：鼠标下不接受当前物品的格子（含装备面板的格子）边框染红
 *
 * 装备槽手势（数据层真脱下模型）：
 *  - 拿起 = 真正卸下（属性立即刷新），实例存在本面板的 HeldEquipmentItem 里
 *  - 放到兼容装备槽 = 穿上；槽位有货 = 交换，旧装备进入抓取状态继续跟手
 *  - 点到不兼容的装备槽 = 无反应（抓取）/ 穿回原始槽位（拖拽松手）
 *  - 点背包空格 = 放入；点有装备的背包格 = 仅当它能穿到【原始槽位】时互换
 *  - 点页签按钮 = 放进该页第一个空格（页满：抓取无反应 / 拖拽穿回原始槽位）
 *  - 右键 = 穿回原始槽位；面板外空地 = 丢到地上；所有面板空隙 = 无反应
 */
UCLASS()
class EMPIREOFBOSS_API UEOB_Widget_Inventory : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 切换到某个背包页（未激活的页不可切换；手持有物品时调用 = 把手持物移进该页，不切页） */
	void SetCurrentTab(int32 NewTab);

	/** 当前显示的背包页下标（页签控件用来判断自己是不是当前页） */
	int32 GetCurrentTab() const { return CurrentTabIndex; }

	/** 关闭所有页签下拉框（可豁免一个——刚打开的那个） */
	void CloseAllTabDropdowns(int32 ExceptTabIndex = INDEX_NONE);

	/** 让所有页签按背包组件当前状态刷新显示 */
	void RefreshTabs();

	/** 当前是否有拿在手上的物品 */
	bool IsHoldingItem() const { return HeldSource != EEOBHeldSourceType::None; }

	/** 格子控件回调：左键按下的瞬间（登记拖拽起点） */
	void NotifySlotLeftPressed(UEOB_Widget_InventorySlot* InSlot, const FVector2D& ScreenPos);

	/** 格子控件回调：原地点击（松手时位移未超阈值）= 抓取手势的"拿起/放下" */
	void OnSlotGrabClicked(UEOB_Widget_InventorySlot* InSlot);

	/** 取消拿起：背包/包裹来源 = 恢复源格显示；装备来源 = 穿回原始槽位（属性重新生效） */
	void CancelHeldItem();

	/** 点击层回调：手持期间点到面板外空地（丢弃按来源规则结算；落在面板空隙 = 无反应） */
	void NotifyWorldLeftClickWhileHolding(const FVector2D& ScreenPos);

	/** 当前唯一的背包面板实例（装备面板/格子的面板引用都靠它；构造登记、析构注销） */
	static UEOB_Widget_Inventory* GetInstance() { return Instance.Get(); }

	/**
	 * 所有"代表面板范围"的控件（各面板的底图/外框，构造时自行登记）。
	 * 手持物落在这些控件的几何范围内（含面板空隙）= 不触发丢弃，按取消/无反应处理。
	 * 注意：登记的一定要是"真正贴着面板边框"的那个控件，别用可能铺满全屏的根节点。
	 */
	static TArray<TWeakObjectPtr<UWidget>>& GetAllPanelBoundsWidgets();

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
	float InventorySlotSize = 110.f;

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
	/** 当前唯一的背包面板实例（弱引用，防 GC 误留） */
	static TWeakObjectPtr<UEOB_Widget_Inventory> Instance;

	/** 主角的背包组件 */
	UPROPERTY()
	TObjectPtr<UEOB_InventoryComponent> RefInventory;

	/** 手持物品的来源类型（None = 空手） */
	EEOBHeldSourceType HeldSource = EEOBHeldSourceType::None;

	/** 手持来源：背包格时为所在页，包裹栏位时为页签下标 */
	int32 HeldTab = INDEX_NONE;

	/** 手持来源：背包格的格内下标（包裹栏位/装备槽来源时恒为 INDEX_NONE） */
	int32 HeldSlot = INDEX_NONE;

	/** 手持来源：装备槽的槽位身份（装备的"原始槽位"，穿回/互换判定都用它） */
	EEOBEquipSlot HeldEquipSlot = EEOBEquipSlot::Weapon;

	/** 手持来源：装备槽时，被脱下的装备实例本体（真脱下模型，数据不在任何容器里） */
	UPROPERTY()
	FEOBItemInstance HeldEquipmentItem;

	/** 被拿起的源格控件（幽灵格，拿起期间显示为空；仅背包格/包裹栏位来源用） */
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

	/** 手持期间的全屏透明点击层（视口最底层；点面板外空地 = 丢弃/取消，右键 = 取消） */
	UPROPERTY()
	TObjectPtr<UEOB_Widget_HoldClickCatcher> ClickCatcher;

	/** 尝试从指定格子拿起物品（装备槽 = 真脱下；背包格/包裹栏位 = 懒转移 + 源格幽灵化） */
	void TryPickUpFromSlot(UEOB_Widget_InventorySlot* InSlot);

	/**
	 * 把手上的东西放到目标格子（交换/穿装备/装备包裹/互换页签等，按来源与目标类型分发）。
	 * 返回 true = 落点接受（注意：装备槽交换后手持会继续拿着换下来的旧装备）；
	 * 返回 false = 目标不接受（红框规则），手持仍在——调用方决定保持手持（抓取点击）还是取消（拖拽松手）。
	 */
	bool PlaceHeldOnSlot(UEOB_Widget_InventorySlot* TargetSlot);

	/**
	 * 把手上的东西放进某页的第一个空格（点/拖到页签按钮）。
	 * 成功 = 手持结束；失败（页满/未激活/同页）：抓取 = 无反应保持手持，拖拽 = 取消回原位。
	 */
	void PlaceHeldOnTab(int32 TabIndex, bool bFromDrag);

	/** 拖拽松手时对落点结算：格子→放下；页签→进该页；面板外空地→丢弃；其余（含面板空隙）→取消回原位 */
	void ResolveDropAtScreenPosition(const FVector2D& ScreenPos);

	/** 查找屏幕坐标下的格子控件（全局注册表：背包格/包裹栏位/装备格都在里面） */
	UEOB_Widget_InventorySlot* FindSlotAtScreenPosition(const FVector2D& ScreenPos) const;

	/** 查找屏幕坐标下的页签下标（页签按钮整体，不含已先行命中的包裹栏位格子）；找不到返回 INDEX_NONE */
	int32 FindTabIndexAtScreenPosition(const FVector2D& ScreenPos) const;

	/** 该屏幕坐标是否落在"本面板或任一已登记面板范围控件"内（含空隙；用于丢弃判定，命中时会打日志） */
	bool IsScreenPositionOverAnyPanel(const FVector2D& ScreenPos) const;

	/** 每帧刷新"禁止投放"红框：手持时，鼠标下不接受当前物品的格子染红（含装备面板格子） */
	void UpdateForbiddenHighlights(const FVector2D& ScreenPos);

	/**
	 * 判定目标格子按当前规则能否接受手持物（纯判定，不动数据）。
	 * 以后做"禁止装备"（职业/等级不符等）也在这个函数里加规则。
	 */
	bool WouldAcceptDrop(const UEOB_Widget_InventorySlot* TargetSlot) const;

	/** 手持的是"已装备的包"时，包内是否为空（非空不能卸下） */
	bool IsHeldBagEmpty() const;

	/** 显示跟手图标 + 张开全屏点击层（拿起时调用） */
	void BeginHeldIcon(UTexture2D* Icon);

	/** 结束手持状态；bRestoreSourceVisual=true 时恢复原位（背包/包裹 = 恢复源格显示；装备 = 穿回原始槽位） */
	void ClearHeldItem(bool bRestoreSourceVisual);
};
