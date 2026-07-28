#include "EOB_PickupBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EmpireOfBossCharacter.h"
#include "EOPGameInstance.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"

AEOB_PickupBase::AEOB_PickupBase()
{
	PrimaryActorTick.bCanEverTick = false;

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	RootComponent = PickupSphere;

	// 🌟 只和主角（HeroType = ECC_GameTraceChannel1）产生重叠，其他全部忽略
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionObjectType(ECC_WorldDynamic);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
	PickupSphere->SetGenerateOverlapEvents(true);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(PickupSphere);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AEOB_PickupBase::BeginPlay()
{
	Super::BeginPlay();

	// 放在 BeginPlay 设置，蓝图实例上改的 PickupRadius 才能生效
	PickupSphere->SetSphereRadius(PickupRadius);

	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AEOB_PickupBase::OnSphereOverlap);
}

void AEOB_PickupBase::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
                                      bool bFromSweep, const FHitResult& SweepResult)
{
	AEmpireOfBossCharacter* Hero = Cast<AEmpireOfBossCharacter>(OtherActor);
	if (!Hero || Hero->bIsDead) return;

	// ===== 1. 金币入口 =====
	if (GoldValue > 0)
	{
		if (UEOPGameInstance* GI = Cast<UEOPGameInstance>(UGameplayStatics::GetGameInstance(this)))
		{
			GI->AddGold(GoldValue);
			UE_LOG(LogTemp, Log, TEXT("[拾取] 金币 +%d，当前总金币: %d"), GoldValue, GI->GoldAmount);
		}
	}

	// ===== 2. 药水/增益入口：直接灌 GE =====
	if (GrantedEffectClass && Hero->AbilitySystemComponent)
	{
		FGameplayEffectContextHandle Context = Hero->AbilitySystemComponent->MakeEffectContext();
		Context.AddSourceObject(this);
		FGameplayEffectSpecHandle SpecHandle = Hero->AbilitySystemComponent->MakeOutgoingSpec(
			GrantedEffectClass, 1.f, Context);
		if (SpecHandle.IsValid())
		{
			Hero->AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			UE_LOG(LogTemp, Log, TEXT("[拾取] 药水生效，已对主角施加 GE: %s"), *GrantedEffectClass->GetName());
		}
	}

	// 蓝图表现钩子（音效/特效），然后销毁
	OnCollected(Hero);
	Destroy();
}

void AEOB_PickupBase::OnCollected_Implementation(AEmpireOfBossCharacter* Hero)
{
	// 基类默认无表现，蓝图子类重写来播拾取音效/粒子
}
