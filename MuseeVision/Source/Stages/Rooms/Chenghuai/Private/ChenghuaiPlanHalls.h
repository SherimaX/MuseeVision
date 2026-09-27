#pragma once

#include "ChenghuaiHall.h"
#include "ChenghuaiPlan.h"
#include "CoreMinimal.h"

/**
 * The building table (§3.1): every building of Chenghuai as the one composition's spec, in its own frame, with the
 * numbers of the sheet's table and the galleries' (抄手游廊). The east side hall, the east ear room and the east galleries
 * are the west ones mirrored about the inner court's axis (x' = -18.40 - x).
 */
namespace Musee::Chenghuai::Plan
{
	/** The inner court's axis, x -9.20: the east buildings mirror the west ones about it. */
	inline constexpr double CourtAxis = -9.200;

	/** Even stations: From, From + Step, … (Count of them). */
	inline TArray<double> Even(double From, double Step, int32 Count)
	{
		TArray<double> Out;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Out.Add(From + Step * Index);
		}
		return Out;
	}

	/** 倒座房 · 臨池: faces north onto the front court; its back is the site's south wall; its east end is the gate's gable. */
	inline FHallSpec FrontRow()
	{
		FHallSpec H;
		H.Name = TEXT("front-row");
		H.Room = TEXT("linchi");
		H.Frame = FBuildingFrame::Of(FVector2D(-20.700, -5.300), FVector2D(1.0, 0.0), FVector2D(0.0, 1.0));
		H.U0 = 0.000;
		H.U1 = 18.900;
		H.GableT = 0.500;
		H.bOpenAtU1 = true;
		H.ColU = Even(0.500, 17.900 / 6.0, 7);
		H.Floor = 0.300;
		H.PlatformU0 = 0.000;
		H.PlatformU1 = 18.400;
		H.PlatformFront = -0.600;
		H.PlatformBack = 5.300;
		H.ColH = 2.500;
		H.ColD = 0.260;
		H.PurlinV = {0.000, 1.2625, 2.525, 3.7875, 5.050};
		H.Rise = {0.50, 0.72, -0.72, -0.50};
		H.Ridge = ERidge::Qingshui;
		H.FacadeV = 0.000;
		H.bBackWall = true;
		H.BackWallV0 = 4.800;
		H.BackWallV1 = 5.300;
		H.bSealedBack = true;
		H.EaveFront = 0.750;
		H.EaveBack = 0.280;
		H.Bays = {EBay::DoorsOpen, EBay::Windows, EBay::Windows, EBay::Windows, EBay::Windows, EBay::DoorsOpen};
		H.bPainted = true;
		H.WallBody = TEXT("brick.pointed");
		H.bRoofSpan = true;
		H.RoofU0 = 0.000;
		H.RoofU1 = 18.450;
		H.Lattice = ELattice::StepBrocade;
		H.OpenBayLattice = ELattice::StepBrocade;
		return H;
	}

	/** 广亮大门: the gate on the Rotunda's axis, facing south; its joinery is the gate's own (§3.7). */
	inline FHallSpec GateHall()
	{
		FHallSpec H;
		H.Name = TEXT("gate");
		H.Frame = FBuildingFrame::Of(FVector2D(-2.300, -0.300), FVector2D(1.0, 0.0), FVector2D(0.0, -1.0));
		H.U0 = 0.000;
		H.U1 = 4.600;
		H.GableT = 0.500;
		H.ColU = {0.800, 3.800};
		H.Floor = 0.900;
		H.PlatformU0 = 0.000;
		H.PlatformU1 = 4.600;
		H.PlatformFront = -0.300;
		H.PlatformBack = 5.200;
		H.ColH = 2.950;
		H.ColD = 0.300;
		H.PurlinV = {0.000, 1.225, 2.450, 3.675, 4.900};
		H.Rise = {0.50, 0.72, -0.72, -0.50};
		H.bTong = true;
		H.Ridge = ERidge::Qingshui;
		H.EaveFront = 0.900;
		H.EaveBack = 0.900;
		H.Bays = {EBay::Open};
		H.WallBody = TEXT("brick.rubbed");
		H.FloorSurface = TEXT("brick.on-edge");
		H.PlatformBody = TEXT("brick.rubbed");
		H.bPlaster = false;
		H.bColumns = false;
		H.bFrames = false;
		H.GableBackV = 5.360;
		return H;
	}

	/** 西厢房 · 天青: faces east onto the inner court; its veranda runs on through both gables as the galleries. */
	inline FHallSpec SideHall()
	{
		FHallSpec H;
		H.Name = TEXT("tianqing-hall");
		H.Room = TEXT("tianqing");
		H.Frame = FBuildingFrame::Of(FVector2D(-15.200, -24.400), FVector2D(0.0, 1.0), FVector2D(-1.0, 0.0));
		H.U0 = 0.000;
		H.U1 = 10.400;
		H.GableT = 0.500;
		H.ColU = {0.500, 3.550, 6.850, 9.900};
		H.Floor = 0.450;
		H.PlatformU0 = 0.000;
		H.PlatformU1 = 10.400;
		H.PlatformFront = -0.600;
		H.PlatformBack = 5.500;
		H.ColH = 2.900;
		H.ColD = 0.270;
		H.PurlinV = {0.000, 1.100, 2.350, 3.100, 4.350, 5.250};
		H.Rise = {0.50, 0.70, 0.00, -0.70, -0.45};
		H.bRolled = true;
		H.Ridge = ERidge::Rolled;
		H.FacadeV = 1.100;
		H.bBackWall = true;
		H.BackWallV0 = 5.000;
		H.BackWallV1 = 5.500;
		H.bSealedBack = true;
		H.EaveFront = 0.850;
		H.EaveBack = 0.300;
		H.Bays = {EBay::Windows, EBay::DoorsOpen, EBay::Windows};
		H.bPainted = true;
		H.GableOpenings = {{false, 0.060, 1.040, 2.300}, {true, 0.060, 1.040, 2.300}};
		H.Lattice = ELattice::StepBrocade;
		H.OpenBayLattice = ELattice::StepBrocade;
		return H;
	}

	/** 正房 · 澄懷堂: the main hall, facing south, open to the rafters; its gables open at the veranda and into the ear rooms. */
	inline FHallSpec MainHall()
	{
		FHallSpec H;
		H.Name = TEXT("main-hall");
		H.Room = TEXT("chenghuaitang");
		H.Frame = FBuildingFrame::Of(FVector2D(-14.700, -27.000), FVector2D(1.0, 0.0), FVector2D(0.0, -1.0));
		H.U0 = 0.000;
		H.U1 = 11.000;
		H.GableT = 0.500;
		H.ColU = {0.500, 3.550, 7.450, 10.500};
		H.InnerColV = {6.360};
		H.Floor = 0.750;
		H.PlatformU0 = 0.000;
		H.PlatformU1 = 11.000;
		H.PlatformFront = -0.700;
		H.PlatformBack = 7.900;
		H.ColH = 3.900;
		H.ColD = 0.320;
		H.PurlinV = {0.000, 1.300, 2.575, 3.850, 5.100, 6.360, 7.650};
		H.Rise = {0.50, 0.70, 0.90, -0.90, -0.70, -0.50};
		H.bTong = true;
		H.Ridge = ERidge::Qingshui;
		H.FacadeV = 1.300;
		H.bBackWall = true;
		H.BackWallV0 = 7.400;
		H.BackWallV1 = 7.900;
		H.bSealedBack = true;
		H.EaveFront = 1.100;
		H.EaveBack = 0.320;
		H.Bays = {EBay::Windows, EBay::DoorsOpen, EBay::Windows};
		H.bPainted = true;
		H.FloorSurface = TEXT("floor.chiqi");
		H.PlatformBody = TEXT("brick.rubbed");
		H.GableOpenings = {{false, 0.080, 1.220, 2.450}, {true, 0.080, 1.220, 2.450}, {false, 4.400, 5.600, 2.300}, {true, 4.400, 5.600, 2.300}};
		H.Lattice = ELattice::StepBrocade;
		H.OpenBayLattice = ELattice::StepBrocade;
		return H;
	}

	/** 西耳房 · 清閟: faces south beside the main hall; its east end is the hall's gable. */
	inline FHallSpec EarRoom()
	{
		FHallSpec H;
		H.Name = TEXT("qingbi-room");
		H.Room = TEXT("qingbi");
		H.Frame = FBuildingFrame::Of(FVector2D(-17.700, -28.900), FVector2D(1.0, 0.0), FVector2D(0.0, -1.0));
		H.U0 = 0.000;
		H.U1 = 3.000;
		H.GableT = 0.500;
		H.bOpenAtU1 = true;
		H.ColU = {0.500, 3.000};
		H.Floor = 0.500;
		H.PlatformU0 = 0.000;
		H.PlatformU1 = 3.000;
		H.PlatformFront = -0.600;
		H.PlatformBack = 5.000;
		H.ColH = 3.400;
		H.ColD = 0.260;
		H.PurlinV = {0.000, 1.1875, 2.375, 3.5625, 4.750};
		H.Rise = {0.50, 0.72, -0.72, -0.50};
		H.Ridge = ERidge::Qingshui;
		H.FacadeV = 0.000;
		H.bBackWall = true;
		H.BackWallV0 = 4.500;
		H.BackWallV1 = 5.000;
		H.bSealedBack = true;
		H.EaveFront = 0.800;
		H.EaveBack = 0.250;
		H.Bays = {EBay::Doors};
		H.FloorSurface = TEXT("floor.chiqi");
		H.bRoofSpan = true;
		H.RoofU0 = 0.000;
		H.RoofU1 = 3.050;
		return H;
	}

	/** 后罩房 · 舒卷: the rear row, facing south; its back is the site's north wall. */
	inline FHallSpec RearRow()
	{
		FHallSpec H;
		H.Name = TEXT("rear-row");
		H.Room = TEXT("shujuan");
		H.Frame = FBuildingFrame::Of(FVector2D(-20.700, -39.700), FVector2D(1.0, 0.0), FVector2D(0.0, -1.0));
		H.U0 = 0.000;
		H.U1 = 23.000;
		H.GableT = 0.500;
		H.ColU = Even(0.500, 22.000 / 7.0, 8);
		H.Floor = 0.300;
		H.PlatformU0 = 0.000;
		H.PlatformU1 = 23.000;
		H.PlatformFront = -0.600;
		H.PlatformBack = 5.200;
		H.ColH = 2.500;
		H.ColD = 0.260;
		H.PurlinV = {0.000, 1.2375, 2.475, 3.7125, 4.950};
		H.Rise = {0.50, 0.72, -0.72, -0.50};
		H.Ridge = ERidge::Qingshui;
		H.FacadeV = 0.000;
		H.bBackWall = true;
		H.BackWallV0 = 4.700;
		H.BackWallV1 = 5.200;
		H.bSealedBack = true;
		H.EaveFront = 0.750;
		H.EaveBack = 0.280;
		H.Bays = {EBay::DoorsOpen, EBay::Windows, EBay::Windows, EBay::Doors, EBay::Windows, EBay::Windows, EBay::DoorsOpen};
		H.bPainted = true;
		H.WallBody = TEXT("brick.pointed");
		H.Lattice = ELattice::StepBrocade;
		H.OpenBayLattice = ELattice::StepBrocade;
		return H;
	}

	/** 花厅 · 林泉: the flower hall at the garden's north end, facing south onto its terrace; chestnut, limewashed. */
	inline FHallSpec FlowerHall()
	{
		FHallSpec H;
		H.Name = TEXT("flower-hall");
		H.Room = TEXT("linquan");
		H.Frame = FBuildingFrame::Of(FVector2D(6.600, -38.500), FVector2D(1.0, 0.0), FVector2D(0.0, -1.0));
		H.U0 = 0.000;
		H.U1 = 8.100;
		H.GableT = 0.400;
		H.ColU = {0.400, 2.400, 5.700, 7.700};
		H.Floor = 0.450;
		H.PlatformU0 = -0.050;
		H.PlatformU1 = 8.150;
		H.PlatformFront = -0.300;
		H.PlatformBack = 3.950;
		H.ColH = 3.000;
		H.ColD = 0.240;
		H.PurlinV = {0.000, 1.150, 1.625, 2.125, 2.600, 3.750};
		H.Rise = {0.50, 0.75, 0.00, -0.75, -0.50};
		H.bRolled = true;
		H.Ridge = ERidge::Rolled;
		H.FacadeV = 0.000;
		H.bBackWall = true;
		H.BackWallV0 = 3.600;
		H.BackWallV1 = 3.900;
		H.EaveFront = 0.800;
		H.EaveBack = 0.700;
		H.Bays = {EBay::Doors, EBay::DoorsOpen, EBay::Doors};
		H.Timber = TEXT("timber.garden");
		H.Rafters = TEXT("timber.garden");
		H.WallBody = TEXT("wall.limewash");
		H.FloorSurface = TEXT("garden.bluestone");
		H.PlatformBody = TEXT("garden.bluestone");
		H.Lattice = ELattice::IceCrack;
		H.OpenBayLattice = ELattice::Lantern;
		return H;
	}

	/** 水榭 · 知魚: the water pavilion on the pond's east edge, facing west over the water, its west part standing in it. */
	inline FHallSpec WaterPavilion()
	{
		FHallSpec H;
		H.Name = TEXT("water-pavilion");
		H.Frame = FBuildingFrame::Of(FVector2D(13.300, -29.100), FVector2D(0.0, -1.0), FVector2D(1.0, 0.0));
		H.U0 = 0.000;
		H.U1 = 4.500;
		H.ColU = {0.300, 4.200};
		H.Floor = 0.350;
		H.PlatformU0 = -0.050;
		H.PlatformU1 = 4.550;
		H.PlatformFront = -0.300;
		H.PlatformBack = 3.100;
		H.ColH = 2.600;
		H.ColD = 0.200;
		H.PurlinV = {0.000, 1.050, 1.750, 2.800};
		H.Rise = {0.55, 0.00, -0.55};
		H.bRolled = true;
		H.Ridge = ERidge::Rolled;
		H.bBackColumns = true;
		H.EaveFront = 0.750;
		H.EaveBack = 0.100;
		H.Bays = {EBay::Open};
		H.Timber = TEXT("timber.garden");
		H.Rafters = TEXT("timber.garden");
		H.FloorSurface = TEXT("garden.bluestone");
		H.PlatformBody = TEXT("garden.bluestone");
		H.bPlaster = false;
		H.bRoofSpan = true;
		H.RoofU0 = -0.550;
		H.RoofU1 = 5.050;
		return H;
	}

	/** 复廊 · 遊目: the covered walk along the house's east wall, a single slope to the wall's limewashed face at x 2.30. */
	inline FHallSpec CoveredWalk()
	{
		FHallSpec H;
		H.Name = TEXT("walk");
		H.Frame = FBuildingFrame::Of(FVector2D(3.550, -5.500), FVector2D(0.0, -1.0), FVector2D(-1.0, 0.0));
		H.U0 = 0.000;
		H.U1 = 33.800;
		H.ColU = Even(0.300, 2.700, 13);
		H.Floor = 0.150;
		H.PlatformU0 = -0.100;
		H.PlatformU1 = 33.900;
		H.PlatformFront = -0.150;
		H.PlatformBack = 1.280;
		H.ColH = 2.300;
		H.ColD = 0.200;
		H.PurlinV = {0.000, 1.250};
		H.Rise = {0.45};
		H.Ridge = ERidge::None;
		H.EaveFront = 0.550;
		H.EaveBack = 0.060;
		for (int32 Bay = 0; Bay < 12; ++Bay)
		{
			H.Bays.Add(EBay::Open);
		}
		H.Timber = TEXT("timber.garden");
		H.Rafters = TEXT("timber.garden");
		H.FloorSurface = TEXT("garden.bluestone");
		H.PlatformBody = TEXT("garden.bluestone");
		H.bPlaster = false;
		H.bRoofSpan = true;
		H.RoofU0 = -0.300;
		H.RoofU1 = 34.100;
		return H;
	}

	/** A gallery's common section: painted, rolled 合瓦, floor 0.45, columns 2.60 × 0.22, purlins at 0, 0.45, 0.85, 1.30. */
	inline FHallSpec Gallery(const TCHAR* Name, const FBuildingFrame& Frame)
	{
		FHallSpec H;
		H.Name = Name;
		H.Frame = Frame;
		H.Floor = 0.450;
		H.ColH = 2.600;
		H.ColD = 0.220;
		H.PurlinV = {0.000, 0.450, 0.850, 1.300};
		H.Rise = {0.55, 0.00, -0.55};
		H.bRolled = true;
		H.Ridge = ERidge::Rolled;
		H.bBackColumns = true;
		H.EaveFront = 0.600;
		H.EaveBack = 0.600;
		H.bPainted = true;
		H.bPlaster = false;
		H.PlatformFront = -0.100;
		H.PlatformBack = 1.400;
		H.bRoofSpan = true;
		return H;
	}

	/** G1, the south gallery along the court wall, from the corner to the 垂花门; its back eave runs 0.12 into the wall. */
	inline FHallSpec GalleryG1()
	{
		FHallSpec H = Gallery(TEXT("gallery.g1.west"), FBuildingFrame::Of(FVector2D(-16.300, -12.900), FVector2D(1.0, 0.0), FVector2D(0.0, 1.0)));
		H.U0 = 0.000;
		H.U1 = 5.400;
		H.ColU = {0.000, 1.300, 3.400, 5.200};
		H.Bays = {EBay::Open, EBay::Open, EBay::Open};
		H.PlatformU0 = -0.100;
		H.PlatformU1 = 5.400;
		H.EaveBack = 0.120;
		H.MitreAtU0 = -1;
		H.RoofU0 = 1.300;
		H.RoofU1 = 5.450;
		return H;
	}

	/** G2, the west gallery's short south part, from the corner to the side hall's gable. */
	inline FHallSpec GalleryG2()
	{
		FHallSpec H = Gallery(TEXT("gallery.g2.west"), FBuildingFrame::Of(FVector2D(-15.000, -11.600), FVector2D(0.0, -1.0), FVector2D(-1.0, 0.0)));
		H.U0 = 1.300;
		H.U1 = 2.450;
		H.ColU = {1.300, 2.300};
		H.Bays = {EBay::Open};
		H.PlatformU0 = 1.400;
		H.PlatformU1 = 2.400;
		H.bColumns = false;
		H.FrameU = {2.300};
		H.MitreAtU0 = -1;
		H.RoofU0 = 1.300;
		H.RoofU1 = 2.450;
		return H;
	}

	/** G3, the west gallery's north part, from the side hall's north gable to the corner by the main hall. */
	inline FHallSpec GalleryG3()
	{
		FHallSpec H = Gallery(TEXT("gallery.g3.west"), FBuildingFrame::Of(FVector2D(-15.000, -24.400), FVector2D(0.0, -1.0), FVector2D(-1.0, 0.0)));
		H.U0 = -0.050;
		H.U1 = 3.800;
		H.ColU = {0.100, 2.400};
		H.Bays = {EBay::Open};
		H.bPlatform = false;
		H.bColumns = false;
		H.FrameU = {0.100, 2.400};
		H.MitreAtU1 = 1;
		H.RoofU0 = -0.050;
		H.RoofU1 = 2.500;
		return H;
	}

	/** G4, its short turn east into the main hall's veranda, through the gable. */
	inline FHallSpec GalleryG4()
	{
		FHallSpec H = Gallery(TEXT("gallery.g4.west"), FBuildingFrame::Of(FVector2D(-15.000, -26.900), FVector2D(1.0, 0.0), FVector2D(0.0, -1.0)));
		H.U0 = 0.000;
		H.U1 = 0.350;
		H.ColU = {0.000, 0.350};
		H.Bays = {EBay::Open};
		H.bPlatform = false;
		H.bColumns = false;
		H.bFrames = false;
		H.MitreAtU0 = -1;
		H.RoofU0 = 0.000;
		H.RoofU1 = 0.350;
		return H;
	}

	/** The 垂花门's two roofs (一殿一卷) over x -11.35 … -7.05: the front from y -10.20, the rolled back from y -11.30, sharing the
	 *  valley's purlin; its columns, beams and posts are the gate's own (§3.7). */
	inline FHallSpec ChuihuaRoof(bool bFront)
	{
		FHallSpec H;
		H.Name = bFront ? TEXT("chuihuamen.front-roof") : TEXT("chuihuamen.back-roof");
		H.Frame = FBuildingFrame::Of(FVector2D(-11.350, bFront ? -10.200 : -11.300), FVector2D(1.0, 0.0), FVector2D(0.0, -1.0));
		H.U0 = 0.000;
		H.U1 = 4.300;
		H.ColU = {0.750, 3.550};
		H.Floor = ChuiFloor;
		H.ColH = 2.620;
		H.ColD = 0.240;
		H.bTong = true;
		if (bFront)
		{
			H.PurlinV = {0.000, 0.550, 1.100};
			H.Rise = {0.80, -0.80};
			H.Ridge = ERidge::Qingshui;
			H.EaveFront = 0.750;
			H.EaveBack = 0.040;
		}
		else
		{
			H.PurlinV = {0.000, 0.420, 0.720, 1.100};
			H.Rise = {0.65, 0.00, -0.90};
			H.bRolled = true;
			H.Ridge = ERidge::Rolled;
			H.EaveFront = 0.040;
			H.EaveBack = 0.700;
			H.SkipPurlins = {0};
		}
		H.Bays = {EBay::Open};
		H.bPainted = true;
		H.bPlaster = false;
		H.bPlatform = false;
		H.bColumns = false;
		H.bFrames = false;
		H.bRoofSpan = true;
		H.RoofU0 = 0.000;
		H.RoofU1 = 4.300;
		return H;
	}
}
