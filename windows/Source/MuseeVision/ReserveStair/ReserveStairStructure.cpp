#include "ReserveStair/ReserveStairStructure.h"

#include "MuseeVision.h"
#include "Components/BoxComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Texture.h"
#include "Geometry/MuseeBake.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Plan/MuseePlan.h"
#include "ProceduralMeshComponent.h"
#include "Salon/SalonKit.h"

/**
 * The long stair and its shaft, in plan metres (x east, y south, z up; the actor at the world origin).
 * The flight runs east and down: riser k (0 … 35) stands at x_k = −90 + 0.3389 k, from the tread
 * above (h −k·r) to the tread below (h −(k+1)·r), r = 5.8/36; tread k runs from x_k to the next
 * nosing, 25 mm past x_{k+1}. A named namespace (the module builds in unity files).
 */
namespace ReserveStairKit
{
	namespace LS = MuseePlan::LongStair;
	using SalonKit::FMeshData;

	constexpr double X0 = LS::OpeningX0, X1 = LS::OpeningX1, Hw = LS::HalfWidth;
	constexpr double TopX = LS::FirstNosingX, Going = LS::Going, Riser = LS::Riser, Depth = LS::Depth;
	constexpr int32 Risers = LS::Risers;
	constexpr double WallX = LS::ReserveWallOuterX, LandingEnd = LS::LandingEnd, Soffit = LS::SoffitHeight;
	constexpr double Slope = Riser / Going;
	constexpr double Sink = 0.002;

	// The treads' nosings: 25 mm proud, 40 mm deep, a 12 mm round on top and a 12 mm undercut.
	constexpr double Nose = 0.025, NoseDepth = 0.04, NoseRound = 0.012, Undercut = 0.012;
	constexpr int32 NoseSegments = 4;

	// The closed strings: 25 mm proud of the brick, 24 cm over the nosings, a 15 cm skirting on the landing.
	constexpr double StringFace = Hw - 0.025;
	constexpr double StringIn = StringFace - Sink;       // where the treads end, 2 mm into the strings
	constexpr double StringRise = 0.24, StringDrop = 0.35, SkirtHeight = 0.15;
	constexpr double StringTopCap = -0.06;               // at the top, under the landing's nosing

	// The shaft: walls from under the landing; the curb round the opening; the closed outside.
	constexpr double WallBottom = -Depth - 0.05;
	constexpr double CurbZ = -0.30;
	constexpr double OuterY = Hw + 0.4, OuterX0 = X0 - 0.4, OuterBottom = -Depth - 0.35, CapZ = -0.01;
	constexpr double ShaftEndX = WallX + Sink;           // 2 mm into the Reserve's west wall

	// The handrails.
	constexpr double RailY = Hw - 0.085, RailR = 0.025, RailHeight = 0.90, RailStartZ = -0.05;
	constexpr double RailEndX = WallX - 0.30;
	constexpr double KneeRadius = 0.15, ReturnRadius = 0.06;
	constexpr int32 RailArcSegments = 20;                // round the rail's section (300° of it)
	constexpr double LedHalfDeg = 30.0;                  // the flat underside: the chord between 240° and 300°
	constexpr double BracketEvery = 1.3, RoseR = 0.03, RoseDepth = 0.008, ArmR = 0.008, ArmDrop = 0.075;
	constexpr int32 FlightLights = 3;

	// The soffit's downlights (surface cans, a recessed lens).
	constexpr double DownlightXs[2] = {-80.2, -76.4};
	constexpr double CanR = 0.05, CanDrop = 0.08, LensR = 0.042, LensRecess = 0.015;

	// The guards round the opening.
	constexpr double GuardHeight = 1.0, GuardThick = 0.06;

	const FVector Up(0, 0, 1);

	double RiserX(int32 K) { return TopX + Going * K; }
	/** The line through the nosings (the pitch line). */
	double Nosing(double X) { return -(X - TopX) * Slope; }
	double RailStartX() { return TopX + (RailHeight - RailStartZ) / Slope; }
	double KneeX() { return TopX + Depth / Slope; }

	/** The ramp the visitor walks on: flat, then through the middle of every tread, then flat. */
	double WalkZ(double X) { return FMath::Clamp(-Riser * ((X - TopX) / Going + 0.5), -Depth, 0.0); }
	double WalkX0() { return TopX - Going / 2; }
	double WalkX1() { return TopX + (Risers - 0.5) * Going; }

	double TreadZ(double X)
	{
		if (X < TopX + Nose) { return 0.0; }
		const int32 K = FMath::Min(FMath::FloorToInt32((X - TopX - Nose) / Going), Risers - 1);
		return -Riser * (K + 1);
	}

	FVector2D FaceUV(const FVector& P, const FVector& N) { return SalonKit::FaceUV(P, N); }

	/** A strip of the stair's profile from P0 to P1 (x, z) across y ±Y, with (x, z) normals N0, N1. */
	void Strip(FMeshData& M, const FVector2D& P0, const FVector2D& P1, const FVector2D& N0, const FVector2D& N1, double Y)
	{
		// Plan (x, y) on the treads, (y, down) on the risers and nosings: one choice for the whole strip, metres either way.
		const bool bFlat = FMath::Abs((N0 + N1).GetSafeNormal().Y) > 0.7;
		auto V = [&M, bFlat](const FVector2D& P, const FVector2D& N, double Yv)
		{
			const FVector Pos(P.X, Yv, P.Y), Nr(N.X, 0, N.Y);
			return M.Vertex(Pos, Nr, bFlat ? FVector2D(P.X, Yv) : FVector2D(Yv, -P.Y));
		};
		M.Quad(V(P0, N0, -Y), V(P1, N1, -Y), V(P1, N1, Y), V(P0, N0, Y));
	}

