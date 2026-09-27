#include "Almanac/LunarCoordinates.h"

#include "Almanac/AstronomicalTime.h"
#include "Almanac/CelestialAngles.h"
#include "Almanac/CelestialSphere.h"

namespace Musee::Time::LunarTerms
{
	/** A term of Meeus table 47.A: multiples of D, M, M', F; the sine coefficient for longitude (1e-6 degree) and the
	 *  cosine coefficient for distance (1e-3 km). */
	struct FLongitudeTerm
	{
		int8 D;
		int8 M;
		int8 MoonM;
		int8 F;
		int32 Longitude;
		int32 Distance;
	};

	/** A term of Meeus table 47.B: multiples of D, M, M', F and the sine coefficient for latitude (1e-6 degree). */
	struct FLatitudeTerm
	{
		int8 D;
		int8 M;
		int8 MoonM;
		int8 F;
		int32 Latitude;
	};

	constexpr FLongitudeTerm LongitudeTerms[] = {
		{0, 0, 1, 0, 6288774, -20905355}, {2, 0, -1, 0, 1274027, -3699111}, {2, 0, 0, 0, 658314, -2955968},
		{0, 0, 2, 0, 213618, -569925}, {0, 1, 0, 0, -185116, 48888}, {0, 0, 0, 2, -114332, -3149},
		{2, 0, -2, 0, 58793, 246158}, {2, -1, -1, 0, 57066, -152138}, {2, 0, 1, 0, 53322, -170733},
		{2, -1, 0, 0, 45758, -204586}, {0, 1, -1, 0, -40923, -129620}, {1, 0, 0, 0, -34720, 108743},
		{0, 1, 1, 0, -30383, 104755}, {2, 0, 0, -2, 15327, 10321}, {0, 0, 1, 2, -12528, 0},
		{0, 0, 1, -2, 10980, 79661}, {4, 0, -1, 0, 10675, -34782}, {0, 0, 3, 0, 10034, -23210},
		{4, 0, -2, 0, 8548, -21636}, {2, 1, -1, 0, -7888, 24208}, {2, 1, 0, 0, -6766, 30824},
		{1, 0, -1, 0, -5163, -8379}, {1, 1, 0, 0, 4987, -16675}, {2, -1, 1, 0, 4036, -12831},
		{2, 0, 2, 0, 3994, -10445}, {4, 0, 0, 0, 3861, -11650}, {2, 0, -3, 0, 3665, 14403},
		{0, 1, -2, 0, -2689, -7003}, {2, 0, -1, 2, -2602, 0}, {2, -1, -2, 0, 2390, 10056},
		{1, 0, 1, 0, -2348, 6322}, {2, -2, 0, 0, 2236, -9884}, {0, 1, 2, 0, -2120, 5751},
		{0, 2, 0, 0, -2069, 0}, {2, -2, -1, 0, 2048, -4950}, {2, 0, 1, -2, -1773, 4130},
		{2, 0, 0, 2, -1595, 0}, {4, -1, -1, 0, 1215, -3958}, {0, 0, 2, 2, -1110, 0},
		{3, 0, -1, 0, -892, 3258}, {2, 1, 1, 0, -810, 2616}, {4, -1, -2, 0, 759, -1897},
		{0, 2, -1, 0, -713, -2117}, {2, 2, -1, 0, -700, 2354}, {2, 1, -2, 0, 691, 0},
		{2, -1, 0, -2, 596, 0}, {4, 0, 1, 0, 549, -1423}, {0, 0, 4, 0, 537, -1117},
		{4, -1, 0, 0, 520, -1571}, {1, 0, -2, 0, -487, -1739}, {2, 1, 0, -2, -399, 0},
		{0, 0, 2, -2, -381, -4421}, {1, 1, 1, 0, 351, 0}, {3, 0, -2, 0, -340, 0},
		{4, 0, -3, 0, 330, 0}, {2, -1, 2, 0, 327, 0}, {0, 2, 1, 0, -323, 1165},
		{1, 1, -1, 0, 299, 0}, {2, 0, 3, 0, 294, 0}, {2, 0, -1, -2, 0, 8752},
	};
	static_assert(UE_ARRAY_COUNT(LongitudeTerms) == 60, "Meeus table 47.A has 60 terms.");

