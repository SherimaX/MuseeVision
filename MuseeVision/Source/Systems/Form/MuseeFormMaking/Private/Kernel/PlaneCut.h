#pragma once

#include "CoreMinimal.h"

namespace Musee::Form
{
	class FMeshBuilder;

	/**
	 * Cuts a made solid on a plane, as a block is sawn: every triangle is kept, clipped or dropped by the side of the plane
	 * it lies on (what lies on the normal's side is taken away), and the solid is closed again by the cut face, lying in
	 * the plane and facing along the normal, in the builder slot CutSlot. A clipped vertex takes its attributes (normal,
	 * UVs, colour) from the edge it was cut from, so a smooth surface stays smooth to the cut and a crease stays crisp.
	 * The cut face's UVs are metres in the plane.
	 *
	 * A plane that misses the solid leaves it as it was. Returns false, leaving the mesh as it was, when the plane takes the
	 * whole solid away, or when the cut's edge can't be closed (the solid was open along the plane, or the cut crosses
	 * itself).
	 */
	bool CutMesh(FMeshBuilder& Mesh, const FVector& PointMeters, const FVector& Normal, FName CutSlot, FString& OutWhy);
}
