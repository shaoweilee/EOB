#include "CPP_Enemy_Base.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

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
	// 放在 BeginPlay 里，游戏跑起来的瞬间会强行洗掉蓝图的一切垃圾缓存
	if (UCapsuleComponent* CapCollision = GetCapsuleComponent())
	{
		CapCollision->SetCollisionProfileName(TEXT("EnemyCapsule"));
		CapCollision->SetGenerateOverlapEvents(true);

		UE_LOG(LogTemp, Log, TEXT("[碰撞强刷]: 成功在 BeginPlay 中强行应用 EnemyCapsule 预设！"));
	}
	OnTakeAnyDamage.AddDynamic(this, &ACPP_Enemy_Base::OnDamageTaken);
}

void ACPP_Enemy_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ACPP_Enemy_Base::OnDamageTaken(AActor* DamagedActor, float Damage, const class UDamageType* DamageType,
                                    class AController* InstigatedBy, AActor* DamageCauser)
{
	UE_LOG(LogTemp, Warning, TEXT("---%f---"), Damage);
}
