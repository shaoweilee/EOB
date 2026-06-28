#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChestActor.generated.h"

class UInteractableComponent; // 引入交互组件

UCLASS()
class EMPIREOFBOSS_API AChestActor : public AActor
{
	GENERATED_BODY()

public:
	AChestActor();

protected:
	virtual void BeginPlay() override;

	// 🌟 新增：重写 Tick 函数声明
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "Components")
	UStaticMeshComponent* ChestMeshBase;

	// 🌟 顺手帮你加上了 UPROPERTY()，方便在蓝图细节面板微调铰链位置
	UPROPERTY(EditAnywhere, Category = "Components")
	USceneComponent* ChestTopSceneRoot;

	UPROPERTY(EditAnywhere, Category = "Components")
	UStaticMeshComponent* ChestMeshTop;

	// 🌟 宝箱只需挂载这个组件即可
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Components")
	UInteractableComponent* InteractableComponent;

	// 🌟 交互成功后的回调函数
	UFUNCTION()
	void OnChestOpened(APlayerController* InteractingPC);

private:
	bool bIsOpened = false;

	// 🌟 新增：控制 Tick 中是否执行旋转的布尔变量
	bool bShouldRotate = false;

	// 🌟 新增：存储盖子完全打开时的目标相对角度
	FRotator TargetLidRotation;
};
