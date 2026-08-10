#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_ItemTypes.h"
#include "EOB_Widget_InventorySlot.generated.h"

class UButton;
class UImage;
class UEOB_InventoryComponent;

/** 格子工作模式：背包格 / 装备格 */
UENUM(BlueprintType)
enum class EEOBSlotWidgetMode : uint8
{
	Inventory,
	Equipment
};

/**
 * 背包/装备格子基类：逻辑全在 C++，蓝图子类只负责画。
 * 蓝图里必须包含四个同名控件：Button、Image_icon、Image_frame、Image_glow（BindWidget 自动绑定）
 * 层级从下到上：Image_glow（径向辉光）→ Image_frame（品质边框）→ Button（透明，管点击）→ Image_icon
 *
 * 交互约定：
 *  - 右键单击背包格：装备穿到默认槽位；背包物品装到第一个空背包栏位
 *  - 左键单击：留给"抓取/拖拽"（下一步实现），目前背包格无动作；装备格左键 = 卸下（过渡方案）
 */
UCLASS()
class EMPIREOFBOSS_API UEOB_Widget_InventorySlot : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 背包格初始化：记录所属标签页 + 页内格号 */
	void InitInventorySlot(UEOB_InventoryComponent* Inv, int32 InTabIndex, int32 InSlotInTab);

	/** 装备格初始化 */
	void InitEquipmentSlot(UEOB_InventoryComponent* Inv, EEOBEquipSlot InEquipSlot);

	/** 刷新显示（图标 + 品质边框/辉光染色） */
	void UpdateSlot(const FEOBItemInstance& Item);

protected:
	virtual void NativeConstruct() override;

	/** 右键单击走这里（Button 只响应左键，右键会冒泡到控件自身） */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

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
};
