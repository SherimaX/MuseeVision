#pragma once

#include "CoreMinimal.h"

/**
 * The Reserve's plan: every number the undercroft is built to, in metres in the stage's frame (origin on the axis midway
 * between the end walls' inner faces, on the finished floor; x east, y south, z up), from its design sheet,
 * Docs/Stages/reserve.md (the section is given with each group). The sheet's own sources are the as-built Reserve
 * (windows/, D-017) and the Story. Finishes, light, hanging and motion are the schedules' (Data/Stages/reserve), never
 * here. In the museum the stage stands at (-44.000, 0.000, -5.800), yaw 0 (§1).
 */
namespace Musee::Reserve::Plan
{
	/** The stage's key (§1). */
	inline constexpr const TCHAR* Stage = TEXT("reserve");

	// ---- §1 the box: the walls' faces, the footings and the two tops ---------------------------------------------------
	/** The walls' inner faces, x ±30.000 and y ±12.000, and their thickness. */
	inline constexpr double InnerX = 30.000;
	inline constexpr double InnerY = 12.000;
	inline constexpr double WallMeters = 0.600;
	inline constexpr double OuterX = InnerX + WallMeters;
	inline constexpr double OuterY = InnerY + WallMeters;
	/** The footings' underside, and the flat brick bed under the flags from there to the flags' underside. */
	inline constexpr double Footing = -0.350;
	/** The box's top: 5.470 under the lawn's 0.30 of soil, 5.750 under the Salon's floor, over x ±30.550, |y| ≤ 7.550. */
	inline constexpr double TopLow = 5.470;
	inline constexpr double TopHigh = 5.750;
	inline constexpr double SalonHalfX = 30.550;
	inline constexpr double SalonHalfY = 7.550;

	// ---- §2 the port: the west door, the class opening through the west wall ------------------------------------------
	inline constexpr const TCHAR* PortClass = TEXT("stairwell.straight.2400x3200.arched");
	inline constexpr const TCHAR* PortMates = TEXT("nympheas.stair");
	inline constexpr double DoorWidth = 2.400;
	inline constexpr double DoorSpringing = 2.000;
	/** The oval's well the port receives: museum x -90.80…-74.60, y ±1.60, z -6.15…0.00 (nympheas §2), here in local. */
	inline constexpr double WellWestX = -46.800;
	inline constexpr double WellHalfY = 1.600;
	/** The well's top, the oval's floor (museum 0.00), 5.80 above the Reserve's floor. */
	inline constexpr double WellTop = 5.800;
	/** The doorway's surround on the hall face: the museum's slim architrave P6 (nympheas §3.2). */
	inline constexpr const TCHAR* ArchitraveProfile = TEXT("architrave.two-fascia.160");

	// ---- §3 plan: the piers' lines, the bays and the rows -------------------------------------------------------------
	/** The piers stand on the arcade lines y ±5.000 at x -22.50 + 7.50 k (museum -66.5 + 7.5 k), seven a side. */
	inline constexpr double ArcadeY = 5.000;
	inline constexpr double PierPitch = 7.500;
	inline constexpr double FirstPierX = -22.500;
	inline constexpr int32 PierLines = 7;
	inline constexpr int32 Bays = 8;
	/** Half a pier's (and a band's) width: the clear spans stop 0.35 short of every pier line. */
	inline constexpr double BandHalf = 0.350;
	/** The nave between the arcade bands' faces, and each aisle from its arcade band's face to the side wall. */
	inline constexpr double NaveHalf = ArcadeY - BandHalf;
	inline constexpr double AisleInner = ArcadeY + BandHalf;
	/** The aisles' middle line, y ±8.675, and their half-width. */
	inline constexpr double AisleMiddle = 0.5 * (AisleInner + InnerY);
	inline constexpr double AisleHalf = 0.5 * (InnerY - AisleInner);

