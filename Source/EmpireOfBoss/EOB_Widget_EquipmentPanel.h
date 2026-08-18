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
 * 可选控件：PanelBounds（底图/最外层容器，任意控件类型，勾"是变量"并命名 PanelBounds）。
 * 它的几何范围被登记为"面板范围"：手持物落在上面 = 面板空隙 = 无反应，不会误丢到地上。
 * 不绑定则退回用根控件——如果根控件铺满整张设计画布，丢弃判定会失灵，建议务必绑定。
 *
 * 本面板只做两件事：
 *  1. 把 10 个格子登记成"装备格"并指向手势总管（背包面板），抓取/拖拽/右键全由总管结算；
 *  2. 订阅装备/背包变化事件刷新 10 个格子的显示。
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

	/**
	 * 兜底吞掉"面板空隙"上的点击（装备格处理过的点击不会冒泡到这里），
	 * 防止穿透到游戏世界让角色移动；右键 = 取消手持（装备穿回原始槽位）。
	 */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply
	NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

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

	/** 可选：面板范围框（把包住整个面板的底图/最外层容器命名 PanelBounds 并勾"是变量"） */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "EOB|UI")
	TObjectPtr<UWidget> PanelBounds;

	UPROPERTY()
	TObjectPtr<UEOB_InventoryComponent> RefInventory;

private:
	/** 已登记进"面板范围列表"的控件（析构时注销用） */
	TWeakObjectPtr<UWidget> RegisteredBounds;
};
