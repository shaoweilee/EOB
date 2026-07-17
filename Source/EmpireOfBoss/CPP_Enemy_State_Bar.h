// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPP_Enemy_State_Bar.generated.h"

/**
 * 
 */
UCLASS()
class EMPIREOFBOSS_API UCPP_Enemy_State_Bar : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	void ShowStateBar(ESlateVisibility IsShow);
};
