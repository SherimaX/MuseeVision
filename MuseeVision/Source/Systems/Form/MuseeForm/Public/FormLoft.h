#pragma once

#include "CoreMinimal.h"
#include "FormProfile.h"

/** One section of a loft: a closed outline in the frame's (x, y), stood at a height along its z axis. */
struct FFormLoftSection
{
	/** The outline, closed, (x, y) in metres. */
	FFormProfile Outline;

	/** How far along the frame's z axis it stands, metres. */
	double AtMeters = 0.0;

	/**
	 * How the surfaces on either side of this section meet: Auto keeps them smooth unless they turn by more than the
	 * crease rule's 20 degrees; Crisp makes an arris round the section (a stop, the foot of a chamfer).
	 */
	EFormJoint Joint = EFormJoint::Auto;
};

/**
 * A lofted solid: a shaft or a blade whose section changes along its length, as a mason works a tapered herm, a joiner a
 * post with stopped chamfers, or a founder a sun's tapering ray. Its sections stand one above another along the drawing
 * frame's z axis, each a closed outline in (x, y); the surface runs straight from each section to the next, point to
 * corresponding point, and the first and last sections close its ends flat. It is one closed solid.
 *
 * The sections are drawn alike, step for step: the same number of lines and arcs in the same order, so each point has
 * its fellow in every other section (an arc is divided into as many chords as its largest fellow needs). A step may
 * shrink to nothing in one section and open out in the next: a chamfer that stops, a ridge that rises from a flat.
 */
struct FFormLoft
{
	/** The side. */
	static constexpr const TCHAR* SidePart = TEXT("side");
	/** The two ends. */
	static constexpr const TCHAR* EndPart = TEXT("end");

	/** At least two, rising strictly along z. */
	TArray<FFormLoftSection> Sections;
};
