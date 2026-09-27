#pragma once

#include "CoreMinimal.h"

/**
 * The Nymphéas oval's plan: every number the room is built to, in metres in the stage's frame (origin at the oval's
 * centre on the finished floor; x east, y south, z up), taken from its design sheet, Docs/Stages/nympheas.md (the
 * section is given with each group). The sheet's own sources are the as-built oval (windows/, D-017) and the approved
 * boards (D-019). Finishes, light, planting, hanging and motion are the schedules' (Data/Stages/nympheas), never here.
 */
namespace Musee::Nympheas::Plan
{
	/** The stage's key and its name for visitors (§1). */
	inline constexpr const TCHAR* Stage = TEXT("nympheas");

	// ---- §3.1 the oval: its inner face, the ellipse a × b about the origin, and its wall -----------------------------
	inline constexpr double OvalA = 11.000;
	inline constexpr double OvalB = 7.500;
	inline constexpr double WallMeters = 0.600;
	/** The plaster face runs from the floor to the cove's foot. */
	inline constexpr double CoveFoot = 5.000;
	/** The roof ring's slab (§3.3), which the wall carries: its soffit and its top. */
	inline constexpr double RingSoffit = 7.000;
	inline constexpr double RingTop = 7.200;
	/** The curves round the oval are built at 720 stations of the ellipse's parameter (§1). */
	inline constexpr int32 OvalStations = 720;

	// ---- §3.1 the door at the east apex and the passage to the port face ----------------------------------------------
	inline constexpr double DoorWidth = 3.000;
	inline constexpr double DoorSpringing = 3.300;
	inline constexpr double PortFaceX = 11.900;
	/** The passage block fills |y| ≤ 2.100 between the port face and the oval's outer face, floor to a flat top. */
	inline constexpr double BlockHalfWidth = 2.100;
	inline constexpr double BlockTop = 5.200;

	/** The door's arch is set out at 64 stations per semicircle (§3.1, as built: SalonStructure.cpp:41-42). */
	inline constexpr int32 ArchStations = 64;

	// ---- §3.2 mouldings: the catalogue's cove, skirting and architrave --------------------------------------------------
	inline constexpr const TCHAR* CoveProfile = TEXT("cove.quarter-ellipse.1450");
	inline constexpr const TCHAR* SkirtingProfile = TEXT("skirting.flat.100");
	/**
	 * P6, the slim architrave round the door on the oval face, 0.160 wide on the face. The skirting stops against its outer
	 * edge on either side of the door.
	 */
	inline constexpr const TCHAR* ArchitraveProfile = TEXT("architrave.two-fascia.160");
	inline constexpr double ArchitraveWidth = 0.160;

	// ---- §3.3 velarium, attic and glass dome, all within the rim line: the cove's rim, ≈ the ellipse 9.55 × 6.05 -------
	inline constexpr double CoveReach = 1.450;
	inline constexpr double RimA = OvalA - CoveReach;
	inline constexpr double RimB = OvalB - CoveReach;
	inline constexpr double RimZ = 6.350;
	inline constexpr double VelariumRise = 0.500;
	/** The rim ring, a bronze angle 60 × 60 × 6 on the cove's top, its horizontal leg under the cloth's edge. */
	inline constexpr double RimAngleLeg = 0.060;
	inline constexpr double RimAngleThickness = 0.006;
	/** The attic upstand at the rim line, 0.10 thick, from the cove's top to the ring's soffit. */
	inline constexpr double UpstandMeters = 0.100;
	/** The glass dome springs from the ring's top. The cloth and the glass both rise as z = springing + rise (1 − ρ²). */
	inline constexpr double DomeRise = 0.600;
	/**
	 * The dome's laminated glass: two 8 mm low-iron panes on a 1.52 mm PVB interlayer. Overhead glazing is laminated
	 * safety glass with an interlayer of at least 0.76 mm (DIN 18008-2:2020, 6.2); 8 + 8 on 1.52 is the usual build for
	 * a walk-off roof light of this span.
	 */
	inline constexpr double DomeGlassMeters = 0.008 + 0.008 + 0.00152;

