#pragma once

#include "CoreMinimal.h"

/**
 * The Hall of Light's plan: every number the hall is built to, in metres in the stage's frame (origin on the hall's
 * axis, on the finished floor, on the Rotunda drum's outer face: the west port's threshold point; x east, y south,
 * z up), taken from its design sheet, Docs/Stages/hall-of-light.md (the section is given with each group). The sheet's
 * own sources are the as-built hall (windows/, D-017, every old x less 11.200) and the board. Finishes, light, planting
 * and hanging are the schedules' (Data/Stages/hall-of-light), never here.
 *
 * Two measures run through the sheet: the depth d of a member along a glass wall, in from the glass line towards the
 * axis (a north member stands at y = -(4.50 - d), a south one at +(4.50 - d)); and the vault angle theta from the crown,
 * positive south, so a point at theta and radius rho is (y, z) = (rho sin theta, 2.20 + rho cos theta).
 */
namespace Musee::HallOfLight::Plan
{
	/** The stage's key (sec. 1). */
	inline constexpr const TCHAR* Stage = TEXT("hall-of-light");

	// ---- sec. 1 the frame's place in the museum and the neighbours' axes --------------------------------------------------
	/** museum = local + (11.200, 0, 0), yaw 0. */
	inline constexpr double MuseumX = 11.200;
	/** The Rotunda's axis (local (-11.200, 0)) and its drum's outer face, the hall's west port face. */
	inline constexpr double RotundaAxisX = -11.200;
	inline constexpr double WestFaceRadius = 11.200;
	/** The Atrium's axis (local (42.800, 0), museum (54.000, 0)) and its stone base's face, the hall's east port face. */
	inline constexpr double AtriumAxisX = 42.800;
	inline constexpr double EastFaceRadius = 14.800;
	/** The two end linings, 50 mm of travertine on the port faces (sec. 3.1): inner faces R 11.250 and R 14.850. */
	inline constexpr double LiningMeters = 0.050;
	/** The hall's length on the axis, between the port faces (sec. 1, sec. 2). */
	inline constexpr double EastPortX = 28.000;

	// ---- sec. 1 datums across the hall ----------------------------------------------------------------------------------
	/** The glass line, |y|. */
	inline constexpr double GlassLine = 4.500;
	inline constexpr double PlinthTop = 0.300;
	/** The wall glass stands on setting blocks at the shoe's bed and runs to its head under the eaves. */
	inline constexpr double WallGlassFoot = 0.310;
	inline constexpr double WallGlassHead = 4.850;
	inline constexpr double EavesBottom = 4.860;
	inline constexpr double EavesTop = 5.100;
	/** The vault: radius 5.30 about (y 0, z 2.20); its crown 7.50. */
	inline constexpr double VaultRadius = 5.300;
	inline constexpr double VaultAxisZ = 2.200;
	inline constexpr double VaultCrown = VaultAxisZ + VaultRadius;
	/**
	 * The purlins' outer lines lie where the vault crosses the Atrium's coping at z 6.00 (theta +-44.19 deg); the others
	 * divide that angle in thirds, so the purlins stand at 0, +-14.73, +-29.46 and +-44.19 deg
	 * (windows/.../HallOfLightStructure.cpp:93-104).
	 */
	inline constexpr double CopingLineZ = 6.000;
	inline constexpr int32 PurlinThirds = 3;

	// ---- sec. 1 rhythms along the hall -------------------------------------------------------------------------------------
	/** Posts k 0...9 at -0.20 + 3k. */
	inline constexpr int32 Posts = 10;
	inline constexpr double FirstPostX = -0.200;
	inline constexpr double PostStep = 3.000;
	/** Arches k 0...17 at 1.30 + 1.5k: odd arches over posts, even ones over print bays. */
	inline constexpr int32 Arches = 18;
	inline constexpr double FirstArchX = 1.300;
	inline constexpr double ArchStep = 1.500;
	/** Print bays 1...9 at 1.30 + 3(b - 1); bay 10 at 27.75. Bays 1-8 hold prints, 9 and 10 the autochromes. */
	inline constexpr int32 PrintBays = 8;
	inline constexpr double FirstBayX = 1.300;
	inline constexpr double BayStep = 3.000;
	inline constexpr double LastBayX = 27.750;
	/** The five flush viewing stones on the axis at 2.80 + 6k. */
	inline constexpr int32 ViewingStones = 5;
	inline constexpr double FirstViewingX = 2.800;
	inline constexpr double ViewingStep = 6.000;
	/** The four stereo stones at 5.80 + 6k, off the axis by 2.40, north first and then alternating. */
	inline constexpr int32 StereoStones = 4;
	inline constexpr double FirstStereoX = 5.800;
	inline constexpr double StereoStep = 6.000;
	inline constexpr double StereoOffAxis = 2.400;

