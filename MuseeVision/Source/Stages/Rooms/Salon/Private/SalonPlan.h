#pragma once

#include "CoreMinimal.h"

/**
 * The Salon Impression's plan: every number the room is built to, in metres in the stage's frame (origin on the axis in
 * the plane of bay 1's east end wall's silk face, on the finished floor; x east, y south, z up), taken from its design
 * sheet, Docs/Stages/salon.md (the section is given with each group). The sheet's own sources are the as-built Salon
 * (windows/, D-017) and the approved salon-interior boards. Finishes, light, planting and the hang are the schedules'
 * (Data/Stages/salon), never here.
 */
namespace Musee::Salon::Plan
{
	/** The stage's key (§1). */
	inline constexpr const TCHAR* Stage = TEXT("salon");

	/** A point of a plan or a section, metres. */
	struct FPoint
	{
		double X;
		double Y;
	};

	// ---- §1 the footprint: clockwise on the north-up plan from the north-east corner, and its height --------------------
	inline constexpr FPoint Footprint[] = {{0.600, -7.600}, {0.600, 7.600}, {-60.600, 7.600}, {-60.600, -7.600}, {-10.400, -7.600},
		{-10.400, -16.000}, {-1.600, -16.000}, {-1.600, -7.600}};
	/** From the floor build-up's underside, bearing on the Reserve's top, to the vines' growth space over bay 3's pergola (§7). */
	inline constexpr double FootprintBottom = -0.050;
	inline constexpr double FootprintTop = 16.750;

	// ---- §2 ports: frames on the port faces at the threshold, and the classes' openings ------------------------------------
	inline constexpr const TCHAR* EastClass = TEXT("doorway.3000x6000.arched");
	inline constexpr const TCHAR* WestClass = TEXT("doorway.3000x4800.arched");
	inline constexpr const TCHAR* EastMates = TEXT("rotunda.west");
	inline constexpr const TCHAR* WestMates = TEXT("nympheas.east");
	inline constexpr double EastPortX = 0.600;
	inline constexpr double WestPortX = -60.600;
	/** Both port walls are 0.600 deep. */
	inline constexpr double PortWallDepth = 0.600;
	/** The clear opening of both classes and of the Salon's own doors: 3.000 wide under a semicircular head. */
	inline constexpr double DoorWidth = 3.000;
	/** The Salon's doors spring at 3.300 (crown 4.800); the Rotunda's class springs at 4.500 (crown 6.000). */
	inline constexpr double DoorSpringing = 3.300;
	inline constexpr double EastClassSpringing = 4.500;
	/** The class's lining allowance: the rough opening is 0.050 wider each side and at the head, its sill 0.050 down. */
	inline constexpr double LiningAllowance = 0.050;
	/** The east wall steps from the class's rough opening (x 0.600 to 0.300) to the Salon's own door (0.300 to 0.000). */
	inline constexpr double EastStepX = 0.300;

	// ---- §3 plan: the silk planes, masonry faces and outer faces; the bays by their silk faces; the piers -----------------
	inline constexpr double SilkHalf = 7.000;
	inline constexpr double MasonryHalf = 7.025;
	inline constexpr double OuterHalf = 7.600;
	inline constexpr double EndSilkX = 0.000;
	inline constexpr double EndMasonryX = 0.025;
	inline constexpr double EndOuterX = 0.600;
	inline constexpr double FarSilkX = -60.000;
	inline constexpr double FarMasonryX = -60.025;
	inline constexpr double FarOuterX = -60.600;
	/** The masonry rises from the floor build-up's underside to the roof slab's soffit (§3.1, §3.4). */
	inline constexpr double WallBed = -0.050;
	inline constexpr double WallTop = 13.850;

	struct FBay
	{
		double West;
		double East;
	};
	inline constexpr int32 BayCount = 5;
	inline constexpr FBay Bays[BayCount] = {{-11.400, 0.000}, {-23.400, -12.600}, {-35.400, -24.600}, {-47.400, -36.600}, {-60.000, -48.600}};

