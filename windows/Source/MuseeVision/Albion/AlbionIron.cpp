#include "Albion/AlbionBuild.h"

/**
 * Albion's ironwork and lead (AlbionBuild.h), as a Victorian ironfounder would have made it: clustered cast-iron shafts
 * on bolted shoes over the stone capitals; box girders on the arcade lines, riveted, carrying the lead valley gutters;
 * pointed arcade arches with wrought-iron rings in their spandrels; the vaults' ribs (built-up I-section main ribs on
 * the column lines, each with a lighter inner arch and a chain of rings between, and plain T ribs between them), purlins
 * and glazing bars; the end screens' mullions and transoms; saddle bars in the lancets; ridges, ventilators, flashings,
 * rainwater heads and downpipes.
 *
 * The glass lies on the vaults' arcs (AlbionBuild::VaultHalf); the iron hangs under it: glazing bars to 5 cm below the
 * glass, purlins to 15 cm, ribs below them.
 */
namespace AlbionIronImpl
{
	namespace AP = AlbionPlan;
	using namespace AlbionKit;
	using namespace AlbionBuild;

	const FVector kUp(0, 0, 1);
	const FVector kY(0, 1, 0);
	const FVector kX(1, 0, 0);

	constexpr double kBarDepth = 0.05, kPurlinDepth = 0.10;
	constexpr double kRibTop = kBarDepth + kPurlinDepth;          // 0.15 under the glass
	constexpr double kMainDepth = 0.26, kLightDepth = 0.14;
	constexpr double kInnerArch = 0.62;                           // the main ribs' inner arch, under the glass
	constexpr double kGlassY0 = AP::Y0 - AP::Wall * 0.5, kGlassY1 = AP::Y1 + AP::Wall * 0.5;   // the screens' planes

	/** Stations along a vault half from angle T0 to T1 (N steps) at plane y = Y: O on the glass arc, A its outward
	 *  normal, B along the building. */
	TArray<FStation> VaultStations(const FArcHalf& H, double Y, double T0, double T1, int32 N)
	{
		TArray<FStation> Out;
		for (int32 i = 0; i <= N; ++i)
		{
			const double T = FMath::Lerp(T0, T1, double(i) / N);
			const FVector2D P = H.At(T), Nr = H.Normal(T);
			Out.Add({FVector(P.X, Y, P.Y), FVector(Nr.X, 0.0, Nr.Y), kY});
		}
		return Out;
	}

	/** A hex bolt head (and washer) on a face at C, facing N. */
	void Bolt(FMeshData& M, const FVector& C, const FVector& N, double R = 0.016)
	{
		const FVector A = FVector::CrossProduct(N, FMath::Abs(N.Z) < 0.9 ? kUp : kX).GetSafeNormal();
		const FVector B = FVector::CrossProduct(N, A);
		TArray<FVector2D> Hex;
		for (int32 i = 0; i < 6; ++i) { Hex.Add(FVector2D(R * FMath::Cos(kPi * i / 3.0), R * FMath::Sin(kPi * i / 3.0))); }
		TArray<FStation> St = {{C - N * 0.002, A, B}, {C + N * 0.012, A, B}};
		SweepStations(M, St, Hex);
	}

	/** A rivet head: a low dome. */
	void Rivet(FMeshData& M, const FVector& C, const FVector& N, double R = 0.011)
	{
		const FVector A = FVector::CrossProduct(N, FMath::Abs(N.Z) < 0.9 ? kUp : kX).GetSafeNormal();
		const FVector B = FVector::CrossProduct(N, A);
		constexpr int32 Seg = 8, Rings = 2;
		const int32 Base = M.Positions.Num();
		for (int32 r = 0; r <= Rings; ++r)
		{
			const double T = 0.5 * kPi * r / Rings;
			for (int32 s = 0; s < Seg; ++s)
			{
				const double A2 = 2.0 * kPi * s / Seg;
				const FVector Dir = A * FMath::Cos(A2) + B * FMath::Sin(A2);
				const FVector P = C + Dir * (R * FMath::Cos(T)) + N * (R * 0.55 * FMath::Sin(T) - 0.001);
				M.Vertex(P, (Dir * FMath::Cos(T) + N * FMath::Sin(T)).GetSafeNormal(), FVector2D(P.X + P.Y, P.Z));
			}
		}
		for (int32 r = 0; r < Rings; ++r)
		{
			for (int32 s = 0; s < Seg; ++s)
			{
				const int32 s1 = (s + 1) % Seg;
				M.Quad(Base + r * Seg + s, Base + r * Seg + s1, Base + (r + 1) * Seg + s1, Base + (r + 1) * Seg + s);
			}
		}
	}

	// ============================================================================================ the clusters

