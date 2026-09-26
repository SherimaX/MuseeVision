#include "Almanac/Almanac.h"

#include "Almanac/CelestialAngles.h"
#include "Almanac/Precession.h"
#include "Almanac/SiderealTime.h"

namespace Musee::Time::AlmanacConstants
{
	/** The refraction adopted at the horizon, 34 arcminutes (0.5667 degree): Meeus ch. 15; NREL SPA sec. A.2. A body's
	 *  disc can show while its airless altitude is above minus this and its semidiameter. */
	constexpr double HorizonRefractionDegrees = 0.5667;
}

namespace Musee::Time
{
	FObserver FObserver::At(double InLatitudeDegrees, double InLongitudeDegrees, double InElevationMeters)
	{
		FObserver Observer;
		Observer.LatitudeDegrees = InLatitudeDegrees;
		Observer.LongitudeDegrees = InLongitudeDegrees;
		Observer.ElevationMeters = InElevationMeters;
		Observer.PressureHectopascals = StandardPressureHectopascals(InElevationMeters);
		Observer.TemperatureCelsius = StandardTemperatureCelsius(InElevationMeters);
		return Observer;
	}

	double LocalSiderealDegrees(const FInstant& Instant, const FObserver& Observer)
	{
		const FNutation NutationNow = Nutation(Instant.JulianEphemerisDay());
		return ReduceDegrees(ApparentSiderealDegrees(Instant.JulianDayUniversal, NutationNow) + Observer.LongitudeDegrees);
	}

	FSkyPlace SkyPlace(const FEquatorialPlace& Geocentric, double ParallaxDegrees, double SemidiameterDegrees,
		double LocalSidereal, const FObserver& Observer)
	{
		FSkyPlace Place;
		const double HourAngle = SignedDegrees(LocalSidereal - Geocentric.RightAscensionDegrees);
		Place.Topocentric = Topocentric(Geocentric, HourAngle, ParallaxDegrees, Observer.LatitudeDegrees, Observer.ElevationMeters);
		Place.Airless = HorizontalFromHourAngle(
			Place.Topocentric.HourAngleDegrees, Place.Topocentric.DeclinationDegrees, Observer.LatitudeDegrees);
		Place.Apparent = Place.Airless;
		if (Place.Airless.AltitudeDegrees >= -(SemidiameterDegrees + AlmanacConstants::HorizonRefractionDegrees))
		{
			Place.Apparent.AltitudeDegrees += RefractionDegrees(
				Place.Airless.AltitudeDegrees, Observer.PressureHectopascals, Observer.TemperatureCelsius);
		}
		Place.Direction = DirectionFromHorizontal(Place.Apparent);
		Place.SemidiameterDegrees = SemidiameterDegrees;
		return Place;
	}

	FSunPosition Sun(const FInstant& Instant, const FObserver& Observer)
	{
		FSunPosition Result;
		Result.Coordinates = SolarCoordinates(Instant.JulianEphemerisDay());
		Result.LocalSiderealDegrees = ReduceDegrees(
			ApparentSiderealDegrees(Instant.JulianDayUniversal, Result.Coordinates.Nutation) + Observer.LongitudeDegrees);
		FEquatorialPlace Geocentric;
		Geocentric.RightAscensionDegrees = Result.Coordinates.RightAscensionDegrees;
		Geocentric.DeclinationDegrees = Result.Coordinates.DeclinationDegrees;
		Result.Place = SkyPlace(Geocentric, Result.Coordinates.HorizontalParallaxDegrees(),
			Result.Coordinates.SemidiameterDegrees(), Result.LocalSiderealDegrees, Observer);
		return Result;
	}

	FMoonPosition Moon(const FInstant& Instant, const FObserver& Observer)
	{
		const double JulianEphemerisDay = Instant.JulianEphemerisDay();
		FMoonPosition Result;
		Result.Coordinates = LunarCoordinates(JulianEphemerisDay);
		Result.Phase = LunarPhase(Result.Coordinates, SolarCoordinates(JulianEphemerisDay));
		const double LocalSidereal = ReduceDegrees(
			ApparentSiderealDegrees(Instant.JulianDayUniversal, Result.Coordinates.Nutation) + Observer.LongitudeDegrees);
		FEquatorialPlace Geocentric;
		Geocentric.RightAscensionDegrees = Result.Coordinates.RightAscensionDegrees;
		Geocentric.DeclinationDegrees = Result.Coordinates.DeclinationDegrees;
		Result.Place = SkyPlace(Geocentric, Result.Coordinates.HorizontalParallaxDegrees,
			Result.Coordinates.SemidiameterDegrees(), LocalSidereal, Observer);
		Result.AgeDays = JulianEphemerisDay - PreviousNewMoon(JulianEphemerisDay);
		return Result;
	}

	FMatrix StarFieldOrientation(const FInstant& Instant, const FObserver& Observer)
	{
		const double JulianEphemerisDay = Instant.JulianEphemerisDay();
		const FNutation NutationNow = Nutation(JulianEphemerisDay);
		const double LocalSidereal = ReduceDegrees(
			ApparentSiderealDegrees(Instant.JulianDayUniversal, NutationNow) + Observer.LongitudeDegrees);
		// Row vectors: first carried from the catalogue's J2000 frame to the true equator of date, then over the
		// observer's horizon.
		return TrueEquatorOfDateFromJ2000(JulianEphemerisDay, NutationNow)
			* EquatorialToHorizonFrame(LocalSidereal, Observer.LatitudeDegrees);
	}
}
