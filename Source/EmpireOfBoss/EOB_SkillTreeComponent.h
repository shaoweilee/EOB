#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EOB_SkillTreeComponent.generated.h"

class UGameplayAbility;

/** 技能树节点定义（组件默认值里逐行配置，树形靠前置 ID 串） */
USTRUCT(BlueprintType)
struct FEOBSkillDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Skill")
	FName SkillID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Skill")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Skill")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Skill")
	TSubclassOf<UGameplayAbility> AbilityClass;

	/** 前置技能 ID（None = 无前置） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Skill")
	FName PrerequisiteSkillID;
};

/**
 * M3b：技能树组件。记录已学技能、校验前置、花技能点学习、授予 Ability、右键当前技能管理
 */
UCLASS(ClassGroup=(EOB), meta=(BlueprintSpawnableComponent))
class EMPIREOFBOSS_API UEOB_SkillTreeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UEOB_SkillTreeComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Skill")
	TArray<FEOBSkillDefinition> SkillDefinitions;

	/** 已学会的技能 ID（按学习顺序，快捷栏 1~4 也按此顺序） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EOB|Skill")
	TArray<FName> LearnedSkills;

	/** 右键当前技能槽位（已学列表下标；数字键直放时同步） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EOB|Skill")
	int32 CurrentSkillSlot = 0;

	UFUNCTION(BlueprintPure, Category = "EOB|Skill")
	bool HasSkill(FName SkillID) const;

	/** 未学 + 已实装 + 前置已学 + 技能点 ≥1 */
	UFUNCTION(BlueprintPure, Category = "EOB|Skill")
	bool CanLearnSkill(FName SkillID) const;

	/** 花 1 技能点学习，返回是否成功 */
	UFUNCTION(BlueprintCallable, Category = "EOB|Skill")
	bool LearnSkill(FName SkillID);

	/** Tab 循环切换右键当前技能 */
	UFUNCTION(BlueprintCallable, Category = "EOB|Skill")
	void CycleCurrentSkill();

	/** 已学列表第 Index 个技能的 AbilityClass */
	TSubclassOf<UGameplayAbility> GetLearnedAbilityAt(int32 Index) const;

	const FEOBSkillDefinition* FindDefinition(FName SkillID) const;

private:
	void NotifyHUD();
};