	/** A flat rectangle split into cells of at most Cell metres: Origin + U·s + V·t. */
	void Panel(FMeshData& M, const FVector& Origin, const FVector& U, const FVector& V, double LU, double LV, const FVector& N, double Cell = 2.0)
	{
		const int32 NU = FMath::Max(1, FMath::CeilToInt32(LU / Cell - 1e-9)), NV = FMath::Max(1, FMath::CeilToInt32(LV / Cell - 1e-9));
		for (int32 I = 0; I < NU; ++I)
		{
			for (int32 J = 0; J < NV; ++J)
			{
				const FVector A = Origin + U * (LU * I / NU) + V * (LV * J / NV);
				const FVector B = Origin + U * (LU * (I + 1) / NU) + V * (LV * J / NV);
				const FVector C = Origin + U * (LU * (I + 1) / NU) + V * (LV * (J + 1) / NV);
				const FVector D = Origin + U * (LU * I / NU) + V * (LV * (J + 1) / NV);
				M.Rect(A, B, C, D, N);
			}
		}
	}

	struct FStairMeshes
	{
		FMeshData Treads, Landings, Strings;       // the stair
		FMeshData Brick, Outside, Plaster, Curb;   // the shaft
		FMeshData Bronze, Led;                     // the rails and downlights
		FMeshData Walk;                            // hidden
	};

	// ============================================================================ The stair

	void BuildStair(FStairMeshes& Out)
	{
		const FVector2D UpN(0, 1), East(1, 0);
		// The top landing, flush with the oval's floor, wall to wall; its nosing is riser 0's.
		Strip(Out.Landings, FVector2D(X0, 0), FVector2D(TopX + Nose - NoseRound, 0), UpN, UpN, Hw);
		for (int32 K = 0; K < Risers; ++K)
		{
			const double XK = RiserX(K), ZK = -Riser * K, ZN = -Riser * (K + 1);
			// The landing's nosing and riser run 2 mm into the walls; the others into the strings.
			const double Y = K == 0 ? Hw + Sink : StringIn;
			// The nosing's round, its front and its undercut, then the riser.
			const FVector2D RC(XK + Nose - NoseRound, ZK - NoseRound);
			for (int32 I = 0; I < NoseSegments; ++I)
			{
				const double A0 = UE_DOUBLE_PI / 2 * (1.0 - double(I) / NoseSegments), A1 = UE_DOUBLE_PI / 2 * (1.0 - double(I + 1) / NoseSegments);
				const FVector2D D0(FMath::Cos(A0), FMath::Sin(A0)), D1(FMath::Cos(A1), FMath::Sin(A1));
				Strip(Out.Treads, RC + D0 * NoseRound, RC + D1 * NoseRound, D0, D1, Y);
			}
			Strip(Out.Treads, FVector2D(XK + Nose, ZK - NoseRound), FVector2D(XK + Nose, ZK - NoseDepth), East, East, Y);
			const FVector2D UnderN = FVector2D(Undercut, -Nose).GetSafeNormal();
			Strip(Out.Treads, FVector2D(XK + Nose, ZK - NoseDepth), FVector2D(XK, ZK - NoseDepth - Undercut), UnderN, UnderN, Y);
			Strip(Out.Treads, FVector2D(XK, ZK - NoseDepth - Undercut), FVector2D(XK, ZN), East, East, Y);
			if (K + 1 < Risers)
			{
				Strip(Out.Treads, FVector2D(XK, ZN), FVector2D(RiserX(K + 1) + Nose - NoseRound, ZN), UpN, UpN, StringIn);
			}
			else
			{
				// The bottom landing, between the strings to the Reserve's wall; through its door wall to wall.
				Strip(Out.Landings, FVector2D(XK, ZN), FVector2D(WallX, ZN), UpN, UpN, StringIn);
				for (const FVector2D& Ys : {FVector2D(-Hw, -StringIn), FVector2D(-StringIn, StringIn), FVector2D(StringIn, Hw)})
				{
					Out.Landings.Rect(FVector(WallX, Ys.X, -Depth), FVector(LandingEnd, Ys.X, -Depth), FVector(LandingEnd, Ys.Y, -Depth),
									  FVector(WallX, Ys.Y, -Depth), Up);
				}
			}
		}
	}

	double StringTopX0() { return TopX + (StringRise - StringTopCap) / Slope; }
	double StringTopX1() { return TopX + (Depth - SkirtHeight + StringRise) / Slope; }
	double StringFootX() { return TopX + (Depth + Sink - StringDrop) / Slope; }
	double StringTop(double X)
	{
		if (X <= StringTopX0()) { return StringTopCap; }
		if (X <= StringTopX1()) { return Nosing(X) + StringRise; }
		return -Depth + SkirtHeight;
	}
	double StringBottom(double X) { return FMath::Max(Nosing(X) - StringDrop, -Depth - Sink); }

