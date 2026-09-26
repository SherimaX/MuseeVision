#include "Chenghuai/ChenghuaiGeometry.h"

#include "Chenghuai/ChenghuaiHall.h"
#include "Chenghuai/ChenghuaiPaint.h"
#include "Chenghuai/ChenghuaiParts.h"
#include "Chenghuai/ChenghuaiPlan.h"

/**
 * The house (Plan board): every building's spec in its own frame, the walls, the gate, the screen wall, the festooned
 * gate, the galleries, the courts. The garden (ChenghuaiGarden.cpp) and the display furniture (ChenghuaiDisplay.cpp)
 * add theirs.
 */
namespace ChenghuaiBuild
{
	void BuildGarden(FChMeshes& Out);
	void BuildDisplay(FChMeshes& Out);
	void GardenLights(TArray<FChLight>& Out);
	void DisplayLights(TArray<FChLight>& Out);
}

namespace ChenghuaiHouse
{
	namespace Kit = ChenghuaiKit;
	namespace CB = ChenghuaiBuild;
	namespace CH = Chenghuai;
	namespace CP = ChenghuaiParts;
	using namespace ChenghuaiHall;
	using Kit::FLocal;
	using Kit::FProfile;
	using CB::FChMeshes;

	FLocal Frame(double X, double Y, double UX, double UY, double VX, double VY)
	{
		FLocal L;
		L.Origin = FVector2D(X, Y);
		L.U = FVector2D(UX, UY);
		L.V = FVector2D(VX, VY);
		return L;
	}

	/** The spec mirrored about the inner court's axis (x' = 2·Axis − x): the east side's halls and galleries. */
	FHallSpec Mirror(FHallSpec S)
	{
		S.L.Origin.X = 2.0 * CH::Axis - S.L.Origin.X;
		S.L.U.X = -S.L.U.X;
		S.L.V.X = -S.L.V.X;
		S.Name += TEXT(" (east)");
		return S;
	}

	TArray<double> Even(double A, double Step, int32 Count)
	{
		TArray<double> Out;
		for (int32 k = 0; k < Count; ++k) { Out.Add(A + Step * k); }
		return Out;
	}

	// ------------------------------------------------------------------------------------------------ the halls

	FHallSpec FrontRow()
	{
		FHallSpec H;
		H.Name = TEXT("Front row");
		H.L = Frame(CH::SiteW, CH::FrontRowFace, 1, 0, 0, 1);    // faces north: v runs south, to its back wall
		H.U0 = 0.0; H.U1 = 18.9; H.GableT = 0.5;
		H.ColU = Even(0.5, 17.9 / 6.0, 7);
		H.Floor = CH::FrontRowPlinth; H.PlatformFront = -0.6; H.PlatformBack = 5.3; H.PlatformU0 = 0.0; H.PlatformU1 = 18.4;
		H.ColH = CH::FrontRowCol; H.ColD = 0.26;
		H.PurlinV = {0.0, 1.2625, 2.525, 3.7875, 5.05};
		H.Rise = {0.5, 0.72, -0.72, -0.5};
		H.Tiles = ChenghuaiRoof::ETiles::He; H.Ridge = ERidge::Qingshui;
		H.FacadeV = 0.0; H.bBackWall = true; H.BackWallV0 = 4.8; H.BackWallV1 = 5.3; H.bSealedBack = true;
		H.EaveFront = 0.75; H.EaveBack = 0.28;
		H.Bays = {EBay::DoorsOpen, EBay::Windows, EBay::Windows, EBay::Windows, EBay::Windows, EBay::DoorsOpen};
		// Painted, restrained (the renderings: dark green lintels, one white panel with a small ink landscape a bay).
		H.bPainted = true; H.PaintRow = 2; H.bRafterEnds = false;
		H.WallPart = CB::BrickPointed; H.BaseCourse = CB::BrickFine; H.FloorPart = CB::FloorSmall;
		// Its east end is the gate's west wall (the gate builds it).
		H.bEndOpen = true; H.RoofU0 = 0.0; H.RoofU1 = 18.45;
		H.Steps = {{0.5 + 17.9 / 12.0, 1.5}, {18.4 - 17.9 / 12.0, 1.5}};
		return H;
	}

	FHallSpec GateHall()
	{
		FHallSpec H;
		H.Name = TEXT("Gate");
		H.L = Frame(-2.3, CH::GateColS, 1, 0, 0, -1);            // faces south
		H.U0 = 0.0; H.U1 = 4.6; H.GableT = 0.5;
		H.ColU = {0.5, 4.1};
		H.Floor = CH::GatePlinth; H.PlatformFront = -0.3; H.PlatformBack = 5.2; H.PlatformU0 = 0.0; H.PlatformU1 = 4.6;
		H.ColH = 2.95; H.ColD = 0.3;
		H.PurlinV = {0.0, 1.225, 2.45, 3.675, 4.9};
		H.Rise = {0.5, 0.72, -0.72, -0.5};
		H.Tiles = ChenghuaiRoof::ETiles::Tong; H.Ridge = ERidge::Qingshui;
		H.bBackWall = false; H.bSealedBack = false; H.EaveFront = 0.9; H.EaveBack = 0.9;
		H.Bays = {EBay::Open};
		H.bPainted = false; H.bRafterEnds = false; H.bColumns = false; H.bFrames = false;
		H.WallPart = CB::BrickFine; H.BaseCourse = CB::BrickFine; H.bInteriorPlaster = false; H.FloorPart = CB::FloorSmall;
		H.GableBackV = 5.36;                                        // on over the front row's corner
		H.Steps = {{2.3, 2.4, false, 0.22}, {2.3, 2.4, true, 0.28}};
		return H;
	}

	FHallSpec SideHall()
	{
		FHallSpec H;
		H.Name = TEXT("West side hall");
		H.L = Frame(CH::SideHallEave, -41.4, 0, 1, -1, 0);          // faces east: u south, v west
		H.U0 = 0.0; H.U1 = 10.4; H.GableT = 0.5;
		H.ColU = {0.5, 3.55, 6.85, 9.9};
		H.Floor = CH::SideHallPlinth; H.PlatformFront = -0.6; H.PlatformBack = 5.5; H.PlatformU0 = 0.0; H.PlatformU1 = 10.4;
		H.ColH = CH::SideHallCol; H.ColD = 0.27;
		H.PurlinV = {0.0, 1.1, 2.35, 3.1, 4.35, 5.25};
		H.Rise = {0.5, 0.7, 0.0, -0.7, -0.45};
		H.bRolled = true; H.Tiles = ChenghuaiRoof::ETiles::He; H.Ridge = ERidge::Rolled;
		H.FacadeV = 1.1; H.bBackWall = true; H.BackWallV0 = 5.0; H.BackWallV1 = 5.5; H.bSealedBack = true;
		H.EaveFront = 0.85; H.EaveBack = 0.3;
		H.Bays = {EBay::Windows, EBay::DoorsOpen, EBay::Windows};
		H.bPainted = true; H.PaintRow = 0;
		H.WallPart = CB::BrickHairline; H.BaseCourse = CB::BrickFine; H.FloorPart = CB::FloorSmall;
		// The veranda runs on through both gables (the galleries), 1 m wide.
		H.GableOpenings = {{false, 0.06, 1.04, 2.3, true}, {true, 0.06, 1.04, 2.3, true}};
		H.Steps = {{5.2, 2.0, false, 0.3}};
		return H;
	}

