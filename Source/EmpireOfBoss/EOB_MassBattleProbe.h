/*
* EmpireOfBoss — M6b 诊断探针
* 新建 C++ 文件，父类：AActor
* 用途：定位"怪物到达目标格后永久冻结"的根因
*/

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EOB_MassBattleProbe.generated.h"

class AFlowField;

UCLASS()
class EMPIREOFBOSS_API AEOB_MassBattleProbe : public AActor
{
	GENERATED_BODY()

public:
	AEOB_MassBattleProbe();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 指向关卡里的 BP_FlowFieldCanvas 实例（必须指定，否则探针只报红字） */
	UPROPERTY(EditAnywhere, Category = "M6B Probe")
	TObjectPtr<AFlowField> FlowFieldActor = nullptr;

	/** 探针输出间隔（秒） */
	UPROPERTY(EditAnywhere, Category = "M6B Probe")
	float ProbeInterval = 1.0f;

	/** 以主角为中心的怪物统计半径（厘米），要罩住整片刷怪区 */
	UPROPERTY(EditAnywhere, Category = "M6B Probe")
	float TraceRadius = 8000.0f;

private:
	FTimerHandle ProbeTimerHandle;

	void RunProbe();
};
