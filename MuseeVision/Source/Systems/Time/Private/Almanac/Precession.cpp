#include "Almanac/Precession.h"

#include "Almanac/AstronomicalTime.h"
#include "Almanac/CelestialAngles.h"
#include "Almanac/Nutation.h"

namespace Musee::Time::PrecessionRotations
{
	/** A 3x3 rotation acting on column vectors, in the IAU convention where R1, R2 and R3 turn the frame about x, y
	 *  and z (Explanatory Supplement to the Astronomical Almanac, 1992, sec. 11.4). */
	struct FFrameRotation
	{
		double M[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};

		FFrameRotation operator*(const FFrameRotation& Other) const
		{
			FFrameRotation Product;
			for (int32 Row = 0; Row < 3; ++Row)
			{
				for (int32 Column = 0; Column < 3; ++Column)
				{
					Product.M[Row][Column] = M[Row][0] * Other.M[0][Column] + M[Row][1] * Other.M[1][Column]
						+ M[Row][2] * Other.M[2][Column];
				}
			}
			return Product;
		}

		FVector Apply(const FVector& V) const
		{
			return FVector(M[0][0] * V.X + M[0][1] * V.Y + M[0][2] * V.Z, M[1][0] * V.X + M[1][1] * V.Y + M[1][2] * V.Z,
				M[2][0] * V.X + M[2][1] * V.Y + M[2][2] * V.Z);
		}
	};

	FFrameRotation AboutX(double Degrees)
	{
		const double C = CosDegrees(Degrees);
		const double S = SinDegrees(Degrees);
		FFrameRotation R;
		R.M[1][1] = C;
		R.M[1][2] = S;
		R.M[2][1] = -S;
		R.M[2][2] = C;
		return R;
	}

	FFrameRotation AboutY(double Degrees)
	{
		const double C = CosDegrees(Degrees);
		const double S = SinDegrees(Degrees);
		FFrameRotation R;
		R.M[0][0] = C;
		R.M[0][2] = -S;
		R.M[2][0] = S;
		R.M[2][2] = C;
		return R;
	}

	FFrameRotation AboutZ(double Degrees)
	{
		const double C = CosDegrees(Degrees);
		const double S = SinDegrees(Degrees);
		FFrameRotation R;
		R.M[0][0] = C;
		R.M[0][1] = S;
		R.M[1][0] = -S;
		R.M[1][1] = C;
		return R;
	}

	/** IAU 1976 precession from J2000.0 to T Julian centuries later: zeta, z and theta in arcseconds (Meeus eq. 21.3
	 *  with the starting epoch at J2000.0), composed as R3(-z) R2(theta) R3(-zeta). */
	FFrameRotation PrecessionFromJ2000(double T)
	{
		const double T2 = T * T;
		const double T3 = T2 * T;
		const double Zeta = (2306.2181 * T + 0.30188 * T2 + 0.017998 * T3) / ArcsecondsPerDegree;
		const double Z = (2306.2181 * T + 1.09468 * T2 + 0.018203 * T3) / ArcsecondsPerDegree;
		const double Theta = (2004.3109 * T - 0.42665 * T2 - 0.041833 * T3) / ArcsecondsPerDegree;
		return AboutZ(-Z) * AboutY(Theta) * AboutZ(-Zeta);
	}
}

namespace Musee::Time
{
	FMatrix TrueEquatorOfDateFromJ2000(double JulianEphemerisDay, const FNutation& Nutation)
	{
		using namespace PrecessionRotations;
		const double T = (JulianEphemerisDay - J2000JulianDay) / DaysPerJulianCentury;
		const FFrameRotation Nutating = AboutX(-Nutation.TrueObliquityDegrees()) * AboutZ(-Nutation.LongitudeDegrees)
			* AboutX(Nutation.MeanObliquityDegrees);
		const FFrameRotation Carry = Nutating * PrecessionFromJ2000(T);
		return FMatrix(Carry.Apply(FVector::XAxisVector), Carry.Apply(FVector::YAxisVector), Carry.Apply(FVector::ZAxisVector),
			FVector::ZeroVector);
	}
}
