#pragma once

#include "CoreMinimal.h"

/**
 * Chenghuai's plan: every number the house is built to, in metres in the stage's frame (origin on the gate's axis at the
 * site's south outer face, on the Rotunda's floor; x east, y south, z up), taken from its design sheet,
 * Docs/Stages/chenghuai.md (the section is given with each group). The sheet's own sources are the as-built wing
 * (windows/Source/MuseeVision/Chenghuai, y + 17.000, D-017) and the approved boards (D-001). Finishes, light, planting and
 * hanging are the schedules' (Data/Stages/chenghuai), never here.
 *
 * The plan is split by section for its size: this file holds the site, the port and the house's trades (§1-§3.8, §3.11);
 * ChenghuaiPlanHalls.h the building table (§3.1); ChenghuaiPlanGarden.h the garden (§3.9) and ChenghuaiPlanFittings.h the
 * fittings (§3.10).
 */
namespace Musee::Chenghuai
{
	/** A point of a section as the sheet writes it: (A, B), smooth where the sheet marks it with an asterisk. */
	struct FSectionPoint
	{
		double A = 0.0;
		double B = 0.0;
		bool bSmooth = false;
	};
}

namespace Musee::Chenghuai::Plan
{
	/** A point on the plan. */
	struct FPlanPoint
	{
		double X, Y;
	};
	/** Rectangles on the plan: x from X0 to X1, y from Y0 to Y1. */
	struct FPlanRect
	{
		double X0, X1, Y0, Y1;
	};

	/** The stage's key (§1). */
	inline constexpr const TCHAR* Stage = TEXT("chenghuai");

	// ---- §1 identity, footprint and neighbours --------------------------------------------------------------------------
	/** museum = local + (0, -17.000, 0): the mating chenghuai.south to rotunda.north. */
	inline constexpr double MuseumOffsetY = -17.000;
	/** The site's outer faces: the house x -20.70 … 2.30, the garden 2.30 … 16.90; y 0 (south) … -44.90 (north). */
	inline constexpr double SiteWest = -20.700;
	inline constexpr double SiteEast = 16.900;
	inline constexpr double SiteSouth = 0.000;
	inline constexpr double SiteNorth = -44.900;
	/** The Rotunda's drum: its outer face, a cylinder of radius 11.20 about (0, 17.000), bulging into the porch. */
	inline constexpr double DrumCentreY = 17.000;
	inline constexpr double DrumRadius = 11.200;
	/** The drum's arc of the footprint is set out at this many stations between the porch walls' outer faces. */
	inline constexpr int32 DrumArcStations = 24;
	/** From the footings under the bridge's piers (outside the tribune's zone) to the main hall's ridge top. */
	inline constexpr double FootprintBottom = -0.950;
	inline constexpr double FootprintTop = 8.463;
	/** The Classical Hall beneath: nothing below z -0.50 inside x ±4.20 north of the drum or r 8.30 of (0, -22.30). */
	inline constexpr double HallClearZ = -0.500;
	inline constexpr double TribuneX = 0.000;
	inline constexpr double TribuneY = -22.300;
	inline constexpr double TribuneClear = 8.300;

	// ---- §1 the enclosures' plan extents and the courts' open tops ---------------------------------------------------------
	/** A court's extent: x from X0 to X1, y from Y0 (south) to Y1 (north); its top is Open at CourtTop. */
	struct FCourtExtent
	{
		const TCHAR* Local;
		double X0, X1, Y0, Y1;
	};
	inline constexpr double CourtTop = 3.200;
	inline constexpr FCourtExtent Courts[] = {
		{TEXT("entry-yard"), -1.900, 1.800, -5.500, -11.100},
		{TEXT("front-court"), -20.200, -2.300, -5.900, -11.100},
		{TEXT("inner-court"), -20.200, 1.800, -11.500, -30.700},
		{TEXT("rear-court"), -20.200, 1.800, -31.100, -39.700},
	};

	// ---- §2 the port and the porch -----------------------------------------------------------------------------------------
	inline constexpr const TCHAR* PortClass = TEXT("doorway.4000x6500.arched");
	inline constexpr const TCHAR* PortMates = TEXT("rotunda.north");
	/** On the drum's face at the threshold, on the axis; the porch is open, so the passage through Chenghuai is nil. */
	inline constexpr double PortY = 5.800;
	/** The class: clear 4.000 (jambs ±2.000), a semicircular head of r 2.000 springing at 4.500, crown 6.500. */
	inline constexpr double PortClearWidth = 4.000;
	inline constexpr double PortCrown = 6.500;
	/** The porch walls stand at |x| 2.35 … 2.95 from y 0 to the drum; z -0.10 … 0.98; coping 0.98 … 1.10, 0.04 proud. */
	inline constexpr double PorchInner = 2.350;
	inline constexpr double PorchOuter = 2.950;
	inline constexpr double PorchWallBottom = -0.100;
	inline constexpr double PorchWallTop = 0.980;
	inline constexpr double PorchCopingTop = 1.100;
	inline constexpr double PorchCopingProud = 0.040;
	/** The porch paving: top 0.000, 0.12 deep, cut round the gate's front flight and side stones (|x| ≤ 1.50 to y 1.17) and the
	 *  gate's footing and gable ends (to y 0.06). */
	inline constexpr double PorchPavingDepth = 0.120;
	inline constexpr double PorchStepsHalf = 1.500;
	inline constexpr double PorchStepsReach = 1.170;
	inline constexpr double PorchGateClear = 0.060;