	/** A clustered shaft on its shoe over a capital (or a half-column's), to its cast capital under the girder. */
	void Cluster(FParts& P, const FVector2D& C)
	{
		FMeshData& M = P[SlotIron];
		const double Z0 = AP::CapitalTop, Z1 = AP::ClusterTop;
		// The shoe: a plate with four bolts into the abacus, a moulded base.
		M.Box(FVector(C.X - 0.22, C.Y - 0.22, Z0 - 0.002), FVector(C.X + 0.22, C.Y + 0.22, Z0 + 0.03), FMeshData::AllFaces);
		for (const FVector2D& Corner : {FVector2D(-1, -1), FVector2D(1, -1), FVector2D(1, 1), FVector2D(-1, 1)})
		{
			Bolt(M, FVector(C.X + Corner.X * 0.16, C.Y + Corner.Y * 0.16, Z0 + 0.03), kUp);
		}
		TArray<FLathePoint> Base = {{0.0, Z0 + 0.03, false}, {0.20, Z0 + 0.03, false}, {0.20, Z0 + 0.05, true}, {0.17, Z0 + 0.09, true},
									{0.15, Z0 + 0.10, false}, {0.15, Z0 + 0.14, false}, {0.0, Z0 + 0.14, false}};
		Lathe(M, FVector(C.X, C.Y, 0.0), Base, 32);
		// The core and four colonnettes, with two annulets.
		const double ZS0 = Z0 + 0.13, ZS1 = Z1 - 0.28;
		Cylinder(M, C, 0.075, ZS0, ZS1, 20, false, false);
		for (int32 k = 0; k < 4; ++k)
		{
			const double A = kPi * 0.25 + kPi * 0.5 * k;
			Cylinder(M, C + FVector2D(FMath::Cos(A), FMath::Sin(A)) * 0.095, 0.045, ZS0, ZS1, 14, false, false);
		}
		for (const double ZA : {7.45, 8.65})
		{
			TArray<FLathePoint> Ring = {{0.0, ZA - 0.03, false}, {0.15, ZA - 0.03, false}, {0.165, ZA - 0.01, true}, {0.165, ZA + 0.01, true},
										{0.15, ZA + 0.03, false}, {0.0, ZA + 0.03, false}};
			Lathe(M, FVector(C.X, C.Y, 0.0), Ring, 32);
		}
		// The cast capital: a bell of iron leaves under a square top plate.
		TArray<FLathePoint> Cap = {{0.0, ZS1 - 0.01, false}, {0.15, ZS1 - 0.01, false}, {0.165, ZS1 + 0.02, true}, {0.15, ZS1 + 0.05, false},
								   {0.155, ZS1 + 0.08, true}, {0.19, ZS1 + 0.17, true}, {0.25, Z1 - 0.04, false}, {0.0, Z1 - 0.04, false}};
		Lathe(M, FVector(C.X, C.Y, 0.0), Cap, 32);
		for (int32 k = 0; k < 8; ++k)
		{
			const double A = 2.0 * kPi * k / 8.0 + kPi / 8.0;
			const FVector Rad(FMath::Cos(A), FMath::Sin(A), 0.0);
			const FVector Tan(-Rad.Y, Rad.X, 0.0);
			// A cast leaf: a flat tongue up the bell, its tip rolled out.
			TArray<FStation> St;
			for (int32 i = 0; i <= 6; ++i)
			{
				const double U = double(i) / 6.0;
				const double Z = ZS1 + 0.07 + U * 0.16;
				const double R = 0.155 + U * 0.08 + 0.05 * U * U * U;
				St.Add({FVector(C.X, C.Y, 0.0) + Rad * R + FVector(0, 0, Z - 0.03 * U * U * U), Tan, (Rad * 0.3 + kUp).GetSafeNormal()});
			}
			SweepStations(M, St, RectSection(-0.035, 0.035, 0.0, 0.012));
		}
		M.Box(FVector(C.X - 0.30, C.Y - 0.30, Z1 - 0.045), FVector(C.X + 0.30, C.Y + 0.30, Z1 + 0.002), FMeshData::AllFaces);
	}

	// ============================================================================================ the girders