	// ---- §3.2 floor --------------------------------------------------------------------------------------------------
	/** Belgian bluestone flags 0.750 square and 0.060 thick in 3 mm lime joints on the grid x = 0.75 k, y = 0.75 k. */
	inline constexpr double FlagSize = 0.750;
	inline constexpr double FlagThickness = 0.060;
	inline constexpr double FlagJoint = 0.003;
	/**
	 * The nave's flags are finished as the nave, the rest as the aisles; they part on the grid's joint line at |y| 4.50,
	 * the one nearest the arcade bands' faces (|y| 4.65), so no flag is finished two ways (the sheet's Departures).
	 */
	inline constexpr double NaveFlagsHalf = 4.500;
	/** The flush floor tracks: a 30 mm cut lined by a bronze channel 20 mm clear and 45 deep, 5 mm lips flush with the flags. */
	inline constexpr double SlotCut = 0.030;
	inline constexpr double SlotClear = 0.020;
	inline constexpr double SlotDepth = 0.045;
	inline constexpr double SlotFrom = 1.200;
	inline constexpr double SlotTo = 11.450;

	// ---- §3.3 piers and responds -------------------------------------------------------------------------------------
	/** The square-splayed base (base.square-splayed), (half-width, z) from the bed up; its foot stands on the flags' bed. */
	inline constexpr double BaseSection[][2] = {{0.45, 0.00}, {0.45, 0.36}, {0.41, 0.44}, {0.41, 0.50}, {0.37, 0.54}};
	inline constexpr double BaseTop = 0.540;
	/** The chamfered brick shaft, 0.70 square with 40 mm chamfers, from the base to the impost. */
	inline constexpr double ShaftHalf = 0.350;
	inline constexpr double ShaftChamfer = 0.040;
	inline constexpr double ImpostBottom = 3.640;
	/** The impost (impost.square-splayed), (half-width, z); its top at 4.000 is every arch's springing. */
	inline constexpr double ImpostSection[][2] = {{0.37, 3.64}, {0.41, 3.69}, {0.45, 3.78}, {0.45, 4.00}};
	inline constexpr double Springing = 4.000;
	/** A respond, out from its wall's face (d): the pilaster, its two-stage base and its impost block (d, z). */
	inline constexpr double PilasterHalf = 0.350;
	inline constexpr double PilasterD = 0.120;
	inline constexpr double RespondBaseHalf = 0.450;
	inline constexpr double RespondBaseD = 0.220;
	inline constexpr double RespondBaseTop = 0.360;
	inline constexpr double RespondPlinthHalf = 0.410;
	inline constexpr double RespondPlinthD = 0.180;
	inline constexpr double RespondImpostSection[][2] = {{0.00, 3.64}, {0.14, 3.64}, {0.22, 3.76}, {0.22, 4.00}, {0.00, 4.00}};

	// ---- §3.4 vaults: 24 groins, 8 bays × 3 rows, all springing at 4.000 ------------------------------------------------
	/** The webs spring 0.06 above the bands, so every band stands 0.06 proud of the webs beside it. */
	inline constexpr double WebLift = 0.060;
	/** The rises: the nave's barrels 1.55 (crown 5.61), the aisles' 1.25 (crown 5.31), the arcade bands' 1.25. */
	inline constexpr double NaveRise = 1.550;
	inline constexpr double AisleRise = 1.250;
	inline constexpr double ArcadeRise = 1.250;
	/** Every arch is set out at the same 40 angular stations, so the groins are shared edges on the bays' diagonals. */
	inline constexpr int32 ArchStations = 40;