	FHallSpec MainHall()
	{
		FHallSpec H;
		H.Name = TEXT("Main hall");
		H.L = Frame(-14.7, CH::HallEave, 1, 0, 0, -1);              // faces south
		H.U0 = 0.0; H.U1 = 11.0; H.GableT = 0.5;
		H.ColU = {0.5, 3.55, 7.45, 10.5};
		H.Floor = CH::HallPlinth; H.PlatformFront = -0.7; H.PlatformBack = 7.9; H.PlatformU0 = 0.0; H.PlatformU1 = 11.0;
		H.ColH = CH::HallCol; H.ColD = 0.32;
		H.PurlinV = {0.0, 1.3, 2.575, 3.85, 5.1, 6.36, 7.65};
		H.Rise = {0.5, 0.7, 0.9, -0.9, -0.7, -0.5};
		H.Tiles = ChenghuaiRoof::ETiles::Tong; H.Ridge = ERidge::Qingshui;
		H.FacadeV = 1.3; H.InnerColV = {6.36};
		H.bBackWall = true; H.BackWallV0 = 7.4; H.BackWallV1 = 7.9; H.bSealedBack = true;
		H.EaveFront = 1.1; H.EaveBack = 0.32;
		H.Bays = {EBay::Windows, EBay::DoorsOpen, EBay::Windows};
		H.bPainted = true; H.PaintRow = 1;
		H.WallPart = CB::BrickHairline; H.BaseCourse = CB::BrickFine; H.FloorPart = CB::FloorLarge;
		// The veranda's ends open through the gables (the galleries); a door into each ear room.
		H.GableOpenings = {{false, 0.08, 1.22, 2.45, true}, {true, 0.08, 1.22, 2.45, true},
						   {false, 4.4, 5.6, 2.3, true}, {true, 4.4, 5.6, 2.3, true}};
		H.Steps = {{5.5, 3.0, false, 0.25}};
		return H;
	}

	FHallSpec EarRoom()
	{
		FHallSpec H;
		H.Name = TEXT("West ear room");
		H.L = Frame(-17.7, CH::EarS, 1, 0, 0, -1);                  // faces south
		H.U0 = 0.0; H.U1 = 3.0; H.GableT = 0.5;
		H.ColU = {0.5, 3.0};
		H.Floor = CH::EarPlinth; H.PlatformFront = -0.6; H.PlatformBack = 5.0; H.PlatformU0 = 0.0; H.PlatformU1 = 3.0;
		H.ColH = CH::EarCol; H.ColD = 0.26;
		H.PurlinV = {0.0, 1.1875, 2.375, 3.5625, 4.75};
		H.Rise = {0.5, 0.72, -0.72, -0.5};
		H.Tiles = ChenghuaiRoof::ETiles::He; H.Ridge = ERidge::Qingshui;
		H.bBackWall = true; H.BackWallV0 = 4.5; H.BackWallV1 = 5.0; H.bSealedBack = true;
		H.EaveFront = 0.8; H.EaveBack = 0.25;
		H.Bays = {EBay::Doors};
		H.bPainted = false; H.bRafterEnds = false;
		H.WallPart = CB::BrickHairline; H.BaseCourse = CB::BrickFine; H.FloorPart = CB::FloorLarge;
		H.bEndOpen = true; H.RoofU1 = 3.05;                         // its east side is the main hall's gable
		return H;
	}

	FHallSpec RearRow()
	{
		FHallSpec H;
		H.Name = TEXT("Rear row");
		H.L = Frame(CH::SiteW, CH::RearRowFace, 1, 0, 0, -1);      // faces south
		H.U0 = 0.0; H.U1 = 23.0; H.GableT = 0.5;
		H.ColU = Even(0.5, 22.0 / 7.0, 8);
		H.Floor = CH::RearRowPlinth; H.PlatformFront = -0.6; H.PlatformBack = 5.2; H.PlatformU0 = 0.0; H.PlatformU1 = 23.0;
		H.ColH = CH::RearRowCol; H.ColD = 0.26;
		H.PurlinV = {0.0, 1.2375, 2.475, 3.7125, 4.95};
		H.Rise = {0.5, 0.72, -0.72, -0.5};
		H.Tiles = ChenghuaiRoof::ETiles::He; H.Ridge = ERidge::Qingshui;
		H.bBackWall = true; H.BackWallV0 = 4.7; H.BackWallV1 = 5.2; H.bSealedBack = true;
		H.EaveFront = 0.75; H.EaveBack = 0.28;
		H.Bays = {EBay::DoorsOpen, EBay::Windows, EBay::Windows, EBay::Doors, EBay::Windows, EBay::Windows, EBay::DoorsOpen};
		H.bPainted = true; H.PaintRow = 0; H.bRafterEnds = false;   // restrained painting, as the front row

		H.WallPart = CB::BrickPointed; H.BaseCourse = CB::BrickFine; H.FloorPart = CB::FloorSmall;
		const double B = 22.0 / 7.0;
		H.Steps = {{0.5 + 0.5 * B, 1.5}, {0.5 + 3.5 * B, 1.5}, {0.5 + 6.5 * B, 1.5}};
		return H;
	}

	/** A gallery's common section: 1.3 m between its column lines, a rolled roof, painted. */
	FHallSpec Gallery(const TCHAR* Name, const FLocal& L)
	{
		FHallSpec H;
		H.Name = Name;
		H.L = L;
		H.GableT = 0.0; H.bGableWalls = false;
		H.Floor = CH::GalleryFloor; H.ColH = CH::GalleryCol; H.ColD = 0.22;
		H.PurlinV = {0.0, 0.45, 0.85, 1.3};
		H.Rise = {0.55, 0.0, -0.55};
		H.bRolled = true; H.Tiles = ChenghuaiRoof::ETiles::He; H.Ridge = ERidge::Rolled;
		H.bBackWall = false; H.bSealedBack = false; H.bBackColumns = true;
		H.EaveFront = 0.6; H.EaveBack = 0.6;
		H.bPainted = true; H.PaintRow = 3;
		H.FloorPart = CB::FloorSmall;
		H.PlatformFront = -0.1; H.PlatformBack = 1.4;
		return H;
	}

	/** The south gallery (west half): from the corner at x −16.3 along the court wall to the festooned gate. */
	FHallSpec GalleryG1()
	{
		FHallSpec H = Gallery(TEXT("Gallery south"), Frame(CH::GalleryWestX0, CH::GalleryColY, 1, 0, 0, 1));
		H.U0 = 0.0; H.U1 = 5.4;
		H.ColU = {0.0, 1.3, 3.4, 5.2};
		H.Bays = {EBay::Open, EBay::Open, EBay::Open};
		H.PlatformU0 = -0.1; H.PlatformU1 = 5.4;
		H.EaveBack = 0.12;                                          // into the court wall
		H.MitreAtU0 = -1; H.MitreRefV = 0.0; H.RoofU0 = 1.3; H.RoofU1 = 5.45;
		return H;
	}

	/** The short west gallery from the corner north to the side hall's gable. */
	FHallSpec GalleryG2()
	{
		FHallSpec H = Gallery(TEXT("Gallery west south"), Frame(-15.0, -28.6, 0, -1, -1, 0));
		H.U0 = 1.3; H.U1 = 2.45;
		H.ColU = {1.3, 2.3};
		H.Bays = {EBay::Open};
		H.PlatformU0 = 1.4; H.PlatformU1 = 2.4;
		H.bColumns = false; H.FrameU = {2.3};
		H.MitreAtU0 = -1; H.MitreRefV = 0.0; H.RoofU0 = 1.3; H.RoofU1 = 2.45;
		return H;
	}

	/** The west gallery from the side hall's north gable to the corner by the main hall (its floor steps up). */
	FHallSpec GalleryG3()
	{
		FHallSpec H = Gallery(TEXT("Gallery west north"), Frame(-15.0, -41.4, 0, -1, -1, 0));
		H.U0 = -0.05; H.U1 = 3.8;
		H.ColU = {0.1, 2.4};
		H.Bays = {EBay::Open};
		H.bPlatform = false; H.bColumns = false; H.FrameU = {0.1, 2.4};
		H.MitreAtU1 = 1; H.MitreRefV = 0.0; H.RoofU0 = -0.05; H.RoofU1 = 2.5;
		return H;
	}

