#pragma once

#include "CoreMinimal.h"
#include "FormProfile.h"

/**
 * A turned solid: a column's base, a post, a baluster, a dome. The section (A the radius from the axis, never
 * negative; B the height along it) is turned about the drawing frame's z axis, from its x axis towards its y axis (east
 * towards south). A section that ends on the axis at both ends, or a closed one, turned through a full turn is a closed
 * solid.
 *
 * A square lathe works the same section round a square, or any regular polygon, instead of a circle: an abacus, a
 * plinth, a moulded die, as a mason runs a moulding round a block with mitred corners. A is then the distance from the
 * axis square to each side (the apothem), so a point on the axis stays there and a section of half-width 0.45 makes a
 * block 0.90 square.
 */
struct FFormLathe
{
	/** The section, (radius, height), metres. */
	FFormProfile Section;

	/** Where the turn starts and ends, degrees from the frame's x axis. A full turn is 0 to 360. */
	double FromDegrees = 0.0;
	double ToDegrees = 360.0;

	/**
	 * The plan the section is turned round: zero for a circle, or 3 or more for a regular polygon of that many sides,
	 * the middle of its first side on the frame's x axis (4: a square with its sides square to x and y). A part turn
	 * ends square to the side it stops on.
	 */
	int32 Sides = 0;
};
