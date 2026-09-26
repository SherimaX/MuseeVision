#include "SculptureHall/SculptureHallStructure.h"

#include "Geometry/MuseeBake.h"
#include "Materials/MaterialInterface.h"
#include "Plan/MuseePlan.h"
#include "ProceduralMeshComponent.h"
#include "Salon/SalonKit.h"

#if !UE_BUILD_SHIPPING
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/DelayedAutoRegister.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#endif

/**
 * The Sculpture Hall in plan metres (x east, plan y south, height up; the Rotunda's centre is the origin), from
 * MuseePlan::SculptureHall and Shared/Wings/SculptureHall.swift. A named namespace: the module builds in unity files.
 * Constants are prefixed k so no local can hide them.
 */
namespace SculptureHallBuild
{
	namespace SH = MuseePlan::SculptureHall;
	using SalonKit::FFrame;
	using SalonKit::FMeshData;
	using SalonKit::FProfile;

	constexpr double kPi = UE_DOUBLE_PI;

	// ---------------------------------------------------------------- The court
	constexpr double kW = SH::West, kE = SH::East, kN = SH::North, kS = SH::South;          // inner faces
	constexpr double kOW = kW - SH::Wall, kOE = kE + SH::Wall, kON = kN - SH::Wall, kOS = kS + SH::Wall;   // outer faces
	constexpr double kSink = 0.05;      // wall faces, the jambs and the architrave run this far into the ground
	constexpr double kInto = 0.002;     // a face that stops runs this far on into the solid it meets
	constexpr double kCeil = SH::CeilingHeight;   // 9.65
	constexpr double kDeck = 10.30;     // the roof deck, over the ceiling's slab
	constexpr double kTop = 10.50;      // the walls' tops (inside the coping) and the kerb's
	constexpr double kCopingHalf = SH::Wall / 2 + 0.05;   // the coping overhangs both faces by 5 cm

	// ---------------------------------------------------------------- The door (on the axis of the south wall)
	constexpr double kDoorHalf = SH::DoorWidth / 2, kSpring = SH::DoorSpring;
	constexpr int32 kArch = 48;         // stations per semicircle: the Rotunda's ArchSegments, so the reveals meet
	constexpr double kArchitrave = 0.30;
	constexpr double kDieIn = 0.26;     // where the skirting and string beads stop, out from the opening (inside the architrave)

	// ---------------------------------------------------------------- Mouldings (heights, m)
	constexpr double kLowBead = 3.20, kHighBead = 6.40, kBeadHalf = 0.02;
	constexpr double kCorniceFoot = 9.30;

	// ---------------------------------------------------------------- The laylight
	constexpr double kLW = SH::LaylightWest, kLE = SH::LaylightEast, kLN = SH::LaylightNorth, kLS = SH::LaylightSouth;
	constexpr double kLay = SH::LaylightHeight;   // 9.97: the diffuser
	constexpr double kKerb = 0.30;                // the kerb round the well
	constexpr double kKW = kLW - kKerb, kKE = kLE + kKerb, kKN = kLN - kKerb, kKS = kLS + kKerb;
	constexpr double kDiffuserInto = 0.05;        // the diffuser runs this far into the slab round the well
	constexpr double kBarHalf = 0.02, kPerimeterBar = 0.05, kBarBottom = 9.90;
	constexpr double kGrid = 1.0;                 // the bars on the panel texture's lines (one tile per metre)
	constexpr double kPrintedLine = 6.0 / 256.0;  // the texture's printed line (left and bottom edges of a tile)
	constexpr double kLightDrop = 0.01;           // a rect light 1 cm under the bars

	// ---------------------------------------------------------------- The glass roof: hipped, on the kerb's centreline
	constexpr double kEaveX = kLE + kKerb / 2, kEaveN = kLN - kKerb / 2, kEaveS = kLS + kKerb / 2;   // 7.15, −29.85, −15.15
	constexpr double kMidY = 0.5 * (kLN + kLS);   // −22.5
	constexpr double kHalfX = kEaveX, kHalfY = 0.5 * (kEaveS - kEaveN);   // 7.15, 7.35
	constexpr double kRidge = 12.30;
	constexpr double kRidgeN = kMidY - (kHalfY - kHalfX), kRidgeS = kMidY + (kHalfY - kHalfX);   // −22.7, −22.3
	constexpr double kSlope = (kRidge - kTop) / kHalfX;
	constexpr double kRafterPitch = 1.1;
	constexpr double kGutterWidth = 0.13, kGutterHeight = 0.08;
	constexpr double kRidgeCapHalf = 0.04;

	const FVector kUp(0.0, 0.0, 1.0);
	const FVector kAxisX(1.0, 0.0, 0.0);
	const FVector kAxisY(0.0, 1.0, 0.0);

	struct FNamedPart
	{
		const TCHAR* Name;
		const FMeshData* Part;
	};

	/** Everything the actor writes, by section. */
	struct FParts
	{
		FMeshData WallsInner, Exterior;
		FMeshData Trim;
		FMeshData Cornice, Frame;
		FMeshData Ceiling;
		FMeshData Deck, Steel;
		FMeshData Glass;
		FMeshData Diffuser, Bars;
		FMeshData Floor;
		FMeshData Plinths, Bench;

		/** Every part with a name, for the geometry dump. */
		TArray<FNamedPart> Named() const
		{
			return {
				{TEXT("walls_inner"), &WallsInner}, {TEXT("exterior"), &Exterior}, {TEXT("trim"), &Trim},
				{TEXT("cornice"), &Cornice}, {TEXT("frame"), &Frame}, {TEXT("ceiling"), &Ceiling},
				{TEXT("deck"), &Deck}, {TEXT("steel"), &Steel}, {TEXT("glass"), &Glass},
				{TEXT("diffuser"), &Diffuser}, {TEXT("bars"), &Bars}, {TEXT("floor"), &Floor},
				{TEXT("plinths"), &Plinths}, {TEXT("bench"), &Bench},
			};
		}
	};

	// ================================================================ Small helpers

	TArray<double> SortedUnique(TArray<double> Values, double Tolerance = 1e-9)
	{
		Values.Sort();
		TArray<double> Result;
		for (double V : Values)
		{
			if (Result.Num() == 0 || V - Result.Last() > Tolerance) { Result.Add(V); }
		}
		return Result;
	}

