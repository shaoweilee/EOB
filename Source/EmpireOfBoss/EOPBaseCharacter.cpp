#include "EOPBaseCharacter.h"
#include "AbilitySystemComponent.h"
#include "EOB_AttributeSet.h" // 🌟 引入我们刚才确定好名字的属性集头文件
#include "MyGameplayTagsLibrary.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AEOPBaseCharacter::AEOPBaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	// 1. 创建 ASC 组件
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// 2. 🌟 解冻：正式实例化我们的 EOB 属性集
	AttributeSet = CreateDefaultSubobject<UEOB_AttributeSet>(TEXT("AttributeSet"));
}

void AEOPBaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		// 防御：旧蓝图资产导致 AttributeSet 丢失时自动重建
		if (!AttributeSet)
		{
			AttributeSet = NewObject<UEOB_AttributeSet>(this, TEXT("AttributeSet"));
			UE_LOG(LogTemp, Warning, TEXT("[GAS 角色基类]: %s 的 AttributeSet 丢失，已防御性重建！"), *GetName());
		}
		// 3. 初始化 GAS 宿主信息
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

// 🌟 实现数据驱动属性初始化
void AEOPBaseCharacter::InitializeDefaultAttributes()
{
	if (AbilitySystemComponent && DefaultAttributesGE)
	{
		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		EffectContext.AddSourceObject(this);

		// 默认生成 1 级角色的属性 Spec
		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
			DefaultAttributesGE, 1.f, EffectContext);
		if (SpecHandle.IsValid())
		{
			// 将初始属性一瞬间拍在自己身上
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			UE_LOG(LogTemp, Log, TEXT("[GAS 角色基类]: 成功应用初始属性 GE！"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[GAS 角色基类]: 未在蓝图中指定 DefaultAttributesGE，角色当前初始属性为 0！"));
	}
}

UAbilitySystemComponent* AEOPBaseCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// ===================== M1 新增：死亡管线 =====================

void AEOPBaseCharacter::HandleDeath()
{
	// 兜底：伤害结算可能同一帧灌进来多次，只死一次
	if (bIsDead) return;
	bIsDead = true;

	// 1. 打死亡标签 + 打断所有正在释放的技能
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(FMyGameplayTags::State_Dead);
		AbilitySystemComponent->CancelAllAbilities();
	}

	// 2. 停止移动与当前动画蒙太奇
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}
	StopAnimMontage();

	// 3. 胶囊体关闭碰撞：尸体不挡路、鼠标射线也扫不到
	//    （敌人血条悬停、点击攻击都会因此自动忽略尸体）
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 4. 分发钩子：先 C++ 子类逻辑（敌人掉落），再蓝图表现（死亡动画/UI）
	OnDeath();
	K2_OnDeath();

	UE_LOG(LogTemp, Warning, TEXT("[死亡管线] %s 已死亡。"), *GetName());
}

void AEOPBaseCharacter::OnDeath()
{
	// 基类默认无额外逻辑，敌人子类（CPP_Enemy_Base）重写：掉落 + 尸体销毁
}
