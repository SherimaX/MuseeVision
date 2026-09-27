#pragma once

#include "CoreMinimal.h"

/**
 * Albion's plan: every number the court and its porch are built to, in metres in the stage's frame (origin at the
 * court's centre on the nave's axis and on column line 4, at the finished floor; x east, y south, z up), taken from its
 * design sheet, Docs/Stages/albion.md (the section is given with each group). The sheet's own sources are the as-built
 * court (windows/, D-017, every old number moved by y - 35.200) and the approved boards. A few thicknesses the sheet
 * leaves unsaid are the trade's usual ones or the as-built's, each named with its source. Finishes, light and the hang
 * are the schedules' (Data/Stages/albion), never here.
 */
namespace Musee::Albion::Plan
{
	/** The stage's key (§1). */
	inline constexpr const TCHAR* Stage = TEXT("albion");

	// ---- §1 setting-out ------------------------------------------------------------------------------------------------
	/** Column lines 1 to 7 every 6.00 from y -18; bays 1 to 6 between them. */
	inline constexpr int32 Lines = 7;
	inline constexpr double FirstLineY = -18.000;
	inline constexpr double BayMeters = 6.000;
	constexpr double LineY(int32 Line) { return FirstLineY + BayMeters * (Line - 1); }
	constexpr double BayY(int32 Bay) { return FirstLineY + BayMeters * (Bay - 0.5); }
	/** The arcade lines, the aisles' crowns, the side walls' faces and the end walls' faces. */
	inline constexpr double ArcadeX = 6.000;
	inline constexpr double AisleCrownX = 10.000;
	inline constexpr double SideInnerX = 14.000;
	inline constexpr double SideOuterX = 14.600;
	inline constexpr double EndInnerY = 18.000;
	inline constexpr double EndOuterY = 18.600;
	/**
	 * The glass line, 0.30 into every 0.60 wall: the lancets' glass (x ±14.30), the screens' planes (y ±18.30), the side
	 * parapets' and the end parapets' inner faces. Each wall is laid as two leaves meeting there: the court's banded leaf
	 * inside and the weathered leaf outside.
	 */
	inline constexpr double SideGlassX = 14.300;
	inline constexpr double EndGlassY = 18.300;
	/** The porch: inner faces x ±3.00, outer ±3.60; the port plane and its end lining's porch face. */
	inline constexpr double PorchInnerX = 3.000;
	inline constexpr double PorchOuterX = 3.600;
	inline constexpr double PortPlaneY = -24.000;
	inline constexpr double LiningFaceY = -23.900;
	/** The footings' foot: the plinth course, the buttresses' base courses and the outer leaves run down to it (§1). */
	inline constexpr double FootingZ = -1.200;
	/** The footprint's envelope, clockwise on the north-up plan, and the stage's height (§1). */
	inline constexpr double FootprintPoints[][2] = {{-3.70, -24.00}, {3.70, -24.00}, {3.70, -19.60}, {15.60, -19.60}, {15.60, 19.60},
		{-15.60, 19.60}, {-15.60, -19.60}, {-3.70, -19.60}};
	inline constexpr double FootprintBottom = -1.200;
	inline constexpr double FootprintTop = 17.600;

	// ---- §2 the port ---------------------------------------------------------------------------------------------------
	inline constexpr const TCHAR* PortClass = TEXT("doorway.4000x6500.arched");
	/** The class: clear 4.000 (jambs x ±2.000), a semicircular head springing at 4.500, crown 6.500. */
	inline constexpr double PortClearHalf = 2.000;
	inline constexpr double PortSpringing = 4.500;
	/** The rough opening through the 0.10 lining: the clear opening grown by the class allowance, its sill below the floor. */
	inline constexpr double RoughAllowance = 0.050;
	inline constexpr double RoughSill = -0.050;
	inline constexpr double LiningMeters = 0.100;
	/** The lining's porch face stays clear for the joint's surround within these (the skirting stops there). */
	inline constexpr double SurroundClearHalf = 2.350;
	inline constexpr double SurroundClearTop = 6.850;
	/** museum = local + (0, 35.200, 0): the translation Site must derive from the mating with rotunda.south. */
	inline constexpr double MuseumOffsetY = 35.200;
	/** The axis (Albion.AxisClear): the gilt sun's eye to the Attainment's centre, a sightline 0.60 across. */
	inline constexpr double SunEye[3] = {0.0, -35.200, 1.600};
	inline constexpr double AttainmentCentre[3] = {0.0, 17.950, 2.220};
	inline constexpr double SightlineDiameter = 0.600;

	// ---- §3.1 coursing -------------------------------------------------------------------------------------------------
	/** One buff course from the floor to 0.90, then a red course of 0.30 every 1.20 (inside eight, outside nine to 10.80). */
	inline constexpr double FirstRed = 0.900;
	inline constexpr double CourseMeters = 0.300;
	inline constexpr double RedPitch = 1.200;
	inline constexpr int32 RedCourses = 9;