	// ---- sec. 1 footprint --------------------------------------------------------------------------------------------------
	/** F1-F2 and F13-F14: the gardens' outer edges, |y| 14.25, from x -0.30 (F1, F14) to 29.90 (F2, F13). */
	inline constexpr double FootprintHalf = 14.250;
	inline constexpr double FootprintWestX = -0.300;
	inline constexpr double FootprintEastX = 29.900;
	/** The hall's body passes the gardens' west and east edges at |y| 4.80, the kerbs' outer face. */
	inline constexpr double HallHalf = 4.800;
	/** The Atrium's socle, R 15.30 about its axis; its portal's front at x 27.80, |y| 6.05 (F4-F6, F9-F11). */
	inline constexpr double SocleRadius = 15.300;
	inline constexpr double PortalFrontX = 27.800;
	inline constexpr double PortalHalf = 6.050;
	/** The gardens' soil and the birches' tops with their sway (sec. 1, vertical extent). */
	inline constexpr double FootprintBottom = -1.000;
	inline constexpr double FootprintTop = 12.000;
	/**
	 * The footprint's arcs are set out at Form's chord rule, the stations no more than 1 mm off the true circle
	 * (FormMaking's DefaultChordToleranceMeters), so the outline lies on the curves Form draws.
	 */
	inline constexpr double ChordToleranceMeters = 0.001;

	// ---- sec. 2 ports -------------------------------------------------------------------------------------------------------
	inline constexpr const TCHAR* WestClass = TEXT("doorway.3000x6000.arched");
	inline constexpr const TCHAR* EastClass = TEXT("doorway.4000x4500.square");
	/** doorway.3000x6000.arched: clear 3.000, a semicircular head springing at 4.500, crown 6.000 (MuseePlan.h:44-47). */
	inline constexpr double WestClearWidth = 3.000;
	inline constexpr double WestSpringing = 4.500;
	/** doorway.4000x4500.square: clear 4.000, a square head at 4.500 (MuseePlan.h:171, AtriumBaseStructure.cpp:66-67). */
	inline constexpr double EastClearWidth = 4.000;
	inline constexpr double EastHead = 4.500;
	/** The rough openings stand Site's allowance of 0.050 round the class's clear opening, their sills 0.050 below the threshold. */
	inline constexpr double RoughAllowance = 0.050;

	// ---- sec. 3.1 masonry ----------------------------------------------------------------------------------------------------
	inline constexpr double FloorBottom = -0.100;
	/** The floor's slabs, 1.20 along the hall by 0.80 across, an unstaggered grid with a joint on the axis, 2.5 mm joints. */
	inline constexpr double FloorSlabLength = 1.200;
	inline constexpr double FloorSlabWidth = 0.800;
	inline constexpr double FloorJoint = 0.0025;
	/** The floor runs between the plinths' inner faces, d 0.18. */
	inline constexpr double FloorHalf = GlassLine - 0.180;
	/** A viewing stone: a disc R 0.450 from -0.100 to -0.030 and R 0.400 above it to the floor; its bronze ring fills the rebate. */
	inline constexpr double ViewingDiscRadius = 0.450;
	inline constexpr double ViewingStoneRadius = 0.400;
	inline constexpr double ViewingRebateZ = -0.030;

	/** The plinth on the glass line: d +-0.180, from -0.100 to 0.300, its top arrises chamfered 10 mm. */
	inline constexpr double PlinthHalf = 0.180;
	inline constexpr double PlinthBottom = -0.100;
	inline constexpr double PlinthChamfer = 0.010;
	/** The step lights' housings at every post x: x +-0.050, z 0.110-0.170, from the plinth's inner face (d 0.180) back to d 0.060. */
	inline constexpr double HousingHalf = 0.050;
	inline constexpr double HousingBottom = 0.110;
	inline constexpr double HousingTop = 0.170;
	inline constexpr double HousingBackD = 0.060;

	/** The planter kerbs outside the glass: d -0.300 to -0.180 (|y| 4.68-4.80), z -0.120 to 0.060, a 10 mm outer chamfer. */
	inline constexpr double KerbOuterD = -0.300;
	inline constexpr double KerbInnerD = -0.180;
	inline constexpr double KerbBottom = -0.120;
	inline constexpr double KerbTop = 0.060;
	inline constexpr double KerbChamfer = 0.010;

	/**
	 * The linings' outline in (y, z), mirrored: 4.800 wide to z 0.060 (the kerbs' ends), 4.680 to 0.300 (the plinths'),
	 * 4.600 to 5.100 (the eaves'), then the arc R 5.420 about (0, 2.200) to the crown at 7.620.
	 */
	inline constexpr double LiningBottom = -0.120;
	inline constexpr double LiningKerbHalf = 4.800;
	inline constexpr double LiningPlinthHalf = 4.680;
	inline constexpr double LiningEavesHalf = 4.600;
	inline constexpr double LiningArcRadius = 5.420;
	inline constexpr double LiningCrown = VaultAxisZ + LiningArcRadius;
	/** The east lining's lunette, {z >= 6.200, r <= 5.190 about (0, 2.200)}, its sill clear of the Atrium's coping. */
	inline constexpr double LunetteSill = 6.200;
	inline constexpr double LunetteRadius = 5.190;

