#include "HallOfLight/HallOfLightStructure.h"

#include "Geometry/MuseeBake.h"
#include "Components/SpotLightComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "Plan/MuseePlan.h"
#include "ProceduralMeshComponent.h"
#include "Salon/SalonKit.h"

/**
 * The Hall of Light's geometry, in plan metres (x east, y south, z up; the Rotunda's centre is the
 * origin), written in centimetres by SalonKit::FMeshData, whose triangles face along their normals.
 * Every member is a closed solid swept from a closed section (Solid); where solids meet they overlap
 * by 1–5 cm, and no two faces are closer than 1 cm. A Python port of this file checks that
 * (check_hall_of_light.py, beside it). A named namespace: the module builds in unity files.
 */
namespace HallOfLightBuild
{
	namespace H = MuseePlan::HallOfLight;
	namespace E = MuseePlan::Elan;
	using SalonKit::FMeshData;

	constexpr double Pi = UE_DOUBLE_PI;
	constexpr double VR = H::VaultRadius;            // 5.3
	constexpr double VC = H::VaultCentreHeight;      // 2.2
	constexpr double HW = H::HalfWidth;              // 4.5
	/**
	 * How far things run into the drums (and the glass through the Atrium's glass): the steel and the glass
	 * 2.8 cm, the end posts' and collars' backs 1.4 cm, the plinths 4 cm and the planter edges 5.5 cm, so no
	 * two ends lie within 1 cm of each other or of the drum's face. The steel stops 1.2 cm short of the
	 * Atrium's glass (the collar), the purlins 1.2 cm inside the collar.
	 */
	constexpr double Embed = 0.028;
	constexpr double Seat = 0.014;
	constexpr double PlinthEmbed = 0.04, KerbEmbed = 0.055;
	constexpr double DrumGap = 0.012;
	constexpr double PurlinDrumGap = DrumGap + 0.012;
	constexpr double GlassFoot = H::PlinthHeight - 0.01;   // the glass and the shoes stand 1 cm into the plinth
	constexpr double PostFoot = H::PlinthHeight - 0.02;    // the posts 2 cm
	constexpr int32 StripsPerPanel = 6;              // glass strips (and arch stations) to a purlin panel
	constexpr int32 LatheSegments = 64;
	constexpr int32 ArchSegments = 48;               // round the Rotunda door's arch
	constexpr int32 DiscSegments = 96;               // round a viewing stone
	constexpr int32 SquareSteps = 8;                 // points along each side of a viewing stone's square
	constexpr double BarClear = 0.025;               // the crosses' bars stop 2.5 cm short of each node (inside the purlin or the eaves)

	/** The floor: its edges under the plinths; where the west and east pieces meet the grid. */
	constexpr double FloorEdge = HW - 0.16;          // 4.34
	constexpr double FloorWestX = 11.3, FloorEastX = 39.1;
	constexpr double StoneCell = 0.6, StoneDisc = 0.40, StoneRing = 0.45;
	/** Radii (about the Rotunda's and the Atrium's centres) inside their walls, where the floor's end pieces turn. */
	constexpr double InRotundaWall = 10.7, InAtriumWall = 14.6;

	/** Eaves, transom, shoe (sections in depth d in from the glass line, and height). */
	constexpr double EavesBottom = 4.86, EavesTop = 5.10;
	constexpr double PostTop = 4.90;                 // 4 cm into the eaves, clear of the arches' feet
	constexpr double TransomHalf = 0.03;
	constexpr double ShoeTop = 0.34;

	/** The night lights. */
	constexpr double UplightInset = 0.15, UplightZ = 5.13, UplightAimY = 1.0, UplightAimZ = 7.4;
	constexpr double PathInset = 0.21, PathZ = 0.14, PathAimY = 2.0;   // aimed across, nearly level, at the far side's foot

	inline double Cross2(const FVector2D& A, const FVector2D& B) { return A.X * B.Y - A.Y * B.X; }
	inline double Mix(double A, double B, double T) { return A * (1.0 - T) + B * T; }

	// -------------------------------------------------------------------------------------------
	// Where the hall meets its neighbours

	double RotundaFaceX(double Y) { return FMath::Sqrt(H::RotundaOuterRadius * H::RotundaOuterRadius - Y * Y); }
	double AtriumFaceX(double Y, bool bDrum)
	{
		const double Ra = bDrum ? H::AtriumDrumRadius : H::AtriumOuterRadius;
		return E::CentreX - FMath::Sqrt(Ra * Ra - Y * Y);
	}
	/** Where the hall's steel starts: into the Rotunda's drum. */
	double WestEnd(double Y) { return RotundaFaceX(Y) - Embed; }
	/** … and ends: into the Atrium's stone base, or inside the collar short of its glass drum. */
	double EastEnd(double Y, bool bDrum) { return bDrum ? AtriumFaceX(Y, true) - PurlinDrumGap : AtriumFaceX(Y, false) + Embed; }
	/** The glass runs into the stone base, and through the drum's glass above it (a butt joint, sealed). */
	double GlassEastEnd(double Y, bool bDrum) { return AtriumFaceX(Y, bDrum) + Embed; }

	/** The vault: a point at angle Theta from the crown (south positive) and radius Radius, at x. */
	FVector VaultPoint(double X, double Theta, double Radius) { return FVector(X, Radius * FMath::Sin(Theta), VC + Radius * FMath::Cos(Theta)); }
	/** The springing, where the vault meets the walls' head at h 5 (58.1°). */
	double SpringAngle() { return FMath::Asin(HW / VR); }
	/** Where the vault crosses the Atrium's coping at h 6 (44.2°): above it the vault runs to the drum's glass. */
	double CopingAngle() { return FMath::Acos((E::BaseHeight - VC) / VR); }

	/** The purlins' angles: the springings (the eaves take them), the coping's lines, and five between. */
	TArray<double> PurlinAngles()
	{
		const double S = SpringAngle(), C = CopingAngle();
		TArray<double> A = {-S};
		for (int32 K = 0; K <= 6; ++K) { A.Add(-C + 2.0 * C * K / 6.0); }
		A.Add(S);
		return A;
	}

	/** The glass's strips and the arches' stations: each purlin panel in StripsPerPanel. */
	TArray<double> StripAngles()
	{
		const TArray<double> P = PurlinAngles();
		TArray<double> A;
		for (int32 K = 0; K + 1 < P.Num(); ++K)
		{
			for (int32 J = 0; J < StripsPerPanel; ++J) { A.Add(Mix(P[K], P[K + 1], double(J) / StripsPerPanel)); }
		}
		A.Add(P.Last());
		return A;
	}

	TArray<double> Between(double T0, double T1, int32 Steps)
	{
		TArray<double> Ts;
		for (int32 I = 0; I <= Steps; ++I) { Ts.Add(Mix(T0, T1, double(I) / Steps)); }
		return Ts;
	}

	double PostX(int32 K) { return H::FirstPost + H::PostStep * K; }
	double ArchX(int32 K) { return H::FirstArch + H::ArchStep * K; }