	// ---- §3.1 the side walls -------------------------------------------------------------------------------------------
	/** The court's face runs to 9.73 under the cornice; the inner leaf rises behind it to the springing. */
	inline constexpr double SideFaceTop = 9.730;
	inline constexpr double Springing = 10.000;
	/** The lancets, one on each bay's centre line: 1.40 wide, sill 5.00, head 9.00, two-centred of radius 0.808. */
	inline constexpr double LancetHalf = 0.700;
	inline constexpr double LancetSill = 5.000;
	inline constexpr double LancetHead = 9.000;
	inline constexpr double LancetRadius = 0.808;
	/** Pilasters on lines 2 to 6 of the side walls: 0.30 wide, 0.10 proud, 0.19 to 9.30, and their corbel's three steps. */
	inline constexpr double PilasterHalf = 0.150;
	inline constexpr double PilasterProud = 0.100;
	inline constexpr double PilasterFoot = 0.190;
	inline constexpr double PilasterTop = 9.300;
	struct FCorbelStep
	{
		double Top;
		double Half;
		double Proud;
	};
	inline constexpr FCorbelStep CorbelSteps[] = {{9.450, 0.170, 0.150}, {9.600, 0.190, 0.200}, {9.740, 0.210, 0.260}};
	/** The aisle cornice's section, (out from the face, z), from its foot (§3.1). */
	inline constexpr double CornicePoints[][2] = {{-0.010, 9.720}, {0.030, 9.720}, {0.075, 9.770}, {0.075, 9.840}, {0.120, 9.880},
		{0.120, 10.000}, {-0.010, 10.000}};
	/** The corner corbels in each aisle corner. */
	inline constexpr double CornerCorbel = 0.220;
	inline constexpr double CornerCorbelFoot = 9.350;
	inline constexpr double CornerCorbelTop = 9.740;
	/** The slate skirting: 0.20 high, 0.015 proud, a 4 mm bevel on its top arris. */
	inline constexpr double SkirtingTop = 0.200;
	inline constexpr double SkirtingProud = 0.015;
	inline constexpr double SkirtingBevel = 0.004;

	// ---- §3.1 the end walls ----------------------------------------------------------------------------------------------
	/** The nave's ends are stone to its cill; the cill's front 8.38 to 8.52, 0.02 proud, weathered to 8.47 at 0.05 outside. */
	inline constexpr double CillBed = 8.380;
	inline constexpr double CillFrontTop = 8.520;
	inline constexpr double CillProud = 0.020;
	inline constexpr double CillOuterTop = 8.470;
	inline constexpr double CillOuterProud = 0.050;
	/** The court door in the north wall: 4.00 wide, pointed, springing 3.30, crown 5.50. */
	inline constexpr double CourtDoorHalf = 2.000;
	inline constexpr double CourtDoorSpringing = 3.300;
	inline constexpr double CourtDoorCrown = 5.500;
	/** The hood mould on the court face: the door's outline grown 0.16, level returns 0.28 at the springing; (across, out). */
	inline constexpr double HoodGrow = 0.160;
	inline constexpr double HoodReturn = 0.280;
	inline constexpr double HoodPoints[][2] = {{-0.045, -0.010}, {0.045, -0.010}, {0.045, 0.035}, {0.020, 0.070}, {-0.030, 0.070}, {-0.045, 0.050}};
	/** Where the skirting stops beside the court door, on its court face and its porch face. */
	inline constexpr double CourtSkirtingStopX = 2.101;
	inline constexpr double PorchSkirtingStopX = 2.061;
	/** The tablets: 0.03 proud with a 0.02 chamfered edge. Ruskin's over the court door; ALBION in the west aisle. */
	inline constexpr double TabletProud = 0.030;
	inline constexpr double TabletChamfer = 0.020;
	inline constexpr double RuskinTablet[4] = {-4.600, 4.600, 6.500, 7.500};
	inline constexpr double AlbionTablet[4] = {-12.600, -7.400, 1.850, 3.450};

	// ---- §3.2 outside ----------------------------------------------------------------------------------------------------
	/** The plinth course round court and porch, (out from the face, z). */
	inline constexpr double PlinthPoints[][2] = {{-0.020, -1.200}, {0.080, -1.200}, {0.080, 0.520}, {0.000, 0.600}, {-0.020, 0.600}};
	/** The parapets carry their copings from 10.88 (the sheet's 10.90 is the top under the coping's weathering). */
	inline constexpr double CopingBed = 10.880;
	/** The copings' section, (out from the outer face, z): two U runs round each side from x ∓6. */
	inline constexpr double CopingPoints[][2] = {{-0.350, 10.880}, {0.050, 10.880}, {0.050, 10.960}, {-0.150, 11.050}, {-0.350, 10.960}};
	/** The buttresses: 1.00 wide, 0.90 deep to the set-off, weathered back to 0.558, then steeply into the wall at the top. */
	inline constexpr double ButtressHalf = 0.500;
	inline constexpr double ButtressDepth = 0.900;
	inline constexpr double ButtressSetOff = 4.800;
	inline constexpr double ButtressSetOffTop = 5.150;
	inline constexpr double ButtressUpperDepth = 0.558;
	inline constexpr double ButtressTopWeathering = 0.600;
	inline constexpr double SideButtressTop = 9.900;
	inline constexpr double EndButtressTop = 10.400;
	/** Their base courses, 0.06 proud all round, weathered from 0.52 to 0.60 (the plinth's heights). */
	inline constexpr double BaseCourseProud = 0.060;
	inline constexpr double BaseCourseTop = 0.520;
	inline constexpr double BaseCourseWeathered = 0.600;
	/** The lancets outside: the sill weathered from 5.04 at the glass to 5.00 at the face; a drip block under it. */
	inline constexpr double OuterSillAtGlass = 5.040;
	inline constexpr double DripHalf = 0.840;
	inline constexpr double DripProud = 0.070;
	inline constexpr double DripFoot = 4.880;
	/** The side gutters: lead from the inner face to the parapet at 10.012, turned up 0.15 against the parapet. */
	inline constexpr double SideGutterTop = 10.012;
	inline constexpr double GutterTurnUp = 0.150;
	/** Milled lead, Code 7 (3.15 mm, BS EN 12588), for the turn-ups and the porch's flat. */
	inline constexpr double LeadMeters = 0.00315;

