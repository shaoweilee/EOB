// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "EOPGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class EMPIREOFBOSS_API UEOPGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// 重写虚函数 Init
	virtual void Init() override;
	// ===================== M1 新增：金币账户 =====================
	// 挂在 GameInstance 上：切地图、开关 HUD 都不会丢失

	/** 当前金币数量 */
	UPROPERTY(BlueprintReadOnly, Category = "EOB|Economy")
	int32 GoldAmount = 0;

	/** 增加金币（负数为扣钱，内部会兜底不为负） */
	UFUNCTION(BlueprintCallable, Category = "EOB|Economy")
	void AddGold(int32 Amount);

	/** 金币变化时通知 UI（蓝图重写来刷新 HUD 上的金币显示） */
	UFUNCTION(BlueprintImplementableEvent, Category = "EOB|Economy")
	void OnGoldChanged(int32 NewGoldAmount);
};