	/** The door's edge at arch station K (0: the east springing, kArch: the west): across (x) and height. */
	void DoorStation(int32 K, double& L, double& Z)
	{
		const double Phi = kPi * K / kArch;
		L = kDoorHalf * FMath::Cos(Phi);
		Z = kSpring + kDoorHalf * FMath::Sin(Phi);
	}

	/** A vertical wall plane: At(u, z) = Origin + U u + z up; N faces the side it bounds. */
	struct FWallPlane
	{
		FVector Origin = FVector::ZeroVector;
		FVector U = FVector::ForwardVector;
		FVector N = FVector::RightVector;

		FVector At(double Along, double Z) const { return Origin + U * Along + FVector(0.0, 0.0, Z); }
	};

	FWallPlane Plane(double OX, double OY, const FVector& Along, const FVector& Facing)
	{
		FWallPlane W;
		W.Origin = FVector(OX, OY, 0.0);
		W.U = Along;
		W.N = Facing;
		return W;
	}

	/**
	 * The face of a wall over [U0, U1] × [Z0, Z1], less the round-arched door at u = 0 when bDoor. A column stands at
	 * every arch station, so the reveal meets the face vertex for vertex; beside each jamb the face carries a vertex
	 * at the springing, where the jamb's top corner is.
	 */
	void WallFace(FMeshData& M, const FWallPlane& W, double U0, double U1, double Z0, double Z1, bool bDoor)
	{
		struct FColumn
		{
			double U;
			double Head;
			bool bDoor;
			bool bJamb;
		};
		TArray<FColumn> Columns = {{U0, Z0, false, false}, {U1, Z0, false, false}};
		if (bDoor)
		{
			for (int32 K = 0; K <= kArch; ++K)
			{
				double L = 0.0, Z = 0.0;
				DoorStation(K, L, Z);
				Columns.Add({L, Z, true, K == 0 || K == kArch});
			}
		}
		Columns.Sort([](const FColumn& A, const FColumn& B) { return A.U < B.U; });
		for (int32 i = 0; i + 1 < Columns.Num(); ++i)
		{
			const FColumn& A = Columns[i];
			const FColumn& B = Columns[i + 1];
			if (B.U - A.U < 1e-9) { continue; }
			if (A.bDoor && B.bDoor)
			{
				// Over the arch.
				M.Rect(W.At(A.U, A.Head), W.At(B.U, B.Head), W.At(B.U, Z1), W.At(A.U, Z1), W.N);
			}
			else if (B.bJamb)
			{
				// Fanned from the springing beside the jamb, so no triangle is degenerate.
				M.Poly({W.At(B.U, kSpring), W.At(B.U, Z1), W.At(A.U, Z1), W.At(A.U, Z0), W.At(B.U, Z0)}, W.N);
			}
			else if (A.bJamb)
			{
				M.Poly({W.At(A.U, kSpring), W.At(A.U, Z0), W.At(B.U, Z0), W.At(B.U, Z1), W.At(A.U, Z1)}, W.N);
			}
			else
			{
				M.Rect(W.At(A.U, Z0), W.At(B.U, Z0), W.At(B.U, Z1), W.At(A.U, Z1), W.N);
			}
		}
	}

	/** Four trapezoids between two nested rectangles (x0, x1, y0 north, y1 south) at height Z, facing N. */
	void RectRing(FMeshData& M, const double Out[4], const double In[4], double Z, const FVector& N)
	{
		auto P = [Z](double X, double Y) { return FVector(X, Y, Z); };
		M.Rect(P(Out[0], Out[2]), P(Out[1], Out[2]), P(In[1], In[2]), P(In[0], In[2]), N);
		M.Rect(P(Out[1], Out[2]), P(Out[1], Out[3]), P(In[1], In[3]), P(In[1], In[2]), N);
		M.Rect(P(Out[1], Out[3]), P(Out[0], Out[3]), P(In[0], In[3]), P(In[1], In[3]), N);
		M.Rect(P(Out[0], Out[3]), P(Out[0], Out[2]), P(In[0], In[2]), P(In[0], In[3]), N);
	}

	/** The four faces of a rectangle's sides (x0, x1, y0, y1) from Z0 to Z1, facing out of it (bOut) or into it. */
	void RectSides(FMeshData& M, double X0, double X1, double Y0, double Y1, double Z0, double Z1, bool bOut)
	{
		const double F = bOut ? 1.0 : -1.0;
		M.Rect(FVector(X0, Y0, Z0), FVector(X1, Y0, Z0), FVector(X1, Y0, Z1), FVector(X0, Y0, Z1), -kAxisY * F);
		M.Rect(FVector(X1, Y0, Z0), FVector(X1, Y1, Z0), FVector(X1, Y1, Z1), FVector(X1, Y0, Z1), kAxisX * F);
		M.Rect(FVector(X1, Y1, Z0), FVector(X0, Y1, Z0), FVector(X0, Y1, Z1), FVector(X1, Y1, Z1), kAxisY * F);
		M.Rect(FVector(X0, Y1, Z0), FVector(X0, Y0, Z0), FVector(X0, Y0, Z1), FVector(X0, Y1, Z1), -kAxisX * F);
	}

	/**
	 * A closed bar of rectangular section along A → B: Width across, Height out along Up (square to the bar), its
	 * underside Drop below the line (so it sits into what it lies on).
	 */
	void Bar(FMeshData& M, const FVector& A, const FVector& B, const FVector& Up, double Width, double Height, double Drop)
	{
		const FVector D = (B - A).GetSafeNormal();
		const FVector Out = (Up - D * FVector::DotProduct(Up, D)).GetSafeNormal();
		const FVector Lat = FVector::CrossProduct(D, Out).GetSafeNormal();
		const double T0 = -Drop, T1 = Height - Drop, H = 0.5 * Width;
		auto P = [&Lat, &Out, H](const FVector& O, double Side, double T) { return O + Lat * (Side * H) + Out * T; };
		M.Rect(P(A, -1, T0), P(B, -1, T0), P(B, -1, T1), P(A, -1, T1), -Lat);
		M.Rect(P(A, 1, T0), P(B, 1, T0), P(B, 1, T1), P(A, 1, T1), Lat);
		M.Rect(P(A, -1, T1), P(B, -1, T1), P(B, 1, T1), P(A, 1, T1), Out);
		M.Rect(P(A, -1, T0), P(B, -1, T0), P(B, 1, T0), P(A, 1, T0), -Out);
		M.Rect(P(A, -1, T0), P(A, 1, T0), P(A, 1, T1), P(A, -1, T1), -D);
		M.Rect(P(B, -1, T0), P(B, 1, T0), P(B, 1, T1), P(B, -1, T1), D);
	}