	// ---- §3.6 overhead tracks, one per rack ---------------------------------------------------------------------------
	/** The channel: 60 wide, underside 3.580, top 3.700, a 12 mm top plate, 8 mm sides, lips leaving a 26 mm slot. */
	inline constexpr double TrackHalf = 0.030;
	inline constexpr double TrackUnderside = 3.580;
	inline constexpr double TrackTop = 3.700;
	inline constexpr double TrackPlate = 0.012;
	inline constexpr double TrackSide = 0.008;
	/** The lips return the sides' 8 mm under the channel (as built, the section's one metal). */
	inline constexpr double TrackLip = 0.008;
	inline constexpr double TrackSlotHalf = 0.013;
	/** From the wall plate's face to the nave end, |y| 0.900, and the stop beyond it, 68 × 12, z 3.574 … 3.704. */
	inline constexpr double TrackEnd = 0.900;
	inline constexpr double StopHalf = 0.034;
	inline constexpr double StopThickness = 0.012;
	inline constexpr double StopBottom = 3.574;
	inline constexpr double StopTop = 3.704;
	/** The wall plate, 0.150 wide, z 3.510 … 3.760; of its 42 mm, 12 stand proud of the brick (30 are let in). */
	inline constexpr double WallPlateHalf = 0.075;
	inline constexpr double WallPlateBottom = 3.510;
	inline constexpr double WallPlateTop = 3.760;
	inline constexpr double WallPlateProud = 0.012;
	/** Its four bolts: hex heads across flats 22, 12 high, at x ±0.05, z 3.535 and 3.735. */
	inline constexpr double BoltX = 0.050;
	inline constexpr double BoltZs[] = {3.535, 3.735};
	inline constexpr double BoltAcrossFlats = 0.022;
	inline constexpr double BoltHead = 0.012;
	/** Three hanger rods a track at |y| 7.60, 5.00, 1.20: a clamp on the channel, a collar, the rod, a canopy on the brick. */
	inline constexpr double RodYs[] = {7.600, 5.000, 1.200};
	inline constexpr double ClampAcross = 0.072;
	inline constexpr double ClampAlong = 0.060;
	inline constexpr double ClampHeight = 0.042;
	inline constexpr double RodDiameter = 0.014;
	inline constexpr double CollarDiameter = 0.028;
	inline constexpr double CollarHeight = 0.028;
	inline constexpr double RodCanopyDiameter = 0.076;
	inline constexpr double RodCanopyHeight = 0.050;

