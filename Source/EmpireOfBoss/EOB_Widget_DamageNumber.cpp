#include "EOB_Widget_DamageNumber.h"
#include "Components/TextBlock.h"

void UEOB_Widget_DamageNumber::SetDamageValue(float Damage, FLinearColor Color, bool bIsCrit)
{
	if (DamageText)
	{
		DamageText->SetText(FText::AsNumber(FMath::RoundToInt(Damage)));
		DamageText->SetColorAndOpacity(FSlateColor(Color));
		// 💥 M3a：暴击数字放大 1.6 倍，一眼看出这刀暴击了
		DamageText->SetRenderScale(bIsCrit ? FVector2D(1.6f, 1.6f) : FVector2D(1.f, 1.f));
	}
}

void UEOB_Widget_DamageNumber::SetCustomText(const FText& Text, FLinearColor Color)
{
	if (DamageText)
	{
		DamageText->SetText(Text);
		DamageText->SetColorAndOpacity(FSlateColor(Color));
	}
}