	// ================================================================ Profiles (metres; A out of the surface, B up)

	/** The skirting: 18 cm of travertine, 3 cm proud, a rounded top and a fillet. */
	FProfile Skirting()
	{
		FProfile P;
		P.Add(0.030, -kInto).Add(0.030, 0.150).SmoothLast()
			.Arc(0.010, 0.150, 0.020, 0.020, 0.0, 90.0, 6)
			.Add(0.010, 0.180).Add(0.000, 0.180);
		return P;
	}

	/** A string bead: a half-round 4 cm high, 12 mm proud, centred at height Z. */
	FProfile StringBead(double Z)
	{
		FProfile P;
		P.Add(0.000, Z - kBeadHalf).Add(0.004, Z - kBeadHalf).SmoothLast()
			.Arc(0.004, Z, 0.008, kBeadHalf, -90.0, 90.0, 12).SmoothLast()
			.Add(0.000, Z + kBeadHalf);
		return P;
	}

	/** The cornice (9.30–9.65 m, 0.25 m out): a fillet, an ovolo, the corona over its soffit, a fillet and a cyma. */
	FProfile Cornice()
	{
		FProfile P;
		P.Add(0.000, kCorniceFoot).Add(0.012, kCorniceFoot).Add(0.012, 9.318)
			.Arc(0.012, 9.378, 0.060, 0.060, -90.0, 0.0, 10)
			.Add(0.082, 9.378).Add(0.082, 9.390).Add(0.150, 9.390).Add(0.150, 9.500).Add(0.162, 9.500).Add(0.162, 9.512)
			.Arc(0.162, 9.569, 0.038, 0.057, -90.0, 0.0, 8).SmoothLast()
			.Arc(0.238, 9.569, 0.038, 0.057, 180.0, 90.0, 8).SmoothLast()
			.Add(0.250, 9.626).Add(0.250, kCeil);
		return P;
	}

	/** The laylight's frame (A out from the opening, over the ceiling): a fascia and bead lining the arris, a soffit, a cavetto to the ceiling. */
	FProfile LaylightFrame()
	{
		FProfile P;
		P.Add(0.000, kCeil).Add(0.000, 9.580).SmoothLast()
			.Arc(0.020, 9.580, 0.020, 0.020, 180.0, 270.0, 6).SmoothLast()
			.Add(0.110, 9.560)
			.Arc(0.190, 9.560, 0.080, 0.080, 180.0, 90.0, 10)
			.Add(0.205, 9.640).Add(0.205, kCeil);
		return P;
	}

	/** The door's architrave (A out from the opening in the wall, B proud): a rounded back band, two fasciae, a return on the reveal. */
	FProfile Architrave()
	{
		FProfile P;
		P.Add(kArchitrave, -kInto).Add(kArchitrave, 0.120).Add(0.280, 0.120).SmoothLast()
			.Arc(0.280, 0.095, 0.025, 0.025, 90.0, 180.0, 8)
			.Add(0.200, 0.095).Add(0.200, 0.080).Add(0.130, 0.070).Add(0.130, 0.055).Add(0.025, 0.045).Add(0.000, 0.028).Add(0.000, 0.000);
		return P;
	}

	/** The coping over the walls (closed; A inward from the wall's centreline): weathered towards the roof, eased arrises. */
	FProfile Coping()
	{
		FProfile P;
		P.bClosed = true;
		const double H = kCopingHalf;
		P.Add(-H, 10.450).Add(H, 10.450).Add(H, 10.525).Add(H - 0.02, 10.545).Add(-H + 0.02, 10.560).Add(-H, 10.540);
		return P;
	}

	/** The glass roof's eaves gutter on the kerb (A out from the glass's foot), open underneath. */
	FProfile Gutter()
	{
		FProfile P;
		P.Add(kGutterWidth, kTop).Add(kGutterWidth, kTop + kGutterHeight).Add(0.000, kTop + kGutterHeight).Add(0.000, kTop);
		return P;
	}

	// ================================================================ The parts

	/** Walls: the court's inner faces (up to the coping), the outer faces, the door's reveal and the coping. */
	void BuildWalls(FParts& Out)
	{
		const double Z0 = -kSink, Z1 = kTop;
		// Inner faces, facing the court (and, above the deck, the roof).
		WallFace(Out.WallsInner, Plane(0.0, kN, kAxisX, kAxisY), kW, kE, Z0, Z1, false);
		WallFace(Out.WallsInner, Plane(kE, 0.0, kAxisY, -kAxisX), kN, kS, Z0, Z1, false);
		WallFace(Out.WallsInner, Plane(0.0, kS, kAxisX, -kAxisY), kW, kE, Z0, Z1, true);
		WallFace(Out.WallsInner, Plane(kW, 0.0, kAxisY, kAxisX), kN, kS, Z0, Z1, false);
		// Outer faces. The south one closes the end of the Rotunda's passage round the arch.
		WallFace(Out.Exterior, Plane(0.0, kON, kAxisX, -kAxisY), kOW, kOE, Z0, Z1, false);
		WallFace(Out.Exterior, Plane(kOE, 0.0, kAxisY, kAxisX), kON, kOS, Z0, Z1, false);
		WallFace(Out.Exterior, Plane(0.0, kOS, kAxisX, kAxisY), kOW, kOE, Z0, Z1, true);
		WallFace(Out.Exterior, Plane(kOW, 0.0, kAxisY, -kAxisX), kON, kOS, Z0, Z1, false);

		// The door's reveal through the wall: its jambs and soffit on the faces' stations (and the Rotunda passage's).
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const double X = Side * kDoorHalf;
			Out.WallsInner.Rect(FVector(X, kS, Z0), FVector(X, kOS, Z0), FVector(X, kOS, kSpring), FVector(X, kS, kSpring), -kAxisX * Side);
		}
		const int32 Base = Out.WallsInner.Positions.Num();
		for (int32 K = 0; K <= kArch; ++K)
		{
			double L = 0.0, Z = 0.0;
			DoorStation(K, L, Z);
			const double Phi = kPi * K / kArch;
			const FVector Nrm = -(kAxisX * FMath::Cos(Phi) + kUp * FMath::Sin(Phi));
			// UV as the Rotunda's reveal: along the axis from the origin, and the arc from the east springing.
			Out.WallsInner.Vertex(FVector(L, kS, Z), Nrm, FVector2D(-kS, kDoorHalf * Phi));
			Out.WallsInner.Vertex(FVector(L, kOS, Z), Nrm, FVector2D(-kOS, kDoorHalf * Phi));
		}
		for (int32 K = 0; K < kArch; ++K)
		{
			const int32 A = Base + 2 * K;
			Out.WallsInner.Quad(A, A + 1, A + 3, A + 2);
		}