	void BuildStrings(FStairMeshes& Out)
	{
		const TArray<double> Xs = {X0, StringTopX0(), StringFootX(), StringTopX1(), WallX};
		for (const double S : {-1.0, 1.0})
		{
			const double YF = S * StringFace, YW = S * (Hw + Sink);
			for (int32 I = 0; I + 1 < Xs.Num(); ++I)
			{
				const double XA = Xs[I], XB = Xs[I + 1];
				Out.Strings.Rect(FVector(XA, YF, StringBottom(XA)), FVector(XB, YF, StringBottom(XB)), FVector(XB, YF, StringTop(XB)),
								 FVector(XA, YF, StringTop(XA)), FVector(0, -S, 0));
				const FVector2D D(XB - XA, StringTop(XB) - StringTop(XA));
				const FVector N = FVector(-D.Y, 0, D.X).GetSafeNormal();
				Out.Strings.Rect(FVector(XA, YF, StringTop(XA)), FVector(XB, YF, StringTop(XB)), FVector(XB, YW, StringTop(XB)),
								 FVector(XA, YW, StringTop(XA)), N);
			}
			// The skirting's end in the Reserve's door.
			Out.Strings.Rect(FVector(WallX, YF, -Depth - Sink), FVector(WallX, S * Hw, -Depth - Sink), FVector(WallX, S * Hw, StringTop(WallX)),
							 FVector(WallX, YF, StringTop(WallX)), FVector(1, 0, 0));
		}
	}

	// ============================================================================ The shaft

	void BuildShaft(FStairMeshes& Out)
	{
		// The walls, split where the opening's curb and the soffit meet them, so the two parts share their vertices.
		const TArray<double> Zs = {WallBottom, -3.85, -1.85, Soffit + Sink, CurbZ, 0.0};
		auto Column = [&Out, &Zs](double XA, double XB, double ZTop, double S)
		{
			const double Y = S * Hw;
			for (int32 K = 0; K + 1 < Zs.Num() && Zs[K + 1] <= ZTop + 1e-9; ++K)
			{
				FMeshData& M = Zs[K] >= CurbZ - 1e-9 ? Out.Curb : Out.Brick;
				M.Rect(FVector(XA, Y, Zs[K]), FVector(XB, Y, Zs[K]), FVector(XB, Y, Zs[K + 1]), FVector(XA, Y, Zs[K + 1]), FVector(0, -S, 0));
			}
		};
		for (const double S : {-1.0, 1.0})
		{
			constexpr int32 NA = 4, NB = 5;
			for (int32 I = 0; I < NA; ++I) { Column(X0 + (X1 - X0) * I / NA, X0 + (X1 - X0) * (I + 1) / NA, 0.0, S); }
			for (int32 I = 0; I < NB; ++I) { Column(X1 + (ShaftEndX - X1) * I / NB, X1 + (ShaftEndX - X1) * (I + 1) / NB, Soffit + Sink, S); }
		}
		// The opening's east edge (the oval floor's slab) and the soffit beyond it.
		Out.Curb.Rect(FVector(X1, -Hw - Sink, Soffit), FVector(X1, Hw + Sink, Soffit), FVector(X1, Hw + Sink, 0.0), FVector(X1, -Hw - Sink, 0.0), FVector(-1, 0, 0));
		Panel(Out.Plaster, FVector(X1, -Hw - Sink, Soffit), FVector(1, 0, 0), FVector(0, 1, 0), ShaftEndX - X1, 2 * (Hw + Sink), -Up);

		// Closed outside (as the Reserve's box): outer faces, the west end, a top 1 cm under the oval's floor.
		Panel(Out.Outside, FVector(OuterX0, -OuterY, OuterBottom), FVector(1, 0, 0), FVector(0, 0, 1), ShaftEndX - OuterX0, CapZ - OuterBottom, FVector(0, -1, 0));
		Panel(Out.Outside, FVector(OuterX0, OuterY, OuterBottom), FVector(1, 0, 0), FVector(0, 0, 1), ShaftEndX - OuterX0, CapZ - OuterBottom, FVector(0, 1, 0));
		Panel(Out.Outside, FVector(OuterX0, -OuterY, OuterBottom), FVector(0, 1, 0), FVector(0, 0, 1), 2 * OuterY, CapZ - OuterBottom, FVector(-1, 0, 0));
		Panel(Out.Outside, FVector(OuterX0, -OuterY, CapZ), FVector(1, 0, 0), FVector(0, 1, 0), X0 - OuterX0, 2 * OuterY, Up);
		Panel(Out.Outside, FVector(X0, Hw, CapZ), FVector(1, 0, 0), FVector(0, 1, 0), X1 - X0, OuterY - Hw, Up);
		Panel(Out.Outside, FVector(X0, -OuterY, CapZ), FVector(1, 0, 0), FVector(0, 1, 0), X1 - X0, OuterY - Hw, Up);
		Panel(Out.Outside, FVector(X1, -OuterY, CapZ), FVector(1, 0, 0), FVector(0, 1, 0), ShaftEndX - X1, 2 * OuterY, Up);
	}

	// ============================================================================ The walkway (hidden)

	void BuildWalk(FStairMeshes& Out)
	{
		const TArray<FVector2D> Line = {FVector2D(X0, 0.0), FVector2D(WalkX0(), 0.0), FVector2D(WalkX1(), -Depth), FVector2D(LandingEnd, -Depth)};
		for (int32 I = 0; I + 1 < Line.Num(); ++I)
		{
			const FVector2D D = Line[I + 1] - Line[I];
			const FVector N = FVector(-D.Y, 0, D.X).GetSafeNormal();
			Out.Walk.Rect(FVector(Line[I].X, -Hw, Line[I].Y), FVector(Line[I + 1].X, -Hw, Line[I + 1].Y), FVector(Line[I + 1].X, Hw, Line[I + 1].Y),
						  FVector(Line[I].X, Hw, Line[I].Y), N);
		}
	}

