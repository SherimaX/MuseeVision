#include "Almanac/SolarCoordinates.h"

#include "Almanac/AstronomicalTime.h"
#include "Almanac/CelestialAngles.h"
#include "Almanac/CelestialSphere.h"

namespace Musee::Time::SolarConstants
{
	/** The constant of aberration: the Sun is seen 20.4898 arcseconds behind its geometric place at 1 au, because
	 *  the Earth moves while the light travels (Meeus eq. 25.10). */
	constexpr double AberrationArcsecondsAtOneAu = 20.4898;
	/** The correction from the VSOP87 dynamical frame to FK5 (Meeus eq. 25.9). */
	constexpr double FrameCorrectionLongitudeArcseconds = -0.09033;
	constexpr double FrameCorrectionLatitudeArcseconds = 0.03916;
	/** The Sun's equatorial horizontal parallax at 1 au (Meeus ch. 40). */
	constexpr double ParallaxArcsecondsAtOneAu = 8.794;
	/** The Sun's semidiameter at 1 au (the Astronomical Almanac's 15'59.63"). */
	constexpr double SemidiameterArcsecondsAtOneAu = 959.63;
	/** The aberration term in the equation of time (Meeus eq. 28.3). */
	constexpr double EquationOfTimeAberrationDegrees = 0.0057183;
	/** A crossing is found when the longitude is within 1e-7 degree, about 9 ms of time. */
	constexpr double CrossingToleranceDegrees = 1e-7;
	constexpr int32 CrossingIterations = 12;
	/** Minutes of time per degree of hour angle (24 h for 360 degrees). */
	constexpr double MinutesOfTimePerDegree = 4.0;

	/** The Sun's mean longitude L0, for tau in Julian millennia of TT (Meeus eq. 28.2; NREL SPA eq. A2). */
	double MeanLongitude(double Tau)
	{
		const double Tau2 = Tau * Tau;
		return ReduceDegrees(280.4664567 + 360007.6982779 * Tau + 0.03032028 * Tau2 + Tau2 * Tau / 49931.0
			- Tau2 * Tau2 / 15300.0 - Tau2 * Tau2 * Tau / 2000000.0);
	}
}

namespace Musee::Time
{
	double FSolarCoordinates::HorizontalParallaxDegrees() const
	{
		return SolarConstants::ParallaxArcsecondsAtOneAu / (ArcsecondsPerDegree * Earth.RadiusAu);
	}

	double FSolarCoordinates::SemidiameterDegrees() const
	{
		return SolarConstants::SemidiameterArcsecondsAtOneAu / (ArcsecondsPerDegree * Earth.RadiusAu);
	}

	FSolarCoordinates SolarCoordinates(double JulianEphemerisDay)
	{
		using namespace SolarConstants;
		FSolarCoordinates Result;
		Result.Earth = EarthHeliocentric(JulianEphemerisDay);
		Result.Nutation = Nutation(JulianEphemerisDay);
		const double T = (JulianEphemerisDay - J2000JulianDay) / DaysPerJulianCentury;

		// The Result is where the Earth is not: turn the heliocentric place half a turn, then refer it to FK5.
		const double Theta = ReduceDegrees(Result.Earth.LongitudeDegrees + HalfTurnDegrees);
		const double LambdaPrime = Theta - 1.397 * T - 0.00031 * T * T;
		Result.GeometricLongitudeDegrees = ReduceDegrees(Theta + FrameCorrectionLongitudeArcseconds / ArcsecondsPerDegree);
		Result.LatitudeDegrees = -Result.Earth.LatitudeDegrees
			+ FrameCorrectionLatitudeArcseconds / ArcsecondsPerDegree * (CosDegrees(LambdaPrime) - SinDegrees(LambdaPrime));

		const double Aberration = -AberrationArcsecondsAtOneAu / (ArcsecondsPerDegree * Result.Earth.RadiusAu);
		Result.ApparentLongitudeDegrees = ReduceDegrees(Result.GeometricLongitudeDegrees + Result.Nutation.LongitudeDegrees + Aberration);

		const double Obliquity = Result.Nutation.TrueObliquityDegrees();
		const FEquatorialPlace Place = EclipticToEquatorial(Result.ApparentLongitudeDegrees, Result.LatitudeDegrees, Obliquity);
		Result.RightAscensionDegrees = Place.RightAscensionDegrees;
		Result.DeclinationDegrees = Place.DeclinationDegrees;

		const double Tau = (JulianEphemerisDay - J2000JulianDay) / DaysPerJulianMillennium;
		Result.MeanLongitudeDegrees = MeanLongitude(Tau);
		const double EquationDegrees = SignedDegrees(Result.MeanLongitudeDegrees - EquationOfTimeAberrationDegrees
			- Result.RightAscensionDegrees + Result.Nutation.LongitudeDegrees * CosDegrees(Obliquity));
		Result.EquationOfTimeMinutes = EquationDegrees * MinutesOfTimePerDegree;
		return Result;
	}

	double SolarLongitudeInstant(double TargetDegrees, double JulianEphemerisDayGuess)
	{
		using namespace SolarConstants;
		double JulianEphemerisDay = JulianEphemerisDayGuess;
		for (int32 Iteration = 0; Iteration < CrossingIterations; ++Iteration)
		{
			const double Remaining = SignedDegrees(TargetDegrees - SolarCoordinates(JulianEphemerisDay).ApparentLongitudeDegrees);
			JulianEphemerisDay += Remaining / SolarMeanMotionDegreesPerDay;
			if (FMath::Abs(Remaining) < CrossingToleranceDegrees) { break; }
		}
		return JulianEphemerisDay;
	}
}
