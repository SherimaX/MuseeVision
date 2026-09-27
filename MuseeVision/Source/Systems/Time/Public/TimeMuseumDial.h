#pragma once

#include "CoreMinimal.h"
#include "Templates/ValueOrError.h"
#include "TimeDialMark.h"

namespace Musee::Time
{
	/**
	 * The museum's sun clock as Time keeps it (Decision D-003): the Rotunda's dial, on which the idealised sun's path
	 * lands the bronze node's point of shadow on the civil hour, 15 degrees an hour in every season, VI in the west, XII
	 * in the north, VI in the east. It is read from Data/Time/Museum.json, the same record the museum's sun is drawn from,
	 * so the stage that lays the hour ring, the hour lines, the meridian, the minute ticks and the numerals from it lays
	 * them where the sun Time shows puts the shadow.
	 *
	 * Everything is in metres in the dial's stage frame: x east, y south, z up, the dial's centre at the origin on its
	 * face (the lowered dial's plane).
	 */
	class MUSEETIME_API FMuseumDial
	{
	public:
		/** The museum's dial, from its time record; the error names the record and the field at fault. */
		[[nodiscard]] static TValueOrError<FMuseumDial, FString> Read();

		/** A dial with its node a height above the centre, its shadow circle, and the first and last hours of its sun. */
		FMuseumDial(double InNodusHeightMeters, double InShadowRadiusMeters, double InFirstHour, double InLastHour);

		/** The node's height above the dial's face. */
		[[nodiscard]] double NodusHeightMeters() const { return NodusHeight; }

		/** The radius of the circle the shadow walks: the hour ring's centre line. */
		[[nodiscard]] double ShadowRadiusMeters() const { return ShadowRadius; }

		/** The first and last civil hours the sun is up and the dial reads. */
		[[nodiscard]] double FirstHour() const { return First; }
		[[nodiscard]] double LastHour() const { return Last; }

		/** Where the shadow falls at a civil hour, in degrees from +x towards +y: 180 at VI, 270 at XII, 360 at VI. */
		[[nodiscard]] double AngleDegrees(double CivilHour) const;

		/** The node's point of shadow on the dial's face at a civil hour. */
		[[nodiscard]] FVector ShadowPoint(double CivilHour) const;

		/** The unit direction towards the sun at a civil hour, which casts the node's shadow there. */
		[[nodiscard]] FVector SunDirection(double CivilHour) const;

		/** The civil hour a point on the face reads, by its bearing about the centre. */
		[[nodiscard]] double ReadHour(const FVector& PointOnFace) const;

		/** Every mark from the first hour to the last, one every so many minutes (the whole hours among them). */
		[[nodiscard]] TArray<FDialMark> Marks(int32 EveryMinutes) const;

		/** The numeral of a civil hour on a twelve-hour dial, in Roman capitals: VI at 06:00, XII at noon, I at 13:00. */
		[[nodiscard]] static FString Numeral(int32 CivilHour);

	private:
		double NodusHeight = 0.0;
		double ShadowRadius = 0.0;
		double First = 0.0;
		double Last = 0.0;
	};
}