	// -------------------------------------------------------------------------------------------
	// Polygons and solids

	double PolygonArea(const TArray<FVector2D>& P)
	{
		double Area = 0.0;
		for (int32 I = 0; I < P.Num(); ++I) { Area += Cross2(P[I], P[(I + 1) % P.Num()]); }
		return 0.5 * Area;
	}

	/** Ear-clips a simple polygon (either way round) into index triples; points on an ear's edge block it. */
	TArray<int32> Triangulate(const TArray<FVector2D>& Poly)
	{
		TArray<int32> Out;
		const int32 Count = Poly.Num();
		if (Count < 3) { return Out; }
		const double Orient = PolygonArea(Poly) > 0.0 ? 1.0 : -1.0;
		TArray<int32> Left;
		for (int32 I = 0; I < Count; ++I) { Left.Add(I); }
		auto Turning = [&Poly, Orient](int32 A, int32 B, int32 C) { return Cross2(Poly[B] - Poly[A], Poly[C] - Poly[A]) * Orient; };
		auto Inside = [&Poly, Orient](const FVector2D& Q, int32 A, int32 B, int32 C)
		{
			return Cross2(Poly[B] - Poly[A], Q - Poly[A]) * Orient >= -1e-12 && Cross2(Poly[C] - Poly[B], Q - Poly[B]) * Orient >= -1e-12 &&
				   Cross2(Poly[A] - Poly[C], Q - Poly[C]) * Orient >= -1e-12;
		};
		while (Left.Num() > 3)
		{
			int32 Ear = INDEX_NONE;
			for (int32 I = 0; I < Left.Num() && Ear == INDEX_NONE; ++I)
			{
				const int32 A = Left[(I + Left.Num() - 1) % Left.Num()], B = Left[I], C = Left[(I + 1) % Left.Num()];
				if (Turning(A, B, C) <= 1e-12) { continue; }
				bool bBlocked = false;
				for (const int32 Other : Left)
				{
					if (Other != A && Other != B && Other != C && Inside(Poly[Other], A, B, C)) { bBlocked = true; break; }
				}
				if (!bBlocked) { Ear = I; }
			}
			if (Ear == INDEX_NONE) { break; }   // not simple: leave the rest open (the checks would show it)
			Out.Append({Left[(Ear + Left.Num() - 1) % Left.Num()], Left[Ear], Left[(Ear + 1) % Left.Num()]});
			Left.RemoveAt(Ear);
		}
		if (Left.Num() == 3 && Turning(Left[0], Left[1], Left[2]) > 1e-12) { Out.Append({Left[0], Left[1], Left[2]}); }
		return Out;
	}

	/** Where a section point Q lands at path parameter T. */
	using FPlace = TFunctionRef<FVector(double, const FVector2D&)>;

	/**
	 * A closed solid: the closed section swept through the stations Ts, flat across each of the section's
	 * edges (their normals from the surface's own derivatives, facing out of the section) and smooth along
	 * the path, with its ends capped. U runs along the path and V round the section, both in metres.
	 */
	void Solid(FMeshData& M, const TArray<FVector2D>& Section, const TArray<double>& Ts, FPlace Place, bool bCapStart = true, bool bCapEnd = true)
	{
		const int32 NP = Section.Num();
		if (NP < 3 || Ts.Num() < 2) { return; }
		const double Orient = PolygonArea(Section) > 0.0 ? 1.0 : -1.0;
		const double Step = 1e-6 * FMath::Max(1.0, FMath::Abs(Ts.Last() - Ts[0]));
		double V0 = 0.0;
		for (int32 J = 0; J < NP; ++J)
		{
			const FVector2D A = Section[J], B = Section[(J + 1) % NP];
			const FVector2D D = B - A;
			const FVector2D Out = FVector2D(D.Y, -D.X).GetSafeNormal() * Orient;
			const FVector2D Mid = (A + B) * 0.5;
			const double Len = D.Size();
			const int32 Base = M.Positions.Num();
			FVector PrevA = FVector::ZeroVector, PrevB = FVector::ZeroVector;
			double UA = 0.0, UB = 0.0;
			for (int32 I = 0; I < Ts.Num(); ++I)
			{
				const double T = Ts[I];
				const FVector PA = Place(T, A), PB = Place(T, B);
				if (I > 0)
				{
					UA += FVector::Dist(PrevA, PA);
					UB += FVector::Dist(PrevB, PB);
				}
				const FVector Along = Place(T + Step, Mid) - Place(T - Step, Mid);
				const FVector Hint = Place(T, Mid + Out * 1e-4) - Place(T, Mid);
				FVector Normal = FVector::CrossProduct(Along, PB - PA);
				if (!Normal.Normalize(1e-30)) { Normal = Hint.GetSafeNormal(1e-30); }
				else if (FVector::DotProduct(Normal, Hint) < 0.0) { Normal = -Normal; }
				M.Vertex(PA, Normal, FVector2D(UA, V0));
				M.Vertex(PB, Normal, FVector2D(UB, V0 + Len));
				PrevA = PA;
				PrevB = PB;
			}
			for (int32 I = 0; I + 1 < Ts.Num(); ++I)
			{
				const int32 K = Base + 2 * I;
				M.Quad(K, K + 2, K + 3, K + 1);
			}
			V0 += Len;
		}
		const TArray<int32> Tris = Triangulate(Section);
		FVector2D Centroid = FVector2D::ZeroVector;
		for (const FVector2D& Q : Section) { Centroid += Q / NP; }
		for (const bool bEnd : {false, true})
		{
			if (bEnd ? !bCapEnd : !bCapStart) { continue; }
			const double T = bEnd ? Ts.Last() : Ts[0];
			const FVector Normal = (Place(T + Step, Centroid) - Place(T - Step, Centroid)).GetSafeNormal(1e-30) * (bEnd ? 1.0 : -1.0);
			const int32 Base = M.Positions.Num();
			for (const FVector2D& Q : Section) { M.Vertex(Place(T, Q), Normal, Q); }
			for (int32 K = 0; K + 2 < Tris.Num(); K += 3) { M.Tri(Base + Tris[K], Base + Tris[K + 1], Base + Tris[K + 2]); }
		}
	}

	TArray<FVector2D> Rect(double A0, double A1, double B0, double B1)
	{
		return {FVector2D(A0, B0), FVector2D(A1, B0), FVector2D(A1, B1), FVector2D(A0, B1)};
	}

	/** An I-section: a flange ±HalfLow wide from B0 to B1, a web ±HalfWeb up to B2, a flange ±HalfHigh from B2 to B3. */
	TArray<FVector2D> ISection(double HalfLow, double HalfWeb, double HalfHigh, double B0, double B1, double B2, double B3)
	{
		return {FVector2D(-HalfLow, B0), FVector2D(HalfLow, B0), FVector2D(HalfLow, B1), FVector2D(HalfWeb, B1),
				FVector2D(HalfWeb, B2), FVector2D(HalfHigh, B2), FVector2D(HalfHigh, B3), FVector2D(-HalfHigh, B3),
				FVector2D(-HalfHigh, B2), FVector2D(-HalfWeb, B2), FVector2D(-HalfWeb, B1), FVector2D(-HalfLow, B1)};
	}