	/** Its short turn east into the main hall's veranda (through the gable). */
	FHallSpec GalleryG4()
	{
		FHallSpec H = Gallery(TEXT("Gallery to the hall"), Frame(-15.0, -43.9, 1, 0, 0, -1));
		H.U0 = 0.0; H.U1 = 0.35;
		H.ColU = {0.0, 0.35};
		H.Bays = {EBay::Open};
		H.bPlatform = false; H.bColumns = false; H.bFrames = false;
		H.MitreAtU0 = -1; H.MitreRefV = 0.0; H.RoofU0 = 0.0; H.RoofU1 = 0.35;
		return H;
	}

	// ------------------------------------------------------------------------------------------------ the gate

	void GateJoinery(FChMeshes& Out)
	{
		const FHallSpec H = GateHall();
		const FLocal& L = H.L;
		const double F = H.Floor;
		const FSection S = MakeSection(H);
		// The four columns (at the walls, 0.3 in from them), and the two on the door's line that frame it.
		for (const double U : {0.8, 3.8})
		{
			for (const double V : {0.0, 4.9})
			{
				const FVector At = L.P(U, V, F);
				Kit::Box(Out[CB::StoneKerb], TEXT("Column base"), At + FVector(-0.28, -0.28, -0.08), At + FVector(0.28, 0.28, 0.012));
				Kit::Rod(Out[CB::RedLacquer], TEXT("Gate column"), At + FVector(0, 0, 0.01), L.P(U, V, S.Zc), 0.15, 20, 0.145);
			}
			// The door's posts (中柱) on the ridge line: the frame stands between them.
			Kit::Rod(Out[CB::RedLacquer], TEXT("Gate middle column"), L.P(U, 2.45, F + 0.01), L.P(U, 2.45, S.Seat[2] - 0.3), 0.16, 20, 0.155);
		}
		// Beams across at the columns (front and back halves), under the purlins.
		for (const double U : {0.8, 3.8})
		{
			Kit::Member(Out[CB::RedLacquer], TEXT("Gate beam"), L.P(U, -0.2, S.Seat[0] - 0.28), L.P(U, 2.45, S.Seat[0] - 0.28), L.U3(), 0.26, 0.34, 0.03);
			Kit::Member(Out[CB::RedLacquer], TEXT("Gate beam"), L.P(U, 2.45, S.Seat[4] - 0.28), L.P(U, 5.1, S.Seat[4] - 0.28), L.U3(), 0.26, 0.34, 0.03);
			Kit::Box(Out[CB::RedLacquer], TEXT("Strut"), L.P(U, 1.225, S.Seat[0] - 0.11) - FVector(0.09, 0.09, 0.0), L.P(U, 1.225, S.Seat[1] - 0.2) + FVector(0.09, 0.09, 0.0));
			Kit::Box(Out[CB::RedLacquer], TEXT("Strut"), L.P(U, 3.675, S.Seat[4] - 0.11) - FVector(0.09, 0.09, 0.0), L.P(U, 3.675, S.Seat[3] - 0.2) + FVector(0.09, 0.09, 0.0));
		}
		// The door frame on the ridge line (广亮大门): posts, the sill (门槛), the middle and top rails, the panel over the
		// door (走马板), the side panels (余塞板) between the frame and the columns; four door pins (门簪) on the rail.
		const double V = 2.45, DoorW = 1.9, Head = F + 2.75, Top = S.Seat[2] - 0.35;
		const double UA = 2.3 - 0.5 * DoorW, UB = 2.3 + 0.5 * DoorW;
		Kit::LBox(Out[CB::RedLacquer], TEXT("Door post"), L, UA - 0.12, UA, V - 0.09, V + 0.09, F, Head);
		Kit::LBox(Out[CB::RedLacquer], TEXT("Door post"), L, UB, UB + 0.12, V - 0.09, V + 0.09, F, Head);
		Kit::LBox(Out[CB::RedLacquer], TEXT("Sill"), L, UA - 0.12, UB + 0.12, V - 0.1, V + 0.1, F, F + 0.25);
		Kit::LBox(Out[CB::RedLacquer], TEXT("Middle rail"), L, 0.95, 3.65, V - 0.09, V + 0.09, Head, Head + 0.2);
		Kit::LBox(Out[CB::RedLacquer], TEXT("Top rail"), L, 0.95, 3.65, V - 0.09, V + 0.09, Top - 0.15, Top);
		Kit::LBox(Out[CB::RedLacquer], TEXT("Panel over the door"), L, 0.95, 3.65, V - 0.03, V + 0.03, Head + 0.2, Top - 0.15);
		Kit::LBox(Out[CB::RedLacquer], TEXT("Side panel"), L, 0.95, UA - 0.12, V - 0.03, V + 0.03, F + 0.25, Head);
		Kit::LBox(Out[CB::RedLacquer], TEXT("Side panel"), L, UB + 0.12, 3.65, V - 0.03, V + 0.03, F + 0.25, Head);
		for (int32 k = 0; k < 4; ++k)
		{
			const double U = UA + DoorW * (0.2 + 0.2 * k);
			FProfile Pin;
			Pin.Add(0.0, 0.0).Add(0.075, 0.0).Add(0.075, 0.1).Add(0.06, 0.12, true).Add(0.0, 0.125);
			Kit::Lathe(Out[CB::GreenLacquer], TEXT("Door pin"), L.P(U, V - 0.09, Head + 0.1), -L.V3(), Pin, 16, 0.07);
			ChenghuaiPaint::AtlasDisc(Out[CB::Plaques], L.P(U, V - 0.216, Head + 0.1), -L.V3(), FVector::UpVector, 0.06,
									  ChenghuaiPlaques::Rect(ChenghuaiPlaques::DiscYi - k));
		}
		// The leaves: black lacquer, swung open inwards (north) against the side panels, brass knockers (门钹) on them.
		const double LW = 0.5 * DoorW;
		for (const int32 Side : {0, 1})
		{
			const double Hinge = Side == 0 ? UA + 0.01 : UB - 0.01;
			const double Dir = Side == 0 ? 1.0 : -1.0;
			Kit::LBox(Out[CB::BlackLacquer], TEXT("Gate leaf"), L, Hinge - (Side == 0 ? 0.0 : 0.08), Hinge + (Side == 0 ? 0.08 : 0.0), V + 0.12, V + 0.12 + LW, F + 0.26, Head - 0.02);
			// Its planks' battens and the knocker plate, on the face towards the door's middle (now facing across).
			const double Face = Hinge + Dir * 0.08;
			const FVector C = L.P(Face, V + 0.12 + 0.75 * LW, F + 1.35);
			FProfile Plate;
			Plate.Add(0.0, 0.0).Add(0.11, 0.0).Add(0.11, 0.006).Add(0.05, 0.03, true).Add(0.0, 0.035);
			Kit::Lathe(Out[CB::Brass], TEXT("Knocker plate"), C, L.U3() * Dir, Plate, 8, 0.1);
			FProfile Ring;
			for (int32 k = 0; k < 12; ++k)
			{
				const double T = 2.0 * UE_DOUBLE_PI * k / 12;
				Ring.Add(0.07 + 0.009 * FMath::Cos(T), 0.03 + 0.009 * FMath::Sin(T), true);
			}
			Ring.bClosed = true;
			Kit::Lathe(Out[CB::Brass], TEXT("Knocker ring"), C - FVector(0, 0, 0.07), FVector::UpVector, Ring, 20, 0.07);
		}
		// The box-type stone drums (门墩) either side of the sill, out towards the steps.
		for (const double U : {UA - 0.3, UB + 0.3})
		{
			Kit::LBox(Out[CB::StoneKerb], TEXT("Drum base"), L, U - 0.17, U + 0.17, V - 0.75, V + 0.35, F - 0.05, F + 0.28);
			Kit::LBox(Out[CB::StoneKerb], TEXT("Drum box"), L, U - 0.15, U + 0.15, V - 0.72, V - 0.12, F + 0.28, F + 0.7);
			Kit::LBox(Out[CB::StoneKerb], TEXT("Drum lion"), L, U - 0.1, U + 0.1, V - 0.62, V - 0.22, F + 0.7, F + 0.86);
		}
	}

