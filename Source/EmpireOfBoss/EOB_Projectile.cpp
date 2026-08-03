#include "EOB_Projectile.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/OverlapResult.h"
#include "MyGameplayTagsLibrary.h"

AEOB_Projectile::AEOB_Projectile()
{
	PrimaryActorTick.bCanEverTick = false;

	Sphere = CreateDefaultSubobject<USphereComponent>(TEXT("Sphere"));
	RootComponent = Sphere;
	Sphere->InitSphereRadius(16.f);
	Sphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Sphere->SetGenerateOverlapEvents(true);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Sphere;
	Movement->InitialSpeed = 1200.f;
	Movement->MaxSpeed = 1200.f;
	Movement->ProjectileGravityScale = 0.f; // 直线飞行
}

void AEOB_Projectile::BeginPlay()
{
	Super::BeginPlay();

	// 🌟 兜底：蓝图上误把根球体改成 NoCollision 时，运行时强制改回。
	//    根球体没碰撞 = 重叠检测全废 = 火球永远打不到人（你这次就是踩的这个坑）。
	Sphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Sphere->SetGenerateOverlapEvents(true);

	Sphere->OnComponentBeginOverlap.AddDynamic(this, &AEOB_Projectile::OnSphereOverlap);
	SetLifeSpan(LifeTime);
}

void AEOB_Projectile::Init(float InDamage, bool bInIsCrit, TSubclassOf<UGameplayEffect> InDamageGE,
                           UAbilitySystemComponent* InSourceASC, AActor* InInstigator)
{
	Damage = InDamage;
	bIsCrit = bInIsCrit;
	DamageGE = InDamageGE;
	SourceASC = InSourceASC;
	SetInstigator(Cast<APawn>(InInstigator));
	SetOwner(InInstigator);
}

void AEOB_Projectile::OnSphereOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                      const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == GetInstigator() || OtherActor == GetOwner()) return;

	UE_LOG(LogTemp, Warning, TEXT("[火球] 碰到了：%s"), *OtherActor->GetName());

	// 碰到有 GAS 的敌人 → 爆炸；碰到其他东西（墙/箱子/地面）→ 直接湮灭
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OtherActor);
	if (TargetASC && !TargetASC->HasMatchingGameplayTag(FMyGameplayTags::State_Dead))
	{
		Explode();
		return;
	}

	if (!TargetASC)
	{
		Destroy();
	}
}

void AEOB_Projectile::Explode()
{
	UAbilitySystemComponent* ASC = SourceASC.Get();
	if (ASC && DamageGE)
	{
		TArray<FOverlapResult> Overlaps;
		FCollisionShape Shape = FCollisionShape::MakeSphere(ExplodeRadius);
		GetWorld()->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity, ECC_Pawn, Shape);

		int32 HitCount = 0;
		for (const FOverlapResult& Result : Overlaps)
		{
			AActor* HitActor = Result.GetActor();
			if (!HitActor || HitActor == GetInstigator()) continue;

			UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor);
			if (!TargetASC || TargetASC->HasMatchingGameplayTag(FMyGameplayTags::State_Dead)) continue;

			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			Context.AddInstigator(GetInstigator(), GetInstigator());
			FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(DamageGE, 1.f, Context);
			if (!SpecHandle.IsValid()) continue;

			SpecHandle.Data.Get()->SetSetByCallerMagnitude(FMyGameplayTags::Data_Damage, Damage);
			SpecHandle.Data.Get()->SetSetByCallerMagnitude(FMyGameplayTags::Data_IsCrit, bIsCrit ? 1.f : 0.f);
			ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			HitCount++;
		}
		UE_LOG(LogTemp, Warning, TEXT("[火球] 爆炸！命中 %d 个目标"), HitCount);
	}

	Destroy();
}
