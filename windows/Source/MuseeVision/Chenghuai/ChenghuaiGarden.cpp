#include "Chenghuai/ChenghuaiGeometry.h"

#include "Chenghuai/ChenghuaiGardenPlan.h"
#include "Chenghuai/ChenghuaiHall.h"
#include "Chenghuai/ChenghuaiParts.h"
#include "Chenghuai/ChenghuaiPlan.h"
#include "Chenghuai/ChenghuaiRockery.h"
#include "Chenghuai/ChenghuaiWater.h"

/**
 * The garden (Garden board, 臥遊): white walls under grey tile copings, the covered walk along the house's wall, the pond
 * with its rock-lined banks, the zigzag bridge, the flower hall (花厅 · 林泉) and its terrace, the water pavilion (水榭 ·
 * 知魚), the rockery (假山) and its hexagonal pavilion (亭 · 見山), the paths of pebble mosaic. Chestnut timber.
 */
namespace ChenghuaiGardenBuild
{
	namespace Kit = ChenghuaiKit;
	namespace CB = ChenghuaiBuild;
	namespace CH = Chenghuai;
	namespace GP = ChenghuaiGardenPlan;
	namespace CP = ChenghuaiParts;
	using namespace ChenghuaiHall;
	using Kit::FLocal;
	using Kit::FProfile;
	using CB::FChMeshes;

	constexpr double Ground = 0.02;          // the garden's earth, a little over the grounds' plane (a centimetre under the
	                                         // buildings' footing courses, which stood coplanar with it and flickered)
	constexpr double PondBed = -0.85;        // the pond's bed where it is deep
	constexpr double PondShallow = -0.46;    // over the Classical Hall's tribune

	FLocal Frame(double X, double Y, double UX, double UY, double VX, double VY)
	{
		FLocal L;
		L.Origin = FVector2D(X, Y);
		L.U = FVector2D(UX, UY);
		L.V = FVector2D(VX, VY);
		return L;
	}

	/** The bed's depth at a point: shallow over the tribune's masonry (r 7.9 about (0, −39.3), top −0.55). */
	double BedAt(const FVector2D& P)
	{
		const double R = FVector2D::Distance(P, FVector2D(CH::TribuneCX, CH::TribuneCY));
		const double T = FMath::Clamp((R - (CH::TribuneShell + 0.4)) / 1.5, 0.0, 1.0);
		return FMath::Lerp(PondShallow, PondBed, T);
	}

	TArray<FVector2D> Poly(const double (*Pts)[2], int32 N)
	{
		TArray<FVector2D> Out;
		for (int32 i = 0; i < N; ++i) { Out.Add(FVector2D(Pts[i][0], Pts[i][1])); }
		return Out;
	}

	// ------------------------------------------------------------------------------------------------ walls

	void Walls(FChMeshes& Out)
	{
		CP::FWallSpec W;
		W.BasePart = CB::BrickFine;
		W.BodyPart = CB::PlasterWhite;
		W.BaseH = 0.55;
		W.Height = 3.6;
		// North (y −61.9 … −61.4), east (x 16.4 … 16.9), south (y −17.5 … −17.0).
		// Leak windows (漏窗) of grey tile and brick in the white walls, where the garden's paths and the rockery look at
		// them (Garden board: ice-crack, coins, hexagons, crabapple, octagon), none behind the pavilion or the flower hall.
		using CP::EHole;
		CP::Wall(Out, Frame(0.0, CH::SiteN, 1, 0, 0, 1), CH::HouseE, CH::SiteE, 0.0, CH::Wall, W,
				 {{EHole::Window, 3.9, 4.9, 1.15, 2.15, 2}, {EHole::Window, 15.0, 16.0, 1.15, 2.15, 4}});
		CP::Wall(Out, Frame(CH::GardenInE, 0.0, 0, -1, 1, 0), -CH::InS, -CH::InN, 0.0, CH::Wall, W,
				 {{EHole::Window, 38.0, 39.0, 1.15, 2.15, 0}, {EHole::Window, 42.6, 43.6, 1.15, 2.15, 1}, {EHole::Window, 54.8, 55.8, 1.15, 2.15, 2}});
		CP::Wall(Out, Frame(0.0, CH::InS, 1, 0, 0, 1), CH::HouseE, CH::GardenInE, 0.0, CH::Wall, W,
				 {{EHole::Window, 4.4, 5.4, 1.15, 2.15, 4}, {EHole::Window, 14.2, 15.2, 1.15, 2.15, 0}});
	}

