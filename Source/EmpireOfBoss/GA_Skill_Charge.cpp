#include "GA_Skill_Charge.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "TimerManager.h"

void UGA_Skill_Charge::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

	// 1. 面向鼠标 → 路径伤害（窄扇形）
	RotateAvatarToCursor();

	bool bIsCrit = false;
	const float Damage = ComputeSkillDamage(DamageMultiplier, true, bIsCrit);
	ApplyDamageFan(DamageRadius, 25.f, Damage, bIsCrit);

	// 2. 位移：胶囊体扫掠找落脚点（撞墙则停在墙前），然后世界定时器驱动平滑冲刺
	if (ACharacter* C = Cast<ACharacter>(Avatar))
	{
		const FVector Start = C->GetActorLocation();
		const FVector End = Start + C->GetActorForwardVector() * ChargeDistance;

		// ⚠️ 角色原点在"脚底"，胶囊中心在胸口。
		//    直接从 Start 扫 = 半个胶囊插在地里 → 起扫就和地面穿插 → 误判撞墙 → 落点被拉回身后 50cm。
		//    所以扫掠起点/终点都要抬高一个胶囊半高。
		const UCapsuleComponent* Capsule = C->GetCapsuleComponent();
		const float CapHalf = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 90.f;
		const float CapRadius = Capsule ? Capsule->GetScaledCapsuleRadius() : 40.f;
		const FCollisionShape Shape = FCollisionShape::MakeCapsule(CapRadius, CapHalf);
		const FVector SweepStart = Start + FVector(0.f, 0.f, CapHalf);
		const FVector SweepEnd = End + FVector(0.f, 0.f, CapHalf);

		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(C);

		FVector Dest = End;
		if (GetWorld()->SweepSingleByChannel(Hit, SweepStart, SweepEnd, FQuat::Identity, ECC_WorldStatic, Shape,
		                                     Params))
		{
			// Hit.Location 是胸口高度，落点 Z 拉回脚底，并在墙前留一步
			Dest = FVector(Hit.Location.X, Hit.Location.Y, Start.Z) + C->GetActorForwardVector() * -50.f;
		}

		// 🌟 平滑冲刺：世界定时器 + Lambda（按值捕获全部状态），
		//    不再依赖技能实例存活（实例成员定时器在本工程实测不触发，陨石同款问题）。
		C->GetCharacterMovement()->StopMovementImmediately();

		UWorld* World = GetWorld();
		const TWeakObjectPtr<UWorld> WeakWorld = World;
		const TWeakObjectPtr<AActor> WeakAvatar = C;
		const FVector From = Start;
		const FVector To = Dest;
		const float Duration = DashDuration;
		TSharedPtr<float> Elapsed = MakeShareable(new float(0.f));
		TSharedPtr<FTimerHandle> SharedTimer = MakeShareable(new FTimerHandle);

		FTimerDelegate DashDelegate;
		DashDelegate.BindLambda([WeakAvatar, WeakWorld, From, To, Duration, Elapsed, SharedTimer]()
		{
			AActor* Av = WeakAvatar.Get();
			UWorld* W = WeakWorld.Get();
			if (!Av || !W)
			{
				return;
			}

			*Elapsed += 0.016f;
			const float Alpha = FMath::Clamp(*Elapsed / Duration, 0.f, 1.f);
			Av->SetActorLocation(FMath::Lerp(From, To, Alpha), true); // sweep=true：途中撞墙/撞怪即停，不穿模

			if (Alpha >= 1.f)
			{
				W->GetTimerManager().ClearTimer(*SharedTimer);
			}
		});
		World->GetTimerManager().SetTimer(*SharedTimer, DashDelegate, 0.016f, true);
	}

	UE_LOG(LogTemp, Log, TEXT("[技能] 冲锋！伤害 %.1f%s"), Damage, bIsCrit ? TEXT("（暴击！）") : TEXT(""));
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