	// ---- §3.7 the racks: 36 moving bodies, three a bay in both aisles, bays 1–6 ------------------------------------------
	/** The racks stand at the bays' pitch centres -26.25 + 7.50 (b - 1) and 1.90 either side, edge-on to the nave. */
	inline constexpr double FirstPitchCentre = -26.250;
	inline constexpr double RackSpacing = 1.900;
	inline constexpr int32 RackBays = 6;
	/** Home: centre |y| 9.150 (0.35 off the side wall); out: centre |y| 3.500, 5.650 along. */
	inline constexpr double RackHomeY = 9.150;
	inline constexpr double RackTravel = 5.650;
	/** End posts: drawn hollow sections 0.090 along × 0.160 across, capped, arrises r 8, centred ±2.455, z 0.070 … 3.400. */
	inline constexpr double PostAlong = 0.090;
	inline constexpr double PostAcross = 0.160;
	inline constexpr double PostArris = 0.008;
	inline constexpr double PostY = 2.455;
	inline constexpr double RackFoot = 0.070;
	inline constexpr double RackTop = 3.400;
	/** The rails between the posts' faces (y ±2.410): top 0.100 × 0.110 (z 3.290 …), kick 0.100 × 0.150 (… 0.220). */
	inline constexpr double RailHalf = 0.050;
	inline constexpr double RailEnd = 2.410;
	inline constexpr double TopRailBottom = 3.290;
	inline constexpr double KickRailTop = 0.220;
	/**
	 * The mesh: CuZn30 wire Ø 3.0 at 25 mm pitch, its edges clamped 12 mm into slots in the posts and rails. The weave is
	 * drawn as its two layers of straight wires touching on the centre plane (Form makes no crimp at this count; see the
	 * README), so the slots are 8 mm wide for the mesh's 6 mm.
	 */
	inline constexpr double WireDiameter = 0.003;
	inline constexpr double WirePitch = 0.025;
	inline constexpr double MeshClamp = 0.012;
	inline constexpr double MeshSlotHalf = 0.004;
	/** The wires end 1 mm short of their slots' floors, clamped, never bearing on them. */
	inline constexpr double WireEndClear = 0.001;
	/** The trolleys at y ±2.100: a saddle 0.072 × 0.140 × 0.014 on the top rail, a nut Ø 32 × 16, a stem Ø 16 to 3.610. */
	inline constexpr double TrolleyY = 2.100;
	inline constexpr double SaddleAcross = 0.072;
	inline constexpr double SaddleAlong = 0.140;
	inline constexpr double SaddleHeight = 0.014;
	inline constexpr double NutDiameter = 0.032;
	inline constexpr double NutHeight = 0.016;
	inline constexpr double StemDiameter = 0.016;
	inline constexpr double StemTop = 3.610;
	/** Guide shoes under the kick rail at y ±2.200, 0.044 × 0.140 from 0.050; guide fins 9 mm × 0.100 down to -0.030. */
	inline constexpr double ShoeY = 2.200;
	inline constexpr double ShoeAcross = 0.044;
	inline constexpr double ShoeAlong = 0.140;
	inline constexpr double ShoeBottom = 0.050;
	inline constexpr double FinThickness = 0.009;
	inline constexpr double FinAlong = 0.100;
	inline constexpr double FinBottom = -0.030;
	/** The pull on the nave end post: a bar Ø 22, z 0.950 … 1.600, 58 mm off the post on two stand-offs Ø 14 at 1.000 and 1.550. */
	inline constexpr double PullDiameter = 0.022;
	inline constexpr double PullBottom = 0.950;
	inline constexpr double PullTop = 1.600;
	inline constexpr double PullOff = 0.058;
	inline constexpr double StandOffDiameter = 0.014;
	inline constexpr double StandOffZs[] = {1.000, 1.550};
	/** Its Use anchor at the bar's middle: 1.275 above the floor. */
	inline constexpr double PullMiddle = 0.5 * (PullBottom + PullTop);
	/** The picture light's trough, 64 × 44 at ±0.420 across, centre z 3.320, on two arms Ø 14, 0.12 in from its ends. */
	inline constexpr double TroughAcross = 0.064;
	inline constexpr double TroughHeight = 0.044;
	inline constexpr double TroughOff = 0.420;
	inline constexpr double TroughCentreZ = 3.320;
	inline constexpr double TroughArmDiameter = 0.014;
	inline constexpr double TroughArmIn = 0.120;
	/** The arms' plates on the top rail, 0.050 along × 0.060 high, 6 mm thick (a bronze plate's usual gauge, as built). */
	inline constexpr double TroughPlateAlong = 0.050;
	inline constexpr double TroughPlateHeight = 0.060;
	inline constexpr double TroughPlateThickness = 0.006;
	/**
	 * The eight faces hung with works (§2, §8), each lit by the lamp in its trough (§4): the rack, the face (-1 west, +1
	 * east) and the lamp's length. A's east face waits with the send-to-easel carry (§12).
	 */
	struct FLitFace
	{
		const TCHAR* Rack;
		int32 Face;
		double LampMeters;
	};
	inline constexpr FLitFace LitFaces[] = {{TEXT("a"), -1, 1.19}, {TEXT("b"), -1, 1.56}, {TEXT("b"), 1, 0.84}, {TEXT("c"), -1, 3.80},
		{TEXT("d"), -1, 1.04}, {TEXT("d"), 1, 1.19}, {TEXT("e"), -1, 1.03}, {TEXT("e"), 1, 0.89}};
	/** The works' backs stand 0.012 off the rack's centre plane (the Wall anchors' plane). */
	inline constexpr double WorksBack = 0.012;

