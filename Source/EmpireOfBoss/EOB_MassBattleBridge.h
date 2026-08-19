#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/MassBattleAgentInterface.h"
#include "MassBattleStructs.h"
#include "EOB_MassBattleBridge.generated.h"

class UMassBattleAgentConfigDataAsset;
class AFlowField;
class UDataTable;

/**
 * M6b：EOB × MassBattle 对接桥（割草体系入口）
 *
 * 职责：
 *  1. 开局把主角塞进流场 GoalActors（怪群自动涌向主角），并周期性刷新流场；
 *  2. 按配置环形刷出 Mass 怪物实体（SpawnWave，可在编辑器详情面板随时手动再刷）；
 *  3. 对每只实体绑定流场 + 事件接收者（本 Actor）；
 *  4. 收插件 OnDeath 回调 → 复用 EOB 掉落表掷点生成 AEOB_PickupBase + 给主角发经验。
 *
 * 摆放：测试关卡里放一个，详情面板填 EnemyConfig / FlowFieldActor / LootTable 即可。
 */
UCLASS()
class EMPIREOFBOSS_API AEOB_MassBattleBridge : public AActor, public IMassBattleAgentInterface
{
	GENERATED_BODY()

public:
	AEOB_MassBattleBridge();

	virtual void BeginPlay() override;

	// ===================== 刷怪配置 =====================

	/** 怪物配置（插件的 Agent 配置数据资产，一种怪一个 DA） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|MassBattle")
	TObjectPtr<UMassBattleAgentConfigDataAsset> EnemyConfig;

	/** 关卡里摆好的流场（插件 BP_FlowFieldCanvas），怪群沿它涌向主角 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|MassBattle")
	TObjectPtr<AFlowField> FlowFieldActor;

	/** 每波刷多少只 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|MassBattle", meta = (ClampMin = "1"))
	int32 SpawnQuantity = 300;

	/** 怪物队伍编号（1 = 敌方，与伤害过滤的队伍标签对应） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|MassBattle")
	int32 TeamIndex = 1;

	/** 环形刷怪带外半径（以 Bridge 所在位置为圆心） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|MassBattle", meta = (ClampMin = "0"))
	float SpawnOuterRadius = 3000.f;

	/** 环形刷怪带内半径（>0 即成空心环，不会直接刷在主角脸上） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|MassBattle", meta = (ClampMin = "0"))
	float SpawnInnerRadius = 1500.f;

	/** 刷怪贴地检测的地面对象类型（空 = 直接按 Bridge 的高度刷；地面高低不平时填，如 WorldStatic） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|MassBattle")
	TArray<TEnumAsByte<EObjectTypeQuery>> SpawnGroundObjectTypes;

	/** 进游戏是否自动刷第一波 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|MassBattle")
	bool bSpawnOnBeginPlay = true;

	/** 流场刷新间隔秒数（主角移动后怪群最多这么久改向，对齐插件默认 0.5） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|MassBattle", meta = (ClampMin = "0.1"))
	float FlowFieldRefreshInterval = 0.5f;

	// ===================== 掉落 / 经验（复用 M1/M3a 管线） =====================

	/** 掉落表（直接填 M1 的 DT_Loot，行结构 FEOBLootTableRow） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|MassBattle")
	TObjectPtr<UDataTable> LootTable;

	/** 每只怪的击杀经验 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|MassBattle", meta = (ClampMin = "0"))
	float XPReward = 10.f;

	// ===================== 操作 =====================

	/** 再刷一波（蓝图可调，编辑器里也可用"调用函数"按钮手动触发） */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "EOB|MassBattle")
	void SpawnWave();

	// ===================== MassBattle 事件接口 =====================
	// 插件其余 11 个事件（移动/索敌/攻击等）本里程碑用不到，不重写，走默认空实现。

	/** 实体死亡回调：掷掉落 + 发经验 */
	virtual void OnDeath_Implementation(const FDeathData& Data) override;

protected:
	/** 开局引导：绑主角进流场 → 启动流场刷新 → 刷第一波 */
	void BeginBringUp();

	/** 周期刷新流场（让怪群追踪移动中的主角） */
	void RefreshFlowField();

	/** 给单只实体绑定流场 + 事件接收者 */
	void BindEntity(const FEntityHandle& Handle);

	/** 在指定位置按掉落表掷点爆拾取物（模式与 CPP_Enemy_Base::SpawnLoot 一致） */
	void SpawnLootAt(const FVector& Center);

private:
	FTimerHandle BringUpTimerHandle;
	FTimerHandle FlowFieldRefreshTimerHandle;

	/** 累计刷怪数（仅日志用） */
	int32 TotalSpawned = 0;
};
