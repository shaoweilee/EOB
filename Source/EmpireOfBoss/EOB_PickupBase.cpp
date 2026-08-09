#include "EOB_PickupBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EmpireOfBossCharacter.h"
#include "EOPGameInstance.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EOB_ItemDefinition.h"
#include "EOB_InventoryComponent.h"

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

	// 类默认就配了装备定义的（如 BP_Pickup_Sword），也按 DA 刷新一次外观
	ApplyDefinitionVisuals();

	// 装备拾取物把网格落地（金币/药水没有 DroppedItemDefinition，不受影响）
	if (DroppedItemDefinition)
	{
		SnapMeshToGround();
	}
}

void AEOB_PickupBase::SetDroppedItemDefinition(UEOB_ItemDefinition* NewDefinition)
{
	DroppedItemDefinition = NewDefinition;
	// 掉落行覆盖定义时外观也要跟着换（此时 BeginPlay 已跑完，必须手动刷）
	ApplyDefinitionVisuals();
}

void AEOB_PickupBase::ApplyDefinitionVisuals()
{
	// DA 里配了掉落外观网格就套用；没配则保留拾取物蓝图自己的网格
	if (DroppedItemDefinition && DroppedItemDefinition->WorldMesh && PickupMesh)
	{
		PickupMesh->SetStaticMesh(DroppedItemDefinition->WorldMesh);
		// 换完网格包围盒变了，重新落地
		SnapMeshToGround();
	}
}

void AEOB_PickupBase::SnapMeshToGround()
{
	if (!PickupMesh || !PickupMesh->GetStaticMesh()) return;

	// 网格资产的本地包围盒：盒底 = Origin.Z - BoxExtent.Z
	// 把组件 Z 抬成 BoxExtent.Z - Origin.Z，盒底就正好贴到 Actor 原点（= 地面）
	const FBoxSphereBounds Bounds = PickupMesh->GetStaticMesh()->GetBounds();
	FVector Loc = PickupMesh->GetRelativeLocation();
	Loc.Z = Bounds.BoxExtent.Z - Bounds.Origin.Z;
	PickupMesh->SetRelativeLocation(Loc);
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

	// ===== 3. M2 新增：装备入口：掷品质 → 生成实例 → 进背包 =====
	if (DroppedItemDefinition)
	{
		UEOB_InventoryComponent* Inv = Hero->InventoryComponent;
		if (!Inv) return;

		const EEOBRarity Rarity = UEOB_InventoryComponent::RollRarity(
			WhiteWeight, GreenWeight, BlueWeight, GoldWeight);
		const FEOBItemInstance NewItem = Inv->CreateItemInstance(DroppedItemDefinition, Rarity);
		const int32 SlotIndex = Inv->AddItem(NewItem);

		if (SlotIndex == INDEX_NONE)
		{
			// 背包满了：留在地上，不触发拾取表现、不销毁
			UE_LOG(LogTemp, Warning, TEXT("[拾取] 背包已满！%s 留在原地。"),
			       *DroppedItemDefinition->ItemName.ToString());
			return;
		}

		UE_LOG(LogTemp, Log, TEXT("[拾取] 装备入包：%s（品质: %s）"),
		       *DroppedItemDefinition->ItemName.ToString(),
		       *UEnum::GetValueAsString(Rarity));
	}

	// 蓝图表现钩子（音效/特效），然后销毁
	OnCollected(Hero);
	Destroy();
}

void AEOB_PickupBase::OnCollected_Implementation(AEmpireOfBossCharacter* Hero)
{
	// 基类默认无表现，蓝图子类重写来播拾取音效/粒子
}
