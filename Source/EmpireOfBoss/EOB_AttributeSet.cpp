#include "EOB_AttributeSet.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Character.h"

UEOB_AttributeSet::UEOB_AttributeSet()
{
}

// 属性改变前拦截（防止当前血量/蓝量超出上限）
void UEOB_AttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMana());
	}
}

// 属性真正改变后触发（处理暗黑式伤害减伤公式）
void UEOB_AttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// 1. 处理直接修改血量（如喝药水）
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}

	// 2. 处理怪物攻击（伤害灌进 IncomingDamage 伤害池）
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float LocalIncomingDamage = GetIncomingDamage();
		SetIncomingDamage(0.f); // 瞬间清空伤害池

		if (LocalIncomingDamage > 0.f)
		{
			// 🛡️ 经典的暗黑式减伤公式：实际伤害 = 原始伤害 * (100 / (100 + 护甲))
			const float DefenseFactor = 100.f / (100.f + GetArmor());
			const float ActualDamage = LocalIncomingDamage * DefenseFactor;

			// 执行扣血
			const float NewHealth = FMath::Clamp(GetHealth() - ActualDamage, 0.f, GetMaxHealth());
			SetHealth(NewHealth);

			UE_LOG(LogTemp, Log, TEXT("[GAS属性集]: 收到原始伤害 %.1f，经护甲(%.1f)减伤后实际扣血 %.1f，剩余生命: %.1f"),
			       LocalIncomingDamage, GetArmor(), ActualDamage, NewHealth);

			if (NewHealth <= 0.f)
			{
				// 后续可以在这里触发死亡逻辑
			}
		}
	}
}
