#pragma once

#include "CoreMinimal.h"

/** Where the visitor is on Earth (for the real sky, the moon and the solar terms). */
struct FMuseeObserver
{
	double Latitude = 40.0;   // degrees, north positive
	double Longitude = 0.0;   // degrees, east positive

	/** The museum's city (MuseeClock): where the sky, the moon and the stars are seen from. */
	static FMuseeObserver FromTimeZone();
};

/**
 * The museum's clock: the local time of the city it stands in, Beijing or New York (console
 * musee.City, 0 Beijing, 1 New York; C in the museum switches). The sky, the sun clock, the moon and
 * the stars follow it, whatever the PC's own time zone.
 */
namespace MuseeClock
{
	enum class ECity : uint8 { Beijing, NewYork };

	ECity City();
	void SetCity(ECity City);
	FText CityName(ECity City);
	/** The city's offset from UTC at that moment (New York keeps US daylight saving). */
	FTimespan UtcOffset(ECity City, const FDateTime& Utc);
	/** Now in the city, or today at musee.Hour when that is set. */
	FDateTime LocalNow();
	FDateTime UtcNow();
	FMuseeObserver Observer();
}

/**
 * Low-precision ephemerides (after Meeus and the Astronomical Almanac's short formulas), good to a
 * fraction of a degree. A port of Shared/Core/Sky.swift. Directions are Unreal world unit vectors
 * (X east, Y south, Z up).
 */
namespace MuseeEphemeris
{
	double JulianDay(const FDateTime& Utc);
	/** The sun's apparent ecliptic longitude (radians). */
	double SunLongitude(const FDateTime& Utc);

	struct FSun { FVector Direction; double Altitude; };
	struct FMoon { FVector Direction; double Altitude; double Illuminated; bool bWaxing; };

	/** Direction towards the sun, and its altitude (radians). */
	FSun Sun(const FDateTime& Utc, const FMuseeObserver& Observer);
	/** Direction towards the moon, its altitude, illuminated fraction and whether it is waxing. */
	FMoon Moon(const FDateTime& Utc, const FMuseeObserver& Observer);

	/** Takes equatorial unit vectors (x to the vernal equinox, z to the celestial pole) to world. */
	FMatrix SkyRotation(const FDateTime& Utc, const FMuseeObserver& Observer);

	/** Index 0…23 of the current solar term (0 = Lichun, the sun at 315°). */
	int32 SolarTerm(const FDateTime& Utc);
	/** Hanzi, pinyin and English name of a solar term. */
	void SolarTermName(int32 Index, FString& Hanzi, FString& Pinyin, FString& English);
}