	// ------------------------------------------------------------------------------------------------ the festooned gate

	void Chuihuamen(FChMeshes& Out)
	{
		const double F = CH::ChuihuaPlinth;
		// The platform, its kerb, the steps south (three risers), the floor.
		Kit::Box(Out[CB::BrickFine], TEXT("Chuihua platform"), FVector(CH::ChuihuaX0 + 0.01, CH::ChuihuaY0 + 0.01, -0.1), FVector(CH::ChuihuaX1 - 0.01, CH::ChuihuaY1 - 0.01, F - 0.12));
		CP::KerbRect(Out, CH::ChuihuaX0, CH::ChuihuaY0, CH::ChuihuaX1, CH::ChuihuaY1, 0.3, F - 0.12, F, CB::StoneKerb);
		Kit::Box(Out[CB::FloorSmall], TEXT("Chuihua floor"), FVector(CH::ChuihuaX0 + 0.3, CH::ChuihuaY0 + 0.3, F - 0.11), FVector(CH::ChuihuaX1 - 0.3, CH::ChuihuaY1 - 0.3, F - 0.002));
		for (int32 k = 1; k < 3; ++k)
		{
			Kit::Box(Out[CB::StoneKerb], TEXT("Step"), FVector(CH::Axis - 1.0, CH::ChuihuaY1 - 0.02 + 0.3 * (k - 1), -0.08), FVector(CH::Axis + 1.0, CH::ChuihuaY1 + 0.3 * k, F - 0.15 * k));
		}
		// Two roofs, one behind the other (一殿一卷): the front one with a ridge over the hanging posts' line, the back one
		// rolled; a gutter between them over the main columns.
		auto Roof = [](const TCHAR* Name, double V0Y, bool bFront)
		{
			FHallSpec H;
			H.Name = Name;
			H.L = Frame(CH::ChuihuaX0 - 0.45, V0Y, 1, 0, 0, -1);
			H.U0 = 0.0; H.U1 = 4.3;
			H.GableT = 0.0; H.bGableWalls = false;
			H.ColU = {0.75, 3.55};
			H.Floor = CH::ChuihuaPlinth; H.ColH = 2.62; H.ColD = 0.24;
			if (bFront)
			{
				H.PurlinV = {0.0, 0.55, 1.1};
				H.Rise = {0.8, -0.8};
				H.Ridge = ERidge::Qingshui; H.Tiles = ChenghuaiRoof::ETiles::Tong;
				H.EaveFront = 0.75; H.EaveBack = 0.04;
			}
			else
			{
				H.PurlinV = {0.0, 0.42, 0.72, 1.1};
				H.Rise = {0.65, 0.0, -0.9};
				H.bRolled = true; H.Ridge = ERidge::Rolled; H.Tiles = ChenghuaiRoof::ETiles::Tong;
				H.EaveFront = 0.04; H.EaveBack = 0.7;
			}
			H.bBackWall = false; H.bSealedBack = false;
			H.Bays = {EBay::Open};
			H.bPainted = true; H.PaintRow = 2;
			H.bPlatform = false; H.bColumns = false; H.bFrames = false;
			H.RoofU0 = 0.0; H.RoofU1 = 4.3;
			return H;
		};
		FHallSpec Front = Roof(TEXT("Chuihua front roof"), CH::ChuihuaPostY, true);
		FHallSpec Back = Roof(TEXT("Chuihua back roof"), CH::ChuihuaFrontY, false);
		Back.SkipPurlins = {0};                                     // the valley's purlin is the front roof's
		// The back roof's seats at the valley meet the front roof's: its columns there are the main ones (taller).
		BuildHall(Out, Front);
		BuildHall(Out, Back);
		const FSection SF = MakeSection(Front), SB = MakeSection(Back);
		// The main columns (on the wall's line) and the back columns (the screen's), red on stone bases.
		for (const double X : {CH::ChuihuaColX0, CH::ChuihuaColX1})
		{
			for (const double Y : {CH::ChuihuaFrontY, CH::ChuihuaBackY})
			{
				const double Top = Y == CH::ChuihuaFrontY ? SF.Seat[2] - 0.3 : SB.Seat[3] - 0.3;
				Kit::Box(Out[CB::StoneKerb], TEXT("Column base"), FVector(X - 0.22, Y - 0.22, F - 0.08), FVector(X + 0.22, Y + 0.22, F + 0.012));
				Kit::Rod(Out[CB::RedLacquer], TEXT("Chuihua column"), FVector(X, Y, F + 0.01), FVector(X, Y, Top), 0.125, 20, 0.12);
			}
			// The long beam (麻叶抱头梁) from the back column through the main one out over the hanging post, green with
			// a carved end; the hanging post (垂柱) hangs from it, ending in a gilt lotus bud (垂头).
			const double ZB = SF.Seat[0] - 0.3;
			Kit::Member(Out[CB::GreenLacquer], TEXT("Cantilever beam"), FVector(X, CH::ChuihuaBackY - 0.1, ZB), FVector(X, CH::ChuihuaPostY + 0.25, ZB), FVector(1, 0, 0), 0.2, 0.3, 0.03);
			Kit::Rod(Out[CB::RedLacquer], TEXT("Hanging post"), FVector(X, CH::ChuihuaPostY, ZB + 0.15), FVector(X, CH::ChuihuaPostY, F + 2.25), 0.1, 16);
			FProfile Bud;
			Bud.Add(0.0, 0.0).Add(0.05, 0.02, true).Add(0.11, 0.1, true).Add(0.13, 0.19, true).Add(0.12, 0.26, true).Add(0.1, 0.3).Add(0.0, 0.3);
			Kit::Lathe(Out[CB::Brass], TEXT("Lotus bud"), FVector(X, CH::ChuihuaPostY, F + 1.95), FVector::UpVector, Bud, 20, 0.1);
		}
		// Between the hanging posts, under the lintel: a band of lattice (倒挂楣子); between the main columns the frame of the
		// door opening (its leaves are taken down by day).
		const double LintelBottom = SF.Zc - (0.9 * 0.24 + 0.04);
		const FLocal Posts{FVector2D(CH::ChuihuaColX0, CH::ChuihuaPostY), FVector2D(1, 0), FVector2D(0, -1)};
		ChenghuaiHall::LatticePanel(Out, Posts, 0.1, -0.02, LintelBottom - 0.3, CH::ChuihuaColX1 - CH::ChuihuaColX0 - 0.2, 0.3,
									ChenghuaiHall::ELattice::StepBrocade, CB::RedLacquer, false);
		const double DH = LintelBottom - 0.14;
		Kit::Box(Out[CB::RedLacquer], TEXT("Door frame"), FVector(CH::ChuihuaColX0 + 0.12, CH::ChuihuaFrontY - 0.07, F), FVector(CH::ChuihuaColX0 + 0.24, CH::ChuihuaFrontY + 0.07, DH));
		Kit::Box(Out[CB::RedLacquer], TEXT("Door frame"), FVector(CH::ChuihuaColX1 - 0.24, CH::ChuihuaFrontY - 0.07, F), FVector(CH::ChuihuaColX1 - 0.12, CH::ChuihuaFrontY + 0.07, DH));
		Kit::Box(Out[CB::RedLacquer], TEXT("Door head"), FVector(CH::ChuihuaColX0 + 0.12, CH::ChuihuaFrontY - 0.07, DH), FVector(CH::ChuihuaColX1 - 0.12, CH::ChuihuaFrontY + 0.07, LintelBottom + 0.002));
		Kit::Box(Out[CB::RedLacquer], TEXT("Door sill"), FVector(CH::ChuihuaColX0 + 0.12, CH::ChuihuaFrontY - 0.08, F), FVector(CH::ChuihuaColX1 - 0.12, CH::ChuihuaFrontY + 0.08, F + 0.12));
		// The green screen door (屏门) on the back columns' line, shut: four panels, a red disc with a character on each.
		for (int32 k = 0; k < 4; ++k)
		{
			const double X0 = CH::ChuihuaColX0 + 0.14 + 0.63 * k, X1 = X0 + 0.61;
			Kit::Box(Out[CB::GreenLacquer], TEXT("Screen panel"), FVector(X0, CH::ChuihuaScreenY - 0.03, F + 0.13), FVector(X1, CH::ChuihuaScreenY + 0.03, DH - 0.02));
			ChenghuaiPaint::AtlasDisc(Out[CB::Plaques], FVector(0.5 * (X0 + X1), CH::ChuihuaScreenY + 0.032, F + 1.75), FVector(0, 1, 0), FVector::UpVector, 0.16,
									  ChenghuaiPlaques::Rect(ChenghuaiPlaques::DiscZhai - k));
		}
		Kit::Box(Out[CB::RedLacquer], TEXT("Screen head"), FVector(CH::ChuihuaColX0 + 0.12, CH::ChuihuaScreenY - 0.06, DH - 0.02), FVector(CH::ChuihuaColX1 - 0.12, CH::ChuihuaScreenY + 0.06, DH + 0.12));
		Kit::Box(Out[CB::RedLacquer], TEXT("Screen board"), FVector(CH::ChuihuaColX0 + 0.12, CH::ChuihuaScreenY - 0.025, DH + 0.12), FVector(CH::ChuihuaColX1 - 0.12, CH::ChuihuaScreenY + 0.025, LintelBottom + 0.002));
		Kit::Box(Out[CB::RedLacquer], TEXT("Screen sill"), FVector(CH::ChuihuaColX0 + 0.12, CH::ChuihuaScreenY - 0.06, F), FVector(CH::ChuihuaColX1 - 0.12, CH::ChuihuaScreenY + 0.06, F + 0.13));
	}

