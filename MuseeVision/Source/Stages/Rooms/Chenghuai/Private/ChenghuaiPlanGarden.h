#pragma once

#include "ChenghuaiPlan.h"
#include "CoreMinimal.h"

/**
 * The garden's plan (§3.9; ChenghuaiGardenPlan.h:12-105, ChenghuaiGarden.cpp, ChenghuaiRockery.cpp, all y + 17.000): its
 * ground, the pond's edge and bed over the Classical Hall's tribune, the edge rocks, the paths' centre lines, the zigzag
 * bridge, the rockery's outlines and massing, the hexagonal pavilion on its summit, the covered walk's engraved stones, the
 * flower hall's terrace and the water pavilion's base.
 */
namespace Musee::Chenghuai::Plan
{
	// ---- the ground ------------------------------------------------------------------------------------------------------
	/** The garden's ground (Nature's bed) at 0.02 over x 2.30 … 16.40, y -0.50 … -44.40. */
	inline constexpr double GardenGround = 0.020;
	inline constexpr double GardenX0 = 2.300;
	inline constexpr double GardenX1 = 16.400;
	inline constexpr double GardenY0 = -0.500;
	inline constexpr double GardenY1 = -44.400;

	// ---- the pond (ChenghuaiGarden.cpp:79-149) ---------------------------------------------------------------------------
	/** The bank runs from the edge at the ground 0.45 inward and down to the bed; the bed at -0.85, rising to -0.46 over the
	 *  tribune: bed(r) = lerp(-0.46, -0.85, clamp((r - 8.30) / 1.50, 0, 1)), r from (0, -22.30). Water at -0.28. */
	inline constexpr double BankReach = 0.450;
	inline constexpr double BedDeep = -0.850;
	inline constexpr double BedShallow = -0.460;
	inline constexpr double BedRamp = 1.500;
	inline constexpr double WaterLevel = -0.280;
	/** The bed is a slab 0.10 thick under its deep part (to the footprint's -0.95), bottomed at -0.50 over the tribune; the
	 *  bank's block runs 0.10 behind the edge. */
	inline constexpr double BedSlab = 0.100;
	inline constexpr double BankBack = 0.100;
	/** The bed is turned about the tribune's axis 5 cm and a degree of arc past the pond each way. */
	inline constexpr double BedMargin = 0.050;
	inline constexpr double BedMarginDegrees = 1.000;

	/** The edge rocks (湖石驳岸, ChenghuaiRockery.cpp:178-215): every 0.60 … 0.95 a broad stone R 0.42 (±20 %) from the water
	 *  - 0.35 to 0.00 … +0.06; on 55 % an upper stone R 0.30 from -0.03 to +0.12 … +0.42; each a 7-sided outline, its radii
	 *  0.70 … 1.15 of its own (as built), stretched 1.3 along the bank. */
	inline constexpr double RockSpacing0 = 0.600;
	inline constexpr double RockSpacing1 = 0.950;
	inline constexpr double BroadRock = 0.420;
	inline constexpr double UpperRock = 0.300;
	inline constexpr double RockSpread = 0.200;
	inline constexpr double BroadUnderWater = 0.350;
	inline constexpr double BroadTop1 = 0.060;
	inline constexpr double UpperShare = 0.550;
	inline constexpr double UpperBottom = -0.030;
	inline constexpr double UpperTop0 = 0.120;
	inline constexpr double UpperTop1 = 0.420;
	inline constexpr int32 RockSides = 7;
	inline constexpr double RockRadius0 = 0.700;
	inline constexpr double RockRadius1 = 1.150;
	inline constexpr double RockStretch = 1.300;

	// ---- the paths (花街铺地, :154-183) ---------------------------------------------------------------------------------
	/** A path 0.95 wide: a pebble bed 0.83 wide, -0.04 … 0.04, between bluestone kerbs 0.06 wide, -0.06 … 0.055, on the ground. */
	inline constexpr double PathBedHalf = 0.415;
	inline constexpr double PathBedBottom = -0.040;
	inline constexpr double PathBedTop = 0.040;
	inline constexpr double PathKerb = 0.060;
	inline constexpr double PathKerbBottom = -0.060;
	inline constexpr double PathKerbTop = 0.055;
	/** Path 0, the moon gate to the terrace, straight at y -37.452 from x 3.699 to 8.102, dividing round 石兄 on the circle
	 *  r 1.475 about (5.300, -37.300) between x 3.833 and 6.767; path 3, the bridge to the walk, straight at y -27.898. */
	inline constexpr double Path0Y = -37.452;
	inline constexpr double Path0X0 = 3.699;
	inline constexpr double Path0X1 = 8.102;
	inline constexpr double ForkX0 = 3.833;
	inline constexpr double ForkX1 = 6.767;
	inline constexpr double ForkRadius = 1.475;
	inline constexpr double ForkX = 5.300;
	inline constexpr double ForkY = -37.300;
	inline constexpr double Path3Y = -27.898;
	inline constexpr double Path3X0 = 5.199;
	inline constexpr double Path3X1 = 3.699;

