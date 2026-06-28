// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "CPP_Widget_ActionBar_Item.generated.h"

/**
 * 
 */
UCLASS()
class EMPIREOFBOSS_API UCPP_Widget_ActionBar_Item : public UUserWidget
{
	GENERATED_BODY()

protected:
	// UMG 生命周期初始化时绑定事件
	virtual void NativeConstruct() override;

	// 🌟 核心魔法：名字必须和你图片里圈出的最底层原生 UMG Button 名字完全一致（全大写 BUTTON）
	UPROPERTY(meta = (BindWidget), BlueprintReadOnly, Category = "EOB|UI")
	class UButton* BUTTON;

	// 🌟 每个技能格子专属的 GameplayTag（例如：Gameplay.Ability.Skill_Q）
	// 允许在蓝图实例上自由指定，实现“格子与技能”的解耦
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Skill")
	FGameplayTag AssignedSkillTag;

	// 点击按钮后的底层 C++ 回调
	UFUNCTION()
	void OnSlotClicked();
};
