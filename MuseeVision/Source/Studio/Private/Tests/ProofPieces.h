#pragma once

#include "CoreMinimal.h"
#include "MeshDescription.h"

namespace Musee::Studio
{
	/**
	 * A Tuscan column turned on a lathe, drawn in its own frame with the centre of its base at the origin and its axis
	 * along +z. Its proportions follow Vitruvius (De Architectura IV.7): seven lower diameters tall, a base and a
	 * capital each half a diameter high, a round plinth, and a shaft that tapers to three quarters of its lower
	 * diameter with entasis. One slot, stone.
	 */
	FMeshDescription TurnColumn(double HeightMeters);

	/**
	 * A stone wall slab with a glazed opening, drawn with the centre of its base at the origin, its faces to south and
	 * north. Slot 0 is the stone, slot 1 the pane of glass that fills the opening. Every edge is shared, so the stone
	 * is one closed solid.
	 */
	FMeshDescription BuildGlazedWall();

	/** How far the corner normals of the leaning square lean from the vertical, in degrees. */
	inline constexpr double LeanDegrees = 10.0;

	/**
	 * A flat 1 m square facing up whose corner normals lean outward by LeanDegrees: a surface whose drawn normals
	 * differ from the ones its geometry would give, so a cast that kept them can be told from one that recomputed them.
	 */
	FMeshDescription DrawLeaningSquare();
}
