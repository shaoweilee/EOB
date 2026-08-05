// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EOPBaseCharacter.h"
#include "CPP_Enemy_Base.generated.h"

class UDataTable;

UCLASS()
class EMPIREOFBOSS_API ACPP_Enemy_Base : public AEOPBaseCharacter
{
	GENERATED_BODY()

public:
	ACPP_Enemy_Base();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;


	// 🌟 属性改变时的 C++ 回调函数
	void OnPlayerHealthChanged(const struct FOnAttributeChangeData& Data);

	UPROPERTY(BlueprintReadWrite, Category = "EOB|UI")
	class UEOB_HUDWidget* EOBHUDWidget;

	class AEmpireOfBossPlayerController* PC;

	// ===================== M1 新增：掉落与死亡 =====================

	/** 掉落表（DataTable，行结构选 FEOBLootTableRow） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Loot")
	TObjectPtr<UDataTable> LootTable;

	/** 死亡后尸体保留秒数，时间到自动销毁 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Combat")
	float CorpseLifeTime = 3.f;

	/** M3a 新增：击杀经验奖励 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Combat")
	float XPReward = 50.f;

	/** 重写 C++ 死亡钩子：掉落 + 尸体定时销毁 */
	virtual void OnDeath() override;

	/** 按掉落表逐行掷点，在尸体周围散落拾取物 */
	void SpawnLoot();

	// ===================== 敌人等级（测试用沙包） =====================

	/** 敌人等级：1 = 一拳一个的教学怪；调高 = 测试用沙包（生命/护甲按比例放大）。
	 *  在关卡里选中某个敌人实例，细节面板里单独改它自己的等级，互不影响 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Enemy")
	int32 EnemyLevel = 1;

	/** 每高 1 级生命/护甲提升比例（0.5 = 每级 +50%，线性叠加）。
	 *  例：80 级 = 1 + 79 × 0.5 = 40.5 倍生命 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "EOB|Enemy")
	float StatGrowthPerLevel = 0.5f;

	/** 敌人名字 */
	UPROPERTY(EditAnywhere, Category = "EOB|Enemy")
	FText EnemyName = NSLOCTEXT("Enemy", "DefaultEnemyName", "孙狗");

	/** 按 EnemyLevel 放大生命/护甲（攻击力故意不放大，免得高级沙包一拳秒你） */
	void ApplyLevelScaling();
};