	// ------------------------------------------------------------------------------------------------ the screen wall

	void ScreenWall(FChMeshes& Out)
	{
		// Against the court wall's south face: a stone plinth (须弥座), the rubbed-brick body, the carved square heart
		// (影壁心) framed by moulded bricks, a corbelled cornice and a little tiled roof with a ridge.
		const double X0 = -CH::ScreenHalf, X1 = CH::ScreenHalf, Y0 = CH::ScreenY1, Y1 = CH::ScreenY0;   // y −27.55 (face) … −28.1
		Kit::Box(Out[CB::StoneKerb], TEXT("Screen plinth"), FVector(X0 - 0.08, Y1, -0.05), FVector(X1 + 0.08, Y0 + 0.08, 0.18));
		Kit::Box(Out[CB::StoneKerb], TEXT("Screen plinth waist"), FVector(X0 - 0.02, Y1, 0.18), FVector(X1 + 0.02, Y0 + 0.03, 0.42));
		Kit::Box(Out[CB::StoneKerb], TEXT("Screen plinth top"), FVector(X0 - 0.07, Y1, 0.42), FVector(X1 + 0.07, Y0 + 0.07, 0.55));
		Kit::Box(Out[CB::BrickFine], TEXT("Screen body"), FVector(X0, Y1, 0.55), FVector(X1, Y0, 3.1));
		// The heart: 1.8 m square, its face 2 cm back in a moulded frame; the carving is the plaques' atlas (cell 20).
		const double HC = 1.8, CZ = 0.55 + 0.25 + 0.5 * HC + 0.1;
		Kit::Box(Out[CB::BrickFine], TEXT("Heart frame"), FVector(-0.5 * HC - 0.1, Y0 - 0.001, CZ - 0.5 * HC - 0.1), FVector(0.5 * HC + 0.1, Y0 + 0.035, CZ + 0.5 * HC + 0.1));
		ChenghuaiPaint::AtlasQuad(Out[CB::Plaques], FVector(0.0, Y0 + 0.036, CZ), FVector(0, 1, 0), FVector::UpVector, 0.5 * HC, 0.5 * HC,
								  ChenghuaiPlaques::Rect(ChenghuaiPlaques::ScreenHeart));
		// Cornice and roof.
		for (int32 k = 0; k < 4; ++k)
		{
			Kit::Box(Out[CB::BrickFine], TEXT("Screen cornice"), FVector(X0 - 0.04 * (k + 1), Y1, 3.1 + 0.07 * k), FVector(X1 + 0.04 * (k + 1), Y0 + 0.04 * (k + 1), 3.17 + 0.07 * k));
		}
		const FLocal L = Frame(X0 - 0.2, 0.5 * (Y0 + Y1), 1, 0, 0, 1);
		CP::Coping(Out, L, 0.0, X1 - X0 + 0.4, 0.0, (Y1 - Y0) + 0.5, 3.38);
	}

	// ------------------------------------------------------------------------------------------------ walls

	void Walls(FChMeshes& Out)
	{
		using CP::FHole;
		using CP::EHole;
		// The house's outer walls where no building stands (4.2 m, grey lime-pointed brick; white on the garden's face).
		CP::FWallSpec Outer;
		Outer.BodyPart = CB::BrickPointed;
		Outer.Height = 4.2;
		CP::FWallSpec East = Outer;
		East.BackFace = CB::PlasterWhite;   // +v: the garden (the frame below runs v east)
		// West wall (x −20.7 … −20.2): along u = −y.
		{
			const FLocal L = Frame(CH::SiteW, 0.0, 0, -1, 1, 0);
			CP::Wall(Out, L, 22.6, 31.0, 0.0, 0.5, Outer, {});         // the front court's west end to the side hall
			CP::Wall(Out, L, 41.4, 56.35, 0.0, 0.5, Outer, {});        // by the ear room and the rear court
		}
		// East wall (x 1.8 … 2.3): the entry yard (the garden's way back in), the inner court's corner (a lattice window),
		// the east passage and the rear court (three windows, the moon gate).
		{
			const FLocal L = Frame(CH::HouseInE, 0.0, 0, -1, 1, 0);
			CP::Wall(Out, L, 22.5, 31.0, 0.0, 0.5, East,
					 {{EHole::DoorPlain, -CH::YardDoorY1, -CH::YardDoorY0, 0.0, 2.35},
					  {EHole::Window, 29.2, 30.2, 1.25, 2.25, 0}});
			CP::Wall(Out, L, 41.4, 56.35, 0.0, 0.5, East,
					 {{EHole::Window, 43.4, 44.4, 1.25, 2.25, 2}, {EHole::Window, 46.4, 47.4, 1.25, 2.25, 1},
					  {EHole::Window, 49.4, 50.4, 1.25, 2.25, 4},
					  {EHole::MoonGate, -CH::MoonGateY1, -CH::MoonGateY0, 0.0, 2.6}});
		}
		// The court wall between the front and inner courts: white over a grey base, a fan window either side of the gate.
		CP::FWallSpec Court;
		Court.BodyPart = CB::PlasterWhite;
		Court.Height = CH::DivideTop;
		Court.BaseH = CH::DivideBase;
		{
			const FLocal L = Frame(0.0, CH::DivideN, 1, 0, 0, 1);
			CP::Wall(Out, L, CH::InW, CH::ChuihuaX0, 0.0, 0.4, Court, {{EHole::Window, -13.6, -12.6, 1.5, 2.5, 4}});
			CP::Wall(Out, L, CH::ChuihuaX1, CH::YardWallX0, 0.0, 0.4, Court, {{EHole::Window, -5.8, -4.8, 1.5, 2.5, 2}});
			CP::FWallSpec Behind = Court;
			Behind.BodyPart = CB::BrickFine;
			CP::Wall(Out, L, CH::YardWallX0, CH::HouseInE, 0.0, 0.4, Behind, {});   // behind the screen wall
		}
		// The entry yard's west wall, the green screen door into the front court.
		{
			CP::FWallSpec Yard = Court;
			Yard.BodyPart = CB::BrickFine;
			const FLocal L = Frame(CH::YardWallX0, 0.0, 0, -1, 1, 0);
			CP::Wall(Out, L, -CH::GateN, -CH::DivideS, 0.0, 0.4, Yard, {{EHole::Door, -CH::YardDoorY1, -CH::YardDoorY0, 0.0, 2.35}});
		}
		// The cross walls into the rear court (a door in each), from the house's side walls to the ear rooms.
		{
			CP::FWallSpec Cross = Outer;
			Cross.Height = 3.2;
			const FLocal L = Frame(0.0, CH::CrossWallY0, 1, 0, 0, 1);
			CP::Wall(Out, L, CH::InW, -17.7, 0.0, 0.4, Cross, {{EHole::Door, CH::CornerDoorW[0], CH::CornerDoorW[1], 0.0, 2.3}});
			CP::Wall(Out, L, -0.7, CH::HouseInE, 0.0, 0.4, Cross, {{EHole::Door, CH::CornerDoorE[0], CH::CornerDoorE[1], 0.0, 2.3}});
		}
		// The porch: low grey brick walls with a stone coping from the Rotunda's drum to the gate's steps.
		for (const double Side : {-1.0, 1.0})
		{
			const double XA = Side * CH::PorchHalf, XB = Side * CH::PorchWallOut;
			const double YD = -FMath::Sqrt(FMath::Square(CH::DrumOuter) - XB * XB) + 0.08;
			Kit::Box(Out[CB::BrickFine], TEXT("Porch wall"), FVector(FMath::Min(XA, XB), -16.96, -0.1), FVector(FMath::Max(XA, XB), YD, 0.98));
			Kit::Box(Out[CB::StoneKerb], TEXT("Porch coping"), FVector(FMath::Min(XA, XB) - 0.04, -16.96, 0.98), FVector(FMath::Max(XA, XB) + 0.04, YD, 1.1));
		}
	}

