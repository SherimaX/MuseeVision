#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"
#include "NatureBed.h"
#include "NatureWater.h"

#include "NaturePlanting.generated.h"

/** A stage's planting plan, `Data/Stages/<stage>/Planting.json`: its bodies of water and its beds. */
USTRUCT()
struct MUSEENATURE_API FNaturePlanting
{
	GENERATED_BODY()

	/** The file every stage keeps its planting plan in, inside its schedule folder. */
	static constexpr const TCHAR* FileName = TEXT("Planting.json");

	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Stage;

	UPROPERTY()
	TArray<FNatureWater> Water;

	UPROPERTY()
	TArray<FNatureBed> Beds;
};
