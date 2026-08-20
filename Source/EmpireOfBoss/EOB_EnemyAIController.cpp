#include "EOB_EnemyAIController.h"

#include "CPP_Enemy_Base.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Navigation/PathFollowingComponent.h"

AEOB_EnemyAIController::AEOB_EnemyAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AEOB_EnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 每只怪附身时重置状态机（刷出/关卡开始都会走到这里）
	ChaseTarget = nullptr;
	RepathCooldown = 0.f;
	LastDebugState = -1;
}

void AEOB_EnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ACPP_Enemy_Base* Enemy = Cast<ACPP_Enemy_Base>(GetPawn());
	if (!Enemy) return;

	if (Enemy->bIsDead)
	{
		ChaseTarget = nullptr;
		StopMovement();
		DebugReport(0, TEXT("已死亡"), -1.f);
		return;
	}
	if (!Enemy->bChasePlayer)
	{
		DebugReport(1, TEXT("bChasePlayer 是关的"), -1.f);
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = World->GetFirstPlayerController();
	APawn* PlayerPawn = PC ? PC->GetPawn() : nullptr;
	if (!PlayerPawn)
	{
		DebugReport(2, TEXT("找不到主角"), -1.f);
		return;
	}

	const float Dist2D = FVector::Dist2D(Enemy->GetActorLocation(), PlayerPawn->GetActorLocation());

	// 已贴脸：停下（怪打人的攻击逻辑 M6c 再补）
	if (Dist2D <= Enemy->ChaseStopDistance)
	{
		ChaseTarget = nullptr;
		StopMovement();
		DebugReport(3, TEXT("已贴脸，停下"), Dist2D);
		return;
	}

	// 超出警戒范围：原地站桩（AggroRange = 0 表示死追不放）
	if (Enemy->AggroRange > 0.f && Dist2D > Enemy->AggroRange)
	{
		ChaseTarget = nullptr;
		StopMovement();
		DebugReport(4, TEXT("超出警戒范围，站桩"), Dist2D);
		return;
	}

	// 追击中：每 0.5 秒重发一次 MoveTo 跟随移动中的主角，不每帧发（省寻路开销）
	DebugReport(5, TEXT("追击中"), Dist2D);
	RepathCooldown -= DeltaTime;
	if (ChaseTarget.Get() != PlayerPawn || RepathCooldown <= 0.f)
	{
		// 验收半径取停止距离的 9 折；bAllowPartialPaths = true：主角脚下缺导航时也尽量走近
		const EPathFollowingRequestResult::Type ReqResult =
			MoveToActor(PlayerPawn, Enemy->ChaseStopDistance * 0.9f, true, true, false, nullptr, true);
		if (ReqResult == EPathFollowingRequestResult::Failed)
		{
			DebugReport(6, TEXT("MoveTo 失败：这只怪脚下没有导航网格！"), Dist2D);
		}
		ChaseTarget = PlayerPawn;
		RepathCooldown = 0.5f;
	}
}

void AEOB_EnemyAIController::DebugReport(int32 State, const FString& Reason, float Dist2D)
{
	if (!bDebugChase || State == LastDebugState) return;
	LastDebugState = State;

	const FString EnemyName = GetPawn() ? GetPawn()->GetName() : TEXT("无宿主");
	if (Dist2D >= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[追击] %s：%s（距主角 %.0fcm）"), *EnemyName, *Reason, Dist2D);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[追击] %s：%s"), *EnemyName, *Reason);
	}
}
