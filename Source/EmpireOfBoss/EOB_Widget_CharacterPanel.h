#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_LevelComponent.h" // EEOBStatType
#include "EOB_Widget_CharacterPanel.generated.h"

class UTextBlock;
class UButton;

/**
 * M3a：角色面板（C 键开关）
 * 蓝图中控件名必须与下面的变量名完全一致；
 * 用 BindWidgetOptional：少放的控件不报错，只是那一项不刷新。
 */
UCLASS()
class EMPIREOFBOSS_API UEOB_Widget_CharacterPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 从英雄身上拉取最新数据，刷新所有文本（打开面板/加点后呼叫） */
	void RefreshFromHero();

protected:
	virtual void NativeConstruct() override;

	// ===================== 文本（名字必须一致） =====================
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Level;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_XP;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_StatPoints;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SkillPoints;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_STR;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_DEX;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_FOC;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_VIT;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Attack;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Armor;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CritChance;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_CritDamage;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Dodge;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SkillDmg;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_HP;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_MP;

	// ===================== 加点按钮 =====================
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_AddSTR;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_AddDEX;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_AddFOC;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_AddVIT;

	UFUNCTION()
	void OnAddSTRClicked();
	UFUNCTION()
	void OnAddDEXClicked();
	UFUNCTION()
	void OnAddFOCClicked();
	UFUNCTION()
	void OnAddVITClicked();

private:
	void SpendPoint(EEOBStatType Stat);
};
