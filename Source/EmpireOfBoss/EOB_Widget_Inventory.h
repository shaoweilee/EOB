#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_Widget_Inventory.generated.h"

class UUniformGridPanel;
class UEOB_InventoryComponent;
class UEOB_Widget_InventorySlot;

/**
 * 背包面板：自动订阅背包事件、自动铺满格子。
 * 蓝图子类只需放一个同名 Uniform Grid Panel：GridPanel_Items
 */
UCLASS()
class EMPIREOFBOSS_API UEOB_Widget_Inventory : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void RefreshUI();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UUniformGridPanel> GridPanel_Items;

	/** 格子控件类（蓝图默认值里选 WBP_InventorySlot） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	TSubclassOf<UEOB_Widget_InventorySlot> SlotWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	int32 GridColumns = 4;

	UPROPERTY()
	TObjectPtr<UEOB_InventoryComponent> RefInventory;
};