	// ---- §3.2, §3.7 the porch -------------------------------------------------------------------------------------------
	inline constexpr double PorchCeiling = 6.900;
	inline constexpr double LeadFlat = 7.200;
	inline constexpr double PorchParapetInnerX = 3.350;
	inline constexpr double PorchParapetTop = 7.350;
	inline constexpr double PorchCopingInnerX = 3.300;
	inline constexpr double PorchCopingOuterX = 3.650;
	inline constexpr double PorchCopingTop = 7.470;
	/** The oak ceiling's boards: one-inch (25 mm) boarding, the usual for a boarded ceiling of this span. */
	inline constexpr double CeilingBoards = 0.025;
	/** Three moulded beams across, 0.20 × 0.30 with 12 mm eased arrises, drawn up to the boards' soffit (sheet 6.61 to 6.91). */
	inline constexpr double BeamY[] = {-22.600, -21.250, -19.900};
	inline constexpr double BeamHalf = 0.100;
	inline constexpr double BeamSoffit = 6.610;
	inline constexpr double BeamEase = 0.012;
	/** Wall plates along the side walls at x ±2.93, 0.14 × 0.18 from 6.73, drawn up to the boards' soffit. */
	inline constexpr double WallPlateX = 2.930;
	inline constexpr double WallPlateHalf = 0.070;
	inline constexpr double WallPlateSoffit = 6.730;

	// ---- §3.3 floors -----------------------------------------------------------------------------------------------------
	inline constexpr double NaveFieldHalfX = 5.700;
	inline constexpr double NaveFieldHalfY = 17.700;
	inline constexpr double BorderMeters = 0.300;
	/** The aisles' flags run 1 cm under the walls. */
	inline constexpr double UnderWall = 0.010;
	inline constexpr double PorchFieldHalfX = 2.700;
	inline constexpr double PorchFieldNorth = -23.600;
	inline constexpr double PorchFieldSouth = -18.900;
	/** The court door's bronze strip, 0.06 × 4.03 on the wall's middle line, 0.5 mm proud. */
	inline constexpr double StripHalfWidth = 0.030;
	inline constexpr double StripHalfLength = 2.015;
	inline constexpr double StripProud = 0.0005;
	/** A six-inch encaustic tile is three-quarters of an inch (19 mm) thick (Minton & Co.'s floor tiles). */
	inline constexpr double TileMeters = 0.019;
	/** Sawn York stone flags are laid 50 mm thick. */
	inline constexpr double FlagMeters = 0.050;
	/** The threshold strip is a 6 mm bronze flat bedded in the stone. */
	inline constexpr double StripMeters = 0.006;

	// ---- §3.4 the columns --------------------------------------------------------------------------------------------------
	/** The ten stones by line (2 to 6), west then east: A to E, F to J. */
	inline constexpr const TCHAR* WestStones[] = {TEXT("a"), TEXT("b"), TEXT("c"), TEXT("d"), TEXT("e")};
	inline constexpr const TCHAR* EastStones[] = {TEXT("f"), TEXT("g"), TEXT("h"), TEXT("i"), TEXT("j")};
	/** The base's square plinth, 0.72 with 0.07 chamfered corners, 0 to 0.12. */
	inline constexpr double PlinthHalf = 0.360;
	inline constexpr double PlinthChamfer = 0.070;
	inline constexpr double PlinthTop = 0.120;
	/** A turned section's point, (r, z), and whether the section runs smoothly through it (the sheet's *). */
	struct FTurnPoint
	{
		double R;
		double Z;
		bool bSmooth;
	};
	inline constexpr FTurnPoint BasePoints[] = {{0.0, 0.120, false}, {0.345, 0.120, false}, {0.345, 0.135, false}, {0.345, 0.160, true},
		{0.330, 0.185, true}, {0.300, 0.195, false}, {0.285, 0.200, false}, {0.275, 0.235, true}, {0.285, 0.260, false}, {0.290, 0.270, true},
		{0.300, 0.290, true}, {0.290, 0.310, true}, {0.270, 0.318, false}, {0.262, 0.330, false}, {0.262, 0.355, false}, {0.0, 0.355, false}};
	/** The shaft, Ø 0.50, polished, one monolith 0.34 to 5.505. */
	inline constexpr double ShaftRadius = 0.250;
	inline constexpr double ShaftFoot = 0.340;
	inline constexpr double ShaftTop = 5.505;
	/** The capital block: its necking, its bell (r = 0.255 + 0.145 t^1.8 at 12 stations), its abacus. */
	inline constexpr FTurnPoint NeckingPoints[] = {{0.0, 5.490, false}, {0.258, 5.490, false}, {0.262, 5.500, true}, {0.278, 5.525, true},
		{0.262, 5.550, true}, {0.255, 5.560, false}};
	inline constexpr double BellFoot = 5.560;
	inline constexpr double BellTop = 6.080;
	inline constexpr double BellRadius = 0.255;
	inline constexpr double BellFlare = 0.145;
	inline constexpr double BellExponent = 1.8;
	inline constexpr int32 BellStations = 12;
	/** The abacus, 0.90 square: a cavetto from half 0.36 to 0.415, a half-round, a fillet, the face and a chamfer. */
	inline constexpr double AbacusFoot = 6.075;
	inline constexpr double AbacusHalf = 0.450;
	inline constexpr double AbacusCoreHalf = 0.360;
	inline constexpr double CavettoTop = 6.115;
	inline constexpr double CavettoHalf = 0.415;
	inline constexpr double AbacusBeadRadius = 0.018;
	inline constexpr double AbacusFillet[2][2] = {{0.438, 6.151}, {0.450, 6.153}};
	inline constexpr double AbacusFaceTop = 6.188;
	inline constexpr double AbacusChamfer = 0.012;
	inline constexpr double CapitalTop = 6.200;

