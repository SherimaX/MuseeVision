#include "Cube/CubeStructure.h"

#include "HAL/IConsoleManager.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Cube/CubeMesh.h"
#include "Cube/CubePlan.h"
#include "Components/BoxComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/RectLightComponent.h"
#include "Engine/CollisionProfile.h"
#include "Geometry/MuseeBake.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

/**
 * The Cube's geometry, in metres in the Atrium's frame (Cube/CubePlan.h). A named namespace: the module builds in
 * unity files, where other files' helpers share the unit.
 */
namespace CubeBuild
{
	using namespace CubePlan;
	using CubeMesh::FMesh;
	using CubeMesh::Polar;
	using CubeMesh::Outward;
	using CubeMesh::Along;

	constexpr double Turn = 2.0 * UE_DOUBLE_PI;
	constexpr int32 Round = 192;
	constexpr double LedR = 0.5 * (GlassOut + EarthR);   // the cavity's middle

	/** The four tier boundaries of the glass, top to bottom (five heights). */
	double TierZ(int32 K) { return GlassTop + (GlassBottom - GlassTop) * K / GlassTiers; }

	// ---------------------------------------------------------------------------------------------- the panels

	/**
	 * A square face (half-size Half) in the plane of axes (U, V) at Offset along its normal N, facing N; UV0 in metres
	 * from the face's corner, UV1.x the face's index (the panel material's randomness per face). HoleR > 0 cuts a round
	 * hole at the centre (the ceiling's iris): a ring of quads from the circle out to a square of half-size HoleSquare,
	 * then four strips.
	 */
	void Face(FMesh& M, const FVector& Origin, const FVector& U, const FVector& V, const FVector& N, int32 Index, double FaceHalf, double HoleR,
			  double HoleSquare, TFunctionRef<FVector2D(double, double)> UvOf)
	{
		const FLinearColor White(1, 1, 1, 1);
		const FVector2D Id(Index, 0);
		auto P = [&](double A, double B) { return Origin + U * A + V * B; };
		auto Vx = [&](double A, double B) { return M.V(P(A, B), N, UvOf(A, B), White, Id, U); };
		auto Rect = [&](double A0, double B0, double A1, double B1)
		{
			M.Quad(Vx(A0, B0), Vx(A1, B0), Vx(A1, B1), Vx(A0, B1));
		};
		const double H = FaceHalf;
		if (HoleR <= 0)
		{
			Rect(-H, -H, H, H);
			return;
		}
		const double S = HoleSquare;
		Rect(-H, -H, H, -S);
		Rect(-H, S, H, H);
		Rect(-H, -S, -S, S);
		Rect(S, -S, H, S);
		TArray<double> Angles;
		for (int32 i = 0; i < 96; ++i) { Angles.Add(Turn * i / 96); }
		for (int32 k = 0; k < 4; ++k) { Angles.AddUnique(Turn * (0.125 + 0.25 * k)); }
		Angles.Sort();
		for (int32 i = 0; i < Angles.Num(); ++i)
		{
			const double A0 = Angles[i], A1 = i + 1 < Angles.Num() ? Angles[i + 1] : Angles[0] + Turn;
			auto Sq = [S](double A)
			{
				const double C = FMath::Cos(A), Sn = FMath::Sin(A);
				const double T = S / FMath::Max(FMath::Abs(C), FMath::Abs(Sn));
				return FVector2D(C * T, Sn * T);
			};
			const FVector2D Q0 = Sq(A0), Q1 = Sq(A1);
			const FVector2D C0(FMath::Cos(A0) * HoleR, FMath::Sin(A0) * HoleR), C1(FMath::Cos(A1) * HoleR, FMath::Sin(A1) * HoleR);
			M.Quad(Vx(C0.X, C0.Y), Vx(C1.X, C1.Y), Vx(Q1.X, Q1.Y), Vx(Q0.X, Q0.Y));
		}
	}

	FMesh PanelFaces()
	{
		FMesh M;
		const double Mid = 0.5 * (Ceiling + Floor);   // −22
		const FVector X(1, 0, 0), Y(0, 1, 0), Z(0, 0, 1);
		auto Plan = [](double A, double B) { return FVector2D(A + Half, B + Half); };
		// Floor (facing up) and ceiling (facing down, round the iris's soffit ring).
		M.Reserve(2000);
		Face(M, FVector(0, 0, Floor), X, Y, Z, 0, Half, 0, 0, Plan);
		Face(M, FVector(0, 0, Ceiling), X, Y, -Z, 1, Half, SoffitOut + 0.01, 5.6, Plan);
		// Walls: UV (along, down from the ceiling).
		auto WallUv = [](double A, double B) { return FVector2D(A + Half, -B + Half); };
		Face(M, FVector(Half, 0, Mid), Y, Z, -X, 2, Half, 0, 0, WallUv);    // east
		Face(M, FVector(-Half, 0, Mid), Y, Z, X, 3, Half, 0, 0, WallUv);    // west
		Face(M, FVector(0, Half, Mid), X, Z, -Y, 4, Half, 0, 0, WallUv);    // south
		Face(M, FVector(0, -Half, Mid), X, Z, Y, 5, Half, 0, 0, WallUv);    // north
		return M;
	}

