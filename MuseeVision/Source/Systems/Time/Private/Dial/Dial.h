#pragma once

#include "CoreMinimal.h"

/**
 * Dialling (gnomonics): a flat dial face and a nodus, a point whose shadow marks the time. The Rotunda's sun clock
 * is one: its bronze node is the nodus and its floor the face. For the real Sun the hour lines are cut for one
 * latitude (Decision D-002); for the idealised sun (D-003) the hour marks are the points its even path lands on,
 * whatever the latitude (see FIdealisedSun).
 *
 * Everything is in metres in the museum's horizon frame: x east, y south, z up, true north at -y.
 */
namespace Musee::Time
{
	struct FDial
	{
		/** The nodus, the point that casts the shadow. */
		FVector NodusMeters = FVector::ZeroVector;
		/** A point on the face, and the face's unit normal on the nodus's side. */
		FVector FaceOriginMeters = FVector::ZeroVector;
		FVector FaceNormal = FVector::UpVector;
		/** The latitude the hour lines are cut for. */
		double LatitudeDegrees = 0.0;

		/** A horizontal dial with its nodus a height above the face's origin: the Rotunda's floor and node. */
		static FDial Horizontal(double NodusHeightMeters, double InLatitudeDegrees);
	};

	/** A straight line on the dial face. */
	struct FDialLine
	{
		FVector PointMeters = FVector::ZeroVector;
		FVector Direction = FVector::ForwardVector;
	};

	/** The unit vector towards the Sun from its local hour angle (west of the meridian) and declination. */
	FVector SunDirectionAtHourAngle(double HourAngleDegrees, double DeclinationDegrees, double LatitudeDegrees);

	/** Where the nodus's shadow falls for a direction towards the sun; unset when the sun is not in front of the face. */
	TOptional<FVector> DialShadowPoint(const FDial& Dial, const FVector& SunDirection);

	/**
	 * The hour line of a local apparent solar hour (12 at apparent noon): the line on the face where the shadow falls
	 * at that hour on every day of the year. It is where the face meets the hour plane, the plane through the nodus
	 * that holds the polar axis and the Sun's hour circle. Unset when the hour plane is parallel to the face.
	 */
	TOptional<FDialLine> DialHourLine(const FDial& Dial, double ApparentSolarHour);

	/** The local apparent solar hour, [0, 24), that a point on the face reads on this dial's lines. */
	double DialReadApparentHour(const FDial& Dial, const FVector& PointMeters);

	/**
	 * The largest error, in minutes, of this dial read at another latitude: over the whole day in steps of four
	 * minutes and over the year's declinations up to the given one, wherever the Sun is up and lights the face.
	 */
	double DialReadingErrorMinutes(const FDial& Dial, double ObserverLatitudeDegrees, double MaximumDeclinationDegrees);
}