	// ---- the zigzag bridge (:187-213) --------------------------------------------------------------------------------------
	/** Slabs 1.00 wide × 0.14, top 0.10, each run 0.50 past its turns; piers 0.50 square at no more than 1.60 centres, from
	 *  0.10 under the local bed (never below -0.50 over the tribune); a low kerb 0.10 × 0.08 on both edges of every run. */
	inline constexpr FPlanPoint Bridge[] = {{5.200, -27.900}, {7.200, -27.900}, {8.200, -26.500}, {10.400, -26.500}, {11.400, -27.500}, {13.000, -27.500}};
	inline constexpr double BridgeWidth = 1.000;
	inline constexpr double BridgeSlab = 0.140;
	inline constexpr double BridgeTop = 0.100;
	inline constexpr double BridgePast = 0.500;
	inline constexpr double BridgePier = 0.500;
	inline constexpr double BridgePierPitch = 1.600;
	inline constexpr double BridgePierUnderBed = 0.100;
	inline constexpr double BridgeKerb = 0.100;
	inline constexpr double BridgeKerbHigh = 0.080;

	// ---- the covered walk's engraved stones, the terrace, the water pavilion's base ----------------------------------------
	/** Twelve 书条石 0.90 × 0.32, 0.03 proud of the wall (x 2.29 … 2.33), z 1.17 … 1.49, two to a bay: for each mark B the
	 *  stones run y B + 0.35 … B + 1.25 and B + 1.55 … B + 2.45. */
	inline constexpr double StoneBays[] = {-36.250, -25.600, -22.900, -20.200, -17.500, -12.100};
	inline constexpr double StoneX0 = 2.290;
	inline constexpr double StoneX1 = 2.330;
	inline constexpr double StoneZ0 = 1.170;
	inline constexpr double StoneZ1 = 1.490;
	inline constexpr double StoneOffsets[] = {0.350, 1.550};
	inline constexpr double StoneLength = 0.900;
	/** The terrace (月台) before the flower hall: x 8.10 … 13.20, y -36.70 … -38.20, top 0.30. */
	inline constexpr FPlanRect Terrace{8.100, 13.200, -36.700, -38.200};
	inline constexpr double TerraceTop = 0.300;
	/** The water pavilion's west part stands in the pond on a base from the bed: x 13.00 … 14.30 under its platform. */
	inline constexpr double PavilionBaseX1 = 14.300;

