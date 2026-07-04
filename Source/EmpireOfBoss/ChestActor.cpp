#include "ChestActor.h"
#include "InteractableComponent.h"

AChestActor::AChestActor()
{
	// 🌟 修改：允许该 Actor 接收 Tick 每帧刷新（后续会在 BeginPlay 动态优化它）
	PrimaryActorTick.bCanEverTick = true;

	ChestMeshBase = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChestMeshBase"));
	ChestTopSceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ChestTopSceneRoot"));
	ChestMeshTop = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChestMeshTop"));

	RootComponent = ChestMeshBase;
	ChestTopSceneRoot->SetupAttachment(ChestMeshBase);
	ChestMeshTop->SetupAttachment(ChestTopSceneRoot);

	// 🌟 直接创建交互组件
	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("InteractableComponent"));
}

void AChestActor::BeginPlay()
{
	Super::BeginPlay();
	Tags.AddUnique(FName("Chest"));

	// 🌟 性能大招：虽然开启了 bCanEverTick，但游戏一上来先让它的 Tick 彻底沉睡
	// 这样在没开箱之前，成百上千个箱子完全不占一点 CPU 性能！
	SetActorTickEnabled(false);

	// 🌟 只需要向交互组件注册：“一旦你通过了距离和点击判定，就来执行我的开箱逻辑”
	if (InteractableComponent)
	{
		InteractableComponent->OnInteractSucceeded.AddDynamic(this, &AChestActor::OnChestOpened);
	}
}

// 🌟 交互回调：现在只负责改变量和唤醒 Tick
void AChestActor::OnChestOpened(APlayerController* InteractingPC)
{
	if (bIsOpened) return;
	bIsOpened = true;

	// 1. 设置打开后的相对角度（构造顺序为 Pitch, Yaw, Roll。X轴翻转填在最后一个参数）
	// 如果你打算直接旋转盖子，这里写 90.f 即可。如果方向反了改写 -60.f
	TargetLidRotation = FRotator(0.f, 0.f, -60.f);

	// 2. 举起信号旗，并叫醒正在沉睡的 Tick
	bShouldRotate = true;
	SetActorTickEnabled(true);

	UE_LOG(LogTemp, Warning, TEXT("【宝箱通知】接收到开箱信号，开启控制变量，唤醒 Tick！"));
}

// 🌟 真正的动画平滑插值逻辑，在这里通过变量完全控制
void AChestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 🌟 核心开关：只有开箱触发后，这个 if 才会进去
	if (bShouldRotate)
	{
		// 安全检查：由于你有 ChestTopSceneRoot 铰链，建议直接旋转铰链，或者像下面这样旋转 ChestMeshTop 都可以
		if (!ChestMeshTop) return;

		// 1. 获取当前盖子的相对旋转
		FRotator CurrentRot = ChestMeshTop->GetRelativeRotation();

		// 2. 使用 RInterpTo 算出来这一帧应该过渡到的平滑角度
		// 10.f 是旋转速度，数字越大盖子开得越急促，可以按需修改
		FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetLidRotation, DeltaTime, 10.f);

		// 3. 将新角度应用给盖子
		ChestMeshTop->SetRelativeRotation(NewRot);

		// 4. 检查是否已经接近目标值（相差不到 0.5 度即可判定为开完）
		if (CurrentRot.Equals(TargetLidRotation, 0.5f))
		{
			// 强行对齐最终角度，确保绝对精准、不抖动
			ChestMeshTop->SetRelativeRotation(TargetLidRotation);

			// 🌟 功成身退：关闭控制变量，并再次把它的 Tick 关掉，闭环省电！
			bShouldRotate = false;
			SetActorTickEnabled(false);

			UE_LOG(LogTemp, Warning, TEXT("【宝箱通知】盖子掀开完毕，变量关闭，Tick 已重新沉睡。"));
		}
	}
}