		// The coping, round the walls' centreline (A inward), mitred at the corners.
		const double CW = kW - SH::Wall / 2, CE = kE + SH::Wall / 2, CN = kN - SH::Wall / 2, CS = kS + SH::Wall / 2;
		SalonKit::SweepPlan(Out.Exterior, {FVector2D(CW, CS), FVector2D(CW, CN), FVector2D(CE, CN), FVector2D(CE, CS)}, true, Coping());
	}

	/** Frames round the door on the court's face: up the west jamb from under the floor, over the arch, down the east. */
	TArray<FFrame> DoorFrames(const FWallPlane& W)
	{
		TArray<FFrame> Run;
		double S = 0.0;
		auto Push = [&Run, &S, &W](double L, double Z, const FVector& D)
		{
			FFrame F;
			F.Origin = W.At(L, Z);
			if (Run.Num() > 0) { S += FVector::Dist(Run.Last().Origin, F.Origin); }
			F.AxisA = F.NormA = D;
			F.AxisB = F.NormB = W.N;
			F.S = S;
			Run.Add(F);
		};
		Push(-kDoorHalf, -kSink, -W.U);
		for (int32 K = kArch; K >= 0; --K)
		{
			double L = 0.0, Z = 0.0;
			DoorStation(K, L, Z);
			const double Phi = kPi * K / kArch;
			Push(L, Z, W.U * FMath::Cos(Phi) + kUp * FMath::Sin(Phi));
		}
		Push(kDoorHalf, -kSink, W.U);
		return Run;
	}

	/** Along the court's walls from the door's west side, round the room, back to its east side (A into the room). */
	TArray<FVector2D> RoundTheRoom(double EndX)
	{
		return {FVector2D(-EndX, kS), FVector2D(kW, kS), FVector2D(kW, kN), FVector2D(kE, kN), FVector2D(kE, kS), FVector2D(EndX, kS)};
	}

	/** Where a horizontal moulding at height Z stops, dying into the architrave kDieIn out from the opening. */
	double DieInX(double Z)
	{
		const double R = kDoorHalf + kDieIn;
		if (Z <= kSpring) { return R; }
		return FMath::Sqrt(FMath::Max(0.0, R * R - (Z - kSpring) * (Z - kSpring)));
	}

	/** The skirting, the string beads and the architrave; the cornice. */
	void BuildMouldings(FParts& Out)
	{
		SalonKit::SweepPlan(Out.Trim, RoundTheRoom(DieInX(0.0)), false, Skirting());
		SalonKit::SweepPlan(Out.Trim, RoundTheRoom(DieInX(kLowBead)), false, StringBead(kLowBead));
		SalonKit::SweepPlan(Out.Trim, RoundTheRoom(DieInX(kHighBead)), false, StringBead(kHighBead));
		SalonKit::Sweep(Out.Trim, DoorFrames(Plane(0.0, kS, kAxisX, -kAxisY)), Architrave());
		SalonKit::SweepPlan(Out.Cornice, {FVector2D(kW, kS), FVector2D(kW, kN), FVector2D(kE, kN), FVector2D(kE, kS)}, true, Cornice());
	}

	/** The ceiling round the laylight, its frame, the well up to the kerb's top, the roof deck and the kerb. */
	void BuildCeilingAndRoof(FParts& Out)
	{
		const double Room[4] = {kW, kE, kN, kS};
		const double Opening[4] = {kLW, kLE, kLN, kLS};
		const double KerbOut[4] = {kKW, kKE, kKN, kKS};
		RectRing(Out.Ceiling, Room, Opening, kCeil, -kUp);
		// The well: one face each side from the ceiling to the kerb's top (under the diffuser and over it).
		RectSides(Out.Ceiling, kLW, kLE, kLN, kLS, kCeil, kTop, false);
		// The frame round the opening (A outward: the loop runs clockwise seen from above, north edge westward).
		SalonKit::SweepPlan(Out.Frame, {FVector2D(kLE, kLN), FVector2D(kLW, kLN), FVector2D(kLW, kLS), FVector2D(kLE, kLS)}, true, LaylightFrame());
		// The roof: the deck from the parapet to the kerb, the kerb's outer faces and its top.
		RectRing(Out.Deck, Room, KerbOut, kDeck, kUp);
		RectSides(Out.Exterior, kKW, kKE, kKN, kKS, kDeck - kInto, kTop, true);
		RectRing(Out.Exterior, KerbOut, Opening, kTop, kUp);
	}

	/** The diffuser (UVs on the panel texture's lines) and the bars under it. */
	void BuildLaylight(FParts& Out)
	{
		// The panel image prints its line along each tile's left (u 0) and bottom (v 1) edges: shift by half a
		// line so the lines are centred on whole metres, under the bars.
		const double Half = 0.5 * kPrintedLine;
		auto Uv = [Half](double X, double Y) { return FVector2D(X / kGrid + Half, Y / kGrid - Half); };
		const double X0 = kLW - kDiffuserInto, X1 = kLE + kDiffuserInto, Y0 = kLN - kDiffuserInto, Y1 = kLS + kDiffuserInto;
		const int32 A = Out.Diffuser.Vertex(FVector(X0, Y0, kLay), -kUp, Uv(X0, Y0));
		const int32 B = Out.Diffuser.Vertex(FVector(X1, Y0, kLay), -kUp, Uv(X1, Y0));
		const int32 C = Out.Diffuser.Vertex(FVector(X1, Y1, kLay), -kUp, Uv(X1, Y1));
		const int32 D = Out.Diffuser.Vertex(FVector(X0, Y1, kLay), -kUp, Uv(X0, Y1));
		Out.Diffuser.Quad(A, B, C, D);

		// The bars: a perimeter against the well, then 4 cm bars on every whole metre (N–S through, E–W between).
		using F = FMeshData;
		const double Z0 = kBarBottom, Z1 = kLay, P = kPerimeterBar;
		Out.Bars.Box(FVector(kLW, kLN, Z0), FVector(kLE, kLN + P, Z1), F::NegZ | F::PosY);
		Out.Bars.Box(FVector(kLW, kLS - P, Z0), FVector(kLE, kLS, Z1), F::NegZ | F::NegY);
		Out.Bars.Box(FVector(kLW, kLN + P, Z0), FVector(kLW + P, kLS - P, Z1), F::NegZ | F::PosX);
		Out.Bars.Box(FVector(kLE - P, kLN + P, Z0), FVector(kLE, kLS - P, Z1), F::NegZ | F::NegX);
		TArray<double> Xs, Ys;
		for (double X = FMath::CeilToDouble((kLW + P) / kGrid) * kGrid; X < kLE - P - 1e-9; X += kGrid) { Xs.Add(X); }
		for (double Y = FMath::CeilToDouble((kLN + P) / kGrid) * kGrid; Y < kLS - P - 1e-9; Y += kGrid) { Ys.Add(Y); }
		for (double X : Xs)
		{
			Out.Bars.Box(FVector(X - kBarHalf, kLN + P, Z0), FVector(X + kBarHalf, kLS - P, Z1), F::NegZ | F::NegX | F::PosX);
		}
		TArray<double> Stops = {kLW + P};
		for (double X : Xs)
		{
			Stops.Add(X - kBarHalf);
			Stops.Add(X + kBarHalf);
		}
		Stops.Add(kLE - P);
		for (double Y : Ys)
		{
			for (int32 i = 0; i + 1 < Stops.Num(); i += 2)
			{
				Out.Bars.Box(FVector(Stops[i], Y - kBarHalf, Z0), FVector(Stops[i + 1], Y + kBarHalf, Z1), F::NegZ | F::NegY | F::PosY);
			}
		}
	}

	/** The share of the panel the bars leave open. */
	double LaylightOpenFraction()
	{
		const double W = kLE - kLW, D = kLS - kLN, P = kPerimeterBar;
		int32 NX = 0, NY = 0;
		for (double X = FMath::CeilToDouble((kLW + P) / kGrid) * kGrid; X < kLE - P - 1e-9; X += kGrid) { ++NX; }
		for (double Y = FMath::CeilToDouble((kLN + P) / kGrid) * kGrid; Y < kLS - P - 1e-9; Y += kGrid) { ++NY; }
		const double InW = W - 2 * P, InD = D - 2 * P;
		const double Covered = W * D - InW * InD + NX * 2 * kBarHalf * InD + NY * 2 * kBarHalf * (InW - NX * 2 * kBarHalf);
		return 1.0 - Covered / (W * D);
	}

	/** Height of the glass roof over (x, y) inside the eaves. */
	double RoofZ(double X, double Y)
	{
		return kTop + kSlope * FMath::Min(kHalfX - FMath::Abs(X), kHalfY - FMath::Abs(Y - kMidY));
	}

	/** The glass roof: four panes (hipped), the gutter on the kerb, rafters, hips and ridge. */
	void BuildGlassRoof(FParts& Out)
	{
		const double Z = kTop;
		const FVector NE(kEaveX, kEaveN, Z), NW(-kEaveX, kEaveN, Z), SE(kEaveX, kEaveS, Z), SW(-kEaveX, kEaveS, Z);
		const FVector RN(0.0, kRidgeN, kRidge), RS(0.0, kRidgeS, kRidge);
		const FVector East = FVector(kSlope, 0.0, 1.0).GetSafeNormal(), West = FVector(-kSlope, 0.0, 1.0).GetSafeNormal();
		const FVector North = FVector(0.0, -kSlope, 1.0).GetSafeNormal(), South = FVector(0.0, kSlope, 1.0).GetSafeNormal();
		Out.Glass.Poly({NE, SE, RS, RN}, East);
		Out.Glass.Poly({SW, NW, RN, RS}, West);
		Out.Glass.Poly({NW, NE, RN}, North);
		Out.Glass.Poly({SE, SW, RS}, South);

		SalonKit::SweepPlan(Out.Steel, {FVector2D(kEaveX, kEaveN), FVector2D(-kEaveX, kEaveN), FVector2D(-kEaveX, kEaveS), FVector2D(kEaveX, kEaveS)}, true, Gutter());
		Out.Steel.Box(FVector(-kRidgeCapHalf, kRidgeN - 0.05, kRidge - 0.02), FVector(kRidgeCapHalf, kRidgeS + 0.05, kRidge + 0.05), FMeshData::AllFaces);
		Bar(Out.Steel, NE, RN, East + North, 0.06, 0.07, 0.02);
		Bar(Out.Steel, NW, RN, West + North, 0.06, 0.07, 0.02);
		Bar(Out.Steel, SE, RS, East + South, 0.06, 0.07, 0.02);
		Bar(Out.Steel, SW, RS, West + South, 0.06, 0.07, 0.02);
		// Rafters every 1.1 m from the eaves up to the hips or the ridge, 12 mm into the glass.
		const int32 Count = FMath::FloorToInt32(kHalfX / kRafterPitch);
		for (int32 K = -Count; K <= Count; ++K)
		{
			const double Off = K * kRafterPitch;
			// East and west panes, across y.
			const double Y = kMidY + Off;
			// (Those that reach the ridge stop inside its cap, clear of each other.)
			const double XTop = FMath::Max(kRidgeCapHalf - 0.02, FMath::Abs(Off) - (kHalfY - kHalfX));
			if (kEaveX - XTop > 0.1)
			{
				Bar(Out.Steel, FVector(kEaveX, Y, Z), FVector(XTop, Y, RoofZ(XTop, Y)), East, 0.05, 0.06, 0.012);
				Bar(Out.Steel, FVector(-kEaveX, Y, Z), FVector(-XTop, Y, RoofZ(-XTop, Y)), West, 0.05, 0.06, 0.012);
			}
			// North and south panes, across x.
			const double X = Off;
			const double YTopN = kRidgeN - FMath::Abs(X), YTopS = kRidgeS + FMath::Abs(X);
			if (YTopN - kEaveN > 0.1)
			{
				Bar(Out.Steel, FVector(X, kEaveN, Z), FVector(X, YTopN, RoofZ(X, YTopN)), North, 0.05, 0.06, 0.012);
				Bar(Out.Steel, FVector(X, kEaveS, Z), FVector(X, YTopS, RoofZ(X, YTopS)), South, 0.05, 0.06, 0.012);
			}
		}
	}

	// ---------------------------------------------------------------- The floor

	/** Vertex K of the sun clock's 128-gon (r 10.3 m) counted from north (K > 0 east): the Rotunda's floor, and ASunClock's ring. */
	FVector2D SunClockVertex(int32 K)
	{
		const double A = 2.0 * kPi * K / SH::SunClockSides;
		return FVector2D(SH::SunClockRadius * FMath::Sin(A), -SH::SunClockRadius * FMath::Cos(A));
	}

	/**
	 * The floor. The court and the door's reveal are strips on one set of x stations, so no two pieces meet off a
	 * vertex. The passage floor spans the Rotunda's shell round its passage (x ±2.3) from the wall's outer face to
	 * the sun clock, and meets the sun clock only at its own vertices: out to the first beyond the shell, inside the
	 * drum's wall, and back to the shell's line at the drum's outer face.
	 */
	void BuildFloor(FParts& Out)
	{
		const double PassageHalf = kDoorHalf + 0.3;
		const double DrumOut = MuseePlan::Rotunda::Radius + MuseePlan::Rotunda::Wall;
		const double DrumY = -FMath::Sqrt(DrumOut * DrumOut - PassageHalf * PassageHalf);
		int32 KOut = 0;   // the first vertex beyond the shell
		while (SunClockVertex(KOut).X < PassageHalf) { ++KOut; }

		TArray<double> Stations = {-PassageHalf, -kDoorHalf, kDoorHalf, PassageHalf};
		for (int32 K = -KOut + 1; K < KOut; ++K) { Stations.Add(SunClockVertex(K).X); }
		Stations = SortedUnique(Stations);
		TArray<double> Xs = Stations;
		Xs.Add(kW);
		Xs.Add(kE);
		Xs = SortedUnique(Xs);
		auto Cell = [&Out](double X0, double X1, double Y0, double Y1)
		{
			Out.Floor.Rect(FVector(X0, Y0, 0.0), FVector(X1, Y0, 0.0), FVector(X1, Y1, 0.0), FVector(X0, Y1, 0.0), kUp);
		};
		for (int32 i = 0; i + 1 < Xs.Num(); ++i)
		{
			Cell(Xs[i], Xs[i + 1], kN, kS);
			if (Xs[i] >= -kDoorHalf - 1e-9 && Xs[i + 1] <= kDoorHalf + 1e-9) { Cell(Xs[i], Xs[i + 1], kS, kOS); }
		}

		// The passage: zipped between the stations along the wall's outer face and the sun clock's vertices.
		auto V = [&Out](const FVector2D& P) { return Out.Floor.Vertex(FVector(P.X, P.Y, 0.0), kUp, P); };
		TArray<int32> South, North;
		for (double X : Stations) { South.Add(V(FVector2D(X, kOS))); }
		North.Add(V(FVector2D(-PassageHalf, DrumY)));
		for (int32 K = -KOut + 1; K < KOut; ++K) { North.Add(V(SunClockVertex(K))); }
		North.Add(V(FVector2D(PassageHalf, DrumY)));
		auto XOf = [&Out](int32 I) { return Out.Floor.Positions[I].X; };
		int32 S = 0, N = 0;
		while (S + 1 < South.Num() || N + 1 < North.Num())
		{
			const bool bSouth = N + 1 >= North.Num() || (S + 1 < South.Num() && XOf(South[S + 1]) <= XOf(North[N + 1]));
			if (bSouth)
			{
				Out.Floor.Tri(South[S], South[S + 1], North[N]);
				++S;
			}
			else
			{
				Out.Floor.Tri(South[S], North[N + 1], North[N]);
				++N;
			}
		}
		// The two ears inside the drum's wall, out to the vertex beyond the shell.
		Out.Floor.Tri(North[0], V(SunClockVertex(-KOut)), North[1]);
		Out.Floor.Tri(North.Last(), V(SunClockVertex(KOut)), North[North.Num() - 2]);
	}

	// ---------------------------------------------------------------- Plinths and the bench

	struct FPlinth
	{
		double X, Y, Width, Height, Depth;   // centre (m), width E–W, height, depth N–S
	};

	/** SculptureHallPlan.works' plinths (the Gates of Hell have none). */
	const TArray<FPlinth>& Plinths()
	{
		static const TArray<FPlinth> List = {
			{-6.40, -16.30, 0.6, 1.3, 0.6},   // 1 Rude, La Marseillaise
			{-7.70, -21.10, 1.4, 0.8, 3.0},   // 2 Carpeaux, La Danse
			{-6.40, -25.50, 0.9, 0.9, 0.9},   // 3 Rodin, The Age of Bronze
			{-3.60, -26.90, 1.2, 1.2, 1.2},   // 5 The Thinker
			{3.60, -26.90, 1.3, 0.5, 1.1},    // 6 The Kiss
			{5.20, -22.70, 2.8, 0.12, 2.6},   // 7 The Burghers of Calais
			{6.40, -19.10, 1.2, 0.3, 1.2},    // 8 Balzac
			{6.40, -15.90, 1.0, 0.3, 1.0},    // 9 The Walking Man
			{3.90, -17.30, 1.6, 0.5, 1.2},    // 10 Bourdelle, Heracles the Archer
		};
		return List;
	}

	constexpr double kClearOfWall = 0.04;   // a plinth the plan puts against a wall stands this far off it (the skirting is 3 cm)

	/** A rectangle's outline, clockwise seen from above (north edge westward), so A points out of it. */
	TArray<FVector2D> Outline(double X0, double X1, double Y0, double Y1)
	{
		return {FVector2D(X1, Y0), FVector2D(X0, Y0), FVector2D(X0, Y1), FVector2D(X1, Y1)};
	}

	/** A solid swept round an outline, with a flat top at the profile's last point (and a bottom at its first, if asked). */
	void OutlineSolid(FMeshData& M, const TArray<FVector2D>& Loop, const FProfile& Profile, bool bBottom)
	{
		const TArray<TArray<FFrame>> Runs = SalonKit::PlanRuns(Loop, true);
		TArray<FVector> Top, Bottom;
		for (const TArray<FFrame>& Run : Runs)
		{
			SalonKit::Sweep(M, Run, Profile);
			Top.Add(Run[0].At(Profile.Points.Last().P));
			Bottom.Add(Run[0].At(Profile.Points[0].P));
		}
		M.Poly(Top, kUp);
		if (bBottom) { M.Poly(Bottom, -kUp); }
	}

	/** The room stands empty for now (its ten scans were not good enough; relight.py retires them): no plinths. */
	constexpr bool kWorksOnShow = false;

	void BuildFurniture(FParts& Out)
	{
		for (const FPlinth& P : Plinths())
		{
			if (!kWorksOnShow) { break; }
			const double X0 = FMath::Max(P.X - P.Width / 2, kW + kClearOfWall), X1 = FMath::Min(P.X + P.Width / 2, kE - kClearOfWall);
			const double Y0 = FMath::Max(P.Y - P.Depth / 2, kN + kClearOfWall), Y1 = FMath::Min(P.Y + P.Depth / 2, kS - kClearOfWall);
			// A recessed toe (a shadow line at the floor) and an eased top arris; the top at the plan's height.
			const double Toe = P.Height >= 0.3 ? 0.05 : 0.03, Recess = 0.02, Ease = 0.006;
			FProfile Section;
			Section.Add(-Recess, -kInto).Add(-Recess, Toe).Add(0.0, Toe).Add(0.0, P.Height - Ease).Add(-Ease, P.Height);
			OutlineSolid(Out.Plinths, Outline(X0, X1, Y0, Y1), Section, false);
		}

		// The bench opposite The Dance is AMuseeFurniture's now (Furniture/MuseeFurniture.cpp: honed travertine, eased).
	}

	FParts BuildParts()
	{
		FParts Parts;
		BuildWalls(Parts);
		BuildMouldings(Parts);
		BuildCeilingAndRoof(Parts);
		BuildLaylight(Parts);
		BuildGlassRoof(Parts);
		BuildFloor(Parts);
		BuildFurniture(Parts);
		return Parts;
	}
}