	// ---- §3.5 the clusters ---------------------------------------------------------------------------------------------------
	inline constexpr double ShoeHalf = 0.220;
	inline constexpr double ShoeFoot = 6.198;
	inline constexpr double ShoeTop = 6.230;
	inline constexpr FTurnPoint ClusterBasePoints[] = {{0.0, 6.230, false}, {0.200, 6.230, false}, {0.200, 6.250, true}, {0.170, 6.290, true},
		{0.150, 6.300, false}, {0.150, 6.340, false}, {0.0, 6.340, false}};
	/** A core Ø 0.15 and four colonnettes Ø 0.09 on the diagonals at r 0.095, cast as one shaft 6.33 to 9.42. */
	inline constexpr double CoreRadius = 0.075;
	inline constexpr double ColonnetteRadius = 0.045;
	inline constexpr double ColonnetteReach = 0.095;
	inline constexpr double ClusterShaftFoot = 6.330;
	inline constexpr double ClusterShaftTop = 9.420;
	/** Annulets at 7.45 and 8.65, (r, z - its height). */
	inline constexpr double AnnuletZ[] = {7.450, 8.650};
	inline constexpr FTurnPoint AnnuletPoints[] = {{0.0, -0.030, false}, {0.150, -0.030, false}, {0.165, -0.010, true}, {0.165, 0.010, true},
		{0.150, 0.030, false}, {0.0, 0.030, false}};
	inline constexpr FTurnPoint ClusterCapitalPoints[] = {{0.0, 9.410, false}, {0.150, 9.410, false}, {0.165, 9.440, true}, {0.150, 9.470, false},
		{0.155, 9.500, true}, {0.190, 9.590, true}, {0.250, 9.660, false}, {0.0, 9.660, false}};
	inline constexpr double TopPlateHalf = 0.300;
	inline constexpr double TopPlateFoot = 9.655;
	inline constexpr double TopPlateTop = 9.702;
	/** The half-columns' clusters stand 0.12 off the end walls. */
	inline constexpr double HalfClusterY = 17.880;

	// ---- §3.5 the box girders ------------------------------------------------------------------------------------------------
	inline constexpr double GirderEndY = 18.020;
	inline constexpr double GirderFoot = 9.700;
	inline constexpr double GirderTop = 10.000;
	inline constexpr double FlangeMeters = 0.018;
	inline constexpr double BottomFlangeHalf = 0.350;
	inline constexpr double TopFlangeHalf = 0.370;
	inline constexpr double GirderWebX = 0.290;
	inline constexpr double GirderWebMeters = 0.012;
	/** The lead valley gutter on each girder, 0.66 wide, its surface at 10.004. */
	inline constexpr double ValleyHalf = 0.330;
	inline constexpr double ValleyTop = 10.004;

	// ---- §3.5 the arcade -----------------------------------------------------------------------------------------------------
	/** Two-centred from the capitals' tops at ±2.75 from the bay's centre to the extrados crown under the girder. */
	inline constexpr double ArcadeSpringing = 6.200;
	inline constexpr double ArcadeHalfSpan = 2.750;
	inline constexpr double ArcadeCrown = 9.700;
	/**
	 * The rib, 0.16 deep from the extrados: an outer flange 0.11 × 0.018, a web 0.024, an inner flange 0.09 × 0.018 and
	 * a roll Ø 0.044 whose foot is the soffit (9.54 at the crown).
	 */
	inline constexpr double ArcadeRibDepth = 0.160;
	inline constexpr double ArcadeOuterFlangeHalf = 0.055;
	inline constexpr double ArcadeWebHalf = 0.012;
	inline constexpr double ArcadeInnerFlangeHalf = 0.045;
	inline constexpr double ArcadeRollRadius = 0.022;
	inline constexpr double ArcadeBossRadius = 0.050;
	/** The spandrels: a ring R 0.46 at 2.34 from the bay's centre and z 9.20, an inner ring R 0.41, a quatrefoil of four. */
	inline constexpr double SpandrelReach = 2.340;
	inline constexpr double SpandrelZ = 9.200;
	inline constexpr double SpandrelRing = 0.460;
	inline constexpr double SpandrelTube = 0.036;
	inline constexpr double SpandrelInnerRing = 0.410;
	inline constexpr double SpandrelInnerTube = 0.016;
	inline constexpr double QuatrefoilRing = 0.170;
	inline constexpr double QuatrefoilTube = 0.018;
	inline constexpr double QuatrefoilReach = 0.190;
	inline constexpr double LugWidth = 0.040;
	inline constexpr double LugThickness = 0.044;

