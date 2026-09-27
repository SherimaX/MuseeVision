#pragma once

#include "CoreMinimal.h"
#include "FormProfile.h"

/**
 * A solid between two surfaces over a plan: a landform (a pond's bank falling from the garden's ground to its bed), a
 * roof slab bent both ways (a pavilion's concave sweep lifting to its corners), a stone step whose underside is dressed
 * to a stair's helicoid soffit, as a spandrel step's is. Over its outline and holes, drawn in the drawing frame's (x, y),
 * its top lies at TopMeters(x, y) and its underside at BottomMeters(x, y), and its sides stand plumb on the outline: one
 * closed solid of the three parts below. The top must stand above the underside wherever the solid is set out.
 *
 * Each surface is set out on one triangulation of the plan whose points lie no further apart than GridMeters (the
 * outline's stations, its chords divided to that spacing, and a lattice within), and its normals come from the surface's
 * own slope. The grid sets how finely a surface is followed, as a relief's does: a grid g follows a surface whose
 * curvature is at most k within g^2 k / 8.
 */
struct FFormHeightfield
{
	/** The top surface. */
	static constexpr const TCHAR* TopPart = TEXT("top");
	/** The underside. */
	static constexpr const TCHAR* BottomPart = TEXT("bottom");
	/** The plumb sides on the outline and round the holes. */
	static constexpr const TCHAR* SidePart = TEXT("side");

	/** The plan outline, closed, in the frame's (x, y), metres; either way round. */
	FFormProfile Outline;

	/** Holes through the solid, each closed and inside the outline. */
	TArray<FFormProfile> Holes;

	/** The heights of the top and the underside above the frame's origin at (x, y), metres. */
	TFunction<double(double X, double Y)> TopMeters;
	TFunction<double(double X, double Y)> BottomMeters;

	/** The furthest apart the points each surface is set out on may lie, metres. */
	double GridMeters = 0.0;
};