	// ---- the rockery (假山, ChenghuaiRockery.cpp:22-172) -------------------------------------------------------------------
	/** Massing: 1.2 smoothstep(-0.2, 1.8, -d_foot) + 1.05 smoothstep(-0.4, 1.3, -d_upper) + 0.5 smoothstep(0, 1.4, 3.2 - r_p). */
	inline constexpr double FootRise = 1.200;
	inline constexpr double FootFrom = -0.200;
	inline constexpr double FootTo = 1.800;
	inline constexpr double UpperRise = 1.050;
	inline constexpr double UpperFrom = -0.400;
	inline constexpr double UpperTo = 1.300;
	inline constexpr double CrownRise = 0.500;
	inline constexpr double CrownTo = 1.400;
	inline constexpr double CrownReach = 3.200;
	/** Stones a jittered Voronoi of 0.95 cells (their seeds 0.1 … 0.9 across each cell), each lifted (hash - 0.45) × 0.55,
	 *  leaning at most ±0.15 per metre, crevices 0.16 deep within 0.24 of its edge. */
	inline constexpr double StoneCell = 0.950;
	inline constexpr double StoneJitter0 = 0.100;
	inline constexpr double StoneJitter1 = 0.800;
	inline constexpr double StoneLiftFrom = 0.450;
	inline constexpr double StoneLift = 0.550;
	inline constexpr double StoneLean = 0.150;
	inline constexpr double Crevice = 0.160;
	inline constexpr double CreviceWidth = 0.240;
	/** At the foot the stones settle into the ground (none floats): the mass takes the stones between 0.05 and 0.45 of height,
	 *  never under a fifth of its massing, and sinks 0.06 per metre beyond 0.2 outside the foot, to 0.3 under (as built). */
	inline constexpr double SettleFrom = 0.050;
	inline constexpr double SettleTo = 0.450;
	inline constexpr double MassFloor = 0.200;
	inline constexpr double SinkOutside = 0.200;
	inline constexpr double Sink = 0.060;
	inline constexpr double FootOutside = 0.300;
	/** The summit's plateau at 2.60 within r 2.05 of the pavilion's centre, blending to r 2.60. */
	inline constexpr double Summit = 2.600;
	inline constexpr double SummitFlat = 2.050;
	inline constexpr double SummitBlend = 2.600;
	/** The path: half-width 0.55; a ledge at 2.6 × min(up, down) quantised to 0.15 risers (+0.02), each run reaching the
	 *  summit 1.9 short of its end there (as built); the stone within 0.40 beside it stands 0.20 higher or falls away. */
	inline constexpr double PathHalf = 0.550;
	inline constexpr double PathRiser = 0.150;
	inline constexpr double PathOver = 0.020;
	inline constexpr double PathLead = 0.030;
	inline constexpr double PathShort = 1.900;
	inline constexpr double PathBeside = 0.400;
	inline constexpr double PathKerbOver = 0.200;
	/** Sampled every 0.10 over x 4.6 … 16.2, y -2.7 … -19.8. */
	inline constexpr double RockeryGrid = 0.100;
	inline constexpr double RockeryX0 = 4.600;
	inline constexpr double RockeryX1 = 16.200;
	inline constexpr double RockeryY0 = -19.800;
	inline constexpr double RockeryY1 = -2.700;
	/** The rockery's block reaches 0.02 under the datum: the flat margin of its field lies under the garden's ground. */
	inline constexpr double RockeryBack = 0.020;

