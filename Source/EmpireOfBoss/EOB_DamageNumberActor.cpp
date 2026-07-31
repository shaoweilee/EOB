#include "EOB_DamageNumberActor.h"
#include "Components/WidgetComponent.h"
#include "EOB_Widget_DamageNumber.h"

AEOB_DamageNumberActor::AEOB_DamageNumberActor()
{
	PrimaryActorTick.bCanEverTick = true;

	NumberWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("NumberWidget"));
	RootComponent = NumberWidget;

	// 🌟 屏幕空间：永远正对镜头、不会被透视拉伸，暗黑流飘字标准做法
	NumberWidget->SetWidgetSpace(EWidgetSpace::Screen);
	NumberWidget->SetDrawAtDesiredSize(true);
}

void AEOB_DamageNumberActor::BeginPlay()
{
	Super::BeginPlay();

	if (NumberWidgetClass)
	{
		NumberWidget->SetWidgetClass(NumberWidgetClass);
	}

	// 🌟 连击时多个数字会叠在一起，给个随机水平偏移散开
	AddActorLocalOffset(FVector(FMath::RandRange(-40.f, 40.f), FMath::RandRange(-40.f, 40.f), 0.f));

	TryApplyToWidget();
}

void AEOB_DamageNumberActor::InitDamage(float Damage, FLinearColor Color, bool bIsCrit)
{
	PendingDamage = Damage;
	PendingColor = Color;
	bPendingIsCrit = bIsCrit;
	bHasPending = true;
	TryApplyToWidget();
}

void AEOB_DamageNumberActor::InitCustomText(const FText& Text, FLinearColor Color)
{
	PendingCustomText = Text;
	PendingColor = Color;
	bHasPendingCustomText = true;
	TryApplyToWidget();
}

void AEOB_DamageNumberActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Widget 创建完成前持续尝试写入数值
	TryApplyToWidget();

	// 匀速上浮
	AddActorWorldOffset(FVector(0.f, 0.f, RiseSpeed * DeltaTime));

	// 线性渐隐
	ElapsedTime += DeltaTime;
	const float Alpha = FMath::Clamp(1.f - ElapsedTime / LifeTime, 0.f, 1.f);
	if (UUserWidget* W = NumberWidget->GetWidget())
	{
		W->SetRenderOpacity(Alpha);
	}

	if (ElapsedTime >= LifeTime)
	{
		Destroy();
	}
}

void AEOB_DamageNumberActor::TryApplyToWidget()
{
	if (UEOB_Widget_DamageNumber* W = Cast<UEOB_Widget_DamageNumber>(NumberWidget->GetWidget()))
	{
		if (bHasPending)
		{
			W->SetDamageValue(PendingDamage, PendingColor, bPendingIsCrit);
			bHasPending = false;
		}
		if (bHasPendingCustomText)
		{
			W->SetCustomText(PendingCustomText, PendingColor);
			bHasPendingCustomText = false;
		}
	}
}
