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
};
