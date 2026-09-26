#include "Dial/Dial.h"

#include "Almanac/AstronomicalTime.h"
#include "Almanac/CelestialAngles.h"

namespace Musee::Time::Gnomonics
{
	/** Apparent noon, when the hour angle is zero. */
	constexpr double NoonHour = 12.0;
	/** The reading error is sampled every degree of hour angle (four minutes of time) and at nine declinations. */
	constexpr double HourAngleStepDegrees = 1.0;
	constexpr int32 DeclinationSamples = 9;
	/** Hour planes closer than this to parallel with the face (sine of the angle) have no line on it. */
	constexpr double ParallelSine = 1e-9;

	/** A direction's components in the hour-angle frame of a latitude: towards the meridian's equator point, east,
	 *  and the north celestial pole. The inverse of the map in SunDirectionAtHourAngle. */
	FVector HourAngleFrame(const FVector& Direction, double LatitudeDegrees)
	{
		const double SinLatitude = SinDegrees(LatitudeDegrees);
		const double CosLatitude = CosDegrees(LatitudeDegrees);
		return FVector(Direction.Y * SinLatitude + Direction.Z * CosLatitude, Direction.X,
			-Direction.Y * CosLatitude + Direction.Z * SinLatitude);
	}

	/** The unit vector towards the north celestial pole at a latitude. */
	FVector CelestialPole(double LatitudeDegrees)
	{
		return FVector(0.0, -CosDegrees(LatitudeDegrees), SinDegrees(LatitudeDegrees));
	}
}

namespace Musee::Time
{
	FDial FDial::Horizontal(double NodusHeightMeters, double InLatitudeDegrees)
	{
		FDial Dial;
		Dial.NodusMeters = FVector(0.0, 0.0, NodusHeightMeters);
		Dial.LatitudeDegrees = InLatitudeDegrees;
		return Dial;
	}

	FVector SunDirectionAtHourAngle(double HourAngleDegrees, double DeclinationDegrees, double LatitudeDegrees)
	{
		// In the hour-angle frame the Sun is (cos d cos H, -cos d sin H, sin d); the horizon frame then reads
		// east = b, south = a sin(phi) - c cos(phi), up = a cos(phi) + c sin(phi).
		const double A = CosDegrees(DeclinationDegrees) * CosDegrees(HourAngleDegrees);
		const double B = -CosDegrees(DeclinationDegrees) * SinDegrees(HourAngleDegrees);
		const double C = SinDegrees(DeclinationDegrees);
		const double SinLatitude = SinDegrees(LatitudeDegrees);
		const double CosLatitude = CosDegrees(LatitudeDegrees);
		return FVector(B, A * SinLatitude - C * CosLatitude, A * CosLatitude + C * SinLatitude);
	}

	TOptional<FVector> DialShadowPoint(const FDial& Dial, const FVector& SunDirection)
	{
		const double Facing = FVector::DotProduct(SunDirection, Dial.FaceNormal);
		if (Facing <= 0.0) { return {}; }
		const double NodusStandoff = FVector::DotProduct(Dial.NodusMeters - Dial.FaceOriginMeters, Dial.FaceNormal);
		return Dial.NodusMeters - SunDirection * (NodusStandoff / Facing);
	}

	TOptional<FDialLine> DialHourLine(const FDial& Dial, double ApparentSolarHour)
	{
		using namespace Gnomonics;
		const double HourAngle = DegreesPerHour * (ApparentSolarHour - NoonHour);
		const FVector HourPlaneNormal = FVector::CrossProduct(
			CelestialPole(Dial.LatitudeDegrees), SunDirectionAtHourAngle(HourAngle, 0.0, Dial.LatitudeDegrees));
		const FVector LineDirection = FVector::CrossProduct(Dial.FaceNormal, HourPlaneNormal);
		const double LineLengthSquared = LineDirection.SizeSquared();
		if (LineLengthSquared < ParallelSine * ParallelSine) { return {}; }

		// The point where the face (n . x = d1) meets the hour plane (m . x = d2):
		// x = (d1 (m x l) + d2 (l x n)) / |l|^2, with l = n x m.
		const double FaceDistance = FVector::DotProduct(Dial.FaceNormal, Dial.FaceOriginMeters);
		const double HourPlaneDistance = FVector::DotProduct(HourPlaneNormal, Dial.NodusMeters);
		FDialLine Line;
		Line.PointMeters = (FaceDistance * FVector::CrossProduct(HourPlaneNormal, LineDirection)
			+ HourPlaneDistance * FVector::CrossProduct(LineDirection, Dial.FaceNormal)) / LineLengthSquared;
		Line.Direction = LineDirection.GetUnsafeNormal();
		return Line;
	}

	double DialReadApparentHour(const FDial& Dial, const FVector& PointMeters)
	{
		using namespace Gnomonics;
		const FVector TowardsSun = (Dial.NodusMeters - PointMeters).GetSafeNormal();
		const FVector Frame = HourAngleFrame(TowardsSun, Dial.LatitudeDegrees);
		const double HourAngle = Atan2Degrees(-Frame.Y, Frame.X);
		const double Hour = NoonHour + HourAngle / DegreesPerHour;
		return Hour < 0.0 ? Hour + HoursPerDay : (Hour >= HoursPerDay ? Hour - HoursPerDay : Hour);
	}

	double DialReadingErrorMinutes(const FDial& Dial, double ObserverLatitudeDegrees, double MaximumDeclinationDegrees)
	{
		using namespace Gnomonics;
		double LargestMinutes = 0.0;
		for (int32 Sample = 0; Sample < DeclinationSamples; ++Sample)
		{
			const double Declination = MaximumDeclinationDegrees * (2.0 * Sample / (DeclinationSamples - 1) - 1.0);
			for (double HourAngle = -HalfTurnDegrees; HourAngle < HalfTurnDegrees; HourAngle += HourAngleStepDegrees)
			{
				const FVector TowardsSun = SunDirectionAtHourAngle(HourAngle, Declination, ObserverLatitudeDegrees);
				const TOptional<FVector> Shadow = TowardsSun.Z > 0.0 ? DialShadowPoint(Dial, TowardsSun) : TOptional<FVector>();
				if (!Shadow.IsSet()) { continue; }
				const double TrueHour = NoonHour + HourAngle / DegreesPerHour;
				const double ErrorHours = SignedDegrees(DegreesPerHour * (DialReadApparentHour(Dial, Shadow.GetValue()) - TrueHour))
					/ DegreesPerHour;
				LargestMinutes = FMath::Max(LargestMinutes, FMath::Abs(ErrorHours) * MinutesPerHour);
			}
		}
		return LargestMinutes;
	}
}
