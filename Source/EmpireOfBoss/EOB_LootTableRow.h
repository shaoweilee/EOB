#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EOB_LootTableRow.generated.h"

class AEOB_PickupBase;

/**
 * 掉落表行结构
 * 使用方式：创建一个 DataTable（DT_Loot_xxx），行结构选择本结构。
 * 每行 = 一种可能掉落的拾取物，怪物死亡时逐行独立判定概率。
 */
USTRUCT(BlueprintType)
struct EMPIREOFBOSS_API FEOBLootTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 掉落的拾取物类（BP_Pickup_Gold / BP_Pickup_Potion 等） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Loot")
	TSubclassOf<AEOB_PickupBase> PickupClass;

	/** 掉落概率 0~1（1 = 必掉） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Loot", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DropChance = 0.5f;

	/** 掉落数量范围（金币堆一次掉好几枚时用） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Loot", meta = (ClampMin = "1"))
	int32 MinCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Loot", meta = (ClampMin = "1"))
	int32 MaxCount = 1;
};
