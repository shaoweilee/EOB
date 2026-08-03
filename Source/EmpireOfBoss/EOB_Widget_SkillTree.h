#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EOB_Widget_SkillTree.generated.h"

class UTextBlock;
class UButton;

/**
 * M3b：技能树面板（K 键开关）
 * 8 个节点：Text_Skill0~7 显示状态，Btn_Skill0~7 点击学习
 * 控件名必须完全一致；少放不报错（Optional）
 */
UCLASS()
class EMPIREOFBOSS_API UEOB_Widget_SkillTree : public UUserWidget
{
	GENERATED_BODY()

public:
	void RefreshTree();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_SkillPoints;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Skill0;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Skill1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Skill2;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Skill3;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Skill4;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Skill5;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Skill6;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Skill7;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Skill0;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Skill1;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Skill2;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Skill3;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Skill4;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Skill5;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Skill6;
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Btn_Skill7;

	UFUNCTION()
	void OnSkill0Clicked();
	UFUNCTION()
	void OnSkill1Clicked();
	UFUNCTION()
	void OnSkill2Clicked();
	UFUNCTION()
	void OnSkill3Clicked();
	UFUNCTION()
	void OnSkill4Clicked();
	UFUNCTION()
	void OnSkill5Clicked();
	UFUNCTION()
	void OnSkill6Clicked();
	UFUNCTION()
	void OnSkill7Clicked();

private:
	void OnSkillNodeClicked(int32 Index);
};
