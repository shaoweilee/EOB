#include "EOB_SkillTreeComponent.h"

#include "AbilitySystemComponent.h"
#include "GameplayAbilitySpec.h"
#include "EmpireOfBossCharacter.h"
#include "EOB_LevelComponent.h"
#include "EmpireOfBossPlayerController.h"
#include "EOB_HUDWidget.h"

UEOB_SkillTreeComponent::UEOB_SkillTreeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UEOB_SkillTreeComponent::HasSkill(FName SkillID) const
{
	return LearnedSkills.Contains(SkillID);
}

const FEOBSkillDefinition* UEOB_SkillTreeComponent::FindDefinition(FName SkillID) const
{
	return SkillDefinitions.FindByPredicate(
		[&](const FEOBSkillDefinition& Def) { return Def.SkillID == SkillID; });
}

bool UEOB_SkillTreeComponent::CanLearnSkill(FName SkillID) const
{
	const FEOBSkillDefinition* Def = FindDefinition(SkillID);
	if (!Def || !Def->AbilityClass) return false;
	if (HasSkill(SkillID)) return false;
	if (!Def->PrerequisiteSkillID.IsNone() && !HasSkill(Def->PrerequisiteSkillID)) return false;

	const AEmpireOfBossCharacter* Hero = Cast<AEmpireOfBossCharacter>(GetOwner());
	return Hero && Hero->LevelComponent && Hero->LevelComponent->SkillPoints > 0;
}

bool UEOB_SkillTreeComponent::LearnSkill(FName SkillID)
{
	if (!CanLearnSkill(SkillID)) return false;

	const FEOBSkillDefinition* Def = FindDefinition(SkillID);
	AEmpireOfBossCharacter* Hero = Cast<AEmpireOfBossCharacter>(GetOwner());
	if (!Def || !Hero || !Hero->AbilitySystemComponent) return false;

	Hero->LevelComponent->SkillPoints--;
	Hero->AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(Def->AbilityClass, 1));
	LearnedSkills.Add(SkillID);

	// 第一个学会的技能自动成为右键当前技能
	if (LearnedSkills.Num() == 1)
	{
		CurrentSkillSlot = 0;
	}

	UE_LOG(LogTemp, Warning, TEXT("📖 [技能树] 学会技能【%s】！剩余技能点 %d"),
	       *Def->DisplayName.ToString(), Hero->LevelComponent->SkillPoints);

	NotifyHUD();
	return true;
}

void UEOB_SkillTreeComponent::CycleCurrentSkill()
{
	if (LearnedSkills.Num() <= 0) return;

	CurrentSkillSlot = (CurrentSkillSlot + 1) % LearnedSkills.Num();

	if (const FEOBSkillDefinition* Def = FindDefinition(LearnedSkills[CurrentSkillSlot]))
	{
		UE_LOG(LogTemp, Log, TEXT("[技能] 右键当前技能切换为：%s"), *Def->DisplayName.ToString());
	}
}

TSubclassOf<UGameplayAbility> UEOB_SkillTreeComponent::GetLearnedAbilityAt(int32 Index) const
{
	if (!LearnedSkills.IsValidIndex(Index)) return nullptr;
	if (const FEOBSkillDefinition* Def = FindDefinition(LearnedSkills[Index]))
	{
		return Def->AbilityClass;
	}
	return nullptr;
}

void UEOB_SkillTreeComponent::NotifyHUD()
{
	if (UWorld* World = GetWorld())
	{
		if (AEmpireOfBossPlayerController* PC = Cast<AEmpireOfBossPlayerController>(
			World->GetFirstPlayerController()))
		{
			if (PC->EOBHUDWidget)
			{
				PC->EOBHUDWidget->RefreshSkillTreePanel();
				PC->EOBHUDWidget->RefreshCharacterPanel();
			}
		}
	}
}
