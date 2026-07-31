#include "EOB_LevelComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "EOB_AttributeSet.h"
#include "EOPBaseCharacter.h"
#include "EmpireOfBossPlayerController.h"
#include "EOB_HUDWidget.h"

UEOB_LevelComponent::UEOB_LevelComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

float UEOB_LevelComponent::GetXPToNextLevel() const
{
	return FMath::RoundToFloat(BaseXPRequirement * FMath::Pow(static_cast<float>(Level), XPGrowthExponent));
}

void UEOB_LevelComponent::AddExperience(float Amount)
{
	if (Amount <= 0.f) return;

	CurrentXP += Amount;
	UE_LOG(LogTemp, Log, TEXT("[经验] +%.0f，当前 %.0f / %.0f（Lv.%d）"),
	       Amount, CurrentXP, GetXPToNextLevel(), Level);

	HandleLevelUps();
	NotifyHUD();
}

void UEOB_LevelComponent::HandleLevelUps()
{
	while (CurrentXP >= GetXPToNextLevel())
	{
		CurrentXP -= GetXPToNextLevel();
		Level++;
		StatPoints += 5; // TL2：每级 5 属性点
		SkillPoints += 1; // TL2：每级 1 技能点

		// 升级成长：血蓝上限提升（GE 配置），并瞬间回满
		ApplyGEToOwner(LevelUpGrowthGE);

		if (AEOPBaseCharacter* OwnerChar = Cast<AEOPBaseCharacter>(GetOwner()))
		{
			if (OwnerChar->AbilitySystemComponent && OwnerChar->AttributeSet)
			{
				OwnerChar->AbilitySystemComponent->ApplyModToAttribute(
					UEOB_AttributeSet::GetHealthAttribute(), EGameplayModOp::Override,
					OwnerChar->AttributeSet->GetMaxHealth());
				OwnerChar->AbilitySystemComponent->ApplyModToAttribute(
					UEOB_AttributeSet::GetManaAttribute(), EGameplayModOp::Override,
					OwnerChar->AttributeSet->GetMaxMana());
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("🎉 [升级] 升到 Lv.%d！获得 5 属性点（剩 %d）+ 1 技能点（共 %d），血蓝已回满"),
		       Level, StatPoints, SkillPoints);
	}
}

bool UEOB_LevelComponent::SpendStatPoint(EEOBStatType Stat)
{
	if (StatPoints <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[加点] 属性点不足，无法加点！"));
		return false;
	}

	TSubclassOf<UGameplayEffect> GEToApply = nullptr;
	switch (Stat)
	{
	case EEOBStatType::Strength: GEToApply = StatGE_Strength;
		break;
	case EEOBStatType::Dexterity: GEToApply = StatGE_Dexterity;
		break;
	case EEOBStatType::Focus: GEToApply = StatGE_Focus;
		break;
	case EEOBStatType::Vitality: GEToApply = StatGE_Vitality;
		break;
	}

	if (!GEToApply)
	{
		UE_LOG(LogTemp, Error, TEXT("[加点] 对应属性的加点 GE 未配置！请在英雄蓝图的 LevelComponent 默认值里填"));
		return false;
	}

	ApplyGEToOwner(GEToApply);
	StatPoints--;
	NotifyHUD();
	return true;
}

void UEOB_LevelComponent::ApplyGEToOwner(TSubclassOf<UGameplayEffect> GEClass)
{
	if (!GEClass) return;

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddInstigator(GetOwner(), GetOwner());
		ASC->ApplyGameplayEffectToSelf(GEClass.GetDefaultObject(), 1.f, Context);
	}
}

void UEOB_LevelComponent::NotifyHUD()
{
	if (UWorld* World = GetWorld())
	{
		if (AEmpireOfBossPlayerController* PC = Cast<AEmpireOfBossPlayerController>(World->GetFirstPlayerController()))
		{
			if (PC->EOBHUDWidget)
			{
				PC->EOBHUDWidget->RefreshXPDisplay();
				PC->EOBHUDWidget->RefreshCharacterPanel();
			}
		}
	}
}