	// ---- §2 the anchors (the stage contract) --------------------------------------------------------------------------------
	/**
	 * An anchor: its key under the stage, its kind, its origin and its x axis (and its y, for a wall; a mount or a bed has z up
	 * and y following). A Wall's origin is on the hanging face at its left end as the viewer sees it, x to the viewer's right, y
	 * out of the face; a bed on the rockery stands on its surface there.
	 */
	struct FAnchorPlan
	{
		const TCHAR* Local;
		const TCHAR* Kind;
		double X, Y, Z;
		double AxisX[3];
		double AxisY[3];
		bool bOnRockery;
	};
	/** The handscroll beds tilt 11° up the slope towards the reader: cos 11° = 0.9816272, sin 11° = 0.1908090. */
	inline constexpr double BedCos = 0.9816272;
	inline constexpr double BedSin = 0.1908090;
	inline constexpr FAnchorPlan Anchors[] = {
		{TEXT("chenghuaitang.case.west"), TEXT("Wall"), -13.505, -34.365, 0.750, {1, 0, 0}, {0, 1, 0}, false},
		{TEXT("chenghuaitang.case.centre"), TEXT("Wall"), -9.955, -34.365, 0.750, {1, 0, 0}, {0, 1, 0}, false},
		{TEXT("chenghuaitang.case.east"), TEXT("Wall"), -6.655, -34.365, 0.750, {1, 0, 0}, {0, 1, 0}, false},
		{TEXT("qingbi.case.back"), TEXT("Wall"), -16.455, -33.365, 0.500, {1, 0, 0}, {0, 1, 0}, false},
		{TEXT("qingbi.case.side"), TEXT("Wall"), -17.165, -31.445, 0.500, {0, -1, 0}, {1, 0, 0}, false},
		{TEXT("tingyun.case.back"), TEXT("Wall"), -3.155, -33.365, 0.500, {1, 0, 0}, {0, 1, 0}, false},
		{TEXT("tingyun.case.side"), TEXT("Wall"), -1.235, -32.555, 0.500, {0, 1, 0}, {-1, 0, 0}, false},
		{TEXT("linchi.case.bed"), TEXT("Wall"), -4.735, -1.250, 1.215, {-1, 0, 0}, {0, -BedSin, BedCos}, false},
		{TEXT("shujuan.case.thousand-li.bed"), TEXT("Wall"), -19.665, -43.650, 1.215, {1, 0, 0}, {0, BedSin, BedCos}, false},
		{TEXT("shujuan.case.qingming.bed"), TEXT("Wall"), -5.265, -43.650, 1.215, {1, 0, 0}, {0, BedSin, BedCos}, false},
		{TEXT("entry-yard.yingbi"), TEXT("Wall"), -0.900, -10.535, 0.020, {1, 0, 0}, {0, 1, 0}, false},
		{TEXT("walk.stones"), TEXT("Wall"), 2.330, -9.650, 0.150, {0, -1, 0}, {1, 0, 0}, false},
		{TEXT("tianqing.case.deck"), TEXT("Mount"), -19.900, -19.200, 1.282, {0, 1, 0}, {0, 0, 0}, false},
		{TEXT("changnan.case.deck"), TEXT("Mount"), 1.500, -19.200, 1.282, {0, 1, 0}, {0, 0, 0}, false},
		{TEXT("tianqing.pedestal"), TEXT("Mount"), -18.200, -19.200, 1.412, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("changnan.table"), TEXT("Mount"), -0.200, -19.200, 1.250, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("garden.shixiong"), TEXT("Mount"), 5.300, -37.300, 0.020, {-1, 0, 0}, {0, 0, 0}, false},
		{TEXT("chenghuaitang.altar-table"), TEXT("Mount"), -9.200, -32.750, 1.570, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("linchi.desk"), TEXT("Mount"), -3.500, -2.275, 1.120, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("linquan.table"), TEXT("Mount"), 10.650, -40.600, 1.250, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("pond.water"), TEXT("Water"), 9.603, -29.668, -0.280, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("fish-bowl.water"), TEXT("Water"), -9.200, -19.200, 0.606, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("inner-court.tub.w"), TEXT("Bed"), -11.600, -25.600, 0.425, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("inner-court.tub.e"), TEXT("Bed"), -6.800, -25.600, 0.425, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("pond.lotus"), TEXT("Bed"), 9.603, -29.668, -0.280, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("inner-court.bed.nw"), TEXT("Bed"), -12.200, -22.600, 0.080, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("inner-court.bed.ne"), TEXT("Bed"), -6.200, -22.600, 0.080, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("inner-court.bed.sw"), TEXT("Bed"), -12.200, -15.800, 0.080, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("inner-court.bed.se"), TEXT("Bed"), -6.200, -15.800, 0.080, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("rear-court.pit.w"), TEXT("Bed"), -16.800, -35.300, 0.050, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("rear-court.pit.e"), TEXT("Bed"), -1.600, -35.300, 0.050, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("garden.ground"), TEXT("Bed"), 9.350, -22.450, 0.020, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("garden.tree.willow-1"), TEXT("Bed"), 6.000, -23.100, 0.020, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("garden.tree.willow-2"), TEXT("Bed"), 5.100, -20.900, 0.020, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("garden.tree.pine"), TEXT("Bed"), 13.400, -15.900, 0.020, {1, 0, 0}, {0, 0, 0}, true},
		{TEXT("garden.tree.plum-1"), TEXT("Bed"), 15.100, -37.000, 0.020, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("garden.tree.plum-2"), TEXT("Bed"), 14.700, -20.700, 0.020, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("garden.tree.crabapple"), TEXT("Bed"), 6.800, -5.300, 0.020, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("garden.bamboo.east"), TEXT("Bed"), 15.810, -25.990, 0.020, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("garden.bamboo.north-west"), TEXT("Bed"), 7.640, -43.380, 0.020, {1, 0, 0}, {0, 0, 0}, false},
		{TEXT("garden.bamboo.north-east"), TEXT("Bed"), 13.580, -43.630, 0.020, {1, 0, 0}, {0, 0, 0}, false},
	};
	/** The pavilion's four seats (faces k 1, 2, 4, 5), their tops at 3.190, each facing the pavilion's centre (10.600, -11.300). */
	inline constexpr FPlanPoint Seats[] = {{10.600, -10.224}, {9.668, -10.762}, {10.600, -12.376}, {11.532, -11.838}};
	inline constexpr int32 SeatFaces[] = {1, 2, 4, 5};
	inline constexpr double SeatTop = 3.190;
	inline constexpr double PavilionX = 10.600;
	inline constexpr double PavilionY = -11.300;

	// ---- §1 the openings between the stage's own enclosures ------------------------------------------------------------------
	/**
	 * An Open opening shared by two enclosures, declared once for each it opens (a Form opening names one enclosure): its
	 * centre on the plan and in height, its size, and the direction it faces out of the first; the second faces the other way
	 * (none when it opens onto the porch or the garden, which have no shell).
	 */
	struct FSharedOpening
	{
		const TCHAR* Local;
		const TCHAR* First;
		const TCHAR* Second;
		double X, Y, Z;
		double OutX, OutY;
		double Width, Height;
	};
	inline constexpr FSharedOpening SharedOpenings[] = {
		{TEXT("gate.front"), TEXT("gate-hall"), nullptr, 0.000, -0.300, 2.375, 0.0, 1.0, 3.600, 2.950},
		{TEXT("gate.back"), TEXT("gate-hall"), TEXT("entry-yard"), 0.000, -5.200, 2.375, 0.0, -1.0, 3.600, 2.950},
		{TEXT("yard-door"), TEXT("entry-yard"), TEXT("front-court"), -2.100, -8.400, 1.175, -1.0, 0.0, 1.800, 2.350},
		{TEXT("garden-door"), TEXT("entry-yard"), nullptr, 2.050, -8.400, 1.175, 1.0, 0.0, 1.800, 2.350},
		{TEXT("chuihuamen"), TEXT("front-court"), TEXT("inner-court"), -9.200, -11.300, 1.622, 0.0, -1.0, 2.320, 2.104},
		{TEXT("cross-door.west"), TEXT("inner-court"), TEXT("rear-court"), -18.950, -30.900, 1.150, 0.0, -1.0, 1.100, 2.300},
		{TEXT("cross-door.east"), TEXT("inner-court"), TEXT("rear-court"), 0.550, -30.900, 1.150, 0.0, -1.0, 1.100, 2.300},
		{TEXT("moon-gate"), TEXT("rear-court"), nullptr, 2.050, -37.300, 1.250, 1.0, 0.0, 2.400, 2.300},
		{TEXT("hall-door.west"), TEXT("chenghuaitang"), TEXT("qingbi"), -14.450, -32.000, 1.900, -1.0, 0.0, 1.200, 2.300},
		{TEXT("hall-door.east"), TEXT("chenghuaitang"), TEXT("tingyun"), -3.950, -32.000, 1.900, 1.0, 0.0, 1.200, 2.300},
	};

	/** The court each room's open door pairs open onto (none for the flower hall, which opens onto the garden). */
	struct FRoomCourt
	{
		const TCHAR* Room;
		const TCHAR* Court;
	};
	inline constexpr FRoomCourt RoomCourts[] = {{TEXT("linchi"), TEXT("front-court")}, {TEXT("tianqing"), TEXT("inner-court")},
		{TEXT("changnan"), TEXT("inner-court")}, {TEXT("chenghuaitang"), TEXT("inner-court")}, {TEXT("shujuan"), TEXT("rear-court")}};

	// ---- §3.1 the section rule of the Qing hall (ChenghuaiHall.cpp:129-177) ------------------------------------------------
	/** The eave purlin's seat (the rafters' underside) stands 0.12 plus the purlin's diameter over the column's top. */
	inline constexpr double SeatOver = 0.120;
	/** The purlin's radius: 0.45 of the column's diameter, never under 0.09. */
	inline constexpr double PurlinPerDiameter = 0.450;
	inline constexpr double PurlinRadiusLeast = 0.090;
	/** The roof's build-up over a purlin: round rafters 0.095, boarding 0.020, lime and mud 0.140; a rolled ridge's middle 0.06 up. */
	inline constexpr double RafterDiameter = 0.095;
	inline constexpr double Boarding = 0.020;
	inline constexpr double LimeBed = 0.140;
	inline constexpr double RolledRidgeLift = 0.060;
	/** Round rafters reach 0.66 of the eave; flying rafters from 0.90 behind their end to the edge, at 0.55 of the first slope. */
	inline constexpr double RoundRafterReach = 0.660;
	inline constexpr double FlyingRafterTail = 0.900;
	inline constexpr double FlyingRafterSlope = 0.550;
	inline constexpr double FlyingRafter = 0.085;
	/** A sealed back runs the bed on at 0.8 of the last slope to the back wall's outer face plus the back eave. */
	inline constexpr double SealedBackSlope = 0.800;
	/** The bed's knots at the eave: its edge 0.06 and the round rafters' end 0.08 over the flying boarding (as built). */
	inline constexpr double EaveEdgeKnot = 0.060;
	inline constexpr double RoundEndKnot = 0.080;

