#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EOB_ItemTypes.h"
#include "EOB_ItemDefinition.generated.h"

class UTexture2D;

/**
 * 物品静态定义（在编辑器：右键 → 杂项 → 数据资产 → 选 EOB_ItemDefinition 创建实例）
 * 每件装备原型一个：铁剑、皮盔……
 */
UCLASS(BlueprintType)
class EMPIREOFBOSS_API UEOB_ItemDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Item")
	FText ItemName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Item")
	TObjectPtr<UTexture2D> Icon;

	/** 装备槽位（戒指类选 Ring，穿戴时自动分配左右手） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Item")
	EEOBEquipSlot EquipSlot = EEOBEquipSlot::Weapon;

	/** 固定词缀：武器攻击力、防具护甲值等，100% 生效 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Item")
	TArray<FEOBAffixValue> BaseAffixes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Item")
	int32 SellPrice = 0;
};