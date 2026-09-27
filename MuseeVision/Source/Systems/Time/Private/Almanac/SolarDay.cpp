#include "Almanac/SolarDay.h"

#include "Almanac/CelestialAngles.h"
#include "Almanac/SiderealTime.h"

namespace Musee::Time::SolarDaySearch
{
	/** The Sun's hour angle grows by one turn per mean solar day. */
	constexpr double SolarHourAngleDegreesPerDay = DegreesPerTurn;
	/** Newton's method stops when a step is below 1e-7 day (about 9 ms). */
	constexpr double ToleranceDays = 1e-7;
	constexpr int32 Iterations = 12;
	/** A crossing belongs to the transit it was sought from only within half a day of it. */
	constexpr double HalfDay = 0.5;

	struct FSunState
	{
		double HourAngleDegrees = 0.0;
		double DeclinationDegrees = 0.0;
		double AltitudeDegrees = 0.0;
	};

	/** The Sun's geocentric hour angle, declination and altitude at a Julian Day on UT. */
	FSunState GeocentricSun(double JulianDayUniversal, const FObserver& Observer)
	{
		const FInstant Instant = FInstant::FromUtc(UtcFromJulianDay(JulianDayUniversal));
		const FSolarCoordinates Solar = SolarCoordinates(Instant.JulianEphemerisDay());
		const double LocalSidereal = ApparentSiderealDegrees(JulianDayUniversal, Solar.Nutation) + Observer.LongitudeDegrees;
		FSunState State;
		State.HourAngleDegrees = SignedDegrees(LocalSidereal - Solar.RightAscensionDegrees);
		State.DeclinationDegrees = Solar.DeclinationDegrees;
		State.AltitudeDegrees =
			HorizontalFromHourAngle(State.HourAngleDegrees, State.DeclinationDegrees, Observer.LatitudeDegrees).AltitudeDegrees;
		return State;
	}

	/** Newton's method on the altitude, from a first estimate, for the instant the centre is at AltitudeDegrees. */
	TOptional<double> RefineCrossing(double Estimate, double TransitJulianDay, double AltitudeDegrees, const FObserver& Observer)
	{
		double JulianDay = Estimate;
		for (int32 Iteration = 0; Iteration < Iterations; ++Iteration)
		{
			const FSunState State = GeocentricSun(JulianDay, Observer);
			// dh/dt = -cos(phi) cos(delta) sin(H) (dH/dt) / cos(h)
			const double Rate = -CosDegrees(Observer.LatitudeDegrees) * CosDegrees(State.DeclinationDegrees)
				* SinDegrees(State.HourAngleDegrees) * SolarHourAngleDegreesPerDay / CosDegrees(State.AltitudeDegrees);
			if (FMath::IsNearlyZero(Rate)) { return {}; }
			const double Step = (AltitudeDegrees - State.AltitudeDegrees) / Rate;
			JulianDay += Step;
			if (FMath::Abs(Step) < ToleranceDays) { break; }
		}
		if (FMath::Abs(JulianDay - TransitJulianDay) > HalfDay) { return {}; }
		return JulianDay;
	}

	TOptional<FDateTime> ToUtc(const TOptional<double>& JulianDay)
	{
		return JulianDay.IsSet() ? TOptional<FDateTime>(UtcFromJulianDay(JulianDay.GetValue())) : TOptional<FDateTime>();
	}
}

namespace Musee::Time
{
	double FSolarDay::DayLengthHours() const
	{
		if (Sunrise.MorningUtc.IsSet() && Sunrise.EveningUtc.IsSet())
		{
			return (Sunrise.EveningUtc.GetValue() - Sunrise.MorningUtc.GetValue()).GetTotalHours();
		}
		return TransitAltitudeDegrees > SolarDayAltitudes::SunriseDegrees ? HoursPerDay : 0.0;
	}

	double CrossingHourAngleDegrees(double LatitudeDegrees, double DeclinationDegrees, double AltitudeDegrees)
	{
		const double CosHourAngle = (SinDegrees(AltitudeDegrees) - SinDegrees(LatitudeDegrees) * SinDegrees(DeclinationDegrees))
			/ (CosDegrees(LatitudeDegrees) * CosDegrees(DeclinationDegrees));
		return AcosDegrees(CosHourAngle);
	}

	FSolarCrossing SolarCrossing(double TransitJulianDay, double AltitudeDegrees, const FObserver& Observer)
	{
		using namespace SolarDaySearch;
		const FSunState AtTransit = GeocentricSun(TransitJulianDay, Observer);
		const double HourAngle = CrossingHourAngleDegrees(Observer.LatitudeDegrees, AtTransit.DeclinationDegrees, AltitudeDegrees);
		FSolarCrossing Crossing;
		if (HourAngle <= 0.0 || HourAngle >= HalfTurnDegrees)
		{
			return Crossing;   // the centre stays below (or above) this altitude all day
		}
		const double HalfArcDays = HourAngle / SolarHourAngleDegreesPerDay;
		Crossing.MorningUtc = ToUtc(RefineCrossing(TransitJulianDay - HalfArcDays, TransitJulianDay, AltitudeDegrees, Observer));
		Crossing.EveningUtc = ToUtc(RefineCrossing(TransitJulianDay + HalfArcDays, TransitJulianDay, AltitudeDegrees, Observer));
		return Crossing;
	}

	FSolarDay SolarDay(const FDateTime& DayStartUtc, const FObserver& Observer)
	{
		using namespace SolarDaySearch;
		double Transit = JulianDayFromUtc(DayStartUtc) + HalfDay;
		for (int32 Iteration = 0; Iteration < Iterations; ++Iteration)
		{
			const double Step = -GeocentricSun(Transit, Observer).HourAngleDegrees / SolarHourAngleDegreesPerDay;
			Transit += Step;
			if (FMath::Abs(Step) < ToleranceDays) { break; }
		}

		FSolarDay Day;
		Day.TransitUtc = UtcFromJulianDay(Transit);
		Day.TransitAltitudeDegrees = GeocentricSun(Transit, Observer).AltitudeDegrees;
		Day.Sunrise = SolarCrossing(Transit, SolarDayAltitudes::SunriseDegrees, Observer);
		Day.CivilTwilight = SolarCrossing(Transit, SolarDayAltitudes::CivilTwilightDegrees, Observer);
		Day.NauticalTwilight = SolarCrossing(Transit, SolarDayAltitudes::NauticalTwilightDegrees, Observer);
		Day.AstronomicalTwilight = SolarCrossing(Transit, SolarDayAltitudes::AstronomicalTwilightDegrees, Observer);
		return Day;
	}
}
