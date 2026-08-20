#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EOB_HordeSpawner.generated.h"

class ACPP_Enemy_Base;
class UBillboardComponent;
class USceneComponent;

/**
 * M6 重做：原生刷怪器（替代 MassBattle 割草方案）。
 * 默认围着刷怪器自己刷怪（摆在哪儿就刷在哪儿，巢穴模式）；
 * 勾上 bSpawnAroundHero 则围着主角刷（随行压力模式）。
 * 同屏存活数由 MaxAlive 封顶；击杀掉落/经验/词缀全走敌人自身 OnDeath 管线。
 * 不需要导航网格：敌人直线追击 + RVO 避让，空旷地图天然适配。
 */
UCLASS()
class EMPIREOFBOSS_API AEOB_HordeSpawner : public AActor
{
	GENERATED_BODY()

public:
	AEOB_HordeSpawner();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ===================== 配置 =====================

	/** 刷什么怪：选敌人蓝图子类（比如 BP_Enemy_Rat）。掉落表/等级/词缀都在蓝图默认值里配 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Horde")
	TSubclassOf<ACPP_Enemy_Base> EnemyClass;

	/** 刷怪中心：false = 围着刷怪器自己刷（摆哪刷哪）；true = 围着主角刷（怪永远来找你） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Horde")
	bool bSpawnAroundHero = false;

	/** 同屏存活上限：10~20 官方建议区间；想更割草调到 30~40，压测可拉 50/100 看帧率 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Horde", meta = (ClampMin = "1"))
	int32 MaxAlive = 20;

	/** 刷怪环内径（距刷怪中心，厘米）：巢穴模式默认 0 即可；围主角模式建议 1500 防止刷脸 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Horde")
	float InnerRadius = 0.f;

	/** 刷怪环外径：巢穴模式默认 20 米；围主角模式建议 3000 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Horde")
	float OuterRadius = 2000.f;

	/** 补怪间隔（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Horde", meta = (ClampMin = "0.1"))
	float RespawnInterval = 0.5f;

	/** 每个间隔最多补几只（分批补，避免一帧刷一堆造成的突帧和穿帮） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Horde", meta = (ClampMin = "1"))
	int32 SpawnPerTick = 3;

	/** 总刷怪预算：0 = 无限刷（割草模式）；>0 = 刷完即止（波次模式） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Horde", meta = (ClampMin = "0"))
	int32 TotalBudget = 0;

	/** PIE 开始自动刷 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Horde")
	bool bAutoStart = true;

	/** 贴地射线：从刷怪点上空 GroundTraceUp 厘米处往下打 GroundTraceDown 厘米 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Horde")
	float GroundTraceUp = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Horde")
	float GroundTraceDown = 4000.f;

	// ===================== 手动开关（PIE 中选中本 Actor，详情面板点按钮） =====================

	UFUNCTION(CallInEditor, Category = "EOB|Horde")
	void StartSpawn();

	UFUNCTION(CallInEditor, Category = "EOB|Horde")
	void StopSpawn();

	/** 当前存活怪数量（调试用，蓝图可读） */
	UFUNCTION(BlueprintPure, Category = "EOB|Horde")
	int32 GetAliveCount() const;

protected:
	/** 补怪定时器回调：清死亡登记 → 按缺口补怪 */
	void SpawnTick();

	/** 尝试在环带随机点刷一只，返回是否成功（点在深渊/陡坡上则放弃本次） */
	bool TrySpawnOne();

	// ===================== 组件 =====================

	/** 场景根组件：有了它刷怪器才能在关卡里摆放/拖动/定位 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EOB|Horde")
	TObjectPtr<USceneComponent> RootScene;

	/** 编辑器图标：关卡里一眼看到刷怪器位置（游戏中自动隐藏） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EOB|Horde")
	TObjectPtr<UBillboardComponent> EditorSprite;

	// ===================== 运行状态 =====================

	FTimerHandle SpawnTimer;

	/** 存活登记（弱引用，怪死亡销毁后自动失效） */
	TArray<TWeakObjectPtr<ACPP_Enemy_Base>> Spawned;

	/** 累计已刷数量（配合 TotalBudget） */
	int32 SpawnedTotal = 0;
};
