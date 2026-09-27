#pragma once

#include "CoreMinimal.h"
#include "FormArris.h"
#include "FormProfile.h"

/**
 * A pocket sunk into one face of a slab or block, square to it with a flat floor: a recess for an inlay or a mark, the
 * housing a fitting stands in, the rebate a slab's edge is let into. Where its outline stays inside the face it is a
 * blind pocket; where it runs over the slab's edge it is a rebate, open to the side.
 */
struct FFormPocket
{
	/** The pocket's outline, closed, in the frame's (x, y), metres; the part outside the slab cuts nothing. */
	FFormProfile Outline;

	/** How deep it is sunk, metres: less than the slab is thick. */
	double DepthMeters = 0.0;

	/** Sunk into the near face (at z = 0) rather than the far face (at z = DepthMeters). */
	bool bNearFace = false;
};

/**
 * A slab or block cut from a closed outline: a floor round its opening, a plinth, a door leaf, a stair's side. The
 * outline and its holes are drawn in the drawing frame's (x, y) plane and pushed DepthMeters along its z axis, giving a
 * closed solid with both faces and every side. Pockets may be sunk into either face, and the arrises round either face
 * eased. Its parts:
 */
struct FFormExtrusion
{
	/** The far face, at z = DepthMeters: a slab's top, a leaf's face. */
	static constexpr const TCHAR* FarPart = TEXT("far");
	/** The near face, at z = 0: a slab's bed, a blade's underside. */
	static constexpr const TCHAR* NearPart = TEXT("near");
	/** The sides round the outline and the holes, and the eased arrises. */
	static constexpr const TCHAR* SidePart = TEXT("side");
	/** The walls and floors of the pockets. */
	static constexpr const TCHAR* PocketPart = TEXT("pocket");

	/** The outline, closed, in the frame's (x, y), metres. */
	FFormProfile Outline;

	/** Holes through the slab, each closed and inside the outline. */
	TArray<FFormProfile> Holes;

	/** How far the outline is pushed along the frame's z axis, metres. */
	double DepthMeters = 0.0;

	/** Pockets and rebates, none crossing another in the same face. */
	TArray<FFormPocket> Pockets;

	/**
	 * The arrises round the far face and round the near face (the outline's and the holes'), eased. A pocket keeps
	 * clear of an eased arris. The outline's own corners are eased by drawing them as arcs.
	 */
	FFormArris FarArris;
	FFormArris NearArris;
};
