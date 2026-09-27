#pragma once

#include "CoreMinimal.h"

/**
 * The Cube's plan: every number the room is built to, in metres in the stage's frame (the Atrium's: origin on the Élan
 * axis at the museum's floor datum; x east, y south, z up), taken from its design sheet, Docs/Stages/cube.md (the section
 * is given with each group). The sheet's own sources are the as-built Cube (windows/Cube/CubePlan.h, CubeStructure.cpp,
 * D-017) and the Atrium's class iris.4600. Every height is local z = museum z. Finishes, light and motion are the
 * schedules' (Data/Stages/cube), never here.
 *
 * Bearings are compass bearings, clockwise from north: a point at bearing b and radius r is (r sin b, -r cos b). Form
 * turns by plan angle (from +x towards +y), the bearing less 90 degrees.
 */
namespace Musee::Cube::Plan
{
	/** The stage's key (§ head). */
	inline constexpr const TCHAR* Stage = TEXT("cube");

	// ---- §1 placement and key levels ----------------------------------------------------------------------------------
	/** The mating plane, `cube.up`: the Atrium slab's soffit. */
	inline constexpr double MatingPlane = -0.500;
	/** The room's centre, where the visitor's eyes stop. */
	inline constexpr double EyesZ = -22.000;
	/** The car's floor at its `cube` stop: the Use anchor's datum. */
	inline constexpr double CarAtCube = -23.600;
	/** The received well: the class's circle r 2.300 from the plane down to the well's bottom. */
	inline constexpr double WellRadius = 2.300;
	inline constexpr double WellBottom = -23.800;
	/** The footprint: the box's square and the height it spans. */
	inline constexpr double FootprintHalf = 16.200;
	inline constexpr double FootprintBottom = -38.200;
	inline constexpr double FootprintTop = MatingPlane;

	// ---- §3.1 the concrete box, 1.000 thick, round a 1.100 service void -------------------------------------------------
	inline constexpr double BoxInner = 15.200;
	inline constexpr double BoxOuter = 16.200;
	inline constexpr double BaseBottom = -38.200;
	inline constexpr double BaseTop = -37.200;
	inline constexpr double RoofSoffit = -6.800;
	inline constexpr double RoofTop = -5.800;
	/** The roof slab is pierced by the sleeve's outer circle. */
	inline constexpr double RoofHole = 2.750;

	// ---- §3.2 the panel wall: six faces of 0.700 modules, a module corner at each face's centre -------------------------
	inline constexpr double FaceHalf = 14.000;
	inline constexpr double Ceiling = -8.000;
	inline constexpr double Floor = -36.000;
	inline constexpr double Pitch = 0.700;
	inline constexpr int32 ModulesPerEdge = 40;
	/** Each module's face is 0.698 square: 2 mm seams between them. */
	inline constexpr double Seam = 0.002;
	/** The module's depth behind its face: a design value until Light's record gives one. */
	inline constexpr double ModuleDepth = 0.100;
	/** The ceiling's rough opening: the soffit ring's outer edge 4.120 and a 10 mm shadow gap. */
	inline constexpr double RoughOpening = 4.130;
	/** The sheet's counts: 88 positions wholly inside the opening omitted, 44 crossed by it shaped to it; 9,512 in all. */
	inline constexpr int32 OmittedModules = 88;
	inline constexpr int32 ShapedModules = 44;
	inline constexpr int32 Modules = 9512;