// ==================================================================== ASculptureHallStructure

namespace SHB = SculptureHallBuild;

ASculptureHallStructure::ASculptureHallStructure()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	auto Make = [this](const TCHAR* ComponentName, bool bCollide, bool bShadow)
	{
		UProceduralMeshComponent* M = CreateDefaultSubobject<UProceduralMeshComponent>(ComponentName);
		M->SetupAttachment(RootComponent);
		M->bUseAsyncCooking = true;
		M->bUseComplexAsSimpleCollision = true;
		if (bCollide)
		{
			M->SetCollisionProfileName(TEXT("BlockAll"));
			M->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		else
		{
			M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		M->SetCastShadow(bShadow);
		return M;
	};
	Walls = Make(TEXT("Walls"), true, true);
	Trim = Make(TEXT("Trim"), true, true);
	Mouldings = Make(TEXT("Mouldings"), false, true);
	Ceiling = Make(TEXT("Ceiling"), false, true);
	Roof = Make(TEXT("Roof"), false, true);
	RoofGlass = Make(TEXT("RoofGlass"), false, false);
	Laylight = Make(TEXT("Laylight"), false, true);
	LaylightBars = Make(TEXT("LaylightBars"), false, true);
	Floor = Make(TEXT("Floor"), true, true);
	Furniture = Make(TEXT("Furniture"), true, true);

	auto Soft = [](const TCHAR* Name) { return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(FString::Printf(TEXT("/Game/Museum/Materials/%s.%s"), Name, Name))); };
	WallMaterial = Soft(TEXT("M_Travertine_Ashlar"));   // vein-cut travertine ashlar (materials.py)
	ExteriorMaterial = Soft(TEXT("M_Travertine_Honed"));
	TrimMaterial = Soft(TEXT("M_Travertine_Honed"));
	CorniceMaterial = Soft(TEXT("M_Plaster_Moulding"));
	CeilingMaterial = Soft(TEXT("M_Plaster_Coffer"));
	RoofDeckMaterial = Soft(TEXT("M_RibSteel"));
	RoofSteelMaterial = Soft(TEXT("M_RibSteel"));
	GlassMaterial = Soft(TEXT("M_Glass"));
	LaylightMaterial = Soft(TEXT("MI_light_grid_FFF3DA"));
	LaylightBarMaterial = Soft(TEXT("M_RibPearl"));
	FloorMaterial = Soft(TEXT("M_Travertine_Polished"));
	PlinthMaterial = Soft(TEXT("M_Travertine_Honed"));
	BenchMaterial = Soft(TEXT("M_Travertine_Honed"));

	AddTags();
}

