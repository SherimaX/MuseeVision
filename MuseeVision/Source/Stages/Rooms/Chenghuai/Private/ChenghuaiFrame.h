#pragma once

#include "CoreMinimal.h"
#include "FormAxes.h"
#include "MuseeFrame.h"

/**
 * How a building of Chenghuai is set out on the plan (§3.1): a carpenter's frame with u along the building's front, v
 * from its front eave columns to its back, and z up from the stage's datum (the Rotunda's floor). The sheet sets its
 * frames square to the plan and in both hands (a hall facing south runs u east and v north, the mirror of x and y), so a
 * frame keeps its hand, and every drawing it places turns its extrusion to suit: the section a carpenter draws is the
 * same whichever way the building faces.
 */
namespace Musee::Chenghuai
{
	/** Frames set out by their axes are Form's (FormAxes.h). */
	using Musee::Form::FrameOfAxes;

	struct FBuildingFrame
	{
		/** The frame's origin on the plan, and its u and v directions (unit, square), in the stage's (x east, y south). */
		FVector2D Origin = FVector2D::ZeroVector;
		FVector2D U = FVector2D(1.0, 0.0);
		FVector2D V = FVector2D(0.0, 1.0);

		/** The stage's own frame: u east, v south. */
		static FBuildingFrame Stage() { return FBuildingFrame(); }

		/** A frame at a point of the plan with its u and v along two plan directions (normalised here). */
		static FBuildingFrame Of(const FVector2D& Origin, const FVector2D& U, const FVector2D& V);

		/** A frame inside this one: its origin at (u, v) here and its axes along directions given in this frame's (u, v). */
		FBuildingFrame Within(double AtU, double AtV, const FVector2D& UInHere, const FVector2D& VInHere) const;

		/** +1 when u, v and up turn as the stage's x, y and z do; -1 for a mirrored frame. */
		double Hand() const;

		FVector2D Plan(double AtU, double AtV) const { return Origin + U * AtU + V * AtV; }
		FVector At(double AtU, double AtV, double Z) const;
		FVector U3() const { return FVector(U.X, U.Y, 0.0); }
		FVector V3() const { return FVector(V.X, V.Y, 0.0); }

		/** A drawing frame with x along v and y up, facing along u (a (v, z) section run along the building from UStart). */
		FMuseeFrame AlongU(double UStart) const;

		/** A drawing frame with x along u and y up, facing along v (a (u, z) section run through the building from VStart). */
		FMuseeFrame AlongV(double VStart) const;

		/** A drawing frame with x along u and y along v (times the hand), z up, at a height: an outline on the plan. */
		FMuseeFrame Flat(double Z) const;

		/** Where a v given in this frame lies along a drawing's +z (AlongV's z is -Hand v): v times -Hand. */
		double DepthOfV(double AtV) const { return -Hand() * AtV; }

		/** Where a u given in this frame lies along a drawing's +z (AlongU's z is Hand u). */
		double DepthOfU(double AtU) const { return Hand() * AtU; }
	};
}