	// ---- the hexagonal pavilion (亭 · 見山, :401-558) --------------------------------------------------------------------
	/** Centre (10.60, -11.30), corners at φ 60° k; a platform hexagon R 1.70 of bluestone from 2.30 to the floor at 2.75. */
	inline constexpr double HexX = 10.600;
	inline constexpr double HexY = -11.300;
	inline constexpr double HexPlatform = 1.700;
	inline constexpr double HexPlatformBottom = 2.300;
	inline constexpr double HexFloor = 2.750;
	/** Six columns at R 1.45, r 0.09 → 0.0855, 2.35 high, on bases 0.28 square (0.02 under the floor to 0.03 over it). */
	inline constexpr double HexColumns = 1.450;
	inline constexpr double HexColumnR = 0.090;
	inline constexpr double HexColumnTop = 0.0855;
	inline constexpr double HexColumnH = 2.350;
	inline constexpr double HexBase = 0.280;
	inline constexpr double HexBaseUnder = 0.020;
	inline constexpr double HexBaseOver = 0.030;
	/** Lintels 0.12 × 0.20 centred floor + 2.25; step-brocade fretwork 0.28 high from floor + 1.87 on faces 1, 2, 4 and 5. */
	inline constexpr double HexLintelWide = 0.120;
	inline constexpr double HexLintelHigh = 0.200;
	inline constexpr double HexLintelZ = 2.250;
	inline constexpr double HexFretworkZ = 1.870;
	inline constexpr double HexFretwork = 0.280;
	/** The fretwork clears each column by 0.02 (as built). */
	inline constexpr double HexFretworkClear = 0.020;
	inline constexpr int32 HexSeatedFaces[] = {1, 2, 4, 5};
	/** A seat: a plank 0.36 × 0.05 at floor + 0.44 on a base 0.30 × 0.40, 0.18 inside the face; a goose-neck back (美人靠) of
	 *  balusters 0.035 square about every 0.13, rising 0.42 from floor + 0.46 and leaning out 0.02 + 0.20 sin(1.9 t); its back
	 *  rail 0.07 × 0.05 at floor + 0.89, 0.19 outside. The seat's ends clear the columns by 0.01 (as built). */
	inline constexpr double SeatPlank = 0.360;
	inline constexpr double SeatPlankThick = 0.050;
	inline constexpr double SeatZ = 0.440;
	inline constexpr double SeatBaseWide = 0.300;
	inline constexpr double SeatBaseHigh = 0.400;
	inline constexpr double SeatInside = 0.180;
	inline constexpr double SeatClear = 0.010;
	inline constexpr double Baluster = 0.035;
	inline constexpr double BalusterPitch = 0.130;
	inline constexpr double BalusterFoot = 0.460;
	inline constexpr double BalusterRise = 0.420;
	inline constexpr double BalusterLean0 = 0.020;
	inline constexpr double BalusterLean = 0.200;
	inline constexpr double BalusterBow = 1.900;
	inline constexpr int32 BalusterStations = 7;
	inline constexpr double BackRailWide = 0.070;
	inline constexpr double BackRailHigh = 0.050;
	inline constexpr double BackRailZ = 0.890;
	inline constexpr double BackRailOut = 0.190;
	/** The roof: eave corners at R 2.35; its surface z = base + (apex - base)(1 - f)^1.35 + 0.45 c³ f², base 5.45, apex 6.85;
	 *  a soffit 0.14 under the bed; hips rising 0.12 t⁶ at the corners; corner beams 0.10 × 0.16; the finial from 6.73. */
	inline constexpr double HexEave = 2.350;
	inline constexpr double HexRoofBase = 5.450;
	inline constexpr double HexApex = 6.850;
	inline constexpr double HexRoofPower = 1.350;
	inline constexpr double HexCornerLift = 0.450;
	inline constexpr double HexSoffit = 0.140;
	inline constexpr double HipLift = 0.120;
	inline constexpr double HipOver = 0.040;
	inline constexpr double HipFrom = 0.060;
	inline constexpr int32 HipStations = 17;
	inline constexpr double CornerBeamWide = 0.100;
	inline constexpr double CornerBeamHigh = 0.160;
	/** The corner beam runs under the hip from 0.55 of the way out to 1.02, 0.08 then 0.02 under the soffit (as built). */
	inline constexpr double CornerBeamFrom = 0.550;
	inline constexpr double CornerBeamTo = 1.020;
	inline constexpr double CornerBeamUnder0 = 0.080;
	inline constexpr double CornerBeamUnder1 = 0.020;
	inline constexpr double FinialZ = 6.730;
	inline constexpr FSectionPoint HipTop[] = {{-0.08, 0.08}, {-0.05, 0.12, true}, {0.0, 0.13, true}, {0.05, 0.12, true}, {0.08, 0.08}};
	inline constexpr double HipUnder = -0.060;
	inline constexpr FSectionPoint FinialSection[] = {{0.0, 0.0}, {0.2, 0.0}, {0.2, 0.08}, {0.13, 0.14, true}, {0.17, 0.30, true}, {0.12, 0.45, true},
		{0.05, 0.58, true}, {0.0, 0.62}};