void ASculptureHallStructure::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
	ApplyMaterials();
	AddTags();
}

void ASculptureHallStructure::BeginPlay()
{
	Super::BeginPlay();
	// A placed actor doesn't rerun its construction when the map loads: rebuild, so the map never shows an older build.
	Build();
	ApplyMaterials();
}

void ASculptureHallStructure::AddTags()
{
	Tags.AddUnique(FName(TEXT("musee.building")));
	Tags.AddUnique(FName(TEXT("musee.wing:SculptureHall")));
}

void ASculptureHallStructure::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }   // baked into static meshes (Geometry/MuseeBake.h)
	const SHB::FParts Parts = SHB::BuildParts();
	for (UProceduralMeshComponent* Component : {Walls.Get(), Trim.Get(), Mouldings.Get(), Ceiling.Get(), Roof.Get(), RoofGlass.Get(),
												 Laylight.Get(), LaylightBars.Get(), Floor.Get(), Furniture.Get()})
	{
		if (Component) { Component->ClearAllMeshSections(); }
	}
	Parts.WallsInner.Write(Walls, 0, true);
	Parts.Exterior.Write(Walls, 1, true);
	Parts.Trim.Write(Trim, 0, true);
	Parts.Cornice.Write(Mouldings, 0, false);
	Parts.Frame.Write(Mouldings, 1, false);
	Parts.Ceiling.Write(Ceiling, 0, false);
	Parts.Deck.Write(Roof, 0, false);
	Parts.Steel.Write(Roof, 1, false);
	Parts.Glass.Write(RoofGlass, 0, false);
	Parts.Diffuser.Write(Laylight, 0, false);
	Parts.Bars.Write(LaylightBars, 0, false);
	Parts.Floor.Write(Floor, 0, true);
	Parts.Plinths.Write(Furniture, 0, true);
	Parts.Bench.Write(Furniture, 1, true);
}

