#pragma once

#include "CoreMinimal.h"
#include "Almanac/Almanac.h"

/**
 * The Sun's day at a place: its transit of the meridian (solar noon), its rising and setting, and the three
 * twilights. Each event is found on the full ephemeris by Newton's method, to a hundredth of a second.
 */
namespace Musee::Time
{
	/** The Sun's centre altitude at each event (the conventions of the Astronomical Almanac and of Meeus ch. 15). */
	namespace SolarDayAltitudes
	{
		/** Rising and setting: the upper limb on the horizon, with 34' of refraction and a 16' semidiameter. */
		inline constexpr double SunriseDegrees = -0.8333;
		/** Civil, nautical and astronomical twilight begin or end with the centre 6, 12 and 18 degrees down. */
		inline constexpr double CivilTwilightDegrees = -6.0;
		inline constexpr double NauticalTwilightDegrees = -12.0;
		inline constexpr double AstronomicalTwilightDegrees = -18.0;
	}

	/** The morning and evening instants at which the Sun's centre crosses one altitude. Unset when it never does. */
	struct FSolarCrossing
	{
		TOptional<FDateTime> MorningUtc;
		TOptional<FDateTime> EveningUtc;
	};

	struct FSolarDay
	{
		/** Solar noon: the Sun's centre on the meridian. */
		FDateTime TransitUtc;
		/** The Sun's geometric altitude at transit. */
		double TransitAltitudeDegrees = 0.0;
		FSolarCrossing Sunrise;
		FSolarCrossing CivilTwilight;
		FSolarCrossing NauticalTwilight;
		FSolarCrossing AstronomicalTwilight;

		/** Hours from sunrise to sunset: 24 in polar day, 0 in polar night. */
		double DayLengthHours() const;
	};

	/**
	 * The Sun's day for the civil date that begins at DayStartUtc (local midnight, in UTC) at an observer. The transit
	 * is the one nearest local noon; each crossing is sought in the half-day before or after it. Altitudes are
	 * geocentric, as the Astronomical Almanac and the NREL SPA define rising and setting.
	 */
	FSolarDay SolarDay(const FDateTime& DayStartUtc, const FObserver& Observer);

	/** The morning and evening crossings of one centre altitude around a transit (Julian Day, UT). */
	FSolarCrossing SolarCrossing(double TransitJulianDay, double AltitudeDegrees, const FObserver& Observer);

	/**
	 * The hour angle at which a body of a given declination crosses an altitude at a latitude (Meeus eq. 15.1): 0 when
	 * it never climbs that high, 180 when it never sinks that low. Twice it, at 15 degrees an hour, is the time spent
	 * above the altitude.
	 */
	double CrossingHourAngleDegrees(double LatitudeDegrees, double DeclinationDegrees, double AltitudeDegrees);
}
