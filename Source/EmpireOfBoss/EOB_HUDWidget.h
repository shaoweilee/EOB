// UI 逻辑类,负责绑定和控制 Synty UI 蓝图中的具体控件

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_HUDWidget.generated.h"

class UProgressBar;
class UEOB_Widget_Inventory;
class UEOB_Widget_EquipmentPanel;

UCLASS()
class EMPIREOFBOSS_API UEOB_HUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// 🌟 核心绑定：名字必须和 Synty UI 蓝图里的血条组件名字完全一致
	// UPROPERTY(meta = (BindWidget))
	// UProgressBar* HealthBar;

public: /** 🌟 C++ 核心事件：当血量或蓝量改变时，C++ 呼叫这个函数，通知蓝图去刷新视觉效果 */
	UFUNCTION(BlueprintImplementableEvent, Category = "EOB|UI")
	void VM_UpdateHPVisual(float HealthPercent);
	UFUNCTION(BlueprintImplementableEvent, Category = "EOB|UI")
	void VM_UpdateMPVisual(float ManaPercent);

	// 敌人血条
	UFUNCTION(BlueprintImplementableEvent)
	void ShowStateBar(ESlateVisibility IsShow);

	UFUNCTION(BlueprintImplementableEvent)
	void BP_UpdateEnemyHP(float HealthPercent);

	UFUNCTION(BlueprintImplementableEvent)
	void BP_UpdateXP(int CurrentXP, int MaxXP);

	// ===================== M2 新增：背包/装备面板管理 =====================

	/** 开关背包和装备面板（由 PlayerController 的 I 键触发） */
	UFUNCTION(BlueprintCallable, Category = "EOB|UI")
	void ToggleInventoryPanels();

protected:
	/** 在 HUD 蓝图默认值里选你的 WBP_Inventory / WBP_EquipmentPanel */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	TSubclassOf<UEOB_Widget_Inventory> InventoryPanelClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	TSubclassOf<UEOB_Widget_EquipmentPanel> EquipmentPanelClass;

	UPROPERTY()
	TObjectPtr<UEOB_Widget_Inventory> InventoryPanel;

	UPROPERTY()
	TObjectPtr<UEOB_Widget_EquipmentPanel> EquipmentPanel;
};
