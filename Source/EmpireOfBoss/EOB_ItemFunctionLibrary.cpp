#include "EOB_ItemFunctionLibrary.h"

FLinearColor UEOB_ItemFunctionLibrary::GetRarityColor(EEOBRarity Rarity)
{
	switch (Rarity)
	{
	case EEOBRarity::Green: return FLinearColor(0.2f, 0.9f, 0.2f);
	case EEOBRarity::Blue: return FLinearColor(0.25f, 0.5f, 1.f);
	case EEOBRarity::Gold: return FLinearColor(1.f, 0.55f, 0.1f);
	default: return FLinearColor(0.9f, 0.9f, 0.9f);
	}
}

FText UEOB_ItemFunctionLibrary::GetRarityDisplayName(EEOBRarity Rarity)
{
	switch (Rarity)
	{
	case EEOBRarity::Green: return FText::FromString(TEXT("魔法"));
	case EEOBRarity::Blue: return FText::FromString(TEXT("稀有"));
	case EEOBRarity::Gold: return FText::FromString(TEXT("传说"));
	default: return FText::FromString(TEXT("普通"));
	}
}
