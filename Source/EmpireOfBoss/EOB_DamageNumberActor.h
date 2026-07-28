#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EOB_DamageNumberActor.generated.h"

class UWidgetComponent;
class UUserWidget;

/**
 * 伤害飘字 Actor：在目标头顶生成一个屏幕空间的数字 Widget，
 * 自动向上漂浮、渐隐、自毁。
 * 使用方式：创建一个继承本类的 BP_DamageNumberActor，
 * 在默认值里指定 NumberWidgetClass 为你的 WBP_DamageNumber，
 * 然后在角色蓝图（英雄/敌人）的 DamageNumberActorClass 槽位里选上它。
 */
UCLASS()
class EMPIREOFBOSS_API AEOB_DamageNumberActor : public AActor
{
	GENERATED_BODY()

public:
	AEOB_DamageNumberActor();

	/** 由属性集在结算伤害后调用 */
	void InitDamage(float Damage, FLinearColor Color);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UWidgetComponent> NumberWidget;

	/** 飘字用的 Widget 类（在 BP 子类默认值里指定 WBP_DamageNumber） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|UI")
	TSubclassOf<UUserWidget> NumberWidgetClass;

	/** 存活秒数 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|UI")
	float LifeTime = 0.9f;

	/** 上浮速度（cm/s） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EOB|UI")
	float RiseSpeed = 120.f;

private:
	/** Widget 创建有延迟，先缓存数值，就绪后再写入 */
	void TryApplyToWidget();

	float ElapsedTime = 0.f;
	bool bHasPending = false;
	float PendingDamage = 0.f;
	FLinearColor PendingColor = FLinearColor::White;
};
