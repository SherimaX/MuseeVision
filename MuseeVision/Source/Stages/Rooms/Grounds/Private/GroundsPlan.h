#pragma once

#include "CoreMinimal.h"

/**
 * The Grounds' plan: every number the stage is built to, in metres in its frame (the museum's plan frame: origin under
 * the gilt sun at the Rotunda's centre, z 0 the ground floor's finished level; x east, y south, z up), taken from its
 * design sheet, Docs/Stages/grounds.md (the section is given with each group). The sheet's own sources are the as-built
 * grounds (windows/, D-017) and the finished sheets of the buildings it meets. Finishes, light and planting are the
 * schedules' (Data/Stages/grounds), never here. Plan angles α run from +x towards +y (east 0°, south 90°).
 */
namespace Musee::Grounds::Plan
{
	/** The stage's key (§1). */
	inline constexpr const TCHAR* Stage = TEXT("grounds");

	/** A point in plan, (x east, y south). */
	struct FPlanPoint
	{
		double X = 0.0;
		double Y = 0.0;
	};

	// ---- §1 the footprint and the levels --------------------------------------------------------------------------------
	/** The as-built ground's 800 m square (MuseeGround.h:39): x and y from -400 to +400. */
	inline constexpr double FootprintHalf = 400.000;
	/** The soil's bottom, face to face with the Reserve's top, and the tallest birch (12.0) with its sway. */
	inline constexpr double SoilBottom = -0.330;
	inline constexpr double StageTop = 13.000;
	/** The lawn's soil top; the gravel walks', the strip's, the parvis's and the kerbs' and strip edge's tops. */
	inline constexpr double LawnTop = -0.030;
	inline constexpr double GravelTop = 0.018;
	inline constexpr double StripTop = 0.012;
	inline constexpr double ParvisTop = 0.024;
	/** Still water's level, and the basin floors' top. */
	inline constexpr double WaterLevel = 0.300;
	inline constexpr double BasinFloorTop = -0.250;

	// ---- §3.1 setting-out: the front ------------------------------------------------------------------------------------
	/** The front's axis, the portico's (the Salon's mid-length). */
	inline constexpr double AxisX = -44.000;
	/** The parvis x -60 ... -28, from the Salon strip's edge (y 9.16) to y 23.00; the portico's first nosing at y 17.30. */
	inline constexpr double ParvisWestX = -60.000;
	inline constexpr double ParvisEastX = -28.000;
	inline constexpr double SalonStripEdgeY = 9.160;
	inline constexpr double ParvisSouthY = 23.000;
	inline constexpr double PorticoNosingY = 17.300;
	/** The cour x -64 ... -24 (with the side walks), to y 62; the cross walk's south line. */
	inline constexpr double CourWestX = -64.000;
	inline constexpr double CourEastX = -24.000;
	inline constexpr double CourSouthY = 62.000;
	/** The canal's copings' outer faces x -48.50 ... -39.50, y 27.00 ... 57.00. */
	inline constexpr double CanalWestX = -48.500;
	inline constexpr double CanalEastX = -39.500;
	inline constexpr double CanalNorthY = 27.000;
	inline constexpr double CanalSouthY = 57.000;
	/** The lawn panels' kerb centre lines: |x + 44| 7.50 ... 14.00, y 27.00 ... 55.00. */
	inline constexpr double PanelInner = 7.500;
	inline constexpr double PanelOuter = 14.000;
	inline constexpr double PanelNorthY = 27.000;
	inline constexpr double PanelSouthY = 55.000;
	/** The avenue x -48 ... -40 from the cross walk to y 130; its south end meets lawn with no kerb (as built). */
	inline constexpr double AvenueWestX = -48.000;
	inline constexpr double AvenueEastX = -40.000;
	inline constexpr double AvenueSouthY = 130.000;