	// ---- §3.5 the vaults (the glass lies on these arcs; transverse section) -----------------------------------------------------
	inline constexpr double NaveHalfSpan = 6.000;
	inline constexpr double NaveCrown = 17.000;
	inline constexpr double AisleHalfSpan = 4.000;
	inline constexpr double AisleCrown = 15.200;
	/** The glass and its iron run between the screens' planes. */
	inline constexpr double GlassEndY = EndGlassY;
	/** Main ribs: a built-up I 0.26 deep whose top is 0.15 under the glass (the end lines' flanges wider). */
	inline constexpr double RibTop = 0.150;
	inline constexpr double MainRibDepth = 0.260;
	inline constexpr double MainFlangeMeters = 0.016;
	inline constexpr double MainOuterHalf = 0.060;
	inline constexpr double MainOuterHalfEnd = 0.080;
	inline constexpr double MainWebHalf = 0.007;
	inline constexpr double MainInnerHalf = 0.051;
	inline constexpr double MainInnerHalfEnd = 0.068;
	/** The inner arch: a bar 0.03 × 0.018, 0.62 under the glass at the springing, blending as (s/L)^6 into the rib by 94 %. */
	inline constexpr double InnerArchDepth = 0.620;
	inline constexpr double InnerArchBar = 0.030;
	inline constexpr double InnerArchHalf = 0.009;
	inline constexpr double InnerArchExponent = 6.0;
	inline constexpr double InnerArchReach = 0.94;
	/** The ring chain between rib and inner arch: rings R 0.097 (Ø 0.018) at 0.24 pitch to 82 % of the arc. */
	inline constexpr double ChainRing = 0.097;
	inline constexpr double ChainTube = 0.018;
	inline constexpr double ChainPitch = 0.240;
	inline constexpr double ChainReach = 0.82;
	/** The rib's shoe, 0.40 × 0.20 × 0.026, on the girder or the bearer. */
	inline constexpr double RibShoeHalfX = 0.200;
	inline constexpr double RibShoeHalfY = 0.100;
	inline constexpr double RibShoeMeters = 0.026;
	/** Light ribs every 1.50 between the main lines: a tee, its table 0.09 × 0.012, stem 0.012, bead 0.022 × 0.018, 0.14 deep. */
	inline constexpr double LightRibPitch = 1.500;
	inline constexpr double LightRibDepth = 0.140;
	inline constexpr double LightTableHalf = 0.045;
	inline constexpr double LightTableMeters = 0.012;
	inline constexpr double LightStemHalf = 0.006;
	inline constexpr double LightBeadHalf = 0.011;
	inline constexpr double LightBeadMeters = 0.018;
	/** The side bearers along each side wall at the springing, 0.38 into the court, 9.94 to 10.00. */
	inline constexpr double BearerReach = 0.380;
	inline constexpr double BearerFoot = 9.940;
	/** A cast bracket under each light rib's foot: a plate 0.016 thick up the wall from 9.65, its strut to 0.33 in at 9.93. */
	inline constexpr double BracketMeters = 0.016;
	inline constexpr double BracketFoot = 9.650;
	inline constexpr double BracketReach = 0.330;
	inline constexpr double BracketStrutTop = 9.930;
	/** Purlins: 5 a nave half, 3 an aisle half; a tee, flange 0.052 × 0.012 from 0.05 under the glass, web 0.01 to 0.15. */
	inline constexpr int32 NavePurlins = 5;
	inline constexpr int32 AislePurlins = 3;
	inline constexpr double PurlinEndY = 18.280;
	inline constexpr double PurlinTop = 0.050;
	inline constexpr double PurlinFlangeHalf = 0.026;
	inline constexpr double PurlinFlangeMeters = 0.012;
	inline constexpr double PurlinWebHalf = 0.005;
	/** A ridge bar 0.10 wide at each crown, 0.004 to 0.15 under the glass. */
	inline constexpr double RidgeBarHalf = 0.050;
	inline constexpr double RidgeBarTop = 0.004;
	/** Glazing bars, 48 a half at 36.6/49, from 0.10 above the springing: a web 0.008 from 0.003 to 0.05 under, a foot 0.032 × 0.01. */
	inline constexpr int32 GlazingBays = 49;
	inline constexpr double BarsFrom = 0.100;
	inline constexpr double BarTop = 0.003;
	inline constexpr double BarFoot = 0.050;
	inline constexpr double BarWebHalf = 0.004;
	inline constexpr double BarFootHalf = 0.016;
	inline constexpr double BarFootMeters = 0.010;
	/** The vault's panes: flat, 0.60 of arc each from 10.10, each lapping 10 mm over the one below. */
	inline constexpr double PanesFrom = 10.100;
	inline constexpr double PaneArc = 0.600;
	inline constexpr double PaneLap = 0.010;
	/**
	 * Laminated float for the vaults and screens: 4 + 4 mm on a 0.76 mm UV-filtering interlayer, the least interlayer
	 * overhead glazing may have (DIN 18008-2:2020, 6.2).
	 */
	inline constexpr double LaminatedMeters = 0.004 + 0.00076 + 0.004;

