#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_ItemTypes.h"
#include "EOB_Widget_EquipmentPanel.generated.h"

class UEOB_InventoryComponent;
class UEOB_Widget_InventorySlot;

/**
 * 装备面板：10 个装备格在蓝图上手动摆放（复用背包格子控件 WBP_InventorySlot）。
 * 蓝图子类必须放 10 个同名格子控件，缺一不可、名字一字不差：
 * Slot_Weapon / Slot_Shield / Slot_Helmet / Slot_Chest / Slot_Gloves /
 * Slot_Boots / Slot_Belt / Slot_Amulet / Slot_RingLeft / Slot_RingRight
 *
 * 本面板只做两件事：
 *  1. 把 10 个格子登记成"装备格"并指向手势总管（背包面板），抓取/拖拽/右键全由总管结算；
 *  2. 订阅装备/背包变化事件刷新 10 个格子的显示。
 * 另外会把自己登记进"挡丢弃面板"列表：手持物落在本面板的空隙上 = 无反应，不会误丢到地上。
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
	virtual void NativeDestruct() override;

	/** 刷新单个格子（有装备显示装备，无装备显示空槽） */
	void UpdateOneSlot(UEOB_Widget_InventorySlot* Widget, EEOBEquipSlot SlotId) const;

	// ===== 10 个手动摆放的装备格（BindWidget 按名字绑定） =====
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UEOB_Widget_InventorySlot> Slot_Weapon;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UEOB_Widget_InventorySlot> Slot_Shield;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UEOB_Widget_InventorySlot> Slot_Helmet;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UEOB_Widget_InventorySlot> Slot_Chest;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UEOB_Widget_InventorySlot> Slot_Gloves;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UEOB_Widget_InventorySlot> Slot_Boots;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UEOB_Widget_InventorySlot> Slot_Belt;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UEOB_Widget_InventorySlot> Slot_Amulet;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UEOB_Widget_InventorySlot> Slot_RingLeft;

	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UEOB_Widget_InventorySlot> Slot_RingRight;

	UPROPERTY()
	TObjectPtr<UEOB_InventoryComponent> RefInventory;
};
