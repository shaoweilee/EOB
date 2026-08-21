#include "EOB_HordeSpawner.h"

#include "CPP_Enemy_Base.h"
#include "Components/BillboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

AEOB_HordeSpawner::AEOB_HordeSpawner()
{
	// 全靠定时器驱动，不占每帧 Tick
	PrimaryActorTick.bCanEverTick = false;

	// 场景根组件：有了它才能在关卡里摆放/拖动/定位刷怪器
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	// 编辑器图标：关卡里一眼看到刷怪器在哪（游戏中自动隐藏）
	EditorSprite = CreateDefaultSubobject<UBillboardComponent>(TEXT("EditorSprite"));
	EditorSprite->SetupAttachment(RootComponent);
	EditorSprite->bIsScreenSizeScaled = true;
}

void AEOB_HordeSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoStart)
	{
		StartSpawn();
	}
}

void AEOB_HordeSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void AEOB_HordeSpawner::StartSpawn()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (!EnemyClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[刷怪器] %s 没配 EnemyClass，刷不了怪！请在详情面板选择敌人蓝图类。"), *GetName());
		return;
	}

	if (bRequireNav && !UNavigationSystemV1::GetCurrent(World))
	{
		UE_LOG(LogTemp, Warning, TEXT("[刷怪器] %s 勾了只刷在导航上，但世界里没有导航系统！落点安检会全部失败。"), *GetName());
	}

	// 立即补第一批，之后按间隔循环
	World->GetTimerManager().SetTimer(SpawnTimer, this, &AEOB_HordeSpawner::SpawnTick, RespawnInterval, true, 0.f);
	UE_LOG(LogTemp, Log, TEXT("[刷怪器] %s 开刷：中心=%s，同屏上限 %d，环带 %.0f~%.0f，间隔 %.1f 秒，导航安检=%s"),
	       *GetName(), bSpawnAroundHero ? TEXT("主角") : TEXT("刷怪器自身"),
	       MaxAlive, InnerRadius, OuterRadius, RespawnInterval,
	       bRequireNav ? TEXT("开") : TEXT("关"));
}

void AEOB_HordeSpawner::StopSpawn()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimer);
	}
	UE_LOG(LogTemp, Log, TEXT("[刷怪器] %s 停止刷怪（场上已刷的 %d 只不回收）"), *GetName(), GetAliveCount());
}

int32 AEOB_HordeSpawner::GetAliveCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<ACPP_Enemy_Base>& Ptr : Spawned)
	{
		if (Ptr.IsValid() && !Ptr->bIsDead)
		{
			++Count;
		}
	}
	return Count;
}

void AEOB_HordeSpawner::SpawnTick()
{
	// 清掉已销毁/已死亡的登记（尸体 3 秒后 Destroy，弱引用自动失效）
	Spawned.RemoveAllSwap([](const TWeakObjectPtr<ACPP_Enemy_Base>& Ptr)
	{
		return !Ptr.IsValid() || Ptr->bIsDead;
	});

	// 波次模式：预算刷完自动停
	if (TotalBudget > 0 && SpawnedTotal >= TotalBudget)
	{
		StopSpawn();
		return;
	}

	const int32 AliveDeficit = MaxAlive - Spawned.Num();
	const int32 BudgetLeft = TotalBudget > 0 ? TotalBudget - SpawnedTotal : INT32_MAX;
	const int32 ToSpawn = FMath::Min3(AliveDeficit, SpawnPerTick, BudgetLeft);

	for (int32 i = 0; i < ToSpawn; ++i)
	{
		if (TrySpawnOne())
		{
			++SpawnedTotal;
		}
	}
}