	// ---------------------------------------------------------------------------------------------- the concrete box

	FMesh ConcreteBox()
	{
		FMesh M;
		const FLinearColor White(1, 1, 1, 1);
		auto Box = [&M](const FVector& Lo, const FVector& Hi) { M.Box(Lo, Hi); };
		// The walls, full height, each overlapping the base and the roof.
		Box(FVector(BoxIn, -BoxOut, BaseBottom), FVector(BoxOut, BoxOut, RoofTop));
		Box(FVector(-BoxOut, -BoxOut, BaseBottom), FVector(-BoxIn, BoxOut, RoofTop));
		Box(FVector(-BoxIn, BoxIn, BaseBottom), FVector(BoxIn, BoxOut, RoofTop));
		Box(FVector(-BoxIn, -BoxOut, BaseBottom), FVector(BoxIn, -BoxIn, RoofTop));
		Box(FVector(-BoxIn, -BoxIn, BaseBottom), FVector(BoxIn, BoxIn, BaseTop));
		// The roof, pierced for the sleeve: its soffit and top round the hole, and the hole's wall.
		const FVector X(1, 0, 0), Y(0, 1, 0), Z(0, 0, 1);
		auto Plan = [](double A, double B) { return FVector2D(A, B); };
		auto RoofFace = [&](double Zf, const FVector& N) { Face(M, FVector(0, 0, Zf), X, Y, N, 0, BoxIn, SleeveOut, 5.6, Plan); };
		RoofFace(RoofSoffit, -Z);
		RoofFace(RoofTop, Z);
		for (int32 i = 0; i < Round; ++i)
		{
			const double A0 = Turn * i / Round, A1 = Turn * (i + 1) / Round;
			M.Quad(M.V(Polar(SleeveOut, A0, RoofSoffit), -Outward(A0), FVector2D(A0 * SleeveOut, 0), White),
				   M.V(Polar(SleeveOut, A1, RoofSoffit), -Outward(A1), FVector2D(A1 * SleeveOut, 0), White),
				   M.V(Polar(SleeveOut, A1, RoofTop), -Outward(A1), FVector2D(A1 * SleeveOut, 1), White),
				   M.V(Polar(SleeveOut, A0, RoofTop), -Outward(A0), FVector2D(A0 * SleeveOut, 1), White));
		}
		return M;
	}

	// ---------------------------------------------------------------------------------------------- the metal

	struct FMetal
	{
		FMesh Gilt, Dark, Rail;
	};