	inline constexpr int32 PierCount = 4;
	inline constexpr double PierX[PierCount] = {-12.000, -24.000, -36.000, -48.000};
	/** Half a pier's thickness, its jamb face, and its top (the arches' springing, §3.2). */
	inline constexpr double PierHalf = 0.600;
	inline constexpr double JambHalf = 4.500;
	inline constexpr double PierTop = 6.500;

	// ---- §3.1 walls: silk, the cabinet door, the cabinet ------------------------------------------------------------------
	/** Silk over 5 mm felt interlining on 18 mm battens: 25 mm from the masonry face to the silk plane. */
	inline constexpr double SilkBuildUp = 0.025;
	inline constexpr double SilkBottom = 0.150;
	inline constexpr double SilkTop = 6.250;
	inline constexpr double CabinetSilkTop = 5.100;
	/** The cabinet door's centre on the north wall. */
	inline constexpr double CabinetDoorX = -6.000;
	/** Skirtings stop 1.68 from each door's centre, inside the architrave's reach (1.500 + 0.220). */
	inline constexpr double SkirtingStop = 1.680;
	/**
	 * Round each door the silk stops 0.100 outside the opening, under the architrave (0.220 wide), and a stone lining
	 * fills its 25 mm build-up there, so the reveal runs out to the architrave's back.
	 */
	inline constexpr double LiningMargin = 0.100;

	/** The cabinet's silk faces, its walls' outer faces 0.400 beyond them, its ceiling and its roof slab (§3.1, §3.5). */
	inline constexpr double CabinetWest = -10.000;
	inline constexpr double CabinetEast = -2.000;
	inline constexpr double CabinetNorth = -15.600;
	inline constexpr double CabinetSouth = -7.600;
	inline constexpr double CabinetWall = 0.400;
	inline constexpr double CabinetCeiling = 5.500;
	inline constexpr double CabinetRoofBottom = 6.100;
	inline constexpr double CabinetRoofTop = 6.300;
	/** The laylight well round (-6.000, -11.600), its sides from the ceiling to the laylight at 5.900. */
	inline constexpr double WellWest = -8.500;
	inline constexpr double WellEast = -3.500;
	inline constexpr double WellNorth = -14.100;
	inline constexpr double WellSouth = -9.100;
	inline constexpr double LaylightZ = 5.900;

	// ---- §3.2 piers and transverse arches ---------------------------------------------------------------------------------
	/** Pier silk: 12 mm proud on the pier's side faces, stopped 0.14 short of the jamb, from the base to the impost. */
	inline constexpr double PierSilkProud = 0.012;
	inline constexpr double PierSilkJamb = 4.640;
	inline constexpr double PierSilkBottom = 0.300;
	inline constexpr double PierSilkTop = 6.060;
	/** The arches: centred on the axis at the springing, intrados R 4.500, extrados R 5.300, 1.200 thick. */
	inline constexpr double Intrados = 4.500;
	inline constexpr double Extrados = 5.300;
	inline constexpr int32 Voussoirs = 15;
	inline constexpr int32 Keystone = 7;
	/** The keystone's faces stand 20 mm proud of the arch's; its flat top is at z 12.300. */
	inline constexpr double KeystoneProud = 0.020;
	inline constexpr double KeystoneTop = 12.300;
	/** The archivolt (P7) is set out from R 5.296, 4 mm inside the extrados. */
	inline constexpr double ArchivoltRadius = 5.296;
	/** The spandrels run from the extrados to R 7.030, 30 mm behind the vault's face. */
	inline constexpr double SpandrelOuter = 7.030;

