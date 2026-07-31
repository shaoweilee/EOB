#include "CPP_Enemy_Base.h"

#include "AbilitySystemComponent.h"
#include "EOB_AttributeSet.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EOB_HUDWidget.h"
#include "EmpireOfBossPlayerController.h"
#include "EOB_LootTableRow.h"
#include "EOB_PickupBase.h"
#include "Engine/DataTable.h"
#include "EmpireOfBossCharacter.h"
#include "EOB_LevelComponent.h"

ACPP_Enemy_Base::ACPP_Enemy_Base()
{
	PrimaryActorTick.bCanEverTick = true;
	// 2. 自动继承了角色移动组件，在此处直接开启 RVO 避让算法
	// 这样怪物平时撞到一起时就会根据半径自然滑开、围堵玩家，绝对不会重叠变成一个点！
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bUseRVOAvoidance = true;
		MoveComp->AvoidanceConsiderationRadius = 40.f; // 避让判定半径，与怪物的身形大小类似即可
		MoveComp->MaxWalkSpeed = 300.f; // 顺便初始化怪物的慢速游荡/行走速度
	}

	// 3. 为敌人打上 Tag，完美兼容你的 PlayerController 普攻左键点击过滤！
	Tags.Add(FName("Enemy"));
}

void ACPP_Enemy_Base::BeginPlay()
{
	Super::BeginPlay();
	InitializeDefaultAttributes();
	// 放在 BeginPlay 里，游戏跑起来的瞬间会强行洗掉蓝图的一切垃圾缓存
	if (UCapsuleComponent* CapCollision = GetCapsuleComponent())
	{
		CapCollision->SetCollisionProfileName(TEXT("EnemyCapsule"));
		CapCollision->SetGenerateOverlapEvents(true);

		PC = Cast<AEmpireOfBossPlayerController>(GetWorld()->GetFirstPlayerController());


		// FTimerHandle DummyHandle;
		// GetWorldTimerManager().SetTimer(DummyHandle, this, &ACPP_Enemy_Base::InitHealthPercent,
		//                                 0.11f, false);
		GetWorldTimerManager().SetTimerForNextTick(this, &ACPP_Enemy_Base::InitHealthPercent);

		UE_LOG(LogTemp, Log, TEXT("[碰撞强刷]: 成功在 BeginPlay 中强行应用 EnemyCapsule 预设！"));
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(
			this, &ACPP_Enemy_Base::OnPlayerHealthChanged);
	}
}

void ACPP_Enemy_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// M1 新增：尸体不再执行任何 Tick 逻辑（未来的 AI 追敌也不会被尸体触发）
	if (bIsDead) return;
}

void ACPP_Enemy_Base::OnPlayerHealthChanged(const struct FOnAttributeChangeData& Data)
{
	UE_LOG(LogTemp, Warning, TEXT("---怪物扣血---"));
	// Data.NewValue 就是扣血/加血后的最新值！
	float CurrentHealth = Data.NewValue;
	float MaxHealth = AttributeSet->GetMaxHealth();
	float HealthPercent = MaxHealth > 0.f ? (CurrentHealth / MaxHealth) : 0.f;

	if (PC && PC->EOBHUDWidget)
	{
		UE_LOG(LogTemp, Log, TEXT("[UI 联动管道]: 检测到怪物血量发生改变！当前最新血量为: %.1f"), CurrentHealth);
		PC->EOBHUDWidget->BP_UpdateEnemyHP(HealthPercent);
	}
}

void ACPP_Enemy_Base::InitHealthPercent()
{
	float CurrentHealth = AttributeSet->GetHealth();
	float MaxHealth = AttributeSet->GetMaxHealth();
	UE_LOG(LogTemp, Warning, TEXT("---CurrentHealth: %f, MaxHealth: %f---"), CurrentHealth, MaxHealth);
	float HealthPercent = MaxHealth > 0.f ? (CurrentHealth / MaxHealth) : 0.f;
	if (PC && PC->EOBHUDWidget)
	{
		PC->EOBHUDWidget->BP_UpdateEnemyHP(HealthPercent);
	}
}

// ===================== M1 新增：死亡钩子（掉落 + 尸体销毁） =====================

void ACPP_Enemy_Base::OnDeath()
{
	// 1. 按掉落表掷点，爆出金币/药水
	SpawnLoot();
	// 3. M3a 新增：给英雄发击杀经验
	if (UWorld* World = GetWorld())
	{
		APlayerController* PlayerCtlr = World->GetFirstPlayerController();
		if (AEmpireOfBossCharacter* Hero = PlayerCtlr
			                                   ? Cast<AEmpireOfBossCharacter>(PlayerCtlr->GetCharacter())
			                                   : nullptr)
		{
			if (Hero->LevelComponent)
			{
				Hero->LevelComponent->AddExperience(XPReward);
			}
		}
	}
	// 2. 尸体定时销毁（蓝图 K2_OnDeath 里播的死亡动画要控制在此时长内）
	SetLifeSpan(CorpseLifeTime);
}

void ACPP_Enemy_Base::SpawnLoot()
{
	if (!LootTable) return;

	static const FString ContextString(TEXT("EnemyLootRoll"));
	TArray<FEOBLootTableRow*> Rows;
	LootTable->GetAllRows<FEOBLootTableRow>(ContextString, Rows);

	for (const FEOBLootTableRow* Row : Rows)
	{
		if (!Row || !Row->PickupClass) continue;

		// 独立概率判定
		if (FMath::FRand() > Row->DropChance) continue;

		const int32 Count = FMath::RandRange(Row->MinCount, FMath::Max(Row->MinCount, Row->MaxCount));
		for (int32 i = 0; i < Count; ++i)
		{
			// 尸体周围 80cm 内随机散落，火炬之光式的爆一地
			FVector SpawnLoc = GetActorLocation() + FVector(
				FMath::RandRange(-80.f, 80.f), FMath::RandRange(-80.f, 80.f), 0.f);

			// 垂直射线贴地，防止掉在半空或插进地板
			FHitResult FloorHit;
			if (GetWorld()->LineTraceSingleByChannel(FloorHit,
			                                         SpawnLoc + FVector(0.f, 0.f, 100.f),
			                                         SpawnLoc - FVector(0.f, 0.f, 300.f),
			                                         ECC_GameTraceChannel2))
			{
				SpawnLoc = FloorHit.Location + FVector(0.f, 0.f, 2.f);
			}

			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			GetWorld()->SpawnActor<AEOB_PickupBase>(Row->PickupClass, SpawnLoc, FRotator::ZeroRotator, Params);
		}
	}
}
