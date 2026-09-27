#include "Almanac/Nutation.h"

#include "Almanac/AstronomicalTime.h"
#include "Almanac/CelestialAngles.h"

namespace Musee::Time::NutationTerms
{
	/**
	 * One periodic term: the multiples of the Moon's mean elongation D, the Sun's mean anomaly M, the Moon's mean
	 * anomaly M', the Moon's argument of latitude F and the longitude of the Moon's ascending node Omega, then the
	 * sine coefficient for delta psi (a + b T) and the cosine coefficient for delta epsilon (c + d T), in units of
	 * 0.0001 arcsecond. Meeus table 22.A.
	 */
	struct FTerm
	{
		int8 D;
		int8 M;
		int8 MoonM;
		int8 F;
		int8 Omega;
		double LongitudeSine;
		double LongitudeSineRate;
		double ObliquityCosine;
		double ObliquityCosineRate;
	};

	constexpr double TermUnitArcseconds = 0.0001;

	constexpr FTerm Terms[] = {
		{0, 0, 0, 0, 1, -171996, -174.2, 92025, 8.9}, {-2, 0, 0, 2, 2, -13187, -1.6, 5736, -3.1},
		{0, 0, 0, 2, 2, -2274, -0.2, 977, -0.5}, {0, 0, 0, 0, 2, 2062, 0.2, -895, 0.5},
		{0, 1, 0, 0, 0, 1426, -3.4, 54, -0.1}, {0, 0, 1, 0, 0, 712, 0.1, -7, 0},
		{-2, 1, 0, 2, 2, -517, 1.2, 224, -0.6}, {0, 0, 0, 2, 1, -386, -0.4, 200, 0},
		{0, 0, 1, 2, 2, -301, 0, 129, -0.1}, {-2, -1, 0, 2, 2, 217, -0.5, -95, 0.3},
		{-2, 0, 1, 0, 0, -158, 0, 0, 0}, {-2, 0, 0, 2, 1, 129, 0.1, -70, 0},
		{0, 0, -1, 2, 2, 123, 0, -53, 0}, {2, 0, 0, 0, 0, 63, 0, 0, 0},
		{0, 0, 1, 0, 1, 63, 0.1, -33, 0}, {2, 0, -1, 2, 2, -59, 0, 26, 0},
		{0, 0, -1, 0, 1, -58, -0.1, 32, 0}, {0, 0, 1, 2, 1, -51, 0, 27, 0},
		{-2, 0, 2, 0, 0, 48, 0, 0, 0}, {0, 0, -2, 2, 1, 46, 0, -24, 0},
		{2, 0, 0, 2, 2, -38, 0, 16, 0}, {0, 0, 2, 2, 2, -31, 0, 13, 0},
		{0, 0, 2, 0, 0, 29, 0, 0, 0}, {-2, 0, 1, 2, 2, 29, 0, -12, 0},
		{0, 0, 0, 2, 0, 26, 0, 0, 0}, {-2, 0, 0, 2, 0, -22, 0, 0, 0},
		{0, 0, -1, 2, 1, 21, 0, -10, 0}, {0, 2, 0, 0, 0, 17, -0.1, 0, 0},
		{2, 0, -1, 0, 1, 16, 0, -8, 0}, {-2, 2, 0, 2, 2, -16, 0.1, 7, 0},
		{0, 1, 0, 0, 1, -15, 0, 9, 0}, {-2, 0, 1, 0, 1, -13, 0, 7, 0},
		{0, -1, 0, 0, 1, -12, 0, 6, 0}, {0, 0, 2, -2, 0, 11, 0, 0, 0},
		{2, 0, -1, 2, 1, -10, 0, 5, 0}, {2, 0, 1, 2, 2, -8, 0, 3, 0},
		{0, 1, 0, 2, 2, 7, 0, -3, 0}, {-2, 1, 1, 0, 0, -7, 0, 0, 0},
		{0, -1, 0, 2, 2, -7, 0, 3, 0}, {2, 0, 0, 2, 1, -7, 0, 3, 0},
		{2, 0, 1, 0, 0, 6, 0, 0, 0}, {-2, 0, 2, 2, 2, 6, 0, -3, 0},
		{-2, 0, 1, 2, 1, 6, 0, -3, 0}, {2, 0, -2, 0, 1, -6, 0, 3, 0},
		{2, 0, 0, 0, 1, -6, 0, 3, 0}, {0, -1, 1, 0, 0, 5, 0, 0, 0},
		{-2, -1, 0, 2, 1, -5, 0, 3, 0}, {-2, 0, 0, 0, 1, -5, 0, 3, 0},
		{0, 0, 2, 2, 1, -5, 0, 3, 0}, {-2, 0, 2, 0, 1, 4, 0, 0, 0},
		{-2, 1, 0, 2, 1, 4, 0, 0, 0}, {0, 0, 1, -2, 0, 4, 0, 0, 0},
		{-1, 0, 1, 0, 0, -4, 0, 0, 0}, {-2, 1, 0, 0, 0, -4, 0, 0, 0},
		{1, 0, 0, 0, 0, -4, 0, 0, 0}, {0, 0, 1, 2, 0, 3, 0, 0, 0},
		{0, 0, -2, 2, 2, -3, 0, 0, 0}, {-1, -1, 1, 0, 0, -3, 0, 0, 0},
		{0, 1, 1, 0, 0, -3, 0, 0, 0}, {0, -1, 1, 2, 2, -3, 0, 0, 0},
		{2, -1, -1, 2, 2, -3, 0, 0, 0}, {0, 0, 3, 2, 2, -3, 0, 0, 0},
		{2, -1, 0, 2, 2, -3, 0, 0, 0},
	};
	static_assert(UE_ARRAY_COUNT(Terms) == 63, "The IAU 1980 nutation series has 63 terms (Meeus table 22.A).");