	/** The box girder on an arcade line (x = X) from the north wall to the south, 9.7 … 10.0 m, riveted, over its clusters. */
	void Girder(FParts& P, double X)
	{
		FMeshData& M = P[SlotIron];
		const double Y0 = AP::Y0 - 0.02, Y1 = AP::Y1 + 0.02, Z0 = AP::GirderBottom, Z1 = AP::GirderTop;
		const double H = 0.35, W = 0.012;
		// Bottom and top flanges, two webs, and the angles that join them.
		M.Box(FVector(X - H, Y0, Z0), FVector(X + H, Y1, Z0 + 0.018), FMeshData::AllFaces);
		M.Box(FVector(X - H - 0.02, Y0, Z1 - 0.018), FVector(X + H + 0.02, Y1, Z1), FMeshData::AllFaces);
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const double XW = X + Side * (H - 0.06);
			M.Box(FVector(FMath::Min(XW, XW + Side * W), Y0, Z0 + 0.018), FVector(FMath::Max(XW, XW + Side * W), Y1, Z1 - 0.018), FMeshData::AllFaces);
			// The flange angles' outstands along the webs.
			M.Box(FVector(FMath::Min(XW, XW + Side * 0.06), Y0, Z0 + 0.018), FVector(FMath::Max(XW, XW + Side * 0.06), Y1, Z0 + 0.03), FMeshData::AllFaces);
			M.Box(FVector(FMath::Min(XW, XW + Side * 0.06), Y0, Z1 - 0.03), FVector(FMath::Max(XW, XW + Side * 0.06), Y1, Z1 - 0.018), FMeshData::AllFaces);
			// Rivets along the angles, 15 cm apart, seen from the sides.
			for (double Y = Y0 + 0.1; Y < Y1 - 0.05; Y += 0.15)
			{
				Rivet(M, FVector(XW + Side * W, Y, Z0 + 0.06), FVector(Side, 0, 0));
				Rivet(M, FVector(XW + Side * W, Y, Z1 - 0.06), FVector(Side, 0, 0));
			}
			// A cast moulding along the web's face at mid-height (the founder's bead).
			M.Box(FVector(FMath::Min(XW + Side * W, XW + Side * (W + 0.018)), Y0, 0.5 * (Z0 + Z1) - 0.012),
				  FVector(FMath::Max(XW + Side * W, XW + Side * (W + 0.018)), Y1, 0.5 * (Z0 + Z1) + 0.012), FMeshData::AllFaces);
		}
		// Splice plates and bolts over each column.
		for (int32 Line = 1; Line <= AP::Lines; ++Line)
		{
			const double Y = AP::LineY(Line);
			for (int32 Side = -1; Side <= 1; Side += 2)
			{
				const double XW = X + Side * (H - 0.06 + W);
				M.Box(FVector(FMath::Min(XW, XW + Side * 0.014), Y - 0.25, Z0 + 0.04), FVector(FMath::Max(XW, XW + Side * 0.014), Y + 0.25, Z1 - 0.04),
					  FMeshData::AllFaces);
				for (int32 r = 0; r < 2; ++r)
				{
					for (int32 c = -2; c <= 2; ++c)
					{
						Bolt(M, FVector(XW + Side * 0.014, Y + c * 0.09, Z0 + 0.10 + r * 0.10), FVector(Side, 0, 0), 0.013);
					}
				}
			}
		}
		// The lead valley gutter on top, between the two glass skins, its sides dressed up under them.
		FMeshData& L = P[SlotLead];
		L.Rect(FVector(X - 0.33, Y0, Z1 + 0.004), FVector(X + 0.33, Y0, Z1 + 0.004), FVector(X + 0.33, Y1, Z1 + 0.004), FVector(X - 0.33, Y1, Z1 + 0.004), kUp);
	}

	// ============================================================================================ the arcade's arches

	/** One bay's pointed arch on an arcade line (x = X) between lines A and A + 1, with its spandrel rings. */
	void ArcadeBay(FParts& P, double X, int32 Line)
	{
		FMeshData& M = P[SlotIron];
		const double YC = AP::LineY(Line) + 0.5 * AP::Bay;
		const double Crown = AP::ArcadeCrown;
		for (const bool bRight : {false, true})
		{
			FArcHalf H = PointedHalf(YC, AP::ArcadeHalfSpan, AP::ArcadeSpring, Crown, bRight);
			TArray<FStation> St;
			const int32 N = 24;
			const double TC = H.CrownAngle();
			for (int32 i = 0; i <= N; ++i)
			{
				const double T = TC * i / N;
				const FVector2D Pt = H.At(T), Nr = H.Normal(T);
				St.Add({FVector(X, Pt.X, Pt.Y), FVector(0.0, Nr.X, Nr.Y), kX});
			}
			// The rib: a flat soffit plate, a web, and a roll on the intrados.
			SweepStations(M, St, RectSection(0.0, 0.16, -0.012, 0.012));
			SweepStations(M, St, RectSection(0.0, 0.018, -0.055, 0.055));
			SweepStations(M, St, RectSection(0.142, 0.16, -0.045, 0.045));
			TArray<FVector2D> Roll;
			for (int32 i = 0; i < 10; ++i) { const double A = 2.0 * kPi * i / 10.0; Roll.Add(FVector2D(-0.012 + 0.022 * FMath::Cos(A), 0.022 * FMath::Sin(A))); }
			SweepStations(M, St, Roll);
			// The spandrel's ring (the largest that clears the arch, the girder and the cluster), a quatrefoil of rings
			// inside it, tied to the arch below and the girder above by short lugs.
			const double Side = bRight ? 1.0 : -1.0;
			const double D = 2.34, RR = 0.46;
			const FVector RC(X, YC + Side * D, 9.20);
			Torus(M, RC, kX, RR, 0.018, 44, 8);
			Torus(M, RC, kX, RR - 0.05, 0.008, 44, 6);
			for (int32 k = 0; k < 4; ++k)
			{
				const double A = kPi * 0.25 + kPi * 0.5 * k;
				Torus(M, RC + FVector(0.0, 0.19 * FMath::Cos(A), 0.19 * FMath::Sin(A)), kX, 0.17, 0.009, 32, 6);
			}
			const double CentreOff = H.R - AP::ArcadeHalfSpan;
			const double ZExt = AP::ArcadeSpring + FMath::Sqrt(FMath::Max(0.0, FMath::Square(H.R + 0.16) - FMath::Square(D + CentreOff)));
			M.Box(FVector(X - 0.02, RC.Y - 0.022, ZExt - 0.02), FVector(X + 0.02, RC.Y + 0.022, RC.Z - RR + 0.01), FMeshData::AllFaces);
			M.Box(FVector(X - 0.02, RC.Y - 0.022, RC.Z + RR - 0.01), FVector(X + 0.02, RC.Y + 0.022, Crown + 0.005), FMeshData::AllFaces);
		}
		// A boss at the crown, against the girder's soffit.
		Ball(M, FVector(X, YC, Crown - 0.02), 0.05);
	}

	// ============================================================================================ the vaults' ribs

	void MainRib(FMeshData& M, const FArcHalf& H, double Y, bool bEnd)
	{
		const double TC = H.CrownAngle();
		const int32 N = FMath::Max(12, FMath::CeilToInt32(H.Length() / 0.25));
		const TArray<FStation> St = VaultStations(H, Y, 0.0, TC, N);
		const double W = bEnd ? 0.08 : 0.06;
		// Outer flange, web, inner flange (an I built up from plates).
		SweepStations(M, St, RectSection(-kRibTop - 0.016, -kRibTop, -W, W));
		SweepStations(M, St, RectSection(-kRibTop - kMainDepth, -kRibTop, -0.007, 0.007));
		SweepStations(M, St, RectSection(-kRibTop - kMainDepth, -kRibTop - kMainDepth + 0.016, -W * 0.85, W * 0.85));
		// The inner arch: a flat bar on edge, 0.62 m under the glass, springing with the rib and dying into it at the crown.
		const double TIn = TC * 0.94;
		TArray<FStation> In;
		for (int32 i = 0; i <= N; ++i)
		{
			const double T = TIn * i / N;
			const double Blend = FMath::Pow(double(i) / N, 6.0);
			const double Depth = FMath::Lerp(kInnerArch, kRibTop + kMainDepth, Blend);
			const FVector2D Pt = H.At(T, Depth), Nr = H.Normal(T);
			In.Add({FVector(Pt.X, Y, Pt.Y), FVector(Nr.X, 0.0, Nr.Y), kY});
		}
		SweepStations(M, In, RectSection(-0.03, 0.0, -0.009, 0.009));
		// Rings between them, each touching both: a chain up the arch.
		const double Gap = kInnerArch - (kRibTop + kMainDepth);
		const double RR = 0.5 * Gap;
		const double Arc = H.Length() * 0.9;
		const int32 Rings = FMath::Max(3, FMath::FloorToInt32(Arc / (2.0 * RR + 0.03)));
		for (int32 k = 0; k < Rings; ++k)
		{
			const double T = TIn * (k + 0.5) / Rings;
			if (T > TIn * 0.82) { break; }
			const FVector2D C = H.At(T, kRibTop + kMainDepth + RR);
			Torus(M, FVector(C.X, Y, C.Y), kY, RR - 0.008, 0.009, 24, 6);
		}
		// The shoe at its foot: a plate on the girder or the wall's bearer, bolted.
		const FVector2D Foot = H.At(0.0, kRibTop + 0.5 * kMainDepth);
		M.Box(FVector(Foot.X - 0.2, Y - 0.1, AP::Spring - 0.001), FVector(Foot.X + 0.2, Y + 0.1, AP::Spring + 0.025), FMeshData::AllFaces);
		for (int32 s = -1; s <= 1; s += 2) { Bolt(M, FVector(Foot.X + s * 0.14, Y, AP::Spring + 0.025), kUp, 0.014); }
	}

	void LightRib(FMeshData& M, const FArcHalf& H, double Y)
	{
		const double TC = H.CrownAngle();
		const int32 N = FMath::Max(12, FMath::CeilToInt32(H.Length() / 0.3));
		const TArray<FStation> St = VaultStations(H, Y, 0.0, TC, N);
		// A tee: its table under the purlins, its stem hanging below, a bead on the stem's edge.
		SweepStations(M, St, RectSection(-kRibTop - 0.012, -kRibTop, -0.045, 0.045));
		SweepStations(M, St, RectSection(-kRibTop - kLightDepth, -kRibTop, -0.006, 0.006));
		SweepStations(M, St, RectSection(-kRibTop - kLightDepth, -kRibTop - kLightDepth + 0.018, -0.011, 0.011));
	}

	/** Purlins (along the building) and glazing bars (round the arcs) of one vault half. */
	void Purlins(FMeshData& M, const FArcHalf& H)
	{
		const double TC = H.CrownAngle();
		const int32 NP = FMath::Max(2, FMath::RoundToInt32(H.Length() / 1.8));   // a purlin every ~1.8 m
		for (int32 k = 1; k <= NP; ++k)
		{
			const double T = TC * k / NP;
			const FVector2D Pt = H.At(T), Nr = H.Normal(T);
			const FVector2D Tn = H.Tangent(T);
			const FVector A(Nr.X, 0, Nr.Y), B(Tn.X, 0, Tn.Y);
			TArray<FStation> St = {{FVector(Pt.X, kGlassY0 + 0.02, Pt.Y), A, B}, {FVector(Pt.X, kGlassY1 - 0.02, Pt.Y), A, B}};
			if (k == NP)
			{
				// The ridge: a heavier bar at the crown (once, from the east half).
				if (H.Sign() > 0.0) { SweepStations(M, St, RectSection(-kRibTop, -0.004, -0.05, 0.05)); }
			}
			else
			{
				SweepStations(M, St, RectSection(-kBarDepth - 0.012, -kBarDepth, -0.026, 0.026));
				SweepStations(M, St, RectSection(-kRibTop, -kBarDepth, -0.005, 0.005));
			}
		}
		// Glazing bars every 0.75 m, following the arc from the springing to the ridge.
		const int32 NG = FMath::RoundToInt32((kGlassY1 - kGlassY0) / AP::BarPitch);
		const int32 N = FMath::Max(8, FMath::CeilToInt32(H.Length() / AP::PanePitch));
		for (int32 g = 1; g < NG; ++g)
		{
			const double Y = kGlassY0 + (kGlassY1 - kGlassY0) * g / NG;
			const TArray<FStation> St = VaultStations(H, Y, H.AngleAtHeight(AP::Spring + 0.1), TC, N);
			SweepStations(M, St, RectSection(-kBarDepth, -0.003, -0.004, 0.004));
			SweepStations(M, St, RectSection(-kBarDepth, -kBarDepth + 0.01, -0.016, 0.016));
		}
	}

	// ============================================================================================ the end screens

	/** A vertical bar (a mullion) of a screen in the plane y = Y from Z0 to Z1 at x = X; its depth into the room. */
	void Mullion(FMeshData& M, double X, double Y, double Z0, double Z1, double HalfW, double Depth, double NY)
	{
		if (Z1 - Z0 < 0.01) { return; }
		M.Box(FVector(X - HalfW, FMath::Min(Y, Y + NY * Depth), Z0), FVector(X + HalfW, FMath::Max(Y, Y + NY * Depth), Z1), FMeshData::AllFaces);
		M.Box(FVector(X - 0.004, FMath::Min(Y, Y - NY * 0.02), Z0), FVector(X + 0.004, FMath::Max(Y, Y - NY * 0.02), Z1), FMeshData::AllFaces);
	}

	/** The height of a screen's top (the vault's glass arc, less the ribs) at x. */
	double ScreenTop(double X)
	{
		const double AX = FMath::Abs(X);
		if (AX <= AP::NaveHalf)
		{
			const FArcHalf H = VaultHalf(X < 0 ? 0 : 1);
			const double Dx = AX + AP::NaveCentreOffset;
			return AP::Spring + FMath::Sqrt(FMath::Max(0.0, FMath::Square(H.R) - Dx * Dx));
		}
		const double D = FMath::Abs(AX - AP::AisleCentreX);
		const double Dx = D + AP::AisleCentreOffset;
		return AP::Spring + FMath::Sqrt(FMath::Max(0.0, AP::AisleRadius * AP::AisleRadius - Dx * Dx));
	}

	void Screen(FParts& P, bool bNorth)
	{
		FMeshData& M = P[SlotIron];
		const double Y = bNorth ? kGlassY0 : kGlassY1;
		const double NY = bNorth ? 1.0 : -1.0;   // into the court
		// The nave's screen: mullions at 1.6 m, transoms, and bars at 0.8 m in the clear panes.
		for (const double X : AP::ScreenMullions)
		{
			const double Top = ScreenTop(X) - 0.02;
			const bool bEdge = FMath::Abs(FMath::Abs(X) - AP::NaveHalf) < 1e-6;
			Mullion(M, X, Y, AP::NaveStone, Top, bEdge ? 0.06 : 0.035, bEdge ? 0.2 : 0.12, NY);
		}
		for (double X = -AP::NaveHalf + 0.6; X < AP::NaveHalf; X += 0.8)
		{
			bool bOnMullion = false;
			for (const double MX : AP::ScreenMullions) { bOnMullion |= FMath::Abs(MX - X) < 0.1; }
			if (bOnMullion) { continue; }
			const bool bInBand = FMath::Abs(X) < AP::BandX1 - 0.01;
			const double Top = ScreenTop(X) - 0.02;
			if (bInBand && !bNorth)
			{
				Mullion(M, X, Y, AP::NaveStone, AP::LowerRow0, 0.012, 0.05, NY);
				Mullion(M, X, Y, AP::UpperRow1, Top, 0.012, 0.05, NY);
			}
			else
			{
				Mullion(M, X, Y, AP::NaveStone, Top, 0.012, 0.05, NY);
			}
		}
		for (const double Z : AP::ScreenTransoms)
		{
			// The transom stops where the arc comes down to it.
			const double Half = FMath::Min(AP::NaveHalf, VaultHalf(1).HalfWidthAt(Z + 0.05));
			const bool bBand = Z > 8.9 && Z < 12.7 && !bNorth;
			M.Box(FVector(-Half, FMath::Min(Y, Y + NY * (bBand ? 0.14 : 0.1)), Z - (bBand ? 0.05 : 0.035)),
				  FVector(Half, FMath::Max(Y, Y + NY * (bBand ? 0.14 : 0.1)), Z + (bBand ? 0.05 : 0.035)), FMeshData::AllFaces);
		}
		// Upper transoms in the nave's head, every 1.1 m up to the crown.
		for (double Z = 16.0; Z < AP::NaveCrown() - 0.3; Z += 1.1)
		{
			const double Half = FMath::Min(AP::NaveHalf, VaultHalf(1).HalfWidthAt(Z + 0.05));
			if (Half > 0.2) { M.Box(FVector(-Half, FMath::Min(Y, Y + NY * 0.06), Z - 0.015), FVector(Half, FMath::Max(Y, Y + NY * 0.06), Z + 0.015), FMeshData::AllFaces); }
		}
		// The aisles' gables above their walls: mullions at 1.6 m, two transoms.
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			for (double X = 6.8; X < 13.9; X += 1.6)
			{
				const double XX = Side * X;
				Mullion(M, XX, Y, AP::Spring, ScreenTop(XX) - 0.02, 0.03, 0.1, NY);
			}
			for (const double Z : {11.6, 13.4})
			{
				const double HW = FMath::Min(AP::AisleHalfSpan, VaultHalf(5).HalfWidthAt(Z + 0.05));
				const double H0 = AP::AisleCentreX - HW, H1 = AP::AisleCentreX + HW;
				const double XA = Side * H0, XB = Side * H1;
				M.Box(FVector(FMath::Min(XA, XB), FMath::Min(Y, Y + NY * 0.08), Z - 0.03), FVector(FMath::Max(XA, XB), FMath::Max(Y, Y + NY * 0.08), Z + 0.03),
					  FMeshData::AllFaces);
			}
			// The gable's sill on the wall (a cast cill) at the springing.
			const double XA = Side * AP::NaveHalf, XB = Side * AP::X1;
			M.Box(FVector(FMath::Min(XA, XB), FMath::Min(Y - NY * 0.04, Y + NY * 0.1), AP::Spring), FVector(FMath::Max(XA, XB), FMath::Max(Y - NY * 0.04, Y + NY * 0.1), AP::Spring + 0.06),
				  FMeshData::AllFaces);
		}
		// The nave screen's cill on the stone.
		M.Box(FVector(-AP::NaveHalf, FMath::Min(Y - NY * 0.04, Y + NY * 0.12), AP::NaveStone + 0.02), FVector(AP::NaveHalf, FMath::Max(Y - NY * 0.04, Y + NY * 0.12), AP::NaveStone + 0.09),
			  FMeshData::AllFaces);
	}

	// ============================================================================================ lancets, lead, rainwater

	void SaddleBars(FParts& P)
	{
		FMeshData& M = P[SlotIron];
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const double X = Side * (AP::X1 + 0.30);
			for (int32 b = 0; b < 6; ++b)
			{
				const double YC = AP::BayCentreY(b);
				for (double Z = AP::LancetSill + 0.45; Z < AP::LancetSpring() + 0.2; Z += 0.45)
				{
					const double Half = LancetOpening(YC).HalfAt(Z) + 0.03;
					Beam(M, FVector(X - Side * 0.015, YC - Half, Z), FVector(X - Side * 0.015, YC + Half, Z), kUp, 0.008, 0.008);
				}
			}
		}
	}

	/** Lead: ridge rolls and the nave's ridge ventilator, the verges over the screens. */
	void Lead(FParts& P)
	{
		FMeshData& L = P[SlotLead];
		FMeshData& I = P[SlotIron];
		const double Y0 = kGlassY0 - 0.12, Y1 = kGlassY1 + 0.12;
		// Ridge rolls on the three crowns.
		for (const double X : {-AP::AisleCentreX, 0.0, AP::AisleCentreX})
		{
			const double Z = FMath::IsNearlyZero(X) ? AP::NaveCrown() : AP::AisleCrown();
			TArray<FVector2D> Roll;
			for (int32 i = 0; i < 12; ++i) { const double A = 2.0 * kPi * i / 12.0; Roll.Add(FVector2D(0.045 * FMath::Cos(A), 0.045 * FMath::Sin(A))); }
			SweepStations(L, {{FVector(X, Y0, Z + 0.05), kX, kUp}, {FVector(X, Y1, Z + 0.05), kX, kUp}}, Roll);
			if (!FMath::IsNearlyZero(X))
			{
				// The aisles' ridge: a low cast cresting (the outside's 15.6 m).
				I.Box(FVector(X - 0.015, Y0, Z + 0.08), FVector(X + 0.015, Y1, Z + 0.34), FMeshData::AllFaces);
				for (double Y = Y0 + 0.3; Y < Y1; Y += 0.6) { Torus(I, FVector(X, Y, Z + 0.26), kX, 0.08, 0.008, 16, 4); }
				continue;
			}
			// The nave's ridge ventilator: louvred cheeks on cast standards under a lead cap (to 17.6 m), narrow (0.44 m) so its
			// shadow down the nave is a line, not a band.
			const double VZ0 = Z + 0.08, VZ1 = Z + 0.48;
			for (double Y = Y0 + 0.2; Y < Y1; Y += 1.5)
			{
				I.Box(FVector(-0.15, Y - 0.02, VZ0), FVector(0.15, Y + 0.02, VZ1), FMeshData::AllFaces);
			}
			for (int32 Side = -1; Side <= 1; Side += 2)
			{
				for (int32 k = 0; k < 4; ++k)
				{
					const double Z0 = VZ0 + 0.04 + k * 0.09;
					const FVector A(Side * 0.12, Y0, Z0), B(Side * 0.17, Y0, Z0 + 0.06);
					const FVector Blade = (B - A).GetSafeNormal(), Across = FVector::CrossProduct(kY, Blade);
					SweepStations(I, {{A, Blade, Across}, {FVector(A.X, Y1, A.Z), Blade, Across}}, RectSection(0.0, FVector::Distance(A, B), -0.004, 0.004));
				}
			}
			L.Poly({FVector(-0.22, Y0, VZ1), FVector(0.0, Y0, VZ1 + 0.12), FVector(0.0, Y1, VZ1 + 0.12), FVector(-0.22, Y1, VZ1)}, FVector(-0.33, 0, 0.94).GetSafeNormal());
			L.Poly({FVector(0.22, Y0, VZ1), FVector(0.0, Y0, VZ1 + 0.12), FVector(0.0, Y1, VZ1 + 0.12), FVector(0.22, Y1, VZ1)}, FVector(0.33, 0, 0.94).GetSafeNormal());
			L.Poly({FVector(-0.22, Y0, VZ1 - 0.01), FVector(0.22, Y0, VZ1 - 0.01), FVector(0.22, Y1, VZ1 - 0.01), FVector(-0.22, Y1, VZ1 - 0.01)}, -kUp);
		}
	}

	/** Rainwater heads and downpipes: beside the side walls' buttresses and on the end buttresses at x ±6. */
	void Rainwater(FParts& P)
	{
		FMeshData& M = P[SlotIron];
		auto Pipe = [&](const FVector2D& At, const FVector2D& Out, double HeadZ)
		{
			const double R = 0.052;
			// The head: a moulded box on the wall, its outlet through the parapet above.
			const FVector2D Along(-Out.Y, Out.X);
			auto Q = [&](double A, double D, double Z) { const FVector2D Q2 = At + Along * A + Out * D; return FVector(Q2.X, Q2.Y, Z); };
			const FVector Lo = Q(-0.19, -0.01, HeadZ - 0.36), Hi = Q(0.19, 0.28, HeadZ);
			M.Box(FVector(FMath::Min(Lo.X, Hi.X), FMath::Min(Lo.Y, Hi.Y), Lo.Z), FVector(FMath::Max(Lo.X, Hi.X), FMath::Max(Lo.Y, Hi.Y), Hi.Z), FMeshData::AllFaces);
			const FVector Lo2 = Q(-0.22, -0.01, HeadZ - 0.02), Hi2 = Q(0.22, 0.31, HeadZ + 0.05);
			M.Box(FVector(FMath::Min(Lo2.X, Hi2.X), FMath::Min(Lo2.Y, Hi2.Y), Lo2.Z), FVector(FMath::Max(Lo2.X, Hi2.X), FMath::Max(Lo2.Y, Hi2.Y), Hi2.Z), FMeshData::AllFaces);
			// The pipe, 12 cm off the wall, down to a shoe over a gully.
			const FVector2D PC = At + Out * 0.14;
			TArray<FLathePoint> Taper = {{0.0, HeadZ - 0.36, false}, {0.09, HeadZ - 0.36, false}, {R, HeadZ - 0.50, false}, {R, 0.18, false},
										 {R + 0.012, 0.16, false}, {R + 0.012, 0.02, false}, {0.0, 0.02, false}};
			Lathe(M, FVector(PC.X, PC.Y, 0.0), Taper, 16);
			// Holderbats every 1.8 m.
			for (double Z = 1.2; Z < HeadZ - 0.6; Z += 1.8)
			{
				TArray<FLathePoint> Ear = {{0.0, Z - 0.03, false}, {R + 0.01, Z - 0.03, false}, {R + 0.01, Z + 0.03, false}, {0.0, Z + 0.03, false}};
				Lathe(M, FVector(PC.X, PC.Y, 0.0), Ear, 16);
				const FVector A3 = Q(0.0, -0.02, Z), B3 = Q(0.0, 0.1, Z);
				Beam(M, A3, B3, kUp, 0.012, 0.025);
			}
		};
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const double X = Side * (AP::X1 + AP::Wall);
			for (const int32 Line : {2, 4, 6})
			{
				Pipe(FVector2D(X, AP::LineY(Line) - AP::ButtressHalf - 0.30), FVector2D(Side, 0), 10.55);
			}
			Pipe(FVector2D(Side * (AP::NaveHalf + AP::ButtressHalf + 0.3), AP::Y0 - AP::Wall), FVector2D(0, -1), 10.35);
			Pipe(FVector2D(Side * (AP::NaveHalf + AP::ButtressHalf + 0.3), AP::Y1 + AP::Wall), FVector2D(0, 1), 10.35);
		}
	}
}