	// ------------------------------------------------------------------------------------------------ courts and paving

	void Courts(FChMeshes& Out)
	{
		const double Z = 0.02;
		// The porch: city bricks on edge from the drum to the gate's steps.
		{
			TArray<FVector2D> P;
			const double R = CH::DrumOuter + 0.002;
			for (int32 k = 0; k <= 12; ++k)
			{
				const double X = -CH::PorchHalf + 2.0 * CH::PorchHalf * k / 12;
				P.Add(FVector2D(X, -FMath::Sqrt(R * R - X * X)));
			}
			P.Add(FVector2D(CH::PorchHalf, -17.02));
			P.Add(FVector2D(-CH::PorchHalf, -17.02));
			Algo::Reverse(P);
			CP::Paving(Out, CB::CourtPath, P, -0.002);
		}
		// The entry yard.
		CP::Paving(Out, CB::CourtPaving, CP::Rect(CH::YardWallX1, CH::YardN, CH::HouseInE, CH::GateN), Z);
		// The front court (between the front row's platform and the court wall), its paths of city bricks.
		CP::Paving(Out, CB::CourtPaving, CP::Rect(CH::InW, CH::DivideS, CH::YardWallX0, CH::FrontRowPlatformN), Z);
		Kit::Box(Out[CB::CourtPath], TEXT("Path"), FVector(-19.6, -26.3, Z - 0.02), FVector(CH::YardWallX0, -24.5, Z + 0.006));
		Kit::Box(Out[CB::CourtPath], TEXT("Path"), FVector(CH::Axis - 0.9, CH::DivideS - 0.01, Z - 0.02), FVector(CH::Axis + 0.9, -24.5, Z + 0.005));
		// The inner court and its corners (the platforms and galleries stand on it), a cross of city bricks, four beds.
		CP::Paving(Out, CB::CourtPaving, CP::Rect(CH::InW, CH::CrossWallY1, CH::HouseInE, CH::DivideN), Z);
		Kit::Box(Out[CB::CourtPath], TEXT("Path"), FVector(CH::Axis - 0.9, -42.3, Z - 0.02), FVector(CH::Axis + 0.9, -29.6, Z + 0.006));
		Kit::Box(Out[CB::CourtPath], TEXT("Path"), FVector(-14.6, -37.1, Z - 0.02), FVector(-3.8, -35.3, Z + 0.005));
		const double Beds[4][4] = {{-13.8, -41.7, -10.6, -37.5}, {-7.8, -41.7, -4.6, -37.5}, {-13.8, -34.9, -10.6, -30.7}, {-7.8, -34.9, -4.6, -30.7}};
		for (const auto& B : Beds)
		{
			CP::KerbRect(Out, B[0], B[1], B[2], B[3], 0.14, -0.05, Z + 0.12, CB::StoneKerb);
			Kit::Box(Out[CB::GardenGround], TEXT("Bed"), FVector(B[0] + 0.14, B[1] + 0.14, -0.1), FVector(B[2] - 0.14, B[3] - 0.14, Z + 0.06));
		}
		// The rear court: paving, its path, the two jujube trees' pits beside it.
		CP::Paving(Out, CB::CourtPaving, CP::Rect(CH::InW, CH::RearRowPlatformS, CH::HouseInE, CH::CrossWallY0), Z);
		Kit::Box(Out[CB::CourtPath], TEXT("Path"), FVector(CH::InW, -55.0, Z - 0.02), FVector(CH::HouseInE, -53.7, Z + 0.006));
		for (const double X : {-16.8, -1.6})
		{
			CP::KerbRect(Out, X - 0.7, -53.0, X + 0.7, -51.6, 0.12, -0.05, Z + 0.1, CB::StoneKerb);
			Kit::Box(Out[CB::GardenGround], TEXT("Tree pit"), FVector(X - 0.58, -52.88, -0.1), FVector(X + 0.58, -51.72, Z + 0.03));
		}
	}

	// ------------------------------------------------------------------------------------------------ galleries' extras

	void GalleryFloors(FChMeshes& Out)
	{
		// The west gallery north of the side hall: level at 0.45 to y −43.0, two risers up to the main hall's veranda
		// level (0.75) by y −43.6, then level round the corner and east into the hall's gable; mirrored east.
		for (const double Side : {-1.0, 1.0})
		{
			auto X = [Side](double XW) { return Side < 0 ? XW : 2.0 * CH::Axis - XW; };
			auto Box = [&](int32 Part, double XA, double YA, double XB, double YB, double Z0, double Z1, const TCHAR* Name)
			{
				Kit::Box(Out[Part], Name, FVector(FMath::Min(X(XA), X(XB)), YA, Z0), FVector(FMath::Max(X(XA), X(XB)), YB, Z1));
			};
			Box(CB::BrickFine, -16.4, -43.0, -14.9, -41.35, -0.1, 0.33, TEXT("Gallery platform"));
			Box(CB::StoneKerb, -14.95, -43.0, -14.85, -41.35, -0.05, 0.45, TEXT("Gallery kerb"));
			Box(CB::FloorSmall, -16.4, -43.0, -14.95, -41.35, 0.33, 0.448, TEXT("Gallery floor"));
			Box(CB::StoneKerb, -16.4, -43.3, -14.85, -43.0, -0.05, 0.6, TEXT("Gallery step"));
			Box(CB::BrickFine, -16.4, -45.35, -14.7, -43.3, -0.1, 0.63, TEXT("Gallery platform"));
			Box(CB::StoneKerb, -14.95, -43.9, -14.7, -43.3, -0.05, 0.75, TEXT("Gallery kerb"));
			Box(CB::StoneKerb, -16.4, -43.35, -14.95, -43.3, -0.05, 0.75, TEXT("Gallery kerb"));
			Box(CB::FloorSmall, -16.4, -45.35, -14.95, -43.35, 0.63, 0.748, TEXT("Gallery floor"));
			Box(CB::FloorSmall, -14.95, -45.35, -14.7, -43.9, 0.63, 0.748, TEXT("Gallery floor"));
			// Steps off the gallery east into the corner court (the way on to the rear court), three risers.
			for (int32 k = 1; k < 3; ++k)
			{
				Box(CB::StoneKerb, -16.4 - 0.3 * k, -43.0, -16.4 - 0.3 * (k - 1) + 0.02, -41.8, -0.08, 0.45 - 0.15 * k, TEXT("Step"));
			}
			// Columns: the court side at x −15.0 (y −41.5, −43.8), the back line at x −16.3, the corner's outer column.
			for (const FVector2D& C : {FVector2D(-15.0, -41.5), FVector2D(-15.0, -43.8), FVector2D(-16.3, -41.5), FVector2D(-16.3, -43.8), FVector2D(-16.3, -45.2)})
			{
				const double F = C.Y < -43.2 ? 0.75 : 0.45;
				const double Zc = CH::GalleryFloor + CH::GalleryCol;
				const FVector At(X(C.X), C.Y, F);
				Kit::Box(Out[CB::StoneKerb], TEXT("Column base"), At + FVector(-0.2, -0.2, -0.08), At + FVector(0.2, 0.2, 0.012));
				Kit::Rod(Out[CB::RedLacquer], TEXT("Gallery column"), At + FVector(0, 0, 0.01), FVector(At.X, At.Y, Zc), 0.11, 16, 0.105);
			}
			// The short west gallery's two columns (y −30.9).
			for (const double XW : {-15.0, -16.3})
			{
				const FVector At(X(XW), -30.9, CH::GalleryFloor);
				Kit::Box(Out[CB::StoneKerb], TEXT("Column base"), At + FVector(-0.2, -0.2, -0.08), At + FVector(0.2, 0.2, 0.012));
				Kit::Rod(Out[CB::RedLacquer], TEXT("Gallery column"), At + FVector(0, 0, 0.01), FVector(At.X, At.Y, CH::GalleryFloor + CH::GalleryCol), 0.11, 16, 0.105);
			}
		}
	}

