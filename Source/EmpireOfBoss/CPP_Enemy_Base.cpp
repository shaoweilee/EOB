#include "CPP_Enemy_Base.h"

#include "AbilitySystemComponent.h"
#include "EOB_AttributeSet.h"
#include "EOB_EnemyAIController.h"
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
		MoveComp->MaxWalkSpeed = 300.f; // 旧默认行走速度（BeginPlay 里会被 ChaseSpeed 覆盖）

		// 🌟 M6 重做：面朝移动方向，寻路移动时自动转身
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.f, 720.f, 0.f);
	}

	// 3. 为敌人打上 Tag，完美兼容你的 PlayerController 普攻左键点击过滤！
	Tags.Add(FName("Enemy"));

	// 5. M6 重做：AI 控制器——引擎自动给每只怪生成并附身。
	//    PlacedInWorldOrSpawned = 手摆的和运行时刷出来的行为完全一致，从根上杜绝两种来源不一致
	AIControllerClass = AEOB_EnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void ACPP_Enemy_Base::BeginPlay()
{
	Super::BeginPlay();
	InitializeDefaultAttributes();

	// 🌟 按实例上配置的 EnemyLevel 放大生命/护甲（1 级怪不变，高级怪变沙包）
	ApplyLevelScaling();

	// 🌟 M6 重做：追击速度写进移动组件（覆盖构造函数里的 300 默认值）
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = ChaseSpeed;
	}

	// 放在 BeginPlay 里，游戏跑起来的瞬间会强行洗掉蓝图的一切垃圾缓存
	if (UCapsuleComponent* CapCollision = GetCapsuleComponent())
	{
		CapCollision->SetCollisionProfileName(TEXT("EnemyCapsule"));
		CapCollision->SetGenerateOverlapEvents(true);

		PC = Cast<AEmpireOfBossPlayerController>(GetWorld()->GetFirstPlayerController());


		// GetWorldTimerManager().SetTimerForNextTick(this, &ACPP_Enemy_Base::InitHealthPercent);

		UE_LOG(LogTemp, Log, TEXT("[碰撞强刷]: 成功在 BeginPlay 中强行应用 EnemyCapsule 预设！"));
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).AddUObject(
			this, &ACPP_Enemy_Base::OnPlayerHealthChanged);
	}
}

void ACPP_Enemy_Base::ApplyLevelScaling()
{
	if (EnemyLevel <= 1 || !AbilitySystemComponent || !AttributeSet) return;

	const float Mult = 1.f + (EnemyLevel - 1) * StatGrowthPerLevel;

	// 生命上限和当前生命一起放大（Override 直接改基础值，等效于升级成长）
	const float NewMaxHealth = AttributeSet->GetMaxHealth() * Mult;
	AbilitySystemComponent->ApplyModToAttribute(
		UEOB_AttributeSet::GetMaxHealthAttribute(), EGameplayModOp::Override, NewMaxHealth);
	AbilitySystemComponent->ApplyModToAttribute(
		UEOB_AttributeSet::GetHealthAttribute(), EGameplayModOp::Override, NewMaxHealth);

	// 护甲也放大，技能打上去数字更小（基础护甲为 0 时乘了也是 0，无副作用）
	AbilitySystemComponent->ApplyModToAttribute(
		UEOB_AttributeSet::GetArmorAttribute(), EGameplayModOp::Override,
		AttributeSet->GetArmor() * Mult);

	// ⚠️ 攻击力故意不放大：80 级沙包是用来挨打的，不是用来一拳秒你的。
	//    哪天想测试"高压生存"了，取消下面这三行的注释：
	// AbilitySystemComponent->ApplyModToAttribute(
	//     UEOB_AttributeSet::GetAttackPowerAttribute(), EGameplayModOp::Override,
	//     AttributeSet->GetAttackPower() * Mult);

	UE_LOG(LogTemp, Warning, TEXT("[敌人] %s 等级 %d：生命 %.0f，护甲 %.1f（放大 %.1f 倍）"),
	       *GetName(), EnemyLevel, NewMaxHealth, AttributeSet->GetArmor(), Mult);
}

void ACPP_Enemy_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// M1：尸体不执行任何 Tick 逻辑
	// M6：追击大脑在 EOB_EnemyAIController 里，这里不用管
	if (bIsDead) return;
}

void ACPP_Enemy_Base::OnPlayerHealthChanged(const struct FOnAttributeChangeData& Data)
{
	float CurrentHealth = Data.NewValue;
	float MaxHealth = AttributeSet->GetMaxHealth();
	float HealthPercent = MaxHealth > 0.f ? (CurrentHealth / MaxHealth) : 0.f;

	// 🌟 核心修复：共享血条同一时刻只属于"鼠标正悬停的那个敌人"
	// 不是我，就别往上面写——否则打 A 指着 B 时血条必串
	if (PC && PC->EOBHUDWidget && PC->LastHoveredEnemy.Get() == this)
	{
		PC->EOBHUDWidget->BP_UpdateEnemyHP(HealthPercent);


		FText HPText = FText::FormatNamed(
			FText::FromString(TEXT("{CurrentHealth} / {MaxHealth}")),
			TEXT("CurrentHealth"), FText::AsNumber(CurrentHealth), // int32 可以直接传
			TEXT("MaxHealth"), FText::AsNumber(MaxHealth)
		);
		PC->EOBHUDWidget->BP_UpdateEnemyText(EnemyName, HPText);
	}
}

// ===================== M1 新增：死亡钩子（掉落 + 尸体销毁） =====================

void ACPP_Enemy_Base::OnDeath()
{
	// 1. 按掉落表掷点，爆出金币/药水/装备
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
			if (AEOB_PickupBase* Pickup = GetWorld()->SpawnActor<AEOB_PickupBase>(
				Row->PickupClass, SpawnLoc, FRotator::ZeroRotator, Params))
			{
				// 🌟 掉落行指定了装备定义时，覆盖拾取物类上的默认值：
				//    这样所有装备共用一个拾取物类，掉落行自己决定掉什么
				if (Row->DroppedItemDefinition)
				{
					Pickup->SetDroppedItemDefinition(Row->DroppedItemDefinition);
				}
			}
		}
	}
}
