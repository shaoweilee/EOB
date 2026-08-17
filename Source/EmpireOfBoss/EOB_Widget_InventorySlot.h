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
 * 交互约定（抓取 = 原地点击拿起/放下；拖拽 = 按住移动超过 8px 后松开；任何时候右键 = 取消手持）：
 *  - 背包格：左键拿起/放下；拿起背包物品点到包裹栏位 = 装备/换包；拖出面板 = 丢到地上；
 *    右键单击（手上没东西时）：装备穿到默认槽位，背包物品装到第一个空包裹栏位
 *  - 包裹栏位：左键拿起/放下（两个栏位间 = 背包互换位置，空栏位 = 挪过去）；
 *    右键单击（手上没东西时）= 取消装备包裹（包内有物品会被组件拒绝）
 *  - 装备格：左键 = 卸下（过渡方案，装备面板的抓取在下一阶段做）
 */
UCLASS()
class EMPIREOFBOSS_API UEOB_Widget_InventorySlot : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 背包格初始化：记录所属标签页 + 页内格号 + 所属背包面板 */
	void InitInventorySlot(UEOB_InventoryComponent* Inv, int32 InTabIndex, int32 InSlotInTab,
	                       UEOB_Widget_Inventory* OwnerPanel);

	/** 装备格初始化（装备面板用，不参与抓取/拖拽） */
	void InitEquipmentSlot(UEOB_InventoryComponent* Inv, EEOBEquipSlot InEquipSlot);

	/** 包裹栏位初始化：记录所属标签页 + 所属背包面板 */
	void InitBagSlot(UEOB_InventoryComponent* Inv, int32 InTabIndex, UEOB_Widget_Inventory* OwnerPanel);

	/** 刷新显示（图标 + 品质边框/辉光染色） */
	void UpdateSlot(const FEOBItemInstance& Item);

	/**
	 * 由"使用者"设定这个格子实例的期望尺寸（只影响本实例，不影响皮肤和其他面板）。
	 * 物品栏调用它把格子定成 110×110；装备面板不调用，格子保持原样。
	 */
	void SetSlotDesiredSize(float InSize);

	EEOBSlotWidgetMode GetMode() const { return Mode; }
	int32 GetTabIndex() const { return TabIndex; }
	int32 GetSlotInTab() const { return SlotInTab; }

protected:
	virtual void NativeConstruct() override;

	/** 右键单击走这里（Button 只响应左键，右键会冒泡到控件自身） */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** 左键按下的"预览"事件：先于 Button 处理到达本控件，用来登记拖拽起点 */
	virtual FReply
	NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** 根尺寸框（可选绑定：蓝图里把根尺寸框勾"是变量"、命名 SizeBox_Root 即可） */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<USizeBox> SizeBox_Root;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UButton> Button;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UImage> Image_icon;

	/** 品质边框（白色边框素材 × 品质色） */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UImage> Image_frame;

	/** 中心径向渐变（白色辉光素材 × 品质色） */
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UImage> Image_glow;

	UFUNCTION()
	void OnSlotClicked();

private:
	EEOBSlotWidgetMode Mode = EEOBSlotWidgetMode::Inventory;
	int32 TabIndex = 0;
	int32 SlotInTab = INDEX_NONE;
	EEOBEquipSlot EquipSlot = EEOBEquipSlot::Weapon;
	TWeakObjectPtr<UEOB_InventoryComponent> RefInventory;
	TWeakObjectPtr<UEOB_Widget_Inventory> RefPanel;
};
