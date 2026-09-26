#include "SunClock/SunClockBuild.h"

/**
 * The dial (the drum's top) and the fixed ring round it.
 *
 * The dial: polished pale marble in concentric bands of two tones, a thin band of rosso round the
 * sun and one of nero inside the hour band, each edged with a bronze fillet. The hour ring lies on
 * the circle where the node's point of shadow falls (r 6.78); inside it, fine bronze ticks for every
 * minute from VI (west) through XII (north) to VI (east), longer at 5, 15 and 30 minutes; the hour
 * lines run in from it to the rosso band; the numerals stand just outside it, tops outward, read
 * from the dial. At the centre, a sunburst in relief: 16 long straight rays and 16 short flames, each
 * with a raised ridge and hollow flanks, round a domed gilt boss with a fine ring and a beaded foot.
 * No meridian.
 *
 * The ring: pale marble slabs of two tones in three courses (48, 32 and 64 to the round), and in the
 * middle course the motto, HORAS · NON · NUMERO · NISI · SERENAS, in bronze capitals 26 cm high
 * with gilt triangular stops, centred on the south and read from the ring, facing the dial.
 */
namespace SunClockBuild
{
	// The dial's bands (radius, material): A, rosso, B, A, B, nero, A.
	constexpr double DialRosso0 = 1.40, DialRosso1 = 1.52;
	constexpr double DialNero0 = 6.30, DialNero1 = 6.38;
	constexpr double HourRing0 = 6.76, HourRing1 = 6.80;      // centred on the shadow's circle (6.78)
	constexpr double NumeralBase = 6.815, NumeralCap = 0.16;
	constexpr double HourLineFrom = 1.60;
	constexpr double TickTo = 6.77;                           // into the hour ring

	// The motto.
	constexpr double MottoBase = 8.53, MottoCap = 0.26;       // the baseline outward: tops towards the dial
	constexpr double RingCourse0 = 7.75, RingCourse1 = 8.95;

	static void DialTop(FSunClockMeshes& M)
	{
		const TArray<double> Grid = GridAngles();
		TArray<double> Rim;
		for (const FStation& S : DrumStations()) { Rim.Add(S.Phi); }
		struct FBand { double R0, R1; FMeshData* Mesh; };
		const FBand Bands[] = {
			{0.0, DialRosso0, &M.DialA},
			{DialRosso0, DialRosso1, &M.DialRosso},
			{DialRosso1, 3.20, &M.DialB},
			{3.20, 4.90, &M.DialA},
			{4.90, DialNero0, &M.DialB},
			{DialNero0, DialNero1, &M.DialNero},
			{DialNero1, SC::DrumOuter, &M.DialA},
		};
		for (const FBand& B : Bands)
		{
			const bool bRim = B.R1 >= SC::DrumOuter - 1e-9;
			Annulus(*B.Mesh, B.R0, Grid, B.R1, bRim ? Rim : Grid, 0.0, true);
		}
	}

	static void DialInlay(FSunClockMeshes& M)
	{
		const TArray<double> Grid = GridAngles();
		FMeshData& Bz = M.DrumBronze;
		// The hour ring (3 mm proud) and the fillets edging the rosso and nero bands (2 mm).
		InlayRing(Bz, HourRing0, HourRing1, 0.0, 0.003, 0.0015, 0.002, Grid);
		for (const double R : {DialRosso0, DialRosso1, DialNero0, DialNero1})
		{
			InlayRing(Bz, R - 0.005, R + 0.005, 0.0, 0.002, 0.0008, 0.002, Grid);
		}
		// The hour lines, VI (west) … XII (north) … VI (east), in from the hour ring to the rosso band.
		for (int32 K = 0; K <= 12; ++K)
		{
			const double Phi = UE_DOUBLE_PI + K * UE_DOUBLE_PI / 12;
			const FVector2D D(FMath::Cos(Phi), FMath::Sin(Phi));
			InlayBar(Bz, D * HourLineFrom, D * TickTo, 0.0, 0.024, 0.0025, 0.001, 0.002);
		}
		// Minute ticks: the shadow moves 3 cm a minute on the hour ring.
		for (int32 Minute = 1; Minute < 12 * 60; ++Minute)
		{
			if (Minute % 60 == 0) { continue; }
			double From = 6.665, Width = 0.003, Height = 0.002, Bevel = 0.0006;
			if (Minute % 30 == 0) { From = 6.52; Width = 0.010; Height = 0.0025; Bevel = 0.001; }
			else if (Minute % 15 == 0) { From = 6.58; Width = 0.007; Height = 0.0025; Bevel = 0.001; }
			else if (Minute % 5 == 0) { From = 6.62; Width = 0.005; Height = 0.0022; Bevel = 0.0008; }
			const double Phi = UE_DOUBLE_PI + Minute * UE_DOUBLE_PI / 720;
			const FVector2D D(FMath::Cos(Phi), FMath::Sin(Phi));
			InlayBar(Bz, D * From, D * TickTo, 0.0, Width, Height, Bevel, 0.002);
		}
	}