	// ============================================================================ The handrails

	/** The round between two straight runs meeting at Corner (directions In and Out): N + 1 points, tangent to both. */
	TArray<FVector> Fillet(const FVector& Corner, const FVector& In, const FVector& Out, double Radius, int32 N)
	{
		const double Alpha = FMath::Acos(FMath::Clamp(FVector::DotProduct(In, Out), -1.0, 1.0));
		const double T = Radius * FMath::Tan(Alpha / 2);
		const FVector T1 = Corner - In * T;
		const FVector W = (Out - In * FVector::DotProduct(Out, In)).GetSafeNormal();
		const FVector O = T1 + W * Radius;
		TArray<FVector> Pts;
		for (int32 I = 0; I <= N; ++I)
		{
			const double Phi = Alpha * I / N;
			Pts.Add(O - W * (Radius * FMath::Cos(Phi)) + In * (Radius * FMath::Sin(Phi)));
		}
		return Pts;
	}

	/** A handrail's axis (side S: −1 north, +1 south): out of the wall, down the flight, round the knee, along the landing, back into the wall. */
	TArray<FVector> RailPath(double S)
	{
		const double XS = RailStartX(), XE = RailEndX, ZL = -Depth + RailHeight;
		const FVector Down = FVector(1, 0, -Slope).GetSafeNormal();
		TArray<FVector> Path = {FVector(XS, S * (Hw + 0.01), RailStartZ)};
		Path.Append(Fillet(FVector(XS, S * RailY, RailStartZ), FVector(0, -S, 0), Down, ReturnRadius, 6));
		Path.Append(Fillet(FVector(KneeX(), S * RailY, ZL), Down, FVector(1, 0, 0), KneeRadius, 8));
		Path.Append(Fillet(FVector(XE, S * RailY, ZL), FVector(1, 0, 0), FVector(0, S, 0), ReturnRadius, 6));
		Path.Add(FVector(XE, S * (Hw + 0.01), ZL));
		return Path;
	}

	/** The rail's axis height at plan x (the straight runs; the knee's round is 3 cm either side of KneeX). */
	double RailZ(double X) { return X <= KneeX() ? Nosing(X) + RailHeight : -Depth + RailHeight; }

	/** Sweeps the rail's section along its path: the round in bronze, the flat underside (the LED line) in Led. */
	void SweepRail(FMeshData& Bronze, FMeshData& Led, const TArray<FVector>& Path)
	{
		const int32 N = Path.Num();
		TArray<FVector> Ts, As, Bs;
		TArray<double> Ss;
		double S = 0;
		for (int32 I = 0; I < N; ++I)
		{
			if (I > 0) { S += FVector::Distance(Path[I - 1], Path[I]); }
			Ss.Add(S);
			const FVector T = (Path[FMath::Min(I + 1, N - 1)] - Path[FMath::Max(I - 1, 0)]).GetSafeNormal();
			const FVector B = (Up - T * FVector::DotProduct(Up, T)).GetSafeNormal();
			Ts.Add(T);
			Bs.Add(B);
			As.Add(FVector::CrossProduct(T, B).GetSafeNormal());
		}
		// The section: (a, b) round the axis, from −60° up over the top to 240°, then the flat back to −60°.
		struct FSec { double Phi; FVector2D N; };
		TArray<FSec> Round;
		const double From = -(90.0 - LedHalfDeg), To = 180.0 + (90.0 - LedHalfDeg);
		for (int32 J = 0; J <= RailArcSegments; ++J)
		{
			const double Phi = FMath::DegreesToRadians(From + (To - From) * J / RailArcSegments);
			Round.Add({Phi, FVector2D(FMath::Cos(Phi), FMath::Sin(Phi))});
		}
		auto Point = [&](int32 I, double Phi) { return Path[I] + (As[I] * FMath::Cos(Phi) + Bs[I] * FMath::Sin(Phi)) * RailR; };
		for (int32 I = 0; I + 1 < N; ++I)
		{
			for (int32 J = 0; J + 1 < Round.Num(); ++J)
			{
				const FSec& P = Round[J];
				const FSec& Q = Round[J + 1];
				const FVector NI0 = As[I] * P.N.X + Bs[I] * P.N.Y, NI1 = As[I] * Q.N.X + Bs[I] * Q.N.Y;
				const FVector NJ0 = As[I + 1] * P.N.X + Bs[I + 1] * P.N.Y, NJ1 = As[I + 1] * Q.N.X + Bs[I + 1] * Q.N.Y;
				const double V0 = RailR * (P.Phi - Round[0].Phi), V1 = RailR * (Q.Phi - Round[0].Phi);
				const int32 A = Bronze.Vertex(Point(I, P.Phi), NI0, FVector2D(Ss[I], V0));
				const int32 B = Bronze.Vertex(Point(I + 1, P.Phi), NJ0, FVector2D(Ss[I + 1], V0));
				const int32 C = Bronze.Vertex(Point(I + 1, Q.Phi), NJ1, FVector2D(Ss[I + 1], V1));
				const int32 D = Bronze.Vertex(Point(I, Q.Phi), NI1, FVector2D(Ss[I], V1));
				Bronze.Quad(A, B, C, D);
			}
			// The flat underside, from 240° to 300° (= −60°).
			const double P0 = Round.Last().Phi, P1 = Round[0].Phi + 2 * UE_DOUBLE_PI;
			const int32 A = Led.Vertex(Point(I, P0), -Bs[I], FVector2D(Ss[I], 0));
			const int32 B = Led.Vertex(Point(I + 1, P0), -Bs[I + 1], FVector2D(Ss[I + 1], 0));
			const int32 C = Led.Vertex(Point(I + 1, P1), -Bs[I + 1], FVector2D(Ss[I + 1], 0.025));
			const int32 D = Led.Vertex(Point(I, P1), -Bs[I], FVector2D(Ss[I], 0.025));
			Led.Quad(A, B, C, D);
		}
	}

