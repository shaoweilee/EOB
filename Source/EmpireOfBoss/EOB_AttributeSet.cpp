#include "EOB_AttributeSet.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Character.h"
#include "EOPBaseCharacter.h"
#include "EmpireOfBossCharacter.h"
#include "EOB_DamageNumberActor.h"

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

			// ===================== M1 新增：伤害飘字 =====================
			// 结算完立刻在挨打者头顶弹数字：白色=怪挨打，红色=玩家挨打
			if (AEOPBaseCharacter* OwnerChar = Cast<AEOPBaseCharacter>(GetOwningActor()))
			{
				if (OwnerChar->DamageNumberActorClass)
				{
					const FVector SpawnLoc = OwnerChar->GetActorLocation() + FVector(0.f, 0.f, 100.f);
					FActorSpawnParameters Params;
					Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

					if (AEOB_DamageNumberActor* NumActor = OwnerChar->GetWorld()->SpawnActor<AEOB_DamageNumberActor>(
						OwnerChar->DamageNumberActorClass, SpawnLoc, FRotator::ZeroRotator, Params))
					{
						const bool bIsPlayer = OwnerChar->IsA(AEmpireOfBossCharacter::StaticClass());
						NumActor->InitDamage(ActualDamage, bIsPlayer ? FLinearColor::Red : FLinearColor::White);
					}
				}
			}

			if (NewHealth <= 0.f)
			{
				// ===================== M1 新增：死亡管线入口 =====================
				// 属性集不自己处理死亡表现，统一呼叫角色基类的 HandleDeath()
				if (AEOPBaseCharacter* OwnerChar = Cast<AEOPBaseCharacter>(GetOwningActor()))
				{
					OwnerChar->HandleDeath();
				}
			}
		}
	}
}
