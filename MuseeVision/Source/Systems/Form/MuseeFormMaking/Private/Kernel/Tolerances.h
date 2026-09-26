#pragma once

#include "CoreMinimal.h"

/**
 * The making kernel's own tolerances: the sizes and angles below which two things are one thing. They belong to the
 * kernel's arithmetic and to the finest work in the museum, not to any piece, so they are written here once.
 */
namespace Musee::Form
{
	/**
	 * Two vertices closer than one micrometre are one point. The finest made detail in the museum, a 0.5 mm arris,
	 * is five hundred times larger, and double rounding at the site's largest distances (about 200 m) is below 1e-13 m.
	 */
	inline constexpr double WeldToleranceMeters = 1.0e-6;

	/**
	 * Two drawn elements that meet turning less than a microradian meet tangent. A turn that small is rounding in the
	 * drawing's arithmetic, never a drawn corner.
	 */
	inline constexpr double TangentJointRadians = 1.0e-6;

	/**
	 * The crease rule for a path known only by its points: a turn above 20 degrees is a corner (crisp), a smaller one
	 * follows a curve (smooth). It is the as-built museum's rule (windows/Source/MuseeVision/Salon/SalonKit.h). At
	 * the 1 mm chord tolerance every arc above 0.1 m radius turns less than 17 degrees from one station to the next,
	 * so it reads as smooth.
	 */
	inline constexpr double CreaseAngleRadians = UE_DOUBLE_PI / 9.0;
}