	FMetal Metalwork()
	{
		FMetal Out;
		using CubeMesh::Cylinder;
		using CubeMesh::Annulus;
		// The lining through the Atrium's slab: its face (r 2.30) from under the floor's lip down to the slab's soffit,
		// and the soffit's ring out to the glass.
		Cylinder(Out.Dark, LiningIn, SlabBottom, -0.03, true, 0, Turn);
		Annulus(Out.Dark, LiningIn, GlassIn + 0.004, SlabBottom, false);
		// The cavity closed top and bottom (seen through the glass): bronze plates from the glass to the ground.
		Annulus(Out.Dark, GlassOut - 0.004, SleeveOut + 0.02, SlabBottom, false);
		Annulus(Out.Dark, GlassOut - 0.004, SleeveOut + 0.02, GlassBottom, true);
		// The ring frames between the tiers (and at both ends): a slim gilt band inside the glass.
		for (int32 K = 0; K <= GlassTiers; ++K)
		{
			const double Zk = TierZ(K);
			const double Z0 = K == 0 ? Zk - FrameHalfHeight * 2 : (K == GlassTiers ? Zk : Zk - FrameHalfHeight);
			const double Z1 = K == 0 ? Zk : (K == GlassTiers ? Zk + FrameHalfHeight * 2 : Zk + FrameHalfHeight);
			CubeMesh::Band(Out.Gilt, FrameIn, FrameOut + 0.002, Z0, Z1);
		}
		// The joints between the panes: slim gilt cover strips inside the glass, on the diagonals.
		for (int32 K = 0; K < GlassTiers; ++K)
		{
			for (int32 j = 0; j < GlassPanesRound; ++j)
			{
				const double A = Turn * (0.125 + double(j) / GlassPanesRound);
				CubeMesh::Block(Out.Gilt, Polar(GlassIn - 0.006, A, 0.5 * (TierZ(K) + TierZ(K + 1))), Outward(A), Along(A), FVector(0, 0, 1),
								FVector(0.006, 0.0125, 0.5 * FMath::Abs(TierZ(K + 1) - TierZ(K))));
			}
		}
		// The sleeve through the box's roof and the void, dark bronze, down to the iris's housing; thin gilt lines at
		// its joints.
		Cylinder(Out.Dark, SleeveIn, HousingTop, GlassBottom, true, 0, Turn);
		CubeMesh::Band(Out.Gilt, SleeveIn - 0.008, SleeveIn + 0.001, RoofSoffit - 0.01, RoofSoffit + 0.01);
		// The iris's housing: its roof (r 2.45 … 4.30 at −7.60) and its outer wall, closing the space the blades move in.
		Annulus(Out.Dark, SleeveIn, HousingOut, HousingTop, false);
		Cylinder(Out.Dark, HousingOut, Ceiling + SoffitThick, HousingTop, true, 0, Turn);
		Cylinder(Out.Dark, SleeveIn, IrisTop + 0.004, HousingTop, true, 0, Turn);
		// The gilt soffit ring in the ceiling: its underside, its inner lip up to the blades, its top, its outer edge.
		Annulus(Out.Gilt, SoffitIn, SoffitOut, Ceiling, false);
		Annulus(Out.Gilt, SoffitIn, HousingOut, Ceiling + SoffitThick, true);
		Cylinder(Out.Gilt, SoffitIn, Ceiling, Ceiling + SoffitThick, true, 0, Turn);
		Cylinder(Out.Gilt, SoffitOut, Ceiling - 0.004, Ceiling + SoffitThick, false, 0, Turn);
		// A fine bead at the ring's inner edge, and a second round its outer edge.
		CubeMesh::Torus(Out.Gilt, SoffitIn + 0.012, 0.009, Ceiling - 0.002);
		CubeMesh::Torus(Out.Gilt, SoffitOut - 0.02, 0.007, Ceiling - 0.001);

		// The guide rails, north and south: a T (foot plate on the frames, blade towards the car), with a bracket at
		// every frame and a flared entry at the top.
		for (const double Bearing : {0.0, 180.0})
		{
			const double A = PlanAngle(Bearing);
			const FVector O = Outward(A), T = Along(A), Up(0, 0, 1);
			const double Mid = 0.5 * (RailTop + RailBottom), Len = 0.5 * (RailTop - RailBottom);
			CubeMesh::Block(Out.Rail, Polar(0.5 * (RailFootIn + RailFootOut - 0.02), A, Mid), O, T, Up,
							FVector(0.5 * (RailFootOut - 0.02 - RailFootIn), 0.5 * RailFootWidth, Len));
			CubeMesh::Block(Out.Rail, Polar(0.5 * (RailTip + RailFootIn), A, Mid), O, T, Up, FVector(0.5 * (RailFootIn - RailTip), 0.5 * RailBlade, Len));
			for (int32 K = 0; K <= GlassTiers; ++K)
			{
				const double Zb = FMath::Clamp(TierZ(K), RailBottom + 0.1, RailTop - 0.1);
				CubeMesh::Block(Out.Dark, Polar(0.5 * (RailFootOut - 0.02 + GlassIn), A, Zb), O, T, Up,
								FVector(0.5 * (GlassIn - RailFootOut + 0.02) + 0.004, 0.5 * RailFootWidth + 0.02, 0.05));
			}
			// On the sleeve: brackets every metre.
			for (double Zb = GlassBottom - 0.4; Zb > RailBottom + 0.1; Zb -= 0.9)
			{
				CubeMesh::Block(Out.Dark, Polar(0.5 * (RailFootOut - 0.02 + SleeveIn), A, Zb), O, T, Up,
								FVector(0.5 * (SleeveIn - RailFootOut + 0.02) + 0.004, 0.5 * RailFootWidth + 0.02, 0.05));
			}
		}
		return Out;
	}

	// ---------------------------------------------------------------------------------------------- the glass

	FMesh GlassPanes()
	{
		FMesh M;
		const FLinearColor White(1, 1, 1, 1);
		const double R = 0.5 * (GlassIn + GlassOut);
		for (int32 K = 0; K < GlassTiers; ++K)
		{
			const double Z1 = TierZ(K) - FrameHalfHeight * (K == 0 ? 2 : 1) + 0.005, Z0 = TierZ(K + 1) + FrameHalfHeight * (K + 1 == GlassTiers ? 2 : 1) - 0.005;
			const int32 N = Round;
			for (int32 i = 0; i < N; ++i)
			{
				const double A0 = Turn * i / N, A1 = Turn * (i + 1) / N;
				M.Quad(M.V(Polar(R, A0, Z0), -Outward(A0), FVector2D(A0 * R, Z0), White),
					   M.V(Polar(R, A1, Z0), -Outward(A1), FVector2D(A1 * R, Z0), White),
					   M.V(Polar(R, A1, Z1), -Outward(A1), FVector2D(A1 * R, Z1), White),
					   M.V(Polar(R, A0, Z1), -Outward(A0), FVector2D(A0 * R, Z1), White));
			}
		}
		return M;
	}

	// ---------------------------------------------------------------------------------------------- the ground

	/** Periodic 3D Perlin noise on the cylinder (seamless round it). */
	double Noise(double A, double Z, double Freq, double Seed)
	{
		const double R = EarthR * Freq;
		return FMath::PerlinNoise3D(FVector(FMath::Cos(A) * R + Seed * 17.1, FMath::Sin(A) * R - Seed * 9.3, Z * Freq + Seed * 3.7));
	}