	// ---- §3.1 setting-out: the walk east, the rond-point, the path north and the terrace ----------------------------------
	/** The walk east, 4.00 wide: y 40 ... 44 from the cour, bending round Albion on x -/+19.50 and y 60.00 (centre lines). */
	inline constexpr double WalkInnerY = 40.000;
	inline constexpr double WalkOuterY = 44.000;
	inline constexpr double WalkBendInnerX = 17.500;
	inline constexpr double WalkBendOuterX = 21.500;
	inline constexpr double WalkSouthInnerY = 58.000;
	inline constexpr double WalkSouthOuterY = 62.000;
	/** The rond-point about (54, 42) on the Élan's meridian: the basin's outer face r 3.00, the ring's kerb line r 7.00. */
	inline constexpr FPlanPoint RondPoint{54.000, 42.000};
	inline constexpr double BasinOuterRadius = 3.000;
	inline constexpr double RingRadius = 7.000;
	/** The path north x 52 ... 56, from the terrace's outer arc to the ring's arc. */
	inline constexpr double PathWestX = 52.000;
	inline constexpr double PathEastX = 56.000;
	/** The Élan's terrace: the annulus about (54, 0) r 15.32 ... 19.40, α -125° -> +125° through east. */
	inline constexpr FPlanPoint ElanAxis{54.000, 0.000};
	inline constexpr double TerraceInnerRadius = 15.320;
	inline constexpr double TerraceOuterRadius = 19.400;
	inline constexpr double TerraceEndDeg = 125.000;
	/** The terrace's radial end kerbs start 0.05 out from its inner arc. */
	inline constexpr double TerraceEndKerbFrom = 15.370;

	// ---- §3.3 the parvis's paving -----------------------------------------------------------------------------------------
	/** 17 courses east-west, each 0.8141 deep from y 9.16; slabs 1.1852 long, 27 to a full course; 6 mm joints (pitches). */
	inline constexpr int32 ParvisCourses = 17;
	inline constexpr double CourseDepth = 0.8141;
	inline constexpr int32 SlabsPerCourse = 27;
	inline constexpr double SlabLength = 1.1852;
	inline constexpr double PavingJoint = 0.006;
	/** The slab's section (as built): -0.120 to +0.024. */
	inline constexpr double PavingBottom = -0.120;

	// ---- §3.4 gravel ----------------------------------------------------------------------------------------------------
	/** Every walk's gravel: top +0.018, bottom -0.120. */
	inline constexpr double GravelBottom = -0.120;

	// ---- §3.5 kerbs and the strip ---------------------------------------------------------------------------------------
	/** The kerb's closed section (A across, centred on its line; B up): 0.12 wide, top +0.045, 15 mm chamfers. */
	inline constexpr FPlanPoint KerbSection[] = {
		{-0.060, -0.160}, {0.060, -0.160}, {0.060, 0.030}, {0.045, 0.045}, {-0.045, 0.045}, {-0.060, 0.030}};
	inline constexpr double KerbHalf = 0.060;
	inline constexpr double KerbStone = 1.200;

	/** The strip round the stone complex: gravel from the wall face to d 1.44, top +0.012, bottom -0.100. */
	inline constexpr double StripGravel = 1.440;
	inline constexpr double StripBottom = -0.100;
	/** The strip's stone edge (d, z) off the wall face: d 1.44 ... 1.56, top +0.045. */
	inline constexpr FPlanPoint StripEdgeSection[] = {
		{1.440, -0.140}, {1.560, -0.140}, {1.560, 0.035}, {1.550, 0.045}, {1.450, 0.045}, {1.440, 0.035}};
	inline constexpr double StripEdgeOuter = 1.560;

	// ---- §1 the received outlines the strip is laid from (the other stages' numbers, museum frame) ------------------------
	/** The Rotunda's drum. */
	inline constexpr double DrumRadius = 11.200;
	/** The Salon x -74.60 ... -13.40, |y| <= 7.60, and its Manet cabinet on its north face. */
	inline constexpr double SalonWestX = -74.600;
	inline constexpr double SalonEastX = -13.400;
	inline constexpr double SalonHalf = 7.600;
	inline constexpr double CabinetWestX = -24.400;
	inline constexpr double CabinetEastX = -15.600;
	inline constexpr double CabinetNorthY = -16.000;
	/** Chenghuai's west wall and its south wall (x -20.70 ... 16.90, y -61.90 ... -17.00); its porch |x| <= 2.95. */
	inline constexpr double ChenghuaiWestX = -20.700;
	inline constexpr double ChenghuaiSouthY = -17.000;
	inline constexpr double ChenghuaiPorchHalf = 2.950;
	/** Albion's porch |x| <= 3.70 from y 11.20; the Hall of Light's west edge x 10.90; the Salon passage's |y| <= 1.80. */
	inline constexpr double AlbionPorchHalf = 3.700;
	inline constexpr double HallOfLightWestX = 10.900;
	inline constexpr double SalonPassageHalf = 1.800;
	/** The Nymphéas oval: its inner ellipse 11.00 x 7.50 about (-86.50, 0), its wall's outer face 0.60 off it; the passage block |y| <= 2.10. */
	inline constexpr FPlanPoint OvalCentre{-86.500, 0.000};
	inline constexpr double OvalA = 11.000;
	inline constexpr double OvalB = 7.500;
	inline constexpr double OvalWall = 0.600;
	inline constexpr double PassageBlockHalf = 2.100;
	/** The oval's strip is set out at 720 even steps of the ellipse's parameter, as the oval itself is (nympheas.md §1). */
	inline constexpr int32 OvalStations = 720;