	// ------------------------------------------------------------------------------------------------ the ground and the pond

	void GroundAndPond(FChMeshes& Out)
	{
		const TArray<FVector2D> Edge = ChenghuaiWater::PondOutline();
		const TArray<FVector2D> Site = {FVector2D(CH::HouseE, CH::InN), FVector2D(CH::GardenInE, CH::InN), FVector2D(CH::GardenInE, CH::InS), FVector2D(CH::HouseE, CH::InS)};
		// The earth: a sheet at Ground with the pond cut out (the rockery and paths stand on it).
		Out[CB::GardenGround].Begin(TEXT("Garden ground"), false);
		Kit::Planar(Out[CB::GardenGround].M, Site, {Edge}, FVector(0, 0, Ground), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1),
					&Kit::PlanUV);
		// The bank: from the edge down under the water, stepping in to the bed; then the bed.
		const int32 N = Edge.Num();
		FVector2D C = FVector2D::ZeroVector;
		for (const FVector2D& P : Edge) { C += P; }
		C /= N;
		const double Sign = Kit::Area2(Edge) > 0.0 ? 1.0 : -1.0;
		auto Inward = [&](int32 i)
		{
			const FVector2D T = (Edge[(i + 1) % N] - Edge[(i + N - 1) % N]).GetSafeNormal();
			return FVector2D(-T.Y, T.X) * Sign;   // left of a counter-clockwise walk: inwards
		};
		TArray<FVector> Lip, Toe;
		TArray<FVector2D> Bed;
		for (int32 i = 0; i < N; ++i)
		{
			const FVector2D In = Inward(i);
			const FVector2D P = Edge[i], Q = Edge[i] + In * 0.45;
			Lip.Add(FVector(P.X, P.Y, Ground));
			Toe.Add(FVector(Q.X, Q.Y, BedAt(Q)));
			Bed.Add(Q);
		}
		Lip.Add(FVector(Lip[0]));
		Toe.Add(FVector(Toe[0]));
		Out[CB::PondBed].Begin(TEXT("Pond bank"), false);
		for (int32 i = 0; i < N; ++i)
		{
			const FVector A = Lip[i], B = Lip[i + 1], Cc = Toe[i + 1], D = Toe[i];
			const FVector Nrm = FVector::CrossProduct(B - A, D - A).GetSafeNormal();
			const FVector Up = FVector(C.X - A.X, C.Y - A.Y, 0.0).GetSafeNormal() + FVector(0, 0, 0.3);
			Out[CB::PondBed].M.Rect(A, B, Cc, D, FVector::DotProduct(Nrm, Up) >= 0.0 ? Nrm : -Nrm);
		}
		// The bed: a fan from the middle, each point at its own depth.
		Out[CB::PondBed].Begin(TEXT("Pond bed"), false);
		{
			Kit::FMeshData& M = Out[CB::PondBed].M;
			const TArray<int32> Tris = Kit::EarClip(Bed);
			const int32 Base = M.Positions.Num();
			for (const FVector2D& P : Bed) { M.Vertex(FVector(P.X, P.Y, BedAt(P)), FVector(0, 0, 1), P); }
			for (int32 t = 0; t + 2 < Tris.Num(); t += 3) { M.Tri(Base + Tris[t], Base + Tris[t + 1], Base + Tris[t + 2]); }
		}
		// Rocks along the edge (湖石驳岸): irregular blocks sitting on the bank, reaching a little over the water.
		ChenghuaiRockery::EdgeRocks(Out[CB::Rockery], Edge, Ground, CH::WaterLevel);
		// The visitor's guard: a low fence 0.4 m in over the water all round (not drawn), open where the zigzag bridge
		// leaves the bank (a guard across its ends kept the visitor off it).
		auto NearBridge = [](const FVector2D& P)
		{
			for (int32 b = 0; b + 1 < 6; ++b)
			{
				const FVector2D BA(CH::Bridge[b][0], CH::Bridge[b][1]), BB(CH::Bridge[b + 1][0], CH::Bridge[b + 1][1]);
				const FVector2D D = (BB - BA).GetSafeNormal();
				const FVector2D QA = BA - D * 0.6, QB = BB + D * 0.6;   // the slabs run on half a width past each turn
				const double T = FMath::Clamp(FVector2D::DotProduct(P - QA, QB - QA) / FMath::Max(1e-6, (QB - QA).SizeSquared()), 0.0, 1.0);
				if (FVector2D::Distance(P, QA + (QB - QA) * T) < 0.8) { return true; }
			}
			return false;
		};
		for (int32 i = 0; i < N; ++i)
		{
			const FVector2D A = Edge[i] + Inward(i) * 0.25, B = Edge[(i + 1) % N] + Inward((i + 1) % N) * 0.25;
			if (NearBridge(A) || NearBridge(B) || NearBridge(0.5 * (A + B))) { continue; }
			Kit::Member(Out[CB::Guard], TEXT("Pond guard"), FVector(A.X, A.Y, 0.5), FVector(B.X, B.Y, 0.5), FVector(0, 0, 1), 1.2, 0.05);
		}
	}

	// ------------------------------------------------------------------------------------------------ paths

	/** A path of pebble mosaic along a centre line, edged with bluestone kerbs, on the ground. */
	void PathAlong(FChMeshes& Out, const TArray<FVector2D>& Centre, double Width)
	{
		FProfile Bed;
		Bed.bClosed = true;
		const double H = 0.5 * Width;
		Bed.Add(-H + 0.06, -0.06).Add(H - 0.06, -0.06).Add(H - 0.06, 0.02).Add(-H + 0.06, 0.02);
		for (const TArray<Kit::FFrame>& Run : SalonKit::PlanRuns(Centre, false, 12.0))
		{
			TArray<Kit::FFrame> R = Run;
			for (Kit::FFrame& F : R) { F.Origin.Z = Ground; }
			Kit::SweepSolid(Out[CB::GardenPebble], TEXT("Pebble path"), R, Bed);
			for (const double Side : {-1.0, 1.0})
			{
				FProfile K;
				K.bClosed = true;
				const double A0 = Side * (H - 0.06), A1 = Side * H;
				K.Add(FMath::Min(A0, A1), -0.08).Add(FMath::Max(A0, A1), -0.08).Add(FMath::Max(A0, A1), 0.035).Add(FMath::Min(A0, A1), 0.035);
				Kit::SweepSolid(Out[CB::BlueStone], TEXT("Path kerb"), R, K);
			}
		}
	}

	void Paths(FChMeshes& Out)
	{
		const TArray<TPair<const double (*)[2], int32>> Lines = {
			{GP::GardenPath0, GP::GardenPath0Count}, {GP::GardenPath1, GP::GardenPath1Count}, {GP::GardenPath2, GP::GardenPath2Count},
			{GP::GardenPath3, GP::GardenPath3Count}, {GP::GardenPath4, GP::GardenPath4Count}, {GP::GardenPath5, GP::GardenPath5Count},
			{GP::GardenPath6, GP::GardenPath6Count}};
		for (const auto& L : Lines) { PathAlong(Out, Poly(L.Key, L.Value), 0.95); }
	}

	// ------------------------------------------------------------------------------------------------ the bridge

	void ZigzagBridge(FChMeshes& Out)
	{
		// Bluestone slabs on stone piers, 1.0 m wide, 0.12 over the water; a low kerb along each side (no rail: Suzhou).
		const double Top = 0.1, Thick = 0.14;
		for (int32 i = 0; i + 1 < 6; ++i)
		{
			const FVector2D A(CH::Bridge[i][0], CH::Bridge[i][1]), B(CH::Bridge[i + 1][0], CH::Bridge[i + 1][1]);
			const FVector2D D = (B - A).GetSafeNormal();
			// Each run overlaps the next by half its width at the turns.
			const FVector PA(A.X - D.X * 0.5, A.Y - D.Y * 0.5, Top - 0.5 * Thick), PB(B.X + D.X * 0.5, B.Y + D.Y * 0.5, Top - 0.5 * Thick);
			Kit::Member(Out[CB::BlueStone], TEXT("Bridge slab"), PA, PB, FVector(-D.Y, D.X, 0.0), 1.0, Thick + 0.002 * (i % 2), 0.01);
			// Piers every 1.6 m.
			const double Len = FVector2D::Distance(A, B);
			const int32 NP = FMath::Max(1, FMath::RoundToInt32(Len / 1.6));
			for (int32 k = 0; k <= NP; ++k)
			{
				const FVector2D P = A + D * (Len * k / NP);
				Kit::Box(Out[CB::BlueStone], TEXT("Bridge pier"), FVector(P.X - 0.25, P.Y - 0.25, PondBed - 0.1), FVector(P.X + 0.25, P.Y + 0.25, Top - Thick));
			}
			// The visitor's guard at the sides (not drawn: the edge is open, as it is in Suzhou).
			for (const double S : {-1.0, 1.0})
			{
				const FVector2D O = FVector2D(-D.Y, D.X) * (0.52 * S);
				Kit::Member(Out[CB::Guard], TEXT("Bridge guard"), FVector(A.X + O.X, A.Y + O.Y, 0.6), FVector(B.X + O.X, B.Y + O.Y, 0.6), FVector(0, 0, 1), 1.0, 0.04);
			}
		}
	}

	// ------------------------------------------------------------------------------------------------ the buildings

	/** The covered walk along the house's wall: a single slope from the wall down to chestnut columns on the garden side. */
	FHallSpec Walk()
	{
		FHallSpec H;
		H.Name = TEXT("Covered walk");
		H.L = Frame(CH::WalkColX, CH::WalkY1, 0, -1, -1, 0);        // u north along the wall, v west to it
		H.U0 = 0.0; H.U1 = CH::WalkY1 - CH::WalkY0;
		H.GableT = 0.0; H.bGableWalls = false;
		H.ColU.Empty();
		for (double U = 0.3; U < H.U1; U += 2.7) { H.ColU.Add(U); }
		H.Floor = 0.15; H.ColH = 2.1; H.ColD = 0.2;
		H.PurlinV = {0.0, 1.25};
		H.Rise = {0.45};
		H.Ridge = ERidge::None; H.Tiles = ChenghuaiRoof::ETiles::He;
		H.bBackWall = false; H.bSealedBack = false; H.EaveFront = 0.55; H.EaveBack = 0.06;
		for (int32 i = 0; i + 1 < H.ColU.Num(); ++i) { H.Bays.Add(EBay::Open); }
		H.bPainted = false; H.bRafterEnds = false; H.TimberPart = CB::Chestnut; H.FloorPart = CB::BlueStone; H.PlatformPart = CB::BlueStone;
		H.PlatformFront = -0.15; H.PlatformBack = 1.28; H.PlatformU0 = -0.1; H.PlatformU1 = H.U1 + 0.1;
		H.RoofU0 = -0.3; H.RoofU1 = H.U1 + 0.3;
		return H;
	}

	FHallSpec FlowerHall()
	{
		FHallSpec H;
		H.Name = TEXT("Flower hall");
		H.L = Frame(6.6, CH::FlowerHallS, 1, 0, 0, -1);              // faces south onto its terrace
		H.U0 = 0.0; H.U1 = 8.1; H.GableT = 0.4;
		H.ColU = {0.4, 2.4, 5.7, 7.7};
		H.Floor = 0.45; H.PlatformFront = -0.3; H.PlatformBack = 3.95; H.PlatformU0 = -0.05; H.PlatformU1 = 8.15;
		H.ColH = 3.0; H.ColD = 0.24;
		H.PurlinV = {0.0, 1.15, 1.625, 2.125, 2.6, 3.75};
		H.Rise = {0.5, 0.75, 0.0, -0.75, -0.5};
		H.bRolled = true; H.Tiles = ChenghuaiRoof::ETiles::He; H.Ridge = ERidge::Rolled;
		H.bBackWall = true; H.BackWallV0 = 3.6; H.BackWallV1 = 3.9; H.bSealedBack = false; H.EaveFront = 0.8; H.EaveBack = 0.7;
		H.Bays = {EBay::Doors, EBay::DoorsOpen, EBay::Doors};
		// Its 长窗 in the gardens' own lattice: cracked ice in the side bays, the lantern (灯景) in the open middle one.
		H.Lattice = int32(ELattice::IceCrack); H.LatticeOpenBay = int32(ELattice::Lantern);
		H.bPainted = false; H.bRafterEnds = false; H.TimberPart = CB::Chestnut;
		H.WallPart = CB::PlasterWhite; H.BaseCourse = CB::BrickFine; H.FloorPart = CB::BlueStone; H.PlatformPart = CB::BlueStone;
		return H;
	}

	FHallSpec WaterPavilion()
	{
		FHallSpec H;
		H.Name = TEXT("Water pavilion");
		H.L = Frame(13.3, CH::WaterPavY1, 0, -1, 1, 0);              // faces west over the pond: u north, v east
		H.U0 = 0.0; H.U1 = 4.5;
		H.GableT = 0.0; H.bGableWalls = false;
		H.ColU = {0.3, 4.2};
		H.Floor = 0.35; H.ColH = 2.6; H.ColD = 0.2;
		H.PurlinV = {0.0, 1.05, 1.75, 2.8};
		H.Rise = {0.55, 0.0, -0.55};
		H.bRolled = true; H.Tiles = ChenghuaiRoof::ETiles::He; H.Ridge = ERidge::Rolled;
		H.bBackWall = false; H.bSealedBack = false; H.bBackColumns = true; H.EaveFront = 0.75; H.EaveBack = 0.1;
		H.Bays = {EBay::Open};
		H.bPainted = false; H.bRafterEnds = false; H.TimberPart = CB::Chestnut; H.FloorPart = CB::BlueStone; H.PlatformPart = CB::BlueStone;
		H.PlatformFront = -0.3; H.PlatformBack = 3.1; H.PlatformU0 = -0.05; H.PlatformU1 = 4.55;
		H.RoofU0 = -0.55; H.RoofU1 = 5.05;
		return H;
	}

	/** The terrace (月台) before the flower hall, a step down from it and two to the garden. */
	void Terrace(FChMeshes& Out)
	{
		Kit::Box(Out[CB::BlueStone], TEXT("Terrace"), FVector(CH::TerraceX0, CH::TerraceY1 - 1.5, -0.1), FVector(CH::TerraceX1, CH::TerraceY1, 0.3));
		for (int32 k = 1; k < 3; ++k)
		{
			Kit::Box(Out[CB::BlueStone], TEXT("Terrace step"), FVector(9.9, CH::TerraceY1 - 0.02 + 0.3 * (k - 1), -0.08), FVector(11.4, CH::TerraceY1 + 0.3 * k, 0.3 - 0.1 * k));
		}
	}

	/**
	 * 遊目: the twelve 书条石 set in the walk's wall (the house's east wall, its garden face at x 2.3), two to a bay in
	 * the six bays without a lattice window, read from the moon gate back to the gate (north to south, right to left as
	 * you face the wall). Each 0.9 × 0.32 m, 3 cm proud, its face an atlas cell (T_ch_shutiao: 3 × 4 cells, left to
	 * right, top to bottom, in reading order); the edges arrised.
	 */
	void EngravedStones(FChMeshes& Out)
	{
		const double Bays[6] = {-53.4, -42.6, -39.9, -37.2, -34.5, -29.1};   // each bay's north column, 2.7 m to the next
		const double W = 0.9, H = 0.32, X0 = CH::WalkX0 - 0.01, X1 = CH::WalkX0 + 0.03, Z0 = 0.15 + 1.02;
		Kit::FPart& P = Out[CB::Engraved];
		int32 k = 0;
		for (double B : Bays)
		{
			for (int32 j = 0; j < 2; ++j, ++k)
			{
				// Viewed facing west: the reader's right is north (−y), so the text's start is at the stone's north end.
				const double YN = B + 0.35 + j * (W + 0.3), YS = YN + W;
				Kit::Box(P, TEXT("Engraved stone body"), FVector(X0, YN, Z0), FVector(X1 - 0.0008, YS, Z0 + H));
				// The face: its cell of the atlas.
				const double CU = (k % 3) / 3.0, CV = (k / 3) / 4.0;
				P.Begin(TEXT("Engraved stone face"), false, TEXT("Engraved stone body"));
				Kit::FMeshData& M = P.M;
				const FVector N(1, 0, 0);
				const int32 A = M.Vertex(FVector(X1, YN, Z0 + H), N, FVector2D(CU + 1.0 / 3.0, CV));
				const int32 Bv = M.Vertex(FVector(X1, YS, Z0 + H), N, FVector2D(CU, CV));
				const int32 C = M.Vertex(FVector(X1, YS, Z0), N, FVector2D(CU, CV + 0.25));
				const int32 D = M.Vertex(FVector(X1, YN, Z0), N, FVector2D(CU + 1.0 / 3.0, CV + 0.25));
				M.Quad(A, Bv, C, D);
			}
		}
	}

	void BuildGarden(FChMeshes& Out)
	{
		EngravedStones(Out);
		Walls(Out);
		GroundAndPond(Out);
		Paths(Out);
		ZigzagBridge(Out);
		BuildHall(Out, Walk());
		BuildHall(Out, FlowerHall());
		Terrace(Out);
		BuildHall(Out, WaterPavilion());
		ChenghuaiRockery::BuildRockery(Out);
	}
}

namespace ChenghuaiBuild
{
	void BuildGarden(FChMeshes& Out) { ChenghuaiGardenBuild::BuildGarden(Out); }

	void GardenLights(TArray<FChLight>& Out) {}
}
