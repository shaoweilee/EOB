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
	// （金币/药水没有 DroppedItemDefinition，函数内部会直接返回，不受影响）
	ApplyDefinitionVisuals();
}

void AEOB_PickupBase::SetDroppedItemDefinition(UEOB_ItemDefinition* NewDefinition)
{
	DroppedItemDefinition = NewDefinition;
	// 掉落行覆盖定义时外观也要跟着换（此时 BeginPlay 已跑完，必须手动刷）
	ApplyDefinitionVisuals();
}

void AEOB_PickupBase::SetPresetInstance(const FEOBItemInstance& Instance)
{
	PresetInstance = Instance;
}

void AEOB_PickupBase::ApplyDefinitionVisuals()
{
	if (!DroppedItemDefinition || !PickupMesh) return;

	// DA 里配了掉落外观网格就套用；没配则保留拾取物蓝图自己的网格
	if (DroppedItemDefinition->WorldMesh)
	{
		PickupMesh->SetStaticMesh(DroppedItemDefinition->WorldMesh);
	}

	// 套用 DA 的旋转和缩放（默认 0 度 / 1 倍时等于不变）
	PickupMesh->SetRelativeRotation(DroppedItemDefinition->WorldMeshRotation);
	PickupMesh->SetRelativeScale3D(DroppedItemDefinition->WorldMeshScale);

	// 网格/旋转/缩放任一变化后包围盒都变了，重新落地
	SnapMeshToGround();
}

void AEOB_PickupBase::SnapMeshToGround()
{
	if (!PickupMesh || !PickupMesh->GetStaticMesh()) return;

	// 把网格的本地包围盒按组件当前的相对变换（含旋转和缩放）变换到 Actor 空间，
	// 求出盒子的最低点，然后把组件抬高相应距离，让盒底正好贴到 Actor 原点（= 地面）。
	// 这样无论"躺倒"还是缩放，落地都自动正确。
	const FBox LocalBox = PickupMesh->GetStaticMesh()->GetBoundingBox();
	const FBox ActorBox = LocalBox.TransformBy(PickupMesh->GetRelativeTransform());

	FVector Loc = PickupMesh->GetRelativeLocation();
	Loc.Z -= ActorBox.Min.Z;
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

	// ===== 3. 装备/背包入口：有预设实例原样入包，没有则掷品质现生成 =====
	if (DroppedItemDefinition)
	{
		UEOB_InventoryComponent* Inv = Hero->InventoryComponent;
		if (!Inv) return;

		FEOBItemInstance NewItem;
		if (PresetInstance.IsValid())
		{
			// 玩家丢弃的物品：品质和词缀保持原样
			NewItem = PresetInstance;
		}
		else
		{
			// 怪物掉落：现掷品质（装备=词缀数量，背包=容量）
			const EEOBRarity Rarity = UEOB_InventoryComponent::RollRarity(
				WhiteWeight, GreenWeight, BlueWeight, GoldWeight);
			NewItem = Inv->CreateItemInstance(DroppedItemDefinition, Rarity);
		}

		const int32 EncodedSlot = Inv->AddItem(NewItem);

		if (EncodedSlot == INDEX_NONE)
		{
			// 所有背包都满了：留在地上，不触发拾取表现、不销毁
			UE_LOG(LogTemp, Warning, TEXT("[拾取] 背包已满！%s 留在原地。"),
			       *DroppedItemDefinition->ItemName.ToString());
			return;
		}

		UE_LOG(LogTemp, Log, TEXT("[拾取] 物品入包：%s（品质: %s，第 %d 页第 %d 格）"),
		       *DroppedItemDefinition->ItemName.ToString(),
		       *UEnum::GetValueAsString(NewItem.Rarity),
		       EncodedSlot / UEOB_InventoryComponent::SlotEncodingBase + 1,
		       EncodedSlot % UEOB_InventoryComponent::SlotEncodingBase + 1);
	}

	// 蓝图表现钩子（音效/特效），然后销毁
	OnCollected(Hero);
	Destroy();
}

void AEOB_PickupBase::OnCollected_Implementation(AEmpireOfBossCharacter* Hero)
{
	// 基类默认无表现，蓝图子类重写来播拾取音效/粒子
}