	/** The hanging fretwork (挂落) under a gallery's lintel between two columns, on the court's side. */
	void Fretwork(FChMeshes& Out, const FVector& A, const FVector& B, double ZTop)
	{
		const FVector D = (B - A).GetSafeNormal();
		const FVector Side = FVector::CrossProduct(FVector::UpVector, D);
		const double Len = FVector::Distance(A, B);
		if (Len < 0.5) { return; }
		FLocal L;
		L.Origin = FVector2D(A.X, A.Y);
		L.U = FVector2D(D.X, D.Y);
		L.V = FVector2D(Side.X, Side.Y);
		ChenghuaiHall::LatticePanel(Out, L, 0.12, -0.012, ZTop - 0.3, Len - 0.24, 0.3, ChenghuaiHall::ELattice::StepBrocade, CB::RedLacquer, false);
	}

	void GalleryFretwork(FChMeshes& Out)
	{
		const double ZTop = CH::GalleryFloor + CH::GalleryCol - (0.9 * 0.22 + 0.04);
		for (const double Side : {-1.0, 1.0})
		{
			auto P = [Side](double XW, double Y) { return FVector(Side < 0 ? XW : 2.0 * CH::Axis - XW, Y, 0.0); };
			const double ColsX[4] = {-16.3, -15.0, -12.9, -11.1};
			for (int32 k = 1; k + 1 < 4; ++k) { Fretwork(Out, P(ColsX[k], CH::GalleryColY), P(ColsX[k + 1], CH::GalleryColY), ZTop); }
			Fretwork(Out, P(-15.0, -30.9), P(-15.0, -29.9), ZTop);
			Fretwork(Out, P(-15.0, -43.8), P(-15.0, -41.5), ZTop);
		}
	}

	// ------------------------------------------------------------------------------------------------ plaques and couplet

	/**
	 * A hall's name board (Names board) inside its room, never over a door or a gate (the user's rule): a black lacquer
	 * board 25 mm thick on the wall, its face the plaques' atlas cell (gilt characters in a gilt frame). N is the way it
	 * faces (an axis), Centre the middle of its face.
	 */
	void WallPlaque(FChMeshes& Out, const FVector& Centre, const FVector& N, double W, double H, int32 Which)
	{
		const FVector Side = FVector::CrossProduct(N, FVector::UpVector).GetSafeNormal();
		const FVector A = Centre - N * 0.025 - Side * (0.5 * W) - FVector(0, 0, 0.5 * H);   // the back, bottom left
		const FVector B = Centre - N * 0.0005 + Side * (0.5 * W) + FVector(0, 0, 0.5 * H);  // just behind the face, top right
		Kit::Box(Out[CB::BlackLacquer], TEXT("Plaque board"), A.ComponentMin(B), A.ComponentMax(B));
		ChenghuaiPaint::AtlasQuad(Out[CB::Plaques], Centre + N * 0.0005, N, FVector::UpVector, 0.5 * W, 0.5 * H, ChenghuaiPlaques::Rect(Which));
	}

	/**
	 * A couplet board curved round a column (抱柱联): a lacquer shell 22 mm thick hugging the column (axis X, Y; radius R)
	 * over Span radians centred on the facing Dir (plan, unit), Z0 … Z1; its outer face the atlas cell.
	 */
	void ColumnCouplet(FChMeshes& Out, double X, double Y, double R, const FVector2D& Dir, double Span, double Z0, double Z1, int32 Which)
	{
		constexpr int32 Segs = 10;
		const double Ri = R + 0.002, Ro = R + 0.024;
		const double Base = FMath::Atan2(Dir.Y, Dir.X);
		auto Radial = [&](double T) { return FVector(FMath::Cos(T), FMath::Sin(T), 0.0); };
		// The angle runs from the viewer's left to right (facing the board: the right is Dir turned clockwise in plan,
		// x east and y south, so the left end is at Base + Span/2).
		auto Angle = [&](int32 k) { return Base + 0.5 * Span - Span * k / Segs; };
		Kit::FPart& Wood = Out[CB::BlackLacquer];
		Wood.Begin(TEXT("Couplet board"));
		Kit::FMeshData& M = Wood.M;
		const FVector C(X, Y, 0.0);
		for (int32 k = 0; k < Segs; ++k)
		{
			const double T0 = Angle(k), T1 = Angle(k + 1);
			const FVector N0 = Radial(T0), N1 = Radial(T1);
			// Outer and inner faces.
			for (int32 f = 0; f < 2; ++f)
			{
				const double Rr = f ? Ri : Ro;
				const FVector S = f ? -1.0 * FVector::OneVector : FVector::OneVector;
				const int32 A = M.Vertex(C + N0 * Rr + FVector(0, 0, Z0), N0 * S.X, FVector2D(0, 0));
				const int32 B = M.Vertex(C + N1 * Rr + FVector(0, 0, Z0), N1 * S.X, FVector2D(0, 0));
				const int32 D = M.Vertex(C + N1 * Rr + FVector(0, 0, Z1), N1 * S.X, FVector2D(0, 0));
				const int32 E = M.Vertex(C + N0 * Rr + FVector(0, 0, Z1), N0 * S.X, FVector2D(0, 0));
				M.Quad(A, B, D, E);
			}
			// Top and bottom edges.
			for (int32 f = 0; f < 2; ++f)
			{
				const double Z = f ? Z1 : Z0;
				const FVector Nz(0, 0, f ? 1.0 : -1.0);
				const int32 A = M.Vertex(C + N0 * Ri + FVector(0, 0, Z), Nz, FVector2D(0, 0));
				const int32 B = M.Vertex(C + N1 * Ri + FVector(0, 0, Z), Nz, FVector2D(0, 0));
				const int32 D = M.Vertex(C + N1 * Ro + FVector(0, 0, Z), Nz, FVector2D(0, 0));
				const int32 E = M.Vertex(C + N0 * Ro + FVector(0, 0, Z), Nz, FVector2D(0, 0));
				M.Quad(A, B, D, E);
			}
		}
		// The two long edges.
		for (int32 e = 0; e < 2; ++e)
		{
			const double T = Angle(e ? Segs : 0);
			const FVector Rn = Radial(T);
			const FVector Tn = FVector(-FMath::Sin(T), FMath::Cos(T), 0.0) * (e ? -1.0 : 1.0);
			const int32 A = M.Vertex(C + Rn * Ri + FVector(0, 0, Z0), Tn, FVector2D(0, 0));
			const int32 B = M.Vertex(C + Rn * Ro + FVector(0, 0, Z0), Tn, FVector2D(0, 0));
			const int32 D = M.Vertex(C + Rn * Ro + FVector(0, 0, Z1), Tn, FVector2D(0, 0));
			const int32 E = M.Vertex(C + Rn * Ri + FVector(0, 0, Z1), Tn, FVector2D(0, 0));
			M.Quad(A, B, D, E);
		}
		// The face: the atlas cell, left to right as the viewer reads it, top to bottom.
		const FBox2D UV = ChenghuaiPlaques::Rect(Which);
		Kit::FPart& Face = Out[CB::Plaques];
		Face.Begin(TEXT("Couplet face"), false, TEXT("Couplet board"));
		Kit::FMeshData& F = Face.M;
		const double Rf = Ro + 0.0005;
		for (int32 k = 0; k < Segs; ++k)
		{
			const double T0 = Angle(k), T1 = Angle(k + 1);
			const double U0 = FMath::Lerp(UV.Min.X, UV.Max.X, double(k) / Segs), U1 = FMath::Lerp(UV.Min.X, UV.Max.X, double(k + 1) / Segs);
			const int32 A = F.Vertex(C + Radial(T0) * Rf + FVector(0, 0, Z0), Radial(T0), FVector2D(U0, UV.Max.Y));
			const int32 B = F.Vertex(C + Radial(T1) * Rf + FVector(0, 0, Z0), Radial(T1), FVector2D(U1, UV.Max.Y));
			const int32 D = F.Vertex(C + Radial(T1) * Rf + FVector(0, 0, Z1), Radial(T1), FVector2D(U1, UV.Min.Y));
			const int32 E = F.Vertex(C + Radial(T0) * Rf + FVector(0, 0, Z1), Radial(T0), FVector2D(U0, UV.Min.Y));
			F.Quad(A, B, D, E);
		}
	}

