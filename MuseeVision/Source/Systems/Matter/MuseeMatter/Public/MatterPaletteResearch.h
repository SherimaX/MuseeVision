#pragma once

#include "CoreMinimal.h"

#include "MatterPaletteResearch.generated.h"

/** What research a palette entry rests on: the records that describe its material, and what is still to be found. */
USTRUCT()
struct MUSEEMATTER_API FMatterPaletteResearch
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FString> Records;

	UPROPERTY()
	FString Needed;
};