	// ---- §3.2 platforms, columns and flights -------------------------------------------------------------------------------
	/** The footing course, 0.05 proud all round, -0.10 … +0.03; the kerb 0.34 wide × 0.13 round the top; corner stones 0.30. */
	inline constexpr double FootingBottom = -0.100;
	inline constexpr double FootingTop = 0.030;
	inline constexpr double FootingProud = 0.050;
	inline constexpr double KerbWidth = 0.340;
	inline constexpr double KerbDepth = 0.130;
	inline constexpr double CornerStone = 0.300;
	/** The corner stones stand 5 mm proud of the platform's faces (as built). */
	inline constexpr double CornerStoneProud = 0.005;
	/** A platform no higher than this has solid kerbs from the footing to its floor (the walk). */
	inline constexpr double LowPlatform = 0.210;
	/** The column's base stone, 1.9 D square, floor -0.08 … +0.012; its drum from the stone's top; the shaft from +0.06. */
	inline constexpr double BaseStonePerDiameter = 1.900;
	inline constexpr double BaseStoneBottom = -0.080;
	inline constexpr double BaseStoneTop = 0.012;
	inline constexpr double ShaftFoot = 0.060;
	/** The shaft's taper (收分): R0 (1 - 0.001 H / D); its foot's arris eased 8 mm. */
	inline constexpr double ShaftTaper = 0.001;
	inline constexpr double ShaftArris = 0.008;
	/** The turned drum (鼓镜) on the base stone, (r in diameters of the column, z metres); its shoulder smooth. */
	inline constexpr FSectionPoint DrumSection[] = {{0.0, 0.0}, {0.68, 0.0}, {0.68, 0.012}, {0.62, 0.045, true}, {0.55, 0.055}, {0.0, 0.055}};
	/** Facade-line and 金柱 columns are 0.03 thicker, and stop 0.05 under their purlin's lintel. */
	inline constexpr double InnerColumnThicker = 0.030;
	inline constexpr double InnerColumnGap = 0.050;
	/** A flight's treads are bedded 0.08 into the ground under their foot. */
	inline constexpr double FlightBed = 0.080;
	/** The sloping side stones (垂带), 0.30 wide each side, 0.10 over the ground at 0.05 past the foot (as built). */
	inline constexpr double SideStone = 0.300;
	inline constexpr double SideStoneToe = 0.100;
	inline constexpr double SideStoneReach = 0.050;

	/** The west gallery's floors (their east twins mirrored about x -9.20): the lower at 0.45 with its kerb on the court's side,
	 *  the upper at 0.75 by the main hall's veranda with a kerb stone at its corner (as built). */
	inline constexpr double GalleryLowerX0 = -16.400;
	inline constexpr double GalleryLowerX1 = -14.950;
	inline constexpr double GalleryLowerY0 = -26.000;
	inline constexpr double GalleryLowerY1 = -24.350;
	inline constexpr double GalleryKerbX1 = -14.850;
	inline constexpr double GalleryLower = 0.450;
	inline constexpr double GalleryUpperX1 = -14.700;
	inline constexpr double GalleryUpperY0 = -28.350;
	inline constexpr double GalleryUpperY1 = -26.330;
	inline constexpr double GalleryUpper = 0.750;
	inline constexpr double GalleryCornerKerbY0 = -26.900;
	inline constexpr double GalleryCornerKerbY1 = -26.300;
	/** The galleries' own columns (red, on bases 0.40 square), each on its floor; their tops at the galleries' 3.05. */
	struct FGalleryColumn
	{
		double X, Y, Floor;
	};
	inline constexpr FGalleryColumn GalleryColumns[] = {{-15.000, -24.500, 0.450}, {-16.300, -24.500, 0.450}, {-15.000, -26.800, 0.750},
		{-16.300, -26.800, 0.750}, {-16.300, -28.200, 0.750}, {-15.000, -13.900, 0.450}, {-16.300, -13.900, 0.450}};
	inline constexpr double GalleryColumnBase = 0.400;
	inline constexpr double GalleryColumnD = 0.220;
	inline constexpr double GalleryColumnTop = 3.050;

	/**
	 * A flight (踏跺) by Blondel (2R + T = 0.63): its top at a platform's edge, running Out (a unit plan direction) down to
	 * the ground, centred on Across; Risers of (Top - Ground) / Risers on goings of Going.
	 */
	struct FFlightPlan
	{
		const TCHAR* Local;
		double EdgeX, EdgeY;
		double OutX, OutY;
		double Width;
		double Ground, Top;
		int32 Risers;
		double Going;
		bool bSideStones;
	};
	inline constexpr FFlightPlan Flights[] = {
		{TEXT("gate.front"), 0.000, 0.000, 0.0, 1.0, 2.400, 0.000, 0.900, 5, 0.275, true},
		{TEXT("gate.back"), 0.000, -5.500, 0.0, -1.0, 2.400, 0.020, 0.900, 5, 0.278, true},
		{TEXT("front-row.west"), -18.708, -5.900, 0.0, -1.0, 1.500, 0.020, 0.300, 2, 0.350, true},
		{TEXT("front-row.east"), -3.792, -5.900, 0.0, -1.0, 1.500, 0.020, 0.300, 2, 0.350, true},
		{TEXT("tianqing"), -14.600, -19.200, 1.0, 0.0, 2.000, 0.020, 0.450, 3, 0.343, true},
		{TEXT("changnan"), -3.800, -19.200, -1.0, 0.0, 2.000, 0.020, 0.450, 3, 0.343, true},
		{TEXT("chenghuaitang"), -9.200, -26.300, 0.0, 1.0, 3.000, 0.020, 0.750, 4, 0.265, true},
		{TEXT("rear-row.west"), -18.629, -39.100, 0.0, 1.0, 1.500, 0.020, 0.300, 2, 0.350, true},
		{TEXT("rear-row.middle"), -9.200, -39.100, 0.0, 1.0, 1.500, 0.020, 0.300, 2, 0.350, true},
		{TEXT("rear-row.east"), 0.229, -39.100, 0.0, 1.0, 1.500, 0.020, 0.300, 2, 0.350, true},
		{TEXT("chuihuamen"), -9.200, -9.900, 0.0, 1.0, 2.000, 0.020, 0.450, 3, 0.343, true},
		{TEXT("gallery.west.to-hall"), -15.675, -26.330, 0.0, 1.0, 1.450, 0.450, 0.750, 2, 0.330, false},
		{TEXT("gallery.east.to-hall"), -2.725, -26.330, 0.0, 1.0, 1.450, 0.450, 0.750, 2, 0.330, false},
		{TEXT("gallery.west.to-passage"), -16.400, -25.400, -1.0, 0.0, 1.200, 0.020, 0.450, 3, 0.343, true},
		{TEXT("gallery.east.to-passage"), -2.000, -25.400, 1.0, 0.0, 1.200, 0.020, 0.450, 3, 0.343, true},
		{TEXT("zhiyu.south"), 15.200, -29.050, 0.0, 1.0, 1.200, 0.020, 0.350, 2, 0.300, true},
		{TEXT("zhiyu.north"), 15.200, -33.650, 0.0, -1.0, 1.200, 0.020, 0.350, 2, 0.300, true},
		{TEXT("terrace"), 10.650, -36.700, 0.0, 1.0, 1.500, 0.020, 0.300, 2, 0.350, true},
	};