	// ---- §3.5 the end screens and the gables --------------------------------------------------------------------------------------
	inline constexpr double ScreenMullionX[] = {-6.0, -4.8, -3.2, -1.6, 0.0, 1.6, 3.2, 4.8, 6.0};
	/** Mullions, (across, deep): the edge ones 0.12 × 0.20, the others 0.07 × 0.12. */
	inline constexpr double EdgeMullion[2] = {0.120, 0.200};
	inline constexpr double ScreenMullion[2] = {0.070, 0.120};
	/** Glazing bars 0.024 × 0.05 at x = -5.4 + 0.8k (15). */
	inline constexpr double ScreenBarFirstX = -5.400;
	inline constexpr double ScreenBarPitch = 0.800;
	inline constexpr int32 ScreenBars = 15;
	inline constexpr double ScreenBar[2] = {0.024, 0.050};
	/** Transoms (the nave's cill on the stone is the first), each stopping where the arc comes down; (high, deep). */
	inline constexpr double ScreenTransomZ[] = {9.000, 10.700, 10.900, 12.600, 14.800};
	inline constexpr double ScreenTransom[2] = {0.070, 0.100};
	inline constexpr double BandTransom[2] = {0.100, 0.140};
	inline constexpr double UpperTransomZ = 16.000;
	inline constexpr double UpperTransom[2] = {0.030, 0.060};
	/** The nave's iron cill on the stone, 0.16 deep, 8.52 to 8.59. */
	inline constexpr double ScreenCillDeep = 0.160;
	inline constexpr double ScreenCillTop = 8.590;
	/** The screens' iron and glass stop 0.02 under the vault's arc. */
	inline constexpr double UnderArc = 0.020;
	/** The aisles' gables: mullions at |x| 6.8 to 13.2 (0.06 × 0.10), transoms at 11.60 and 13.40 (0.06 × 0.08), a cast cill. */
	inline constexpr double GableMullionX[] = {6.8, 8.4, 10.0, 11.6, 13.2};
	inline constexpr double GableMullion[2] = {0.060, 0.100};
	inline constexpr double GableTransomZ[] = {11.600, 13.400};
	inline constexpr double GableTransom[2] = {0.060, 0.080};
	/** The gables' cast cill, 0.06 high on the aisle ends' tops; 0.14 deep as built (AlbionIron.cpp:405). */
	inline constexpr double GableCillHigh = 0.060;
	inline constexpr double GableCillDeep = 0.140;

	// ---- §3.5 lancets' iron, lead and rainwater --------------------------------------------------------------------------------------
	/** Saddle bars 0.016 square, 0.015 inside the glass line, 0.03 into each jamb. */
	inline constexpr double SaddleBar = 0.016;
	inline constexpr double SaddleInside = 0.015;
	inline constexpr double SaddleReach = 0.030;
	inline constexpr double SaddleZ[] = {5.450, 5.900, 6.350, 6.800, 7.250, 7.700, 8.150};
	/** Lead rolls Ø 0.09 on the three crowns, 0.05 above them, y ±18.42. */
	inline constexpr double RidgeRollRadius = 0.045;
	inline constexpr double RidgeRollLift = 0.050;
	inline constexpr double RidgeRollEndY = 18.420;
	/** The aisles' crestings: a plate 0.03 thick, 15.28 to 15.54. */
	inline constexpr double CrestingMeters = 0.030;
	inline constexpr double CrestingFoot = 15.280;
	inline constexpr double CrestingTop = 15.540;
	/** The nave ventilator: standards 0.30 × 0.04, 17.08 to 17.48 every 1.5; a lead cap from x ±0.22 at 17.48 to 17.60. */
	inline constexpr double StandardHalfX = 0.150;
	inline constexpr double StandardHalfY = 0.020;
	inline constexpr double StandardFoot = 17.080;
	inline constexpr double StandardTop = 17.480;
	inline constexpr double StandardPitch = 1.500;
	inline constexpr double CapHalf = 0.220;
	inline constexpr double CapRidge = 17.600;
	/** Rainwater: ten downpipes Ø 0.104, 0.14 off the face, heads to 10.55 on the sides and 10.35 on the ends. */
	inline constexpr double PipeRadius = 0.052;
	inline constexpr double PipeOff = 0.140;
	inline constexpr double SidePipeY[] = {-12.800, -0.800, 11.200};
	inline constexpr double SideHeadTop = 10.550;
	inline constexpr double EndPipeX = 6.800;
	inline constexpr double EndHeadTop = 10.350;
	/** A head 0.38 × 0.29 × 0.36 (along, out, high) with a lip 0.44 × 0.32 × 0.07; the pipe tapers from it over 0.14. */
	inline constexpr double HeadAlongHalf = 0.190;
	inline constexpr double HeadOut = 0.290;
	inline constexpr double HeadHigh = 0.360;
	inline constexpr double LipAlongHalf = 0.220;
	inline constexpr double LipOut = 0.320;
	inline constexpr double LipHigh = 0.070;
	inline constexpr double PipeTaper = 0.140;
	/** The head's outlet the pipe tapers from, r 0.09 as built (AlbionIron.cpp:500). */
	inline constexpr double OutletRadius = 0.090;
	inline constexpr double ShoeRadius = 0.064;
	inline constexpr double PipeShoeFoot = 0.020;
	inline constexpr double PipeShoeTop = 0.160;
	/** Holderbats every 1.80 from 1.20: a collar 0.06 high, 0.01 proud of the pipe, and its arm to the wall, as built (AlbionIron.cpp:504-509). */
	inline constexpr double HolderbatFirst = 1.200;
	inline constexpr double HolderbatPitch = 1.800;
	inline constexpr double HolderbatHalf = 0.030;
	inline constexpr double HolderbatProud = 0.010;
	inline constexpr double HolderbatArm[2] = {0.024, 0.050};

