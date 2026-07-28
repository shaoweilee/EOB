#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EOB_PickupBase.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UGameplayEffect;
class AEmpireOfBossCharacter;

/**
 * 拾取物基类：金币、药水、（未来的）装备掉落都继承它。
 * 主角走进 PickupRadius 范围自动拾取，无需点击。
 *
 * 两个开箱即用的子类配置：
 *  - BP_Pickup_Gold：设置 GoldValue > 0，拾取后金币进 GameInstance
 *  - BP_Pickup_Potion：设置 GrantedEffectClass 为一个 Instant 回血 GE
 */
UCLASS()
class EMPIREOFBOSS_API AEOB_PickupBase : public AActor
{
	GENERATED_BODY()

public:
	AEOB_PickupBase();

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

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
						 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
						 bool bFromSweep, const FHitResult& SweepResult);

	/** 被拾取时的表现钩子：蓝图重写来播音效/特效（基类已处理数值逻辑，随后自动销毁） */
	UFUNCTION(BlueprintNativeEvent, Category = "EOB|Pickup")
	void OnCollected(AEmpireOfBossCharacter* Hero);
	void OnCollected_Implementation(AEmpireOfBossCharacter* Hero);
};