#include "RotundaStairLine.h"

#include "RotundaPlan.h"

namespace Musee::Rotunda::Stair
{
	namespace
	{
		/** Two slopes of the pitch line that differ by less than this (metres a degree) are one: the flights' knots are in line. */
		constexpr double SameSlope = 1.0e-9;

		/** The pitch line's knots, (φ, z), falling in φ as the stair descends. */
		const TArray<FVector2D>& PitchKnots()
		{
			static const TArray<FVector2D> Knots = []
			{
				const double HalfGoing = 0.5 * Plan::TreadDeg;
				TArray<FVector2D> Out;
				Out.Add(FVector2D(Plan::TopLandingBackDeg, TreadTop(1)));
				Out.Add(FVector2D(Front(1) + HalfGoing, TreadTop(1)));
				for (int32 K = 2; K < Plan::Risers; ++K)
				{
					if (K == Plan::MidLandingTread)
					{
						Out.Add(FVector2D(Front(K - 1) - HalfGoing, TreadTop(K)));
						Out.Add(FVector2D(Front(K) + HalfGoing, TreadTop(K)));
					}
					else
					{
						Out.Add(FVector2D(0.5 * (Front(K) + Front(K - 1)), TreadTop(K)));
					}
				}
				Out.Add(FVector2D(Front(Plan::Risers - 1) - HalfGoing, Plan::LandingZ));
				return Out;
			}();
			return Knots;
		}

		/** Where the pitch line passes a height between two knots, added to the list. */
		void Crossings(double Level, TArray<double>& Out)
		{
			const TArray<FVector2D>& Knots = PitchKnots();
			for (int32 Index = 1; Index < Knots.Num(); ++Index)
			{
				const FVector2D& A = Knots[Index - 1];
				const FVector2D& B = Knots[Index];
				if ((A.Y - Level) * (B.Y - Level) < 0.0)
				{
					Out.Add(FMath::Lerp(A.X, B.X, (Level - A.Y) / (B.Y - A.Y)));
				}
			}
		}
	}

	double Front(int32 K)
	{
		if (K <= 0)
		{
			return Plan::TopLandingBackDeg;
		}
		if (K < Plan::MidLandingTread)
		{
			return Plan::FirstNosingDeg - (K - 1) * Plan::TreadDeg;
		}
		return Front(Plan::MidLandingTread - 1) - Plan::MidLandingDeg - (K - Plan::MidLandingTread) * Plan::TreadDeg;
	}

	double TreadTop(int32 K)
	{
		return -Plan::Riser * K;
	}

	double Pitch(double Deg)
	{
		const TArray<FVector2D>& Knots = PitchKnots();
		if (Deg >= Knots[0].X)
		{
			return Knots[0].Y;
		}
		for (int32 Index = 1; Index < Knots.Num(); ++Index)
		{
			if (Deg >= Knots[Index].X)
			{
				const FVector2D& A = Knots[Index - 1];
				const FVector2D& B = Knots[Index];
				return FMath::Lerp(A.Y, B.Y, (Deg - A.X) / (B.X - A.X));
			}
		}
		return Knots.Last().Y;
	}

	double Soffit(double Deg)
	{
		return FMath::Max(Pitch(Deg) - Plan::SoffitUnderPitch, Plan::SoffitFloor);
	}

	double StringTop(double Deg)
	{
		return FMath::Min(Pitch(Deg) + Plan::StringOverPitch, Plan::StringCeiling);
	}

	TArray<double> Kinks()
	{
		const TArray<FVector2D>& Knots = PitchKnots();
		TArray<double> Out;
		for (int32 Index = 1; Index + 1 < Knots.Num(); ++Index)
		{
			const double In = (Knots[Index].Y - Knots[Index - 1].Y) / (Knots[Index].X - Knots[Index - 1].X);
			const double Leaving = (Knots[Index + 1].Y - Knots[Index].Y) / (Knots[Index + 1].X - Knots[Index].X);
			if (FMath::Abs(In - Leaving) > SameSlope)
			{
				Out.Add(Knots[Index].X);
			}
		}
		Out.Add(Knots[0].X);
		Out.Add(Knots.Last().X);
		Crossings(Plan::SoffitFloor + Plan::SoffitUnderPitch, Out);
		Crossings(Plan::StringCeiling - Plan::StringOverPitch, Out);
		Out.Sort();
		return Out;
	}

	double AngleOf(double X, double Y, double NearDeg)
	{
		double Deg = FMath::RadiansToDegrees(FMath::Atan2(Y, X));
		while (Deg < NearDeg - 180.0)
		{
			Deg += 360.0;
		}
		while (Deg > NearDeg + 180.0)
		{
			Deg -= 360.0;
		}
		return Deg;
	}

	FFormGround Cylinder(double Radius)
	{
		// Its meridian drawn up the cylinder, so W points away from the axis and V is the height.
		return FFormGround::Revolved(FFormProfile::Polyline({FVector2D(Radius, 0.0), FVector2D(Radius, 1.0)}, false), Radius);
	}

	double Around(double Radius, double Deg)
	{
		return Radius * FMath::DegreesToRadians(Deg);
	}

	TArray<FVector2D> LineOver(double Radius, double FromDeg, double ToDeg, TFunctionRef<double(double)> Height, double Over)
	{
		TArray<FVector2D> Line;
		auto Add = [&](double Deg) { Line.Add(FVector2D(Around(Radius, Deg), Height(Deg) + Over)); };
		Add(FromDeg);
		for (const double Kink : Kinks())
		{
			if (Kink > FromDeg + UE_KINDA_SMALL_NUMBER && Kink < ToDeg - UE_KINDA_SMALL_NUMBER)
			{
				Add(Kink);
			}
		}
		Add(ToDeg);
		return Line;
	}
}
