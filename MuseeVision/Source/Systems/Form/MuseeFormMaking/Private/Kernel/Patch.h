#pragma once

#include "CoreMinimal.h"

namespace Musee::Form
{
	class FMeshBuilder;

	/**
	 * A smooth surface sampled on a grid of stations: a vault's cell, a pendentive, a dome's shell where a lathe will
	 * not do. Point (I, J) is the surface at the I-th U station and the J-th V station.
	 */
	struct FPatchGrid
	{
		int32 CountU = 0;
		int32 CountV = 0;
		TArray<FVector> Points;

		const FVector& At(int32 I, int32 J) const { return Points[I * CountV + J]; }

		/**
		 * Samples Surface(u, v), in metres, at every pair of stations. Surfaces that meet this one take the same
		 * stations along the edge they share (Stations.h).
		 */
		static FPatchGrid Sample(TConstArrayView<double> StationsU, TConstArrayView<double> StationsV,
			TFunctionRef<FVector(double U, double V)> Surface);
	};

	/**
	 * Adds a patch: smooth across the grid, crisp at its edges. Each normal comes from the grid's own differences
	 * (central inside, one-sided at the edges), so it belongs to the surface as sampled, and is turned to face
	 * FacingHint(point). UVs are metres along the grid lines: U is the length travelled along a line of constant J,
	 * V along a line of constant I.
	 */
	void Patch(FMeshBuilder& Mesh, const FPatchGrid& Grid, TFunctionRef<FVector(const FVector&)> FacingHint);

	/** Adds a patch with UVs the caller gives for each grid point. */
	void Patch(FMeshBuilder& Mesh, const FPatchGrid& Grid, TFunctionRef<FVector(const FVector&)> FacingHint,
		TFunctionRef<FVector2D(int32 I, int32 J)> UVAt);
}
