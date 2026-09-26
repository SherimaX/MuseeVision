#pragma once

#include "CoreMinimal.h"
#include "MuseeUnits.h"

/**
 * The human measure: the body the museum is built for, as constants only. Form cuts steps and walkable faces to it,
 * Visitor's body is made from it, and Site's reach agent walks with it, so the three can never disagree. The formulas
 * that use these constants live with their users (Blondel's rule in Form, walking in Visitor, the reach agent in
 * Site); this type holds the measurements and where they come from.
 */
struct FMuseeHumanMeasure
{
	/**
	 * Standing eye height: the museum's standard eye, for hanging and for the visitor's view. It lies between the
	 * median standing eye heights of British adult women (1.505 m) and men (1.630 m) in S. Pheasant and C. Haslegrave,
	 * Bodyspace, 3rd edition (2006).
	 */
	static constexpr Musee::Meters EyeHeight{1.60};

	/**
	 * The crown of the head above the eye: stature less eye height for the same median adults, 110 mm for men
	 * (1.740 m less 1.630 m) and 105 mm for women (1.610 m less 1.505 m).
	 */
	static constexpr Musee::Meters CrownAboveEye{0.11};

	/** The height of the body whose eye is at EyeHeight. */
	static constexpr Musee::Meters Stature = EyeHeight + CrownAboveEye;

	/**
	 * The radius of the body's capsule: half the shoulder breadth of Fruin's body ellipse, 24 by 18 inches (0.61 m by
	 * 0.46 m) for a clothed adult (J. J. Fruin, Pedestrian Planning and Design, 1971), the unit of pedestrian and egress
	 * design.
	 */
	static constexpr Musee::Meters CapsuleRadius{0.305};

	/** Half the height of the body's capsule, whose ends are hemispheres: the capsule is as tall as the body. */
	static constexpr Musee::Meters CapsuleHalfHeight = Stature / 2.0;

	/**
	 * The tallest rise the body steps up without climbing: the greatest rise any stair may have under the Building
	 * Regulations for England, Approved Document K (2013), table 1.1 (a private stair, 220 mm). Kerbs and risers are
	 * stepped; anything taller is a ledge.
	 */
	static constexpr Musee::Meters MaxStepHeight{0.22};

	/**
	 * The steepest ground the body walks on: EN ISO 14122-1:2016 sets a ramp as the means of access up to 20 degrees;
	 * steeper ground needs steps.
	 */
	static constexpr Musee::Degrees MaxWalkableSlope{20.0};

	/** The least clear height over any walked surface: Approved Document K (2013), 2.0 m over stairs and landings. */
	static constexpr Musee::Meters MinHeadroom{2.0};

	/**
	 * How far in front of the shoulder a hand grips: the forward grip reach of the 5th-percentile British woman,
	 * 0.65 m (Bodyspace, 3rd edition), so what is within reach is within every visitor's reach.
	 */
	static constexpr Musee::Meters Reach{0.65};

	/**
	 * The height of guarding at an edge the body could fall from: Approved Document K (2013), table 3.1, 1100 mm at the
	 * edges of floors, landings and balconies in buildings other than dwellings.
	 */
	static constexpr Musee::Meters GuardHeight{1.10};

	/**
	 * One pace of the walking body: the step rule of DIN 18065:2020, two rises and one going make one pace of 590 mm to
	 * 650 mm, taken at its mean of 630 mm; the rule descends from F. Blondel, Cours d'architecture (1675).
	 */
	static constexpr Musee::Meters Pace{0.63};
};

static_assert(FMuseeHumanMeasure::CapsuleHalfHeight > FMuseeHumanMeasure::CapsuleRadius, "A capsule is taller than it is wide.");
static_assert(FMuseeHumanMeasure::Stature < FMuseeHumanMeasure::MinHeadroom, "The body walks upright under the least headroom.");
static_assert(FMuseeHumanMeasure::MaxStepHeight < FMuseeHumanMeasure::GuardHeight, "A step is lower than the guarding of an edge.");