	// ---- §3.3 the shaft's glass, tier frames and joint strips -------------------------------------------------------------
	inline constexpr double GlassInner = 2.450;
	inline constexpr double GlassOuter = 2.510;
	/** The tiers' boundaries, top down: the glass's four tiers lie between them. */
	inline constexpr double TierLevels[] = {-0.500, -1.825, -3.150, -4.475, -5.800};
	inline constexpr int32 Tiers = 4;
	/** Each tier's pane edges, bottom and top: each laps 5 mm behind its frames. */
	inline constexpr double PaneEdges[Tiers][2] = {{-1.800, -0.555}, {-3.125, -1.850}, {-4.450, -3.175}, {-5.745, -4.500}};
	/** Four curved panes a tier, 90 degrees each, their joints on bearings 45 + 90k. */
	inline constexpr int32 PanesPerTier = 4;
	inline constexpr double FirstJointBearing = 45.0;
	inline constexpr double FrameInner = 2.410;
	inline constexpr double FrameOuter = GlassInner;
	/** The five gilt bands inside the glass, bottom and top: the top and bottom frames, and 60 mm on each tier level. */
	inline constexpr double FrameBands[Tiers + 1][2] = {{-0.560, -0.500}, {-1.855, -1.795}, {-3.180, -3.120}, {-4.505, -4.445}, {-5.800, -5.740}};
	inline constexpr double StripInner = 2.438;
	inline constexpr double StripOuter = GlassInner;
	inline constexpr double StripWidth = 0.025;
	/** Each strip runs one tier between its frames, bottom and top. */
	inline constexpr double StripRuns[Tiers][2] = {{-1.795, -0.560}, {-3.120, -1.855}, {-4.445, -3.180}, {-5.740, -4.505}};
	/** The frames are notched 0.190 wide round each rail and its bracket (5 mm clear of the bracket's 0.180). */
	inline constexpr double FrameNotch = 0.190;

	// ---- §3.4 plates and webs ---------------------------------------------------------------------------------------------
	inline constexpr double PlateInner = GlassInner;
	inline constexpr double PlateOuter = 2.770;
	inline constexpr double HeadPlate[2] = {-0.510, -0.500};
	inline constexpr double FootPlate[2] = {-5.800, -5.790};
	/** The tier webs, 10 mm thick, centred on the three inner tier levels, stop 60 mm short of the cut face. */
	inline constexpr double WebOuter = 2.600;
	inline constexpr double WebThickness = 0.010;

	// ---- §3.5 guide rails, tails and brackets (the Atrium's section) ------------------------------------------------------
	inline constexpr double RailBearings[] = {0.0, 180.0};
	inline constexpr double RailBladeInner = 2.265;
	inline constexpr double RailFootInner = 2.385;
	inline constexpr double RailFootOuter = 2.425;
	inline constexpr double RailBladeHalf = 0.008;
	inline constexpr double RailFootHalf = 0.070;
	/** The rails run 5 mm over the top blade; the tails hang 5 mm under the stack, so the car is on rack to the clamp. */
	inline constexpr double RailTop = -0.520;
	inline constexpr double RailBottom = -7.937;
	inline constexpr double TailTop = -7.973;
	inline constexpr double TailBottom = -8.345;
	/** The brackets fixing each rail's foot, 0.180 wide and 0.100 tall, on the glass's and the sleeve's faces. */
	inline constexpr double BracketInner = RailFootOuter;
	inline constexpr double BracketOuter = GlassInner;
	inline constexpr double BracketHalf = 0.090;
	inline constexpr double BracketHeight = 0.100;
	/** Each bracket's foot: at the head plate, the three webs (centred on them), the foot plate and the sleeve (-6.200, -7.100). */
	inline constexpr double BracketFeet[] = {-0.600, -1.875, -3.200, -4.525, -5.800, -6.250, -7.150};
	/** Each tail hangs from a gilt bracket bolted up into the soffit ring. */
	inline constexpr double TailBracketOuter = 2.560;
	inline constexpr double TailBracketBottom = -8.060;

