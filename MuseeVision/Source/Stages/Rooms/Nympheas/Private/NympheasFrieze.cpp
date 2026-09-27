#include "NympheasFrieze.h"

/**
 * The frieze's drawing. Its dimensions are the carver's: petals, pads and stems in metres on the band, the as-built
 * design (SalonPond.cpp:168-441), reviewed for large motifs, deep relief (40 to 55 mm) and one flowing composition.
 */
namespace Musee::Nympheas
{
	namespace
	{
		/** The keystone motif's length at each apex, and the repeat's length it is fitted to between them. */
		constexpr double KeystoneLength = 0.36;
		constexpr double RepeatLength = 1.0;
		/** The bins along u that index the shapes, and how far a shape may reach before the half's start. */
		constexpr double BinLength = 0.05;
		constexpr double BinMargin = 0.3;
		/** The carving fades to the ground within 4 mm of the band's foot and top (§3.5). */
		constexpr double EdgeFade = 0.004;
		/** A curve's stations when a stem's cubic is set out. */
		constexpr int32 StemStations = 24;

		/** A rounded shoulder: the carving rises to full height within R of its edge. */
		double Shoulder(double Inside, double R)
		{
			const double T = FMath::Clamp(Inside / R, 0.0, 1.0);
			return FMath::Sqrt(1.0 - (1.0 - T) * (1.0 - T));
		}

		double WrapAngle(double A) { return FMath::Abs(FMath::UnwindRadians(A)); }

		/** The carver's small irregularity of repeat K: a fixed number in [0, 1) for each (K, Salt). */
		double Jitter(int32 K, int32 Salt)
		{
			const double X = FMath::Sin(K * 12.9898 + Salt * 78.233) * 43758.5453;
			return X - FMath::FloorToDouble(X);
		}

		/**
		 * A floating pad seen low: a thin leaf carved as a broad plate, its face flat to faintly dished, its rim lifted, its
		 * edge cut square, its radial veins sunk, a gentle ripple round it, the slit open to the ground.
		 */
		double PadHeight(double H, double A, double B, double Notch, double Rot, double Phase, double U, double V)
		{
			const double C = FMath::Cos(Rot);
			const double S = FMath::Sin(Rot);
			const double RX = C * U + S * V;
			const double RY = -S * U + C * V;
			const double Q = FMath::Sqrt(FMath::Square(RX / A) + FMath::Square(RY / B));
			if (Q >= 1.0)
			{
				return 0.0;
			}
			const double Inside = (1.0 - Q) * B;
			const double Angle = FMath::Atan2(RY / B, RX / A);
			const double FromSlit = WrapAngle(Angle - Notch);
			const double SlitHalf = 0.16 * Q + 0.015;
			if (Q > 0.12 && FromSlit < SlitHalf)
			{
				return 0.0;
			}
			const double SlitEdge = FMath::Clamp((FromSlit - SlitHalf) * Q * B / 0.003, 0.0, 1.0);
			const double Edge = Q > 0.12 ? FMath::Min(1.0, 0.25 + SlitEdge) : 1.0;
			const double Dish = 0.93 + 0.07 * Q * Q;
			const double Ripple = 0.035 * Q * FMath::Sin(3.0 * Angle + 7.0 * Rot + Phase);
			const double Rim = 0.09 * FMath::Exp(-FMath::Square((1.0 - Q) * B / 0.0045));
			const double Veins = -0.075 * FMath::Exp(-FMath::Square(WrapAngle(Angle * 11.0) / 0.16)) * FMath::Clamp((Q - 0.1) / 0.3, 0.0, 1.0)
				* FMath::Clamp((0.97 - Q) / 0.1, 0.0, 1.0);
			const double Navel = -0.05 * FMath::Exp(-FMath::Square(Q / 0.07));
			return H * Shoulder(Inside, 0.0015) * (Dish + Ripple + Rim + Veins + Navel) * Edge;
		}
	}