	// ---- §3.8 luminaire bodies -------------------------------------------------------------------------------------------
	/** Pendants at each bay's clear centre: nave globes Ø 0.44 at 3.95 on a chain; aisle globes Ø 0.34 at 4.42 on a rod. */
	inline constexpr double NaveGlobeDiameter = 0.440;
	inline constexpr double NaveGlobeCentre = 3.950;
	inline constexpr double AisleGlobeDiameter = 0.340;
	inline constexpr double AisleGlobeCentre = 4.420;
	/** The globe's cut, neck and rolled lip, in its radius R: cut at 0.34 R, neck 0.12 R tall. */
	inline constexpr double GlobeNeck = 0.34;
	inline constexpr double GlobeNeckHeight = 0.12;
	inline constexpr double GlobeLip[][2] = {{0.000, -0.006}, {0.004, -0.004}, {0.005, -0.002}, {-0.004, 0.000}};
	/** Cased opal glass, mouth-blown: a wall of 3 mm, as Lamberts' blown globes of this size are made. */
	inline constexpr double GlobeGlass = 0.003;
	/** The gallery, spun from 1.5 mm bronze, and the socket cup, (r in R or metres as marked, z above the cut). */
	inline constexpr double GalleryRadii[] = {0.52, 0.535, 0.52, 0.44, 0.41, 0.41, 0.38, 0.20, 0.13};
	inline constexpr double GalleryZs[] = {-0.013, -0.009, -0.005, 0.004, 0.008, 0.030, 0.035, 0.041, 0.043};
	inline constexpr double GallerySheet = 0.0015;
	inline constexpr double CupRadii[] = {0.13, 0.13, 0.15, 0.15, 0.12, 0.04, 0.00};
	inline constexpr double CupZs[] = {0.042, 0.091, 0.093, 0.097, 0.100, 0.104, 0.104};
	/** The canopy at the crown (r in k, z from the crown): k 1.00 in the nave, 0.85 in the aisles; its top 0.02 in the brick. */
	inline constexpr double CanopyRadii[] = {0.070, 0.070, 0.064, 0.045, 0.020};
	inline constexpr double CanopyZs[] = {0.020, -0.006, -0.020, -0.034, -0.041};
	inline constexpr double CanopyNeckRadius = 0.012;
	inline constexpr double CanopyNeckZs[] = {-0.043, -0.050};
	inline constexpr double NaveCanopyK = 1.00;
	inline constexpr double AisleCanopyK = 0.85;
	/** The nave chain: links of 6.4 mm wire, 28 × 12 inside, 28 mm pitch, each turned 90°, a loop Ø 9.5 inside at each end. */
	inline constexpr double ChainWire = 0.0064;
	inline constexpr double LinkInsideLength = 0.028;
	inline constexpr double LinkInsideWidth = 0.012;
	inline constexpr double LoopInside = 0.0095;
	/** The aisle rod Ø 15 from the locknut to the swivel ball Ø 26 at the crown - 0.062; a hex locknut 22 × 12 on the cup. */
	inline constexpr double PendantRodDiameter = 0.015;
	inline constexpr double SwivelDiameter = 0.026;
	inline constexpr double SwivelBelowCrown = 0.062;
	inline constexpr double LocknutAcrossFlats = 0.022;
	inline constexpr double LocknutHeight = 0.012;
	/**
	 * Three thumb screws through the gallery's band at 90° + 120° k to the neck: M4 shanks with knurled heads 12 across
	 * and 6 long, the lighting trade's usual gallery screw (the sheet gives their places, not their size).
	 */
	inline constexpr double ThumbShank = 0.004;
	inline constexpr double ThumbHeadDiameter = 0.012;
	inline constexpr double ThumbHeadLength = 0.006;
	inline constexpr double ThumbZ = 0.019;
	/** Uplights, four on each pier's impost ledges: lens at (±0.40, ±0.40, 4.095), aimed 58° up along the diagonal outwards. */
	inline constexpr double UplightOff = 0.400;
	inline constexpr double UplightLensZ = 4.095;
	inline constexpr double UplightElevationDeg = 58.0;
	/** The can Ø 44 × 75, its lens 6 mm behind a 4 mm bezel; its knuckle Ø 20 behind it on a stem Ø 14 to the ledge. */
	inline constexpr double CanDiameter = 0.044;
	inline constexpr double CanLength = 0.075;
	inline constexpr double LensSetBack = 0.006;
	inline constexpr double BezelWidth = 0.004;
	inline constexpr double KnuckleDiameter = 0.020;
	inline constexpr double UplightStemDiameter = 0.014;
	/** The easel spot: a canopy on the nave web of bay 8 at (24.40, 1.90), a stem Ø 12 down to the head at z 4.25. */
	inline constexpr double SpotX = 24.400;
	inline constexpr double SpotY = 1.900;
	inline constexpr double SpotZ = 4.250;
	inline constexpr double SpotAim[] = {28.900, 0.000, 1.515};
	inline constexpr double SpotStemDiameter = 0.012;
	/** Its head is the catalogue spot's small body (track-spot.3000k.12deg): Ø 0.102, 0.175 long, its face at the point. */
	inline constexpr double SpotHeadDiameter = 0.102;
	inline constexpr double SpotHeadLength = 0.175;

