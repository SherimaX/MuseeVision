#pragma once

#include "CoreMinimal.h"
#include "MuseeFrame.h"

/**
 * A dome over an oval or round plan standing on its springing: a half ellipsoid, as a masonry dome is turned; or a shallow
 * paraboloid, as the Nymphéas oval's glass dome and the linen velarium stretched beneath it rise (z = rise (1 − ρ²), ρ the
 * elliptic radius fraction; the sheet's section 3.3). In its own frame its outer face is
 * (a cos p cos t, b cos p sin t, rise sin p) for the ellipsoid and (a ρ cos t, b ρ sin t, rise (1 − ρ²)) with ρ = cos p
 * for the paraboloid, for the parametric latitude p from the springing (0) to the crown (90 degrees) and t round the plan.
 * A shell of some thickness is one closed solid: its outer face, its inner face (the outer face moved inwards by the
 * thickness along its normal, so the shell is equally thick everywhere) and the ring it springs from. A dome of no
 * thickness is one sheet of glass or cloth, seen from the room beneath it: it faces inwards and takes the inner face's
 * surface. The crown zone, out to OculusFraction of the plan (where cos p is at most that fraction), is its own surface:
 * the glazing of an eye, a velarium's centre.
 */
struct FFormDome
{
	FMuseeFrame Frame;                 // the dome's springing centre, z up
	double SemiAxisXMeters = 0;        // plan semi-axis along x
	double SemiAxisYMeters = 0;        // plan semi-axis along y
	double RiseMeters = 0;             // springing to crown
	bool bParaboloid = false;          // a paraboloid (a stretched cloth, a shallow rooflight's glass), not a half ellipsoid
	double ThicknessMeters = 0;        // shell thickness (0: a single glazed or cloth surface)
	double OculusFraction = 0;         // 0 = none; else the crown zone out to this fraction of the plan, its own surface
	FName Surface;                     // the outer face
	FName InnerSurface;                // the inner face (NAME_None = Surface)
	FName OculusSurface;               // the crown zone (glazing, a velarium)
};