	/** A body of revolution about the vertical at (CX, CY): Section in (radius, height). */
	void Lathe(FMeshData& M, double CX, double CY, const TArray<FVector2D>& Section)
	{
		Solid(M, Section, Between(0.0, 2.0 * Pi, LatheSegments),
			  [CX, CY](double T, const FVector2D& Q) { return FVector(CX + Q.X * FMath::Cos(T), CY + Q.X * FMath::Sin(T), Q.Y); }, false, false);
	}

	/** A straight run along x on one side (Side −1 north, +1 south): Q is (depth in from the glass line, height). */
	void AlongSide(FMeshData& M, int32 Side, const TArray<FVector2D>& Section, TFunctionRef<double(double)> FromX,
				   TFunctionRef<double(double)> ToX)
	{
		Solid(M, Section, {0.0, 1.0}, [Side, &FromX, &ToX](double T, const FVector2D& Q)
		{
			const double Y = Side * (HW - Q.X);
			return FVector(Mix(FromX(Y), ToX(Y), T), Y, Q.Y);
		});
	}

	/** A vertical member at x on one side: Q is (along x, depth in from the glass line). */
	void Upright(FMeshData& M, int32 Side, double X, const TArray<FVector2D>& Section, double Z0, double Z1)
	{
		Solid(M, Section, {0.0, 1.0}, [Side, X, Z0, Z1](double T, const FVector2D& Q)
		{
			return FVector(X + Q.X, Side * (HW - Q.Y), Mix(Z0, Z1, T));
		});
	}

	// -------------------------------------------------------------------------------------------
	// The parts

	struct FParts
	{
		FMeshData Floor, ViewingStones, Rings;
		FMeshData Stone, Mouldings;
		FMeshData Frame, Lattice, Glass, Bronze;
	};

