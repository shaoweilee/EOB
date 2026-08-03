#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EOB_Projectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UGameplayEffect;
class UAbilitySystemComponent;

/**
 * M3b：火球投射物。直线飞行，碰到敌人爆炸成小 AoE。
 * 建一个继承本类的 BP_EOB_Projectile，在里面加球体网格/粒子做外观。
 */
UCLASS()
class EMPIREOFBOSS_API AEOB_Projectile : public AActor
{
	GENERATED_BODY()

public:
	AEOB_Projectile();

	/** 由技能在生成后立刻呼叫：灌入伤害数据与来源 */
	void Init(float InDamage, bool bInIsCrit, TSubclassOf<UGameplayEffect> InDamageGE,
	          UAbilitySystemComponent* InSourceASC, AActor* InInstigator);

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	                     UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
	                     const FHitResult& SweepResult);

	/** 命中后对爆炸半径内所有敌人灌伤害并自毁 */
	void Explode();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USphereComponent> Sphere;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> Movement;

	/** 爆炸 AoE 半径 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|Skill")
	float ExplodeRadius = 180.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|Skill")
	float LifeTime = 3.f;

private:
	float Damage = 0.f;
	bool bIsCrit = false;
	TSubclassOf<UGameplayEffect> DamageGE;
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
};
