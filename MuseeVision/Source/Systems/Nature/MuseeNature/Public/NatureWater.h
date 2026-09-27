#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"
#include "NatureOutline.h"

#include "NatureWater.generated.h"

/**
 * A body of water a stage holds, as its planting plan (Planting.json) gives it: the Water anchor at the level of its
 * surface, the body that carries it (a basin that lifts), its plan outline, its depth, the freeboard between the
 * surface and the rim that holds it, and the palette key of its water (`water.still`).
 */
USTRUCT()
struct MUSEENATURE_API FNatureWater
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Key;

	/** The Water anchor: the water's level at its centre. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Anchor;

	/** The body that carries the water, or the none key for water that stands still. */
	UPROPERTY()
	FMuseeKey Body;

	UPROPERTY(meta = (MuseeRequired))
	FNatureOutline Outline;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	float Depth = 0.0f;

	/** How far below the rim the surface stands; zero for brimming water. */
	UPROPERTY(meta = (Units = "Meters"))
	float Freeboard = 0.0f;

	/** The palette key of the water. */
	UPROPERTY(meta = (MuseeRequired))
	FName Material;

	UPROPERTY()
	FString Note;
};
