#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"

#include "MatterFinish.generated.h"

/**
 * One line of a stage's finish schedule: a named surface and the palette material, working and history it is made of
 * (Data/Stages/<stage>/Finishes.json).
 */
USTRUCT()
struct MUSEEMATTER_API FMatterFinish
{
	GENERATED_BODY()

	/** The surface a piece names: coping, lip, canvas.monet-willows-morning. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Surface;

	/** The palette key: stone.travertine-romano. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Material;

	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Working;

	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey History;

	UPROPERTY()
	FString Note;
};