	/** The polished floor, through both doors, with the viewing stones on the axis. */
	void BuildFloor(FParts& Out)
	{
		const FVector Up(0, 0, 1);
		auto Plane = [&Up](FMeshData& M, const TArray<FVector2D>& Poly)
		{
			const TArray<int32> Tris = Triangulate(Poly);
			const int32 Base = M.Positions.Num();
			for (const FVector2D& P : Poly) { M.Vertex(FVector(P.X, P.Y, 0.0), Up, P); }
			for (int32 K = 0; K + 2 < Tris.Num(); K += 3) { M.Tri(Base + Tris[K], Base + Tris[K + 1], Base + Tris[K + 2]); }
		};

		// West: through the Rotunda's east door from the sun clock's edge (its own vertices), turning
		// inside the drum wall beyond the jambs.
		{
			const double Step = 2.0 * Pi / H::SunClockSegments;
			const int32 Span = FMath::CeilToInt(FMath::Asin(H::RotundaDoorHalfWidth / H::SunClockRadius) / Step) + 1;
			const double XIn = FMath::Sqrt(InRotundaWall * InRotundaWall - FloorEdge * FloorEdge);
			TArray<FVector2D> West;
			for (int32 I = -Span; I <= Span; ++I) { West.Add(FVector2D(FMath::Cos(I * Step), FMath::Sin(I * Step)) * H::SunClockRadius); }
			West.Append({FVector2D(XIn, FloorEdge), FVector2D(FloorWestX, FloorEdge), FVector2D(FloorWestX, StoneCell),
						 FVector2D(FloorWestX, -StoneCell), FVector2D(FloorWestX, -FloorEdge), FVector2D(XIn, -FloorEdge)});
			Plane(Out.Floor, West);
		}
		// East: through the Atrium's door to its floor's edge: the export's 96-gon, and where the jambs' lines
		// (y ±2) cross it, as AAtriumBaseStructure's doorway floor has them (the same arithmetic, so the same points).
		{
			const double Step = 2.0 * Pi / H::AtriumFloorSegments;
			const int32 Half = H::AtriumFloorSegments / 2;
			const int32 Span = FMath::CeilToInt(FMath::Asin(H::AtriumDoorHalfWidth / H::AtriumFloorRadius) / Step) + 1;
			const double XIn = E::CentreX - FMath::Sqrt(InAtriumWall * InAtriumWall - FloorEdge * FloorEdge);
			const FVector2D Hub(E::CentreX, E::CentreY);
			auto Poly = [&Hub, Step](int32 I) { return Hub + FVector2D(FMath::Cos(I * Step), FMath::Sin(I * Step)) * H::AtriumFloorRadius; };
			TArray<FVector2D> East = {FVector2D(FloorEastX, -FloorEdge), FVector2D(FloorEastX, -StoneCell), FVector2D(FloorEastX, StoneCell),
									  FVector2D(FloorEastX, FloorEdge), FVector2D(XIn, FloorEdge)};
			for (int32 I = Half - Span; I <= Half + Span; ++I)
			{
				East.Add(Poly(I));
				if (I == Half + Span) { break; }
				// A jamb's line crossing this edge (going north, y falls).
				const FVector2D P = Poly(I), Q = Poly(I + 1);
				for (const double Y : {H::AtriumDoorHalfWidth, -H::AtriumDoorHalfWidth})
				{
					const double DP = P.Y - Hub.Y - Y, DQ = Q.Y - Hub.Y - Y;
					if (DP * DQ < 0.0) { East.Add(P + (Q - P) * (DP / (DP - DQ))); }
				}
			}
			East.Add(FVector2D(XIn, -FloorEdge));
			Plane(Out.Floor, East);
		}
		// The hall: a grid cut round each viewing stone's square. Each square's sides carry SquareSteps points
		// (for the ring to be zipped to), and the cells beside it carry the same points on their shared sides.
		TArray<FVector2D> SquarePoints;
		auto SquareLoop = [](double S)
		{
			const FVector2D Corners[4] = {FVector2D(StoneCell, StoneCell), FVector2D(-StoneCell, StoneCell), FVector2D(-StoneCell, -StoneCell),
										  FVector2D(StoneCell, -StoneCell)};
			TArray<FVector2D> Loop;
			for (int32 C = 0; C < 4; ++C)
			{
				for (int32 K = 0; K < SquareSteps; ++K)
				{
					const FVector2D Q = Corners[C] + (Corners[(C + 1) % 4] - Corners[C]) * (double(K) / SquareSteps);
					Loop.Add(FVector2D(S + Q.X, Q.Y));
				}
			}
			return Loop;
		};
		for (const double S : H::ViewingStones) { SquarePoints.Append(SquareLoop(S)); }
		auto Cell = [&SquarePoints](double X0, double X1, double Y0, double Y1)
		{
			const FVector2D Corners[4] = {FVector2D(X0, Y0), FVector2D(X1, Y0), FVector2D(X1, Y1), FVector2D(X0, Y1)};
			TArray<FVector2D> Poly;
			for (int32 C = 0; C < 4; ++C)
			{
				const FVector2D From = Corners[C], To = Corners[(C + 1) % 4];
				Poly.Add(From);
				const FVector2D Dir = To - From;
				const double Len2 = Dir.SizeSquared();
				TArray<TPair<double, FVector2D>> OnSide;
				for (const FVector2D& Q : SquarePoints)
				{
					const double T = FVector2D::DotProduct(Q - From, Dir) / Len2;
					if (T > 1e-9 && T < 1.0 - 1e-9 && FMath::Abs(Cross2(Dir, Q - From)) < 1e-9) { OnSide.Add(TPair<double, FVector2D>(T, Q)); }
				}
				OnSide.Sort([](const TPair<double, FVector2D>& L, const TPair<double, FVector2D>& R) { return L.Key < R.Key; });
				for (const TPair<double, FVector2D>& Q : OnSide) { Poly.Add(Q.Value); }
			}
			return Poly;
		};
		TArray<double> Xs = {FloorWestX, FloorEastX};
		for (const double S : H::ViewingStones) { Xs.Append({S - StoneCell, S + StoneCell}); }
		Xs.Sort();
		const TArray<double> Ys = {-FloorEdge, -StoneCell, StoneCell, FloorEdge};
		for (int32 I = 0; I + 1 < Xs.Num(); ++I)
		{
			for (int32 J = 0; J + 1 < Ys.Num(); ++J)
			{
				const double X0 = Xs[I], X1 = Xs[I + 1], Y0 = Ys[J], Y1 = Ys[J + 1];
				bool bStone = false;
				for (const double S : H::ViewingStones) { bStone |= FMath::Abs(0.5 * (X0 + X1) - S) < StoneCell && FMath::Abs(0.5 * (Y0 + Y1)) < StoneCell; }
				if (!bStone) { Plane(Out.Floor, Cell(X0, X1, Y0, Y1)); }
			}
		}
		// Each viewing stone: a honed disc, its bronze ring, and the floor between the ring and its square.
		for (const double S : H::ViewingStones)
		{
			auto Circle = [S](double Radius, int32 K)
			{
				const double T = 2.0 * Pi * (K % DiscSegments) / DiscSegments;
				return FVector(S + Radius * FMath::Cos(T), Radius * FMath::Sin(T), 0.0);
			};
			auto PlanUV = [](const FVector& P) { return FVector2D(P.X, P.Y); };
			const int32 Hub = Out.ViewingStones.Vertex(FVector(S, 0.0, 0.0), Up, FVector2D(S, 0.0));
			for (int32 K = 0; K < DiscSegments; ++K) { const FVector P = Circle(StoneDisc, K); Out.ViewingStones.Vertex(P, Up, PlanUV(P)); }
			for (int32 K = 0; K < DiscSegments; ++K) { Out.ViewingStones.Tri(Hub, Hub + 1 + K, Hub + 1 + (K + 1) % DiscSegments); }
			const int32 RingBase = Out.Rings.Positions.Num();
			for (int32 K = 0; K < DiscSegments; ++K)
			{
				const FVector P = Circle(StoneDisc, K), Q = Circle(StoneRing, K);
				Out.Rings.Vertex(P, Up, PlanUV(P));
				Out.Rings.Vertex(Q, Up, PlanUV(Q));
			}
			for (int32 K = 0; K < DiscSegments; ++K)
			{
				const int32 A = RingBase + 2 * K, B = RingBase + 2 * ((K + 1) % DiscSegments);
				Out.Rings.Quad(A, B, B + 1, A + 1);
			}
			// Zip the square's points and the ring's by angle: each square point sees the ring points between
			// its neighbours' angles (at most 14° apart), so no triangle crosses the ring.
			struct FLoopPoint
			{
				double Angle;
				int32 Index;
			};
			TArray<FLoopPoint> Square, Ring;
			for (const FVector2D& Q : SquareLoop(S))
			{
				double A = FMath::Atan2(Q.Y, Q.X - S);
				if (A < 0.0) { A += 2.0 * Pi; }
				Square.Add({A, Out.Floor.Vertex(FVector(Q.X, Q.Y, 0.0), Up, Q)});
			}
			for (int32 K = 0; K < DiscSegments; ++K)
			{
				const FVector P = Circle(StoneRing, K);
				Ring.Add({2.0 * Pi * K / DiscSegments, Out.Floor.Vertex(P, Up, PlanUV(P))});
			}
			Square.Sort([](const FLoopPoint& L, const FLoopPoint& R) { return L.Angle < R.Angle; });
			Square.Add({Square[0].Angle + 2.0 * Pi, Square[0].Index});
			Ring.Add({Ring[0].Angle + 2.0 * Pi, Ring[0].Index});
			int32 I = 0, J = 0;
			while (I + 1 < Square.Num() || J + 1 < Ring.Num())
			{
				if (J + 1 >= Ring.Num() || (I + 1 < Square.Num() && Square[I + 1].Angle <= Ring[J + 1].Angle))
				{
					Out.Floor.Tri(Square[I].Index, Square[I + 1].Index, Ring[J].Index);
					++I;
				}
				else
				{
					Out.Floor.Tri(Square[I].Index, Ring[J + 1].Index, Ring[J].Index);
					++J;
				}
			}
		}
	}

	/** The plinths under the glass and the stone planter edges outside them, along the gardens. */
	void BuildPlinths(FParts& Out)
	{
		const TArray<FVector2D> Plinth = {FVector2D(-0.18, -0.10), FVector2D(0.18, -0.10), FVector2D(0.18, 0.29), FVector2D(0.17, 0.30),
										  FVector2D(-0.17, 0.30), FVector2D(-0.18, 0.29)};
		const TArray<FVector2D> Kerb = {FVector2D(-0.30, -0.12), FVector2D(-0.17, -0.12), FVector2D(-0.17, 0.06), FVector2D(-0.29, 0.06),
										FVector2D(-0.30, 0.05)};
		for (const int32 Side : {-1, 1})
		{
			AlongSide(Out.Stone, Side, Plinth, [](double Y) { return RotundaFaceX(Y) - PlinthEmbed; },
					  [](double Y) { return AtriumFaceX(Y, false) + PlinthEmbed; });
			AlongSide(Out.Stone, Side, Kerb, [](double Y) { return RotundaFaceX(Y) - KerbEmbed; },
					  [](double Y) { return AtriumFaceX(Y, false) + KerbEmbed; });
		}
	}