	// ---- §3.3 mouldings (A out of the host face, B up; metres) ----------------------------------------------------------
	/** One element of a drawn section: a line to a point, or an arc about a centre (either way round). */
	struct FStep
	{
		bool bArc = false;
		double A = 0.0;
		double B = 0.0;
		double CenterA = 0.0;
		double CenterB = 0.0;
		double RadiusA = 0.0;
		double RadiusB = 0.0;
		double From = 0.0;
		double To = 0.0;
	};
	constexpr FStep To(double A, double B) { return FStep{false, A, B}; }
	constexpr FStep Arc(double CenterA, double CenterB, double RadiusA, double RadiusB, double From, double To)
	{
		return FStep{true, 0.0, 0.0, CenterA, CenterB, RadiusA, RadiusB, From, To};
	}

	/**
	 * P1, the cornice (cornice.trough-lipped.470), as the sheet draws it from the silk-top slot round the lip to the
	 * trough's back at the springing; its back is closed down the masonry face (A -0.025), where the silk tucks behind it.
	 */
	inline constexpr FStep Cornice[] = {To(0.000, 5.625), To(0.040, 5.625), To(0.040, 5.600), To(0.075, 5.600), To(0.075, 5.800),
		To(0.095, 5.800), To(0.095, 5.825), Arc(0.095, 5.905, 0.080, 0.080, -90.0, 0.0), To(0.175, 5.925), To(0.195, 5.925),
		To(0.195, 5.945), To(0.470, 5.945), To(0.470, 6.150), To(0.490, 6.150), To(0.490, 6.180), Arc(0.490, 6.280, 0.040, 0.100, -90.0, 0.0),
		Arc(0.570, 6.280, 0.040, 0.100, 180.0, 90.0), To(0.585, 6.380), To(0.585, 6.470), To(0.510, 6.470), To(0.510, 6.250),
		To(0.000, 6.250), To(0.000, 6.500), To(-SilkBuildUp, 6.500), To(-SilkBuildUp, 5.625)};

	/** P2, the skirting (skirting.slotted.180), closed back to the masonry face under the silk. */
	inline constexpr FStep Skirting[] = {To(0.060, 0.000), To(0.060, 0.160), To(0.040, 0.180), To(0.030, 0.180), To(0.030, 0.150),
		To(-SilkBuildUp, 0.150), To(-SilkBuildUp, 0.000)};

	/** P3, the pier base. */
	inline constexpr FStep PierBase[] = {To(0.100, 0.000), To(0.100, 0.160), To(0.060, 0.160), Arc(0.060, 0.200, 0.040, 0.040, -90.0, 90.0),
		To(0.045, 0.240), To(0.030, 0.270), To(0.030, 0.300), To(0.000, 0.300), To(0.000, 0.000)};

	/** P4, the pier impost. */
	inline constexpr FStep PierImpost[] = {To(0.000, 6.060), To(0.020, 6.060), To(0.020, 6.085), Arc(0.020, 6.140, 0.055, 0.055, -90.0, 0.0),
		To(0.075, 6.160), To(0.085, 6.160), To(0.085, 6.400), To(0.100, 6.400), To(0.100, 6.500), To(0.000, 6.500)};

	/** P5, the door architrave (architrave.two-fascia.220): A out from the opening's edge, B out of the wall. */
	inline constexpr FStep Architrave[] = {To(0.220, 0.000), To(0.220, 0.090), To(0.205, 0.090), Arc(0.205, 0.070, 0.020, 0.020, 90.0, 180.0),
		To(0.160, 0.070), To(0.160, 0.064), To(0.100, 0.058), To(0.100, 0.052), To(0.020, 0.046), To(0.000, 0.030), To(0.000, 0.000)};
	/** The architrave's reach from the opening's edge. */
	inline constexpr double ArchitraveReach = 0.220;

	/** P7, the archivolt (archivolt.bead-fascia-cavetto.175): A radially out from R 5.296, B out of the arch's face. */
	inline constexpr FStep Archivolt[] = {To(0.175, 0.000), Arc(0.175, 0.012, 0.022, 0.012, -90.0, -180.0), To(0.030, 0.012), To(0.026, 0.004),
		Arc(0.013, 0.004, 0.013, 0.010, 0.0, 180.0), To(0.000, 0.000)};

