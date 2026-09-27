#include "Almanac/CelestialSphere.h"

#include "Almanac/CelestialAngles.h"

namespace Musee::Time::EarthFigure
{
	/** The Earth's equatorial radius, 6378.140 km, and polar-to-equatorial ratio b/a = 1 - 1/298.257: the IAU 1976
	 *  ellipsoid, as Meeus (ch. 11) and the NREL SPA use for the observer's place. */
	constexpr double EquatorialRadiusMeters = 6378140.0;
	constexpr double PolarToEquatorialRatio = 0.99664719;
}

namespace Musee::Time
{
	FEquatorialPlace EclipticToEquatorial(double LongitudeDegrees, double LatitudeDegrees, double ObliquityDegrees)
	{
		const double SinLongitude = SinDegrees(LongitudeDegrees);
		const double CosObliquity = CosDegrees(ObliquityDegrees);
		const double SinObliquity = SinDegrees(ObliquityDegrees);
		FEquatorialPlace Place;
		Place.RightAscensionDegrees = ReduceDegrees(Atan2Degrees(
			SinLongitude * CosObliquity - TanDegrees(LatitudeDegrees) * SinObliquity, CosDegrees(LongitudeDegrees)));
		Place.DeclinationDegrees = AsinDegrees(
			SinDegrees(LatitudeDegrees) * CosObliquity + CosDegrees(LatitudeDegrees) * SinObliquity * SinLongitude);
		return Place;
	}

	FHorizontalPlace HorizontalFromHourAngle(double HourAngleDegrees, double DeclinationDegrees, double LatitudeDegrees)
	{
		const double SinLatitude = SinDegrees(LatitudeDegrees);
		const double CosLatitude = CosDegrees(LatitudeDegrees);
		const double SinDeclination = SinDegrees(DeclinationDegrees);
		const double CosDeclination = CosDegrees(DeclinationDegrees);
		const double CosHour = CosDegrees(HourAngleDegrees);
		FHorizontalPlace Place;
		Place.AltitudeDegrees = AsinDegrees(SinLatitude * SinDeclination + CosLatitude * CosDeclination * CosHour);
		// Measured westward from south (Meeus eq. 13.5, multiplied through by cos(declination)), then turned to be
		// measured eastward from north.
		const double FromSouth = Atan2Degrees(SinDegrees(HourAngleDegrees) * CosDeclination,
			CosHour * SinLatitude * CosDeclination - SinDeclination * CosLatitude);
		Place.AzimuthDegrees = ReduceDegrees(FromSouth + HalfTurnDegrees);
		return Place;
	}

	FVector DirectionFromHorizontal(const FHorizontalPlace& Place)
	{
		const double CosAltitude = CosDegrees(Place.AltitudeDegrees);
		return FVector(CosAltitude * SinDegrees(Place.AzimuthDegrees), -CosAltitude * CosDegrees(Place.AzimuthDegrees),
			SinDegrees(Place.AltitudeDegrees));
	}

	FHorizontalPlace HorizontalFromDirection(const FVector& Direction)
	{
		const FVector Unit = Direction.GetSafeNormal();
		FHorizontalPlace Place;
		Place.AltitudeDegrees = AsinDegrees(Unit.Z);
		Place.AzimuthDegrees = ReduceDegrees(Atan2Degrees(Unit.X, -Unit.Y));
		return Place;
	}

	FTopocentricPlace Topocentric(const FEquatorialPlace& Geocentric, double HourAngleDegrees, double ParallaxDegrees,
		double LatitudeDegrees, double ElevationMeters)
	{
		using namespace EarthFigure;
		// The observer's geocentric position: rho cos(phi') and rho sin(phi') (Meeus p. 82).
		const double ReducedLatitude = AtanDegrees(PolarToEquatorialRatio * TanDegrees(LatitudeDegrees));
		const double Height = ElevationMeters / EquatorialRadiusMeters;
		const double RhoCos = CosDegrees(ReducedLatitude) + Height * CosDegrees(LatitudeDegrees);
		const double RhoSin = PolarToEquatorialRatio * SinDegrees(ReducedLatitude) + Height * SinDegrees(LatitudeDegrees);

		const double SinParallax = SinDegrees(ParallaxDegrees);
		const double CosDeclination = CosDegrees(Geocentric.DeclinationDegrees);
		const double Denominator = CosDeclination - RhoCos * SinParallax * CosDegrees(HourAngleDegrees);
		const double RightAscensionShift = Atan2Degrees(-RhoCos * SinParallax * SinDegrees(HourAngleDegrees), Denominator);

		FTopocentricPlace Place;
		Place.RightAscensionDegrees = ReduceDegrees(Geocentric.RightAscensionDegrees + RightAscensionShift);
		Place.DeclinationDegrees = Atan2Degrees(
			(SinDegrees(Geocentric.DeclinationDegrees) - RhoSin * SinParallax) * CosDegrees(RightAscensionShift), Denominator);
		Place.HourAngleDegrees = HourAngleDegrees - RightAscensionShift;
		return Place;
	}

	FMatrix EquatorialToHorizonFrame(double LocalSiderealDegrees, double LatitudeDegrees)
	{
		const double SinTheta = SinDegrees(LocalSiderealDegrees);
		const double CosTheta = CosDegrees(LocalSiderealDegrees);
		const double SinLatitude = SinDegrees(LatitudeDegrees);
		const double CosLatitude = CosDegrees(LatitudeDegrees);
		// Each row is the image of one equatorial axis. An equatorial vector turned by -theta about the pole gives
		// (a, b, c) = (towards the meridian's equator point, east, pole); the horizon frame then reads
		// east = b, south = a sin(phi) - c cos(phi), up = a cos(phi) + c sin(phi).
		const FVector EquinoxAxis(-SinTheta, CosTheta * SinLatitude, CosTheta * CosLatitude);
		const FVector QuarterAxis(CosTheta, SinTheta * SinLatitude, SinTheta * CosLatitude);
		const FVector PoleAxis(0.0, -CosLatitude, SinLatitude);
		return FMatrix(EquinoxAxis, QuarterAxis, PoleAxis, FVector::ZeroVector);
	}
}