	/** A round bar from P0 to P1, radius Rad, capped at either end. */
	void Bar(FMeshData& M, const FVector& P0, const FVector& P1, double Rad, bool bCap0, bool bCap1, int32 Segs = 16)
	{
		const FVector Ax = (P1 - P0).GetSafeNormal();
		const FVector U = FVector::CrossProduct(Ax, FMath::Abs(Ax.Z) < 0.9 ? Up : FVector(1, 0, 0)).GetSafeNormal();
		const FVector V = FVector::CrossProduct(Ax, U);
		const double Len = FVector::Distance(P0, P1);
		for (int32 I = 0; I < Segs; ++I)
		{
			const double A0 = 2 * UE_DOUBLE_PI * I / Segs, A1 = 2 * UE_DOUBLE_PI * (I + 1) / Segs;
			const FVector D0 = U * FMath::Cos(A0) + V * FMath::Sin(A0), D1 = U * FMath::Cos(A1) + V * FMath::Sin(A1);
			const int32 A = M.Vertex(P0 + D0 * Rad, D0, FVector2D(Rad * A0, 0));
			const int32 B = M.Vertex(P0 + D1 * Rad, D1, FVector2D(Rad * A1, 0));
			const int32 C = M.Vertex(P1 + D1 * Rad, D1, FVector2D(Rad * A1, Len));
			const int32 D = M.Vertex(P1 + D0 * Rad, D0, FVector2D(Rad * A0, Len));
			M.Quad(A, B, C, D);
			const FVector2D C0(0, 0), E0(Rad * FMath::Cos(A0), Rad * FMath::Sin(A0)), E1(Rad * FMath::Cos(A1), Rad * FMath::Sin(A1));
			if (bCap0) { M.Tri(M.Vertex(P0, -Ax, C0), M.Vertex(P0 + D0 * Rad, -Ax, E0), M.Vertex(P0 + D1 * Rad, -Ax, E1)); }
			if (bCap1) { M.Tri(M.Vertex(P1, Ax, C0), M.Vertex(P1 + D0 * Rad, Ax, E0), M.Vertex(P1 + D1 * Rad, Ax, E1)); }
		}
	}

	void Ball(FMeshData& M, const FVector& C, double Rad)
	{
		constexpr int32 Lon = 16, Lat = 8;
		auto D = [](int32 I, int32 J)
		{
			const double La = -UE_DOUBLE_PI / 2 + UE_DOUBLE_PI * I / Lat, Lo = 2 * UE_DOUBLE_PI * J / Lon;
			return FVector(FMath::Cos(La) * FMath::Cos(Lo), FMath::Cos(La) * FMath::Sin(Lo), FMath::Sin(La));
		};
		for (int32 I = 0; I < Lat; ++I)
		{
			for (int32 J = 0; J < Lon; ++J)
			{
				const FVector A = D(I, J), B = D(I, J + 1), E = D(I + 1, J + 1), F = D(I + 1, J);
				// UVs: longitude and latitude, as arc lengths.
				auto UV = [Rad](int32 Ii, int32 Jj) { return FVector2D(Rad * 2 * UE_DOUBLE_PI * Jj / Lon, Rad * UE_DOUBLE_PI * Ii / Lat); };
				const int32 VA = M.Vertex(C + A * Rad, A, UV(I, J)), VB = M.Vertex(C + B * Rad, B, UV(I, J + 1));
				const int32 VE = M.Vertex(C + E * Rad, E, UV(I + 1, J + 1)), VF = M.Vertex(C + F * Rad, F, UV(I + 1, J));
				M.Quad(VA, VB, VE, VF);
			}
		}
	}

	/** Where the brackets stand along a run (plan x): evenly, about BracketEvery apart. */
	TArray<double> Spaced(double From, double To)
	{
		const int32 N = FMath::Max(2, FMath::CeilToInt32((To - From) / BracketEvery) + 1);
		TArray<double> Xs;
		for (int32 I = 0; I < N; ++I) { Xs.Add(From + (To - From) * I / (N - 1)); }
		return Xs;
	}

	TArray<double> BracketXs()
	{
		TArray<double> Xs = Spaced(RailStartX() + 0.35, KneeX() - 0.35);
		Xs.Append(Spaced(KneeX() + 0.4, RailEndX - 0.25));
		return Xs;
	}

