// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EOPBaseCharacter.h"
#include "EmpireOfBossCharacter.generated.h"

class UEOB_InventoryComponent;
class UEOB_LevelComponent;
class UEOB_SkillTreeComponent;
class UGameplayEffect;

/**
 *  A controllable top-down perspective character
 */
UCLASS(abstract)
class AEmpireOfBossCharacter : public AEOPBaseCharacter
{
	GENERATED_BODY()

private:
	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;


	/** 防卡肉定时器 */
	FTimerHandle GhostWalkSafeTimerHandle;
	/** 检查是否可以安全地恢复实体碰撞 */
	void TryRestoreSolidCollision();

public:
	/** Constructor */
	AEmpireOfBossCharacter();

	/** Initialization */
	virtual void BeginPlay() override;

	/** Update */
	virtual void Tick(float DeltaSeconds) override;

	/** Returns the camera component **/
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }

	/** Returns the Camera Boom component **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	UPROPERTY(VisibleAnywhere)
	AActor* CurrentTarget;

	UPROPERTY(VisibleAnywhere)
	bool bIsTryingToAttack = false;
	// 攻击检测逻辑
	void CheckAttackRangeAndExecute();

	void PerformMeleeAttack();

	// 暴露给蓝图，让动画蓝图能够呼叫它
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ApplyFanDamage();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UAnimMontage* AttackMontage; // 在蓝图中配置你的普攻动画蒙太奇

	// 🌟 在蓝图中把你刚才建好的 GE_Damage 拖到这个槽位里！
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TSubclassOf<class UGameplayEffect> DamageEffectClass;

	/** M2 新增：背包与装备组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EOB|Inventory")
	TObjectPtr<UEOB_InventoryComponent> InventoryComponent;

	/** M3a 新增：经验/升级/属性点组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EOB|Level")
	TObjectPtr<UEOB_LevelComponent> LevelComponent;

	/** M3b 新增：技能树组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EOB|Skill")
	TObjectPtr<UEOB_SkillTreeComponent> SkillTreeComponent;

	/** M3b 新增：出生即常驻的被动 GE（如回蓝），英雄蓝图默认值里配置 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|GAS")
	TArray<TSubclassOf<UGameplayEffect>> PassiveEffects;

	/** 开启/关闭穿怪状态 */
	UFUNCTION(BlueprintCallable, Category = "EOB|Combat")
	void SetGhostWalkEnabled(bool bEnabled);
};
