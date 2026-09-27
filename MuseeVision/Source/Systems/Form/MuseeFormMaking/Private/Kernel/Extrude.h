#pragma once

#include "CoreMinimal.h"

namespace Musee::Form
{
	class FMeshBuilder;
	struct FPlaneFrame;
	struct FProfileStations;

	/**
	 * Extrudes a closed outline with holes straight out of its plane: a slab round its stair well, a plinth, a wall
	 * block, a door leaf. The outline is drawn in the plane's (u, v) metres and pushed DepthMeters along its normal.
	 *
	 * The sides face out of the solid whichever way the loops were drawn: the outline is run counter-clockwise and
	 * the holes clockwise, so each keeps its solid on the left. On the sides U runs along the depth and V round the
	 * loop; the two faces, when asked for, have UVs of (u, v) metres. Returns false, adding nothing, for an open or
	 * crossing loop.
	 */
	bool Extrude(FMeshBuilder& Mesh, const FPlaneFrame& Plane, const FProfileStations& Outline,
		TConstArrayView<FProfileStations> Holes, double DepthMeters, bool bFaces);
}
