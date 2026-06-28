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

UCLASS()
class EMPIREOFBOSS_API AEOPBaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AEOPBaseCharacter();

	// 实现 IAbilitySystemInterface 必须重写的接口函数
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// 🌟 核心修改：让外界直接能拿到我们具体的 EOB 属性集
	UEOB_AttributeSet* GetEOBAttributeSet() const { return AttributeSet; }
	// 🌟 新增：专用于初始化属性的辅助函数
	void InitializeDefaultAttributes();

protected:
	virtual void BeginPlay() override;

	// GAS 能力系统组件
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EOP|GAS", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	// 🌟 核心修改：明确指定类型为我们写好的 UEOB_AttributeSet
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EOP|GAS")
	TObjectPtr<UEOB_AttributeSet> AttributeSet;

	// 🌟 新增：允许在编辑器蓝图中指定的“初始属性配置 GE”
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOP|GAS")
	TSubclassOf<UGameplayEffect> DefaultAttributesGE;
};
