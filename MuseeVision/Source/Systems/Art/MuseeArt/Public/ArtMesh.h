#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"

#include "ArtMesh.generated.h"

/** A run of parallel wires, as far apart as their pitch: the first's and the last's centres, and the pitch, metres. */
USTRUCT()
struct MUSEEART_API FArtWireRun
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double From = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double To = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double Pitch = 0.0;
};

/**
 * A wire mesh works hang on (a store's rack, `Data/Stages/reserve/Hang.json`), as the stage builds it behind a Wall
 * anchor: its level wires (their heights above the anchor's floor line) and its upright wires (their places along the
 * anchor's x), the level wires' centres `behind` the anchor's plane, and the wires' diameter. A framed work on the
 * anchor hangs from a level wire by a pair of S-hooks through the screw eyes in its frame's back, bent from the wire
 * Data/Art/Hooks.json gives and finished as the `hooks` surface of the stage's Finishes.json; each hook passes the
 * uprights halfway between two.
 */
USTRUCT()
struct MUSEEART_API FArtMesh
{
	GENERATED_BODY()

	/** The Wall anchor whose plane the works' backs stand on, in front of the mesh. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Anchor;

	/** The surface in the stage's Finishes.json of the works' S-hooks and their frames' screw eyes. */
	UPROPERTY(meta = (MuseeRequired))
	FName Hooks;

	/** The level wires the hooks hang over: their centres' heights above the anchor's floor line. */
	UPROPERTY(meta = (MuseeRequired))
	FArtWireRun Levels;

	/** The upright wires: their centres along the anchor's x. */
	UPROPERTY(meta = (MuseeRequired))
	FArtWireRun Uprights;

	/** How far behind the anchor's plane the level wires' centres lie. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double Behind = 0.0;

	/** The wires' diameter. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double Wire = 0.0;

	UPROPERTY()
	FString Note;
};
