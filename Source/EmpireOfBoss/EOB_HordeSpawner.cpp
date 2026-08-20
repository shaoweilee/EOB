#include "EOB_HordeSpawner.h"

#include "CPP_Enemy_Base.h"
#include "Components/BillboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
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

	// 立即补第一批，之后按间隔循环
	World->GetTimerManager().SetTimer(SpawnTimer, this, &AEOB_HordeSpawner::SpawnTick, RespawnInterval, true, 0.f);
	UE_LOG(LogTemp, Log, TEXT("[刷怪器] %s 开刷：中心=%s，同屏上限 %d，环带 %.0f~%.0f，间隔 %.1f 秒"),
	       *GetName(), bSpawnAroundHero ? TEXT("主角") : TEXT("刷怪器自身"),
	       MaxAlive, InnerRadius, OuterRadius, RespawnInterval);
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

	// 环带上随机取点
	const float AngleDeg = FMath::RandRange(0.f, 360.f);
	const float Radius = FMath::RandRange(InnerRadius, FMath::Max(InnerRadius, OuterRadius));
	FVector SpawnLoc = CenterLoc + FVector(
		FMath::Cos(FMath::DegreesToRadians(AngleDeg)) * Radius,
		FMath::Sin(FMath::DegreesToRadians(AngleDeg)) * Radius,
		0.f);

	// 垂直下扫贴地（和 SpawnLoot 同一个地面通道 GameTraceChannel2）
	FHitResult GroundHit;
	FCollisionQueryParams Params;
	if (Hero)
	{
		Params.AddIgnoredActor(Hero);
	}
	if (!World->LineTraceSingleByChannel(GroundHit,
	                                     SpawnLoc + FVector(0.f, 0.f, GroundTraceUp),
	                                     SpawnLoc - FVector(0.f, 0.f, GroundTraceDown),
	                                     ECC_GameTraceChannel2, Params))
	{
		// 点在深渊/镂空上：宁可这次不刷，也绝不把怪扔进地下
		return false;
	}

	// 只认接近水平的可行走面（0.7 ≈ 45° 坡度上限），防止把怪刷在岩壁立面上
	if (GroundHit.ImpactNormal.Z < 0.7f)
	{
		return false;
	}

	// 🌟 落点抬到胶囊体半高：从根上杜绝"刷出来卡进地里/掉下去"
	const float HalfHeight = EnemyClass->GetDefaultObject<ACPP_Enemy_Base>()
	                                   ->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	SpawnLoc = GroundHit.ImpactPoint + FVector(0.f, 0.f, HalfHeight + 2.f);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ACPP_Enemy_Base* NewEnemy = World->SpawnActor<ACPP_Enemy_Base>(
		EnemyClass, SpawnLoc, FRotator::ZeroRotator, SpawnParams);
	if (!NewEnemy) return false;

	Spawned.Add(NewEnemy);
	return true;
}
