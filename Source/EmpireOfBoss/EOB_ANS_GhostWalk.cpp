#include "EOB_ANS_GhostWalk.h"
#include "Components/SkeletalMeshComponent.h"
#include "EmpireOfBossCharacter.h"

void UEOB_ANS_GhostWalk::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                     float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (AEmpireOfBossCharacter* Hero = Cast<AEmpireOfBossCharacter>(MeshComp->GetOwner()))
		{
			Hero->SetGhostWalkEnabled(true); // 进度条开始，进入无视怪堆的丝滑走位
		}
	}
}

void UEOB_ANS_GhostWalk::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                   const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (MeshComp && MeshComp->GetOwner())
	{
		if (AEmpireOfBossCharacter* Hero = Cast<AEmpireOfBossCharacter>(MeshComp->GetOwner()))
		{
			Hero->SetGhostWalkEnabled(false); // 进度条结束，进入安全落脚点判定
		}
	}
}
