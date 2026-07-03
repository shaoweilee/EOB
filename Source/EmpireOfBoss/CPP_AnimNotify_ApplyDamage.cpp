// Copyright Epic Games, Inc. All Rights Reserved.

#include "CPP_AnimNotify_ApplyDamage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "EmpireOfBossCharacter.h" // 引入你的主角类

UCPP_AnimNotify_ApplyDamage::UCPP_AnimNotify_ApplyDamage()
{
	// 可以在这里做一些编辑器下的默认命名等配置
}

void UCPP_AnimNotify_ApplyDamage::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                         const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp) return;

	// 1. 骨骼组件依附的宿主
	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor) return;

	// 2. 强转为你的暗黑流主角
	AEmpireOfBossCharacter* MyHero = Cast<AEmpireOfBossCharacter>(OwnerActor);
	if (MyHero)
	{
		// 3. 隔空呼叫你的纯 C++ 2米扇形 AOE 伤害算法！
		MyHero->ApplyFanDamage();
	}
}