	uint32 Hash(int32 X, int32 Y, uint32 Seed)
	{
		uint32 H = uint32(X) * 0x8da6b343u ^ uint32(Y) * 0xd8163841u ^ Seed * 0xcb1ab31fu;
		H ^= H >> 13; H *= 0x5bd1e995u; H ^= H >> 15;
		return H;
	}
	double Rand01(uint32 H) { return (H & 0xFFFFFF) / double(0x1000000); }

	/**
	 * Stones in a matrix: jittered cells of Cell (m) along the arc (U) and down (Z), a stone in a cell with probability
	 * Fill; each a dome (radii RMin…RMax, elongated), protruding up to Prot. Returns the height and how much of a stone
	 * the point is on (0…1).
	 */
	FVector2D Stones(double U, double Z, double Cell, double Fill, double RMin, double RMax, double Prot, uint32 Seed, double Shape = 0.5)
	{
		const int32 CU = FMath::FloorToInt32(U / Cell), CZ = FMath::FloorToInt32(Z / Cell);
		double Best = 0, On = 0;
		for (int32 dU = -1; dU <= 1; ++dU)
		{
			for (int32 dZ = -1; dZ <= 1; ++dZ)
			{
				const uint32 H = Hash(CU + dU, CZ + dZ, Seed);
				if (Rand01(H) > Fill) { continue; }
				const double Cu = (CU + dU + 0.15 + 0.7 * Rand01(H * 7u + 1u)) * Cell;
				const double Cz = (CZ + dZ + 0.15 + 0.7 * Rand01(H * 13u + 5u)) * Cell;
				const double Ru = FMath::Lerp(RMin, RMax, Rand01(H * 31u + 3u));
				const double Rz = Ru * FMath::Lerp(0.55, 0.95, Rand01(H * 37u + 11u));
				const double Rot = Rand01(H * 41u + 17u) * UE_DOUBLE_PI;
				const double Du = U - Cu, Dz = Z - Cz;
				const double Pu = Du * FMath::Cos(Rot) + Dz * FMath::Sin(Rot), Pz = -Du * FMath::Sin(Rot) + Dz * FMath::Cos(Rot);
				const double Q = FMath::Square(Pu / Ru) + FMath::Square(Pz / Rz);
				if (Q >= 1) { continue; }
				const double Hgt = Prot * FMath::Lerp(0.5, 1.0, Rand01(H * 43u + 19u)) * FMath::Pow(1 - Q, Shape);   // Shape 0.5: a dome with steep sides (stones in a matrix); 1.5: a knob swelling out of the face
				if (Hgt > Best) { Best = Hgt; On = FMath::Clamp((1 - Q) * 4.0, 0.0, 1.0); }
			}
		}
		return FVector2D(Best, On);
	}

	/** The ground at a point of the cut face: how far it stands proud of the face (m), and its layers. */
	struct FSample
	{
		double Proud = 0;
		FLinearColor Layers;   // R soil, G clay, B chalk, A flint (crushed stone: none of them)
		FVector2D Extra;       // UV1: x fossil tint, y topsoil (dark) … subsoil
	};