	constexpr FLatitudeTerm LatitudeTerms[] = {
		{0, 0, 0, 1, 5128122}, {0, 0, 1, 1, 280602}, {0, 0, 1, -1, 277693}, {2, 0, 0, -1, 173237},
		{2, 0, -1, 1, 55413}, {2, 0, -1, -1, 46271}, {2, 0, 0, 1, 32573}, {0, 0, 2, 1, 17198},
		{2, 0, 1, -1, 9266}, {0, 0, 2, -1, 8822}, {2, -1, 0, -1, 8216}, {2, 0, -2, -1, 4324},
		{2, 0, 1, 1, 4200}, {2, 1, 0, -1, -3359}, {2, -1, -1, 1, 2463}, {2, -1, 0, 1, 2211},
		{2, -1, -1, -1, 2065}, {0, 1, -1, -1, -1870}, {4, 0, -1, -1, 1828}, {0, 1, 0, 1, -1794},
		{0, 0, 0, 3, -1749}, {0, 1, -1, 1, -1565}, {1, 0, 0, 1, -1491}, {0, 1, 1, 1, -1475},
		{0, 1, 1, -1, -1410}, {0, 1, 0, -1, -1344}, {1, 0, 0, -1, -1335}, {0, 0, 3, 1, 1107},
		{4, 0, 0, -1, 1021}, {4, 0, -1, 1, 833}, {0, 0, 1, -3, 777}, {4, 0, -2, 1, 671},
		{2, 0, 0, -3, 607}, {2, 0, 2, -1, 596}, {2, -1, 1, -1, 491}, {2, 0, -2, 1, -451},
		{0, 0, 3, -1, 439}, {2, 0, 2, 1, 422}, {2, 0, -3, -1, 421}, {2, 1, -1, 1, -366},
		{2, 1, 0, 1, -351}, {4, 0, 0, 1, 331}, {2, -1, 1, 1, 315}, {2, -2, 0, -1, 302},
		{0, 0, 1, 3, -283}, {2, 1, 1, -1, -229}, {1, 1, 0, -1, 223}, {1, 1, 0, 1, 223},
		{0, 1, -2, -1, -220}, {2, 1, -1, -1, -220}, {1, 0, 1, 1, -185}, {2, -1, -2, -1, 181},
		{0, 1, 2, 1, -177}, {4, 0, -2, -1, 176}, {4, -1, -1, -1, 166}, {1, 0, 1, -1, -164},
		{4, 0, 1, -1, 132}, {1, 0, -1, -1, -119}, {4, -1, 0, -1, 115}, {2, -2, 0, 1, 107},
	};
	static_assert(UE_ARRAY_COUNT(LatitudeTerms) == 60, "Meeus table 47.B has 60 terms.");

	/** Units of the tables: 1e-6 degree for longitude and latitude, 1e-3 km for distance. */
	constexpr double MicrodegreesPerDegree = 1e6;
	constexpr double MetersPerKilometer = 1e3;
	/** The mean distance the distance terms are added to, 385000.56 km (Meeus ch. 47). */
	constexpr double MeanDistanceKilometers = 385000.56;
	/** The Earth's equatorial radius in km, for the parallax (Meeus ch. 47). */
	constexpr double EarthRadiusKilometers = 6378.14;
	/** Semidiameter numerator: s = 358473400 / distance arcseconds (Meeus ch. 55). */
	constexpr double SemidiameterArcsecondKilometers = 358473400.0;

	/** The fundamental arguments of the lunar theory at T Julian centuries of TT (Meeus eqs. 47.1-47.6). */
	struct FArguments
	{
		double MeanLongitude;
		double D;
		double M;
		double MoonM;
		double F;
		double A1;
		double A2;
		double A3;
		/** The decrease of the Earth orbit's eccentricity, which scales every term in M (Meeus eq. 47.6). */
		double E;
	};

	FArguments Arguments(double T)
	{
		const double T2 = T * T;
		const double T3 = T2 * T;
		const double T4 = T3 * T;
		FArguments Result;
		Result.MeanLongitude = 218.3164477 + 481267.88123421 * T - 0.0015786 * T2 + T3 / 538841.0 - T4 / 65194000.0;
		Result.D = 297.8501921 + 445267.1114034 * T - 0.0018819 * T2 + T3 / 545868.0 - T4 / 113065000.0;
		Result.M = 357.5291092 + 35999.0502909 * T - 0.0001536 * T2 + T3 / 24490000.0;
		Result.MoonM = 134.9633964 + 477198.8675055 * T + 0.0087414 * T2 + T3 / 69699.0 - T4 / 14712000.0;
		Result.F = 93.2720950 + 483202.0175233 * T - 0.0036539 * T2 - T3 / 3526000.0 + T4 / 863310000.0;
		Result.A1 = 119.75 + 131.849 * T;
		Result.A2 = 53.09 + 479264.290 * T;
		Result.A3 = 313.45 + 481266.484 * T;
		Result.E = 1.0 - 0.002516 * T - 0.0000074 * T2;
		return Result;
	}

