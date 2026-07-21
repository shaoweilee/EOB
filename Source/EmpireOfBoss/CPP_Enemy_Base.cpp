#include "CPP_Enemy_Base.h"

#include "AbilitySystemComponent.h"
#include "EOB_AttributeSet.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EOB_HUDWidget.h"
#include "EmpireOfBossPlayerController.h"

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
}

void ACPP_Enemy_Base::OnPlayerHealthChanged(const struct FOnAttributeChangeData& Data)
{
	UE_LOG(LogTemp, Warning, TEXT("---怪物扣血---"));
	// Data.NewValue 就是扣血/加血后的最新值！
	float CurrentHealth = Data.NewValue;
	float MaxHealth = AttributeSet->GetMaxHealth();
	float HealthPercent = MaxHealth > 0.f ? (CurrentHealth / MaxHealth) : 0.f;

	UE_LOG(LogTemp, Warning, TEXT("-EOBHUDWidgetname--%s---"), *PC->EOBHUDWidget->GetName());
	if (PC->EOBHUDWidget)
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
	if (PC->EOBHUDWidget)
	{
		PC->EOBHUDWidget->BP_UpdateEnemyHP(HealthPercent);
	}
}
