#include "MyGameplayTagsLibrary.h"
#include "GameplayTagsManager.h"

// =====================================================================
// 必须在 cpp 中为这三个静态变量分配内存（定义）
// =====================================================================
FGameplayTag FMyGameplayTags::State_Rooted;
FGameplayTag FMyGameplayTags::State_Restricted_KnockedBack;
FGameplayTag FMyGameplayTags::Ability_Type_CCBreak;
FGameplayTag FMyGameplayTags::Data_Damage;
FGameplayTag FMyGameplayTags::State_Dead;
FGameplayTag FMyGameplayTags::Data_AffixMagnitude;
// M3a 新增
FGameplayTag FMyGameplayTags::Data_IsCrit;
FGameplayTag FMyGameplayTags::Cooldown_Skill_MeleeSmash;
FGameplayTag FMyGameplayTags::Cooldown_Skill_Whirlwind;
FGameplayTag FMyGameplayTags::Cooldown_Skill_Fireball;
FGameplayTag FMyGameplayTags::Cooldown_Skill_FlameStorm;
FGameplayTag FMyGameplayTags::Cooldown_Skill_Meteor;
FGameplayTag FMyGameplayTags::Cooldown_Skill_WarCry;
FGameplayTag FMyGameplayTags::Cooldown_Skill_Charge;
FGameplayTag FMyGameplayTags::Cooldown_Skill_Earthquake;
FGameplayTag FMyGameplayTags::Buff_WarCry;

void FMyGameplayTags::InitializeNativeTags()
{
	// 直接为静态变量赋值，去掉了之前的 Tags. 前缀
	State_Rooted = FGameplayTag::RequestGameplayTag(FName("State.Rooted"));
	State_Restricted_KnockedBack = FGameplayTag::RequestGameplayTag(FName("State.Restricted.KnockedBack"));
	Ability_Type_CCBreak = FGameplayTag::RequestGameplayTag(FName("Ability.Type.CCBreak"));

	Data_Damage = FGameplayTag::RequestGameplayTag(FName("Data.Damage"));
	// M1 新增
	State_Dead = FGameplayTag::RequestGameplayTag(FName("State.Dead"));
	// M2 新增
	Data_AffixMagnitude = FGameplayTag::RequestGameplayTag(FName("Data.AffixMagnitude"));
	// M3a 新增
	Data_IsCrit = FGameplayTag::RequestGameplayTag(FName("Data.IsCrit"));
	// M3b 新增
	Cooldown_Skill_MeleeSmash = FGameplayTag::RequestGameplayTag(FName("Cooldown.Skill.MeleeSmash"));
	Cooldown_Skill_Whirlwind = FGameplayTag::RequestGameplayTag(FName("Cooldown.Skill.Whirlwind"));
	Cooldown_Skill_Fireball = FGameplayTag::RequestGameplayTag(FName("Cooldown.Skill.Fireball"));
	Cooldown_Skill_FlameStorm = FGameplayTag::RequestGameplayTag(FName("Cooldown.Skill.FlameStorm"));
	Cooldown_Skill_Meteor = FGameplayTag::RequestGameplayTag(FName("Cooldown.Skill.Meteor"));
	Cooldown_Skill_WarCry = FGameplayTag::RequestGameplayTag(FName("Cooldown.Skill.WarCry"));
	Cooldown_Skill_Charge = FGameplayTag::RequestGameplayTag(FName("Cooldown.Skill.Charge"));
	Cooldown_Skill_Earthquake = FGameplayTag::RequestGameplayTag(FName("Cooldown.Skill.Earthquake"));
	Buff_WarCry = FGameplayTag::RequestGameplayTag(FName("Buff.WarCry"));
}