	FSample Ground(double A, double Z)
	{
		const double U = A * EarthR;
		FSample S;
		// The boundaries, wavy; a solution pipe where the clay runs down into the chalk (at bearing 205°).
		const double Hardcore = -0.72 + 0.025 * Noise(A, 0, 4, 1);
		const double TopSub = -1.02 + 0.06 * Noise(A, 0, 2.5, 2) + 0.02 * Noise(A, 0, 9, 3);
		const double SoilClay = SoilBottom + 0.07 * Noise(A, 0, 2.0, 4) + 0.03 * Noise(A, 0, 7, 5);
		double Pipe = A - PlanAngle(205.0);
		Pipe = FMath::Fmod(Pipe + 3 * UE_DOUBLE_PI, Turn) - UE_DOUBLE_PI;
		const double ClayChalk = ClayBottom + 0.12 * Noise(A, 0, 1.6, 6) + 0.05 * Noise(A, 0, 6, 7) - 0.75 * FMath::Exp(-FMath::Square(Pipe * EarthR / 0.32));
		auto Blend = [](double Zp, double Edge, double W) { return FMath::Clamp((Zp - Edge) / W + 0.5, 0.0, 1.0); };   // 1 above the edge
		const double wHard = Blend(Z, Hardcore, 0.03);
		const double wSoil = (1 - wHard) * Blend(Z, SoilClay, 0.05);
		const double wClay = (1 - wHard - wSoil) * Blend(Z, ClayChalk, 0.04);
		const double wChalk = FMath::Max(0.0, 1 - wHard - wSoil - wClay);
		double Proud = 0, Flint = 0;
		// Crushed stone under the slab: angular stones packed tight.
		if (wHard > 0)
		{
			const FVector2D St = Stones(U, Z, 0.045, 0.95, 0.012, 0.028, 0.014, 11);
			Proud += wHard * (St.X + 0.004 * Noise(A, Z, 40, 12));
		}
		// Topsoil and subsoil: crumbly, a few small stones.
		if (wSoil > 0)
		{
			const FVector2D St = Stones(U, Z, 0.07, 0.3, 0.008, 0.02, 0.009, 21);
			Proud += wSoil * (0.007 * Noise(A, Z, 18, 13) + 0.004 * Noise(A, Z, 55, 14) + St.X);
		}
		// Clay-with-flints: smooth, spade-cut (faint vertical tool marks), with angular flints in it.
		if (wClay > 0)
		{
			const FVector2D Fl = Stones(U, Z, 0.11, 0.28, 0.015, 0.045, 0.016, 31);
			const double Marks = 0.0018 * FMath::Sin(U * Turn / 0.11 + 2.0 * Noise(A, Z, 3, 15));
			Proud += wClay * (0.004 * Noise(A, Z, 7, 16) + Marks + Fl.X);
			Flint = FMath::Max(Flint, wClay * Fl.Y);
		}
		// Chalk: blocky, with bedding partings and two bands of flint nodules.
		if (wChalk > 0)
		{
			const double Blocky = 0.01 * FMath::Abs(Noise(A, Z, 5, 17)) + 0.005 * Noise(A, Z, 16, 18);
			const double Bed = -0.005 * FMath::Exp(-FMath::Square(FMath::Fmod(FMath::Abs(Z - 0.03 * Noise(A, 0, 3, 19)), 0.37) / 0.012 - 1.0));
			double Nodule = 0, NoduleOn = 0;
			for (const double Band : {-4.58, -5.36})
			{
				const double Zb = Band + 0.05 * Noise(A, 0, 3, Band);
				// Nodules 10-25 cm across and half as tall (squashed only 1.3× into their band), so the 2.5 cm mesh draws them
				// as rounded knobs, not as flat plates with stepped edges (the audit: they read as extruded tiles).
				const FVector2D N = Stones(U, (Z - Zb) * 1.3, 0.24, 0.62, 0.06, 0.12, 0.04, uint32(-Band * 100), 1.5);
				if (FMath::Abs(Z - Zb) < 0.22 && N.X > Nodule)   // (wide enough that no nodule is cut flat by its band)
				{
					Nodule = N.X * (0.8 + 0.25 * Noise(A, Z, 30, 20));
					NoduleOn = FMath::SmoothStep(0.0, 1.0, N.Y);   // the cortex fades into the chalk, no hard outline
				}
			}
			Proud += wChalk * (Blocky + Bed + Nodule);
			Flint = FMath::Max(Flint, wChalk * NoduleOn);
		}
		S.Proud = Proud;
		const double Keep = 1 - Flint;
		S.Layers = FLinearColor(float(wSoil * Keep), float(wClay * Keep), float(wChalk * Keep), float(Flint));
		S.Extra = FVector2D(0, FMath::Clamp((Z - TopSub) / 0.06 + 0.5, 0.0, 1.0));
		return S;
	}

	FMesh GroundFace()
	{
		FMesh M;
		constexpr double Step = 0.0125;   // 1.25 cm: the flint nodules round, not stepped (2.5 cm drew them as plates)
		const int32 NA = FMath::RoundToInt32(Turn * EarthR / Step);
		const double Top = SlabBottom + 0.004, Bottom = GlassBottom - 0.004;
		const int32 NZ = FMath::RoundToInt32((Top - Bottom) / Step);
		M.Reserve((NA + 1) * (NZ + 1));
		TArray<int32> Ids;
		Ids.SetNumUninitialized((NA + 1) * (NZ + 1));
		for (int32 j = 0; j <= NZ; ++j)
		{
			const double Z = Top + (Bottom - Top) * j / NZ;
			for (int32 i = 0; i <= NA; ++i)
			{
				const double A = Turn * (i % NA) / NA;
				const FSample S = Ground(A, Z);
				const double R = EarthR - S.Proud;
				Ids[j * (NA + 1) + i] = M.V(Polar(R, A, Z), -Outward(A), FVector2D(Turn * i / NA * EarthR, -Z), S.Layers, S.Extra, Along(A));
			}
		}
		for (int32 j = 0; j < NZ; ++j)
		{
			for (int32 i = 0; i < NA; ++i)
			{
				const int32 A0 = Ids[j * (NA + 1) + i], A1 = Ids[j * (NA + 1) + i + 1];
				const int32 B0 = Ids[(j + 1) * (NA + 1) + i], B1 = Ids[(j + 1) * (NA + 1) + i + 1];
				M.Quad(A0, A1, B1, B0);
			}
		}
		// The seam round the circle shares positions (i = 0 and i = NA), so the smoothed normals match there too.
		M.SmoothNormals(true);
		return M;
	}