	/**
	 * The velarium's frame (§3.3): bronze tees 40 × 40 × 5 lying on the cloth over its seams, where the 1.40 widths are
	 * double-stitched (y = ±0.70 … ±4.90), their ends on the rim ring; each hung from the dome's bar above it by Ø 6 rods
	 * at 1.20.
	 */
	inline constexpr double SeamYs[] = {0.70, 2.10, 3.50, 4.90};
	inline constexpr double RibFlange = 0.040;
	inline constexpr double RibDepth = 0.040;
	inline constexpr double RibMetal = 0.005;
	inline constexpr double RodDiameter = 0.006;
	inline constexpr double RodSpacing = 1.200;
	/**
	 * The dome's glazing bars (§3.3): 40 wide × 70 deep under the glass, along x over the seams and along y at x = 0,
	 * ±1.20 … ±8.40, bearing on a bronze curb of their own section round the rim line on the ring's inner face.
	 */
	inline constexpr double BarWidth = 0.040;
	inline constexpr double BarDepth = 0.070;
	inline constexpr double BarSpacing = 1.200;
	inline constexpr int32 BarsEachSide = 7;
	/**
	 * The ribs and bars follow the cloth's and the glass's rise through points this far apart along them: the rises bend
	 * at most 2 × 0.60 / 6.05² = 0.033 per metre, so a chord of 0.25 m departs from them by under 0.3 mm, within the
	 * museum's 1 mm chord tolerance.
	 */
	inline constexpr double RiseStep = 0.250;

	// ---- §3.4 floors -------------------------------------------------------------------------------------------------
	/** The honed travertine, finished level at z 0. */
	inline constexpr double FloorStone = 0.030;
	/** The viewing stones, discs flush in the floor with their gilt rings, at (±7.20, 0). */
	inline constexpr double ViewingStoneX = 7.200;
	inline constexpr double ViewingStoneRadius = 0.400;
	inline constexpr double ViewingRingRadius = 0.450;

	// ---- §2, §3.7 the well under the pond: its opening, curb, walls, soffit, base ---------------------------------------
	inline constexpr double OpeningWestX = -3.900;
	inline constexpr double OpeningEastX = 3.600;
	inline constexpr double ShaftHalf = 1.200;
	inline constexpr double ShaftOuterHalf = 1.600;
	inline constexpr double CurbDepth = 0.300;
	inline constexpr double SlabSoffit = -0.400;
	inline constexpr double EndWallWestX = -4.300;
	inline constexpr double HeadWallX = 11.500;
	inline constexpr double WallBed = -5.850;
	inline constexpr double FoundationUnderside = -6.150;
	inline constexpr double WellFloor = -5.800;
	/** The Reserve's door in the head wall (the stair port's class): the shaft's clear width, arched from 2.00 over the landing. */
	inline constexpr double ReserveDoorSpringing = 2.000;

	// ---- §3.7 the stair: 36 risers of 5.80 / 36 running east down from the top landing ---------------------------------
	inline constexpr int32 Risers = 36;
	inline constexpr double StairRise = 5.800;
	/**
	 * The going, set to a comfortable pace: 2R + T = 0.630 m, the middle of Blondel's rule for a stair walked easily
	 * (2R + T between 0.60 and 0.64 m; DIN 18065's step-length rule, 0.59 to 0.65 m), with R = 5.80 / 36 = 0.16111. The
	 * as-built going, 0.3389, made 2R + T = 0.661, a stride too long for the rise (the sheet's Departures).
	 */
	inline constexpr double Going = 0.3078;
	inline constexpr double TopRiserX = -3.500;
	/** The strings' faces, 25 mm proud of the brick. */
	inline constexpr double StringFace = 1.175;
	inline constexpr double StringOverNosing = 0.240;
	inline constexpr double StringCapZ = -0.060;
	inline constexpr double StringUnderNosing = 0.350;
	inline constexpr double LandingSkirting = 0.150;

