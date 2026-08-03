#include "GA_Skill_Meteor.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "MyGameplayTagsLibrary.h"

void UGA_Skill_Meteor::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo,
                                       const FGameplayAbilityActivationInfo ActivationInfo,
                                       const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UWorld* World = GetWorld();
	if (!Avatar || !SourceASC || !World)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	if (!GetCursorGroundPoint(PendingStrikePoint))
	{
		PendingStrikePoint = Avatar->GetActorLocation();
	}

	// 🌟 关键改动：不再把定时器绑在技能实例上（实例成员定时器在本工程实测不触发，
	//    导致落点日志和调试圈都没有、伤害也不结算）。
	//    改为世界定时器 + Lambda：伤害、落点、半径按值捕获，英雄/ASC 弱引用，
	//    即使技能实例被回收，这发陨石也必然砸下来。
	bool bIsCrit = false;
	const float Damage = ComputeSkillDamage(DamageMultiplier, false, bIsCrit);
	const FVector StrikePoint = PendingStrikePoint;
	const float StrikeRadius = Radius;
	const TSubclassOf<UGameplayEffect> DGE = DamageEffectClass;
	const TWeakObjectPtr<AActor> WeakAvatar = Avatar;
	const TWeakObjectPtr<UAbilitySystemComponent> WeakASC = SourceASC;

	FTimerDelegate StrikeDelegate;
	StrikeDelegate.BindLambda([WeakAvatar, WeakASC, DGE, StrikePoint, StrikeRadius, Damage]()
	{
		AActor* Av = WeakAvatar.Get();
		UAbilitySystemComponent* ASC = WeakASC.Get();
		if (!Av || !ASC || !DGE) return;

		int32 HitCount = 0;
		TArray<FOverlapResult> Overlaps;
		FCollisionShape Shape = FCollisionShape::MakeSphere(StrikeRadius);
		Av->GetWorld()->OverlapMultiByChannel(Overlaps, StrikePoint, FQuat::Identity, ECC_Pawn, Shape);

		for (const FOverlapResult& Result : Overlaps)
		{
			AActor* HitActor = Result.GetActor();
			if (!HitActor || HitActor == Av) continue;

			UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor);
			if (!TargetASC || TargetASC->HasMatchingGameplayTag(FMyGameplayTags::State_Dead)) continue;

			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			Context.AddInstigator(Av, Av);
			FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(DGE, 1.f, Context);
			if (!SpecHandle.IsValid()) continue;

			SpecHandle.Data.Get()->SetSetByCallerMagnitude(FMyGameplayTags::Data_Damage, Damage);
			SpecHandle.Data.Get()->SetSetByCallerMagnitude(FMyGameplayTags::Data_IsCrit, 0.f);
			ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			HitCount++;
		}

		// 橙色调试圈：落点 + 半径，存在 3 秒，验收完可删
		DrawDebugSphere(Av->GetWorld(), StrikePoint, StrikeRadius, 24, FColor::Orange, false, 3.f);

		UE_LOG(LogTemp, Warning, TEXT("[技能] 陨石落地！伤害 %.1f，命中 %d 个目标"), Damage, HitCount);
	});

	World->GetTimerManager().SetTimer(StrikeTimerHandle, StrikeDelegate, ImpactDelay, false);

	UE_LOG(LogTemp, Log, TEXT("[技能] 陨石召唤中……落点 %s"), *StrikePoint.ToString());
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
