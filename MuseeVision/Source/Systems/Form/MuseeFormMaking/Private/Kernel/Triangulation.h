#pragma once

#include "CoreMinimal.h"

namespace Musee::Form
{
	/**
	 * Triangulates a planar polygon with holes: a floor round its column bases, a slab round its stair well, the end of
	 * a moulding. The triangles index the points in order, the outer boundary first and then each hole, and no point
	 * is added or moved: the triangles' edges along every boundary are exactly the boundary's own edges, so the face
	 * meets the surfaces round it vertex for vertex.
	 *
	 * It is a constrained Delaunay triangulation (GeometryAlgorithms' TConstrainedDelaunay2, with exact predicates),
	 * so no triangle is a needless sliver and concave outlines need no special care. Loops may wind either way.
	 * Returns false, with no triangles, when a boundary crosses itself or another, or has fewer than three points.
	 */
	bool TriangulatePolygon(TConstArrayView<FVector2D> Outer, TConstArrayView<TArray<FVector2D>> Holes,
		TArray<FIntVector>& OutTriangles);

	/**
	 * Triangulates a planar polygon with holes and with points inside it, as TriangulatePolygon does: a surface set out
	 * over a plan (a landform, a roof bent both ways), whose inner points carry its height. The triangles index the
	 * outline's points, then each hole's, then the inner points. An inner point must lie inside the outline and clear of
	 * every boundary; one outside is left out of every triangle. Returns false, with no triangles, where
	 * TriangulatePolygon would.
	 */
	bool TriangulatePolygonWithPoints(TConstArrayView<FVector2D> Outer, TConstArrayView<TArray<FVector2D>> Holes,
		TConstArrayView<FVector2D> Inner, TArray<FIntVector>& OutTriangles);
}
