#pragma once

#include "CoreMinimal.h"
#include "FormProfile.h"

/**
 * An ornament in relief: a carved frieze, a cast panel, a boss, drawn as a height field over a flat or a curved
 * ground (D-014's Ornament). The carving stands HeightMeters(U, V) proud of its ground, sampled on a grid of GridMeters
 * (the carver's finest cut), and is a closed block BackMeters deep behind the ground.
 *
 * - **On a plan path** (Path set): U runs along the path in metres from its start, V is the height above the drawing
 *   frame's origin from BottomMeters to TopMeters, and the ground stands OffsetMeters to the path's right with the
 *   carving proud towards the right: the die of a coping, the frieze round a drum.
 * - **On a plane** (Path empty): U and V are the drawing frame's x and y over SizeMeters, and the carving stands proud
 *   along its z axis.
 */
struct FFormRelief
{
	/** The carved face. */
	static constexpr const TCHAR* FacePart = TEXT("face");
	/** The back and the edges of the block. */
	static constexpr const TCHAR* BlockPart = TEXT("block");

	/** The path the ground follows in plan, or empty for a flat ground. */
	FFormProfile Path;

	/** On a path: the ground's distance to the path's right, and the band of heights it covers, metres. */
	double OffsetMeters = 0.0;
	double BottomMeters = 0.0;
	double TopMeters = 0.0;

	/** On a plane: the ground's extent along x and y, metres. */
	FVector2D SizeMeters = FVector2D::ZeroVector;

	/** The carving's grid, metres. */
	double GridMeters = 0.0;

	/** The depth of the block behind the ground, metres. */
	double BackMeters = 0.0;

	/** How far the carving stands proud of the ground at (U, V), metres, never negative. */
	TFunction<double(double U, double V)> HeightMeters;

	/** Whether the ground follows a path rather than the frame's plane. */
	[[nodiscard]] bool IsOnPath() const { return !Path.Start.IsEmpty(); }
};