	// ---- §3.6 the basins ------------------------------------------------------------------------------------------------
	/** The coping's closed section (A out from the nose line, away from the water; B up), 0.50 wide, top at sitting height. */
	inline constexpr FPlanPoint CopingFoot[] = {{0.040, -0.310}, {0.500, -0.310}, {0.500, 0.400}};
	inline constexpr FPlanPoint CopingOuterArc{0.450, 0.400};
	inline constexpr double CopingOuterArcRadius = 0.050;
	inline constexpr FPlanPoint CopingSeatInner{0.030, 0.450};
	inline constexpr FPlanPoint CopingNoseArc{0.030, 0.420};
	inline constexpr double CopingNoseArcRadius = 0.030;
	inline constexpr FPlanPoint CopingUnderside[] = {{0.000, 0.360}, {0.040, 0.360}};
	inline constexpr double CopingWidth = 0.500;
	/** The wetted face, 40 mm behind the nose: the water meets it. */
	inline constexpr double WettedFace = 0.040;
	inline constexpr double CopingTop = 0.450;
	inline constexpr double CopingBottom = -0.310;
	/** The canal's nose line x -48.00 ... -40.00, y 27.50 ... 56.50; its stones 1.25 long, mitred at the corners. */
	inline constexpr double CopingStone = 1.250;
	/** The round basin's nose r 2.50, in 12 radially jointed stones. */
	inline constexpr double BasinNoseRadius = 2.500;
	inline constexpr int32 BasinStones = 12;

	// ---- §3.7 the lamp standards ----------------------------------------------------------------------------------------
	/** A vertex of a turned section (r, z from the foot); bSmooth marks the sheet's smooth vertices. */
	struct FTurnPoint
	{
		double R = 0.0;
		double Z = 0.0;
		bool bSmooth = false;
	};

	/** The post: a moulded base Ø 0.54, a shaft tapering Ø 0.17 -> 0.124, a collar and a cup with a flat top at 3.41. */
	inline constexpr FTurnPoint PostSection[] = {{0.000, 0.000}, {0.270, 0.000}, {0.270, 0.070}, {0.230, 0.110}, {0.210, 0.400, true},
		{0.170, 0.460, true}, {0.120, 0.520, true}, {0.100, 0.600}, {0.085, 0.700}, {0.062, 3.050}, {0.095, 3.100}, {0.095, 3.170},
		{0.060, 3.210}, {0.070, 3.300, true}, {0.150, 3.380}, {0.160, 3.410}, {0.000, 3.410}};
	inline constexpr double PostBaseRadius = 0.270;
	inline constexpr double CupTop = 3.410;
	/** The globe: a sphere r 0.24 about z 3.62, truncated at the cup's top, of 3 mm cased-opal glass. */
	inline constexpr double GlobeCentreZ = 3.620;
	inline constexpr double GlobeRadius = 0.240;
	inline constexpr double GlobeGlass = 0.003;
	/** The finial, seated on the globe's crown (its foot r 0.09); overall 4.01. */
	inline constexpr FTurnPoint FinialSection[] = {{0.090, 3.842}, {0.060, 3.910}, {0.025, 3.940}, {0.035, 3.980}, {0.000, 4.010}};

	/** The ten standards: eight on the canal walks (axis +/- 4.95, the base 0.18 off the coping), two at the parvis's corners. */
	inline constexpr FPlanPoint Lamps[] = {{-48.950, 29.000}, {-39.050, 29.000}, {-48.950, 37.000}, {-39.050, 37.000}, {-48.950, 45.000},
		{-39.050, 45.000}, {-48.950, 53.000}, {-39.050, 53.000}, {-58.500, 21.500}, {-29.500, 21.500}};