	void BuildRails(FStairMeshes& Out)
	{
		for (const double S : {-1.0, 1.0})
		{
			SweepRail(Out.Bronze, Out.Led, RailPath(S));
			// Brackets: a rose on the wall, an arm out under the rail, a stem up into it.
			for (const double X : BracketXs())
			{
				const double ZR = RailZ(X), ZA = ZR - ArmDrop;
				const FVector Elbow(X, S * RailY, ZA);
				Bar(Out.Bronze, FVector(X, S * (Hw + Sink), ZA), FVector(X, S * (Hw - RoseDepth), ZA), RoseR, false, true, 24);
				Bar(Out.Bronze, FVector(X, S * (Hw - RoseDepth), ZA), Elbow, ArmR, false, false);
				Ball(Out.Bronze, Elbow, ArmR * 1.25);
				Bar(Out.Bronze, Elbow, FVector(X, S * RailY, ZR - RailR * 0.5), ArmR * 0.85, false, false);
			}
		}
		// The soffit's downlights: a bronze can, its rim, and the lens recessed in it.
		for (const double X : DownlightXs)
		{
			const FVector Top(X, 0, Soffit + Sink), Bottom(X, 0, Soffit - CanDrop), Lens(X, 0, Soffit - CanDrop + LensRecess);
			Bar(Out.Bronze, Top, Bottom, CanR, false, false, 32);
			constexpr int32 Segs = 32;
			for (int32 I = 0; I < Segs; ++I)
			{
				const double A0 = 2 * UE_DOUBLE_PI * I / Segs, A1 = 2 * UE_DOUBLE_PI * (I + 1) / Segs;
				const FVector D0(FMath::Cos(A0), FMath::Sin(A0), 0), D1(FMath::Cos(A1), FMath::Sin(A1), 0);
				Out.Bronze.Rect(Bottom + D0 * LensR, Bottom + D1 * LensR, Bottom + D1 * CanR, Bottom + D0 * CanR, -Up);
				const double U0 = LensR * A0, U1 = LensR * A1;
				Out.Bronze.Quad(Out.Bronze.Vertex(Bottom + D0 * LensR, -D0, FVector2D(U0, 0)), Out.Bronze.Vertex(Bottom + D1 * LensR, -D1, FVector2D(U1, 0)),
								Out.Bronze.Vertex(Lens + D1 * LensR, -D1, FVector2D(U1, LensRecess)), Out.Bronze.Vertex(Lens + D0 * LensR, -D0, FVector2D(U0, LensRecess)));
				Out.Led.Tri(Out.Led.Vertex(Lens, -Up, FVector2D(X, 0)), Out.Led.Vertex(Lens + D0 * LensR, -Up, FVector2D(X + D0.X * LensR, D0.Y * LensR)),
							Out.Led.Vertex(Lens + D1 * LensR, -Up, FVector2D(X + D1.X * LensR, D1.Y * LensR)));
			}
		}
	}

	// ============================================================================ Lights

	struct FRailLight { FVector At; FVector Facing; FVector Along; double Length; };

	/** Under each rail: three lights down the flight and one along the landing, just under the LED line. */
	TArray<FRailLight> RailLightPlaces()
	{
		TArray<FRailLight> Out;
		const double Below = RailR * FMath::Sin(FMath::DegreesToRadians(90.0 - LedHalfDeg)) + 0.004;
		const double Tilt = FMath::DegreesToRadians(25.0);
		for (const double S : {-1.0, 1.0})
		{
			const FVector Inward(0, -S, 0);
			TArray<FVector2D> Runs;   // (from, to) in plan x
			const double XA = RailStartX() + 0.15, XB = KneeX();
			for (int32 I = 0; I < FlightLights; ++I) { Runs.Add(FVector2D(XA + (XB - XA) * I / FlightLights, XA + (XB - XA) * (I + 1) / FlightLights)); }
			Runs.Add(FVector2D(KneeX() + 0.1, RailEndX - 0.1));
			for (const FVector2D& Run : Runs)
			{
				const double XM = 0.5 * (Run.X + Run.Y);
				const bool bFlight = Run.Y <= KneeX() + 1e-9;
				const FVector T = bFlight ? FVector(1, 0, -Slope).GetSafeNormal() : FVector(1, 0, 0);
				const FVector B = (Up - T * FVector::DotProduct(Up, T)).GetSafeNormal();
				FRailLight Light;
				Light.At = FVector(XM, S * RailY, RailZ(XM)) - B * Below;
				Light.Facing = (-B * FMath::Cos(Tilt) + Inward * FMath::Sin(Tilt)).GetSafeNormal();
				Light.Along = T;
				Light.Length = (Run.Y - Run.X) / T.X;
				Out.Add(Light);
			}
		}
		return Out;
	}

	constexpr int32 RailLightCount = 2 * (FlightLights + 1);

	void BuildAll(FStairMeshes& Out)
	{
		BuildStair(Out);
		BuildStrings(Out);
		BuildShaft(Out);
		BuildWalk(Out);
		BuildRails(Out);
	}
}

namespace RSK = ReserveStairKit;

