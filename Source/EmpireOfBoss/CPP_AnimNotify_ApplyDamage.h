// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "CPP_AnimNotify_ApplyDamage.generated.h"

/**
 * 纯 C++ 暗黑流暗夜扇形伤害结算通知
 */
UCLASS()
class EMPIREOFBOSS_API UCPP_AnimNotify_ApplyDamage : public UAnimNotify
{
	GENERATED_BODY()

public:
	UCPP_AnimNotify_ApplyDamage();

	// 重写虚幻引擎通知触发的核心虚函数
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	                    const FAnimNotifyEventReference& EventReference) override;
};