	// ---- the outlines (y + 17.000) -----------------------------------------------------------------------------------------
	/** The pond's edge, 75 points (the water pavilion's notch applied). */
	inline constexpr FPlanPoint PondEdge[] = {
		{14.199, -35.500}, {13.896, -35.745}, {13.369, -35.943}, {12.712, -36.098}, {12.023, -36.216}, {11.398, -36.301},
		{10.833, -36.352}, {10.261, -36.364}, {9.692, -36.341}, {9.136, -36.285}, {8.602, -36.199}, {8.074, -36.088},
		{7.549, -35.950}, {7.047, -35.778}, {6.590, -35.564}, {6.199, -35.301}, {5.871, -34.974}, {5.590, -34.589},
		{5.363, -34.168}, {5.198, -33.731}, {5.102, -33.301}, {5.116, -32.877}, {5.239, -32.445}, {5.404, -32.005},
		{5.547, -31.557}, {5.602, -31.102}, {5.534, -30.640}, {5.390, -30.173}, {5.220, -29.697}, {5.073, -29.207},
		{5.000, -28.699}, {4.997, -28.159}, {5.030, -27.590}, {5.105, -27.011}, {5.225, -26.441}, {5.398, -25.898},
		{5.622, -25.370}, {5.895, -24.842}, {6.216, -24.339}, {6.585, -23.883}, {7.000, -23.500}, {7.485, -23.185},
		{8.042, -22.923}, {8.635, -22.717}, {9.233, -22.575}, {9.801, -22.500}, {10.352, -22.499}, {10.911, -22.568},
		{11.454, -22.697}, {11.958, -22.878}, {12.398, -23.102}, {12.793, -23.378}, {13.160, -23.714}, {13.469, -24.092},
		{13.692, -24.493}, {13.801, -24.898}, {13.735, -25.315}, {13.516, -25.759}, {13.230, -26.214}, {12.963, -26.666},
		{12.801, -27.102}, {12.767, -27.474}, {12.804, -27.795}, {12.877, -28.130}, {12.954, -28.544}, {13.221, -29.102},
		{14.300, -29.882}, {14.300, -30.845}, {14.300, -31.867}, {14.300, -32.827}, {13.212, -33.602}, {13.227, -34.165},
		{13.585, -34.600}, {13.950, -34.943}, {14.197, -35.231},
	};
	/** The rockery's foot, 64 points. */
	inline constexpr FPlanPoint RockeryFoot[] = {
		{15.699, -18.398}, {15.242, -18.817}, {14.544, -19.157}, {13.724, -19.392}, {12.898, -19.500}, {12.057, -19.405},
		{11.150, -19.144}, {10.242, -18.860}, {9.398, -18.699}, {8.622, -18.776}, {7.881, -18.987}, {7.200, -19.155},
		{6.602, -19.102}, {6.060, -18.743}, {5.575, -18.187}, {5.203, -17.538}, {5.000, -16.898}, {5.073, -16.278},
		{5.362, -15.626}, {5.671, -14.960}, {5.801, -14.301}, {5.627, -13.661}, {5.277, -13.026}, {4.939, -12.378},
		{4.801, -11.699}, {4.991, -10.969}, {5.382, -10.201}, {5.783, -9.432}, {6.000, -8.699}, {5.899, -8.018},
		{5.608, -7.364}, {5.338, -6.729}, {5.301, -6.102}, {5.562, -5.448}, {6.006, -4.782}, {6.572, -4.175},
		{7.199, -3.699}, {7.903, -3.377}, {8.706, -3.168}, {9.555, -3.050}, {10.398, -3.000}, {11.268, -2.996},
		{12.174, -3.056}, {13.043, -3.213}, {13.801, -3.500}, {14.455, -3.948}, {15.037, -4.532}, {15.501, -5.199},
		{15.801, -5.898}, {15.847, -6.629}, {15.692, -7.413}, {15.491, -8.241}, {15.398, -9.102}, {15.488, -10.029},
		{15.664, -11.014}, {15.832, -11.993}, {15.898, -12.898}, {15.802, -13.717}, {15.606, -14.481}, {15.406, -15.203},
		{15.301, -15.898}, {15.406, -16.592}, {15.639, -17.269}, {15.802, -17.885},
	};
	/** The rockery's upper outline, 64 points. */
	inline constexpr FPlanPoint RockeryUpper[] = {
		{13.763, -15.704}, {13.479, -15.963}, {13.046, -16.172}, {12.538, -16.317}, {12.027, -16.382}, {11.505, -16.324},
		{10.941, -16.163}, {10.377, -15.987}, {9.855, -15.887}, {9.373, -15.934}, {8.914, -16.065}, {8.491, -16.169},
		{8.118, -16.134}, {7.785, -15.914}, {7.485, -15.571}, {7.254, -15.170}, {7.129, -14.774}, {7.173, -14.387},
		{7.352, -13.982}, {7.543, -13.570}, {7.624, -13.161}, {7.515, -12.764}, {7.298, -12.371}, {7.090, -11.970},
		{7.005, -11.548}, {7.124, -11.094}, {7.366, -10.618}, {7.614, -10.142}, {7.747, -9.688}, {7.685, -9.264},
		{7.503, -8.858}, {7.335, -8.463}, {7.312, -8.075}, {7.474, -7.670}, {7.750, -7.258}, {8.103, -6.882},
		{8.495, -6.586}, {8.931, -6.387}, {9.428, -6.258}, {9.955, -6.185}, {10.478, -6.156}, {11.015, -6.153},
		{11.577, -6.190}, {12.116, -6.287}, {12.586, -6.462}, {12.990, -6.740}, {13.351, -7.102}, {13.639, -7.516},
		{13.823, -7.952}, {13.853, -8.405}, {13.757, -8.891}, {13.633, -9.403}, {13.575, -9.935}, {13.630, -10.511},
		{13.739, -11.123}, {13.844, -11.730}, {13.887, -12.290}, {13.827, -12.799}, {13.704, -13.273}, {13.579, -13.721},
		{13.516, -14.151}, {13.580, -14.581}, {13.724, -15.000}, {13.826, -15.383},
	};
	/** The rockery path's centre line, west end first, 55 points. */
	inline constexpr FPlanPoint RockeryPath[] = {
		{5.602, -14.102}, {5.826, -14.017}, {6.142, -13.902}, {6.513, -13.764}, {6.904, -13.613}, {7.279, -13.456},
		{7.602, -13.301}, {7.867, -13.146}, {8.104, -12.982}, {8.325, -12.813}, {8.541, -12.642}, {8.762, -12.470},
		{9.000, -12.301}, {9.250, -12.141}, {9.504, -11.989}, {9.763, -11.837}, {10.030, -11.677}, {10.309, -11.501},
		{10.602, -11.301}, {10.920, -11.077}, {11.266, -10.840}, {11.624, -10.587}, {11.977, -10.314}, {12.309, -10.019},
		{12.602, -9.699}, {12.882, -9.345}, {13.171, -8.959}, {13.438, -8.550}, {13.652, -8.131}, {13.783, -7.711},
		{13.801, -7.301}, {13.690, -6.875}, {13.474, -6.419}, {13.175, -5.962}, {12.815, -5.536}, {12.416, -5.171},
		{12.000, -4.898}, {11.541, -4.715}, {11.015, -4.597}, {10.451, -4.538}, {9.875, -4.538}, {9.316, -4.592},
		{8.801, -4.699}, {8.312, -4.884}, {7.823, -5.152}, {7.351, -5.474}, {6.912, -5.825}, {6.522, -6.176},
		{6.199, -6.500}, {5.953, -6.822}, {5.770, -7.168}, {5.637, -7.513}, {5.540, -7.834}, {5.465, -8.105},
		{5.398, -8.301},
	};
	/** Path 1, the terrace to the water pavilion. */
	inline constexpr FPlanPoint Path1[] = {
		{13.199, -37.500}, {13.402, -37.404}, {13.694, -37.276}, {14.025, -37.116}, {14.345, -36.924}, {14.602, -36.699},
		{14.799, -36.430}, {14.970, -36.114}, {15.112, -35.773}, {15.223, -35.428}, {15.301, -35.102}, {15.329, -34.772},
		{15.312, -34.422}, {15.270, -34.088}, {15.225, -33.804}, {15.199, -33.602},
	};
	/** Path 2, the water pavilion to the bridge. */
	inline constexpr FPlanPoint Path2[] = {
		{14.801, -29.102}, {14.830, -28.959}, {14.881, -28.754}, {14.926, -28.520}, {14.941, -28.291}, {14.898, -28.102},
		{14.785, -27.946}, {14.617, -27.803}, {14.416, -27.679}, {14.203, -27.576}, {14.000, -27.500}, {13.790, -27.461},
		{13.559, -27.456}, {13.333, -27.471}, {13.138, -27.490}, {13.000, -27.500},
	};
	/** Path 4, the walk to the pond's south. */
	inline constexpr FPlanPoint Path4[] = {
		{3.699, -18.301}, {4.055, -18.579}, {4.551, -18.982}, {5.139, -19.436}, {5.770, -19.867}, {6.398, -20.199},
		{7.046, -20.418}, {7.747, -20.571}, {8.464, -20.684}, {9.161, -20.784}, {9.801, -20.898}, {10.391, -21.016},
		{10.955, -21.118}, {11.484, -21.221}, {11.968, -21.343}, {12.398, -21.500}, {12.777, -21.721}, {13.112, -21.995},
		{13.397, -22.278}, {13.628, -22.527}, {13.801, -22.699},
	};
	/** Path 5, the walk to the rockery's west foot. */
	inline constexpr FPlanPoint Path5[] = {
		{3.699, -14.301}, {3.956, -14.284}, {4.319, -14.265}, {4.743, -14.235}, {5.185, -14.184}, {5.602, -14.102},
		{6.005, -13.984}, {6.423, -13.835}, {6.839, -13.665}, {7.238, -13.484}, {7.602, -13.301}, {7.942, -13.097},
		{8.271, -12.867}, {8.571, -12.638}, {8.820, -12.440}, {9.000, -12.301},
	};
	/** Path 6, the rockery's east foot to the walk. */
	inline constexpr FPlanPoint Path6[] = {
		{3.699, -8.398}, {3.958, -8.383}, {4.340, -8.361}, {4.761, -8.337}, {5.141, -8.316}, {5.398, -8.301},
	};
}
