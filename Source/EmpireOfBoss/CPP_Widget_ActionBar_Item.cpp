// Fill out your copyright notice in the Description page of Project Settings.


#include "CPP_Widget_ActionBar_Item.h"
#include "Components/Button.h"
#include "AbilitySystemComponent.h"
#include "EmpireOfBossCharacter.h"

void UCPP_Widget_ActionBar_Item::NativeConstruct()
{
	Super::NativeConstruct(); // 自动安全绑定底层标准 Button 的点击事件
	if (BUTTON)
	{
		BUTTON->OnClicked.AddDynamic(this, &UCPP_Widget_ActionBar_Item::OnSlotClicked);
	}
}

void UCPP_Widget_ActionBar_Item::OnSlotClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("---按了下技能---"));
	// 🌟 极其优雅的地方：任何嵌套深处的 Widget 都能通过 GetOwningPlayer 直接拿到 PC
	if (AEmpireOfBossCharacter* MyHero = Cast<AEmpireOfBossCharacter>(GetOwningPlayerPawn()))
	{
		if (MyHero && MyHero->AbilitySystemComponent && AssignedSkillTag.IsValid())
		{
			// 🌟 暗黑流标准闭环：点击格子，直接通过标签让 GAS 释放对应技能！
			FGameplayTagContainer AbilityContainer(AssignedSkillTag);
			MyHero->AbilitySystemComponent->TryActivateAbilitiesByTag(AbilityContainer);
			UE_LOG(LogTemp, Log, TEXT("EOB_UI: 成功通过点击技能格子触发 GAS 技能标签 [%s]"), *AssignedSkillTag.ToString());
		}
	}
}
