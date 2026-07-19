// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EOPBaseCharacter.h"
#include "CPP_Enemy_Base.generated.h"

UCLASS()
class EMPIREOFBOSS_API ACPP_Enemy_Base : public AEOPBaseCharacter
{
	GENERATED_BODY()

public:
	ACPP_Enemy_Base();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
};
