#include "EmpireOfBossGameMode.h"
#include "EmpireOfBossPlayerController.h"
#include "EOB_GameInitSubsystem.h"
#include "GameFramework/PlayerStart.h"

AEmpireOfBossGameMode::AEmpireOfBossGameMode()
{
}

void AEmpireOfBossGameMode::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("---GameMode-BeginPlay---"));
	FTimerHandle DummyHandle;
	GetWorldTimerManager().SetTimer(DummyHandle, this, &AEmpireOfBossGameMode::InitLocalPlayer,
	                                0.1f, false);
}


void AEmpireOfBossGameMode::InitLocalPlayer()
{
	UE_LOG(LogTemp, Warning, TEXT("---GameMode-InitLocalPlayer---"));

	// 直接获取本地0号玩家控制器（单机固定只有一个玩家）
	AEmpireOfBossPlayerController* EOBPC = Cast<AEmpireOfBossPlayerController>(GetWorld()->GetFirstPlayerController());
	if (!EOBPC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] 未找到玩家控制器，初始化跳过"));
		return;
	}

	// 获取世界子系统
	UEOB_GameInitSubsystem* InitSubsystem = GetWorld()->GetSubsystem<UEOB_GameInitSubsystem>();
	if (!InitSubsystem) return;

	// 自动读取关卡里的 PlayerStart 作为出生点，不用硬编码坐标
	FVector SpawnLoc = FVector::ZeroVector;
	FRotator SpawnRot = FRotator::ZeroRotator;
	AActor* PlayerStart = FindPlayerStart(EOBPC);
	if (PlayerStart)
	{
		SpawnLoc = PlayerStart->GetActorLocation();
		SpawnRot = PlayerStart->GetActorRotation();
	}

	// 执行全套初始化
	InitSubsystem->OrchestratePlayerInitialization(EOBPC, SpawnLoc, SpawnRot);
}