bool AEOB_HordeSpawner::TrySpawnOne()
{
	UWorld* World = GetWorld();
	if (!World || !EnemyClass) return false;

	const APawn* Hero = UGameplayStatics::GetPlayerPawn(this, 0);

	// 刷怪中心：默认围着刷怪器自己（摆哪刷哪）；勾了 bSpawnAroundHero 就围着主角刷
	FVector CenterLoc;
	if (bSpawnAroundHero)
	{
		if (!Hero) return false;
		CenterLoc = Hero->GetActorLocation();
	}
	else
	{
		CenterLoc = GetActorLocation();
	}

	// 敌人胶囊体真实尺寸（贴地抬升和占位检测都要用）
	const ACPP_Enemy_Base* EnemyCDO = EnemyClass->GetDefaultObject<ACPP_Enemy_Base>();
	const float CapsuleRadius = EnemyCDO->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float CapsuleHalfHeight = EnemyCDO->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	// 导航系统（勾了 bRequireNav 才用）
	UNavigationSystemV1* NavSys = bRequireNav ? UNavigationSystemV1::GetCurrent(World) : nullptr;
	if (bRequireNav && !NavSys)
	{
		return false; // StartSpawn 里已报警告，这里静默放弃
	}

	for (int32 Attempt = 0; Attempt < FMath::Max(1, MaxSpawnAttempts); ++Attempt)
	{
		// 环带上随机取点
		const float AngleDeg = FMath::RandRange(0.f, 360.f);
		const float Radius = FMath::RandRange(InnerRadius, FMath::Max(InnerRadius, OuterRadius));
		const FVector CandidateXY = CenterLoc + FVector(
			FMath::Cos(FMath::DegreesToRadians(AngleDeg)) * Radius,
			FMath::Sin(FMath::DegreesToRadians(AngleDeg)) * Radius,
			0.f);

		// ① 地面安检：垂直下扫，深渊/镂空直接换点
		FHitResult GroundHit;
		FCollisionQueryParams TraceParams;
		if (Hero)
		{
			TraceParams.AddIgnoredActor(Hero);
		}
		if (!World->LineTraceSingleByChannel(GroundHit,
		                                     CandidateXY + FVector(0.f, 0.f, GroundTraceUp),
		                                     CandidateXY - FVector(0.f, 0.f, GroundTraceDown),
		                                     ECC_GameTraceChannel2, TraceParams))
		{
			continue;
		}
		// 陡坡/立面（0.7 ≈ 45° 上限）换点
		if (GroundHit.ImpactNormal.Z < 0.7f)
		{
			continue;
		}

		FVector GroundLoc = GroundHit.ImpactPoint;

		// ② 导航安检：把落点投影到导航网格上。
		//    投不上 = 障碍内部/无导航区/石头顶，换点；投上了用导航的精确表面位置
		if (NavSys)
		{
			FNavLocation NavLoc;
			if (!NavSys->ProjectPointToNavigation(GroundLoc, NavLoc, NavSnapExtent))
			{
				continue;
			}
			GroundLoc = NavLoc.Location;
		}

		// ③ 占位安检：用怪的真实胶囊体试摆，被岩石/树干/别的怪/主角占着就换点，
		//    绝不把怪刷进障碍物内部
		const FVector SpawnLoc = GroundLoc + FVector(0.f, 0.f, CapsuleHalfHeight + 2.f);
		const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);

		FCollisionObjectQueryParams OccupyObjectTypes;
		OccupyObjectTypes.AddObjectTypesToQuery(ECC_WorldStatic);
		OccupyObjectTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
		OccupyObjectTypes.AddObjectTypesToQuery(ECC_Pawn);

		if (World->OverlapAnyTestByObjectType(SpawnLoc, FQuat::Identity, OccupyObjectTypes, CapsuleShape))
		{
			continue;
		}

		// ④ 三关全过，刷！
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ACPP_Enemy_Base* NewEnemy = World->SpawnActor<ACPP_Enemy_Base>(
			EnemyClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams);
		if (!NewEnemy) return false;

		Spawned.Add(NewEnemy);
		return true;
	}

	// 连试 MaxSpawnAttempts 个点都不合格（比如环带大部分在障碍区）：放弃这只，下个间隔再试
	return false;
}
