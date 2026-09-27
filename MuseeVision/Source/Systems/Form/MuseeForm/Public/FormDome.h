#pragma once

#include "CoreMinimal.h"
#include "MuseeFrame.h"

/**
 * A dome on an elliptical plan: a glass dome over a velarium, the velarium's own cloth, a cupola. It springs from the
 * ellipse of its semi-axes at its frame's origin and rises RiseMeters to its crown. With a thickness it is a shell of
 * two faces; with none it is a single glazed or cloth surface. An oculus, when there is one, is the crown zone out to
 * OculusFraction of the plan, finished in its own surface.
 */
struct FFormDome
{
	FMuseeFrame Frame;                 // the dome's springing centre, z up
	double SemiAxisXMeters = 0;        // plan semi-axis along x
	double SemiAxisYMeters = 0;        // plan semi-axis along y
	double RiseMeters = 0;             // springing to crown
	double ThicknessMeters = 0;        // shell thickness (0: a single glazed or cloth surface)
	double OculusFraction = 0;         // 0 = none; else the crown zone out to this fraction of the plan, its own surface
	FName Surface;                     // the outer face
	FName InnerSurface;                // the inner face (NAME_None = Surface)
	FName OculusSurface;               // the crown zone (glazing, a velarium)
};