	double EccentricityFactor(const FArguments& Arguments, int32 MultipleOfM)
	{
		return MultipleOfM == 0 ? 1.0 : (FMath::Abs(MultipleOfM) == 1 ? Arguments.E : Arguments.E * Arguments.E);
	}

	/** Sigma l and Sigma r of Meeus ch. 47, with the additive terms for Venus, Jupiter and the Earth's flattening. */
	void LongitudeAndDistanceSums(const FArguments& A, double& OutLongitude, double& OutDistance)
	{
		OutLongitude = 0.0;
		OutDistance = 0.0;
		for (const FLongitudeTerm& Term : LongitudeTerms)
		{
			const double Argument = Term.D * A.D + Term.M * A.M + Term.MoonM * A.MoonM + Term.F * A.F;
			const double Factor = EccentricityFactor(A, Term.M);
			OutLongitude += Term.Longitude * Factor * SinDegrees(Argument);
			OutDistance += Term.Distance * Factor * CosDegrees(Argument);
		}
		OutLongitude += 3958.0 * SinDegrees(A.A1) + 1962.0 * SinDegrees(A.MeanLongitude - A.F) + 318.0 * SinDegrees(A.A2);
	}

	/** Sigma b of Meeus ch. 47, with its additive terms. */
	double LatitudeSum(const FArguments& A)
	{
		double Sum = 0.0;
		for (const FLatitudeTerm& Term : LatitudeTerms)
		{
			const double Argument = Term.D * A.D + Term.M * A.M + Term.MoonM * A.MoonM + Term.F * A.F;
			Sum += Term.Latitude * EccentricityFactor(A, Term.M) * SinDegrees(Argument);
		}
		return Sum - 2235.0 * SinDegrees(A.MeanLongitude) + 382.0 * SinDegrees(A.A3) + 175.0 * SinDegrees(A.A1 - A.F)
			+ 175.0 * SinDegrees(A.A1 + A.F) + 127.0 * SinDegrees(A.MeanLongitude - A.MoonM)
			- 115.0 * SinDegrees(A.MeanLongitude + A.MoonM);
	}
}

namespace Musee::Time
{
	double FLunarCoordinates::SemidiameterDegrees() const
	{
		return LunarTerms::SemidiameterArcsecondKilometers / DistanceKilometers / ArcsecondsPerDegree;
	}

	FLunarCoordinates LunarCoordinates(double JulianEphemerisDay)
	{
		using namespace LunarTerms;
		const double T = (JulianEphemerisDay - J2000JulianDay) / DaysPerJulianCentury;
		const FArguments A = Arguments(T);
		double LongitudeTotal = 0.0;
		double DistanceTotal = 0.0;
		LongitudeAndDistanceSums(A, LongitudeTotal, DistanceTotal);

		FLunarCoordinates Result;
		Result.Nutation = Nutation(JulianEphemerisDay);
		Result.LongitudeDegrees = ReduceDegrees(A.MeanLongitude + LongitudeTotal / MicrodegreesPerDegree);
		Result.LatitudeDegrees = LatitudeSum(A) / MicrodegreesPerDegree;
		Result.DistanceKilometers = MeanDistanceKilometers + DistanceTotal / MetersPerKilometer;
		Result.HorizontalParallaxDegrees = AsinDegrees(EarthRadiusKilometers / Result.DistanceKilometers);
		Result.ApparentLongitudeDegrees = ReduceDegrees(Result.LongitudeDegrees + Result.Nutation.LongitudeDegrees);

		const FEquatorialPlace Place = EclipticToEquatorial(
			Result.ApparentLongitudeDegrees, Result.LatitudeDegrees, Result.Nutation.TrueObliquityDegrees());
		Result.RightAscensionDegrees = Place.RightAscensionDegrees;
		Result.DeclinationDegrees = Place.DeclinationDegrees;
		return Result;
	}
}
