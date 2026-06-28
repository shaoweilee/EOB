#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "EmpireOfBossGameMode.generated.h"

UCLASS(abstract)
class EMPIREOFBOSS_API AEmpireOfBossGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AEmpireOfBossGameMode();

protected:
	// 新增：重写游戏启动生命周期
	virtual void BeginPlay() override;

private:
	// 新增：单机玩家初始化逻辑
	void InitLocalPlayer();
};
