#include "EOB_Projectile.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
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

	// 🌟 碰撞三重保险（针对蓝图覆盖 + 自定义通道默认阻挡两个坑）：
	//    1. 根球体必须是 OverlapAllDynamic（蓝图上已经改好）
	// Sphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	//    2. OverlapAllDynamic 预设对项目自定义通道（HeroType/Enemy）默认是【阻挡】，
	//       阻挡不产生重叠事件 → 火球撞怪毫无反应。这里强制全部通道改为重叠。
	// Sphere->SetCollisionResponseToAllChannels(ECR_Overlap);
	//    3. 唯独忽略主角（HeroType = GameTraceChannel1），免得出生瞬间和自己叠一下
	// Sphere->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);

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
		// 🟠 调试范围显示：橙色线框球 = 爆炸判定半径，存在 2 秒，验收完可删
		DrawDebugSphere(GetWorld(), GetActorLocation(), ExplodeRadius, 24, FColor::Orange, false, 2.f, 0, 3.f);

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
