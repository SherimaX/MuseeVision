#pragma once

#include "CoreMinimal.h"
#include "FormProfile.h"

/**
 * A flat ceiling or roof slab with a daylight opening: the roof ring round a glass dome, a laylight's frame, a slab
 * round a lantern. The slab's soffit is the drawing frame's (x, y) plane and it rises ThicknessMeters along z. When the
 * opening is glazed, a pane of PaneMeters fills it (a separate piece, of GlazingSurface, cast as glass); otherwise the
 * opening is open to what lies above.
 */
struct FFormCeiling
{
	/** The slab's outline, closed, in the frame's (x, y), metres. */
	FFormProfile Outline;

	/** The opening, closed and inside the outline. */
	FFormProfile Opening;

	double ThicknessMeters = 0.0;

	/** The pane's thickness, metres; zero leaves the opening unglazed. */
	double PaneMeters = 0.0;

	/** The pane's underside above the soffit, metres: zero sets it flush with the soffit. */
	double PaneLevelMeters = 0.0;

	/** The finish schedule's surface for the pane (glass, stained glass, silk). */
	FName GlazingSurface;
};