	// ---- §3.6 glazing --------------------------------------------------------------------------------------------------------------
	/** The band: six columns of 1.60 from x 4.80, the upper row 10.90 to 12.60, the lower 9.00 to 10.70; a 0.07 border. */
	inline constexpr double BandEastX = 4.800;
	inline constexpr double BandPanelWidth = 1.600;
	inline constexpr double BandUpperFoot = 10.900;
	inline constexpr double BandLowerFoot = 9.000;
	inline constexpr double BandPanelHigh = 1.700;
	inline constexpr double BandBorder = 0.070;
	/** Protective glazing 0.05 outside every band panel and lancet. */
	inline constexpr double GuardOffset = 0.050;
	/** The lancets' glass: the light's outline grown 0.01 (1.42 × 4.01) at the glass line. */
	inline constexpr double LancetGlassGrow = 0.010;
	/** Lamberts' mouth-blown antique glass is about 3 mm thick. */
	inline constexpr double LeadedGlassMeters = 0.003;
	/** A quarter-inch (6 mm) H-section lead came, as deep as it is wide. */
	inline constexpr double CameMeters = 0.006;
	/** The lancets' plain diamond quarries, 0.13 × 0.18, as built (Scripts/albion_glass.py:283). */
	inline constexpr double QuarryWidth = 0.130;
	inline constexpr double QuarryHigh = 0.180;