	double FFriezeDesign::FShape::Height(double U, double V) const
	{
		switch (Kind)
		{
		case EKind::Pad:
			// Each pad's ripple starts where it lies along the band, so no two ripple alike.
			return PadHeight(H, A, B, Angle, Rot, 40.0 * X, U - X, V - Y);
		case EKind::Petal:
		{
			// A pointed petal from its base along Angle, length A, width B: domed across, a groove down its middle.
			const double DX = FMath::Cos(Angle);
			const double DY = FMath::Sin(Angle);
			const double Along = ((U - X) * DX + (V - Y) * DY) / A;
			if (Along <= 0.0 || Along >= 1.0)
			{
				return 0.0;
			}
			const double Lateral = -(U - X) * DY + (V - Y) * DX;
			const double HalfWidth = 0.5 * B * FMath::Pow(FMath::Sin(UE_DOUBLE_PI * FMath::Pow(Along, 0.62)), Fat);
			const double Inside = FMath::Min(HalfWidth - FMath::Abs(Lateral), FMath::Min(Along, 1.0 - Along) * A * 0.5);
			if (Inside <= 0.0)
			{
				return 0.0;
			}
			const double Dome = FMath::Sqrt(FMath::Clamp(1.0 - FMath::Square(Lateral / FMath::Max(HalfWidth, 1e-4)), 0.0, 1.0));
			const double Profile = Base + (1.0 - Base) * FMath::Sin(UE_DOUBLE_PI * FMath::Clamp(Along * 0.9 + 0.1, 0.0, 1.0));
			const double Groove = (bGroove && Along > 0.15) ? 1.0 - 0.18 * FMath::Exp(-FMath::Square(Lateral / 0.0025)) : 1.0;
			return H * Shoulder(Inside, 0.003) * (0.6 + 0.4 * Dome) * Profile * Groove;
		}
		case EKind::Stem:
		{
			double Nearest = TNumericLimits<double>::Max();
			double Fraction = 0.0;
			const int32 Spans = Points.Num() - 1;
			for (int32 Index = 0; Index < Spans; ++Index)
			{
				const FVector2D From = Points[Index];
				const FVector2D Span = Points[Index + 1] - From;
				const double T = FMath::Clamp(((U - From.X) * Span.X + (V - From.Y) * Span.Y) / FMath::Max(1e-12, Span.SizeSquared()), 0.0, 1.0);
				const double Distance = FVector2D::Distance(FVector2D(U, V), From + Span * T);
				if (Distance < Nearest)
				{
					Nearest = Distance;
					Fraction = (Index + T) / Spans;
				}
			}
			const double Width = FMath::Lerp(W0, W1, Fraction);
			return Nearest >= Width ? 0.0 : H * FMath::Sqrt(1.0 - FMath::Square(Nearest / Width));
		}
		default:
		{
			const double Distance = FVector2D::Distance(FVector2D(U, V), FVector2D(X, Y));
			return Distance >= A ? 0.0 : H * FMath::Sqrt(1.0 - FMath::Square(Distance / A));
		}
		}
	}

	void FFriezeDesign::AddPad(double X, double Y, double A, double B, double H, double Notch, double Rot)
	{
		FShape& S = Shapes.AddDefaulted_GetRef();
		S.Kind = FShape::EKind::Pad;
		S.X = X; S.Y = Y; S.A = A; S.B = B; S.H = H; S.Angle = Notch; S.Rot = Rot;
		const double R = FMath::Max(A, B);
		S.Box = FBox2D(FVector2D(X - R, Y - R), FVector2D(X + R, Y + R));
	}

	void FFriezeDesign::AddPetal(double X, double Y, double Angle, double Length, double Width, double H, bool bGroove, double Fat, double Base)
	{
		FShape& S = Shapes.AddDefaulted_GetRef();
		S.Kind = FShape::EKind::Petal;
		S.X = X; S.Y = Y; S.Angle = Angle; S.A = Length; S.B = Width; S.H = H; S.bGroove = bGroove; S.Fat = Fat; S.Base = Base;
		const FVector2D Tip(X + Length * FMath::Cos(Angle), Y + Length * FMath::Sin(Angle));
		S.Box = FBox2D(FVector2D(FMath::Min(X, Tip.X) - Width, FMath::Min(Y, Tip.Y) - Width),
			FVector2D(FMath::Max(X, Tip.X) + Width, FMath::Max(Y, Tip.Y) + Width));
	}

	void FFriezeDesign::AddBump(double X, double Y, double Radius, double H)
	{
		FShape& S = Shapes.AddDefaulted_GetRef();
		S.X = X; S.Y = Y; S.A = Radius; S.H = H;
		S.Box = FBox2D(FVector2D(X - Radius, Y - Radius), FVector2D(X + Radius, Y + Radius));
	}

	void FFriezeDesign::AddStem(const FVector2D& P0, const FVector2D& P1, const FVector2D& P2, const FVector2D& P3, double W0, double W1, double H)
	{
		FShape& S = Shapes.AddDefaulted_GetRef();
		S.Kind = FShape::EKind::Stem;
		S.W0 = W0; S.W1 = W1; S.H = H;
		for (int32 Index = 0; Index <= StemStations; ++Index)
		{
			const double T = double(Index) / StemStations;
			const double M = 1.0 - T;
			const FVector2D P = P0 * (M * M * M) + P1 * (3.0 * M * M * T) + P2 * (3.0 * M * T * T) + P3 * (T * T * T);
			S.Points.Add(P);
			S.Box += P;
		}
		S.Box = S.Box.ExpandBy(FMath::Max(W0, W1));
	}

