#include "EOB_GameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/PlayerController.h"
#include "Engine/OverlapResult.h"
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

	FHitResult GroundHit;
	if (PC->GetHitResultUnderCursor(ECC_GameTraceChannel2, false, GroundHit))
	{
		OutPoint = GroundHit.ImpactPoint;
		return true;
	}
	return false;
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

	// 扇形 = 以自己为圆心、按朝向过滤角度的落点伤害
	// 直接在下方核心逻辑里处理角度过滤
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