	// ---- §3.7 handrails, both walls ----------------------------------------------------------------------------------
	inline constexpr double HandrailAxisY = 1.115;
	inline constexpr double HandrailOverNosing = 0.900;
	inline constexpr double HandrailStartX = -1.502;
	/** The rail turns level onto the landing one going past the last riser, where the nosing line reaches the landing. */
	inline constexpr double HandrailKneeX = TopRiserX + Risers * Going;
	inline constexpr double HandrailKneeRadius = 0.150;
	inline constexpr double HandrailEndX = 11.200;
	inline constexpr double HandrailDiameter = 0.050;
	/** The flat underside that carries the LED strip: the chord from 240° to 300°, 25 mm wide. */
	inline constexpr double HandrailFlatHalfAngleDeg = 30.0;
	inline constexpr double BracketSpacing = 1.300;
	inline constexpr double RoseDiameter = 0.060;
	inline constexpr double RoseThickness = 0.008;
	inline constexpr double ArmDiameter = 0.016;
	inline constexpr double ArmDrop = 0.075;

	// ---- §3.5 the pond: the water's edge, an ellipse about the centre, and everything built on its parallels ------------
	inline constexpr double PondA = 3.900;
	inline constexpr double PondB = 2.000;
	inline constexpr double WaterLevel = 0.380;
	inline constexpr double BedTop = 0.060;
	inline constexpr double ShelfX = 3.200;
	inline constexpr double ShelfTop = 0.260;
	/** The lining's face, 3 mm inside the water's edge; it rises to the lip's foot. */
	inline constexpr double LiningD = -0.003;
	inline constexpr double LipFoot = 0.355;
	/** The bronze tray under everything, out to the coping's toe; it bears on the floor at rest. */
	inline constexpr double TrayMeters = 0.012;
	inline constexpr double ToeD = 0.425;
	/** The coping: 16 blocks, block 0 centred on the east apex, joints 3 mm, measured on the die's parallel. */
	inline constexpr int32 CopingBlocks = 16;
	inline constexpr double CopingJoint = 0.003;
	/** The frieze's band on the die: its ground's parallel, its foot and top, and the carver's grid. */
	inline constexpr double DieD = 0.385;
	inline constexpr double BandFoot = 0.075;
	inline constexpr double BandTop = 0.2725;
	inline constexpr double FriezeGrid = 0.0025;
	/** The seat ring the visitor sits on (§10): d 0.25 in from the coping's water edge, at the seat's height. */
	inline constexpr double SeatD = 0.250;
	inline constexpr double SeatZ = 0.420;

	/** One point of a drawn section, (d out from the water's edge, pond-z). */
	struct FSectionPoint
	{
		double D;
		double Z;
	};

	/**
	 * The coping's section outside the frieze (§3.5, upper and lower profiles), from its outer foot up round the torus,
	 * over the seat and down its inner face: the lower profile's ledge, ovolo, plinth and toe; the die (the frieze's
	 * block, below); the cavetto, fillet and torus; the seat and its wash; the rebate that takes the lip's bead.
	 */
	inline constexpr double OvoloRadius = 0.015;
	inline constexpr double TorusRadius = 0.055;
	inline constexpr double CavettoRadius = 0.0275;
	inline constexpr double TorusStartDeg = -60.0;
	inline constexpr FSectionPoint OvoloCentre{0.425, 0.060};
	inline constexpr FSectionPoint TorusCentre{0.385, 0.365};
	inline constexpr FSectionPoint CavettoCentre{0.4125, 0.2725};
	inline constexpr FSectionPoint PlinthOut{0.440, 0.018};
	inline constexpr FSectionPoint ToeIn{0.425, 0.018};
	inline constexpr FSectionPoint Fillet{0.4125, 0.300};
	inline constexpr FSectionPoint SeatOuter{0.385, 0.4200};
	inline constexpr FSectionPoint SeatInner{0.120, 0.4200};
	inline constexpr FSectionPoint Wash{0.050, 0.4194};
	/** The rebate under the lip's bead: the bead's far side, 13 mm in, from its springing down to the lip's back. */
	inline constexpr FSectionPoint BeadTop{0.013, 0.4100};
	inline constexpr FSectionPoint BeadFoot{0.013, 0.394};

	/**
	 * The lip (§3.5): a 10 mm band on the coping's inner face with a slot 13 × 5 mm for the line of light, capped by a
	 * bead over the arris, drawn from its foot up the water side, over the bead and back down behind it.
	 */
	inline constexpr FSectionPoint LipPoints[] = {
		{-0.010, 0.355}, {-0.010, 0.3840}, {-0.005, 0.3840}, {-0.005, 0.3970}, {-0.010, 0.3970}, {-0.010, 0.403}};
	inline constexpr FSectionPoint BeadCentre{0.0015, 0.403};
	inline constexpr double BeadRadius = 0.0115;

