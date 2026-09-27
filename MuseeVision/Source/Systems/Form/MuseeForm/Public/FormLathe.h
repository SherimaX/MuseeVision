#pragma once

#include "CoreMinimal.h"
#include "FormProfile.h"

/**
 * A turned solid: a column's base, a post, a baluster, a dome. The section (A the radius from the axis, never
 * negative; B the height along it) is turned about the drawing frame's z axis, from its x axis towards its y axis (east
 * towards south). A section that ends on the axis at both ends, or a closed one, turned through a full turn is a closed
 * solid.
 */
struct FFormLathe
{
	/** The section, (radius, height), metres. */
	FFormProfile Section;

	/** Where the turn starts and ends, degrees from the frame's x axis. A full turn is 0 to 360. */
	double FromDegrees = 0.0;
	double ToDegrees = 360.0;
};
