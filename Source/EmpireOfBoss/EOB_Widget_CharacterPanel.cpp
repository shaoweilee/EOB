#include "EOB_Widget_CharacterPanel.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "EmpireOfBossCharacter.h"
#include "EOB_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"


namespace
{
	/** 面板上展示的所有属性：任何一个变了都触发面板自刷 */
	const TArray<FGameplayAttribute>& GetWatchedAttributes()
	{
		static const TArray<FGameplayAttribute> Watched = {
			UEOB_AttributeSet::GetStrengthAttribute(),
			UEOB_AttributeSet::GetDexterityAttribute(),
			UEOB_AttributeSet::GetFocusAttribute(),
			UEOB_AttributeSet::GetVitalityAttribute(),
			UEOB_AttributeSet::GetAttackPowerAttribute(),
			UEOB_AttributeSet::GetArmorAttribute(),
			UEOB_AttributeSet::GetCritChanceAttribute(),
			UEOB_AttributeSet::GetCritDamageAttribute(),
			UEOB_AttributeSet::GetDodgeChanceAttribute(),
			UEOB_AttributeSet::GetSkillDamageBonusAttribute(),
			UEOB_AttributeSet::GetHealthAttribute(),
			UEOB_AttributeSet::GetMaxHealthAttribute(),
			UEOB_AttributeSet::GetManaAttribute(),
			UEOB_AttributeSet::GetMaxManaAttribute(),
		};
		return Watched;
	}
}

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

	BindAttributeDelegates(Hero); // 🌟 只绑一次，之后装备/升级一改属性面板就自动刷新

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

void UEOB_Widget_CharacterPanel::NativeDestruct()
{
	UnbindAttributeDelegates();
	Super::NativeDestruct();
}

void UEOB_Widget_CharacterPanel::BindAttributeDelegates(AEmpireOfBossCharacter* Hero)
{
	if (bDelegatesBound || !Hero || !Hero->AbilitySystemComponent) return;

	UAbilitySystemComponent* ASC = Hero->AbilitySystemComponent;
	for (const FGameplayAttribute& Attr : GetWatchedAttributes())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(Attr).AddUObject(
			this, &UEOB_Widget_CharacterPanel::OnWatchedAttributeChanged);
	}

	BoundASC = ASC;
	bDelegatesBound = true;
}

void UEOB_Widget_CharacterPanel::UnbindAttributeDelegates()
{
	if (!bDelegatesBound) return;

	// ⚠️ 必须解绑：ASC 比 Widget 活得久，不解绑的话 Widget 销毁后回调打到野指针直接崩
	if (UAbilitySystemComponent* ASC = BoundASC.Get())
	{
		for (const FGameplayAttribute& Attr : GetWatchedAttributes())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(Attr).RemoveAll(this);
		}
	}
	BoundASC = nullptr;
	bDelegatesBound = false;
}

void UEOB_Widget_CharacterPanel::OnWatchedAttributeChanged(const FOnAttributeChangeData& Data)
{
	RefreshFromHero();
}
