/*
* EmpireOfBoss — M6b 诊断探针
* 新建 C++ 文件，父类：AActor
*/

#include "EOB_MassBattleProbe.h"

#include "FlowField.h"                        // AFlowField / FFlowFieldNavResult（FlowFieldCanvas 模块）
#include "FuncLibs/MassBattleFuncLib.h"       // UMassBattleFuncLib（MassBattle 模块）
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AEOB_MassBattleProbe::AEOB_MassBattleProbe()
{
	PrimaryActorTick.bCanEverTick = false; // 全程用定时器，不占 Tick
}

void AEOB_MassBattleProbe::BeginPlay()
{
	Super::BeginPlay();

	// 首次延迟 1 秒，等 Bridge 刷怪完成；之后按间隔循环
	GetWorldTimerManager().SetTimer(
		ProbeTimerHandle, this, &AEOB_MassBattleProbe::RunProbe,
		ProbeInterval, true, 1.0f);
}

void AEOB_MassBattleProbe::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(ProbeTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void AEOB_MassBattleProbe::RunProbe()
{
	// ---- 0. 基础检查 ----
	if (!IsValid(FlowFieldActor))
	{
		UE_LOG(LogTemp, Error, TEXT("[M6B-PROBE] Flow Field Actor 未指定！请在 Details 面板把关卡里的 BP_FlowFieldCanvas 实例指进来。"));
		return;
	}

	// ---- 1. 主角位置 ----
	const APawn* HeroPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector HeroLoc = HeroPawn ? HeroPawn->GetActorLocation() : FVector::ZeroVector;

	// ---- 2. 流场目标侧：目标缓存是否还在跟踪主角 ----
	const int32 GoalActorCount = FlowFieldActor->GoalActors.Num();
	FString GoalCacheStr = TEXT("(空)");
	float GoalToHeroDist = -1.f;
	if (FlowFieldActor->GoalActorLocations.Num() > 0)
	{
		const FVector& GoalLoc0 = FlowFieldActor->GoalActorLocations[0];
		GoalToHeroDist = FVector::Dist2D(GoalLoc0, HeroLoc);
		GoalCacheStr = GoalLoc0.ToString();
	}

	// ---- 3. 怪物侧：总数 / 在动数 / 停住数，并找出离主角最近的“停住”的怪 ----
	bool bHit = false;
	TArray<FTraceResult> TraceResults;
	UMassBattleFuncLib::SphereTraceForAgents(this, bHit, TraceResults, -1, HeroLoc, TraceRadius);
	const TArray<FEntityHandle> Handles = UMassBattleFuncLib::ConvertTraceResultsToEntityHandles(TraceResults);

	int32 MovingCount = 0;
	float NearestStoppedDistSq = FLT_MAX;
	FVector NearestStoppedLoc = FVector::ZeroVector;
	bool bHasStopped = false;

	for (const FEntityHandle& Handle : Handles)
	{
		const FVector Vel = UMassBattleFuncLib::GetAgentVelocity(this, Handle);
		if (Vel.Size2D() > 10.f)
		{
			MovingCount++;
			continue;
		}

		FVector Loc, PrevLoc, InitLoc;
		UMassBattleFuncLib::GetAgentLocation(this, Handle, Loc, PrevLoc, InitLoc);
		const float DistSq = FVector::DistSquared2D(Loc, HeroLoc);
		if (DistSq < NearestStoppedDistSq)
		{
			NearestStoppedDistSq = DistSq;
			NearestStoppedLoc = Loc;
			bHasStopped = true;
		}
	}

	// ---- 4. 探针亲自做两次流场查询（与怪物每帧调用的是同一个函数）----
	// 查询参数模拟 DA_MBA_Grunt 的小胶囊：搜索半径 40，搜索盒 (50, 50, 150)
	const FVector SearchExtent(50.0, 50.0, 150.0);

	FString StoppedQueryStr = TEXT("(当前没有停住的怪)");
	if (bHasStopped)
	{
		const FFlowFieldNavResult R = FlowFieldActor->QueryNavigationAdvanced(
			NearestStoppedLoc, 40.0f, SearchExtent, nullptr);
		StoppedQueryStr = FString::Printf(
			TEXT("位置=%s | Valid=%d | AtGoal=%d | DistToGoal=%d | Dir=%s | GoalLoc=%s"),
			*NearestStoppedLoc.ToString(),
			R.bIsValid ? 1 : 0,
			R.bIsAtGoal ? 1 : 0,
			R.DistanceToGoal,
			*R.Direction.ToString(),
			*R.GoalLocation.ToString());
	}

	const FFlowFieldNavResult HeroR = FlowFieldActor->QueryNavigationAdvanced(HeroLoc, 40.0f, SearchExtent, nullptr);
	const FString HeroQueryStr = FString::Printf(
		TEXT("Valid=%d | AtGoal=%d | DistToGoal=%d | Dir=%s"),
		HeroR.bIsValid ? 1 : 0,
		HeroR.bIsAtGoal ? 1 : 0,
		HeroR.DistanceToGoal,
		*HeroR.Direction.ToString());

	// ---- 5. 汇总输出 ----
	UE_LOG(LogTemp, Warning,
	       TEXT(
		       "[M6B-PROBE] 主角=%s | GoalActors=%d | 目标缓存[0]=%s | 缓存离主角=%.0f | bIsBeginPlay=%d | NextTickLeft=%.2f || 怪物: 总数=%d 在动=%d 停住=%d || 最近停住怪查询: %s || 主角脚下查询: %s"
	       ),
	       *HeroLoc.ToString(),
	       GoalActorCount,
	       *GoalCacheStr,
	       GoalToHeroDist,
	       FlowFieldActor->bIsBeginPlay ? 1 : 0,
	       FlowFieldActor->nextTickTimeLeft,
	       Handles.Num(), MovingCount, Handles.Num() - MovingCount,
	       *StoppedQueryStr,
	       *HeroQueryStr);
}
