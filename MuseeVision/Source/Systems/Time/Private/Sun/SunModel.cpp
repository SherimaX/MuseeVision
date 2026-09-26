#include "Sun/SunModel.h"

#include "Almanac/CelestialAngles.h"

namespace Musee::Time::IdealisedGeometry
{
	/** At civil noon the idealised sun stands due south, where the real one stands at apparent noon north of the
	 *  tropics, so XII falls due north on the dial. */
	constexpr double NoonHour = 12.0;
	constexpr double NoonAzimuthDegrees = 180.0;
}

namespace Musee::Time
{
	double FIdealisedSun::AltitudeDegrees() const
	{
		return Atan2Degrees(NodeHeightMeters, HourRingRadiusMeters);
	}

	double FIdealisedSun::AzimuthDegrees(double CivilHour) const
	{
		using namespace IdealisedGeometry;
		return ReduceDegrees(NoonAzimuthDegrees + DegreesPerHour * (CivilHour - NoonHour));
	}

	bool FIdealisedSun::IsUp(double CivilHour) const
	{
		return CivilHour >= FirstHour && CivilHour <= LastHour;
	}

	FVector FIdealisedSun::Direction(double CivilHour) const
	{
		FHorizontalPlace Place;
		Place.AzimuthDegrees = AzimuthDegrees(CivilHour);
		Place.AltitudeDegrees = AltitudeDegrees();
		return DirectionFromHorizontal(Place);
	}

	FVector FIdealisedSun::ShadowPoint(double CivilHour) const
	{
		const double Bearing = AzimuthDegrees(CivilHour) + HalfTurnDegrees;
		return FVector(HourRingRadiusMeters * SinDegrees(Bearing), -HourRingRadiusMeters * CosDegrees(Bearing), 0.0);
	}

	double FIdealisedSun::ReadHour(const FVector& PointOnDial) const
	{
		using namespace IdealisedGeometry;
		const double Bearing = Atan2Degrees(PointOnDial.X, -PointOnDial.Y);
		const double SunAzimuth = Bearing - HalfTurnDegrees;
		const double Hour = NoonHour + SignedDegrees(SunAzimuth - NoonAzimuthDegrees) / DegreesPerHour;
		return Hour < FirstHour ? Hour + HoursPerDay : Hour;
	}

	FMuseumSun MuseumSun(ESunModel Model, const FIdealisedSun& Idealised, double CivilHour, const FSunPosition& RealSun)
	{
		FMuseumSun Result;
		Result.Model = Model;
		if (Model == ESunModel::Idealised)
		{
			Result.Direction = Idealised.Direction(CivilHour);
			Result.AzimuthDegrees = Idealised.AzimuthDegrees(CivilHour);
			Result.AltitudeDegrees = Idealised.AltitudeDegrees();
			Result.bUp = Idealised.IsUp(CivilHour);
			return Result;
		}
		Result.Direction = RealSun.Place.Direction;
		Result.AzimuthDegrees = RealSun.Place.Apparent.AzimuthDegrees;
		Result.AltitudeDegrees = RealSun.Place.Apparent.AltitudeDegrees;
		Result.bUp = RealSun.Place.Apparent.AltitudeDegrees + RealSun.Place.SemidiameterDegrees > 0.0;
		return Result;
	}
}
