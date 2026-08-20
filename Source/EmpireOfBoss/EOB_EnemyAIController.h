#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EOB_EnemyAIController.generated.h"

class ACPP_Enemy_Base;

/**
 * M6 重做：敌人 AI 控制器（CPP_Enemy_Base 构造函数里自动指定并附身，无需手动添加）。
 * 大脑极简：主角进警戒范围 → MoveToActor 贴脸；贴脸/出范围 → 停下。
 * 寻路走关卡导航网格（NavMeshBoundsVolume），障碍物自动绕行；
 * 没铺导航的区域 MoveTo 会失败，日志会直接报出来。
 */
UCLASS()
class EMPIREOFBOSS_API AEOB_EnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEOB_EnemyAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

	/** 输出每只怪追击状态变化的调试日志（排查用，验收完关掉） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|AI")
	bool bDebugChase = true;

protected:
	/** 状态变化时打一行日志；状态没变不重复刷屏 */
	void DebugReport(int32 State, const FString& Reason, float Dist2D);

	/** 当前追随中的目标（避免每帧重复发 MoveTo） */
	TWeakObjectPtr<APawn> ChaseTarget;

	/** 距上次发 MoveTo 的剩余冷却（主角会移动，每 0.5 秒重发一次路径请求） */
	float RepathCooldown = 0.f;

	/** 上一次已上报的状态 */
	int32 LastDebugState = -1;
};