	/** P8, the cabinet's cornice (cornice.slot-cove.330), closed back to the masonry face above the silk. */
	inline constexpr FStep CabinetCornice[] = {To(0.000, 5.125), To(0.040, 5.125), To(0.040, 5.100), To(0.070, 5.100), To(0.070, 5.180),
		To(0.090, 5.180), To(0.090, 5.200), Arc(0.270, 5.200, 0.180, 0.180, 180.0, 90.0), To(0.290, 5.380), To(0.290, 5.420), To(0.330, 5.420),
		To(0.330, 5.500), To(-SilkBuildUp, 5.500), To(-SilkBuildUp, 5.125)};

	// ---- §3.4 the barrel vault and its coffers -----------------------------------------------------------------------------
	/** The intrados: centre on the axis at the springing (6.500), R 7.000, crown 13.500. */
	inline constexpr double VaultRadius = 7.000;
	inline constexpr double Springing = 6.500;
	/** 19 rows of coffers round the semicircle, row 0 at the north springing; 8 columns a bay between plain bands. */
	inline constexpr int32 Rows = 19;
	inline constexpr int32 Columns = 8;
	inline constexpr double Band = 0.7704;
	/** The bands run on 4 mm behind the arches' and end walls' faces, sealing the joint. */
	inline constexpr double SealIn = 0.004;
	/**
	 * A coffer's section from the rib line: (inset along the intrados, depth into the vault). The rib's face is 0.100
	 * each side of its line; three steps fall to the panel 0.240 deep.
	 */
	inline constexpr FPoint CofferSection[] = {{0.000, 0.000}, {0.100, 0.000}, {0.115, 0.080}, {0.175, 0.080}, {0.190, 0.160},
		{0.240, 0.160}, {0.280, 0.240}};
	inline constexpr double CofferDepth = 0.240;
	/**
	 * A face that stops against another solid runs on into it by 2 mm, so the two meet without a hairline (the as-built
	 * rule, windows/Source/MuseeVision/Salon/SalonStructure.cpp:24).
	 */
	inline constexpr double Sink = 0.002;
	/**
	 * The vault's shell behind the coffers' panels: run lime plaster on expanded metal lath hung from the roof's
	 * structure, 50 mm (three coats of about 16 mm on the lath's key, as a plastered vault on metal lath is laid).
	 */
	inline constexpr double VaultShell = 0.050;

	/** The lanterns: row 9, columns 3 and 4 of each bay (§3.4). */
	inline constexpr int32 LanternRow = 9;
	inline constexpr int32 LanternColumn = 3;

	// ---- §3.4 rosettes: one per coffer on the panel's centre, cast and gilt (SalonStructure.cpp:889-937) ------------------
	inline constexpr double RosetteRadius = 0.112;
	struct FPetals
	{
		double Inner;
		double Outer;
		double Width;
		double HeightAtBase;
		double HeightAtTip;
		double Midrib;
		double OffsetDeg;
	};
	/** Twelve outer petals, and twelve inner ones set 15° between them; each petal is domed and sunk along its midrib. */
	inline constexpr int32 PetalsPerRing = 12;
	inline constexpr FPetals OuterPetals{0.022, 0.112, 0.044, 0.016, 0.004, 0.004, 0.0};
	inline constexpr FPetals InnerPetals{0.020, 0.078, 0.034, 0.026, 0.012, 0.005, 15.0};
	/**
	 * A petal's outline and dome, as built: its half-width follows sin(π·lerp(Base, 1, v))^Fullness·(1 - Taper·v) along
	 * its length v, so it is rounded at its base, widest a third of the way out and pointed at its tip; its midrib is
	 * sunk across a Gaussian of Spread times its half-width, fading in over the first 1/Reach of its length.
	 */
	inline constexpr double PetalBase = 0.12;
	inline constexpr double PetalFullness = 0.7;
	inline constexpr double PetalTaper = 0.25;
	inline constexpr double MidribSpread = 0.22;
	inline constexpr double MidribReach = 1.15;
	/** The boss, a low dome 30 mm high, in a bead ring of 4 mm tube at r 0.025, 12 mm up. */
	inline constexpr double BossRadius = 0.021;
	inline constexpr double BossHeight = 0.030;
	inline constexpr double BeadRadius = 0.025;
	inline constexpr double BeadTube = 0.004;
	inline constexpr double BeadHeight = 0.012;
	/** Cast in staff and gilt, finished at a 3 mm grid. */
	inline constexpr double RosetteGrid = 0.003;

