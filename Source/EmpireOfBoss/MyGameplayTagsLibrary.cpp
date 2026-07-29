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
}
