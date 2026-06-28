#include "EOPBaseCharacter.h"
#include "AbilitySystemComponent.h"
#include "EOB_AttributeSet.h" // 🌟 引入我们刚才确定好名字的属性集头文件

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
		// 3. 初始化 GAS 宿主信息
		AbilitySystemComponent->InitAbilityActorInfo(this, this);

		// 4. 🌟 核心注入：触发属性默认值初始化
		// 在EOB_GameInitSubsystem.cpp中初始化，InitializeDefaultAttributes();
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
