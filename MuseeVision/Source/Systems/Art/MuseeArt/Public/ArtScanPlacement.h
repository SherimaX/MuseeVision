#pragma once

#include "CoreMinimal.h"

#include "ArtScanPlacement.generated.h"

/**
 * How a work made from a scan (a cast in the Classical Hall, the Rotunda's niches, the Salon's Little Dancer) is set:
 * the scan's record (its sources file and id), its mesh, the depth the plan measured it at, the base slice it stands on
 * [w, d] and the turn from the scan's axes to the stage's (three rows), all as the hanging plan writes them.
 *
 * The collection's meshes are already stood up: y up, the front towards +z, as glTF has it (a `.glb` under
 * assets/classical, converted from the print files with SOURCES.json's turn applied; an `.mvm` under assets/sculptures,
 * turned the same way, its CREDITS.md). `turn` records the turn that conversion applied, so the mesh is never turned by
 * it again; the work's own turn in the stage is its entry's `facing_deg`.
 */
USTRUCT()
struct MUSEEART_API FArtScanPlacement
{
	GENERATED_BODY()

	/** The file that records the scan's provenance (assets/classical/SOURCES.json). */
	UPROPERTY()
	FString Source;

	/** The scan's id in that file. */
	UPROPERTY()
	FString Id;

	/** The scan's mesh, from the repository's root. */
	UPROPERTY()
	FString Mesh;

	/** The work's depth as scanned, metres. */
	UPROPERTY(meta = (Units = "Meters"))
	float Depth = 0.0f;

	/** The base slice's width and depth [w, d], metres. */
	UPROPERTY()
	TArray<double> Base;

	/** The rotation from the scan's axes to the stage's, as its three rows. */
	UPROPERTY()
	TArray<FVector> Turn;
};