	/** An open Nymphaea from the side: a back row of petals, the front row fanning over it, two sepals, the receptacle. */
	void FFriezeDesign::FlowerSide(double CX, double CY, double Size, double H, double Lean, int32 K, bool bHalfOpen)
	{
		const TArray<double> Back = bHalfOpen ? TArray<double>{-30, -10, 10, 30} : TArray<double>{-58, -34, -11, 11, 34, 58};
		for (const double A : Back)
		{
			AddPetal(CX, CY + 0.006, FMath::DegreesToRadians(90.0 + A + Lean), Size * 0.92, Size * 0.36, H * 0.62, false, 0.6);
		}
		const TArray<double> Front = bHalfOpen ? TArray<double>{-40, -20, 0, 20, 40} : TArray<double>{-70, -46, -23, 0, 23, 46, 70};
		for (int32 Index = 0; Index < Front.Num(); ++Index)
		{
			const double A = Front[Index];
			const double Facing = 1.0 - FMath::Abs(A) / 120.0;
			const double Length = Size * (0.8 + 0.2 * FMath::Cos(FMath::DegreesToRadians(A))) * (0.96 + 0.08 * Jitter(K * 7 + Index, 3));
			AddPetal(CX, CY, FMath::DegreesToRadians(90.0 + A + Lean), Length, Size * (bHalfOpen ? 0.40 : 0.44), H * (0.78 + 0.22 * Facing), true, 0.62);
		}
		for (const double A : {-116.0, 116.0})
		{
			AddPetal(CX, CY - 0.006, FMath::DegreesToRadians(90.0 + A + Lean), Size * 0.66, Size * 0.32, H * 0.7);
		}
		AddBump(CX, CY - 0.008, Size * 0.22, H * 0.72);
	}

	void FFriezeDesign::Bud(double X, double Y, double Angle, double Length, double Width, double H)
	{
		AddPetal(X, Y, Angle, Length, Width, H, false, 0.5, 0.75);
		for (const double Off : {-0.2, 0.2})
		{
			AddPetal(X, Y, Angle + Off, Length * 0.7, Width * 0.5, H * 1.06, false);
		}
	}

	/** The lily from the front at an end of the long axis: three rings of petals, cupped, the stamen crown; a pad and a bud beside. */
	void FFriezeDesign::Keystone(double CX, bool bMirror)
	{
		const double CY = 0.1;
		const int32 Side = bMirror ? -1 : 1;
		AddPad(CX + Side * 0.2, 0.058, 0.125, 0.046, 0.047, -UE_DOUBLE_HALF_PI + Side * 0.6, Side * 0.06);
		AddStem(FVector2D(CX + Side * 0.07, 0.03), FVector2D(CX + Side * 0.13, 0.05), FVector2D(CX + Side * 0.12, 0.13),
			FVector2D(CX + Side * 0.17, 0.155), 0.0065, 0.0055, 0.028);
		Bud(CX + Side * 0.17, 0.155, FMath::DegreesToRadians(90.0 - Side * 30.0), 0.06, 0.024, 0.04);
		const double Turn = 2.0 * UE_DOUBLE_PI;
		for (int32 Index = 0; Index < 12; ++Index) { AddPetal(CX, CY, Turn * (Index + 0.5) / 12, 0.094, 0.042, 0.03, true, 0.55, 0.8); }
		for (int32 Index = 0; Index < 10; ++Index) { AddPetal(CX, CY, Turn * Index / 10, 0.072, 0.036, 0.042, true, 0.55, 0.85); }
		for (int32 Index = 0; Index < 8; ++Index) { AddPetal(CX, CY, Turn * (Index + 0.5) / 8, 0.048, 0.03, 0.05, true, 0.55, 0.9); }
		AddBump(CX, CY, 0.024, 0.05);
		for (int32 Index = 0; Index < 16; ++Index)
		{
			const double A = Turn * Index / 16;
			AddBump(CX + 0.018 * FMath::Cos(A), CY + 0.018 * FMath::Sin(A), 0.006, 0.056);
		}
		for (int32 Index = 0; Index < 6; ++Index)
		{
			const double A = Turn * Index / 6 + 0.5;
			AddBump(CX + 0.007 * FMath::Cos(A), CY + 0.007 * FMath::Sin(A), 0.0045, 0.056);
		}
	}