	// ---- §3.4 lanterns, their shafts, grids and panes; the outer lanterns on the roof ------------------------------------
	/** The shafts' walls, rendered block 100 mm, from 4 mm inside the vault's face up to the roof's top. */
	inline constexpr double ShaftWall = 0.100;
	inline constexpr double GridBottom = 13.510;
	inline constexpr double InnerPaneZ = 13.580;
	inline constexpr double GridFrame = 0.040;
	inline constexpr double GridBar = 0.030;
	/** Five cross bars at the sixths. */
	inline constexpr int32 GridDivisions = 6;
	/**
	 * The inner panes are overhead glass under the weather glazing: laminated safety glass, 2 × 6 mm on a 0.76 mm PVB
	 * interlayer (DIN 18008-2:2020, 6.2: overhead glazing laminated, interlayer at least 0.76 mm).
	 */
	inline constexpr double InnerPane = 0.006 + 0.006 + 0.00076;
	/** Bay 4's linen scrim at 13.490 on a 25 × 25 angle frame 13.465 to 13.494 (a fixed diffuser, D-014). */
	inline constexpr int32 ScrimBay = 3;
	inline constexpr int32 OpalBay = 1;
	inline constexpr double ScrimZ = 13.490;
	inline constexpr double ScrimFrameLeg = 0.025;
	inline constexpr double ScrimFrameBottom = 13.465;
	inline constexpr double ScrimFrameTop = 13.494;
	/** A 25 × 25 angle is rolled 3 mm thick (EN 10056-1: the lightest equal angle of 25 mm legs). */
	inline constexpr double ScrimFrameThickness = 0.003;
	/**
	 * The linen hangs 30 mm at its middle (the sheet's depth; its shape is the stretched cloth's own, Form's membrane). It is
	 * a medium linen about 0.5 mm thick (textile thickness by ASTM D1777: medium-weight linen gauges 0.4 to 0.6 mm), set out
	 * on a grid of 0.05, which follows its sag (curvature under 0.05 per metre) within a hundredth of a millimetre.
	 */
	inline constexpr double ScrimSag = 0.030;
	inline constexpr double ScrimThickness = 0.0005;
	inline constexpr double ScrimGrid = 0.050;
	/** The outer lantern: a kerb 0.150 wide from the roof's top to 14.200, two glass slopes at 30° to a ridge along x. */
	inline constexpr double KerbWidth = 0.150;
	inline constexpr double KerbTop = 14.200;
	inline constexpr double OuterPitchDeg = 30.0;
	/** The weather glazing, overhead: 8 + 8 mm low-iron on a 1.52 mm PVB interlayer (DIN 18008-2:2020, 6.2), as the oval's dome. */
	inline constexpr double OuterPane = 0.008 + 0.008 + 0.00152;
	/** Bronze cap bars on the slopes at the sixths, 30 wide as the inner grid's bars, standing 20 mm over the glass. */
	inline constexpr double CapBarWidth = 0.030;
	inline constexpr double CapBarProud = 0.020;
	/** The ridge's cap flashing, 60 wide and 30 high over the ridge. */
	inline constexpr double RidgeCapWidth = 0.060;
	inline constexpr double RidgeCapHeight = 0.030;

