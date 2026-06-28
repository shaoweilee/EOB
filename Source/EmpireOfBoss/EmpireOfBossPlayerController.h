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


	/** True if the controlled character should navigate to the mouse cursor. */
	uint32 bMoveToMouseCursor : 1;

	/** Saved location of the character movement destination */
	UPROPERTY(VisibleAnywhere)
	FVector CachedDestination;

	/** Time that the click input has been pressed */
	float FollowTime = 0.0f;

public:
	/** Constructor */
	AEmpireOfBossPlayerController();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ✅ 全局唯一入口
	UPROPERTY(VisibleAnywhere)
	AEmpireOfBossCharacter* MyHero;
	UCharacterMovementComponent* MoveComp;
	UAbilitySystemComponent* UABC;

	UPROPERTY(BlueprintReadWrite, Category = "EOB|UI")
	class UEOB_HUDWidget* EOBHUDWidget;

	float OriginMaxWalkSpeed;

	// 1. 供定时器实时读取的动态目标朝向
	FRotator DynamicTargetRotation;
	// 2. 将定时器句柄作为类成员变量，彻底解决 Lambda 捕获的生命周期问题
	FTimerHandle SmoothRotateTimerHandle;

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
};
