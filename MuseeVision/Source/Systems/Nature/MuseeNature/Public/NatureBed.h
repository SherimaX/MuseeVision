#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"

#include "NatureBed.generated.h"

/**
 * One planting in a stage's planting plan: a species grown in a Bed anchor, so many plants set out at a spacing
 * around the anchor, on the body that carries the bed (if it moves).
 */
USTRUCT()
struct MUSEENATURE_API FNatureBed
{
	GENERATED_BODY()

	/** The Bed anchor: the soil's surface at the planting's centre. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Bed;

	/** The species by its scientific name, as its record in Data/Nature/Species names it (`Nymphaea alba`). */
	UPROPERTY(meta = (MuseeRequired))
	FString Species;

	/** How many plants (clumps, crowns, colonies) the bed holds. */
	UPROPERTY(meta = (MuseeRequired))
	int32 Count = 0;

	/** The planting distance between neighbouring plants. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	float Spacing = 0.0f;

	/** The body that carries the bed, or the none key. */
	UPROPERTY()
	FMuseeKey Body;

	UPROPERTY()
	FString Note;
};
