#include "EOB_GameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/PlayerController.h"
#include "Engine/OverlapResult.h"
#include "NavigationSystem.h"
#include "EOPBaseCharacter.h"
#include "EOB_AttributeSet.h"
#include "MyGameplayTagsLibrary.h"

UEOB_GameplayAbility::UEOB_GameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UEOB_GameplayAbility::RotateAvatarToCursor() const
{
	FVector Point;
	if (GetCursorGroundPoint(Point))
	{
		if (AActor* Avatar = GetAvatarActorFromActorInfo())
		{
			FVector Dir = Point - Avatar->GetActorLocation();
			Dir.Z = 0.f;
			if (!Dir.IsNearlyZero())
			{
				Avatar->SetActorRotation(Dir.Rotation());
			}
		}
	}
}

bool UEOB_GameplayAbility::GetCursorGroundPoint(FVector& OutPoint) const
{
	if (!CurrentActorInfo) return false;
	APlayerController* PC = CurrentActorInfo->PlayerController.Get();
	if (!PC) return false;

	FHitResult CursorHit;
	if (!PC->GetHitResultUnderCursor(ECC_GameTraceChannel2, false, CursorHit))
	{
		return false;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	UWorld* World = Avatar ? Avatar->GetWorld() : nullptr;
	const FVector HeroLoc = Avatar ? Avatar->GetActorLocation() : CursorHit.ImpactPoint;

	// ① 限制瞄准距离：光标点中 Z=2400 的远景山体时，把落点拉回英雄身边 MaxAimDistance 以内
	FVector AimPoint = CursorHit.ImpactPoint;
	{
		FVector Flat = AimPoint - HeroLoc;
		Flat.Z = 0.f;
		const float FlatLen = Flat.Size();
		if (FlatLen > MaxAimDistance)
		{
			AimPoint = HeroLoc + Flat.GetSafeNormal() * MaxAimDistance; // Z 自动变成英雄高度
		}
	}

	// ② 优先投影到导航网格：怪物在哪条路上走，技能就落在哪条路上。
	//    这一步同时解决"点中树冠/屋顶/高台边沿导致 Z 悬空"——导航网格只存在于可行走地面上。
	if (World)
	{
		if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World))
		{
			FNavLocation NavLoc;
			if (NavSys->ProjectPointToNavigation(AimPoint, NavLoc, FVector(300.f, 300.f, 4000.f)))
			{
				OutPoint = NavLoc.Location;
				return true;
			}
		}
	}

	// ③ 瞄准点不在可行走区域（背景山体/深渊）：退而求其次，落在英雄自己脚下，
	//    至少砸在面前，而不是飞到 Z=2400 的半山腰上
	if (World)
	{
		if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World))
		{
			FNavLocation NavLoc;
			if (NavSys->ProjectPointToNavigation(HeroLoc, NavLoc, FVector(300.f, 300.f, 4000.f)))
			{
				OutPoint = NavLoc.Location;
				return true;
			}
		}
	}

	// ④ 连英雄脚下都没有导航网格（不该发生）：退回垂直下扫，取与英雄高度最接近的表面
	const float RefZ = HeroLoc.Z;
	TArray<FHitResult> Hits;
	const FVector SkyStart(AimPoint.X, AimPoint.Y, RefZ + 5000.f);
	const FVector SkyEnd(AimPoint.X, AimPoint.Y, RefZ - 5000.f);
	FCollisionQueryParams Params;
	if (Avatar)
	{
		Params.AddIgnoredActor(Avatar);
	}
	if (World && World->LineTraceMultiByChannel(Hits, SkyStart, SkyEnd, ECC_GameTraceChannel2, Params) && Hits.Num() >
		0)
	{
		const FHitResult* Best = &Hits[0];
		for (const FHitResult& H : Hits)
		{
			if (FMath::Abs(H.ImpactPoint.Z - RefZ) < FMath::Abs(Best->ImpactPoint.Z - RefZ))
			{
				Best = &H;
			}
		}
		OutPoint = Best->ImpactPoint;
		return true;
	}

	// ⑤ 什么都探不到：用限距后的 XY，高度取英雄脚下
	OutPoint = FVector(AimPoint.X, AimPoint.Y, HeroLoc.Z);
	return true;
}

