#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "MyGameplayTagsLibrary.generated.h"

USTRUCT(BlueprintType)
struct FMyGameplayTags
{
	GENERATED_BODY()

public:
	// 初始化函数保持静态
	static void InitializeNativeTags();

	// =====================================================================
	// 将所有 Tag 变量改为静态 (static)
	// =====================================================================
	static FGameplayTag State_Rooted;
	static FGameplayTag State_Restricted_KnockedBack;
	static FGameplayTag Ability_Type_CCBreak;
	static FGameplayTag Data_Damage;
	// M1 新增：死亡状态标签（攻击逻辑/AI/UI 统一查询尸体用）
	static FGameplayTag State_Dead;
};

UCLASS()
class EMPIREOFBOSS_API UMyGameplayTagsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 暴露给蓝图的函数，内部也要同步修改去掉 .Get()
	UFUNCTION(BlueprintPure, Category = "GameplayTags")
	static FGameplayTag GetStateRooted() { return FMyGameplayTags::State_Rooted; }
};
