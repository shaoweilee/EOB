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

	UFUNCTION() // 要加这个才能被用作委托处理函数
	void OnDamageTaken(AActor* DamagedActor, float Damage, const class UDamageType* DamageType,
	                   class AController* InstigatedBy, AActor* DamageCauser);
};
