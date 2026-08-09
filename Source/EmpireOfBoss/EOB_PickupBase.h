#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EOB_PickupBase.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UGameplayEffect;
class AEmpireOfBossCharacter;
class UEOB_ItemDefinition;

/**
 * 拾取物基类：金币、药水、装备掉落都继承它。
 * 主角走进 PickupRadius 范围自动拾取，无需点击。
 *
 * 三个开箱即用的子类配置：
 *  - BP_Pickup_Gold：设置 GoldValue > 0，拾取后金币进 GameInstance
 *  - BP_Pickup_Potion：设置 GrantedEffectClass 为一个 Instant 回血 GE
 *  - 装备拾取物：设置 DroppedItemDefinition，拾取时掷品质进背包
 */
UCLASS()
class EMPIREOFBOSS_API AEOB_PickupBase : public AActor
{
	GENERATED_BODY()

public:
	AEOB_PickupBase();

	/** 生成后覆盖装备定义（掉落表行指定装备时用，见 CPP_Enemy_Base::SpawnLoot），并同步刷新外观网格 */
	void SetDroppedItemDefinition(UEOB_ItemDefinition* NewDefinition);

	/** 按当前装备定义刷新掉落物外观（DA 里配了 WorldMesh 才覆盖拾取物蓝图自己的网格） */
	void ApplyDefinitionVisuals();

	/** 把网格按包围盒"落地"：让网格最低点贴到 Actor 原点（生成时原点已被射线贴到地面） */
	void SnapMeshToGround();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USphereComponent> PickupSphere;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	/** 自动拾取半径（cm） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Pickup")
	float PickupRadius = 80.f;

	/** 金币数量：>0 时拾取后加进 GameInstance 的金币账户 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Pickup")
	int32 GoldValue = 0;

	/** 拾取时对主角施加的 GE（药水回血等，Instant 类型） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Pickup")
	TSubclassOf<UGameplayEffect> GrantedEffectClass;

	// ===================== M2 新增：装备掉落 =====================

	/** 装备定义（设置后此拾取物变为装备：拾取时掷品质进背包） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Pickup")
	TObjectPtr<UEOB_ItemDefinition> DroppedItemDefinition;

	/** 品质权重：白/绿/蓝/金（按比例生效，不用凑满 100） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Pickup")
	float WhiteWeight = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Pickup")
	float GreenWeight = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Pickup")
	float BlueWeight = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Pickup")
	float GoldWeight = 2.f;

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	                     UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	                     bool bFromSweep, const FHitResult& SweepResult);

	/** 被拾取时的表现钩子：蓝图重写来播音效/特效（基类已处理数值逻辑，随后自动销毁） */
	UFUNCTION(BlueprintNativeEvent, Category = "EOB|Pickup")
	void OnCollected(AEmpireOfBossCharacter* Hero);
	void OnCollected_Implementation(AEmpireOfBossCharacter* Hero);
};
