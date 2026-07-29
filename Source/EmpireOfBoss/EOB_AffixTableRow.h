#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EOB_ItemTypes.h"
#include "EOB_AffixTableRow.generated.h"

/**
 * 词缀池行结构：建 DataTable（DT_Affixes）时行结构选 EOBAffixTableRow。
 * 每行 = 一条可能被随机抽中的词缀。
 */
USTRUCT(BlueprintType)
struct EMPIREOFBOSS_API FEOBAffixTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 词缀名（UI 悬浮提示显示，如"力量的"） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Affix")
	FText AffixName;

	/** 修改哪个属性 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Affix")
	FGameplayAttribute TargetAttribute;

	/** 用哪个 GE 蓝图施加（必须与属性对应） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Affix")
	TSubclassOf<UGameplayEffect> GEClass;

	/** 数值 roll 范围 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Affix")
	float MinValue = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Affix")
	float MaxValue = 5.f;

	/** 能出现在哪些槽位的装备上（武器词缀不会 roll 到鞋子上） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Affix")
	TArray<EEOBEquipSlot> ApplicableSlots;

	/** 抽取权重（越大越常见） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Affix")
	float Weight = 10.f;
};