	/** The glass: the walls from the plinth to the eaves, and the vault's strips, trimmed to the drums. */
	void BuildGlass(FParts& Out)
	{
		FMeshData& M = Out.Glass;
		const double C = CopingAngle(), S = SpringAngle();
		const TArray<double> Th = StripAngles();
		auto Inward = [](double Theta) { return FVector(0.0, -FMath::Sin(Theta), -FMath::Cos(Theta)); };
		auto UV = [](double X, double Theta) { return FVector2D(X, VR * Theta); };
		auto Add = [&M, &Inward, &UV](double X, double Theta)
		{
			return M.Vertex(VaultPoint(X, Theta, VR), Inward(Theta), UV(X, Theta));
		};
		for (int32 I = 0; I + 1 < Th.Num(); ++I)
		{
			const double T0 = Th[I], T1 = Th[I + 1];
			const bool bDrum = FMath::Abs(0.5 * (T0 + T1)) < C;
			auto WestAt = [](double Theta) { return WestEnd(VR * FMath::Sin(Theta)); };
			auto EastAt = [bDrum](double Theta) { return GlassEastEnd(VR * FMath::Sin(Theta), bDrum); };
			const int32 W0 = Add(WestAt(T0), T0), E0 = Add(EastAt(T0), T0), E1 = Add(EastAt(T1), T1), W1 = Add(WestAt(T1), T1);
			// A strip against the drum whose edge lies on the coping's line also passes through the point
			// where the strip below it (against the stone) ends: split its edge there, fanning from its far corner.
			const bool bSplit0 = bDrum && FMath::Abs(FMath::Abs(T0) - C) < 1e-9, bSplit1 = bDrum && FMath::Abs(FMath::Abs(T1) - C) < 1e-9;
			if (bSplit0)
			{
				const int32 Mid = Add(GlassEastEnd(VR * FMath::Sin(T0), false), T0);
				M.Tri(E1, W1, W0);
				M.Tri(E1, W0, Mid);
				M.Tri(E1, Mid, E0);
			}
			else if (bSplit1)
			{
				const int32 Mid = Add(GlassEastEnd(VR * FMath::Sin(T1), false), T1);
				M.Tri(E0, W0, W1);
				M.Tri(E0, W1, Mid);
				M.Tri(E0, Mid, E1);
			}
			else
			{
				M.Quad(W0, E0, E1, W1);
			}
		}
		// The walls, from 1 cm into the plinth up to the springing (sharing the vault's first and last points).
		for (const int32 Side : {-1, 1})
		{
			const double Theta = Side * S;
			const FVector TopW = VaultPoint(WestEnd(VR * FMath::Sin(Theta)), Theta, VR), TopE = VaultPoint(GlassEastEnd(VR * FMath::Sin(Theta), false), Theta, VR);
			const FVector In(0.0, -Side, 0.0);
			const FVector BotW(TopW.X, TopW.Y, GlassFoot), BotE(TopE.X, TopE.Y, GlassFoot);
			auto WallUV = [Side](const FVector& P) { return FVector2D(P.X * Side, -P.Z); };
			const int32 A = M.Vertex(BotW, In, WallUV(BotW)), B = M.Vertex(BotE, In, WallUV(BotE));
			const int32 Cc = M.Vertex(TopE, In, WallUV(TopE)), D = M.Vertex(TopW, In, WallUV(TopW));
			M.Quad(A, B, Cc, D);
		}
	}

	/** The walls' steel: posts, transoms, glazing shoes, end posts and eaves. */
	void BuildWallFrame(FParts& Out)
	{
		FMeshData& M = Out.Frame;
		// Posts: an I-section, its cap 1–3 cm outside the glass, its web through it, its flange inside.
		const TArray<FVector2D> Post = ISection(0.03, 0.01, 0.03, -0.03, -0.01, 0.14, 0.155);
		const TArray<FVector2D> Transom = Rect(0.012, 0.11, H::TransomHeight - TransomHalf, H::TransomHeight + TransomHalf);
		const TArray<FVector2D> Shoe = Rect(-0.025, 0.025, GlassFoot, ShoeTop);
		const TArray<FVector2D> Eaves = {FVector2D(-0.10, EavesBottom), FVector2D(0.20, EavesBottom), FVector2D(0.20, 5.05), FVector2D(0.15, EavesTop),
										 FVector2D(-0.10, EavesTop)};
		const TArray<FVector2D> EndPost = Rect(-Seat, 0.07, -0.04, 0.12);
		for (const int32 Side : {-1, 1})
		{
			for (int32 K = 0; K < H::Posts; ++K) { Upright(M, Side, PostX(K), Post, PostFoot, PostTop); }
			// The transom runs through the posts' webs (inside their flanges), from stone to stone.
			AlongSide(M, Side, Transom, [](double Y) { return WestEnd(Y); }, [](double Y) { return EastEnd(Y, false); });
			AlongSide(M, Side, Shoe, [](double Y) { return WestEnd(Y); }, [](double Y) { return EastEnd(Y, false); });
			AlongSide(M, Side, Eaves, [](double Y) { return WestEnd(Y); }, [](double Y) { return EastEnd(Y, false); });
			// End posts against the drums, their backs 1.4 cm into the stone.
			Solid(M, EndPost, {0.0, 1.0}, [Side](double T, const FVector2D& Q)
			{
				const double Y = Side * (HW - Q.Y);
				return FVector(RotundaFaceX(Y) + Q.X, Y, Mix(PostFoot, PostTop, T));
			});
			Solid(M, EndPost, {0.0, 1.0}, [Side](double T, const FVector2D& Q)
			{
				const double Y = Side * (HW - Q.Y);
				return FVector(AtriumFaceX(Y, false) - Q.X, Y, Mix(PostFoot, PostTop, T));
			});
		}
	}