	// ---- §3.4 the roof slab and bay 3's pergola --------------------------------------------------------------------------
	inline constexpr double RoofSoffit = 13.850;
	inline constexpr double RoofTop = 14.050;
	inline constexpr int32 PergolaBay = 2;
	inline constexpr double PostHalf = 0.035;
	inline constexpr double PostX[2] = {-33.000, -27.000};
	inline constexpr double PergolaY = 2.600;
	inline constexpr double BeamBottom = 16.390;
	inline constexpr double BeamTop = 16.470;
	inline constexpr double BeamHalfWidth = 0.025;
	inline constexpr double BeamWest = -33.150;
	inline constexpr double BeamEast = -26.850;
	inline constexpr int32 PergolaBars = 13;
	inline constexpr double BarPitch = 0.500;
	inline constexpr double BarHalfWidth = 0.014;
	inline constexpr double BarTop = 16.500;
	inline constexpr double BarHalfLength = 2.720;
	/** The planters: 0.70 square and 0.55 high, walls and bed 60 thick, so 0.49 deep inside. */
	inline constexpr double PlanterSide = 0.700;
	inline constexpr double PlanterHeight = 0.550;
	inline constexpr double PlanterWall = 0.060;
	inline constexpr double PlanterX[2] = {-32.600, -27.400};
	inline constexpr double PlanterY = 2.200;
	/** The soil stands 60 under the rim: the vines' beds (§2). */
	inline constexpr double SoilBelowRim = 0.060;

	// ---- §3.6 floors -----------------------------------------------------------------------------------------------------
	/** The build-up over the Reserve's top: oak (22 mm) or travertine (30 mm) on its bed, 50 mm in all, finished at z 0. */
	inline constexpr double FloorBuildUp = 0.050;
	/** The bronze strips: 12 wide, 12 deep, 0.5 mm proud. */
	inline constexpr double StripWidth = 0.012;
	inline constexpr double StripDepth = 0.012;
	inline constexpr double StripProud = 0.0005;
	inline constexpr double ViewingStoneRadius = 0.400;
	inline constexpr double ViewingRingRadius = 0.450;
	inline constexpr FPoint ViewingStones[] = {{-6.00, -4.00}, {-5.70, 3.60}, {-6.00, -11.60}, {-18.00, 0.00}, {-18.00, -3.60}, {-16.00, 3.80},
		{-21.20, 4.60}, {-30.00, 0.00}, {-30.00, -3.60}, {-30.00, 3.60}, {-42.00, 0.00}, {-42.00, -3.80}, {-42.00, 3.80}, {-54.30, 0.00},
		{-54.30, -3.80}, {-54.30, 3.80}};

	// ---- §3.7 fittings: the velvet benches -------------------------------------------------------------------------------
	/** The benches' centres, bays 2 to 5, north (n, y -2.600) then south. */
	inline constexpr double BenchX[4] = {-18.000, -30.000, -42.000, -54.300};
	inline constexpr double BenchY = 2.600;
	inline constexpr double BenchLength = 2.400;
	inline constexpr double BenchWidth = 0.600;
	inline constexpr double SeatTop = 0.440;
	inline constexpr double PlinthHeight = 0.075;
	inline constexpr double PlinthSetBack = 0.040;
	inline constexpr double PlinthArris = 0.004;
	/** The velvet body: top edge rolled r 0.040, bottom r 0.014, ends r 0.040 in plan; flat top at 0.428. */
	inline constexpr double TopRoll = 0.040;
	inline constexpr double BottomRoll = 0.014;
	inline constexpr double FlatTop = 0.428;
	/** The velvet's edge is swept round a loop 30 mm inside the outline, whose corners keep 10 mm of the ends' 40 mm radius. */
	inline constexpr double EdgeReach = 0.030;
	/** Quilted in 4 panels along its length, each domed 16 mm; 4 mm piping round top and bottom and across each seam. */
	inline constexpr int32 QuiltPanels = 4;
	inline constexpr double QuiltDome = 0.016;
	inline constexpr double Piping = 0.004;
	/** The quilting's grid: a centimetre, finer than a 16 mm dome's curvature needs. */
	inline constexpr double QuiltGrid = 0.010;
	/** Two seat frames on each face, 0.60 either side of the bench's middle. */
	inline constexpr double SeatAlong = 0.600;

