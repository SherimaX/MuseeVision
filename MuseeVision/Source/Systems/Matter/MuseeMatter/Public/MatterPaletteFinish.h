#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"

#include "MatterPaletteFinish.generated.h"

/**
 * One finish a palette entry offers the rooms: its material worked one way and aged one way. Matter's make step makes
 * one material instance for each, `/Game/Generated/Matter/MI_<Palette>_<Working>_<History>`, which a stage's finish
 * schedule names by the same three keys.
 */
USTRUCT()
struct MUSEEMATTER_API FMatterPaletteFinish
{
	GENERATED_BODY()

	/** A working key (Data/Matter/Shading/Workings): honed, trowelled, gilt, laminated.uv-filtering. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Working;

	/** A history regime (the last word of Data/Matter/Shading/Histories/<master>.<regime>): clean, worn, soiled. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey History;

	/** Where the rooms use it, in plain words. */
	UPROPERTY()
	FString Note;
};