	/** The stereo stones, turned: R 0.350, 1.000 high, a 15 mm chamfer round the top. */
	inline constexpr double StereoStoneRadius = 0.350;
	inline constexpr double StereoStoneHeight = 1.000;
	inline constexpr double StereoStoneChamfer = 0.015;

	// ---- sec. 3.2 steelwork (pearl steel) ------------------------------------------------------------------------------------
	/** One point of a drawn section: across (x rel. a post, or d), and up (d, or z). */
	struct FSectionPoint
	{
		double A;
		double B;
	};

	/** The posts: an I in (x rel. post, d), the pressure cap outside, the web through the glass line, the flange inside. */
	inline constexpr FSectionPoint PostSection[] = {{-0.030, -0.030}, {0.030, -0.030}, {0.030, -0.010}, {0.010, -0.010}, {0.010, 0.140},
		{0.030, 0.140}, {0.030, 0.155}, {-0.030, 0.155}, {-0.030, 0.140}, {-0.010, 0.140}, {-0.010, -0.010}, {-0.030, -0.010}};
	inline constexpr double PostHalf = 0.030;
	inline constexpr double PostWebHalf = 0.010;
	/** The posts bear on the plinth and meet the eaves' underside. */
	inline constexpr double PostFoot = PlinthTop;
	inline constexpr double PostHead = EavesBottom;

	/** The glazing shoes: a U in (d, z) on the plinth top, its bed at 0.310, its cheeks 15 mm either side of the glass. */
	inline constexpr double ShoeHalf = 0.025;
	inline constexpr double ShoeCheekInner = 0.010;
	inline constexpr double ShoeBottom = PlinthTop;
	inline constexpr double ShoeBed = WallGlassFoot;
	inline constexpr double ShoeTop = 0.340;

	/** The transoms inside the glass, d 0.012-0.110, z 3.170-3.230, in segments between the post webs. */
	inline constexpr double TransomFromD = 0.012;
	inline constexpr double TransomToD = 0.110;
	inline constexpr double TransomBottom = 3.170;
	inline constexpr double TransomTop = 3.230;

	/** The eaves box in (d, z): 0.30 x 0.24 with a 50 mm chamfer on its inner top arris. */
	inline constexpr FSectionPoint EavesSection[] = {{-0.100, 4.860}, {0.200, 4.860}, {0.200, 5.050}, {0.150, 5.100}, {-0.100, 5.100}};

	/** The end posts, fixed to the linings: 0.070 along x from the lining's inner face, d -0.040 to 0.120. */
	inline constexpr double EndPostAlong = 0.070;
	inline constexpr double EndPostFromD = -0.040;
	inline constexpr double EndPostToD = 0.120;

	/** One plate of the arches' I (b out from R 5.30, half its width along x): the inner flange, the web, the cap over the glass. */
	struct FPlate
	{
		double FromB;
		double ToB;
		double Half;
	};

	inline constexpr FPlate ArchFlange{-0.150, -0.130, 0.035};
	inline constexpr FPlate ArchWeb{-0.130, 0.025, 0.012};
	inline constexpr FPlate ArchCap{0.025, 0.045, 0.035};

	/** The purlins: across +-0.030, b -0.120 to +0.015, in segments between the arch webs and to the collars. */
	inline constexpr double PurlinHalf = 0.030;
	inline constexpr double PurlinFromB = -0.120;
	inline constexpr double PurlinToB = 0.015;

	/**
	 * The crosses, one in each panel of the 17 bays between arches: bar A (+-0.015, b -0.080 to -0.020) runs node to node;
	 * bar B (+-0.015, b -0.090 to -0.030) is cut at the crossing and welded to A. Each stops at least 0.025 short of its
	 * nodes.
	 */
	inline constexpr double BarHalf = 0.015;
	inline constexpr FPlate BarA{-0.080, -0.020, BarHalf};
	inline constexpr FPlate BarB{-0.090, -0.030, BarHalf};
	inline constexpr double BarShortOfNode = 0.025;

	/** The collars on the linings: 0.080 in from the lining's face, b -0.110 to -0.012; the vault glass's end rests on them. */
	inline constexpr double CollarAlong = 0.080;
	inline constexpr double CollarFromB = -0.110;
	inline constexpr double CollarToB = -0.012;