	void Plaques(FChMeshes& Out)
	{
		namespace P = ChenghuaiPlaques;
		const double Skin = 0.016;   // the rooms' lime plaster
		// 臨池: on the front row's east end wall (the gate's gable) over the scholar's desk, facing west (rendering 12).
		WallPlaque(Out, FVector(CH::FrontRowX1 - 0.002 - 0.025, -19.95, CH::FrontRowPlinth + 2.15), FVector(-1, 0, 0), 1.0, 0.5, P::Linchi);
		// 天青 and 昌南: on the side halls' south gable walls inside, between the wall case and the lattice, facing north.
		WallPlaque(Out, FVector(-18.1, CH::SideHallS - Skin - 0.025, CH::SideHallPlinth + 2.45), FVector(0, -1, 0), 1.1, 0.55, P::Tianqing);
		WallPlaque(Out, FVector(2.0 * CH::Axis + 18.1, CH::SideHallS - Skin - 0.025, CH::SideHallPlinth + 2.45), FVector(0, -1, 0), 1.1, 0.55, P::Changnan);
		// 清閟 and 停雲: on the main hall's gable (the ear rooms' inner wall), beside their door from the hall, not over it.
		WallPlaque(Out, FVector(-14.7 - 0.025, -47.05, CH::EarPlinth + 2.55), FVector(-1, 0, 0), 0.8, 0.4, P::Qingbi);
		WallPlaque(Out, FVector(2.0 * CH::Axis + 14.7 + 0.025, -47.05, CH::EarPlinth + 2.55), FVector(1, 0, 0), 0.8, 0.4, P::Tingyun);
		// 舒卷: on the rear row's back (north) wall over the Thousand Li's case, facing south.
		WallPlaque(Out, FVector(CH::Axis, CH::InN + Skin + 0.025, CH::RearRowPlinth + 2.12), FVector(0, 1, 0), 1.1, 0.55, P::Shujuan);
		// 林泉: on the flower hall's back wall over the painting table, facing south.
		WallPlaque(Out, FVector(0.5 * (CH::FlowerHallX0 + CH::FlowerHallX1), CH::FlowerHallN + Skin + 0.025, 0.45 + 2.5), FVector(0, 1, 0), 1.2, 0.6, P::Linquan);
		// 澄懷堂: the hall's name board, inside (the design hangs it over the door, which the user's rule keeps clear of
		// characters). A lintel (金枋) between the middle pair of the rear 金柱, painted as the hall's other inner lintels,
		// and the board on its face, over the central bay and Fan Kuan's case behind: it clears the case's top as seen
		// from the door (the sight line passes 0.1 m under it).
		{
			const double Y = CH::HallEave - 6.36;                 // the rear 金柱 line (MainHall's InnerColV)
			const double X0 = CH::HallCols[1] + 0.175, X1 = CH::HallCols[2] - 0.175;
			const double Z0 = CH::HallPlinth + 3.92, Z1 = Z0 + 0.3;
			ChenghuaiPaint::Box(Out[CB::PaintedBeam], TEXT("Hall rear lintel"), Frame(0.0, 0.0, 1, 0, 0, 1), X0, X1, Y - 0.1, Y + 0.1, Z0, Z1, 4, 0.1, 0.9);
			WallPlaque(Out, FVector(CH::Axis, Y + 0.1 + 0.026, 0.5 * (Z0 + Z1) + 0.02), FVector(0, 1, 0), 2.0, 0.64, P::Chenghuaitang);
		}
		// The couplet on the main hall's two middle veranda columns, facing the court: the first line on the right (east).
		const double Zc0 = CH::HallPlinth + 1.3, Zc1 = CH::HallPlinth + 2.7;   // 1.4 m: the cell's 1 : 4 over the 0.35 m arc
		ColumnCouplet(Out, CH::HallCols[1], CH::HallEave, 0.16, FVector2D(0, 1), 1.9, Zc0, Zc1, P::CoupletLeft);
		ColumnCouplet(Out, CH::HallCols[2], CH::HallEave, 0.16, FVector2D(0, 1), 1.9, Zc0, Zc1, P::CoupletRight);
	}

	// ------------------------------------------------------------------------------------------------ the whole house

	void BuildHouse(FChMeshes& Out)
	{
		BuildHall(Out, FrontRow());
		BuildHall(Out, GateHall());
		GateJoinery(Out);
		BuildHall(Out, SideHall());
		BuildHall(Out, Mirror(SideHall()));
		BuildHall(Out, MainHall());
		BuildHall(Out, EarRoom());
		BuildHall(Out, Mirror(EarRoom()));
		BuildHall(Out, RearRow());
		BuildHall(Out, GalleryG1());
		BuildHall(Out, Mirror(GalleryG1()));
		BuildHall(Out, GalleryG2());
		BuildHall(Out, Mirror(GalleryG2()));
		BuildHall(Out, GalleryG3());
		BuildHall(Out, Mirror(GalleryG3()));
		BuildHall(Out, GalleryG4());
		BuildHall(Out, Mirror(GalleryG4()));
		GalleryFloors(Out);
		GalleryFretwork(Out);
		Chuihuamen(Out);
		ScreenWall(Out);
		Walls(Out);
		Courts(Out);
		Plaques(Out);
	}
}

namespace ChenghuaiBuild
{
	FChMeshes BuildMeshes()
	{
		FChMeshes Out;
		ChenghuaiHouse::BuildHouse(Out);
		BuildGarden(Out);
		BuildDisplay(Out);
		return Out;
	}

	TArray<FChLight> Lights()
	{
		TArray<FChLight> Out;
		DisplayLights(Out);
		GardenLights(Out);
		return Out;
	}
}