	/** The vault's steel: arches, purlins, a cross in every panel, and the collars at the drums. */
	void BuildLattice(FParts& Out)
	{
		FMeshData& M = Out.Lattice;
		const double C = CopingAngle(), S = SpringAngle();
		const TArray<double> Strips = StripAngles();
		const TArray<double> Purlins = PurlinAngles();

		// Arches: an I-section (B out from the glass): flange 15–13 cm under it, the web through it, a cap 2.5–4.5 cm over it.
		const TArray<FVector2D> Arch = ISection(0.035, 0.012, 0.035, -0.15, -0.13, 0.025, 0.045);
		for (int32 K = 0; K < H::Arches; ++K)
		{
			const double X = ArchX(K);
			Solid(M, Arch, Strips, [X](double Theta, const FVector2D& Q) { return VaultPoint(X + Q.X, Theta, VR + Q.Y); });
		}

		// Purlins (A across, B out from the glass), from drum to drum. Those on the coping's lines lie along
		// the Atrium's coping into the drum's glass, and cover where the collars change.
		const TArray<FVector2D> Purlin = Rect(-0.03, 0.03, -0.12, 0.015);
		for (int32 K = 1; K + 1 < Purlins.Num(); ++K)
		{
			const double Theta = Purlins[K];
			const bool bDrum = FMath::Abs(Theta) < C + 1e-9;
			const FVector U(0.0, FMath::Sin(Theta), FMath::Cos(Theta)), Across(0.0, FMath::Cos(Theta), -FMath::Sin(Theta));
			Solid(M, Purlin, {0.0, 1.0}, [&U, &Across, bDrum](double T, const FVector2D& Q)
			{
				const FVector P = FVector(0.0, 0.0, VC) + U * (VR + Q.Y) + Across * Q.X;
				return FVector(Mix(WestEnd(P.Y), EastEnd(P.Y, bDrum), T), P.Y, P.Z);
			});
		}

		// A cross of flat bars under the glass in every panel between two arches; one bar 1 cm deeper than the other.
		const TArray<FVector2D> BarA = Rect(-0.015, 0.015, -0.08, -0.02), BarB = Rect(-0.015, 0.015, -0.09, -0.03);
		auto Diagonal = [&M](const TArray<FVector2D>& Bar, double X0, double T0, double X1, double T1)
		{
			const double Trim = BarClear / FMath::Sqrt(FMath::Square(X1 - X0) + FMath::Square(VR * (T1 - T0)));
			Solid(M, Bar, Between(Trim, 1.0 - Trim, 8), [X0, T0, X1, T1](double T, const FVector2D& Q)
			{
				const double X = Mix(X0, X1, T), Theta = Mix(T0, T1, T);
				const FVector Tangent = FVector(X1 - X0, VR * FMath::Cos(Theta) * (T1 - T0), -VR * FMath::Sin(Theta) * (T1 - T0)).GetSafeNormal();
				const FVector U(0.0, FMath::Sin(Theta), FMath::Cos(Theta));
				return VaultPoint(X, Theta, VR + Q.Y) + FVector::CrossProduct(U, Tangent).GetSafeNormal() * Q.X;
			});
		};
		for (int32 I = 0; I + 1 < H::Arches; ++I)
		{
			for (int32 K = 0; K + 1 < Purlins.Num(); ++K)
			{
				Diagonal(BarA, ArchX(I), Purlins[K], ArchX(I + 1), Purlins[K + 1]);
				Diagonal(BarB, ArchX(I), Purlins[K + 1], ArchX(I + 1), Purlins[K]);
			}
		}

		// Collars where the vault meets the drums (A in from the face, B out from the glass): under the glass's edge,
		// 1.4 cm into the stone; at the Atrium, into the stone base below the coping and 1.2 cm short of the glass above it.
		const TArray<FVector2D> Collar = Rect(-Seat, 0.08, -0.11, -0.012);
		const TArray<FVector2D> DrumCollar = Rect(DrumGap, 0.08, -0.11, -0.012);
		Solid(M, Collar, Strips, [](double Theta, const FVector2D& Q)
		{
			const FVector P = VaultPoint(0.0, Theta, VR + Q.Y);
			return FVector(RotundaFaceX(P.Y) + Q.X, P.Y, P.Z);
		});
		auto EastCollar = [&M, &Strips](const TArray<FVector2D>& Section, double From, double To, bool bDrum)
		{
			TArray<double> Ts;
			for (const double Theta : Strips)
			{
				if (Theta >= From - 1e-9 && Theta <= To + 1e-9) { Ts.Add(Theta); }
			}
			Solid(M, Section, Ts, [bDrum](double Theta, const FVector2D& Q)
			{
				const FVector P = VaultPoint(0.0, Theta, VR + Q.Y);
				return FVector(AtriumFaceX(P.Y, bDrum) - Q.X, P.Y, P.Z);
			});
		};
		EastCollar(Collar, -S, -C, false);
		EastCollar(DrumCollar, -C, C, true);
		EastCollar(Collar, C, S, false);
	}

	/** The two doors' architraves: round the Rotunda's east door on its drum, round the Atrium's west door on its base. */
	void BuildDoorSurrounds(FParts& Out)
	{
		// (s out from the opening's edge, p proud of the wall): a quirk, two fasciae, an ovolo and a back band, 1.5 cm into the wall.
		TArray<FVector2D> Section = {FVector2D(0.29, -0.015), FVector2D(0.012, -0.015), FVector2D(0.012, 0.024), FVector2D(0.10, 0.030),
									 FVector2D(0.10, 0.040), FVector2D(0.19, 0.046), FVector2D(0.19, 0.054), FVector2D(0.205, 0.054)};
		for (int32 K = 1; K <= 6; ++K)
		{
			const double A = Pi - 0.5 * Pi * K / 6;   // the ovolo, a quarter round out to the back band
			Section.Add(FVector2D(0.245 + 0.04 * FMath::Cos(A), 0.054 + 0.04 * FMath::Sin(A)));
		}
		Section.Add(FVector2D(0.29, 0.094));
		const double Floor = -0.01;

		// The Rotunda's door: up the north jamb (T −1…0), round the arch (0…π), down the south jamb.
		{
			const double Half = H::RotundaDoorHalfWidth, Spring = MuseePlan::Rotunda::DoorSpring;
			TArray<double> Ts = {-1.0};
			Ts.Append(Between(0.0, Pi, ArchSegments));
			Ts.Add(Pi + 1.0);
			Solid(Out.Mouldings, Section, Ts, [Half, Spring, Floor](double T, const FVector2D& Q)
			{
				const double Reach = Half + Q.X;
				double Lateral, Z;
				if (T < 0.0) { Lateral = -Reach; Z = Mix(Floor, Spring, T + 1.0); }
				else if (T > Pi) { Lateral = Reach; Z = Mix(Spring, Floor, T - Pi); }
				else { Lateral = -Reach * FMath::Cos(T); Z = Spring + Reach * FMath::Sin(T); }
				const double Radius = H::RotundaOuterRadius + Q.Y;
				return FVector(FMath::Sqrt(Radius * Radius - Lateral * Lateral), Lateral, Z);
			});
		}
		// The Atrium's door: square-headed, mitred at the corners (three runs sharing their mitres).
		{
			const double Half = H::AtriumDoorHalfWidth, Head = E::DoorHeight;
			auto OnBase = [](double Lateral, double Z, double Proud)
			{
				const double Radius = H::AtriumOuterRadius + Proud;
				return FVector(E::CentreX - FMath::Sqrt(Radius * Radius - Lateral * Lateral), E::CentreY + Lateral, Z);
			};
			Solid(Out.Mouldings, Section, {0.0, 1.0}, [&OnBase, Half, Head, Floor](double T, const FVector2D& Q)
			{
				return OnBase(-(Half + Q.X), Mix(Floor, Head + Q.X, T), Q.Y);
			}, true, false);
			Solid(Out.Mouldings, Section, {0.0, 1.0}, [&OnBase, Half, Head](double T, const FVector2D& Q)
			{
				return OnBase(Mix(-(Half + Q.X), Half + Q.X, T), Head + Q.X, Q.Y);
			}, false, false);
			Solid(Out.Mouldings, Section, {0.0, 1.0}, [&OnBase, Half, Head, Floor](double T, const FVector2D& Q)
			{
				return OnBase(Half + Q.X, Mix(Head + Q.X, Floor, T), Q.Y);
			}, false, true);
		}
	}

	// The bronze threshold across the Rotunda's east door (on the sun clock's edge) is AMuseeFurniture's now, flush, as at
	// every doorway (Furniture/MuseeFurniture.cpp).