	// ---- §3.8 fittings ---------------------------------------------------------------------------------------------------------------
	/** The picture rail: a rod Ø 0.022 at 4.20, 0.035 off the side walls, bay by bay, with ball ends and brackets. */
	inline constexpr double RailZ = 4.200;
	inline constexpr double RailRadius = 0.011;
	inline constexpr double RailOff = 0.035;
	inline constexpr double RailClearOfPilaster = 0.030;
	inline constexpr double RailClearOfEndWall = 0.050;
	inline constexpr double RailBallRadius = 0.016;
	inline constexpr double RailBracketPitch = 1.400;
	inline constexpr double RailPlate[3] = {0.060, 0.009, 4.140};
	inline constexpr double RailArm[3] = {0.024, 4.165, 4.188};
	/** The benches, centred (±3.2, 4.8), long along y: top 3.00 × 0.50 at 0.45 in two 0.065 boards with a 12 mm rounded arris. */
	inline constexpr double BenchX = 3.200;
	inline constexpr double BenchY = 4.800;
	inline constexpr double BenchHalfLength = 1.500;
	inline constexpr double BenchHalfDepth = 0.250;
	inline constexpr double BenchTop = 0.450;
	inline constexpr double BenchBoard = 0.065;
	inline constexpr double BenchArris = 0.012;
	/** Its ends, boards on edge 0.38 × 0.07 centred 0.30 in from each end, 0.10 to the top; feet 0.14 × 0.08 at x ±0.15. */
	inline constexpr double BenchEndIn = 0.300;
	inline constexpr double BenchEndHalf = 0.190;
	inline constexpr double BenchEndThick = 0.070;
	inline constexpr double BenchEndFoot = 0.100;
	inline constexpr double BenchFootX = 0.150;
	inline constexpr double BenchFoot[2] = {0.140, 0.080};
	/** Cleats under the top inside each end, 0.42 × 0.10 × 0.06, as built (AlbionFurniture.cpp:44). */
	inline constexpr double BenchCleat[3] = {0.420, 0.100, 0.060};
	/** The stretcher 0.06 × 0.11 at 0.20, from y -1.28 to 1.28, wedged through mortises in the ends. */
	inline constexpr double StretcherHalf[2] = {0.030, 0.055};
	inline constexpr double StretcherZ = 0.200;
	inline constexpr double StretcherHalfLength = 1.280;
	/** The seats' anchors (§2): across the bench, their tops. */
	inline constexpr double SeatOuter = 3.325;
	inline constexpr double SeatInner = 3.075;
	inline constexpr double SeatY[] = {4.200, 5.400};
	/** The table cases, centred (±10.4, ∓6.0), 1.00 × 2.20, top 0.95. */
	inline constexpr double CaseX = 10.400;
	inline constexpr double CaseY[] = {-6.000, 6.000};
	inline constexpr double CaseHalfX = 0.500;
	inline constexpr double CaseHalfY = 1.100;
	inline constexpr double CaseTop = 0.950;
	/** The legs, 0.05 in from the case's edges as built (AlbionFurniture.cpp:60): turned 0 to 0.36, a 0.07 square block to 0.54. */
	inline constexpr double LegIn = 0.050;
	inline constexpr double LegBlockHalf = 0.035;
	inline constexpr double LegTurnTop = 0.360;
	inline constexpr double LegBlockTop = 0.540;
	/** The turning, fitted inside the block, after the as-built legs (AlbionFurniture.cpp:63). */
	inline constexpr FTurnPoint LegPoints[] = {{0.0, 0.0, false}, {0.030, 0.0, false}, {0.032, 0.030, true}, {0.024, 0.060, true},
		{0.022, 0.280, true}, {0.030, 0.330, true}, {0.033, 0.360, false}, {0.0, 0.360, false}};
	/** Stretchers 0.036 × 0.05 at 0.12: two along and one across, as built. */
	inline constexpr double CaseStretcher[2] = {0.036, 0.050};
	inline constexpr double CaseStretcherZ = 0.120;
	/** The apron under the deck from 0.38, 24 mm boards as built (AlbionFurniture.cpp:69). */
	inline constexpr double ApronFoot = 0.380;
	inline constexpr double ApronMeters = 0.024;
	/** The deck 0.50 to 0.535, its moulded edge 0.015 proud of the apron; the baize bed 0.535 to 0.550. */
	inline constexpr double DeckFoot = 0.500;
	inline constexpr double DeckTop = 0.535;
	inline constexpr double BaizeTop = 0.550;
	/** The glazed box: oak corner posts 0.028 square on the leg blocks, top rails 0.028 × 0.024. */
	inline constexpr double CasePost = 0.028;
	inline constexpr double CaseRailHigh = 0.024;
	/** The case's glass: low-iron laminated, 3 + 3 mm on a 0.38 mm interlayer (EN 14449). */
	inline constexpr double CaseGlassMeters = 0.003 + 0.00038 + 0.003;
	/** The lectern at (12.2, -13.30), the reader facing east. */
	inline constexpr double LecternX = 12.200;
	inline constexpr double LecternY = -13.300;
	/** Cross feet 0.60 × 0.09 × 0.08 on four pads 0.10 square (12 mm as built, AlbionChaucer.cpp:236). */
	inline constexpr double FootHalfLength = 0.300;
	inline constexpr double FootHalfWidth = 0.045;
	inline constexpr double FootHigh = 0.080;
	inline constexpr double PadHalf = 0.050;
	inline constexpr double PadMeters = 0.012;
	inline constexpr double PadReach = 0.280;
	/** The post, stop-chamfered and tapering 0.11 to 0.085 square from 0.07 to 0.90 (drawn at its mean, §13); a collar 0.88 to 0.92. */
	inline constexpr double PostFoot = 0.070;
	inline constexpr double PostTop = 0.900;
	inline constexpr double PostFootWidth = 0.110;
	inline constexpr double PostHeadWidth = 0.085;
	inline constexpr double CollarHalf = 0.075;
	inline constexpr double CollarFoot = 0.880;
	inline constexpr double CollarTop = 0.920;
	/** The desk 0.74 × 0.60 × 0.03 sloping 18°, its far edge's top at 1.20; the ledge 0.035 up it. */
	inline constexpr double DeskSlopeDeg = 18.0;
	inline constexpr double DeskWidth = 0.740;
	inline constexpr double DeskDepth = 0.600;
	inline constexpr double DeskBoard = 0.030;
	inline constexpr double DeskFarTop = 1.200;
	inline constexpr double LedgeUp = 0.035;
	/** The ledge: 0.03 up the desk, 0.02 proud, as built (AlbionChaucer.cpp:258). */
	inline constexpr double LedgeWidth = 0.030;
	inline constexpr double LedgeProud = 0.020;
	/** Two brackets each side from the post's head to the desk, 0.04 × 0.044, as built (AlbionChaucer.cpp:248-249). */
	inline constexpr double LecternBracket[2] = {0.040, 0.044};
	inline constexpr double BracketFootY = 0.050;
	inline constexpr double BracketHeadY = 0.240;
	inline constexpr double BracketUp[] = {0.050, 0.520};
	/** The tapestry's batten: oak 7.00 × 0.10 × 0.03 in the lining's sleeve, its top at 3.44, on two brackets at x ±2.50. */
	inline constexpr double BattenHalf = 3.500;
	inline constexpr double BattenHigh = 0.100;
	inline constexpr double BattenThick = 0.030;
	inline constexpr double BattenTop = 3.440;
	inline constexpr double BattenBracketX = 2.500;
	/** The tapestry: 6.95 × 2.44, its face 0.05 off the south wall (Hang.json); its linen lining 2 mm, behind 3 mm of weave. */
	inline constexpr double TapestryHalf = 3.475;
	inline constexpr double TapestryFoot = 1.000;
	inline constexpr double TapestryFace = 17.950;
	inline constexpr double TapestryWeave = 0.003;
	inline constexpr double LiningLinen = 0.002;
	/** The batten's brackets: painted flat iron 0.04 wide, 8 mm thick, a plate 0.10 up the wall and an arm under the batten. */
	inline constexpr double BattenBracketHalf = 0.020;
	inline constexpr double BattenBracketMeters = 0.008;
	inline constexpr double BattenBracketPlate = 0.100;

	// ---- §2 anchors ---------------------------------------------------------------------------------------------------------------
	/** The band's Glazing anchors: origins at x 4.80 to -3.20, the upper row's at 10.90 and the lower's at 9.00, y 18.30. */
	inline constexpr int32 BandColumns = 6;
	/** The lancets' anchors: 0.70 off the bay's centre line at the sill. */
	inline constexpr double LancetAnchorZ = 5.000;
}
