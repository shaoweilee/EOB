#include "EOB_Widget_DamageNumber.h"
#include "Components/TextBlock.h"

void UEOB_Widget_DamageNumber::SetDamageValue(float Damage, FLinearColor Color)
{
	if (DamageText)
	{
		DamageText->SetText(FText::AsNumber(FMath::RoundToInt(Damage)));
		DamageText->SetColorAndOpacity(FSlateColor(Color));
	}
}
