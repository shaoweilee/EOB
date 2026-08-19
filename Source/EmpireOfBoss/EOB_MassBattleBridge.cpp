#include "EOB_MassBattleBridge.h"

#include "FuncLibs/MassBattleFuncLib.h"
#include "DataAssets/MassBattleAgentConfigDataAsset.h"
#include "MassBattleEnums.h"
#include "FlowField.h"

#include "EOB_LootTableRow.h"
#include "EOB_PickupBase.h"
#include "EOB_LevelComponent.h"
#include "EmpireOfBossCharacter.h"

#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

AEOB_MassBattleBridge::AEOB_MassBattleBridge()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AEOB_MassBattleBridge::BeginPlay()
{
	Super::BeginPlay();

	// 延迟 0.2 秒再引导：等主角 Pawn 生成、MassBattle 各子系统就绪（M6a 已验证它们随世界初始化）
	// 若届时主角仍拿不到，BeginBringUp 里会重试一次
	FTimerDelegate BringUpDelegate;
	BringUpDelegate.BindUObject(this, &AEOB_MassBattleBridge::BeginBringUp);
	GetWorld()->GetTimerManager().SetTimer(BringUpTimerHandle, BringUpDelegate, 0.2f, false);
}

void AEOB_MassBattleBridge::BeginBringUp()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// ---------- 1. 把主角塞进流场目标 ----------
	if (FlowFieldActor)
	{
		ACharacter* HeroChar = nullptr;
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			HeroChar = PC->GetCharacter();
		}

		if (HeroChar)
		{
			FlowFieldActor->GoalActors.AddUnique(TSoftObjectPtr<AActor>(HeroChar));
			FlowFieldActor->UpdateFlowField(false); // 立刻重算一次，让流向指向主角

			// 周期刷新：主角移动后怪群改向（流场重算本身很便宜，33×33 网格是微秒级）
			World->GetTimerManager().SetTimer(
				FlowFieldRefreshTimerHandle, this, &AEOB_MassBattleBridge::RefreshFlowField,
				FlowFieldRefreshInterval, true);

			UE_LOG(LogTemp, Log, TEXT("[EOB×MassBattle] 流场已绑定主角 %s，每 %.1f 秒刷新"),
			       *HeroChar->GetName(), FlowFieldRefreshInterval);
		}
		else
		{
			// 主角还没生成好（极端时序）：0.5 秒后重试一次整个引导
			UE_LOG(LogTemp, Warning, TEXT("[EOB×MassBattle] 主角尚未生成，0.5 秒后重试引导"));
			FTimerDelegate RetryDelegate;
			RetryDelegate.BindUObject(this, &AEOB_MassBattleBridge::BeginBringUp);
			World->GetTimerManager().SetTimer(BringUpTimerHandle, RetryDelegate, 0.5f, false);
			return;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[EOB×MassBattle] Bridge 未配置 FlowFieldActor！怪群不会移动。请在详情面板指向关卡里的流场。"));
	}

	// ---------- 2. 首波刷怪 ----------
	if (bSpawnOnBeginPlay)
	{
		SpawnWave();
	}
}

void AEOB_MassBattleBridge::RefreshFlowField()
{
	if (IsValid(FlowFieldActor))
	{
		FlowFieldActor->UpdateFlowField(false);
	}
}

void AEOB_MassBattleBridge::SpawnWave()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (!EnemyConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("[EOB×MassBattle] Bridge 未配置 EnemyConfig（怪物配置 DA），无法刷怪！"));
		return;
	}

	// 环形刷怪带参数：空心环 + 全圆 + 随机散点
	FAgentSpawnPolygonShapeData Shape;
	Shape.OuterRadius = SpawnOuterRadius;
	Shape.InnerRadius = SpawnInnerRadius;
	Shape.SectorAngle = 360.f;
	Shape.PositioningMode = ESpawnPositioningMode::Random;
	Shape.GroundObjectTypes = SpawnGroundObjectTypes; // 空 = 按 Bridge 高度直接刷

	// 插件官方入口（FuncLib 版，子系统里的同名函数已废弃）：一次性刷出，直接拿到实体句柄
	TArray<FEntityHandle> Handles = UMassBattleFuncLib::SpawnAgentsByConfigCircular(
		this, // WorldContextObject
		EnemyConfig, // 怪物配置 DA
		SpawnQuantity, // 数量
		TeamIndex, // 队伍（1 = 敌方）
		GetActorLocation(), // 圆心 = Bridge 位置
		Shape, // 环形参数
		FVector2D::ZeroVector, // 出生初速度
		EInitialRotation::FacePlayer, // 出生面向玩家
		FRotator::ZeroRotator, // 自定义旋转（FacePlayer 模式下不用）
		FSpawnerMult(), // 属性乘数（全 1，以后做难度缩放用）
		true // 出生即激活
	);

	// 逐只绑定：流场（让它涌向主角）+ 事件接收者（让死亡回调到本类）
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
	// 绑流场：实体空闲时自动沿流场移动（配置 DA 里 bMoveByFlowfieldOnIdle 默认开）
	if (FlowFieldActor)
	{
		UMassBattleFuncLib::SetUseFlowField(this, Handle, FlowFieldActor);
	}
	// 绑事件接收者：死亡回调进本类的 OnDeath_Implementation
	UMassBattleFuncLib::SetEventReceiver(this, Handle, this);
}

void AEOB_MassBattleBridge::OnDeath_Implementation(const FDeathData& Data)
{
	UWorld* World = GetWorld();
	if (!World) return;

	// 1. 取死亡位置（回调触发时实体仍在濒死流程中，位置可查；
	//    若实测取到零向量，备用方案是改收 OnHit（FHitData 自带 HitLocation 和 IsKill））
	FVector DeathLoc = FVector::ZeroVector;
	FVector PrevLoc = FVector::ZeroVector;
	FVector InitLoc = FVector::ZeroVector;
	UMassBattleFuncLib::GetAgentLocation(this, Data.SelfEntity, DeathLoc, PrevLoc, InitLoc);

	// 2. 掷掉落表，爆 EOB 拾取物
	SpawnLootAt(DeathLoc);

	// 3. 给主角发经验（复用 M3a 管线）
	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		if (AEmpireOfBossCharacter* Hero = Cast<AEmpireOfBossCharacter>(PC->GetCharacter()))
		{
			if (Hero->LevelComponent)
			{
				Hero->LevelComponent->AddExperience(XPReward);
			}
		}
	}
}

void AEOB_MassBattleBridge::SpawnLootAt(const FVector& Center)
{
	if (!LootTable) return;

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

			// 垂直射线贴地，防止掉在半空或插进地板（与旧敌人同一条地面通道）
			FHitResult FloorHit;
			if (GetWorld()->LineTraceSingleByChannel(FloorHit,
			                                         SpawnLoc + FVector(0.f, 0.f, 100.f),
			                                         SpawnLoc - FVector(0.f, 0.f, 300.f),
			                                         ECC_GameTraceChannel2))
			{
				SpawnLoc = FloorHit.Location + FVector(0.f, 0.f, 2.f);
			}

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			if (AEOB_PickupBase* Pickup = GetWorld()->SpawnActor<AEOB_PickupBase>(
				Row->PickupClass, SpawnLoc, FRotator::ZeroRotator, Params))
			{
				// 掉落行指定了装备定义时覆盖拾取物默认值（M2 机制原样生效）
				if (Row->DroppedItemDefinition)
				{
					Pickup->SetDroppedItemDefinition(Row->DroppedItemDefinition);
				}
			}
		}
	}
}