	// ---- §3.6 sleeve, housing and ceiling ring ------------------------------------------------------------------------------
	inline constexpr double SleeveInner = GlassInner;
	inline constexpr double SleeveOuter = RoofHole;
	inline constexpr double VoidCourse[2] = {-7.590, RoofSoffit};
	inline constexpr double RoofCourse[2] = {RoofSoffit, RoofTop};
	/** The gilt joint line over the courses' joint, 8 mm proud of the sleeve's bore. */
	inline constexpr double SleeveLineInner = 2.442;
	inline constexpr double SleeveLine[2] = {-6.810, -6.790};
	/** The housing: its inner wall (the iris's seat), the roof the sleeve stands on, and its outer wall on the modules' backs. */
	inline constexpr double HousingInner = GlassInner;
	inline constexpr double SeatRadius = 2.460;
	inline constexpr double HousingInnerWall[2] = {-7.938, -7.600};
	inline constexpr double HousingRoof[2] = {-7.600, -7.590};
	inline constexpr double HousingOuter = 4.300;
	inline constexpr double HousingSkin = 0.010;
	inline constexpr double HousingOuterWall[2] = {-7.900, -7.600};
	/** Fourteen flats hang the soffit ring from the housing's roof, outside the blades' sweep. */
	inline constexpr int32 Hangers = 14;
	inline constexpr double HangerInner = 4.040;
	inline constexpr double HangerOuter = 4.100;
	inline constexpr double HangerHalf = 0.005;
	inline constexpr double Hanger[2] = {-7.970, -7.600};
	/** The soffit ring's section (r, z): a 30 mm gilt annulus with a 4 mm drip at its outer edge. */
	inline constexpr double SoffitRing[][2] = {{2.460, -7.970}, {2.460, -8.000}, {4.116, -8.000}, {4.116, -8.004}, {4.120, -8.004}, {4.120, -7.970}};
	/** The ring's two beads under its soffit: tori (major radius, tube radius, centre z), drawn below the soffit only. */
	inline constexpr double InnerBead[3] = {2.472, 0.009, -8.002};
	inline constexpr double OuterBead[3] = {4.100, 0.007, -8.001};

	// ---- §3.7 the ceiling iris, class iris.4600 -----------------------------------------------------------------------------
	inline constexpr int32 IrisBlades = 7;
	inline constexpr double PinRadius = 3.745;
	/** Each blade is a 110 degree arc of the middle circle, 1.250 wide with round ends; open, it is concentric with the axis. */
	inline constexpr double BladeMiddle = 3.095;
	inline constexpr double BladeWidth = 1.250;
	inline constexpr double BladeArcDeg = 110.0;
	/** 2 mm pearl steel at a 4 mm pitch, the top blade's face 7.442 under the plane. */
	inline constexpr double BladeThickness = 0.002;
	inline constexpr double BladePitch = 0.004;
	inline constexpr double TopBlade = -7.942;
	inline constexpr double StackBottom = -7.968;
	/** Shut, each blade stands turned 52 degrees on its pin towards increasing plan angle from its open place. */
	inline constexpr double SwingDeg = 52.0;
	/** Open, the blades' inner edge; in travel, the farthest they reach. */
	inline constexpr double BladeOpenInner = 2.470;
	inline constexpr double BladeSweep = 4.010;

	// ---- §3.8, §7 the ground's cut face ---------------------------------------------------------------------------------------
	/** The nominal face, the back face, and the most the rule's relief may stand proud towards the axis. */
	inline constexpr double GroundFace = 2.660;
	inline constexpr double GroundBack = 2.860;
	inline constexpr double GroundReliefMax = 0.050;
	/** The face spans the head plate's underside to the foot plate's top; beyond r 2.770 it reaches the slab and the roof. */
	inline constexpr double GroundTop = HeadPlate[0];
	inline constexpr double GroundBottom = FootPlate[1];
	/** The mesh step: 12.5 mm, so a stone rounds rather than steps (1,337 stations round). */
	inline constexpr double GroundGrid = 0.0125;
	/** Beyond the plates' r 2.770 the ground reaches up to the slab and down to the roof, to its back face. */
	inline constexpr double GroundCollarInner = PlateOuter;
	/** The ammonite's ground: a plane square to the shaft 0.400 across, holding the coil and its ribs, on a block 10 mm deep. */
	inline constexpr double AmmoniteField = 0.400;
	inline constexpr double AmmoniteBack = 0.010;

	// ---- §7 the stratigraphic rule: seeded periodic noise N(θ, z, f) round the cylinder at f cycles/m -----------------------
	/** One term a·N(f) of the rule: an amplitude in metres at a frequency in cycles per metre. */
	struct FNoiseTerm
	{
		double Amplitude;
		double CyclesPerMeter;
	};

	/** A contact between two layers: its mean height and the terms that wave it, and how wide the faces blend across it. */
	struct FContactRule
	{
		double Mean;
		FNoiseTerm Waves[2];
		double Blend;
	};

	/** Stones in a matrix: jittered cells, a stone in a share of them, each a dome of a half-size in a range, at most so proud. */
	struct FStoneRule
	{
		double Cell;
		double Share;
		double HalfSizeMin;
		double HalfSizeMax;
		double Proud;
	};

