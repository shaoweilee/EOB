#include "EOB_Widget_SkillTree.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "EmpireOfBossCharacter.h"
#include "EOB_SkillTreeComponent.h"
#include "EOB_LevelComponent.h"
#include "EOB_GameplayAbility.h"

void UEOB_Widget_SkillTree::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_Skill0) Btn_Skill0->OnClicked.AddDynamic(this, &UEOB_Widget_SkillTree::OnSkill0Clicked);
	if (Btn_Skill1) Btn_Skill1->OnClicked.AddDynamic(this, &UEOB_Widget_SkillTree::OnSkill1Clicked);
	if (Btn_Skill2) Btn_Skill2->OnClicked.AddDynamic(this, &UEOB_Widget_SkillTree::OnSkill2Clicked);
	if (Btn_Skill3) Btn_Skill3->OnClicked.AddDynamic(this, &UEOB_Widget_SkillTree::OnSkill3Clicked);
	if (Btn_Skill4) Btn_Skill4->OnClicked.AddDynamic(this, &UEOB_Widget_SkillTree::OnSkill4Clicked);
	if (Btn_Skill5) Btn_Skill5->OnClicked.AddDynamic(this, &UEOB_Widget_SkillTree::OnSkill5Clicked);
	if (Btn_Skill6) Btn_Skill6->OnClicked.AddDynamic(this, &UEOB_Widget_SkillTree::OnSkill6Clicked);
	if (Btn_Skill7) Btn_Skill7->OnClicked.AddDynamic(this, &UEOB_Widget_SkillTree::OnSkill7Clicked);
}

void UEOB_Widget_SkillTree::RefreshTree()
{
	AEmpireOfBossCharacter* Hero = Cast<AEmpireOfBossCharacter>(GetOwningPlayerPawn());
	if (!Hero || !Hero->SkillTreeComponent) return;

	UEOB_SkillTreeComponent* Tree = Hero->SkillTreeComponent;

	UTextBlock* Texts[8] = {
		Text_Skill0, Text_Skill1, Text_Skill2, Text_Skill3,
		Text_Skill4, Text_Skill5, Text_Skill6, Text_Skill7
	};
	UButton* Btns[8] = {
		Btn_Skill0, Btn_Skill1, Btn_Skill2, Btn_Skill3,
		Btn_Skill4, Btn_Skill5, Btn_Skill6, Btn_Skill7
	};

	if (Text_SkillPoints && Hero->LevelComponent)
	{
		Text_SkillPoints->SetText(FText::FromString(
			FString::Printf(TEXT("剩余技能点：%d"), Hero->LevelComponent->SkillPoints)));
	}

	for (int32 i = 0; i < 8; ++i)
	{
		if (!Tree->SkillDefinitions.IsValidIndex(i)) break;
		const FEOBSkillDefinition& Def = Tree->SkillDefinitions[i];

		FString StateText;
		bool bClickable = false;

		if (!Def.AbilityClass)
		{
			StateText = FString::Printf(TEXT("🚧 %s（开发中）"), *Def.DisplayName.ToString());
		}
		else if (Tree->HasSkill(Def.SkillID))
		{
			StateText = FString::Printf(TEXT("✅ %s（已学会）"), *Def.DisplayName.ToString());
		}
		else if (Tree->CanLearnSkill(Def.SkillID))
		{
			StateText = FString::Printf(TEXT("▶ %s（点击学习，1 技能点）\n  %s"),
			                            *Def.DisplayName.ToString(), *Def.Description.ToString());
			bClickable = true;
		}
		else
		{
			FString Reason;
			if (!Def.PrerequisiteSkillID.IsNone() && !Tree->HasSkill(Def.PrerequisiteSkillID))
			{
				const FEOBSkillDefinition* Pre = Tree->FindDefinition(Def.PrerequisiteSkillID);
				Reason = FString::Printf(TEXT("需先学会：%s"),
				                         Pre ? *Pre->DisplayName.ToString() : TEXT("前置技能"));
			}
			else
			{
				Reason = TEXT("技能点不足");
			}
			StateText = FString::Printf(TEXT("🔒 %s（%s）"), *Def.DisplayName.ToString(), *Reason);
		}

		if (Texts[i]) Texts[i]->SetText(FText::FromString(StateText));
		if (Btns[i]) Btns[i]->SetIsEnabled(bClickable);
	}
}

void UEOB_Widget_SkillTree::OnSkillNodeClicked(int32 Index)
{
	if (AEmpireOfBossCharacter* Hero = Cast<AEmpireOfBossCharacter>(GetOwningPlayerPawn()))
	{
		UEOB_SkillTreeComponent* Tree = Hero->SkillTreeComponent;
		if (Tree && Tree->SkillDefinitions.IsValidIndex(Index))
		{
			Tree->LearnSkill(Tree->SkillDefinitions[Index].SkillID);
			RefreshTree();
		}
	}
}

void UEOB_Widget_SkillTree::OnSkill0Clicked() { OnSkillNodeClicked(0); }
void UEOB_Widget_SkillTree::OnSkill1Clicked() { OnSkillNodeClicked(1); }
void UEOB_Widget_SkillTree::OnSkill2Clicked() { OnSkillNodeClicked(2); }
void UEOB_Widget_SkillTree::OnSkill3Clicked() { OnSkillNodeClicked(3); }
void UEOB_Widget_SkillTree::OnSkill4Clicked() { OnSkillNodeClicked(4); }
void UEOB_Widget_SkillTree::OnSkill5Clicked() { OnSkillNodeClicked(5); }
void UEOB_Widget_SkillTree::OnSkill6Clicked() { OnSkillNodeClicked(6); }
void UEOB_Widget_SkillTree::OnSkill7Clicked() { OnSkillNodeClicked(7); }