	/**
	 * An ammonite (a Cretaceous chalk ammonite, Mantelliceras-like: ribbed, moderately involute), lying in the cut
	 * face with its coil facing the shaft, half exposed: a whorl tube round a logarithmic spiral, three and a half
	 * whorls, ribbed, its section compressed.
	 */
	FMesh Ammonite()
	{
		FMesh M;
		const double A = PlanAngle(AmmoniteBearing);
		const FVector W = -Outward(A), Uax = Along(A), Vax(0, 0, 1);
		const FVector Shell = Polar(EarthR - 0.004, A, AmmoniteZ);   // its mid-plane a few mm in front of the face
		const double Whorls = 3.4, Phi1 = Whorls * Turn;
		const double B = FMath::Loge(2.1) / Turn;                     // the whorl expands 2.1× a turn
		const double Rho1 = 0.5 * AmmoniteDiameter / (1 + 0.46);      // the outer whorl's centreline
		const int32 NPhi = 520, NPsi = 22;
		const FLinearColor Chalk(0, 0, 1, 0);
		TArray<int32> Ids;
		for (int32 i = 0; i <= NPhi; ++i)
		{
			const double Phi = Phi1 * i / NPhi;
			const double Rho = Rho1 * FMath::Exp(B * (Phi - Phi1));
			const double Rib = 1.0 + 0.07 * FMath::Pow(FMath::Max(0.0, FMath::Cos(Phi * 26.0 / 1.0)), 3.0) * FMath::Clamp(Phi / Turn, 0.0, 1.0);
			const double S = 0.46 * Rho, T = 0.30 * Rho;
			const FVector Radial = Uax * FMath::Cos(-Phi) + Vax * FMath::Sin(-Phi);
			const FVector Line = Shell + Radial * Rho;
			for (int32 j = 0; j <= NPsi; ++j)
			{
				const double Psi = UE_DOUBLE_PI * (-0.1 + 1.2 * j / NPsi);   // the exposed half, a little beyond
				const double Out = Rib * (FMath::Cos(Psi) > 0 ? Rib : 1.0);
				const FVector P = Line + Radial * (S * FMath::Cos(Psi) * Out) + W * (T * FMath::Sin(Psi) * Rib);
				const FVector N = (Radial * (FMath::Cos(Psi) / S) + W * (FMath::Sin(Psi) / T)).GetSafeNormal();
				Ids.Add(M.V(P, N, FVector2D(Phi * 0.05, Psi * 0.05), Chalk, FVector2D(1, 1), Along(A)));
			}
		}
		for (int32 i = 0; i < NPhi; ++i)
		{
			for (int32 j = 0; j < NPsi; ++j)
			{
				const int32 A0 = Ids[i * (NPsi + 1) + j], A1 = Ids[i * (NPsi + 1) + j + 1];
				const int32 B0 = Ids[(i + 1) * (NPsi + 1) + j], B1 = Ids[(i + 1) * (NPsi + 1) + j + 1];
				M.Quad(A0, A1, B1, B0);
			}
		}
		M.SmoothNormals(false);
		return M;
	}

	/** A heart urchin (Micraster, the chalk's commonest fossil), smaller, nearer the eye on the way down. */
	FMesh Urchin()
	{
		FMesh M;
		const double A = PlanAngle(286.0);
		const FVector W = -Outward(A), Uax = Along(A), Vax(0, 0, 1);
		const FVector Heart = Polar(EarthR + 0.012, A, -5.22);
		const int32 NU = 40, NV = 20;
		const FLinearColor Chalk(0, 0, 1, 0);
		TArray<int32> Ids;
		for (int32 j = 0; j <= NV; ++j)
		{
			const double Th = UE_DOUBLE_PI * 0.5 * j / NV;          // from the crown down to the rim
			for (int32 i = 0; i <= NU; ++i)
			{
				const double Ph = Turn * i / NU;
				// A heart: a front notch; five petals as faint grooves.
				const double Notch = 1.0 - 0.12 * FMath::Exp(-FMath::Square(FMath::Fmod(Ph + Turn, Turn) - UE_DOUBLE_PI / 2) / 0.05);
				const double Petal = 1.0 - 0.025 * FMath::Pow(FMath::Max(0.0, FMath::Cos(5 * Ph)), 8) * FMath::Sin(Th * 2);
				const double R = 0.032 * Notch * Petal;
				const FVector P = Heart + (Uax * FMath::Cos(Ph) * 1.0 + Vax * FMath::Sin(Ph) * 0.88) * R * FMath::Sin(Th + 0.25) + W * 0.024 * FMath::Cos(Th);
				Ids.Add(M.V(P, (P - Heart).GetSafeNormal(), FVector2D(Ph, Th), Chalk, FVector2D(0.7, 1), Uax));
			}
		}
		for (int32 j = 0; j < NV; ++j)
		{
			for (int32 i = 0; i < NU; ++i)
			{
				M.Quad(Ids[j * (NU + 1) + i], Ids[j * (NU + 1) + i + 1], Ids[(j + 1) * (NU + 1) + i + 1], Ids[(j + 1) * (NU + 1) + i]);
			}
		}
		M.SmoothNormals(false);
		return M;
	}
}

namespace CB = CubeBuild;