namespace AlbionBuild
{
	using namespace AlbionIronImpl;

	void BuildIron(FParts& P)
	{
		// Clusters on the ten columns and the four half-columns (set a little into the court).
		for (int32 i = 0; i < 10; ++i) { Cluster(P, FVector2D(Stone(i).X, Stone(i).Y)); }
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			Cluster(P, FVector2D(Side * AlbionPlan::ColumnX, AlbionPlan::Y0 + 0.12));
			Cluster(P, FVector2D(Side * AlbionPlan::ColumnX, AlbionPlan::Y1 - 0.12));
			Girder(P, Side * AlbionPlan::ColumnX);
			for (int32 Line = 1; Line < AlbionPlan::Lines; ++Line) { ArcadeBay(P, Side * AlbionPlan::ColumnX, Line); }
		}
		// The ribs: main ribs on the column lines (the end ones framing the screens), lighter ones every 1.5 m.
		const int32 NR = FMath::RoundToInt32((AlbionPlan::Y1 - AlbionPlan::Y0) / AlbionPlan::RibPitch);
		for (int32 k = 0; k <= NR; ++k)
		{
			const double Y = AlbionPlan::Y0 + AlbionPlan::RibPitch * k;
			const bool bMain = k % 4 == 0;
			for (int32 h = 0; h < 6; ++h)
			{
				const FArcHalf H = VaultHalf(h);
				if (bMain) { MainRib(P[SlotIron], H, Y, k == 0 || k == NR); }
				else { LightRib(P[SlotIron], H, Y); }
			}
		}
		for (int32 h = 0; h < 6; ++h) { Purlins(P[SlotIron], VaultHalf(h)); }
		// The bearers along the side walls for the lighter ribs' feet.
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const double XW = Side * AlbionPlan::X1;
			P[SlotIron].Box(FVector(FMath::Min(XW, XW - Side * 0.38), AlbionPlan::Y0, AlbionPlan::Spring - 0.06),
							FVector(FMath::Max(XW, XW - Side * 0.38), AlbionPlan::Y1, AlbionPlan::Spring), FMeshData::AllFaces);
			for (double Y = AlbionPlan::Y0 + 0.75; Y < AlbionPlan::Y1; Y += 1.5)
			{
				// A cast bracket under the bearer at each lighter rib.
				TArray<FStation> St = {{FVector(XW, Y, AlbionPlan::Spring - 0.35), FVector(0, 1, 0), FVector(-Side, 0, 0)},
									   {FVector(XW, Y, AlbionPlan::Spring - 0.06), FVector(0, 1, 0), FVector(-Side, 0, 0)}};
				SweepStations(P[SlotIron], St, {FVector2D(-0.008, 0.0), FVector2D(0.008, 0.0), FVector2D(0.008, 0.02), FVector2D(-0.008, 0.02)});
				Beam(P[SlotIron], FVector(XW - Side * 0.01, Y, AlbionPlan::Spring - 0.35), FVector(XW - Side * 0.33, Y, AlbionPlan::Spring - 0.07), FVector(0, 1, 0), 0.008, 0.02);
			}
		}
		Screen(P, true);
		Screen(P, false);
		SaddleBars(P);
		Lead(P);
		Rainwater(P);
	}
}
