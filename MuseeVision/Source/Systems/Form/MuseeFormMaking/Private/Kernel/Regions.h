#pragma once

#include "CoreMinimal.h"
#include "Curve/GeneralPolygon2.h"
#include "Kernel/Tolerances.h"

namespace Musee::Form
{
	struct FProfileStations;

	/**
	 * A region of a plane, in metres: polygons with holes, each outer loop anticlockwise in (u, v) (positive signed area)
	 * and each hole clockwise. It is what a slab's face is once pockets are taken from it, what a floor leaves between its
	 * slabs, and a laid unit cut to its floor. Regions are joined, differenced and shrunk exactly (Clipper2, through
	 * GeometryAlgorithms, on integers at better than a nanometre over the region's extent).
	 */
	using FRegion = TArray<UE::Geometry::FGeneralPolygon2d>;

	/**
	 * A polygon of a region smaller than this is no piece of anything: a sliver where two cuts meet within rounding,
	 * a micrometre wide and a tenth of a millimetre long.
	 */
	inline constexpr double SliverSquareMeters = 1.0e-10;

	/**
	 * The shortest edge a cut region keeps for a made thing: a tenth of a millimetre, a fifth of the finest made detail
	 * (a 0.5 mm arris). A shorter one is where two cuts meet within their rounding.
	 */
	inline constexpr double LeastEdgeMeters = 1.0e-4;

	/** The region inside one closed loop and outside the holes; loops may wind either way. */
	FRegion RegionOf(TConstArrayView<FVector2D> Outer, TConstArrayView<TArray<FVector2D>> Holes = {});

	FRegion Union(const FRegion& A, const FRegion& B);
	FRegion Difference(const FRegion& A, const FRegion& B);
	FRegion Intersection(const FRegion& A, const FRegion& B);

	/** The region grown (positive) or shrunk (negative) by a distance square to every edge, its corners kept sharp. */
	FRegion Offset(const FRegion& Region, double Meters);

	double AreaSquareMeters(const FRegion& Region);

	/**
	 * The loops of one polygon of a region, the outer first and then the holes, as points, each without the spikes of no
	 * width and the repeated points a boolean leaves where two outlines ran along each other (CleanLoop at the weld
	 * tolerance). A loop left with no area is left out.
	 */
	TArray<TArray<FVector2D>> LoopsOf(const UE::Geometry::FGeneralPolygon2d& Polygon);

	/** Every loop of a region's every polygon, as points. */
	TArray<TArray<FVector2D>> AllLoops(const FRegion& Region);

	/**
	 * Adds to a closed loop every one of the points that lies on one of its edges (within the weld tolerance), in order
	 * along the edge. Faces that meet along an edge then meet vertex for vertex, with no T-junction left open.
	 */
	void InsertPointsOnEdges(TArray<FVector2D>& Loop, TConstArrayView<FVector2D> Points);

	/**
	 * A closed loop known only by its points as a profile's stations, by the crease rule: crisp where it turns more than
	 * CreaseRadians, smooth where it follows a curve. Points repeating their neighbour are dropped.
	 */
	FProfileStations LoopStations(TConstArrayView<FVector2D> Loop, double CreaseRadians = CreaseAngleRadians);

	/**
	 * A cut loop as a made thing's outline: without a point closer to the one before it than LeastMeters, or one where the
	 * loop turns straight back on itself (a spike of no width), dropped again and again until none is left.
	 */
	TArray<FVector2D> CleanLoop(TConstArrayView<FVector2D> Loop, double LeastMeters = LeastEdgeMeters);

	/** Whether a point lies inside a closed loop (a point on its edge may be read either way). */
	bool IsInside(TConstArrayView<FVector2D> Loop, const FVector2D& Point);
}
