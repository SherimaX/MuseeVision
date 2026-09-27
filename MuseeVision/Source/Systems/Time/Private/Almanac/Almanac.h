#pragma once

#include "CoreMinimal.h"
#include "Almanac/AstronomicalTime.h"
#include "Almanac/CelestialSphere.h"
#include "Almanac/LunarCoordinates.h"
#include "Almanac/LunarPhase.h"
#include "Almanac/Refraction.h"
#include "Almanac/SolarCoordinates.h"

/**
 * The almanac: the real Sun, Moon and stars as an observer at a place on the Earth sees them at an instant. These
 * are pure functions, used at runtime for the museum's sky and at making time for checks.
 */
namespace Musee::Time
{
	/** Where the sky is seen from: a place on the Earth and the air above it. */
	struct FObserver
	{
		/** Geodetic latitude, north positive. */
		double LatitudeDegrees = 0.0;
		/** Longitude, east positive. */
		double LongitudeDegrees = 0.0;
		/** Height above the reference ellipsoid (for the observer's parallax) and of the air column (refraction). */
		double ElevationMeters = 0.0;
		/** The air at the observer, for refraction. */
		double PressureHectopascals = StandardSeaLevelPressureHectopascals;
		double TemperatureCelsius = StandardSeaLevelTemperatureCelsius;

		/** An observer under the standard atmosphere for its elevation (ISO 2533). */
		static FObserver At(double LatitudeDegrees, double LongitudeDegrees, double ElevationMeters);
	};

	/** A body as the observer sees it. */
	struct FSkyPlace
	{
		/** Topocentric right ascension, declination and hour angle (the observer's parallax removed). */
		FTopocentricPlace Topocentric;
		/** Where the body stands geometrically, without the air. */
		FHorizontalPlace Airless;
		/** Where it is seen: raised by refraction while its disc can show above the horizon. */
		FHorizontalPlace Apparent;
		/** The unit vector towards the body as seen, in the museum's horizon frame (x east, y south, z up). */
		FVector Direction = FVector::UpVector;
		double SemidiameterDegrees = 0.0;
	};

	struct FSunPosition
	{
		FSolarCoordinates Coordinates;
		FSkyPlace Place;
		/** Local apparent sidereal time at the instant. */
		double LocalSiderealDegrees = 0.0;
	};

	struct FMoonPosition
	{
		FLunarCoordinates Coordinates;
		FLunarPhase Phase;
		FSkyPlace Place;
		/** Days since the last new Moon. */
		double AgeDays = 0.0;
	};

	/** The local apparent sidereal time: Greenwich apparent sidereal time plus the east longitude. */
	double LocalSiderealDegrees(const FInstant& Instant, const FObserver& Observer);

	/** The real Sun: full precision (about 0.0003 degrees in azimuth and zenith against the NREL SPA). */
	FSunPosition Sun(const FInstant& Instant, const FObserver& Observer);

	/** The real Moon, with its phase and age. */
	FMoonPosition Moon(const FInstant& Instant, const FObserver& Observer);

	/**
	 * The star field's orientation: the map from a star catalogue's frame, the equator and equinox of J2000.0 (x to
	 * the equinox, z to the north celestial pole, as the Yale Bright Star Catalogue and Hipparcos give places), into
	 * the museum's horizon frame. Precession and nutation carry the catalogue to the date; the apparent sidereal time
	 * turns it over the observer (see EquatorialToHorizonFrame). Annual aberration (at most 20.5 arcseconds) and
	 * proper motions are left out: both are below the almanac's 0.01 degree.
	 */
	FMatrix StarFieldOrientation(const FInstant& Instant, const FObserver& Observer);

	/**
	 * The observer's sky place of a body from its geocentric place: hour angle from the local sidereal time, the
	 * observer's parallax, then refraction while the body's disc can show (its airless altitude above minus its
	 * semidiameter and the 0.5667 degree of horizon refraction, as the NREL SPA does).
	 */
	FSkyPlace SkyPlace(const FEquatorialPlace& Geocentric, double ParallaxDegrees, double SemidiameterDegrees,
		double LocalSidereal, const FObserver& Observer);
}
