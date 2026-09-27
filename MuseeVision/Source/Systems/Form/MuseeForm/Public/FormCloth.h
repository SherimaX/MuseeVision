#pragma once

#include "CoreMinimal.h"

/**
 * A cloth stretched on a rectangular frame and sagging under its own weight: a linen scrim under a lantern, a velarium's
 * width between its bars. It is fixed along the frame's four sides, over (0 to SizeMeters.X, 0 to SizeMeters.Y) in the
 * drawing frame's (x, y), and hangs along the frame's -z (set the frame with z up). A cloth held in even tension T and
 * loaded by its weight q takes the membrane's shape, T (w_xx + w_yy) = -q with w = 0 on the frame (Timoshenko and
 * Woinowsky-Krieger, Theory of Plates and Shells, 1959, the membrane analogy of section 22), whose depth is set by the
 * tension it was stretched to; so the drawing gives the depth at the middle, SagMeters, and the shape is that
 * equation's. It is one closed solid of its thickness, both faces and the edges on the frame, set out on a grid no
 * coarser than GridMeters.
 */
struct FFormCloth
{
	/** The frame's clear span along x and y, metres. */
	FVector2D SizeMeters = FVector2D::ZeroVector;

	/** How far the middle hangs below the frame's plane, metres. */
	double SagMeters = 0.0;

	/** The cloth's thickness, metres. */
	double ThicknessMeters = 0.0;

	/** The furthest apart the points the cloth is set out on may lie, metres. */
	double GridMeters = 0.0;
};
