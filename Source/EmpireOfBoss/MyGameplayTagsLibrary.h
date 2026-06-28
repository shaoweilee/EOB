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