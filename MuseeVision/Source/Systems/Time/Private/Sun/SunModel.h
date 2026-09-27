#pragma once

#include "CoreMinimal.h"
#include "Almanac/Almanac.h"

/**
 * The sun the museum shows. Decision D-003 keeps the approved design: an idealised sun that keeps the museum's civil
 * time on the Rotunda's sun clock. Data/Time/Museum.json declares which model shows (idealised by default) and the
 * idealised sun's geometry. The Moon, the stars and the solar terms always follow the real sky.
 */
namespace Musee::Time
{
	enum class ESunModel : uint8
	{
		/** The approved design: an even path that always lands the node's point of shadow on the hour. */
		Idealised,
		/** The real Sun of the almanac, for the museum's city. */
		Ephemeris,
	};

	/**
	 * The idealised sun (the as-built design; windows/Source/MuseeVision/Sky/MuseeSky.cpp, SunClockDirection, and
	 * plan/canvas/Rotunda.dc.html). The geometry, in the Rotunda's frame (x east, y south, z up, the dial's centre at
	 * the origin):
	 *  - the centre of the bronze node stands NodeHeightMeters (20.25 m) above the centre of the dial;
	 *  - its point of shadow falls on the hour ring, a circle of HourRingRadiusMeters (6.78 m) about the centre;
	 *  - at civil hour h the point lies at bearing 270 + 15 (h - 6) degrees from true north: VI due west, XII due
	 *    north, VI due east, 15 degrees an hour in every season, so the dial reads civil time directly;
	 *  - the sun therefore shines from the opposite bearing, 180 + 15 (h - 12) degrees (east at 06:00, south at noon,
	 *    west at 18:00), at a constant altitude atan(20.25 / 6.78) = 71.49 degrees: its path is a cone about the vertical
	 *    through the node, with the node at its apex;
	 *  - it is up from FirstHour (06:00) to LastHour (18:00) and down otherwise.
	 * Nothing here depends on latitude, so the city may change at runtime without recutting the dial (D-002, D-003).
	 */
	struct FIdealisedSun
	{
		double NodeHeightMeters = 0.0;
		double HourRingRadiusMeters = 0.0;
		double FirstHour = 0.0;
		double LastHour = 0.0;

		/** The constant altitude of the sun on its cone. */
		double AltitudeDegrees() const;
		/** The sun's bearing from true north at a civil hour (not wrapped to the up hours). */
		double AzimuthDegrees(double CivilHour) const;
		bool IsUp(double CivilHour) const;
		/** The unit vector towards the sun at a civil hour, in the museum's horizon frame. */
		FVector Direction(double CivilHour) const;
		/** The node's point of shadow on the dial plane at a civil hour, in metres from the dial's centre. */
		FVector ShadowPoint(double CivilHour) const;
		/** The civil hour a point on the dial reads from its bearing about the centre, in [FirstHour, FirstHour + 24):
		 *  the hour scale runs once round from VI. */
		double ReadHour(const FVector& PointOnDial) const;
	};

	/** The sun the museum shows at an instant. */
	struct FMuseumSun
	{
		ESunModel Model = ESunModel::Idealised;
		/** The unit vector towards the sun, in the museum's horizon frame. */
		FVector Direction = FVector::UpVector;
		double AzimuthDegrees = 0.0;
		double AltitudeDegrees = 0.0;
		/** Whether the sun is above the horizon: for the real Sun, its upper limb as seen; for the idealised sun, its
		 *  hours. */
		bool bUp = false;
	};

	/** The museum's sun under a model: the idealised sun at the museum's civil hour, or the real one. */
	FMuseumSun MuseumSun(ESunModel Model, const FIdealisedSun& Idealised, double CivilHour, const FSunPosition& RealSun);
}