ACubeStructure::ACubeStructure()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	auto Make = [this](const TCHAR* ComponentName)
	{
		UProceduralMeshComponent* Component = CreateDefaultSubobject<UProceduralMeshComponent>(ComponentName);
		Component->SetupAttachment(RootComponent);
		Component->bUseAsyncCooking = true;
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCanEverAffectNavigation(false);
		return Component;
	};
	Panels = Make(TEXT("Panels"));
	Panels->bUseComplexAsSimpleCollision = true;
	Panels->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Panels->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Box = Make(TEXT("Box"));
	Glass = Make(TEXT("Glass"));
	Glass->SetCastShadow(false);
	Metal = Make(TEXT("Metal"));
	Ground = Make(TEXT("Ground"));

	// The room's exposure: a post-process bound to the room's box (queried only, so the post-process can test the eyes).
	Room = CreateDefaultSubobject<UBoxComponent>(TEXT("Room"));
	Room->SetupAttachment(RootComponent);
	Room->SetRelativeLocation(FVector(0, 0, CubePlan::Centre * 100.0));
	Room->SetBoxExtent(FVector(CubePlan::Half * 100.0));
	Room->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Room->SetCollisionResponseToAllChannels(ECR_Ignore);
	Room->SetCanEverAffectNavigation(false);
	Room->SetHiddenInGame(true);
	RoomPost = CreateDefaultSubobject<UPostProcessComponent>(TEXT("RoomPost"));
	RoomPost->SetupAttachment(Room);
	RoomPost->bUnbound = false;
	RoomPost->Priority = 50.f;
	RoomPost->BlendRadius = 100.f;

	for (int32 K = 0; K < CubePlan::GlassTiers; ++K)
	{
		for (int32 j = 0; j < 8; ++j)
		{
			URectLightComponent* L = CreateDefaultSubobject<URectLightComponent>(*FString::Printf(TEXT("CavityLight%d_%d"), K + 1, j + 1));
			L->SetupAttachment(RootComponent);
			L->SetMobility(EComponentMobility::Movable);
			CavityLights.Add(L);
		}
	}

	auto Path = [](const TCHAR* Folder, const TCHAR* Name)
	{
		return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(FString::Printf(TEXT("%s/%s.%s"), Folder, Name, Name)));
	};
	const TCHAR* Materials = TEXT("/Game/Museum/Materials");
	const TCHAR* Journeys = TEXT("/Game/Museum/Journeys/Materials");
	PanelMaterial = Path(Journeys, TEXT("M_CubePanel"));
	PanelFallbackMaterial = Path(Materials, TEXT("M_Marble_Nero"));
	ConcreteMaterial = Path(Materials, TEXT("M_Stone"));
	GlassMaterial = Path(Materials, TEXT("M_Glass"));
	GiltMaterial = Path(Materials, TEXT("M_Gilt"));
	DarkBronzeMaterial = Path(Materials, TEXT("M_Bronze_Patina"));
	RailMaterial = Path(Materials, TEXT("M_Bronze_Brushed"));
	GroundMaterial = Path(Journeys, TEXT("M_CubeStrata"));
	GroundFallbackMaterial = Path(Materials, TEXT("M_Strata"));

	AddTags();
	PlaceLights();
}

TArray<FString> ACubeStructure::GetReplacedImportPrims()
{
	return {
		TEXT("/Museum/Elan/The_Square"),
		TEXT("/Museum/Elan/SpotLight_3"), TEXT("/Museum/Elan/SpotLight_4"), TEXT("/Museum/Elan/SpotLight_5"),
		TEXT("/Museum/Elan/SpotLight_6"), TEXT("/Museum/Elan/SpotLight_7"),
	};
}

void ACubeStructure::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
	ApplyMaterials();
	PlaceLights();
	AddTags();
}

void ACubeStructure::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	PlaceLights();
	AddTags();
}

void ACubeStructure::BeginPlay()
{
	Super::BeginPlay();
	Build();
	ApplyMaterials();
	// The room's exposure (see RoomPost).
	FPostProcessSettings& PP = RoomPost->Settings;
	PP.bOverride_AutoExposureMinBrightness = true;
	PP.bOverride_AutoExposureMaxBrightness = true;
	PP.bOverride_AutoExposureBias = true;
	PP.bOverride_AutoExposureBiasCurve = true;
	PP.AutoExposureMinBrightness = RoomMinEV;
	PP.AutoExposureMaxBrightness = RoomMaxEV;
	PP.AutoExposureBias = 0.f;
	PP.AutoExposureBiasCurve = nullptr;
}

void ACubeStructure::AddTags()
{
	Tags.AddUnique(FName(TEXT("musee.building")));
	Tags.AddUnique(FName(TEXT("musee.wing:Elan")));
	Tags.AddUnique(FName(TEXT("musee.cube")));
}

void ACubeStructure::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }
	for (UProceduralMeshComponent* C : {Panels.Get(), Box.Get(), Glass.Get(), Metal.Get(), Ground.Get()})
	{
		if (C) { C->ClearAllMeshSections(); }
	}
	CB::PanelFaces().Write(Panels, 0, true);
	CB::ConcreteBox().Write(Box, 0, false);
	CB::GlassPanes().Write(Glass, 0, false);
	const CB::FMetal M = CB::Metalwork();
	M.Gilt.Write(Metal, 0, false);
	M.Dark.Write(Metal, 1, false);
	M.Rail.Write(Metal, 2, false);
	CB::GroundFace().Write(Ground, 0, false);
	CubeMesh::FMesh Fossils = CB::Ammonite();
	Fossils.Append(CB::Urchin());
	Fossils.Write(Ground, 1, false);
}

