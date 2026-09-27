#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"

#include "ArtHangEntry.generated.h"

/**
 * One work in a stage's hanging plan: which work (its id in the catalogue that owns it, named by `source`, a path from
 * the repository's root), the Wall anchor it hangs on, where (`along` the wall to its start edge, its lower edge
 * `height` above the floor), its real size, its support and frame, and its care class (a key of Data/Art/Care.json).
 */
USTRUCT()
struct MUSEEART_API FArtHangEntry
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	FString Work;

	UPROPERTY(meta = (MuseeRequired))
	FString Source;

	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Anchor;

	/** The distance along the wall (its arc length on a curved wall) from the anchor to the work's start edge. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	float Along = 0.0f;

	/** The work's lower edge above the floor, which the Wall anchor's origin stands on. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	float Height = 0.0f;

	/** The work's width and height [w, h] in metres, its real size. */
	UPROPERTY(meta = (MuseeRequired))
	TArray<double> Size;

	/** `canvas`. */
	UPROPERTY(meta = (MuseeRequired))
	FName Support;

	/** `none`. */
	UPROPERTY()
	FName Frame;

	UPROPERTY(meta = (MuseeRequired))
	FName Care;

	UPROPERTY()
	FString Note;
};
