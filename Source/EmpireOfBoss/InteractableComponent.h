#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractableComponent.generated.h"

// 🌟 声明一个通用的交互成功多播委托（参数带上是谁触发了交互，方便后续扩展）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInteractSignature, APlayerController*, InteractingPC);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class EMPIREOFBOSS_API UInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractableComponent();

protected:
	virtual void BeginPlay() override;

	// 🌟 允许在蓝图或实例中自由调整该物体的专属交互距离
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaction")
	float InteractionDistance = 200.f;

	// 🌟 内部点击事件的底层回调
	UFUNCTION()
	void OnOwnerClicked(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed);

public:
	// 🌟 供宿主 Actor（宝箱、NPC等）在外面订阅的公开事件
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnInteractSignature OnInteractSucceeded;

	// 提供一个公开接口，供玩家真正走近后，由 Controller 强行触发交互（寻路到达后的最终判定）
	void TriggerInteraction(APlayerController* PC);
};
