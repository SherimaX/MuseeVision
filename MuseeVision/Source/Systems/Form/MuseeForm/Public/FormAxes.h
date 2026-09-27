#pragma once

#include "CoreMinimal.h"
#include "MuseeFrame.h"

/**
 * Frames set out by their axes, as a drawing is: a wall anchor whose x runs along the wall and whose y stands out of it,
 * a door leaf's plane, a member's section square to its length.
 */
namespace Musee::Form
{
	/**
	 * The frame at a point of its parent whose x axis runs along X and whose y axis lies in the plane of X and Y, on Y's
	 * side, square to x (Y need not be square to X, only not along it); z completes them as the engine's axes do (x east,
	 * y south, z up). Neither direction need be of unit length.
	 */
	[[nodiscard]] MUSEEFORM_API FMuseeFrame FrameOfAxes(const FVector& OriginMeters, const FVector& X, const FVector& Y);
}
