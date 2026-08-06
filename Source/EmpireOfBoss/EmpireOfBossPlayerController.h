// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "EmpireOfBossCharacter.h"
#include "GameFramework/PlayerController.h"
#include "EmpireOfBossPlayerController.generated.h"

class UNiagaraSystem;
class UInputMappingContext;
class UInputAction;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  Player controller for a top-down perspective game.
 *  Implements point and click based controls
 */
UCLASS(abstract)
class AEmpireOfBossPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	/** Time Threshold to know if it was a short press */
	UPROPERTY(EditAnywhere, Category="Input")
	float ShortPressThreshold = 0.5f;

	/** FX Class that we will spawn when clicking */
	UPROPERTY(EditAnywhere, Category="Input")
	UNiagaraSystem* FXCursor;

	/** MappingContext */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SetDestinationClickAction;
	/** Rooted Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* RootedAction;

	/** M2: 背包开合按键 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ToggleInventoryAction;

	/** M3a: 角色面板开合按键 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ToggleCharacterAction;

	/** M3b: 技能树开合按键（K） */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ToggleSkillTreeAction;

	/** M3b: 技能快捷键 1~4（直放已学列表第 N 个，并设为右键当前技能） */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* Skill1Action;
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* Skill2Action;
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* Skill3Action;
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* Skill4Action;

	/** M3b: 右键放当前技能 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* CastCurrentSkillAction;

	/** M3b: Tab 循环切换右键当前技能 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* CycleSkillAction;

	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;


	/** Time that the click input has been pressed */
	float FollowTime = 0.0f;

public:
	/** Constructor */
	AEmpireOfBossPlayerController();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void PlayerTick(float DeltaTime) override;

	// ✅ 全局唯一入口
	UPROPERTY(VisibleAnywhere)
	AEmpireOfBossCharacter* MyHero;
	UCharacterMovementComponent* MoveComp;

	/** Saved location of the character movement destination */
	UPROPERTY(VisibleAnywhere)
	FVector CachedDestination;

	UPROPERTY(BlueprintReadWrite, Category = "EOB|UI")
	class UEOB_HUDWidget* EOBHUDWidget;

	float OriginMaxWalkSpeed;

	// 1. 供定时器实时读取的动态目标朝向
	FRotator DynamicTargetRotation;
	// 2. 将定时器句柄作为类成员变量，彻底解决 Lambda 捕获的生命周期问题
	FTimerHandle SmoothRotateTimerHandle;

	// 🌟 核心性能优化：缓存上一次射线扫到的敌人指针，避免每帧高频重复调用 UI 逻辑
	UPROPERTY()
	TWeakObjectPtr<AActor> LastHoveredEnemy;

	/** 本次按下是否点在敌人身上（是则松开/长按时不发地面寻路，防止攻击完还往怪身上贴） */
	bool bPressedOnEnemy = false;

	// 🌟 辅助函数：负责每帧检测并切换血条显示状态
	void CheckEnemyHoverUnderCursor();

protected:
	/** Initialize input bindings */
	virtual void SetupInputComponent() override;
	/** Input handlers */
	void OnInputStarted();
	void OnSetDestinationTriggered();
	void OnSetDestinationReleased();

	void OnRootKeyStarted();
	void OnRootKeyCancelled();
	void OnRootKeyCompleted();

	bool IsBlockMove();
	void RotateCharacterToCursor();

	// 🌟 属性改变时的 C++ 回调函数
	void OnPlayerHealthChanged(const struct FOnAttributeChangeData& Data);
	// 🌟 蓝量改变时的 C++ 回调函数
	void OnPlayerManaChanged(const struct FOnAttributeChangeData& Data);

	AActor* GetTargetUnderCursor();
	void OnToggleInventory();
	void OnToggleCharacter();

	void OnToggleSkillTree();
	void OnSkill1();
	void OnSkill2();
	void OnSkill3();
	void OnSkill4();
	void OnCastCurrentSkill();
	void OnCycleSkill();
	/** 放第 SlotIndex 个已学技能（成功则同步为右键当前技能） */
	void CastSkillAtSlot(int32 SlotIndex);
};
