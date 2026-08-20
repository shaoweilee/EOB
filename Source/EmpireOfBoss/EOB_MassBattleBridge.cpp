#include "EOB_MassBattleBridge.h"

#include "Components/SceneComponent.h"
#include "Components/BillboardComponent.h"
#include "Engine/World.h"
#include "Engine/DataTable.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"

// 🌾 MassBattle 插件
#include "FuncLibs/MassBattleFuncLib.h"
#include "DataAssets/MassBattleAgentConfigDataAsset.h"
#include "MassBattleEnums.h"
#include "FlowField.h"

// 🎒 EOB 掉落/经验体系
#include "EOB_LootTableRow.h"
#include "EOB_PickupBase.h"
#include "EOB_LevelComponent.h"
#include "EmpireOfBossCharacter.h"

AEOB_MassBattleBridge::AEOB_MassBattleBridge()
{
	PrimaryActorTick.bCanEverTick = false;

	// 🧷 场景根组件：没有它，摆进关卡的实例既不能移动，GetActorLocation() 也恒为 (0,0,0)——刷怪圆心会跑到世界原点
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// 🏷️ 编辑器小图标：方便在关卡视口里点选（游戏里不渲染）
	UBillboardComponent* EditorIcon = CreateDefaultSubobject<UBillboardComponent>(TEXT("EditorIcon"));
	EditorIcon->SetupAttachment(Root);
}

void AEOB_MassBattleBridge::BeginPlay()
{
	Super::BeginPlay();

	// PIE 下玩家角色可能还没生成完，延迟 0.2s 再启动；英雄仍不在就继续重试（见 BeginBringUp）
	GetWorldTimerManager().SetTimer(
		BringUpTimerHandle,
		FTimerDelegate::CreateUObject(this, &AEOB_MassBattleBridge::BeginBringUp),
		0.2f, false);
}

void AEOB_MassBattleBridge::BeginBringUp()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (FlowFieldActor)
	{
		ACharacter* HeroChar = nullptr;
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			HeroChar = PC->GetCharacter();
		}

		if (!HeroChar)
		{
			// 英雄还没就位：0.5s 后重试
			GetWorldTimerManager().SetTimer(
				BringUpTimerHandle,
				FTimerDelegate::CreateUObject(this, &AEOB_MassBattleBridge::BeginBringUp),
				0.5f, false);
			return;
		}

		// 把英雄设为流场目标：Mass 怪沿流场箭头自动追人，零 AI 代码
		FlowFieldActor->GoalActors.AddUnique(TSoftObjectPtr<AActor>(HeroChar));
		FlowFieldActor->UpdateFlowField(false);

		// 英雄移动后流场定期重算（默认 0.5s 一次）
		GetWorldTimerManager().SetTimer(
			FlowFieldRefreshTimerHandle,
			FTimerDelegate::CreateUObject(this, &AEOB_MassBattleBridge::RefreshFlowField),
			FlowFieldRefreshInterval, true);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[EOB×MassBattle] Bridge 未配置 FlowFieldActor，群怪不会追人！请在关卡实例上指定流场。"));
	}

	if (bSpawnOnBeginPlay)
	{
		SpawnWave();
	}
}

void AEOB_MassBattleBridge::RefreshFlowField()
{
	if (FlowFieldActor)
	{
		FlowFieldActor->UpdateFlowField(false);
	}
}

void AEOB_MassBattleBridge::SpawnWave()
{
	if (!EnemyConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("[EOB×MassBattle] Bridge 未配置 EnemyConfig（DA_MBA_Grunt），无法刷怪。"));
		return;
	}

	// 圆环区域刷怪：内半径~外半径之间随机布点，贴地检测走 SpawnGroundObjectTypes
	FAgentSpawnPolygonShapeData Shape;
	Shape.OuterRadius = SpawnOuterRadius;
	Shape.InnerRadius = SpawnInnerRadius;
	Shape.SectorAngle = 360.f;
	Shape.PositioningMode = ESpawnPositioningMode::Random;
	Shape.GroundObjectTypes = SpawnGroundObjectTypes;

	TArray<FEntityHandle> Handles = UMassBattleFuncLib::SpawnAgentsByConfigCircular(
		this, EnemyConfig, SpawnQuantity, TeamIndex, GetActorLocation(),
		Shape, FVector2D::ZeroVector, EInitialRotation::FacePlayer, FRotator::ZeroRotator,
		FSpawnerMult(), true);

	for (const FEntityHandle& Handle : Handles)
	{
		BindEntity(Handle);
	}

	TotalSpawned += Handles.Num();
	UE_LOG(LogTemp, Log, TEXT("[EOB×MassBattle] 本波刷出 %d 只，场上累计 %d 只（队伍 %d）"),
	       Handles.Num(), TotalSpawned, TeamIndex);
}