	// ---- §3.3 the timber frame (ChenghuaiHall.cpp:485-690) -----------------------------------------------------------------
	/** Purlins run 0.12 into the gables. */
	inline constexpr double PurlinIntoGable = 0.120;
	/** The eave lintel (檐枋): (0.9 D + 0.04) high × 0.75 D under the column's top; its board (垫板) 0.06 thick up to the purlin. */
	inline constexpr double LintelPerDiameter = 0.900;
	inline constexpr double LintelOver = 0.040;
	inline constexpr double LintelWidthPerDiameter = 0.750;
	inline constexpr double LintelBoard = 0.060;
	inline constexpr double LintelBoardUnderPurlin = 0.020;
	/** The main beam: W = 0.8 D + 0.04, H = 1.2 D + 0.06, its top 1.6 purlin radii under the lower of its two seats; each
	 *  beam above 0.88 as wide and 0.85 as high; struts 0.7 W square, blocks and the ridge strut 0.6 W. */
	inline constexpr double BeamWidthPerDiameter = 0.800;
	inline constexpr double BeamWidthOver = 0.040;
	inline constexpr double BeamHeightPerDiameter = 1.200;
	inline constexpr double BeamHeightOver = 0.060;
	inline constexpr double BeamUnderSeat = 1.600;
	inline constexpr double BeamTierWidth = 0.880;
	inline constexpr double BeamTierHeight = 0.850;
	inline constexpr double StrutPerBeam = 0.700;
	inline constexpr double BlockPerBeam = 0.600;
	/** The beams' ends run 0.22 past the front support, 0.18 past an inner purlin, 0.20 past the back one (as built). */
	inline constexpr double BeamPastFront = 0.220;
	inline constexpr double BeamPastInner = 0.180;
	inline constexpr double BeamPastBack = 0.200;
	/** A veranda's tie (穿插枋), 0.6 D × 0.8 D, 0.75 under the column's top. */
	inline constexpr double TieWidthPerDiameter = 0.600;
	inline constexpr double TieHeightPerDiameter = 0.800;
	inline constexpr double TieUnderTop = 0.750;
	/** The veranda's beam (抱头梁) from 0.22 before the eave column to 0.05 past the facade column, its top 1.2 purlin radii
	 *  under the eave's seat; the back beam behind the 金柱 the same at the back; the tie from 0.05 before the eave column;
	 *  the ridge strut to 1.8 radii under the ridge's seat (as built). */
	inline constexpr double VerandaBeamPast = 0.050;
	inline constexpr double VerandaBeamUnderSeat = 1.200;
	inline constexpr double TieFrom = -0.050;
	inline constexpr double RidgeStrutUnderSeat = 1.800;
	/** A block stands under the higher end's purlin where two seats differ by more than 0.03 (as built). */
	inline constexpr double SeatStep = 0.030;
	/** Round rafters at 0.22 (一椽一当), 0.04 past each purlin; the eave's from 0.20 behind the eave purlin; inset 0.12 plus
	 *  the gable from a gable, 0.08 from an open end (as built). */
	inline constexpr double RafterPitch = 0.220;
	inline constexpr double RafterInset = 0.120;
	inline constexpr double RafterInsetOpen = 0.080;

	// ---- §3.4 roofs, tiles and ridges (ChenghuaiRoof.h:61-93) --------------------------------------------------------------
	/** Tiles are sampled at a tenth of their row's pitch: ten stations across a row. */
	inline constexpr int32 TileStationsPerRow = 10;
	/** The rafters and the edge mouldings are sampled every 2 cm: five stations across a rafter. */
	inline constexpr double MemberGrid = 0.020;
	/** Each course's lower end stands 9 mm proud of the next. */
	inline constexpr double TileLip = 0.009;
	/** Rows start 0.13 from a 清水脊's line (0 for a rolled ridge) and 0.26 in from a gable (the edge tiles). */
	inline constexpr double RowsFromRidge = 0.130;
	inline constexpr double EdgeTiles = 0.260;
	/** The relief's block under the tiles and edges: a centimetre into the lime. */
	inline constexpr double ReliefBack = 0.010;

	/** A style of tiling: its row pitch, the lower pan's and upper tile's half-widths, and their courses. */
	struct FTileStyle
	{
		double Pitch, PanHalf, TopHalf, PanCourse, TopCourse;
		const TCHAR* Surface;
	};
	/** 筒瓦: concave pans under half-round covers (the gate, the main hall, the 垂花门). */
	inline constexpr FTileStyle Tong{0.260, 0.115, 0.065, 0.190, 0.300, TEXT("roof.tong")};
	/** 合瓦: pans face up and pans face down over their joints (everything else, and every coping). */
	inline constexpr FTileStyle He{0.250, 0.115, 0.105, 0.170, 0.170, TEXT("roof.he")};
	/** Sections over the bed, o across and h the half-width: the lower pan base -0.01, top 0.02 + 0.03 (o/h)²; the cover base
	 *  0.03, top 0.045 + 0.05 √(1 - (o/h)²); the upper pan base 0.035, top 0.05 + 0.035 (1 - (o/h)²). */
	inline constexpr double PanBase = -0.010;
	inline constexpr double PanTop = 0.020;
	inline constexpr double PanRise = 0.030;
	inline constexpr double CoverTop = 0.045;
	inline constexpr double CoverRise = 0.050;
	inline constexpr double UpperPanTop = 0.050;
	inline constexpr double UpperPanRise = 0.035;

	/** The 清水脊, u(start) + 0.02 … u(end) - 0.02, (across, up) over the ridge's bed. */
	inline constexpr double RidgeEndInset = 0.020;
	/** Scorpion tails centred 0.35 in from each end: a plate 0.56 × 0.32 × 0.07 at +0.40 … +0.47; the tail swept
	 *  u = end ± (0.10 + 0.55 t), z = ridge + 0.44 + 0.34 t + 0.08 t², t 0 … 1 (9 stations). */
	inline constexpr double TailIn = 0.350;
	inline constexpr double TailPlateLength = 0.560;
	inline constexpr double TailPlateWidth = 0.320;
	inline constexpr double TailPlateBottom = 0.400;
	inline constexpr double TailPlateTop = 0.470;
	inline constexpr int32 TailStations = 9;
	/** The gable edges (排山) run down each hard-gabled slope at the bed + 0.02, 17 stations. */
	inline constexpr double GableEdgeOver = 0.020;
	inline constexpr int32 GableEdgeStations = 17;
	/** The flying rafters' square arrises are sampled every centimetre. */
	inline constexpr double FlyingGrid = 0.010;
	/** Rafters stop 0.10 short of a mitred end (as built). */
	inline constexpr double RafterClearOfMitre = 0.100;
	/** The slab is set out along v every 0.12 at most, and every knot of its bed. */
	inline constexpr double SlabStation = 0.120;
	/** The gable band (博缝): a facing of rubbed brick 0.03 thick on the slab's end over a hard gable. */
	inline constexpr double GableBand = 0.030;

	/** The 清水脊's section (across, up) over the ridge's bed. */
	inline constexpr FSectionPoint QingshuiRidge[] = {{-0.17, -0.08}, {0.17, -0.08}, {0.17, 0.06}, {0.13, 0.08}, {0.13, 0.12}, {0.105, 0.14},
		{0.105, 0.34}, {0.12, 0.36}, {0.12, 0.40}, {0.075, 0.44, true}, {0.0, 0.46, true}, {-0.075, 0.44, true}, {-0.12, 0.40}, {-0.12, 0.36},
		{-0.105, 0.34}, {-0.105, 0.14}, {-0.13, 0.12}, {-0.13, 0.08}, {-0.17, 0.06}};
	/** The 过垄脊's section. */
	inline constexpr FSectionPoint RolledRidge[] = {{-0.16, -0.04}, {0.16, -0.04}, {0.16, 0.02}, {0.10, 0.06, true}, {0.0, 0.075, true},
		{-0.10, 0.06, true}, {-0.16, 0.02}};
	/** The scorpion tail's section: its underside at -0.06, its top (across, up). */
	inline constexpr double TailUnder = -0.060;
	inline constexpr FSectionPoint TailTop[] = {{-0.06, 0.04}, {0.0, 0.07, true}, {0.06, 0.04}};
	/** The gable edge's section: its underside at -0.06, its top (across, up). */
	inline constexpr double GableEdgeUnder = -0.060;
	inline constexpr FSectionPoint GableEdgeTop[] = {{-0.145, 0.05}, {-0.08, 0.11, true}, {0.0, 0.13, true}, {0.08, 0.11, true}, {0.145, 0.05}};