	/** The reader over a stereo stone faces the axis: the plan direction it faces (+1 south, −1 north). */
	double ReaderFacing(double StoneY) { return StoneY < 0.0 ? 1.0 : -1.0; }

	/** The stereo stones (round drums of honed travertine) and the bronze stems under their readers. */
	void BuildStereoStones(FParts& Out)
	{
		const double R0 = H::StereoStoneRadius, Top = H::StereoStoneHeight;
		const TArray<FVector2D> Drum = {FVector2D(0.0, -0.01), FVector2D(R0, -0.01), FVector2D(R0, Top - 0.015), FVector2D(R0 - 0.015, Top),
										FVector2D(0.0, Top)};
		// The reader (Swift buildStereograph) is tilted 0.5 rad back from facing the axis, its back 3 cm behind
		// its centre at 1.32 m: a stem 1.6 cm towards its back ends inside the plate.
		const TArray<FVector2D> Stem = {FVector2D(0.0, Top - 0.01), FVector2D(0.015, Top - 0.01), FVector2D(0.015, H::StereoReaderHeight),
										FVector2D(0.0, H::StereoReaderHeight)};
		for (const auto& St : H::StereoStones)
		{
			Lathe(Out.Stone, St[0], St[1], Drum);
			Lathe(Out.Bronze, St[0], St[1] - 0.016 * ReaderFacing(St[1]), Stem);
		}
	}

	/** Each print's mount on two bronze stems on the plinth; the autochromes between bronze glazing bars. */
	void BuildStands(FParts& Out)
	{
		const TArray<FVector2D> StemBar = Rect(-0.015, 0.015, 0.098, 0.110);
		const TArray<FVector2D> Foot = Rect(-0.035, 0.035, 0.080, 0.128);   // a shoe 2 cm into the plinth and 2 cm proud
		const TArray<FVector2D> GlazingBar = Rect(-0.0125, 0.0125, 0.035, 0.055);
		const double StemTop = H::MountTop - 0.05;
		for (const int32 Side : {-1, 1})
		{
			for (int32 B = 0; B < static_cast<int32>(UE_ARRAY_COUNT(H::PrintBays)); ++B)
			{
				const double X = H::PrintBays[B];
				if (B < H::FirstAutochrome)
				{
					for (const double Dx : {-0.30, 0.30})
					{
						Upright(Out.Bronze, Side, X + Dx, StemBar, H::PlinthHeight + 0.01, StemTop);
						Upright(Out.Bronze, Side, X + Dx, Foot, H::PlinthHeight - 0.02, H::PlinthHeight + 0.02);
					}
				}
				else
				{
					for (const double Dx : {-0.56, 0.56}) { Upright(Out.Bronze, Side, X + Dx, GlazingBar, GlassFoot, H::TransomHeight); }
				}
			}
			// The path lights' louvres, in the plinth under each post.
			for (int32 K = 0; K < H::Posts; ++K) { Upright(Out.Bronze, Side, PostX(K), Rect(-0.05, 0.05, 0.17, 0.19), 0.11, 0.17); }
		}
	}

	FParts BuildAll()
	{
		FParts Parts;
		BuildFloor(Parts);
		BuildPlinths(Parts);
		BuildGlass(Parts);
		BuildWallFrame(Parts);
		BuildLattice(Parts);
		BuildDoorSurrounds(Parts);
		BuildStereoStones(Parts);
		BuildStands(Parts);
		return Parts;
	}

	struct FAim
	{
		FVector From;
		FVector To;
	};

	/** Uplights on the eaves, one in every other arch bay (x 11.75 … 38.75, clear of the arches' feet), aimed up across the vault. */
	TArray<FAim> UplightAims()
	{
		TArray<FAim> Out;
		for (const int32 Side : {-1, 1})
		{
			for (int32 K = 0; K < H::Posts; ++K)
			{
				const double X = ArchX(2 * K) - 0.5 * H::ArchStep;
				Out.Add({FVector(X, Side * (HW - UplightInset), UplightZ), FVector(X, -Side * UplightAimY, UplightAimZ)});
			}
		}
		return Out;
	}

	/** Path lights low in the plinths under the posts, aimed across the floor. */
	TArray<FAim> PathLightAims()
	{
		TArray<FAim> Out;
		for (const int32 Side : {-1, 1})
		{
			for (int32 K = 0; K < H::Posts; ++K)
			{
				const double X = PostX(K);
				Out.Add({FVector(X, Side * (HW - PathInset), PathZ), FVector(X, -Side * PathAimY, 0.0)});
			}
		}
		return Out;
	}
}

namespace HLB = HallOfLightBuild;

AHallOfLightStructure::AHallOfLightStructure()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 1.0f;
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
	Floor = Make(TEXT("Floor"), true);
	Stone = Make(TEXT("Stone"), true);
	Frame = Make(TEXT("Frame"), true);
	Lattice = Make(TEXT("Lattice"), false);
	Glass = Make(TEXT("Glass"), true);
	Glass->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);   // glass: the look trace goes through
	Glass->SetCastShadow(false);
	Bronze = Make(TEXT("Bronze"), false);

	for (int32 I = 0; I < 2 * MuseePlan::HallOfLight::Posts; ++I)
	{
		for (const bool bPath : {false, true})
		{
			USpotLightComponent* L = CreateDefaultSubobject<USpotLightComponent>(*FString::Printf(TEXT("%s%02d"), bPath ? TEXT("PathLight") : TEXT("Uplight"), I + 1));
			L->SetupAttachment(RootComponent);
			L->SetMobility(EComponentMobility::Movable);
			(bPath ? PathLights : SpringingUplights).Add(L);
		}
	}

	auto Path = [](const TCHAR* Name) { return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(FString::Printf(TEXT("/Game/Museum/Materials/%s.%s"), Name, Name))); };
	FloorMaterial = Path(TEXT("M_Travertine_Polished"));
	StoneMaterial = Path(TEXT("M_Travertine_Honed"));
	MouldingMaterial = Path(TEXT("M_Plaster_Moulding"));
	SteelMaterial = Path(TEXT("M_RibPearl"));
	GlassMaterial = Path(TEXT("M_Glass"));
	BronzeMaterial = Path(TEXT("M_Bronze_Statuary"));   // materials.py (M_Metal with BronzeColour, BronzeRoughness, M1)
	Parameters = TSoftObjectPtr<UMaterialParameterCollection>(FSoftObjectPath(TEXT("/Game/Museum/Materials/MPC_Musee.MPC_Musee")));

	AddTags();
	PlaceLights();
}

TArray<FString> AHallOfLightStructure::GetReplacedImportPrims()
{
	return {
		TEXT("/Museum/HallOfLight/Hall_of_Light_floor"),
		TEXT("/Museum/HallOfLight/Hall_of_Light_plinths"),
		TEXT("/Museum/HallOfLight/Hall_of_Light_glass"),
		// Posts, eaves and the vault's lattice were one mesh.
		TEXT("/Museum/HallOfLight/Hall_of_Light_lattice"),
		TEXT("/Museum/HallOfLight/Stereo_stones"),
	};
}