	/**
	 * The lunette's mullions: a T 0.100 deep from the east lining's face and +-0.030 across, at the purlins' y. The sheet
	 * gives the T's envelope; its plates are the wall posts' (sec. 3.2): a 20 mm flange on the lining's face (the posts'
	 * cap, d -0.030 to -0.010) and a stem +-0.010 thick (the posts' web).
	 */
	inline constexpr double MullionDepth = 0.100;
	inline constexpr double MullionHalf = 0.030;
	inline constexpr double MullionFlange = 0.020;
	inline constexpr double MullionStemHalf = PostWebHalf;
	inline constexpr double MullionY[] = {-2.607, -1.348, 0.000, 1.348, 2.607};

	// ---- sec. 3.3 glazing ----------------------------------------------------------------------------------------------------
	/** Every pane: low-iron laminate 8 + 1.52 UV-filtering PVB + 8. */
	inline constexpr double GlassMeters = 0.008 + 0.00152 + 0.008;
	/**
	 * The gasket between a pane's edge and the steel it stands against: a wall pane is 3.000 less the web (0.020) and its
	 * gaskets, 2.970, so 5 mm each side; a vault pane 1.500 less the web (0.024) and its gaskets, 1.466, so 5 mm again.
	 */
	inline constexpr double GasketMeters = 0.005;

	// ---- sec. 3.4 bronze ------------------------------------------------------------------------------------------------------
	/** The prints' stems, two per print at the bay's x +-0.300: flat bars x +-0.015, d 0.098-0.120, z 0.320 to 2.000. */
	inline constexpr double StemOffset = 0.300;
	inline constexpr double StemHalf = 0.015;
	inline constexpr double StemFromD = 0.098;
	inline constexpr double StemToD = 0.120;
	inline constexpr double StemBottom = 0.320;
	inline constexpr double StemTop = 2.000;
	/** Their shoes on the plinth top: x +-0.035, d 0.080-0.128, z 0.300-0.320. */
	inline constexpr double StemShoeHalf = 0.035;
	inline constexpr double StemShoeFromD = 0.080;
	inline constexpr double StemShoeToD = 0.128;
	/** The autochromes' glazing bars at the bay's x +-0.560: x +-0.0125, d 0.035-0.055, from the shoe's bed to the transom. */
	inline constexpr double BarOffset = 0.560;
	inline constexpr double GlazingBarHalf = 0.0125;
	inline constexpr double GlazingBarFromD = 0.035;
	inline constexpr double GlazingBarToD = 0.055;
	/**
	 * The stereo readers: a plate 0.680 x 0.400 x 0.030 centred 1.320 over its stone, facing the axis and tilted back
	 * 0.5 rad (28.65 deg; HallOfLight.swift:156-195); a stem 30 mm across from the stone's top into the plate's back,
	 * 0.016 behind the stone's centre.
	 */
	inline constexpr double ReaderWidth = 0.680;
	inline constexpr double ReaderHeight = 0.400;
	inline constexpr double ReaderThickness = 0.030;
	inline constexpr double ReaderCentreZ = 1.320;
	inline constexpr double ReaderTiltRadians = 0.5;
	inline constexpr double ReaderStemRadius = 0.015;
	inline constexpr double ReaderStemBehind = 0.016;

	// ---- sec. 2 anchors --------------------------------------------------------------------------------------------------------
	/** The prints hang on their mounts' face plane (d 0.150), the autochromes on their cassettes' face (d 0.060). */
	inline constexpr double PrintsD = 0.150;
	inline constexpr double AutochromesD = 0.060;
	/** The gardens' ground: x 0.10 to 29.70, |y| 4.80 to 13.75, at z 0. */
	inline constexpr double GardenWestX = 0.100;
	inline constexpr double GardenEastX = 29.700;
	inline constexpr double GardenInner = HallHalf;
	inline constexpr double GardenOuter = 13.750;
	/** The birches in the meadow (sec. 7), plan points (x, y). */
	inline constexpr double Birches[][2] = {{2.80, -8.00}, {5.30, -11.50}, {9.80, -7.20}, {13.30, -11.20}, {16.80, -8.30}, {20.80, -11.60},
		{24.30, -7.60}, {27.30, -11.00}};
	/** The cherries (odd) and apples (even) in the orchard (sec. 7). */
	inline constexpr double FruitTrees[][2] = {{3.30, 7.80}, {9.30, 7.80}, {15.30, 7.80}, {21.30, 7.80}, {26.80, 7.80}, {4.80, 11.60},
		{10.80, 11.60}, {16.80, 11.60}, {22.80, 11.60}, {28.30, 11.60}};
	/** The clipped yew hedges on the lines |y| 14.00 from x -0.20 to 29.80. */
	inline constexpr double HedgeLine = 14.000;
	inline constexpr double HedgeWestX = -0.200;
	inline constexpr double HedgeEastX = 29.800;
}