	// ---- §3.5 walls (ChenghuaiHall.cpp:912-1021, ChenghuaiParts.cpp:226-340) ----------------------------------------------
	/** Gables run from v -0.36 to the back wall's outer face, 0.015 up into the boarding. */
	inline constexpr double GableFront = -0.360;
	inline constexpr double GableIntoBoarding = 0.015;
	/** The base (下碱) of rubbed brick, 1.00 high, 15 mm proud outside; a 15 mm lime skin inside a room. */
	inline constexpr double WallBase = 1.000;
	inline constexpr double BaseProud = 0.015;
	inline constexpr double Skin = 0.015;
	/** A framed gable opening: brick over it, a red frame (head 0.10, jambs 0.08). */
	inline constexpr double OpeningHead = 0.100;
	inline constexpr double OpeningJamb = 0.080;
	/** The corbelled front end (墀头): a stone at the base's top (-0.12 … 0, 0.55 deep); five rubbed courses 0.085 high
	 *  from Zc - 0.35, stepping 0.055 k forward and 0.02 k sideways; a sloping board to the eave. */
	inline constexpr double CorbelStone = 0.120;
	inline constexpr double CorbelStoneDeep = 0.550;
	inline constexpr int32 CorbelCourses = 5;
	inline constexpr double CorbelCourse = 0.085;
	inline constexpr double CorbelUnderZc = 0.350;
	inline constexpr double CorbelForward = 0.055;
	inline constexpr double CorbelSideways = 0.020;
	/** The courses reach 0.40 back into the gable, the board 0.30 before the gable's front (as built). */
	inline constexpr double CorbelBack = 0.400;
	inline constexpr double CorbelBoardReach = 0.300;
	/** The board's top meets the eave's underside 0.05 further out than its foot (as built). */
	inline constexpr double CorbelBoardSlope = 0.050;
	/** The back wall's base from floor - 0.05; a sealed eave's cornice (冰盘檐) of four courses 0.07 stepping 0.06 k out. */
	inline constexpr double BackBaseUnder = 0.050;
	inline constexpr int32 CorniceCourses = 4;
	inline constexpr double CorniceCourse = 0.070;
	inline constexpr double CorniceStep = 0.060;
	/** The cornice's courses start 0.10 inside the wall's outer face (as built). */
	inline constexpr double CorniceBack = 0.100;
	/** The cornice stops 0.01 short of the roof's back edge (as built). */
	inline constexpr double CorniceShort = 0.010;

	/** A free-standing wall's tiled coping: a corbel course 0.07 high, T + 0.10 wide; a lime bed (-Hf, 0) (Hf, 0) (Hf, 0.05)
	 *  (0, 0.18) (-Hf, 0.05), Hf = T/2 + 0.14; 合瓦 rows from 0.06 to Hf down both sides; a coping ridge. */
	inline constexpr double CopingCorbel = 0.070;
	inline constexpr double CopingCorbelWider = 0.100;
	inline constexpr double CopingOverhang = 0.140;
	inline constexpr double CopingEaves = 0.050;
	inline constexpr double CopingCrown = 0.180;
	inline constexpr double CopingRowsFrom = 0.060;
	/** The coping's ridge, (across, up) over the bed's crown. */
	inline constexpr FSectionPoint CopingRidge[] = {{-0.08, -0.04}, {0.08, -0.04}, {0.08, 0.06}, {0.05, 0.11, true}, {0.0, 0.125, true},
		{-0.05, 0.11, true}, {-0.08, 0.06}};
	/** A free-standing wall's base course stands 12 mm proud of both faces, from 0.10 under the ground (as built). */
	inline constexpr double WallBaseProud = 0.012;
	inline constexpr double WallFoot = -0.100;

	/** An opening through a free-standing wall. */
	enum class EWallHole : uint8
	{
		/** A door with a red frame, its leaves folded open against the reveals. */
		Door,
		/** A door with a stone surround and no leaves (the garden door). */
		StoneDoor,
		/** The yard's green screen door, its leaves folded open. */
		ScreenDoor,
		/** A leak window (漏窗): a panel with a shaped hole and its lattice. */
		Window,
		/** The moon gate. */
		MoonGate,
	};

	/** The shape and lattice of a leak window (§3.5). */
	enum class ELeak : uint8
	{
		CircleIceCrack,
		Hexagons,
		Coins,
		Octagon,
	};

	/** An opening in a wall: from A0 to A1 along the wall, Z0 … Z1 above its foot. */
	struct FWallHolePlan
	{
		EWallHole Kind;
		double A0, A1, Z0, Z1;
		ELeak Leak;
	};

	/** A free-standing wall's line: along x (y from Y0 to Y1 its thickness) or along y (x from X0 to X1). */
	struct FWallPlan
	{
		const TCHAR* Local;
		bool bAlongX;
		/** Its run: along x from A0 to A1 at y Face0 … Face1, or along y from A0 to A1 at x Face0 … Face1. */
		double A0, A1, Face0, Face1;
		double Height, Base;
		/** The body's two faces: toward -y (or -x) and toward +y (or +x); the base course's surface. */
		const TCHAR* FaceLow;
		const TCHAR* FaceHigh;
		const TCHAR* BaseSurface;
		bool bCoping;
		TArray<FWallHolePlan> Holes;
	};

	/** Leak windows: R = 0.46 × min(w, h); bars 0.028 × 0.06 on the wall's centre plane; a rim 0.05 × 0.07 round the shape;
	 *  ice-crack of 9 cracks, coins of octagon rings Ø 0.24 on a 0.20 grid, hexagons of side 0.11. */
	inline constexpr double LeakRadius = 0.460;
	inline constexpr int32 LeakCircleSides = 48;
	inline constexpr double LeakBar = 0.028;
	inline constexpr double LeakBarDepth = 0.060;
	inline constexpr double LeakRim = 0.050;
	inline constexpr double LeakRimDepth = 0.070;
	inline constexpr int32 LeakCracks = 9;
	inline constexpr double CoinRadius = 0.120;
	inline constexpr double CoinPitch = 0.200;
	inline constexpr double HexagonSide = 0.110;
	/** The lattice's openings are traced on a 5 mm grid, a sixth of a bar. */
	inline constexpr double LeakTraceGrid = 0.005;

	/** The moon gate (月洞门 · 臥遊): a circle R 1.20 about (y -37.30, z 1.25) in the house's east wall, cut flat at z 0.10; a
	 *  ring of rubbed brick 0.13 wide, 0.025 proud of both faces; the stone sill 1.80 wide, 0.08 past both faces, -0.10 … 0.102. */
	inline constexpr double MoonY = -37.300;
	inline constexpr double MoonZ = 1.250;
	inline constexpr double MoonRadius = 1.200;
	inline constexpr double MoonCut = 0.100;
	inline constexpr double MoonRing = 0.130;
	inline constexpr double MoonRingProud = 0.025;
	inline constexpr double MoonSillWidth = 1.800;
	inline constexpr double MoonSillPast = 0.080;
	inline constexpr double MoonSillBottom = -0.100;
	inline constexpr double MoonSillTop = 0.102;
	/** The panel round the moon gate runs 0.22 past the ring each side (as built). */
	inline constexpr double MoonMargin = 0.220;
	inline constexpr int32 MoonStations = 72;

	/** Door thresholds 0.10 wider than their openings each side, 0.06 over the ground, 0.06 past both faces; posts 0.10. */
	inline constexpr double ThresholdWider = 0.100;
	inline constexpr double ThresholdOver = 0.060;
	inline constexpr double ThresholdPast = 0.060;
	inline constexpr double ThresholdBed = 0.080;
	inline constexpr double DoorPost = 0.100;
	inline constexpr double DoorHeadRail = 0.120;
	inline constexpr double DoorLeaf = 0.050;

