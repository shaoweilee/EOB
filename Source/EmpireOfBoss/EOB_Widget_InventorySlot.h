#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_ItemTypes.h"
#include "EOB_Widget_InventorySlot.generated.h"

class UButton;
class UImage;
class UEOB_InventoryComponent;

/** 格子工作模式：背包格（点击穿上）/ 装备格（点击卸下） */
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
 */
UCLASS()
class EMPIREOFBOSS_API UEOB_Widget_InventorySlot : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 背包格初始化：点击 = 穿上这一格 */
	void InitInventorySlot(UEOB_InventoryComponent* Inv, int32 InSlotIndex);

	/** 装备格初始化：点击 = 卸下这一槽 */
	void InitEquipmentSlot(UEOB_InventoryComponent* Inv, EEOBEquipSlot InEquipSlot);

	/** 刷新显示（图标 + 品质边框/辉光染色） */
	void UpdateSlot(const FEOBItemInstance& Item);

protected:
	virtual void NativeConstruct() override;

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
	int32 SlotIndex = INDEX_NONE;
	EEOBEquipSlot EquipSlot = EEOBEquipSlot::Weapon;
	TWeakObjectPtr<UEOB_InventoryComponent> RefInventory;
};