	static void DialNumerals(FSunClockMeshes& M)
	{
		static const TCHAR* Hours[13] = {TEXT("VI"), TEXT("VII"), TEXT("VIII"), TEXT("IX"), TEXT("X"), TEXT("XI"), TEXT("XII"),
										 TEXT("I"), TEXT("II"), TEXT("III"), TEXT("IV"), TEXT("V"), TEXT("VI")};
		FTextStyle Style;
		Style.Tracking = -0.03;   // the serifs nearly touch, as on a dial
		const double Mid = NumeralBase + NumeralCap / 2;
		for (int32 K = 0; K <= 12; ++K)
		{
			const double HourPhi = UE_DOUBLE_PI + K * UE_DOUBLE_PI / 12;
			const FString Text(Hours[K]);
			const double Width = TextWidth(Text, Style);
			// Tops outward, read from the dial: glyph x runs with the hours (increasing φ), y outward.
			auto Place = [HourPhi, Width, Mid](double X, double Y)
			{
				const double R = NumeralBase + Y * NumeralCap;
				const double Phi = HourPhi + (X - Width / 2) * NumeralCap / Mid;
				return FVector2D(R * FMath::Cos(Phi), R * FMath::Sin(Phi));
			};
			AddText(M.DrumBronze, Text, Style, Place, NumeralCap, 0.0, 0.005, 0.002, 0.002, [](double) {});
		}
	}

	static void DialSun(FSunClockMeshes& M)
	{
		// The rays, as in a gloria: 16 long straight ones (one to XII) and 16 short flames between them,
		// each ridged along its crest with hollow flanks, rising from under the boss.
		constexpr int32 Rays = 32, Along = 24;
		for (int32 I = 0; I < Rays; ++I)
		{
			const bool bLong = I % 2 == 0;
			const double Phi = Turn * I / Rays;
			const FVector Dir = RadialDir(Phi), Side = TangentDir(Phi);
			const double R0 = 0.24, R1 = bLong ? 1.25 : 0.86;
			const double W0 = bLong ? 0.036 : 0.030, H0 = bLong ? 0.042 : 0.030;
			for (const double Sign : {1.0, -1.0})
			{
				auto Pos = [&](int32 U, int32 V) -> FVector
				{
					const double T = double(U) / Along;
					const double W = W0 * FMath::Pow(1.0 - T, bLong ? 0.62 : 0.75);
					const double H = 0.004 + (H0 - 0.004) * FMath::Pow(1.0 - T, 1.2);
					// A flame's crest weaves (one and a half waves, dying out at the tip).
					const double Weave = bLong ? 0.0 : 0.03 * FMath::Sin(3.0 * UE_DOUBLE_PI * T) * FMath::Sqrt(T) * (1.0 - T);
					// Across a flank: the foot, a hollow (lower than the straight line), the ridge.
					const double Across[3] = {1.0, 0.5, 0.0};
					const double Up[3] = {-0.002, 0.40 * H, H};
					return Dir * (R0 + (R1 - R0) * T) + Side * (Weave + Sign * W * Across[V]) + FVector(0, 0, Up[V]);
				};
				M.DrumBronze.Patch(Along, 2, Pos, [](const FVector& P) { return FVector2D(P.X, P.Y); },
								   [Side, Sign](const FVector&) { return (FVector::UpVector * 2 + Side * Sign).GetSafeNormal(); });
			}
		}
		// The boss: a vertical foot, a rounded shoulder, a low dome; a fine ring on it; a beaded foot.
		TArray<double> Coarse;
		for (int32 I = 0; I < Around; I += 4) { Coarse.Add(GridAngle(I)); }
		const double DomeR = 1.1854, DomeZ = 0.085 - 1.1854;
		auto Dome = [DomeR, DomeZ](double R) { return DomeZ + FMath::Sqrt(DomeR * DomeR - R * R); };
		FProfile Boss;
		Boss.Add(0.285, -0.002).Add(0.285, 0.035).Arc(0.265, 0.035, 0.02, 0.02, 0, 90, 6).SmoothLast();
		for (int32 I = 1; I <= 20; ++I)
		{
			const double R = 0.265 * (1.0 - double(I) / 20);
			Boss.Add(R, Dome(R), I < 20);
		}
		Revolve(M.DrumGilt, Boss, Coarse, true);
		auto Bead = [&Coarse](FMeshData& Out, double R, double Z, double A)
		{
			FProfile Ring;
			for (int32 I = 0; I < 16; ++I)
			{
				const double T = Turn * I / 16;
				Ring.Add(R + A * FMath::Cos(T), Z + A * FMath::Sin(T), true);
			}
			Ring.bClosed = true;
			Revolve(Out, Ring, Coarse, true);
		};
		Bead(M.DrumGilt, 0.19, Dome(0.19) + 0.001, 0.005);
		Bead(M.DrumGilt, 0.30, 0.012, 0.014);
	}

