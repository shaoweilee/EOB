#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_ItemTypes.h"
#include "EOB_Widget_InventorySlot.generated.h"

class UButton;
class UImage;
class USizeBox;
class UEOB_InventoryComponent;
class UEOB_Widget_Inventory;

/** 格子工作模式：背包格 / 装备格 / 包裹栏位（页签左侧、装备背包的地方） */
UENUM(BlueprintType)
enum class EEOBSlotWidgetMode : uint8
{
	Inventory,
	Equipment,
	BagSlot
};

/**
 * 背包/装备/包裹栏位格子基类：逻辑全在 C++，蓝图子类只负责画。
 * 蓝图里必须包含四个同名控件：Button、Image_icon、Image_frame、Image_glow（BindWidget 自动绑定）
 * 层级从下到上：Image_glow（径向辉光）→ Image_frame（品质边框）→ Button（透明，管点击）→ Image_icon
 *
 * 可选控件：SizeBox_Root（根尺寸框，勾"是变量"并命名）。
 * 皮肤本身不定死尺寸——物品栏/页签创建格子时调用 SetSlotDesiredSize 定尺寸；
 * 装备面板不调用，格子保持设计器里的大小。
 *
 * 禁止投放提示：面板在每帧检测"鼠标下的格子能不能接受手持物"，不能就调 SetForbiddenHighlight(true)，
 * 本控件把 Image_frame 整框染红；恢复时用缓存的 LastItem 重染品质色，蓝图零改动。
 * 所有格子实例在构造时登记进全局列表（GetAllSlotWidgets），因此装备面板的格子也能被红框覆盖到。
 *
 * 交互约定（抓取 = 原地点击拿起/放下；拖拽 = 按住移动超过阈值后松开；任何时候右键 = 取消手持）：
 *  - 三种模式的左键按下都在预览阶段被吞掉（Button 不捕获鼠标，跟手图标才有实时鼠标坐标），
 *    "原地点击"由背包面板 Tick 在松开时补发：拿起 / 放下 / 交换
 *  - 背包格：右键单击（手上没东西时）= 装备穿到默认槽位 / 背包物品装到第一个空包裹栏位
 *  - 包裹栏位：右键单击（手上没东西时）= 取消装备包裹（包内有物品会被组件拒绝）
 *  - 装备格：拿起 = 真正脱下（属性立即刷新），放下规则全部由背包面板结算
 */
UCLASS()
class EMPIREOFBOSS_API UEOB_Widget_InventorySlot : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 背包格初始化：记录所属标签页 + 页内格号 + 所属背包面板 */
	void InitInventorySlot(UEOB_InventoryComponent* Inv, int32 InTabIndex, int32 InSlotInTab,
	                       UEOB_Widget_Inventory* OwnerPanel);

	/** 装备格初始化：记录槽位身份 + 手势总管（背包面板；装备格的抓取/拖拽同样由它统一结算） */
	void InitEquipmentSlot(UEOB_InventoryComponent* Inv, EEOBEquipSlot InEquipSlot,
	                       UEOB_Widget_Inventory* OwnerPanel);

	/** 包裹栏位初始化：记录所属标签页 + 所属背包面板 */
	void InitBagSlot(UEOB_InventoryComponent* Inv, int32 InTabIndex, UEOB_Widget_Inventory* OwnerPanel);

	/** 刷新显示（图标 + 品质边框/辉光染色）。会缓存 LastItem 供红框恢复用 */
	void UpdateSlot(const FEOBItemInstance& Item);

	/**
	 * 由"使用者"设定这个格子实例的期望尺寸（只影响本实例，不影响皮肤和其他面板）。
	 * 物品栏调用它把格子定成 110×110；装备面板不调用，格子保持原样。
	 */
	void SetSlotDesiredSize(float InSize);

	/** 禁止投放红框：true = 品质框整框染红；false = 按缓存物品恢复品质色 */
	void SetForbiddenHighlight(bool bOn);

	/** 当前所有格子实例（弱引用，构造时登记、析构时注销；含装备面板的格子） */
	static TArray<TWeakObjectPtr<UEOB_Widget_InventorySlot>>& GetAllSlotWidgets();

	EEOBSlotWidgetMode GetMode() const { return Mode; }
	int32 GetTabIndex() const { return TabIndex; }
	int32 GetSlotInTab() const { return SlotInTab; }
	EEOBEquipSlot GetEquipSlot() const { return EquipSlot; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 右键单击走这里（Button 只响应左键，右键会冒泡到控件自身） */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** 左键按下的"预览"事件：先于 Button 处理到达本控件，用来登记拖拽起点并吞掉事件（防鼠标捕获） */
	virtual FReply
	NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** 根尺寸框（可选绑定：蓝图里把根尺寸框勾"是变量"、命名 SizeBox_Root 即可） */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<USizeBox> SizeBox_Root;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UButton> Button;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UImage> Image_icon;

	/** 品质边框（白色边框素材 × 品质色；禁止投放时整框染红） */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UImage> Image_frame;

	/** 中心径向渐变（白色辉光素材 × 品质色） */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UImage> Image_glow;

private:
	EEOBSlotWidgetMode Mode = EEOBSlotWidgetMode::Inventory;
	int32 TabIndex = 0;
	int32 SlotInTab = INDEX_NONE;
	EEOBEquipSlot EquipSlot = EEOBEquipSlot::Weapon;
	TWeakObjectPtr<UEOB_InventoryComponent> RefInventory;
	TWeakObjectPtr<UEOB_Widget_Inventory> RefPanel;

	/** 是否正亮着"禁止投放"红框 */
	bool bForbidden = false;

	/** 最近一次 UpdateSlot 的物品（红框恢复时按它重染品质色） */
	FEOBItemInstance LastItem;
};