void ASculptureHallStructure::ApplyMaterials()
{
	auto Apply = [](UProceduralMeshComponent* Target, int32 Section, const TSoftObjectPtr<UMaterialInterface>& Ref)
	{
		if (!Target || Ref.IsNull()) { return; }
		if (UMaterialInterface* Mat = Ref.LoadSynchronous()) { Target->SetMaterial(Section, Mat); }
	};
	Apply(Walls, 0, WallMaterial);
	Apply(Walls, 1, ExteriorMaterial);
	Apply(Trim, 0, TrimMaterial);
	Apply(Mouldings, 0, CorniceMaterial);
	Apply(Mouldings, 1, CorniceMaterial);
	Apply(Ceiling, 0, CeilingMaterial);
	Apply(Roof, 0, RoofDeckMaterial);
	Apply(Roof, 1, RoofSteelMaterial);
	Apply(RoofGlass, 0, GlassMaterial);
	Apply(Laylight, 0, LaylightMaterial);
	Apply(LaylightBars, 0, LaylightBarMaterial);
	Apply(Floor, 0, FloorMaterial);
	Apply(Furniture, 0, PlinthMaterial);
	Apply(Furniture, 1, BenchMaterial);
}

TArray<FString> ASculptureHallStructure::GetReplacedImportPrims()
{
	return {
		TEXT("/Museum/SculptureHall/Sculpture_court_walls"),
		TEXT("/Museum/SculptureHall/Sculpture_court_mouldings"),
		TEXT("/Museum/SculptureHall/Sculpture_court_floor"),
		TEXT("/Museum/SculptureHall/Sculpture_ceiling"),
		TEXT("/Museum/SculptureHall/Sculpture_laylight"),
		TEXT("/Museum/SculptureHall/Sculpture_bench"),
		TEXT("/Museum/SculptureHall/Sculpture_plinths"),
	};
}

