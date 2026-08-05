#include "GA_Skill_FlameStorm.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"
#include "MyGameplayTagsLibrary.h"

void UGA_Skill_FlameStorm::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo* ActorInfo,
                                           const FGameplayAbilityActivationInfo ActivationInfo,
                                           const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 1. 面向鼠标，取光标地面点（已做限距 + 导航网格投影，落在可行走地面上）
	RotateAvatarToCursor();
	FVector TargetPoint;
	if (!GetCursorGroundPoint(TargetPoint))
	{
		TargetPoint = Avatar->GetActorLocation();
	}

	// 2. 总伤害一次性算好（含暴击判定），均摊到每一跳。
	//    BP 里的 DamageMultiplier=2.5 含义不变：整片风暴从头到尾总共打 2.5× 面板伤害。
	bool bIsCrit = false;
	const float TotalDamage = ComputeSkillDamage(DamageMultiplier, false, bIsCrit);
	const int32 TotalTicks = FMath::Max(1, FMath::RoundToInt(Duration / TickInterval)); // 4/0.5 = 8
	const float DamagePerTick = TotalDamage / TotalTicks;

	// 3. 🌟 持续风暴：世界定时器 + Lambda（按值捕获全部状态），
	//    不依赖技能实例存活（实例成员定时器在本工程实测不触发，陨石/冲锋同款问题）。
	UWorld* World = GetWorld();
	const TWeakObjectPtr<AActor> WeakAvatar = Avatar;
	const TWeakObjectPtr<UAbilitySystemComponent> WeakASC = GetAbilitySystemComponentFromActorInfo();
	const TWeakObjectPtr<UWorld> WeakWorld = World;
	const TSubclassOf<UGameplayEffect> DGE = DamageEffectClass;
	const float StormRadius = Radius;
	// ⚠️ TickInterval 是类成员，lambda 不能直接捕获成员（等于捕获 this，编译器禁止），先复制成局部变量
	const float Interval = TickInterval;
	TSharedPtr<int32> TickCount = MakeShareable(new int32(0));
	TSharedPtr<FTimerHandle> SharedTimer = MakeShareable(new FTimerHandle);

	FTimerDelegate TickDelegate;
	TickDelegate.BindLambda([WeakAvatar, WeakASC, WeakWorld, DGE, TargetPoint, StormRadius, DamagePerTick,
			TotalTicks, Interval, TickCount, SharedTimer]()
		{
			AActor* Av = WeakAvatar.Get();
			UAbilitySystemComponent* ASC = WeakASC.Get();
			UWorld* W = WeakWorld.Get();
			if (!Av || !ASC || !W || !DGE) return;

			(*TickCount)++;

			// 🟠 每跳都重画一次线框球，寿命 = 跳间隔，视觉上就是一个持续存在的风暴圈
			DrawDebugSphere(W, TargetPoint + FVector(0.f, 0.f, 50.f), StormRadius, 24, FColor::Orange, false,
			                Interval, 0, 3.f);

			// 每跳独立扫描：怪物走进走出都会正确结算
			int32 HitCount = 0;
			TArray<FOverlapResult> Overlaps;
			const FCollisionShape Shape = FCollisionShape::MakeSphere(StormRadius);
			W->OverlapMultiByChannel(Overlaps, TargetPoint, FQuat::Identity, ECC_Pawn, Shape);

			for (const FOverlapResult& Result : Overlaps)
			{
				AActor* HitActor = Result.GetActor();
				if (!HitActor || HitActor == Av) continue;

				UAbilitySystemComponent* TargetASC =
					UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor);
				if (!TargetASC || TargetASC->HasMatchingGameplayTag(FMyGameplayTags::State_Dead)) continue;

				FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
				Context.AddInstigator(Av, Av);
				FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(DGE, 1.f, Context);
				if (!SpecHandle.IsValid()) continue;

				SpecHandle.Data.Get()->SetSetByCallerMagnitude(FMyGameplayTags::Data_Damage, DamagePerTick);
				SpecHandle.Data.Get()->SetSetByCallerMagnitude(FMyGameplayTags::Data_IsCrit, 0.f);
				ASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

				HitCount++;
			}

			UE_LOG(LogTemp, Log, TEXT("[技能] 烈焰风暴第 %d/%d 跳：伤害 %.1f，命中 %d 个目标"), *TickCount,
			       TotalTicks, DamagePerTick, HitCount);

			if (*TickCount >= TotalTicks)
			{
				W->GetTimerManager().ClearTimer(*SharedTimer);
				UE_LOG(LogTemp, Log, TEXT("[技能] 烈焰风暴熄灭。"));
			}
		});
	// 最后一个参数 0.f = 第一跳立即触发，不用干等 0.5 秒
	World->GetTimerManager().SetTimer(*SharedTimer, TickDelegate, Interval, true, 0.f);

	UE_LOG(LogTemp, Log, TEXT("[技能] 烈焰风暴召唤中……落点 %s，持续 %.1f 秒，共 %d 跳，总伤害 %.1f"),
	       *TargetPoint.ToString(), Duration, TotalTicks, TotalDamage);
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