	/** Laskar's mean obliquity (Meeus eq. 22.3), in arcseconds, for U = T / 100 (units of 10,000 Julian years). */
	double MeanObliquityArcseconds(double U)
	{
		constexpr double Coefficients[] = {84381.448, -4680.93, -1.55, 1999.25, -51.38, -249.67, -39.05, 7.12, 27.87, 5.79, 2.45};
		double Sum = 0.0;
		double Power = 1.0;
		for (const double Coefficient : Coefficients)
		{
			Sum += Coefficient * Power;
			Power *= U;
		}
		return Sum;
	}
}

namespace Musee::Time
{
	FNutation Nutation(double JulianEphemerisDay)
	{
		using namespace NutationTerms;
		const double T = (JulianEphemerisDay - J2000JulianDay) / DaysPerJulianCentury;
		const double T2 = T * T;
		const double T3 = T2 * T;
		// The fundamental arguments (Meeus p. 144; NREL SPA eqs. 15-19).
		const double D = 297.85036 + 445267.111480 * T - 0.0019142 * T2 + T3 / 189474.0;
		const double M = 357.52772 + 35999.050340 * T - 0.0001603 * T2 - T3 / 300000.0;
		const double MoonM = 134.96298 + 477198.867398 * T + 0.0086972 * T2 + T3 / 56250.0;
		const double F = 93.27191 + 483202.017538 * T - 0.0036825 * T2 + T3 / 327270.0;
		const double Omega = 125.04452 - 1934.136261 * T + 0.0020708 * T2 + T3 / 450000.0;

		double LongitudeSum = 0.0;
		double ObliquitySum = 0.0;
		for (const FTerm& Term : Terms)
		{
			const double Argument = Term.D * D + Term.M * M + Term.MoonM * MoonM + Term.F * F + Term.Omega * Omega;
			LongitudeSum += (Term.LongitudeSine + Term.LongitudeSineRate * T) * SinDegrees(Argument);
			ObliquitySum += (Term.ObliquityCosine + Term.ObliquityCosineRate * T) * CosDegrees(Argument);
		}

		FNutation Result;
		Result.LongitudeDegrees = LongitudeSum * TermUnitArcseconds / ArcsecondsPerDegree;
		Result.ObliquityDegrees = ObliquitySum * TermUnitArcseconds / ArcsecondsPerDegree;
		Result.MeanObliquityDegrees = MeanObliquityArcseconds(T / 100.0) / ArcsecondsPerDegree;
		return Result;
	}
}