	/** One repeat: the whiplash rhizome, its bud, three pads (the third lifted on its stalk) and the open lily on its stem. */
	void FFriezeDesign::Repeat(double X0, int32 K)
	{
		auto J = [K](int32 Salt) { return Jitter(K, Salt) - 0.5; };
		auto P = [X0](double X, double Y) { return FVector2D(X0 + X, Y); };
		const double L = Lambda;
		const bool bHalfOpen = (K % 3) == 2;
		AddStem(P(0.0, 0.03), P(0.25, 0.0), P(0.3, 0.15), P(0.5, 0.15), 0.0085, 0.0085, 0.02);
		AddStem(P(0.5, 0.15), P(0.7, 0.15), P(L - 0.25, 0.06), P(L, 0.03), 0.0085, 0.0085, 0.02);
		AddStem(P(0.36, 0.14), P(0.33, 0.155), P(0.3, 0.165), P(0.27, 0.17), 0.0062, 0.0055, 0.03);
		Bud(X0 + 0.27, 0.17, FMath::DegreesToRadians(150.0 + 8.0 * J(12)), 0.05, 0.021, 0.04);
		AddPad(X0 + 0.15 + 0.02 * J(1), 0.062, 0.125, 0.047, 0.046, -UE_DOUBLE_HALF_PI + 0.5 + 0.3 * J(2), 0.04 * J(3));
		AddPad(X0 + 0.36 + 0.02 * J(4), 0.046, 0.108, 0.041, 0.055, -UE_DOUBLE_HALF_PI - 0.5 + 0.3 * J(5), -0.05);
		AddStem(P(0.78, 0.04), P(0.8, 0.06), P(0.82, 0.08), P(0.84, 0.095), 0.0065, 0.006, 0.03);
		AddPad(X0 + 0.87 + 0.02 * J(6), 0.1, 0.1, 0.042, 0.044, -UE_DOUBLE_HALF_PI + 0.8, 0.28 + 0.06 * J(11));
		const double FX = 0.6 + 0.02 * J(7);
		const double FY = 0.092;
		AddStem(P(0.5, 0.004), P(0.57, 0.02), P(0.555, 0.06), P(FX, FY - 0.01), 0.0072, 0.0065, 0.032);
		FlowerSide(X0 + FX, FY, bHalfOpen ? 0.074 : 0.084, 0.052, -5.0 + 6.0 * J(8), K, bHalfOpen);
	}

	void FFriezeDesign::Index()
	{
		const int32 Count = FMath::CeilToInt32((Half + 2.0 * BinMargin) / BinLength) + 1;
		Bins.SetNum(Count);
		for (int32 Shape = 0; Shape < Shapes.Num(); ++Shape)
		{
			const int32 First = FMath::Clamp(FMath::FloorToInt32((Shapes[Shape].Box.Min.X + BinMargin) / BinLength), 0, Count - 1);
			const int32 Last = FMath::Clamp(FMath::FloorToInt32((Shapes[Shape].Box.Max.X + BinMargin) / BinLength), 0, Count - 1);
			for (int32 Bin = First; Bin <= Last; ++Bin)
			{
				Bins[Bin].Add(Shape);
			}
		}
	}

	FFriezeDesign::FFriezeDesign(double HalfMeters, double BandMeters)
		: Half(HalfMeters), Band(BandMeters)
	{
		Keystone(0.0, false);
		Keystone(Half, true);
		const double Run = Half - 2.0 * KeystoneLength;
		const int32 Repeats = FMath::Max(1, FMath::RoundToInt32(Run / RepeatLength));
		Lambda = Run / Repeats;
		for (int32 K = 0; K < Repeats; ++K)
		{
			Repeat(KeystoneLength + K * Lambda, K);
		}
		Index();
	}

	double FFriezeDesign::Height(double U, double V) const
	{
		double H = 0.0;
		const int32 Bin = FMath::Clamp(FMath::FloorToInt32((U + BinMargin) / BinLength), 0, Bins.Num() - 1);
		for (const int32 Shape : Bins[Bin])
		{
			const FShape& S = Shapes[Shape];
			if (U >= S.Box.Min.X && U <= S.Box.Max.X && V >= S.Box.Min.Y && V <= S.Box.Max.Y)
			{
				H = FMath::Max(H, S.Height(U, V));
			}
		}
		return H * FMath::Clamp(FMath::Min(V, Band - V) / EdgeFade, 0.0, 1.0);
	}
}