	/** The four posts' sleeves and their telescopic stages (§3.5), collapsed at rest, heads flush with the floor. */
	inline constexpr double PostX = 3.000;
	inline constexpr double PostY = 1.350;
	inline constexpr double SleeveDiameter = 0.230;
	inline constexpr double SleeveDepth = 1.000;
	inline constexpr double StageDiameters[] = {0.210, 0.180, 0.150};
	inline constexpr double StageLength = 0.950;

	// ---- §3.6 the guard rails round the opening, housed in chases in the curb when down ---------------------------------
	inline constexpr double RailY = 1.275;
	inline constexpr double RailWestX = -3.500;
	inline constexpr double RailEastX = 3.600;
	inline constexpr double RailStopAtPost = 0.130;
	inline constexpr double EastRailX = 3.675;
	inline constexpr double EastRailHalf = 1.300;
	inline constexpr double ChaseWidth = 0.050;
	inline constexpr double ChaseDepth = 1.050;
	inline constexpr double RailTravel = 1.000;
	inline constexpr double TopRailDiameter = 0.040;
	inline constexpr double TopRailHeight = 1.000;
	inline constexpr double MidRailDiameter = 0.025;
	inline constexpr double MidRailHeight = 0.500;
	inline constexpr double StanchionDiameter = 0.025;
	inline constexpr double StanchionSpacing = 1.100;
	/** The east rail's closed bronze housing below the slab's soffit, 0.10 wide and 0.65 deep. */
	inline constexpr double HousingWidth = 0.100;
	inline constexpr double HousingDepth = 0.650;

	// ---- §3.8 the four curved benches: Catmull-Rom centrelines through these points, 8 subdivisions per span -----------
	inline constexpr int32 BenchSubdivisions = 8;
	inline constexpr double BenchSouthWest[][2] = {{-5.54, 1.39}, {-5.29, 1.81}, {-5.00, 2.17}, {-4.68, 2.48}, {-4.34, 2.76}, {-3.98, 2.99},
		{-3.60, 3.20}, {-3.21, 3.38}, {-2.81, 3.53}, {-2.40, 3.66}, {-1.98, 3.76}};
	inline constexpr double BenchSouthEast[][2] = {{1.98, 3.76}, {2.40, 3.66}, {2.81, 3.53}, {3.21, 3.38}, {3.60, 3.20}, {3.98, 2.99},
		{4.34, 2.76}, {4.68, 2.48}, {5.00, 2.17}, {5.29, 1.81}, {5.54, 1.39}};
	inline constexpr double SeatWidth = 0.550;
	inline constexpr double SeatUnderside = 0.350;
	inline constexpr double SeatTop = 0.450;
	inline constexpr double SeatEase = 0.010;
	inline constexpr double PlinthLength = 0.450;
	inline constexpr double PlinthFromEnd = 0.300;
	inline constexpr double PlinthSetBack = 0.060;
	inline constexpr double PlinthTop = 0.330;
	inline constexpr double PlinthNeckInset = 0.030;
	inline constexpr double PlinthToeHeight = 0.012;
	inline constexpr double PlinthToeInset = 0.008;
	inline constexpr double PlinthEase = 0.006;

	// ---- §2 anchors ----------------------------------------------------------------------------------------------------
	/** The golden iris stands on the coping's seat at the west apex, facing down the axis. */
	inline constexpr double IrisX = -4.150;
	/** The planting shelves' beds (§7), feet on the shelves at their middles. */
	inline constexpr double ShelfBedX = 3.500;
	/** The willows' wall starts at the door's north jamb, at this parametric angle of the inner ellipse (Hang.json). */
	inline constexpr double WillowsFromDeg = -11.537;
	/** The benches' seat frames face out to the willows along the wall's normal. */
	inline constexpr double BenchFacing[2] = {0.53, 0.85};

	// ---- §1 the footprint's height, and the served well's prism (§2) -----------------------------------------------------
	inline constexpr double FootprintBottom = -0.400;
	inline constexpr double FootprintTop = 7.800;
}
