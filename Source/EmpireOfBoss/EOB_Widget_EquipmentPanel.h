#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_ItemTypes.h"
#include "EOB_Widget_EquipmentPanel.generated.h"

class UPanelWidget;
class UEOB_InventoryComponent;
class UEOB_Widget_InventorySlot;

/**
 * 装备面板：自动生成 10 个槽位格（复用背包格子控件）。
 * 蓝图子类只需放一个同名容器（Horizontal Box / 任意面板）：EquipSlotsBox
 */
UCLASS()
class EMPIREOFBOSS_API UEOB_Widget_EquipmentPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void RefreshEquip();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UPanelWidget> EquipSlotsBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	TSubclassOf<UEOB_Widget_InventorySlot> SlotWidgetClass;

	UPROPERTY()
	TObjectPtr<UEOB_InventoryComponent> RefInventory;

	UPROPERTY()
	TArray<TObjectPtr<UEOB_Widget_InventorySlot>> SlotWidgets;
};
