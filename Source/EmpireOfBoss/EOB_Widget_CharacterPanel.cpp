#include "EOB_Widget_CharacterPanel.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "EmpireOfBossCharacter.h"
#include "EOB_AttributeSet.h"

void UEOB_Widget_CharacterPanel::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_AddSTR) Btn_AddSTR->OnClicked.AddDynamic(this, &UEOB_Widget_CharacterPanel::OnAddSTRClicked);
	if (Btn_AddDEX) Btn_AddDEX->OnClicked.AddDynamic(this, &UEOB_Widget_CharacterPanel::OnAddDEXClicked);
	if (Btn_AddFOC) Btn_AddFOC->OnClicked.AddDynamic(this, &UEOB_Widget_CharacterPanel::OnAddFOCClicked);
	if (Btn_AddVIT) Btn_AddVIT->OnClicked.AddDynamic(this, &UEOB_Widget_CharacterPanel::OnAddVITClicked);
}

void UEOB_Widget_CharacterPanel::RefreshFromHero()
{
	AEmpireOfBossCharacter* Hero = Cast<AEmpireOfBossCharacter>(GetOwningPlayerPawn());
	if (!Hero || !Hero->AttributeSet) return;

	const UEOB_AttributeSet* AS = Hero->AttributeSet;
	UEOB_LevelComponent* LC = Hero->LevelComponent;

	// 小工具：控件存在才写，避免空指针
	auto SetTxt = [](UTextBlock* T, const FString& S)
	{
		if (T) T->SetText(FText::FromString(S));
	};

	if (LC)
	{
		SetTxt(Text_Level, FString::Printf(TEXT("等级：%d"), LC->Level));
		SetTxt(Text_XP, FString::Printf(TEXT("经验：%.0f / %.0f"), LC->CurrentXP, LC->GetXPToNextLevel()));
		SetTxt(Text_StatPoints, FString::Printf(TEXT("可用属性点：%d"), LC->StatPoints));
		SetTxt(Text_SkillPoints, FString::Printf(TEXT("技能点：%d"), LC->SkillPoints));
	}

	SetTxt(Text_STR, FString::Printf(TEXT("力量：%.0f"), AS->GetStrength()));
	SetTxt(Text_DEX, FString::Printf(TEXT("敏捷：%.0f"), AS->GetDexterity()));
	SetTxt(Text_FOC, FString::Printf(TEXT("专注：%.0f"), AS->GetFocus()));
	SetTxt(Text_VIT, FString::Printf(TEXT("体力：%.0f"), AS->GetVitality()));

	SetTxt(Text_Attack, FString::Printf(TEXT("攻击力：%.0f"), AS->GetAttackPower()));
	SetTxt(Text_Armor, FString::Printf(TEXT("护甲：%.1f"), AS->GetArmor()));
	SetTxt(Text_CritChance, FString::Printf(TEXT("暴击率：%.1f%%"), AS->GetCritChance()));
	SetTxt(Text_CritDamage, FString::Printf(TEXT("暴击伤害：%.0f%%"), AS->GetCritDamage()));
	SetTxt(Text_Dodge, FString::Printf(TEXT("闪避率：%.1f%%"), AS->GetDodgeChance()));
	SetTxt(Text_SkillDmg, FString::Printf(TEXT("技能伤害：+%.0f%%"), AS->GetSkillDamageBonus()));
	SetTxt(Text_HP, FString::Printf(TEXT("生命：%.0f / %.0f"), AS->GetHealth(), AS->GetMaxHealth()));
	SetTxt(Text_MP, FString::Printf(TEXT("法力：%.0f / %.0f"), AS->GetMana(), AS->GetMaxMana()));
}

void UEOB_Widget_CharacterPanel::SpendPoint(EEOBStatType Stat)
{
	if (AEmpireOfBossCharacter* Hero = Cast<AEmpireOfBossCharacter>(GetOwningPlayerPawn()))
	{
		if (Hero->LevelComponent && Hero->LevelComponent->SpendStatPoint(Stat))
		{
			RefreshFromHero();
		}
	}
}

void UEOB_Widget_CharacterPanel::OnAddSTRClicked() { SpendPoint(EEOBStatType::Strength); }
void UEOB_Widget_CharacterPanel::OnAddDEXClicked() { SpendPoint(EEOBStatType::Dexterity); }
void UEOB_Widget_CharacterPanel::OnAddFOCClicked() { SpendPoint(EEOBStatType::Focus); }
void UEOB_Widget_CharacterPanel::OnAddVITClicked() { SpendPoint(EEOBStatType::Vitality); }
