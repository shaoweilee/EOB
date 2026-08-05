#include "GA_Skill_Charge.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "TimerManager.h"
#include "MyGameplayTagsLibrary.h"

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
	ACharacter* C = Cast<ACharacter>(Avatar);
	if (!C)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	// 1. 面向鼠标
	RotateAvatarToCursor();

	// 伤害和暴击在施放瞬间一次性算好，路径上所有命中共用这一发伤害
	bool bIsCrit = false;
	const float Damage = ComputeSkillDamage(DamageMultiplier, true, bIsCrit);

	// 2. 目标点 = 光标位置，距离钳到 ChargeDistance 以内
	const FVector Start = C->GetActorLocation();
	FVector DashDir = C->GetActorForwardVector();
	float DashDist = ChargeDistance;
	{
		FVector AimPoint;
		if (GetCursorGroundPoint(AimPoint))
		{
			FVector ToTarget = AimPoint - Start;
			ToTarget.Z = 0.f;
			if (!ToTarget.IsNearlyZero())
			{
				DashDir = ToTarget.GetSafeNormal();
				DashDist = FMath::Min(ToTarget.Size(), ChargeDistance);
			}
		}
	}

	// 3. 🌟 冲刺 = 直接给移动组件一个朝目标的高速，胶囊碰撞（撞墙停、撞怪停）全部交给引擎。
	//    定时器只负责：在主角真实位置扫伤害 + 监控"到位/被卡住"就刹车。
	UCharacterMovementComponent* MoveComp = C->GetCharacterMovement();
	MoveComp->StopMovementImmediately();
	MoveComp->SetMovementMode(MOVE_Walking);

	const float Duration = FMath::Max(DashDuration, 0.05f);
	const float DashSpeed = ChargeDistance / Duration; // 600 / 0.15 = 4000 cm/s

	const UCapsuleComponent* Capsule = C->GetCapsuleComponent();
	const float CapHalf = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : 90.f;
	const float CapRadius = Capsule ? Capsule->GetScaledCapsuleRadius() : 40.f;

	UWorld* World = GetWorld();
	const TWeakObjectPtr<UWorld> WeakWorld = World;
	const TWeakObjectPtr<ACharacter> WeakChar = C;
	const TWeakObjectPtr<UAbilitySystemComponent> WeakASC = GetAbilitySystemComponentFromActorInfo();
	const float HitMargin = PathHitRadius;
	const TSubclassOf<UGameplayEffect> DGE = DamageEffectClass;
	TSharedPtr<float> Elapsed = MakeShareable(new float(0.f));
	TSharedPtr<FVector> PrevLoc = MakeShareable(new FVector(Start));
	TSharedPtr<int32> BlockedFrames = MakeShareable(new int32(0));
	TSharedPtr<FTimerHandle> SharedTimer = MakeShareable(new FTimerHandle);
	// 🌟 已命中名单：每个敌人整场冲锋只结算一次伤害
	TSharedPtr<TSet<TWeakObjectPtr<AActor>>> AlreadyHit = MakeShareable(new TSet<TWeakObjectPtr<AActor>>());
	TSharedPtr<int32> HitCount = MakeShareable(new int32(0));

	FTimerDelegate DashDelegate;
	DashDelegate.BindLambda([WeakChar, WeakASC, WeakWorld, Start, DashDir, DashDist, DashSpeed, HitMargin,
			CapRadius, CapHalf, Damage, bIsCrit, DGE, Elapsed, PrevLoc, BlockedFrames,
			SharedTimer, AlreadyHit, HitCount]()
		{
			ACharacter* Chr = WeakChar.Get();
			UWorld* W = WeakWorld.Get();
			if (!Chr || !W)
			{
				return;
			}
			UCharacterMovementComponent* Move = Chr->GetCharacterMovement();
			if (!Move)
			{
				W->GetTimerManager().ClearTimer(*SharedTimer);
				return;
			}

			*Elapsed += 0.016f;

			// 持续给速度：引擎带着胶囊走，撞上墙/怪自然停下，绝不穿模
			Move->Velocity = DashDir * DashSpeed;

			// ⚠️ 一切判定都用主角的真实位置（上一版用理想插值位置，人被挡住后球还在飞，就是那么来的）
			const FVector ActualLoc = Chr->GetActorLocation();
			const float Traveled = FVector::Dist2D(ActualLoc, Start);
			const float MovedSinceLast = FVector::Dist2D(ActualLoc, *PrevLoc);
			*PrevLoc = ActualLoc;

			// 🟠 路径伤害：以"主角胶囊 + 外扩余量"在实际位置扫描，伤害时机 = 两个胶囊贴上
			UAbilitySystemComponent* SourceASC = WeakASC.Get();
			if (SourceASC && DGE)
			{
				const FVector ScanCenter = ActualLoc + FVector(0.f, 0.f, CapHalf);
				DrawDebugCapsule(W, ScanCenter, CapHalf, CapRadius + HitMargin, FQuat::Identity, FColor::Orange,
				                 false, 0.15f, 0, 2.f);

				TArray<FOverlapResult> Overlaps;
				const FCollisionShape HitShape = FCollisionShape::MakeCapsule(CapRadius + HitMargin, CapHalf);
				W->OverlapMultiByChannel(Overlaps, ScanCenter, FQuat::Identity, ECC_Pawn, HitShape);

				for (const FOverlapResult& Result : Overlaps)
				{
					AActor* HitActor = Result.GetActor();
					if (!HitActor || HitActor == Chr) continue;
					if (AlreadyHit->Contains(HitActor)) continue;

					UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(
						HitActor);
					if (!TargetASC || TargetASC->HasMatchingGameplayTag(FMyGameplayTags::State_Dead)) continue;

					FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
					Context.AddInstigator(Chr, Chr);
					FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DGE, 1.f, Context);
					if (!SpecHandle.IsValid()) continue;

					SpecHandle.Data.Get()->SetSetByCallerMagnitude(FMyGameplayTags::Data_Damage, Damage);
					SpecHandle.Data.Get()->SetSetByCallerMagnitude(FMyGameplayTags::Data_IsCrit, bIsCrit ? 1.f : 0.f);
					SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

					AlreadyHit->Add(HitActor);
					(*HitCount)++;
					UE_LOG(LogTemp, Log, TEXT("[技能] 冲锋命中【%s】伤害 %.1f%s"), *HitActor->GetName(), Damage,
					       bIsCrit ? TEXT("（暴击！）") : TEXT(""));
				}
			}

			// 结束条件：冲够距离 / 连续 3 帧几乎没动（被墙或怪卡住）/ 超过 2 秒兜底（防止贴着怪滑停不下来）
			if (MovedSinceLast < 5.f)
			{
				(*BlockedFrames)++;
			}
			else
			{
				*BlockedFrames = 0;
			}

			if (Traveled >= DashDist - 10.f || *BlockedFrames >= 3 || *Elapsed >= 2.f)
			{
				Move->StopMovementImmediately();
				W->GetTimerManager().ClearTimer(*SharedTimer);
				const int32 FinalHitCount = *HitCount; // ⚠️ 先解引用再传 %d，直接传 TSharedPtr 会打印内存地址
				UE_LOG(LogTemp, Log, TEXT("[技能] 冲锋结束（冲出 %.0f 厘米），共命中 %d 个目标。"), Traveled,
				       FinalHitCount);
			}
		});
	World->GetTimerManager().SetTimer(*SharedTimer, DashDelegate, 0.016f, true);

	UE_LOG(LogTemp, Log, TEXT("[技能] 冲锋！伤害 %.1f%s（目标距离 %.0f 厘米）"), Damage,
	       bIsCrit ? TEXT("（暴击！）") : TEXT(""), DashDist);
	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
}
