#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "EOB_ItemTypes.h"
#include "EOB_ItemDefinition.generated.h"

class UTexture2D;
class UStaticMesh;

/**
 * 物品静态定义（在编辑器：右键 → 杂项 → 数据资产 → 选 EOB_ItemDefinition 创建实例）
 * 装备原型：铁剑、皮盔……；背包原型：旅行背包（Kind 选"背包"，品质在掉落时掷，决定容量）
 */
UCLASS(BlueprintType)
class EMPIREOFBOSS_API UEOB_ItemDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Item")
	FText ItemName;

	/** 物品种类：装备（穿装备面板）/ 背包（装备到背包标签栏位） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Item")
	EEOBItemKind Kind = EEOBItemKind::Equipment;

	/** 背包/装备面板里的图标 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Item")
	TObjectPtr<UTexture2D> Icon;

	/** 掉在地上时的外观网格（装备拾取物生成时自动套用；留空则用拾取物蓝图自己的网格） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Item")
	TObjectPtr<UStaticMesh> WorldMesh;

	/** 掉落外观的相对旋转（让装备"躺下"等；仅装备有意义，留 0 则不转） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Item")
	FRotator WorldMeshRotation = FRotator::ZeroRotator;

	/** 掉落外观的缩放（项链比盔甲小这类调节；仅装备有意义，1 = 原始大小） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Item")
	FVector WorldMeshScale = FVector(1.f, 1.f, 1.f);

	/** 装备槽位（戒指类选 Ring，穿戴时自动分配左右手；背包忽略此项） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Item")
	EEOBEquipSlot EquipSlot = EEOBEquipSlot::Weapon;

	/** 固定词缀：武器攻击力、防具护甲值等，100% 生效（背包忽略此项） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Item")
	TArray<FEOBAffixValue> BaseAffixes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Item")
	int32 SellPrice = 0;
};