AReserveStairStructure::AReserveStairStructure()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	auto Make = [this](const TCHAR* ComponentName, bool bCollision)
	{
		UProceduralMeshComponent* Component = CreateDefaultSubobject<UProceduralMeshComponent>(ComponentName);
		Component->SetupAttachment(RootComponent);
		Component->bUseAsyncCooking = true;
		if (bCollision)
		{
			Component->bUseComplexAsSimpleCollision = true;
			Component->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		else
		{
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		return Component;
	};
	Stair = Make(TEXT("Stair"), false);
	Shaft = Make(TEXT("Shaft"), true);
	Rails = Make(TEXT("Rails"), false);
	RailLeds = Make(TEXT("RailLeds"), false);
	RailLeds->SetCastShadow(false);
	Walkway = Make(TEXT("Walkway"), true);
	// Left procedural by the bake: the stone and the LEDs get their materials at play; the walkway is hidden.
	MuseeBake::NoBake(Stair);
	MuseeBake::NoBake(RailLeds);
	MuseeBake::NoBake(Walkway);
	// The ramp: the visitor walks on it; the look trace and the camera pass through it to the treads.
	Walkway->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Walkway->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Walkway->SetVisibility(false);
	Walkway->SetHiddenInGame(true);
	Walkway->SetCastShadow(false);

	for (int32 I = 0; I < 3; ++I)
	{
		UBoxComponent* G = CreateDefaultSubobject<UBoxComponent>(*FString::Printf(TEXT("Guard%d"), I + 1));
		G->SetupAttachment(RootComponent);
		G->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		G->SetCollisionObjectType(ECC_WorldStatic);
		G->SetCollisionResponseToAllChannels(ECR_Ignore);
		G->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		G->SetGenerateOverlapEvents(false);
		G->SetHiddenInGame(true);
		G->CanCharacterStepUpOn = ECB_No;
		Guards.Add(G);
	}
	for (int32 I = 0; I < RSK::RailLightCount; ++I)
	{
		URectLightComponent* L = CreateDefaultSubobject<URectLightComponent>(*FString::Printf(TEXT("RailLight%d"), I + 1));
		L->SetupAttachment(RootComponent);
		L->SetMobility(EComponentMobility::Movable);
		RailLights.Add(L);
	}
	for (int32 I = 0; I < 2; ++I)
	{
		USpotLightComponent* L = CreateDefaultSubobject<USpotLightComponent>(*FString::Printf(TEXT("Downlight%d"), I + 1));
		L->SetupAttachment(RootComponent);
		L->SetMobility(EComponentMobility::Movable);
		Downlights.Add(L);
	}

	auto Path = [](const TCHAR* Folder, const TCHAR* Name)
	{
		return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(FString::Printf(TEXT("%s/%s.%s"), Folder, Name, Name)));
	};
	const TCHAR* Materials = TEXT("/Game/Museum/Materials");
	const TCHAR* Imported = TEXT("/Game/Museum/Materials/USD");
	BrickMaterial = Path(Imported, TEXT("MI_brick"));
	StoneMaterial = Path(Materials, TEXT("M_Travertine_Honed"));
	PlasterMaterial = Path(Imported, TEXT("MI_plaster_shaft"));
	BronzeMaterial = Path(Imported, TEXT("MI_bronze"));
	GlowMaterial = Path(Materials, TEXT("M_Daylit"));

	AddTags();
	PlaceLights();
}

TArray<FString> AReserveStairStructure::GetReplacedImportPrims()
{
	return {
		TEXT("/Museum/Reserve/The_long_stair"),
		TEXT("/Museum/Reserve/Stair_shaft"),
		TEXT("/Museum/Reserve/Handrails"),
	};
}

double AReserveStairStructure::GetWalkHeightAt(double PlanX) { return RSK::WalkZ(PlanX); }

double AReserveStairStructure::GetTreadHeightAt(double PlanX) { return RSK::TreadZ(PlanX); }

void AReserveStairStructure::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
	ApplyMaterials(false);
	PlaceGuards();
	PlaceLights();
	AddTags();
}

void AReserveStairStructure::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	PlaceGuards();
	PlaceLights();
	AddTags();
}

void AReserveStairStructure::BeginPlay()
{
	Super::BeginPlay();
	Build();
	ApplyMaterials(true);
	RetireImported();
}

void AReserveStairStructure::AddTags()
{
	Tags.AddUnique(FName(TEXT("musee.building")));
	Tags.AddUnique(FName(TEXT("musee.wing:Reserve")));
}

void AReserveStairStructure::RetireImported()
{
	const TArray<FString> Prims = GetReplacedImportPrims();
	int32 Count = 0;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor == this) { continue; }
		bool bMatch = false;
		for (const FName& Tag : Actor->Tags)
		{
			FString S = Tag.ToString();
			if (!S.RemoveFromStart(TEXT("prim:"))) { continue; }
			for (const FString& P : Prims) { bMatch |= S == P || S.StartsWith(P + TEXT("/")); }
		}
		if (!bMatch) { continue; }
		Actor->SetActorHiddenInGame(true);
		Actor->SetActorEnableCollision(false);
		Actor->Tags.Remove(FName(TEXT("musee.building")));
		Actor->Tags.AddUnique(FName(TEXT("musee.retired")));
		++Count;
	}
	if (Count > 0) { UE_LOG(LogMusee, Log, TEXT("Long stair: %d imported pieces retired (the native stair, shaft and handrails replace them)."), Count); }
}

void AReserveStairStructure::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }   // baked into static meshes (Geometry/MuseeBake.h)
	RSK::FStairMeshes M;
	RSK::BuildAll(M);
	for (UProceduralMeshComponent* Component : {Stair.Get(), Shaft.Get(), Rails.Get(), RailLeds.Get(), Walkway.Get()})
	{
		if (Component) { Component->ClearAllMeshSections(); }
	}
	M.Treads.Write(Stair, 0, false);
	M.Landings.Write(Stair, 1, false);
	M.Strings.Write(Stair, 2, false);
	M.Brick.Write(Shaft, 0, true);
	M.Outside.Write(Shaft, 1, true);
	M.Plaster.Write(Shaft, 2, true);
	M.Curb.Write(Stair, 3, false);
	M.Bronze.Write(Rails, 0, false);
	M.Led.Write(RailLeds, 0, false);
	M.Walk.Write(Walkway, 0, true);
}