	// ---- §3.7 the glass stele -------------------------------------------------------------------------------------------
	inline constexpr double SteleX = -6.150;
	inline constexpr double SteleThickness = 0.050;
	inline constexpr double SteleNorth = 0.800;
	inline constexpr double SteleSouth = 2.000;
	inline constexpr double SteleBottom = 0.080;
	inline constexpr double SteleTop = 2.480;
	inline constexpr double FootWest = -6.350;
	inline constexpr double FootEast = -5.950;
	inline constexpr double FootNorth = 0.700;
	inline constexpr double FootSouth = 2.100;
	inline constexpr double FootEase = 0.006;

	// ---- §3.7 the 1874 case, in the Manet cabinet -------------------------------------------------------------------------
	inline constexpr double CaseX = -8.600;
	inline constexpr double CaseY = -8.450;
	inline constexpr double CaseHalfLength = 0.800;
	inline constexpr double CaseHalfWidth = 0.350;
	inline constexpr double CasePlinthRecess = 0.040;
	inline constexpr double CasePlinthTop = 0.090;
	inline constexpr double CaseBodyTop = 0.800;
	/** The long faces' frame: rails and stiles 5 mm proud of the panels. */
	inline constexpr double FrameProud = 0.005;
	inline constexpr double BottomRail[2] = {0.110, 0.170};
	inline constexpr double TopRail[2] = {0.730, 0.790};
	inline constexpr double StileWidth = 0.060;
	inline constexpr double StileAt[3] = {-0.800, -0.030, 0.740};
	/** The top rail round the case, 12 mm proud. */
	inline constexpr double CapRail[2] = {0.800, 0.860};
	inline constexpr double CapRailProud = 0.012;
	/** The felt floor's top and the hood's top. */
	inline constexpr double FeltTop = 0.864;
	inline constexpr double HoodTop = 0.990;
	/** The hood's panes: anti-reflective laminated low-iron, 2 × 4 mm on 0.76 mm PVB, as a display case's hood is glazed. */
	inline constexpr double HoodPane = 0.004 + 0.004 + 0.00076;
	/** Bronze angles 12 × 12 on the hood's twelve edges, rolled 2 mm (the lightest angle of 12 mm legs, EN 10056-1). */
	inline constexpr double AngleLeg = 0.012;
	inline constexpr double AngleThickness = 0.002;
	/** Le Charivari's felt board, 0.36 × 0.513 × 0.010, at case-x +0.28. */
	inline constexpr double BoardAt = 0.280;
	inline constexpr double BoardLength = 0.360;
	inline constexpr double BoardWidth = 0.513;
	inline constexpr double BoardThickness = 0.010;
	/** The lock's escutcheon on the reader's rail: a cabinetmaker's 30 × 40 mm plate, 2 mm thick. */
	inline constexpr double EscutcheonWidth = 0.030;
	inline constexpr double EscutcheonHeight = 0.040;
	inline constexpr double EscutcheonThickness = 0.002;

	// ---- §3.7 the Little Dancer's plinth and vitrine ---------------------------------------------------------------------
	inline constexpr double DancerX = -20.500;
	inline constexpr double DancerY = 3.400;
	inline constexpr double DancerPlinthHalf = 0.400;
	inline constexpr double DancerPlinthTop = 1.000;
	inline constexpr double VitrineHalf = 0.380;
	inline constexpr double VitrineTop = 2.100;
	/** The vitrine's panes: toughened low-iron, 10 mm, a free-standing vitrine's usual glass. */
	inline constexpr double VitrinePane = 0.010;
}