float UEOB_GameplayAbility::ComputeSkillDamage(float DamageMultiplier, bool bCanCrit, bool& bOutIsCrit) const
{
	bOutIsCrit = false;

	const AEOPBaseCharacter* OwnerChar = Cast<AEOPBaseCharacter>(GetAvatarActorFromActorInfo());
	if (!OwnerChar || !OwnerChar->AttributeSet) return 1.f;

	const UEOB_AttributeSet* AS = OwnerChar->AttributeSet;

	float Damage = (AS->GetAttackPower() + FMath::RandRange(2.f, 3.f)) * DamageMultiplier;
	Damage *= (1.f + AS->GetSkillDamageBonus() / 100.f);

	if (bCanCrit && AS->GetCritChance() > 0.f && FMath::FRand() * 100.f < AS->GetCritChance())
	{
		Damage *= AS->GetCritDamage() / 100.f;
		bOutIsCrit = true;
	}
	return Damage;
}

TArray<AActor*> UEOB_GameplayAbility::ApplyDamageFan(float Radius, float HalfAngle, float Damage, bool bIsCrit) const
{
	TArray<AActor*> HitActors;
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar) return HitActors;

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC || !DamageEffectClass) return HitActors;

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
	Avatar->GetWorld()->OverlapMultiByChannel(Overlaps, Avatar->GetActorLocation(), FQuat::Identity,
	                                          ECC_Pawn, Shape);

	const FVector Forward = Avatar->GetActorForwardVector();

	for (const FOverlapResult& Result : Overlaps)
	{
		AActor* HitActor = Result.GetActor();
		if (!HitActor || HitActor == Avatar) continue;

		if (HalfAngle < 180.f)
		{
			const FVector DirToTarget = (HitActor->GetActorLocation() - Avatar->GetActorLocation()).GetSafeNormal();
			const float Angle = FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Forward, DirToTarget)));
			if (Angle > HalfAngle) continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor);
		if (!TargetASC || TargetASC->HasMatchingGameplayTag(FMyGameplayTags::State_Dead)) continue;

		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddInstigator(Avatar, Avatar);
		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.f, Context);
		if (!SpecHandle.IsValid()) continue;

		SpecHandle.Data.Get()->SetSetByCallerMagnitude(FMyGameplayTags::Data_Damage, Damage);
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(FMyGameplayTags::Data_IsCrit, bIsCrit ? 1.f : 0.f);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

		HitActors.Add(HitActor);
	}
	return HitActors;
}

TArray<AActor*> UEOB_GameplayAbility::ApplyDamageAtLocation(FVector Center, float Radius, float Damage,
                                                            bool bIsCrit) const
{
	TArray<AActor*> HitActors;
	AActor* Avatar = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!Avatar || !SourceASC || !DamageEffectClass) return HitActors;

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
	Avatar->GetWorld()->OverlapMultiByChannel(Overlaps, Center, FQuat::Identity, ECC_Pawn, Shape);

	for (const FOverlapResult& Result : Overlaps)
	{
		AActor* HitActor = Result.GetActor();
		if (!HitActor || HitActor == Avatar) continue;

		UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor);
		if (!TargetASC || TargetASC->HasMatchingGameplayTag(FMyGameplayTags::State_Dead)) continue;

		FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
		Context.AddInstigator(Avatar, Avatar);
		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.f, Context);
		if (!SpecHandle.IsValid()) continue;

		SpecHandle.Data.Get()->SetSetByCallerMagnitude(FMyGameplayTags::Data_Damage, Damage);
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(FMyGameplayTags::Data_IsCrit, bIsCrit ? 1.f : 0.f);
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

		HitActors.Add(HitActor);
	}
	return HitActors;
}

void UEOB_GameplayAbility::ApplyBuffToSelf(TSubclassOf<UGameplayEffect> BuffGE) const
{
	if (!BuffGE) return;
	AActor* Avatar = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!Avatar || !SourceASC) return;

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddInstigator(Avatar, Avatar);
	SourceASC->ApplyGameplayEffectToSelf(BuffGE.GetDefaultObject(), 1.f, Context);
}
