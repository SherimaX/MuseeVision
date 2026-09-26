#pragma once

#include "CoreMinimal.h"

namespace Musee::Studio
{
	/**
	 * The derived-data traffic of mesh building since the process started: for each resource, how many times it was
	 * fetched from the DDC (a hit) and how many times it had to be built (a miss). The engine counts these in its cook
	 * statistics, which every editor build keeps.
	 */
	struct FDerivedDataTally
	{
		/** Render data: the Nanite resource, the fallback mesh and its buffers. */
		int64 MeshHits = 0;
		int64 MeshMisses = 0;
		/** Signed distance fields, which Lumen's software tracing and distance-field shadows read. */
		int64 FieldHits = 0;
		int64 FieldMisses = 0;
		/** Lumen mesh cards, the surface cache's capture planes. */
		int64 CardHits = 0;
		int64 CardMisses = 0;

		FDerivedDataTally operator-(const FDerivedDataTally& Earlier) const;
	};

	/** The derived-data traffic of mesh building so far in this process. */
	FDerivedDataTally MeasureDerivedData();

	/** The most physical memory this process has held at any moment, in megabytes (2^20 bytes). */
	double MeasurePeakPhysMemoryMB();
}