	void BuildDial(FSunClockMeshes& M)
	{
		DialTop(M);
		DialInlay(M);
		DialNumerals(M);
		DialSun(M);
	}

	void BuildRing(FSunClockMeshes& M)
	{
		// Three courses of slabs, alternating in tone (the outer course offset by half a slab). The outer
		// edge is the imported floor's 128-gon (r 10.3), which the passages' and the vestibule's floors
		// meet in the doors: every sixth station.
		struct FCourse { double R0, R1; int32 Slabs, Offset; };
		const FCourse Courses[] = {{SC::DialRadius, RingCourse0, 48, 0}, {RingCourse0, RingCourse1, 32, 0}, {RingCourse1, SC::RingOuter, 64, 6}};
		constexpr int32 OuterEvery = Around / 128;
		for (int32 C = 0; C < 3; ++C)
		{
			const FCourse& Co = Courses[C];
			const int32 Step = Around / Co.Slabs;
			for (int32 S = 0; S < Co.Slabs; ++S)
			{
				TArray<double> Arc, OuterArc;
				for (int32 I = 0; I <= Step; ++I)
				{
					const int32 G = Co.Offset + S * Step + I;
					Arc.Add(GridAngle(G));
					if (C < 2 || G % OuterEvery == 0) { OuterArc.Add(GridAngle(G)); }
				}
				FMeshData& Mesh = (S + C) % 2 == 0 ? M.RingA : M.RingB;
				Sector(Mesh, Co.R0, Arc, Co.R1, OuterArc, 0.0, true);
			}
		}
		// The motto, centred on the south, running west to east for a reader on the ring facing the dial.
		FTextStyle Style;
		Style.Tracking = 0.16;
		Style.StopGap = 1.15;
		const FString Motto(TEXT("HORAS.NON.NUMERO.NISI.SERENAS"));
		const double Width = TextWidth(Motto, Style);
		const double Mid = MottoBase - MottoCap / 2;
		auto Place = [Width, Mid](double X, double Y)
		{
			const double R = MottoBase - Y * MottoCap;
			const double Phi = UE_DOUBLE_PI / 2 + (Width / 2 - X) * MottoCap / Mid;
			return FVector2D(R * FMath::Cos(Phi), R * FMath::Sin(Phi));
		};
		auto Stop = [&M, &Place](double X)
		{
			// A triangular stop at mid-height, point down, 8 mm high.
			const FVector2D A = Place(X - 0.12, 0.60), B = Place(X + 0.12, 0.60), C = Place(X, 0.36);
			const FVector2D StopMid = (A + B + C) / 3;
			const FVector Apex(StopMid.X, StopMid.Y, 0.008);
			const FVector2D Corners[3] = {A, B, C};
			for (int32 I = 0; I < 3; ++I)
			{
				const FVector2D P = Corners[I], Q = Corners[(I + 1) % 3];
				const FVector P0(P.X, P.Y, -0.002), Q0(Q.X, Q.Y, -0.002), P1(P.X, P.Y, 0.0), Q1(Q.X, Q.Y, 0.0);
				FVector2D Out = ((P + Q) / 2 - StopMid).GetSafeNormal();
				const FVector N(Out.X, Out.Y, 0);
				M.RingGilt.Rect(P0, Q0, Q1, P1, N);
				const FVector Slope = FVector::CrossProduct(Q1 - P1, Apex - P1).GetSafeNormal();
				const FVector Face = FVector::DotProduct(Slope, FVector::UpVector) > 0 ? Slope : -Slope;
				M.RingGilt.Poly(TArray<FVector>({P1, Q1, Apex}), Face);
			}
		};
		AddText(M.RingBronze, Motto, Style, Place, MottoCap, 0.0, 0.008, 0.0025, 0.002, Stop);
	}
}