FBox ASculptureHallStructure::GetLaylightPanel() const
{
	const FBox Local(FVector(SHB::kLW, SHB::kLN, SHB::kLay) * MuseePlan::Cm, FVector(SHB::kLE, SHB::kLS, SHB::kLay) * MuseePlan::Cm);
	return Local.TransformBy(GetActorTransform());
}

FBox ASculptureHallStructure::GetLaylightLightBox() const
{
	const double P = SHB::kPerimeterBar, Z = SHB::kBarBottom - SHB::kLightDrop;
	const FBox Local(FVector(SHB::kLW + P, SHB::kLN + P, Z) * MuseePlan::Cm, FVector(SHB::kLE - P, SHB::kLS - P, Z) * MuseePlan::Cm);
	return Local.TransformBy(GetActorTransform());
}

float ASculptureHallStructure::GetLaylightOpenFraction()
{
	return float(SHB::LaylightOpenFraction());
}

// ==================================================================== Development aids (not in shipping builds)

#if !UE_BUILD_SHIPPING
/**
 * -MuseeSculptureDump=<file>: writes every part's triangles (cm, normals, UVs) at startup, for the geometry checks
 * (add -MuseeSculptureDumpQuit to exit straight after).
 * -MuseeSculptureHall: in a game world with no ASculptureHallStructure placed, spawns one at the origin and hides the
 * imported pieces it replaces (and relight.py's temporary roof over the court), with the laylight's rect light moved
 * under the bars: a preview before native.py places it.
 */
namespace SculptureHallDev
{
	void Dump(const FString& Path)
	{
		const SHB::FParts Parts = SHB::BuildParts();
		TArray<FString> Lines;
		for (const SHB::FNamedPart& Named : Parts.Named())
		{
			const SHB::FMeshData& M = *Named.Part;
			Lines.Add(FString::Printf(TEXT("part %s %d %d"), Named.Name, M.Positions.Num(), M.Indices.Num() / 3));
			for (int32 i = 0; i < M.Positions.Num(); ++i)
			{
				const FVector& P = M.Positions[i];
				const FVector& N = M.Normals[i];
				const FVector2D& T = M.UVs[i];
				Lines.Add(FString::Printf(TEXT("v %.6f %.6f %.6f %.6f %.6f %.6f %.6f %.6f"), P.X, P.Y, P.Z, N.X, N.Y, N.Z, T.X, T.Y));
			}
			for (int32 t = 0; t + 2 < M.Indices.Num(); t += 3)
			{
				Lines.Add(FString::Printf(TEXT("t %d %d %d"), M.Indices[t], M.Indices[t + 1], M.Indices[t + 2]));
			}
		}
		FFileHelper::SaveStringArrayToFile(Lines, *Path);
	}

	void Preview(const FActorsInitializedParams& Params)
	{
		UWorld* World = Params.World;
		if (!World || !World->IsGameWorld()) { return; }
		for (TActorIterator<ASculptureHallStructure> It(World); It; ++It) { return; }
		const TArray<FString> Replaced = ASculptureHallStructure::GetReplacedImportPrims();
		const FBox LightBox = FBox(FVector(SHB::kLW, SHB::kLN, SHB::kBarBottom - SHB::kLightDrop) * MuseePlan::Cm,
								   FVector(SHB::kLE, SHB::kLS, SHB::kBarBottom - SHB::kLightDrop) * MuseePlan::Cm);
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			bool bReplaced = false, bRoof = false, bLaylight = false, bHere = false;
			for (const FName& Tag : Actor->Tags)
			{
				const FString Text = Tag.ToString();
				if (Text.StartsWith(TEXT("prim:")))
				{
					const FString Prim = Text.Mid(5);
					for (const FString& R : Replaced) { bReplaced |= Prim == R || Prim.StartsWith(R + TEXT("/")); }
				}
				bRoof |= Text == TEXT("musee.roof");
				bLaylight |= Text == TEXT("musee.laylight");
				bHere |= Text == TEXT("musee.wing:SculptureHall");
			}
			if (bReplaced || (bRoof && bHere))
			{
				Actor->SetActorHiddenInGame(true);
				Actor->SetActorEnableCollision(false);
			}
			else if (bLaylight && bHere)
			{
				const FVector At = Actor->GetActorLocation();
				Actor->SetActorLocation(FVector(At.X, At.Y, LightBox.Min.Z));
			}
		}
		FActorSpawnParameters Spawn;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		World->SpawnActor<ASculptureHallStructure>(ASculptureHallStructure::StaticClass(), FTransform::Identity, Spawn);
	}

	FDelayedAutoRegisterHelper GRegister(EDelayedRegisterRunPhase::EndOfEngineInit, []
	{
		FString Path;
		if (FParse::Value(FCommandLine::Get(), TEXT("-MuseeSculptureDump="), Path))
		{
			Dump(Path);
			if (FParse::Param(FCommandLine::Get(), TEXT("MuseeSculptureDumpQuit"))) { FPlatformMisc::RequestExit(false); }
		}
		if (FParse::Param(FCommandLine::Get(), TEXT("MuseeSculptureHall")))
		{
			FWorldDelegates::OnWorldInitializedActors.AddStatic(&Preview);
		}
	});
}
#endif