	/** The house's walls (§3.5's table). */
	inline const TArray<FWallPlan> Walls = {
		{TEXT("wall.west.south"), false, -5.600, -14.000, -20.700, -20.200, 4.200, 0.930, TEXT("brick.pointed"), TEXT("brick.pointed"),
			TEXT("brick.rubbed"), false, {}},
		{TEXT("wall.west.north"), false, -24.400, -39.350, -20.700, -20.200, 4.200, 0.930, TEXT("brick.pointed"), TEXT("brick.pointed"),
			TEXT("brick.rubbed"), false, {}},
		{TEXT("wall.east.south"), false, -5.500, -14.000, 1.800, 2.300, 4.200, 0.930, TEXT("brick.pointed"), TEXT("wall.limewash"),
			TEXT("brick.rubbed"), false,
			{{EWallHole::StoneDoor, -7.500, -9.300, 0.000, 2.350, ELeak::Octagon},
				{EWallHole::Window, -12.200, -13.200, 1.250, 2.250, ELeak::CircleIceCrack}}},
		{TEXT("wall.east.north"), false, -24.400, -39.350, 1.800, 2.300, 4.200, 0.930, TEXT("brick.pointed"), TEXT("wall.limewash"),
			TEXT("brick.rubbed"), false,
			{{EWallHole::Window, -26.400, -27.400, 1.250, 2.250, ELeak::Hexagons},
				{EWallHole::Window, -29.400, -30.400, 1.250, 2.250, ELeak::Coins},
				{EWallHole::Window, -32.400, -33.400, 1.250, 2.250, ELeak::Octagon},
				{EWallHole::MoonGate, -36.100, -38.500, 0.000, 2.450, ELeak::Octagon}}},
		{TEXT("wall.court.west"), true, -20.200, -10.900, -11.500, -11.100, 3.200, 0.930, TEXT("wall.limewash"), TEXT("wall.limewash"),
			TEXT("brick.rubbed"), true, {{EWallHole::Window, -13.600, -12.600, 1.500, 2.500, ELeak::Octagon}}},
		{TEXT("wall.court.east"), true, -7.500, -2.300, -11.500, -11.100, 3.200, 0.930, TEXT("wall.limewash"), TEXT("wall.limewash"),
			TEXT("brick.rubbed"), true, {{EWallHole::Window, -5.800, -4.800, 1.500, 2.500, ELeak::Hexagons}}},
		{TEXT("wall.court.screen"), true, -2.300, 1.800, -11.500, -11.100, 3.200, 0.930, TEXT("brick.rubbed"), TEXT("brick.rubbed"),
			TEXT("brick.rubbed"), true, {}},
		{TEXT("wall.yard"), false, -5.500, -11.100, -2.300, -1.900, 3.200, 0.930, TEXT("brick.rubbed"), TEXT("brick.rubbed"),
			TEXT("brick.rubbed"), true, {{EWallHole::ScreenDoor, -7.500, -9.300, 0.000, 2.350, ELeak::Octagon}}},
		{TEXT("wall.cross.west"), true, -20.200, -17.700, -31.100, -30.700, 3.200, 0.930, TEXT("brick.pointed"), TEXT("brick.pointed"),
			TEXT("brick.rubbed"), true, {{EWallHole::Door, -19.500, -18.400, 0.000, 2.300, ELeak::Octagon}}},
		{TEXT("wall.cross.east"), true, -0.700, 1.800, -31.100, -30.700, 3.200, 0.930, TEXT("brick.pointed"), TEXT("brick.pointed"),
			TEXT("brick.rubbed"), true, {{EWallHole::Door, 0.000, 1.100, 0.000, 2.300, ELeak::Octagon}}},
		{TEXT("wall.garden.north"), true, 2.300, 16.900, -44.900, -44.400, 3.600, 0.550, TEXT("wall.limewash"), TEXT("wall.limewash"),
			TEXT("brick.rubbed"), true,
			{{EWallHole::Window, 3.900, 4.900, 1.150, 2.150, ELeak::Hexagons}, {EWallHole::Window, 15.000, 16.000, 1.150, 2.150, ELeak::Octagon}}},
		{TEXT("wall.garden.east"), false, -0.500, -44.400, 16.400, 16.900, 3.600, 0.550, TEXT("wall.limewash"), TEXT("wall.limewash"),
			TEXT("brick.rubbed"), true,
			{{EWallHole::Window, -21.000, -22.000, 1.150, 2.150, ELeak::CircleIceCrack},
				{EWallHole::Window, -25.600, -26.600, 1.150, 2.150, ELeak::Coins},
				{EWallHole::Window, -37.800, -38.800, 1.150, 2.150, ELeak::Hexagons}}},
		{TEXT("wall.garden.south"), true, 2.300, 16.400, -0.500, 0.000, 3.600, 0.550, TEXT("wall.limewash"), TEXT("wall.limewash"),
			TEXT("brick.rubbed"), true,
			{{EWallHole::Window, 4.400, 5.400, 1.150, 2.150, ELeak::Octagon}, {EWallHole::Window, 14.200, 15.200, 1.150, 2.150, ELeak::CircleIceCrack}}},
	};

	/** The screen wall (影壁 · 澄懷觀道) against the court wall's south face, x ±1.40, face y -10.55, back -11.10: a plinth of
	 *  three stages, a rubbed body to 3.10, the heart 1.80 square centred z 1.80 in a moulded frame 0.10 wide 0.035 proud (its
	 *  face 0.02 back from the frame's), a cornice of four courses 0.07 stepping 0.04, a tiled coping at 3.38 over x ±1.60. */
	inline constexpr double ScreenHalf = 1.400;
	inline constexpr double ScreenFace = -10.550;
	inline constexpr double ScreenBack = -11.100;
	struct FPlinthStage
	{
		double Bottom, Top, Proud;
	};
	inline constexpr FPlinthStage ScreenPlinth[] = {{-0.050, 0.180, 0.080}, {0.180, 0.420, 0.020}, {0.420, 0.550, 0.070}};
	inline constexpr double ScreenBodyTop = 3.100;
	inline constexpr double ScreenHeart = 1.800;
	inline constexpr double ScreenHeartZ = 1.800;
	inline constexpr double ScreenFrame = 0.100;
	inline constexpr double ScreenFrameProud = 0.035;
	inline constexpr double ScreenHeartBack = 0.020;
	inline constexpr double ScreenCorniceStep = 0.040;
	inline constexpr double ScreenCopingZ = 3.380;
	inline constexpr double ScreenCopingHalf = 1.600;