void AReserveStairStructure::PlaceGuards()
{
	// The guards: along the opening's north and south edges from the first riser, and across its east edge.
	const double GX0 = RSK::TopX, GX1 = RSK::X1 + RSK::GuardThick;
	const FVector Boxes[3][2] = {
		{FVector(0.5 * (GX0 + GX1), -(RSK::Hw + RSK::GuardThick / 2), RSK::GuardHeight / 2), FVector(0.5 * (GX1 - GX0), RSK::GuardThick / 2, RSK::GuardHeight / 2)},
		{FVector(0.5 * (GX0 + GX1), RSK::Hw + RSK::GuardThick / 2, RSK::GuardHeight / 2), FVector(0.5 * (GX1 - GX0), RSK::GuardThick / 2, RSK::GuardHeight / 2)},
		{FVector(RSK::X1 + RSK::GuardThick / 2, 0, RSK::GuardHeight / 2), FVector(RSK::GuardThick / 2, RSK::Hw + RSK::GuardThick, RSK::GuardHeight / 2)},
	};
	for (int32 I = 0; I < Guards.Num() && I < 3; ++I)
	{
		if (!Guards[I]) { continue; }
		Guards[I]->SetRelativeLocation(Boxes[I][0] * MuseePlan::Cm);
		Guards[I]->SetBoxExtent(Boxes[I][1] * MuseePlan::Cm);
	}
}

void AReserveStairStructure::ApplyMaterials(bool bInstances)
{
	auto Set = [](UProceduralMeshComponent* Component, int32 Section, UMaterialInterface* Material)
	{
		if (Component && Material) { Component->SetMaterial(Section, Material); }
	};
	UMaterialInterface* StoneBase = StoneMaterial.LoadSynchronous();
	UMaterialInterface* Dressed = StoneBase;
	UMaterialInterface* GlowBase = GlowMaterial.LoadSynchronous();
	UMaterialInterface* Glow = GlowBase;
	if (bInstances)
	{
		// Play only (never saved with the map). Each tread, riser and string is one stone: the travertine's
		// world-grid joints off there (the landings keep them).
		if (StoneBase)
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(StoneBase, this);
			MID->SetFlags(RF_Transient);
			MID->SetScalarParameterValue(TEXT("FloorJoints"), 0.f);
			MID->SetScalarParameterValue(TEXT("WallJoints"), 0.f);
			Dressed = MID;
		}
		if (GlowBase)
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(GlowBase, this);
			MID->SetFlags(RF_Transient);
			if (UTexture* White = LoadObject<UTexture>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture")))
			{
				MID->SetTextureParameterValue(TEXT("Image"), White);
			}
			const FLinearColor Warm = FLinearColor::MakeFromColorTemperature(LightKelvin);
			const float Luma = FMath::Max(Warm.GetLuminance(), 1e-3f);
			MID->SetVectorParameterValue(TEXT("Tint"), FLinearColor(Warm.R / Luma, Warm.G / Luma, Warm.B / Luma, 1.f));
			MID->SetScalarParameterValue(TEXT("Luminance"), LedNits);
			MID->SetScalarParameterValue(TEXT("NightFloor"), 1.f);
			Glow = MID;
		}
	}
	UMaterialInterface* Brick = BrickMaterial.LoadSynchronous();
	Set(Stair, 0, Dressed);
	Set(Stair, 1, StoneBase);
	Set(Stair, 2, Dressed);
	Set(Shaft, 0, Brick);
	Set(Shaft, 1, Brick);
	Set(Shaft, 2, PlasterMaterial.LoadSynchronous());
	Set(Stair, 3, Dressed);
	Set(Rails, 0, BronzeMaterial.LoadSynchronous());
	Set(RailLeds, 0, Glow);
}

void AReserveStairStructure::PlaceLights()
{
	const TArray<RSK::FRailLight> Places = RSK::RailLightPlaces();
	for (int32 I = 0; I < RailLights.Num(); ++I)
	{
		URectLightComponent* L = RailLights[I];
		if (!L || !Places.IsValidIndex(I)) { continue; }
		const RSK::FRailLight& P = Places[I];
		L->SetRelativeLocationAndRotation(P.At * MuseePlan::Cm, FRotationMatrix::MakeFromXY(P.Facing, P.Along).Rotator());
		L->SetSourceWidth(static_cast<float>(P.Length * 0.98 * MuseePlan::Cm));
		L->SetSourceHeight(1.2f);
		L->SetBarnDoorAngle(88.f);
		L->SetBarnDoorLength(0.f);
		L->SetIntensityUnits(ELightUnits::Lumens);
		L->SetIntensity(static_cast<float>(RailLumensPerMetre * P.Length));
		L->SetUseTemperature(true);
		L->SetTemperature(LightKelvin);
		L->SetLightColor(FLinearColor::White);
		L->SetAttenuationRadius(400.f);
		L->SetCastShadows(true);
	}
	for (int32 I = 0; I < Downlights.Num() && I < 2; ++I)
	{
		USpotLightComponent* L = Downlights[I];
		if (!L) { continue; }
		const FVector At(RSK::DownlightXs[I], 0, RSK::Soffit - RSK::CanDrop + RSK::LensRecess - 0.005);
		L->SetRelativeLocationAndRotation(At * MuseePlan::Cm, FRotator(-90, 0, 0));
		L->SetIntensityUnits(ELightUnits::Lumens);
		L->SetIntensity(DownlightLumens);
		L->SetUseTemperature(true);
		L->SetTemperature(LightKelvin);
		L->SetLightColor(FLinearColor::White);
		L->SetInnerConeAngle(18.f);
		L->SetOuterConeAngle(38.f);
		L->SetAttenuationRadius(900.f);
		L->SetSourceRadius(3.f);
		L->SetSoftSourceRadius(0.f);
		L->SetCastShadows(true);
	}
}
