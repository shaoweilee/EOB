#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "EOPBaseCharacter.generated.h"

// 前向声明我们具体的属性集类，优化编译速度
class UAbilitySystemComponent;
class UEOB_AttributeSet;
class UGameplayEffect;
class AEOB_DamageNumberActor;

UCLASS()
class EMPIREOFBOSS_API AEOPBaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AEOPBaseCharacter();

	// 实现 IAbilitySystemInterface 必须重写的接口函数
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// 🌟 新增：专用于初始化属性的辅助函数
	void InitializeDefaultAttributes();

	// GAS 能力系统组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EOP|GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	// 🌟 核心修改：明确指定类型为我们写好的 UEOB_AttributeSet
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EOP|GAS")
	TObjectPtr<UEOB_AttributeSet> AttributeSet;
	// ===================== M1 新增：死亡管线 =====================

	/** 是否已死亡（攻击逻辑/AI/UI 统一查询，蓝图可读） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EOP|Combat")
	bool bIsDead = false;

	/** 死亡入口：由属性集在血量归零时自动调用，外部不要直接调 */
	void HandleDeath();

	/** C++ 层死亡钩子：子类重写（敌人在重写里做掉落 + 尸体销毁） */
	virtual void OnDeath();

	/** 蓝图层死亡表现钩子：播死亡蒙太奇 / 布娃娃 / 玩家死亡界面 */
	UFUNCTION(BlueprintImplementableEvent, Category = "EOP|Combat", meta = (DisplayName = "OnDeath"))
	void K2_OnDeath();

	/** 伤害飘字 Actor 类（在英雄/敌人蓝图的默认值里指定 BP_DamageNumberActor） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOP|UI")
	TSubclassOf<AEOB_DamageNumberActor> DamageNumberActorClass;

protected:
	virtual void BeginPlay() override;

	// 🌟 新增：允许在编辑器蓝图中指定的“初始属性配置 GE”
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOP|GAS")
	TSubclassOf<UGameplayEffect> DefaultAttributesGE;
};