	// ---- §3.6 fronts (装修) (ChenghuaiHall.cpp:278-418, 1024-1119) ----------------------------------------------------------
	/** The frame (槛框): posts 0.10 against the columns, a head rail 0.10 under the lintel, 0.10 thick; in a veranda the column
	 *  is 0.015 thicker a side. */
	inline constexpr double FramePost = 0.100;
	inline constexpr double FrameThick = 0.100;
	inline constexpr double VerandaColumnPast = 0.015;
	/** Doors: a sill 0.10 high, 0.12 thick; the door top max(min(floor + 2.55, head - 0.45), floor + 2.05); a transom when 0.18
	 *  or more is left over a 0.10 door head; transom panels ≈ 0.95 wide between 0.06 mullions; four leaves in a bay wider
	 *  than 2.2, else two. */
	inline constexpr double DoorSill = 0.100;
	inline constexpr double DoorSillThick = 0.120;
	inline constexpr double DoorTopMost = 2.550;
	inline constexpr double DoorUnderHead = 0.450;
	inline constexpr double DoorTopLeast = 2.050;
	inline constexpr double TransomLeast = 0.180;
	inline constexpr double DoorHead = 0.100;
	inline constexpr double TransomPanel = 0.950;
	inline constexpr double TransomMullion = 0.060;
	inline constexpr double FourLeafBay = 2.200;
	/** A leaf: stiles 0.065, 0.065 thick; rails 0.06 at the foot, 0.30 H, 0.39 H, 0.92 H and the head; the apron's raised field
	 *  0.07 in; the panels set back 0.01 in 0.025 boards (as built); lattice over paper between the waist and top rails. */
	inline constexpr double LeafStile = 0.065;
	inline constexpr double LeafThick = 0.065;
	inline constexpr double LeafRail = 0.060;
	inline constexpr double LeafApron = 0.300;
	inline constexpr double LeafWaist = 0.390;
	inline constexpr double LeafTopRail = 0.920;
	inline constexpr double LeafField = 0.070;
	inline constexpr double LeafPanelBack = 0.010;
	inline constexpr double LeafPanel = 0.025;
	inline constexpr double LeafFieldProud = 0.008;
	/** Windows (支摘窗): a rubbed sill wall 0.34 thick to floor + 0.79, its board 0.40 deep to floor + 0.85; a window sill 0.08;
	 *  a middle post 0.08; each half split at 0.42 of its height, the lower sash a plain grid. */
	inline constexpr double SillWall = 0.340;
	inline constexpr double SillWallTop = 0.790;
	inline constexpr double SillBoard = 0.400;
	inline constexpr double SillBoardTop = 0.850;
	inline constexpr double WindowSill = 0.080;
	inline constexpr double WindowPost = 0.080;
	inline constexpr double WindowSplit = 0.420;
	inline constexpr double WindowRail = 0.060;
	/** Rails 0.09 deep, posts and window sills 0.10, the door head and sill 0.12: set in the frame's 0.10 (as built). */
	inline constexpr double WindowRailDeep = 0.090;
	/** A panel stands 5 mm clear of the rails, posts and sills round it, and 1 cm in from its transom's mullions (as built). */
	inline constexpr double PanelClear = 0.005;
	inline constexpr double TransomPanelClear = 0.010;
	/** A leaf hangs 3 mm clear of its neighbours; an open one swings about a hinge 0.04 behind the frame's face (as built). */
	inline constexpr double LeafClear = 0.003;
	inline constexpr double LeafHinge = 0.040;
	/** A walled bay (the sealed end of a front): the wall 0.37 thick, its base 0.40 (as built). */
	inline constexpr double BayWall = 0.370;
	inline constexpr double BayWallBase = 0.400;
	/** Lattice panels: a rim 0.030, bars 0.018 × 0.024; paper 6 mm behind the bars on the room's side; grid cells ≈ 0.13 ×
	 *  0.15; the lantern's margin min(0.11, 0.18 × the short side); ice-crack cells of ≈ 0.011 m². */
	inline constexpr double LatticeRim = 0.030;
	inline constexpr double LatticeBar = 0.018;
	inline constexpr double LatticeDepth = 0.024;
	inline constexpr double Paper = 0.006;
	inline constexpr double GridCellWide = 0.130;
	inline constexpr double GridCellHigh = 0.150;
	inline constexpr double LanternMargin = 0.110;
	inline constexpr double LanternMarginShare = 0.180;
	/** Ties across the lantern's margin every 0.16; a light cross in its open centre when wider than 0.25. */
	inline constexpr double LanternTie = 0.160;
	inline constexpr double LanternCross = 0.250;
	inline constexpr double IceCellArea = 0.011;
	inline constexpr double IceCellLeast = 0.0035;
	inline constexpr double IceCutLeast = 0.050;
	/** The hanging fretwork (挂落) under a gallery's lintel on the court side, 0.30 high, set 0.12 in from each column. */
	inline constexpr double Fretwork = 0.300;
	inline constexpr double FretworkIn = 0.120;

	// ---- §3.7 the gates (ChenghuaiHouse.cpp:268-438) -----------------------------------------------------------------------
	/** The gate (广亮大门) in its frame (origin (-2.30, -0.30), u east, v north); floor 0.90 of brick on edge. */
	inline constexpr double GateColumnsU[] = {0.800, 3.800};
	inline constexpr double GateColumnsV[] = {0.000, 4.900};
	inline constexpr double GateColumnRadius = 0.150;
	inline constexpr double GateColumnTop = 0.145;
	inline constexpr double GateBase = 0.560;
	/** The middle columns (中柱) at v 2.45, r 0.16 → 0.155, to Seat[2] - 0.30. */
	inline constexpr double GateMiddleV = 2.450;
	inline constexpr double GateMiddleRadius = 0.160;
	inline constexpr double GateMiddleTop = 0.155;
	inline constexpr double GateMiddleUnder = 0.300;
	/** Beams across at the columns, v -0.2 … 2.45 and 2.45 … 5.1, 0.26 × 0.34 at Seat[0] - 0.28; struts 0.18 square. */
	inline constexpr double GateBeamFront = -0.200;
	inline constexpr double GateBeamBack = 5.100;
	inline constexpr double GateBeamWidth = 0.260;
	inline constexpr double GateBeamHeight = 0.340;
	inline constexpr double GateBeamUnder = 0.280;
	inline constexpr double GateStrut = 0.180;
	/** The struts stand on the beams at the purlins between, v 1.225 and 3.675, to 0.20 under their purlin's seat (as built). */
	inline constexpr double GateStrutsV[] = {1.225, 3.675};
	inline constexpr double GateStrutUnder = 0.200;
	/** The door frame on the ridge line (y -2.75): clear 1.90; posts 0.12 × 0.18 to the head at floor + 2.75; the sill 0.20;
	 *  a middle rail x ±1.35 at 2.75 … 2.95; a top rail under Seat[2] - 0.35 (0.15 deep); the panels over and beside 0.06. */
	inline constexpr double GateDoorClear = 1.900;
	inline constexpr double GateDoorPost = 0.120;
	inline constexpr double GateDoorPostDeep = 0.180;
	inline constexpr double GateDoorHead = 2.750;
	inline constexpr double GateSill = 0.200;
	inline constexpr double GateSillDeep = 0.200;
	inline constexpr double GateRailHalf = 1.350;
	inline constexpr double GateRail = 0.200;
	inline constexpr double GateTopRailUnder = 0.350;
	inline constexpr double GateTopRail = 0.150;
	inline constexpr double GatePanel = 0.060;
	/** Four plain door pins (门簪) on the middle rail at x -0.57, -0.19, 0.19, 0.57, z floor + 2.85, pointing south. */
	inline constexpr double GatePinsX[] = {-0.570, -0.190, 0.190, 0.570};
	inline constexpr double GatePinsZ = 2.850;
	/** A door pin, turned (r, along its axis). */
	inline constexpr FSectionPoint PinSection[] = {{0.0, 0.0}, {0.075, 0.0}, {0.075, 0.10}, {0.06, 0.12, true}, {0.0, 0.125}};
	/** The leaves, 0.95 × 0.08, floor + 0.21 … head - 0.02, open north against the side panels; knocker plates at floor + 1.35
	 *  with rings (a torus R 0.07, tube 0.009) 0.07 lower. */
	inline constexpr double GateLeaf = 0.950;
	inline constexpr double GateLeafThick = 0.080;
	inline constexpr double GateLeafFoot = 0.210;
	inline constexpr double GateLeafHead = 0.020;
	inline constexpr double GateLeafFromRail = 0.120;
	inline constexpr double KnockerZ = 1.350;
	/** The knocker plates stand at three quarters of the leaf's width from its hinge (as built). */
	inline constexpr double KnockerAlong = 0.750;
	inline constexpr double RingRadius = 0.070;
	inline constexpr double RingTube = 0.009;
	inline constexpr double RingDrop = 0.070;
	inline constexpr double RingOut = 0.030;
	/** A knocker's plate (门钹), turned (r, out from the leaf). */
	inline constexpr FSectionPoint KnockerSection[] = {{0.0, 0.0}, {0.11, 0.0}, {0.11, 0.006}, {0.05, 0.03, true}, {0.0, 0.035}};
	/** The stone drums (门墩) at x ±1.25: a base 0.34 × 1.10 (y -2.00 … -3.10) floor - 0.05 … + 0.28; a box 0.30 × 0.60 to
	 *  + 0.70; a lion's block 0.20 × 0.40 to + 0.86. */
	inline constexpr double DrumX = 1.250;
	inline constexpr double DrumBaseWide = 0.340;
	inline constexpr double DrumBaseY0 = -2.000;
	inline constexpr double DrumBaseY1 = -3.100;
	inline constexpr double DrumBaseTop = 0.280;
	inline constexpr double DrumBoxWide = 0.300;
	inline constexpr double DrumBoxLong = 0.600;
	inline constexpr double DrumBoxTop = 0.700;
	inline constexpr double DrumLionWide = 0.200;
	inline constexpr double DrumLionLong = 0.400;
	inline constexpr double DrumLionTop = 0.860;
	/** The box and the lion's block stand from y -2.03 and -2.13 northward (as built). */
	inline constexpr double DrumBoxY0 = -2.030;
	inline constexpr double DrumLionY0 = -2.130;