void ACubeStructure::ApplyMaterials()
{
	auto Pick = [](const TSoftObjectPtr<UMaterialInterface>& Wanted, const TSoftObjectPtr<UMaterialInterface>& Fallback) -> UMaterialInterface*
	{
		if (UMaterialInterface* Loaded = Wanted.LoadSynchronous()) { return Loaded; }
		return Fallback.IsNull() ? nullptr : Fallback.LoadSynchronous();
	};
	const TSoftObjectPtr<UMaterialInterface> None;
	auto Set = [](UProceduralMeshComponent* C, int32 Section, UMaterialInterface* M) { if (C && M) { C->SetMaterial(Section, M); } };
	Set(Panels, 0, Pick(PanelMaterial, PanelFallbackMaterial));
	Set(Box, 0, Pick(ConcreteMaterial, None));
	Set(Glass, 0, Pick(GlassMaterial, None));
	Set(Metal, 0, Pick(GiltMaterial, None));
	Set(Metal, 1, Pick(DarkBronzeMaterial, GiltMaterial));
	Set(Metal, 2, Pick(RailMaterial, GiltMaterial));
	UMaterialInterface* Strata = Pick(GroundMaterial, GroundFallbackMaterial);
	Set(Ground, 0, Strata);
	Set(Ground, 1, Strata);
}

void ACubeStructure::PlaceLights()
{
	int32 Index = 0;
	for (int32 K = 0; K < CubePlan::GlassTiers; ++K)
	{
		const double Z = CB::TierZ(K) - 0.05;
		for (int32 j = 0; j < 8; ++j, ++Index)
		{
			if (!CavityLights.IsValidIndex(Index) || !CavityLights[Index]) { continue; }
			URectLightComponent* L = CavityLights[Index];
			const double A = CB::Turn * (j + 0.5) / 8;
			L->SetRelativeLocationAndRotation(CubeMesh::Polar(CB::LedR, A, Z) * CubePlan::Cm, FRotator(-90.0, FMath::RadiansToDegrees(A), 0.0));
			L->SetIntensityUnits(ELightUnits::Lumens);
			L->SetIntensity(CavityLumens);
			L->SetUseTemperature(true);
			L->SetTemperature(CavityKelvin);
			L->SetSourceWidth(170.f);
			L->SetSourceHeight(2.f);
			L->SetBarnDoorAngle(60.f);
			L->SetBarnDoorLength(4.f);
			L->SetAttenuationRadius(260.f);
			L->SetCastShadows(true);
			// They light the ground's face; the glass in front of them has nothing to scatter it (M_Glass: no diffuse), and
			// lighting the car's and the shaft's glass per pixel with 32 shadowed lines cost ~5 ms at 4K.
			L->SetAffectTranslucentLighting(false);
		}
	}
}

namespace
{
	TAutoConsoleVariable<float> CVarCubeRestEV(TEXT("musee.CubeRestEV"), -1.f,
		TEXT("Élan Cube: the eye's exposure (EV100) at rest in the room, the car cleared (below 0: the Cube's own, RestEV)."));
	FAutoConsoleCommandWithWorldAndArgs GCubeGridCommand(
		TEXT("musee.CubeGrid"),
		TEXT("musee.CubeGrid <x>: the panels' grid light at rest, x the default (MPC_Cube GridGlow; tests)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UMaterialParameterCollection* MPC = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Museum/Journeys/Materials/MPC_Cube.MPC_Cube"));
			if (World && MPC && Args.Num() > 0) { UKismetMaterialLibrary::SetScalarParameterValue(World, MPC, TEXT("GridGlow"), FCString::Atof(*Args[0])); }
		}));
}

void ACubeStructure::SetRest(float Amount)
{
	Amount = FMath::Clamp(Amount, 0.f, 1.f);
	const float Override = CVarCubeRestEV.GetValueOnGameThread();
	const float EV = Override >= 0.f ? Override : RestEV;
	if (FMath::Abs(Amount - AppliedRest) < 0.002f && EV == AppliedRestEV) { return; }
	AppliedRest = Amount;
	AppliedRestEV = EV;
	FPostProcessSettings& PP = RoomPost->Settings;
	PP.AutoExposureMinBrightness = FMath::Lerp(RoomMinEV, EV, Amount);
	PP.AutoExposureMaxBrightness = FMath::Lerp(RoomMaxEV, EV, Amount);
}

void ACubeStructure::SetJourneyMode(bool bOn)
{
	if (bJourneyMode == bOn) { return; }
	bJourneyMode = bOn;
	// The world beyond the panels shows through them: nothing behind them may stand in its way.
	SetActorHiddenInGame(false);
	for (UActorComponent* C : GetComponents())
	{
		USceneComponent* S = Cast<USceneComponent>(C);
		if (!S || S == RootComponent) { continue; }
		// The panels (procedural, or baked: "Panels_Baked…") stay; the box, the shaft and the ground go.
		const bool bPanels = S->GetName().StartsWith(TEXT("Panels"));
		S->SetVisibility(bPanels || !bOn);
	}
}
