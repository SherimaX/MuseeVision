#pragma once

#include "CoreMinimal.h"

/**
 * The geometry of the sky: the frames a body's place is written in and the turns between them. The ecliptic frame
 * follows the Earth's orbit, the equatorial frame the Earth's axis, and the horizon frame the observer's ground.
 *
 * The museum's horizon frame is Foundation's: x east, y south, z up, with true north at -y. It is left-handed, as
 * Unreal's is; the equatorial frame (x to the vernal equinox, z to the north celestial pole) is right-handed.
 */
namespace Musee::Time
{
	/** A place on the celestial sphere in the equatorial frame of the date. */
	struct FEquatorialPlace
	{
		double RightAscensionDegrees = 0.0;
		double DeclinationDegrees = 0.0;
	};

	/** A place in the observer's sky. Azimuth runs from true north through east, as a navigator's does. */
	struct FHorizontalPlace
	{
		double AzimuthDegrees = 0.0;
		double AltitudeDegrees = 0.0;
	};

	/** A body's place seen from the observer's position on the Earth's surface rather than from its centre. */
	struct FTopocentricPlace
	{
		double RightAscensionDegrees = 0.0;
		double DeclinationDegrees = 0.0;
		double HourAngleDegrees = 0.0;
	};

	/** Ecliptic longitude and latitude to right ascension and declination for an obliquity (Meeus eqs. 13.3, 13.4). */
	FEquatorialPlace EclipticToEquatorial(double LongitudeDegrees, double LatitudeDegrees, double ObliquityDegrees);

	/** Hour angle (west of the meridian) and declination to azimuth and altitude at a latitude (Meeus eqs. 13.5, 13.6). */
	FHorizontalPlace HorizontalFromHourAngle(double HourAngleDegrees, double DeclinationDegrees, double LatitudeDegrees);

	/** The unit vector towards a place in the sky, in the museum's horizon frame. */
	FVector DirectionFromHorizontal(const FHorizontalPlace& Place);

	/** The place in the sky of a direction in the museum's horizon frame. */
	FHorizontalPlace HorizontalFromDirection(const FVector& Direction);

	/**
	 * Topocentric right ascension, declination and hour angle (Meeus ch. 40; NREL SPA eqs. 33-38). ParallaxDegrees is
	 * the body's equatorial horizontal parallax; elevation is above the reference ellipsoid (IAU 1976, as Meeus uses).
	 */
	FTopocentricPlace Topocentric(const FEquatorialPlace& Geocentric, double HourAngleDegrees, double ParallaxDegrees,
		double LatitudeDegrees, double ElevationMeters);

	/**
	 * The linear map from the equatorial frame (x to the vernal equinox, y to right ascension 90 degrees, z to the
	 * north celestial pole) into the museum's horizon frame, for a local sidereal time and latitude. As a row-vector
	 * FMatrix: TransformVector(equatorial unit vector) gives the museum-frame direction. Because the museum frame is
	 * left-handed and the equatorial frame right-handed, the map is orthogonal with determinant -1.
	 */
	FMatrix EquatorialToHorizonFrame(double LocalSiderealDegrees, double LatitudeDegrees);
}
