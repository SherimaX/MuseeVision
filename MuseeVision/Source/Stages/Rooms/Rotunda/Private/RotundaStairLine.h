#pragma once

#include "CoreMinimal.h"
#include "FormGround.h"

/**
 * The stair's setting-out (§3.8), in plan angles φ (degrees, from +x towards +y) and heights: the nosing lines, the
 * treads' tops, and the pitch line everything else is measured from. The pitch line runs through each tread's top at its
 * mid-going and lies level over each landing from half a going beyond its edges (the as-built layout,
 * SunClockKit.cpp:83-121): the soffit is 0.40 under it, the string's top 0.12 over it, the balustrade's heights stand
 * over the string's top and the wall rail's over the pitch line.
 *
 * The stair's fittings are drawn on cylinders about the stage's axis, in the cylinder's development (FFormGround): U
 * metres round it from φ 0 the way φ rises, V the height, W out from the axis. A line drawn straight in the development
 * is a helix, so a raked bar is a sweep along a straight line and a ring on the rake is a circle.
 */
namespace Musee::Rotunda::Stair
{
	/** The nosing line of tread K (1 to 39), its front; Front(0) is the top landing's back edge. */
	double Front(int32 K);

	/** The top of tread K. */
	double TreadTop(int32 K);

	/** The pitch line's height at φ. */
	double Pitch(double Deg);

	/** The soffit, 0.40 under the pitch line, never below the landing's limit. */
	double Soffit(double Deg);

	/** The string's top, 0.12 over the pitch line, never above the lowered dial's underside. */
	double StringTop(double Deg);

	/**
	 * The angles at which the pitch line, the soffit or the string's top turns: the landings' ends and where a limit
	 * takes over. Ascending.
	 */
	TArray<double> Kinks();

	/** A plan point's angle, turned to lie within 180° of Near. */
	double AngleOf(double X, double Y, double NearDeg);

	/** The cylinder of a radius about the stage's axis, as a ground: V is the height itself. */
	FFormGround Cylinder(double Radius);

	/** How far round the cylinder of a radius a plan angle lies, metres from φ 0: the development's U. */
	double Around(double Radius, double Deg);

	/**
	 * A line of the development over plan angles From to To (ascending): (U on the cylinder, Height(φ) + Over) at both ends
	 * and at every kink between, so the line's chords are the stair's own.
	 */
	TArray<FVector2D> LineOver(double Radius, double FromDeg, double ToDeg, TFunctionRef<double(double)> Height, double Over);
}