	// ---- §3.9 fittings: the easel and the plan chests -------------------------------------------------------------------
	/** The easel on the axis, its uprights' front faces at x 28.950, facing west (§3.9's table, x, y, z). */
	inline constexpr double EaselFront = 28.950;
	inline constexpr double EaselAnchorX = 28.945;
	inline constexpr double EaselAnchorY = -1.000;
	/**
	 * The easel's parts as boxes, (x from, x to, y from, y to, z from, z to), each at y and mirrored where the table gives
	 * ±. Where the sheet's table runs one part into another (the brackets and the ledge into the uprights' faces, the
	 * cross foot into the runners) the part stops at the face it is fixed to (the sheet's Departures).
	 */
	struct FEaselPart
	{
		const TCHAR* Name;
		const TCHAR* Surface;
		bool bMirrored;
		double Box[6];
	};
	inline constexpr FEaselPart EaselParts[] = {
		{TEXT("runner"), TEXT("easel"), true, {28.55, 29.45, 0.505, 0.595, 0.00, 0.09}},
		{TEXT("foot"), TEXT("easel"), false, {29.25, 29.33, -0.505, 0.505, 0.00, 0.09}},
		{TEXT("upright"), TEXT("easel"), true, {28.95, 29.03, 0.515, 0.585, 0.09, 2.35}},
		{TEXT("cap"), TEXT("easel.fitting"), true, {28.945, 29.035, 0.51, 0.59, 2.35, 2.38}},
		{TEXT("rail.lower"), TEXT("easel"), false, {28.955, 29.025, -0.515, 0.515, 0.35, 0.43}},
		{TEXT("rail.upper"), TEXT("easel"), false, {28.955, 29.025, -0.515, 0.515, 2.18, 2.26}},
		{TEXT("bracket"), TEXT("easel.fitting"), true, {28.84, 28.95, 0.50, 0.60, 0.70, 0.75}},
		{TEXT("ledge"), TEXT("easel"), false, {28.84, 28.95, -0.62, 0.62, 0.75, 0.80}},
		{TEXT("ledge.lip"), TEXT("easel"), false, {28.84, 28.87, -0.62, 0.62, 0.80, 0.835}},
	};
	/** The four chest islands: centres x 18.75 (bay 7) and 26.25 (bay 8), y ±8.675. */
	inline constexpr double ChestXs[] = {18.750, 26.250};
	inline constexpr double ChestY = 8.675;
	/** Plinth x ±1.05, y ±1.00, z 0 … 0.09; carcass x ±1.10, y ±1.05 (two chests back to back), to 0.865; top ±1.12 × ±1.07 to 0.900. */
	inline constexpr double ChestPlinthHalf[] = {1.050, 1.000};
	inline constexpr double ChestPlinthTop = 0.090;
	inline constexpr double ChestHalf[] = {1.100, 1.050};
	inline constexpr double ChestCarcassTop = 0.865;
	inline constexpr double ChestTopHalf[] = {1.120, 1.070};
	inline constexpr double ChestTop = 0.900;
	/** Ten drawers a side, two columns of five: fronts 1.064 × 0.1364, 12 mm gaps, from x -1.07 and z 0.115, 25 thick, 15 proud. */
	inline constexpr double DrawerWidth = 1.064;
	inline constexpr double DrawerHeight = 0.1364;
	inline constexpr double DrawerGap = 0.012;
	inline constexpr double DrawerFromX = -1.070;
	inline constexpr double DrawerFromZ = 0.115;
	inline constexpr double DrawerThickness = 0.025;
	inline constexpr double DrawerProud = 0.015;
	inline constexpr int32 DrawerRows = 5;
	/** The drawer pulls: bars 140 × 18, 22 proud, centred at 0.62 of the front's height. */
	inline constexpr double ChestPullLength = 0.140;
	inline constexpr double ChestPullHeight = 0.018;
	inline constexpr double ChestPullProud = 0.022;
	inline constexpr double ChestPullAt = 0.62;
	/** The felt on the two south islands, 5 mm: 0.99 × 0.76 in bay 7, 0.58 × 0.74 in bay 8. */
	inline constexpr double FeltThickness = 0.005;
	inline constexpr double FeltSizes[][2] = {{0.99, 0.76}, {0.58, 0.74}};
}
