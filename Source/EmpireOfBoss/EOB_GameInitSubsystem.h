#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EOB_GameInitSubsystem.generated.h"

// 前向声明，避免头文件循环包含
class AEmpireOfBossPlayerController;

UCLASS()
class EMPIREOFBOSS_API UEOB_GameInitSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * 玩家开局/切地图/读档的唯一核心编排器
	 * @param PC 玩家控制器
	 * @param SpawnLocation 出生/传送的目标位置
	 * @param SpawnRotation 出生/传送的目标朝向
	 */
	UFUNCTION(BlueprintCallable, Category = "EOB|Init")
	void OrchestratePlayerInitialization(AEmpireOfBossPlayerController* PC, FVector SpawnLocation,
	                                     FRotator SpawnRotation);
};
