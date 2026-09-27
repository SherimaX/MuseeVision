#pragma once

#include "CoreMinimal.h"

/**
 * The Moon's phase: how much of the disc the Sun lights, which way the lit limb faces, and where the Moon is in its
 * month. The phases are defined by the apparent longitudes of the Moon and the Sun (Meeus ch. 49): new Moon when
 * they are equal, first quarter at 90 degrees, full at 180, last quarter at 270.
 */
namespace Musee::Time
{
	struct FLunarCoordinates;
	struct FSolarCoordinates;

	struct FLunarPhase
	{
		/** The geocentric angle between the Moon and the Sun, 0 to 180 degrees (Meeus eq. 48.2). */
		double ElongationDegrees = 0.0;
		/** The Sun-Moon-Earth angle (Meeus eq. 48.3). */
		double PhaseAngleDegrees = 0.0;
		/** The lit fraction of the disc, (1 + cos i) / 2 (Meeus eq. 48.1). */
		double IlluminatedFraction = 0.0;
		/** The position angle of the lit limb's midpoint, from celestial north through east (Meeus eq. 48.5). */
		double BrightLimbAngleDegrees = 0.0;
		/** The Moon's apparent longitude minus the Sun's, [0, 360): 0 new, 90 first quarter, 180 full, 270 last. */
		double LongitudeDifferenceDegrees = 0.0;

		bool IsWaxing() const { return LongitudeDifferenceDegrees < 180.0; }
	};

	/** The phase from the Moon's and the Sun's apparent geocentric places at one instant. */
	FLunarPhase LunarPhase(const FLunarCoordinates& Lunar, const FSolarCoordinates& Solar);

	/**
	 * The Julian Ephemeris Day at which the Moon's apparent longitude leads the Sun's by TargetDegrees (0 new,
	 * 90 first quarter, 180 full, 270 last quarter), the instant nearest the guess (within half a month of it).
	 */
	double LunarPhaseInstant(double TargetDegrees, double JulianEphemerisDayGuess);

	/** The Julian Ephemeris Day of the last new Moon at or before the instant. */
	double PreviousNewMoon(double JulianEphemerisDay);
}