	// ---- §1 seats on the copings ----------------------------------------------------------------------------------------
	/** A seat every 0.80 on each coping's seat line (d 0.25 in from its outer face), none within 0.60 of a corner or a base. */
	inline constexpr double SeatLineIn = 0.250;
	inline constexpr double SeatPitch = 0.800;
	inline constexpr double SeatClear = 0.600;
	inline constexpr double BasinSeatRadius = 2.750;

	// ---- §3.8, §7 the plants' beds (Nature grows them from Planting.json) -------------------------------------------------
	/** The box loops' centre lines, 0.45 inside each panel's kerb (§3.8). */
	inline constexpr FPlanPoint LoopWest[] = {{-51.950, 27.450}, {-57.550, 27.450}, {-57.550, 54.550}, {-51.950, 54.550}};
	inline constexpr FPlanPoint LoopEast[] = {{-36.050, 27.450}, {-30.450, 27.450}, {-30.450, 54.550}, {-36.050, 54.550}};
	/** The box balls, four to a panel, 0.9 in from the loop's corner lines. */
	inline constexpr FPlanPoint Balls[] = {{-52.850, 28.350}, {-56.650, 28.350}, {-52.850, 53.650}, {-56.650, 53.650}, {-35.150, 28.350},
		{-31.350, 28.350}, {-35.150, 53.650}, {-31.350, 53.650}};
	/** The yew exedra's arcs r 14.00 about (-44, 62). */
	inline constexpr FPlanPoint ExedraCentre{-44.000, 62.000};
	inline constexpr double ExedraRadius = 14.000;

	/** A tree's bed: its key under the stage, its root flare's plan point, and whether it stands in the gravel (§7.1). */
	struct FTree
	{
		const TCHAR* Local = nullptr;
		double X = 0.0;
		double Y = 0.0;
		bool bInGravel = false;
	};

	/** 20 birches and 17 osmanthus (§7.1). */
	inline constexpr FTree Trees[] = {
		{TEXT("allee.w.01"), -62.0, 26.0, true}, {TEXT("allee.w.02"), -62.0, 32.0, true}, {TEXT("allee.w.03"), -62.0, 38.0, true},
		{TEXT("allee.w.04"), -62.0, 44.0, true}, {TEXT("allee.w.05"), -62.0, 50.0, true}, {TEXT("allee.w.06"), -62.0, 56.0, true},
		{TEXT("allee.e.01"), -26.0, 26.0, true}, {TEXT("allee.e.02"), -26.0, 32.0, true}, {TEXT("allee.e.03"), -26.0, 38.0, true},
		{TEXT("allee.e.04"), -26.0, 44.0, true}, {TEXT("allee.e.05"), -26.0, 50.0, true}, {TEXT("allee.e.06"), -26.0, 56.0, true},
		{TEXT("avenue.w.01"), -50.5, 80.0}, {TEXT("avenue.w.02"), -50.5, 92.0}, {TEXT("avenue.w.03"), -50.5, 104.0},
		{TEXT("avenue.w.04"), -50.5, 116.0}, {TEXT("avenue.e.01"), -37.5, 80.0}, {TEXT("avenue.e.02"), -37.5, 92.0},
		{TEXT("avenue.e.03"), -37.5, 104.0}, {TEXT("avenue.e.04"), -37.5, 116.0},
		{TEXT("walk.s.01"), -12.0, 63.8}, {TEXT("walk.s.02"), 3.0, 63.8}, {TEXT("walk.s.03"), 18.0, 63.8},
		{TEXT("walk.n.04"), 33.0, 38.2}, {TEXT("walk.s.04"), 33.0, 45.8},
		{TEXT("grove.01"), -40.0, -44.0}, {TEXT("grove.02"), -33.0, -49.0}, {TEXT("grove.03"), -47.0, -51.0},
		{TEXT("grove.04"), -112.0, -8.0}, {TEXT("grove.05"), -115.0, 7.0}, {TEXT("grove.06"), -92.0, 44.0},
		{TEXT("grove.07"), -98.0, 56.0}, {TEXT("grove.08"), 88.0, -18.0}, {TEXT("grove.09"), 92.0, 10.0},
		{TEXT("grove.10"), 30.0, 66.0}, {TEXT("grove.11"), -20.5, 18.0}, {TEXT("grove.12"), -20.5, 30.5}};
}
