#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EOB_ItemTypes.h"
#include "EOB_ItemFunctionLibrary.generated.h"

UCLASS()
class EMPIREOFBOSS_API UEOB_ItemFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** TL2 品质配色：白/绿/蓝/金 */
	UFUNCTION(BlueprintPure, Category = "EOB|Item")
	static FLinearColor GetRarityColor(EEOBRarity Rarity);

	UFUNCTION(BlueprintPure, Category = "EOB|Item")
	static FText GetRarityDisplayName(EEOBRarity Rarity);
};
