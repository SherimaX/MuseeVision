#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"
#include "NatureCast.h"

#include "NatureBed.generated.h"

/**
 * One planting in a stage's planting plan: a species grown in a Bed anchor, so many plants set out at a spacing
 * around the anchor, on the body that carries the bed (if it moves). Each organ is made of its own living tissue (the
 * species record's palette key for it: a lily's pad `living.leaf-floating`, its petals `living.petal`), worked and aged
 * as the entry says. A cast specimen is a single plant grown to its species' form and cast in metal, every organ in the
 * cast's material, working and history (the golden iris, gilt bronze cast from Iris laevigata).
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

	/** The planting distance between neighbouring plants; a planting of one plant stands at its anchor and needs none. */
	UPROPERTY(meta = (Units = "Meters"))
	float Spacing = 0.0f;

	/** The working and history the plants' tissues are resolved with (Data/Matter/Shading); none for a cast specimen. */
	UPROPERTY()
	FName Working;

	UPROPERTY()
	FName History;

	/** For a cast specimen, what it is cast in; every part of the plant takes it. */
	UPROPERTY()
	FNatureCast Cast;

	/** The body that carries the bed, or the none key. */
	UPROPERTY()
	FMuseeKey Body;

	UPROPERTY()
	FString Note;
};