TArray<FVector> AHallOfLightStructure::GetViewingStoneCentres() const
{
	TArray<FVector> Out;
	for (const double S : MuseePlan::HallOfLight::ViewingStones) { Out.Add(GetActorTransform().TransformPosition(FVector(S, 0.0, 0.0) * MuseePlan::Cm)); }
	return Out;
}

void AHallOfLightStructure::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
	ApplyMaterials();
	PlaceLights();
	AddTags();
}

void AHallOfLightStructure::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	PlaceLights();
	AddTags();
}

void AHallOfLightStructure::BeginPlay()
{
	Super::BeginPlay();
	// A placed actor doesn't rerun its construction when the map loads: rebuild, so the map never shows an older build.
	Build();
	ApplyMaterials();
	PlaceLights();
	NightLevel = -1.f;
	Tick(0.f);
}

void AHallOfLightStructure::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld()) { return; }   // the editor keeps the lamps as placed
	float Daylight = 0.f;
	if (UMaterialParameterCollection* Collection = Parameters.LoadSynchronous())
	{
		Daylight = UKismetMaterialLibrary::GetScalarParameterValue(this, Collection, TEXT("Daylight"));
	}
	const float Level = 1.f - FMath::SmoothStep(NightLightsFadeFrom, FMath::Max(NightLightsOffAt, NightLightsFadeFrom + 0.01f), Daylight);
	if (FMath::Abs(Level - NightLevel) > 0.01f || (Level == 0.f) != (NightLevel == 0.f)) { SetNightLevel(Level); }
}

void AHallOfLightStructure::SetNightLevel(float Level)
{
	NightLevel = Level;
	auto Set = [Level](USpotLightComponent* L, float Candela)
	{
		if (!L) { return; }
		L->SetIntensity(Candela * Level);
		L->SetVisibility(Level > 0.001f);
	};
	for (USpotLightComponent* L : SpringingUplights) { Set(L, UplightCandela); }
	for (USpotLightComponent* L : PathLights) { Set(L, PathLightCandela); }
}

void AHallOfLightStructure::AddTags()
{
	// Part of the building (hidden in the Sphere with the rest). The night lights follow the daylight
	// themselves (the inverse of a laylight), so no musee.laylight.
	Tags.AddUnique(FName(TEXT("musee.building")));
	Tags.AddUnique(FName(TEXT("musee.wing:HallOfLight")));
	Tags.AddUnique(MuseeBake::BakeableTag());   // it ticks only to dim its lamps: its meshes never move
}

void AHallOfLightStructure::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }   // baked into static meshes (Geometry/MuseeBake.h)
	const HLB::FParts P = HLB::BuildAll();
	for (UProceduralMeshComponent* Component : {Floor.Get(), Stone.Get(), Frame.Get(), Lattice.Get(), Glass.Get(), Bronze.Get()})
	{
		if (Component) { Component->ClearAllMeshSections(); }
	}
	P.Floor.Write(Floor, 0, true);
	P.ViewingStones.Write(Floor, 1, true);
	P.Rings.Write(Floor, 2, true);
	P.Stone.Write(Stone, 0, true);
	P.Mouldings.Write(Stone, 1, true);
	P.Frame.Write(Frame, 0, true);
	P.Lattice.Write(Lattice, 0, false);
	P.Glass.Write(Glass, 0, true);
	P.Bronze.Write(Bronze, 0, false);
}

void AHallOfLightStructure::ApplyMaterials()
{
	auto Set = [](UProceduralMeshComponent* Component, int32 Section, const TSoftObjectPtr<UMaterialInterface>& Ref)
	{
		if (!Component || Ref.IsNull()) { return; }
		if (UMaterialInterface* Material = Ref.LoadSynchronous()) { Component->SetMaterial(Section, Material); }
	};
	Set(Floor, 0, FloorMaterial);
	Set(Floor, 1, StoneMaterial);
	Set(Stone, 0, StoneMaterial);
	Set(Stone, 1, MouldingMaterial);
	Set(Frame, 0, SteelMaterial);
	Set(Lattice, 0, SteelMaterial);
	Set(Glass, 0, GlassMaterial);
	// Statuary bronze: materials.py's saved M_Bronze_Statuary (a saved instance lets the bake take the floor and the bronze
	// into Nanite meshes with Lumen cards; a run-time instance kept both procedural, without a surface cache).
	UMaterialInterface* Parent = BronzeMaterial.IsNull() ? nullptr : BronzeMaterial.LoadSynchronous();
	if (Parent && Parent->IsA<UMaterialInstanceConstant>())
	{
		if (Floor) { Floor->SetMaterial(2, Parent); }
		if (Bronze) { Bronze->SetMaterial(0, Parent); }
	}
	else if (Parent)   // the bare master (before materials.py has made the instance): M_Metal's parameters
	{
		UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Parent, this);
		Mid->SetVectorParameterValue(TEXT("BaseColor"), BronzeColour);
		Mid->SetScalarParameterValue(TEXT("Roughness"), BronzeRoughness);
		Mid->SetScalarParameterValue(TEXT("Metallic"), 1.f);
		Mid->SetScalarParameterValue(TEXT("Variation"), 0.08f);
		Mid->SetScalarParameterValue(TEXT("PatinaAmount"), 0.05f);
		if (Floor) { Floor->SetMaterial(2, Mid); }
		if (Bronze) { Bronze->SetMaterial(0, Mid); }
	}
}

void AHallOfLightStructure::PlaceLights()
{
	auto Spot = [this](USpotLightComponent* L, const HLB::FAim& Aim, float Candela, float Inner, float Outer, float RangeCm, float SourceCm, bool bShadows)
	{
		if (!L) { return; }
		L->SetRelativeLocationAndRotation(Aim.From * MuseePlan::Cm, FRotationMatrix::MakeFromX(Aim.To - Aim.From).Rotator());
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(Candela * FMath::Max(NightLevel, 0.f));
		L->SetUseTemperature(true);
		L->SetTemperature(LightKelvin);
		L->SetLightColor(FLinearColor::White);
		L->SetInnerConeAngle(Inner);
		L->SetOuterConeAngle(Outer);
		L->SetAttenuationRadius(RangeCm);
		L->SetSourceRadius(SourceCm);
		L->SetSoftSourceRadius(0.f);
		L->SetCastShadows(bShadows);
	};
	const TArray<HLB::FAim> Up = HLB::UplightAims(), Low = HLB::PathLightAims();
	for (int32 I = 0; I < SpringingUplights.Num() && I < Up.Num(); ++I)
	{
		Spot(SpringingUplights[I], Up[I], UplightCandela, 15.f, 40.f, 1200.f, 3.f, bUplightShadows);
	}
	for (int32 I = 0; I < PathLights.Num() && I < Low.Num(); ++I)
	{
		Spot(PathLights[I], Low[I], PathLightCandela, 20.f, 45.f, 900.f, 2.f, bPathLightShadows);
	}
}
