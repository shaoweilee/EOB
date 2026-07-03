#include "InteractableComponent.h"

#include "EmpireOfBossPlayerController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

UInteractableComponent::UInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInteractableComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* Owner = GetOwner();
	if (Owner)
	{
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

	// 自动计算玩家角色与该物体的距离
	float Distance = FVector::Dist(PC->GetPawn()->GetActorLocation(), GetOwner()->GetActorLocation());

	if (Distance <= InteractionDistance)
	{
		// 🌟 情况 A：距离足够近，直接闭环触发交互
		TriggerInteraction(PC);
	}
	else
	{
		// 🌟 情况 B：太远了，命令控制器的路点寻路系统朝这个物体走过来
		// 这里可以直接调用你的 PlayerController 寻路接口，例如：
		AEmpireOfBossPlayerController* EOB_PC = Cast<AEmpireOfBossPlayerController>(PC);
		if (EOB_PC)
		{
			UAIBlueprintHelperLibrary::SimpleMoveToActor(EOB_PC, GetOwner());
		}

		UE_LOG(LogTemp, Warning, TEXT("[%s] 离你太远了，正在自动寻路走过去..."), *GetOwner()->GetName());
	}
}

void UInteractableComponent::TriggerInteraction(APlayerController* PC)
{
	// 广播事件，通知所有订阅了我的宿主（比如宝箱）：可以执行你们自己的逻辑了！
	OnInteractSucceeded.Broadcast(PC);
}