void AEOB_MassBattleBridge::BindEntity(const FEntityHandle& Handle)
{
	if (FlowFieldActor)
	{
		UMassBattleFuncLib::SetUseFlowField(this, Handle, FlowFieldActor);
	}
	UMassBattleFuncLib::SetEventReceiver(this, Handle, this);
}

void AEOB_MassBattleBridge::OnDeath_Implementation(const FDeathData& Data)
{
	// 死亡位置：FDeathData 不带坐标，回调里现取（若取到无效值，备用方案是改收 OnHit，FHitData 自带 HitLocation + IsKill）
	FVector DeathLoc = FVector::ZeroVector;
	FVector PrevLoc = FVector::ZeroVector;
	FVector InitLoc = FVector::ZeroVector;
	UMassBattleFuncLib::GetAgentLocation(this, Data.SelfEntity, DeathLoc, PrevLoc, InitLoc);

	// ① 掉落：复用 EOB 掉落表逻辑，以死亡点为中心撒
	SpawnLootAt(DeathLoc);

	// ② 经验：照旧发给英雄
	ACharacter* HeroChar = nullptr;
	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			HeroChar = PC->GetCharacter();
		}
	}
	if (AEmpireOfBossCharacter* Hero = Cast<AEmpireOfBossCharacter>(HeroChar))
	{
		if (Hero->LevelComponent)
		{
			Hero->LevelComponent->AddExperience(XPReward);
		}
	}
}

void AEOB_MassBattleBridge::SpawnLootAt(const FVector& Center)
{
	if (!LootTable) return;
	UWorld* World = GetWorld();
	if (!World) return;

	// 和 CPP_Enemy_Base::SpawnLoot 同一套掷点逻辑，只是撒落中心从"敌人脚下"换成"实体死亡位置"
	static const FString ContextString(TEXT("MassEnemyLootRoll"));
	TArray<FEOBLootTableRow*> Rows;
	LootTable->GetAllRows<FEOBLootTableRow>(ContextString, Rows);

	for (const FEOBLootTableRow* Row : Rows)
	{
		if (!Row || !Row->PickupClass) continue;

		// 独立概率判定
		if (FMath::FRand() > Row->DropChance) continue;

		const int32 Count = FMath::RandRange(Row->MinCount, FMath::Max(Row->MinCount, Row->MaxCount));
		for (int32 i = 0; i < Count; ++i)
		{
			// 死亡点周围 80cm 内随机散落，火炬之光式的爆一地
			FVector SpawnLoc = Center + FVector(
				FMath::RandRange(-80.f, 80.f), FMath::RandRange(-80.f, 80.f), 0.f);

			// 垂直射线贴地，防止掉在半空或插进地板
			FHitResult FloorHit;
			if (World->LineTraceSingleByChannel(FloorHit,
			                                    SpawnLoc + FVector(0.f, 0.f, 100.f),
			                                    SpawnLoc - FVector(0.f, 0.f, 300.f),
			                                    ECC_GameTraceChannel2))
			{
				SpawnLoc = FloorHit.Location + FVector(0.f, 0.f, 2.f);
			}

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (AEOB_PickupBase* Pickup = World->SpawnActor<AEOB_PickupBase>(
				Row->PickupClass, SpawnLoc, FRotator::ZeroRotator, Params))
			{
				// 🌟 掉落行指定了装备定义时，覆盖拾取物类上的默认值：
				//    这样所有装备共用一个拾取物类，掉落行自己决定掉什么
				if (Row->DroppedItemDefinition)
				{
					Pickup->SetDroppedItemDefinition(Row->DroppedItemDefinition);
				}
			}
		}
	}
}