	/** Hardcore | topsoil: -0.720 + 0.025 N(4). */
	inline constexpr FContactRule Hardcore{-0.720, {{0.025, 4.0}, {0.0, 0.0}}, 0.030};
	/** Topsoil | subsoil: -1.020 + 0.06 N(2.5) + 0.02 N(9); a colour change only, the same crumbly face either side. */
	inline constexpr FContactRule TopsoilSubsoil{-1.020, {{0.060, 2.5}, {0.020, 9.0}}, 0.060};
	/** Subsoil | clay-with-flints: -1.500 + 0.07 N(2) + 0.03 N(7). */
	inline constexpr FContactRule SubsoilClay{-1.500, {{0.070, 2.0}, {0.030, 7.0}}, 0.050};
	/** Clay | chalk: -4.000 + 0.12 N(1.6) + 0.05 N(6), less the solution pipe. */
	inline constexpr FContactRule ClayChalk{-4.000, {{0.120, 1.6}, {0.050, 6.0}}, 0.040};
	/** The solution pipe at bearing 205: 0.75 exp(-(s/0.32)^2), s the arc from it on the nominal face. */
	inline constexpr double PipeBearing = 205.0;
	inline constexpr double PipeDepth = 0.750;
	inline constexpr double PipeHalfWidth = 0.320;

	/** Crushed stone: angular stones in a 45 mm jittered cell, 95 % filled, at most 14 mm proud, over a fine grain. */
	inline constexpr FStoneRule CrushedStones{0.045, 0.95, 0.012, 0.028, 0.014};
	inline constexpr FNoiseTerm CrushedGrain{0.004, 40.0};
	/** Topsoil and subsoil: crumbly, relief 7 mm (18 /m) + 4 mm (55 /m), stones in a 70 mm cell, 30 %, at most 9 mm proud. */
	inline constexpr FNoiseTerm SoilCrumb[2] = {{0.007, 18.0}, {0.004, 55.0}};
	inline constexpr FStoneRule SoilStones{0.070, 0.30, 0.008, 0.020, 0.009};
	/** Clay-with-flints: smooth (4 mm at 7 /m), spade-cut: vertical marks 1.8 mm deep at a 0.11 pitch, wandering 2 rad at 3 /m. */
	inline constexpr FNoiseTerm ClaySmooth{0.004, 7.0};
	inline constexpr double SpadeMark = 0.0018;
	inline constexpr double SpadePitch = 0.110;
	inline constexpr FNoiseTerm SpadeWander{2.0, 3.0};
	/** Chalk: blocky, relief 10 mm (5 /m, one-sided) + 5 mm (16 /m); bedding partings every 0.37, 5 mm deep, 12 mm wide. */
	inline constexpr FNoiseTerm ChalkBlocks{0.010, 5.0};
	inline constexpr FNoiseTerm ChalkGrain{0.005, 16.0};
	inline constexpr double PartingSpacing = 0.370;
	inline constexpr double PartingDepth = 0.005;
	inline constexpr double PartingWidth = 0.012;
	inline constexpr FNoiseTerm PartingWave{0.030, 3.0};

	/**
	 * The ammonite (Mantelliceras-like): 0.360 across at bearing 58, centre z -4.950, its mid-plane 4 mm proud of the face,
	 * coil facing the shaft, half exposed; 3.4 whorls on a logarithmic spiral expanding 2.1 times a turn; the whorl's section
	 * 0.46 by 0.30 of its radius; 26 ribs a turn, 7 % high, growing in over the first whorl.
	 */
	inline constexpr double AmmoniteBearing = 58.0;
	inline constexpr double AmmoniteZ = -4.950;
	inline constexpr double AmmoniteDiameter = 0.360;
	inline constexpr double AmmoniteProud = 0.004;
	inline constexpr double AmmoniteWhorls = 3.4;
	inline constexpr double AmmoniteExpansion = 2.1;
	inline constexpr double WhorlWidth = 0.46;
	inline constexpr double WhorlHeight = 0.30;
	inline constexpr double RibsPerTurn = 26.0;
	inline constexpr double RibHeight = 0.07;
	/** The carving's grid: 2 mm, some fifteen stations to a rib's spacing on the outer whorl. */
	inline constexpr double AmmoniteGrid = 0.002;
}
