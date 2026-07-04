#include "InteractableComponent.h"

#include "EmpireOfBossPlayerController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

UInteractableComponent::UInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// 🌟 降维打击：在构造函数中，直接将宿主默认打上标签
	// 这样美术或你在蓝图编辑器里不需要运行游戏，也能直接看到静态的 Tags 数组里自带这个标签！
	AActor* MyOwner = GetOwner();
	if (MyOwner)
	{
	}
}

void UInteractableComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (Owner)
	{
		Owner->Tags.AddUnique(FName("Interactable"));
		// 🌟 自动化升级：获取宿主 Actor 上的【所有】PrimitiveComponent（包括底座和盖子）
		TArray<UPrimitiveComponent*> PrimitiveComponents;
		Owner->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

		// 🌟 遍历每一个组件，把它们全部绑定上点击事件
		for (UPrimitiveComponent* PrimComp : PrimitiveComponents)
		{
			if (PrimComp)
			{
				// 自动确保每一个部分（底座、盖子、甚至锁扣）都能阻挡鼠标射线
				PrimComp->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

				// 为每一个组件都绑定相同的点击事件
				PrimComp->OnClicked.AddDynamic(this, &UInteractableComponent::OnOwnerClicked);
			}
		}

		// 打印一条日志，方便你调试看它成功绑定了几个组件
		UE_LOG(LogTemp, Log, TEXT("【交互组件】已成功为 %s 的 %d 个碰撞组件绑定了点击事件！"), *Owner->GetName(), PrimitiveComponents.Num());
	}
}

void UInteractableComponent::OnOwnerClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed)
{
	if (ButtonPressed != EKeys::LeftMouseButton) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC || !PC->GetPawn()) return;

	float Distance = FVector::Dist(PC->GetPawn()->GetActorLocation(), GetOwner()->GetActorLocation());

	if (Distance <= InteractionDistance)
	{
		TriggerInteraction(PC);
	}
	else
	{
		AEmpireOfBossPlayerController* EOB_PC = Cast<AEmpireOfBossPlayerController>(PC);
		if (EOB_PC)
		{
			// 🌟 手感升级：计算宝箱底部的精确地表寻路点
			FVector TargetFloorLocation = GetOwner()->GetActorLocation();
			FHitResult FloorHit;
			FVector StartTrace = TargetFloorLocation + FVector(0.f, 0.f, 100.f);
			FVector EndTrace = TargetFloorLocation - FVector(0.f, 0.f, 500.f);

			// 垂直向下走你的地表行走专线
			if (GetWorld()->LineTraceSingleByChannel(FloorHit, StartTrace, EndTrace, ECC_GameTraceChannel2))
			{
				TargetFloorLocation = FloorHit.Location;
			}

			// 同步给 PC 缓存并走过去
			EOB_PC->CachedDestination = TargetFloorLocation;
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(EOB_PC, TargetFloorLocation);
		}

		UE_LOG(LogTemp, Warning, TEXT("[%s] 离你太远了，已成功命令主角精确寻路至其底座地面..."), *GetOwner()->GetName());
	}
}

void UInteractableComponent::TriggerInteraction(APlayerController* PC)
{
	// 广播事件，通知所有订阅了我的宿主（比如宝箱）：可以执行你们自己的逻辑了！
	OnInteractSucceeded.Broadcast(PC);
}