	/** The festooned gate (垂花门 · 齋莊中正): a platform x -10.90 … -7.50, y -9.90 … -12.60, floor 0.45, kerb 0.30. */
	inline constexpr double ChuiX0 = -10.900;
	inline constexpr double ChuiX1 = -7.500;
	inline constexpr double ChuiY0 = -9.900;
	inline constexpr double ChuiY1 = -12.600;
	inline constexpr double ChuiFloor = 0.450;
	inline constexpr double ChuiKerb = 0.300;
	/** Columns at x -10.60, -7.80 on the wall line (y -11.30) and the back line (y -12.40), r 0.125 → 0.12, bases 0.44. */
	inline constexpr double ChuiColumnsX[] = {-10.600, -7.800};
	inline constexpr double ChuiWallY = -11.300;
	inline constexpr double ChuiBackY = -12.400;
	inline constexpr double ChuiColumnRadius = 0.125;
	inline constexpr double ChuiColumnTop = 0.120;
	inline constexpr double ChuiBase = 0.440;
	/** Columns run to 0.30 under their purlin's seat (as built). */
	inline constexpr double ChuiColumnUnder = 0.300;
	/** Cantilever beams (麻叶抱头梁) 0.20 × 0.30 from y -12.50 to -9.95 at z 3.106; hanging posts (垂柱) r 0.10 at y -10.20 from
	 *  3.256 down to 2.70, each ending in a gilt lotus bud from z 2.40. */
	inline constexpr double ChuiBeamY0 = -12.500;
	inline constexpr double ChuiBeamY1 = -9.950;
	inline constexpr double ChuiBeamZ = 3.106;
	inline constexpr double ChuiBeamWide = 0.200;
	inline constexpr double ChuiBeamHigh = 0.300;
	inline constexpr double ChuiPostY = -10.200;
	inline constexpr double ChuiPostRadius = 0.100;
	inline constexpr double ChuiPostTop = 3.256;
	inline constexpr double ChuiPostFoot = 2.700;
	inline constexpr double ChuiBudZ = 2.400;
	/** The lotus bud (垂头), turned (r, z over its foot), its swelling smooth. */
	inline constexpr FSectionPoint BudSection[] = {{0.0, 0.0}, {0.05, 0.02, true}, {0.11, 0.10, true}, {0.13, 0.19, true}, {0.12, 0.26, true},
		{0.10, 0.30}, {0.0, 0.30}};
	/** A step-brocade band 0.30 high between the hanging posts, its bottom at 2.514. */
	inline constexpr double ChuiBandBottom = 2.514;
	/** The door frame on the wall line: posts 0.12 at x -10.48 … -10.36 and -8.04 … -7.92, 0.14 thick; head 2.674 … 2.816;
	 *  sill 0.12. */
	inline constexpr double ChuiDoorPosts[][2] = {{-10.480, -10.360}, {-8.040, -7.920}};
	inline constexpr double ChuiDoorThick = 0.140;
	inline constexpr double ChuiDoorHead0 = 2.674;
	inline constexpr double ChuiDoorHead1 = 2.816;
	inline constexpr double ChuiSill = 0.120;
	/** The green screen door (屏门), shut on y -12.30: four panels 0.61 wide from x -10.46 at 0.63 pitch, 0.06 thick, 0.58 …
	 *  2.654; a red disc Ø 0.32 at z 2.20 on each; the screen's head, board and sill. */
	inline constexpr double ChuiScreenY = -12.300;
	inline constexpr double ChuiScreenX0 = -10.460;
	inline constexpr double ChuiScreenPanel = 0.610;
	inline constexpr double ChuiScreenPitch = 0.630;
	inline constexpr double ChuiScreenThick = 0.060;
	inline constexpr double ChuiScreenFoot = 0.580;
	inline constexpr double ChuiScreenTop = 2.654;
	inline constexpr double ChuiDiscRadius = 0.160;
	inline constexpr double ChuiDiscZ = 2.200;
	inline constexpr double ChuiDiscThick = 0.006;
	/** The screen's head 0.14 high and 0.12 deep over the panels, its board 0.05 to the lintel, its sill 0.12 deep (as built). */
	inline constexpr double ChuiScreenHead = 0.140;
	inline constexpr double ChuiScreenHeadDeep = 0.120;
	inline constexpr double ChuiScreenBoard = 0.050;

	// ---- §3.8 courts and paving (ChenghuaiHouse.cpp:535-576) ---------------------------------------------------------------
	/** Paving 0.12 deep, cut round every platform's footing; the courts' paving at 0.020, the paths of brick on edge at 0.026. */
	inline constexpr double PavingDepth = 0.120;
	inline constexpr double CourtLevel = 0.020;
	inline constexpr double PathLevel = 0.026;
	/** A beds' kerb 0.14 wide to 0.14, soil 0.08; a pit's 0.12 wide to 0.12, soil 0.05; kerbs bedded to -0.05 (as built). */
	inline constexpr double BedKerb = 0.140;
	inline constexpr double BedKerbTop = 0.140;
	inline constexpr double PitKerb = 0.120;
	inline constexpr double PitKerbTop = 0.120;
	inline constexpr double KerbBed = -0.050;
	inline constexpr double PitSide = 1.400;
	inline constexpr FPlanRect CourtPaths[] = {
		{-19.600, -2.300, -7.500, -9.300},
		{-10.100, -8.300, -7.500, -11.100},
		{-10.100, -8.300, -12.600, -25.300},
		{-14.600, -3.800, -18.300, -20.100},
		{-20.200, 1.800, -36.700, -38.000},
	};
	inline constexpr FPlanRect InnerBeds[] = {
		{-13.800, -10.600, -20.500, -24.700},
		{-7.800, -4.600, -20.500, -24.700},
		{-13.800, -10.600, -13.700, -17.900},
		{-7.800, -4.600, -13.700, -17.900},
	};
	inline constexpr FPlanPoint Pits[] = {{-16.800, -35.300}, {-1.600, -35.300}};
	/** The court's fish bowl (鱼缸) on the crossing, foot on the path top, and the two tubs flanking the main hall's steps. */
	inline constexpr double BowlX = -9.200;
	inline constexpr double BowlY = -19.200;
	inline constexpr FPlanPoint Tubs[] = {{-11.600, -25.600}, {-6.800, -25.600}};
	/** The bowl (Ø 1.17, h 0.668, a rolled rim) and a tub (Ø 0.63, h 0.46), turned (r, z over the foot): wall and floor in one. */
	inline constexpr FSectionPoint BowlSection[] = {{0.0, 0.01}, {0.34, 0.01}, {0.36, 0.0}, {0.40, 0.0}, {0.42, 0.04}, {0.50, 0.20}, {0.55, 0.42},
		{0.56, 0.58}, {0.575, 0.63}, {0.585, 0.655}, {0.57, 0.668}, {0.545, 0.66}, {0.535, 0.60}, {0.52, 0.42}, {0.46, 0.18}, {0.36, 0.06},
		{0.0, 0.05}};
	inline constexpr FSectionPoint TubSection[] = {{0.0, 0.01}, {0.20, 0.01}, {0.21, 0.0}, {0.24, 0.0}, {0.25, 0.03}, {0.28, 0.20}, {0.305, 0.40},
		{0.315, 0.445}, {0.31, 0.46}, {0.29, 0.455}, {0.285, 0.40}, {0.26, 0.22}, {0.23, 0.06}, {0.0, 0.05}};

	/** A run of the galleries' fretwork (挂落) on their court side, from (X0, Y0) to (X1, Y1) between columns (the west side). */
	struct FPlanRun
	{
		double X0, Y0, X1, Y1;
	};
	inline constexpr FPlanRun GalleryFretwork[] = {{-15.000, -12.900, -12.900, -12.900}, {-12.900, -12.900, -11.100, -12.900},
		{-15.000, -13.900, -15.000, -12.900}, {-15.000, -26.800, -15.000, -24.500}};

	// ---- §3.11 stock -------------------------------------------------------------------------------------------------------
	/** Court paving 0.40 square (the paving's joints are Matter's working, square.ground-joints). */
	inline constexpr double CourtStone = 0.400;
}
