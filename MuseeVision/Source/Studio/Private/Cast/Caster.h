#pragma once

#include "Cast/CastInput.h"
#include "Cast/CastRules.h"
#include "CoreMinimal.h"
#include "Make/MadePackage.h"
#include "UObject/SoftObjectPath.h"

namespace Musee::Studio
{
	/**
	 * How many meshes build at once. The old museum's bake built ~270 room meshes in one batch, which ran every build
	 * on every core at once and peaked at 100 GB; 24 at a time kept the builds parallel on 32 threads at a fraction of
	 * that peak (windows/Source/MuseeVisionEditor/MuseeBakeLibrary.cpp, 2026).
	 */
	inline constexpr int32 MeshesPerBatch = 24;

	/** What casting cost: meshes built, their triangles, the batches, and the process's peak physical memory. */
	struct FCastStats
	{
		int32 PartsBuilt = 0;
		int64 TrianglesBuilt = 0;
		int32 Batches = 0;
		double PeakPhysMemoryMB = 0.0;
	};

	/** One mesh a cast became: its asset and the kind of part it is. */
	struct FCastMesh
	{
		FSoftObjectPath Path;
		ECastKind Kind = ECastKind::Opaque;
	};

	/** The meshes one cast became, opaque part first; a placement of the cast places each of them. Empty if it failed. */
	struct FCastResult
	{
		TArray<FCastMesh> Meshes;
	};

	/**
	 * Casts each input into static mesh assets, one per kind its slots use: opaque parts as Nanite with a distance
	 * field and Lumen cards, translucent parts without. Only parts whose input hash changed are built, in batches of
	 * MeshesPerBatch, and saved; the rest are left on disk untouched. Every part's package is reported in OutMade.
	 * Inputs are consumed.
	 */
	TArray<FCastResult> CastMeshes(TArray<FCastInput>& Inputs, TArray<FMadePackage>& OutMade, FCastStats& OutStats);
}
