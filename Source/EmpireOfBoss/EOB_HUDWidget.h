// UI 逻辑类,负责绑定和控制 Synty UI 蓝图中的具体控件

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_HUDWidget.generated.h"

class UProgressBar;
class UEOB_Widget_Inventory;
class UEOB_Widget_EquipmentPanel;
class UEOB_Widget_CharacterPanel;
class UEOB_Widget_SkillTree;

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
	void BP_UpdateEnemyText(const FText& TextName, const FText& TextHealth);

	UFUNCTION(BlueprintImplementableEvent)
	void BP_UpdateXP(int CurrentXP, int MaxXP);

	// ===================== M2 新增：背包/装备面板管理 =====================

	/** 开关背包和装备面板（由 PlayerController 的 I 键触发） */
	UFUNCTION(BlueprintCallable, Category = "EOB|UI")
	void ToggleInventoryPanels();

	/** M3a：开关角色面板（C 键） */
	UFUNCTION(BlueprintCallable, Category = "EOB|UI")
	void ToggleCharacterPanel();

	/** M3a：从英雄的 LevelComponent 读经验，驱动现成的 BP_UpdateXP 事件刷新 Synty 经验条 */
	void RefreshXPDisplay();

	/** M3a：若角色面板开着则刷新（加点/升级后呼叫） */
	void RefreshCharacterPanel();

	/** M3b：开关技能树面板（K 键） */
	UFUNCTION(BlueprintCallable, Category = "EOB|UI")
	void ToggleSkillTreePanel();

	/** M3b：若技能树面板开着则刷新（学习技能后呼叫） */
	void RefreshSkillTreePanel();

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

	// ===================== M3a：角色面板（Synty UI 没有，自建） =====================
	/** 在 HUD 蓝图默认值里选你的 WBP_CharacterPanel */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	TSubclassOf<UEOB_Widget_CharacterPanel> CharacterPanelClass;

	UPROPERTY()
	TObjectPtr<UEOB_Widget_CharacterPanel> CharacterPanel;

	// ===================== M3b：技能树面板 =====================
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|UI")
	TSubclassOf<UEOB_Widget_SkillTree> SkillTreePanelClass;

	UPROPERTY()
	TObjectPtr<UEOB_Widget_SkillTree> SkillTreePanel;
};
