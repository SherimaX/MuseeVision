#pragma once

#include "CoreMinimal.h"

/**
 * The Classical Hall's plan: every number the hall is built to, in metres in the stage's frame (origin at the tribune's
 * centre on its finished floor; x east, y south, z up), taken from its design sheet, Docs/Stages/classical-hall.md (the
 * section is given with each group). The sheet's own sources are the as-built hall (windows/, D-017; old y + 39.30,
 * old z + 6.00) and the rulings on its section. Finishes, light and the hang are the schedules' (Data/Stages/classical-hall),
 * never here: the carriers below are the room's own furniture, sized by the sheet's §8.
 */
namespace Musee::ClassicalHall::Plan
{
	/** The stage's key (§1). */
	inline constexpr const TCHAR* Stage = TEXT("classical-hall");

	// ---- §1 the shell's faces and the stage's height -------------------------------------------------------------------
	inline constexpr double SlabUnderside = -0.400;
	inline constexpr double RoofTop = 5.450;
	/** The roof slab's soffit, where the vault crowns meet it and every laylight's plenum ends (§3.3, ruled). */
	inline constexpr double RoofSoffit = 5.150;
	inline constexpr double ShellHalfWidth = 4.200;
	inline constexpr double ShellSouthY = 31.400;
	inline constexpr double ShellRadius = 7.900;
	/** The footprint's arcs are built at 1.5° stations, 240 round (§1). */
	inline constexpr double ArcStationDeg = 1.5;

	// ---- §2 the port and the received well ---------------------------------------------------------------------------
	/** The well's axis W = (0, 39.30), its cylinder r 7.60, from the slab's underside to the Rotunda's floor (z 6.00). */
	inline constexpr double WellAxisY = 39.300;
	inline constexpr double WellRadius = 7.600;
	inline constexpr double WellTop = 6.000;
	/** The port face crosses the axis at y 31.70; the wall is 0.600 deep there. */
	inline constexpr double PortFaceY = 31.700;
	inline constexpr const TCHAR* PortClass = TEXT("stairwell.spiral.3600x4400.arched");
	inline constexpr const TCHAR* PortMates = TEXT("rotunda.stair");
	/** The class opening: clear 3.600, head r 1.800 springing 2.600; grown by Site's lining allowance on every side. */
	inline constexpr double PortClearHalf = 1.800;
	inline constexpr double PortSpringing = 2.600;
	inline constexpr double LiningAllowance = 0.050;
	/** The sleeve round the passage (the collar envelope): x ±2.10, a round head r 2.10 springing with the opening. */
	inline constexpr double SleeveHalf = 2.100;
	/**
	 * Form cuts no passage through a curved wall, so the sleeve's head over the rough opening is laid in horizontal
	 * courses cut in plan to the port cylinder, each course's bed the width of the arch at its foot; 150 mm courses
	 * keep the step behind Site's lining under the lining's own allowance of 50 mm over most of the arch.
	 */
	inline constexpr double SleeveCourse = 0.150;

	// ---- §1, §3.1 the gallery, the throat and the tribune's wall ------------------------------------------------------
	inline constexpr double GalleryHalf = 3.000;
	inline constexpr double SouthFaceY = 31.100;
	/** The screen line and the lintel's south face. */
	inline constexpr double ScreenY = 7.100;
	inline constexpr int32 Bays = 4;
	inline constexpr double BayCentres[Bays] = {28.100, 22.100, 16.100, 10.100};
	inline constexpr double BayLength = 6.000;
	inline constexpr double Ledge = 3.700;
	inline constexpr double LintelSoffit = 3.075;
	inline constexpr double TribuneRadius = 6.600;
	/** Every wall splits at the dado's top, 0.45 (§3.1). */
	inline constexpr double DadoTop = 0.450;
	/** Crema Marfil is a laid finish, 30 mm over the travertine core (§3 build-ups); so is the dado. */
	inline constexpr double LaidFinish = 0.030;
	/** The opus sectile, 30 mm on the slab, finished level at z 0 (§3.2). */
	inline constexpr double FloorStone = 0.030;
	/** The profiles' sink into the face they meet (§3: A −0.010). */
	inline constexpr double Sink = 0.010;

	// ---- §3.1 the niches ------------------------------------------------------------------------------------------------
	/** Gallery niches: half-round R 0.75 on the wall plane, sill 0.45, springing 2.19, crown 2.94. */
	inline constexpr double GalleryNicheRadius = 0.750;
	inline constexpr double GalleryNicheSpringing = 2.190;
	/** Tribune niches: half-round R 0.90 on the mouth plane tangent to r 6.60, springing 2.05, crown 2.95. */
	inline constexpr double TribuneNicheRadius = 0.900;
	inline constexpr double TribuneNicheSpringing = 2.050;
	inline constexpr double TribuneNicheDeg[] = {270.0, 330.0, 210.0, 30.0, 150.0};
	inline constexpr const TCHAR* TribuneNicheNames[] = {TEXT("n"), TEXT("ne"), TEXT("nw"), TEXT("se"), TEXT("sw")};
	/** Each niche's floor slab, its top at the sill, 0.05 thick. */
	inline constexpr double NicheSillSlab = 0.050;
	/**
	 * The niche's lining: lime plaster run on the core, burnished with red ochre, 20 mm in three coats, the thickness
	 * of traditional lime plasterwork on masonry (BS 8481:2006, 6.4: two or three coats, 15 to 25 mm overall).
	 */
	inline constexpr double NicheLining = 0.020;
	/** The niche's surround band (§3.4); the niche's flat bay reaches to its outer edge. */
	inline constexpr double SurroundBand = 0.100;
	/** A niche's bay, the flat block of wall it is cut in, from the mouth plane back to 1.20 (the gallery wall's depth). */
	inline constexpr double NicheBayDepth = 1.200;

	// ---- §3.3 the gallery barrel ----------------------------------------------------------------------------------------
	inline constexpr double VaultRadius = 3.8284;
	inline constexpr double VaultAxisZ = 1.3216;
	/** s is the arc from the crown along the face, + east; the springing is at s ±3.4473, the coffer field within ±3.00. */
	inline constexpr double VaultSpringingS = 3.4473;
	inline constexpr double CofferFieldS = 3.000;
	inline constexpr double CofferCell = 0.600;
	/** Along y: plain 31.10 → 30.50 and 7.70 → 7.10; each bay's coffers its centre ± 2.40; pair bands between. */
	inline constexpr double PlainSouthY = 30.500;
	inline constexpr double PlainNorthY = 7.700;
	inline constexpr double BayCofferHalf = 2.400;
	/** A coffer: a flat rib 0.06 inside the cell's edge, three risers of 0.06 with setbacks of 0.035 between: 0.18 deep. */
	inline constexpr double CofferRib = 0.060;
	inline constexpr double CofferRiser = 0.060;
	inline constexpr double CofferSetback = 0.035;
	inline constexpr int32 CofferSteps = 3;
	/**
	 * The vault's run plaster, over the shell: 20 mm of lime plaster run in place, as the niches' (BS 8481:2006, 6.4).
	 */
	inline constexpr double VaultPlaster = 0.020;
	/** The carver's grid for the coffered field and its rosettes: 5 mm, twenty samples across the smallest rosette's petal. */
	inline constexpr double CofferGrid = 0.005;

	/** The lanterns: opening s and y within ±1.20 of the bay's centre; a flat frame 0.06 inside its edge. */
	inline constexpr double LanternHalf = 1.200;
	inline constexpr double LanternFrame = 0.060;
	/** The laylight glass, 2.2464 × 2.280, underside 4.940; laminated 2 × 4 mm with its interlayer, top 4.949. */
	inline constexpr double LaylightUnderside = 4.940;
	inline constexpr double LaylightGlass = 0.009;
	/**
	 * The cheeks that close the glass up to the frame's inner edge: trowelled lime plaster on a 20 mm board, the
	 * run plaster's thickness.
	 */
	inline constexpr double LaylightCheek = 0.020;

	// ---- §3.3 the rosettes -----------------------------------------------------------------------------------------------
	/** A rosette's radius is 0.3 of its coffer back's smaller side; its heights are drawn at R 0.08 and scale with it. */
	inline constexpr double RosetteOfBack = 0.300;
	inline constexpr double RosetteDrawnRadius = 0.080;
	/** The turned section: (radius fraction, height), from the flat underside's rim over the petals to the boss. */
	inline constexpr double RosetteSection[][2] = {{1.00, 0.004}, {0.86, 0.014}, {0.62, 0.020}, {0.36, 0.023}, {0.20, 0.032}, {0.00, 0.036}};
	inline constexpr double RosetteUnderside = -0.004;
	/** Eight petals: on the section's rows 1.00 to 0.36 the radius swells by 0.84 + 0.16 |cos 4α|. */
	inline constexpr int32 RosettePetalRows = 4;
	inline constexpr double RosettePetalBase = 0.840;
	inline constexpr double RosettePetalSwell = 0.160;
	inline constexpr double RosettePetals = 8.0;
	/** A rosette cast separately sits in a shallow bed 2 mm behind the curved back it is fixed to; its grid is 4 mm. */
	inline constexpr double RosetteBed = 0.002;
	inline constexpr double RosetteGrid = 0.004;

	// ---- §3.3 the annular vault -------------------------------------------------------------------------------------------
	/** Its section: a circle R 1.5009 centred (r 5.10, z 3.6491), springing from r 6.60 and r 3.60 at 3.70, crown 5.15. */
	inline constexpr double AnnularRadius = 1.5009;
	inline constexpr double AnnularCentreR = 5.100;
	inline constexpr double AnnularCentreZ = 3.6491;
	inline constexpr double AnnularOuterSpringingR = 6.600;
	inline constexpr double AnnularInnerSpringingR = 3.600;
	/** Coffers between the section angles 13.395° and 166.605°: 7 across, 60 round in 6° cells from φ 15°. */
	inline constexpr double AnnularCofferFromDeg = 13.395;
	inline constexpr double AnnularCofferToDeg = 166.605;
	inline constexpr int32 AnnularAcross = 7;
	inline constexpr int32 AnnularRound = 60;
	inline constexpr double AnnularFirstRibDeg = 15.0;
	/** Its coffers are the gallery's with a rib of 0.05 and every step × 0.85. */
	inline constexpr double TribuneCofferRib = 0.050;
	inline constexpr double AnnularStepScale = 0.850;

	// ---- §3.3 the dome and the eye ----------------------------------------------------------------------------------------
	/** A spherical cap R 4.3377 about (0, 0, 0.7716) from the ring beam's inner ledge (r 3.20, 3.70) to the eye (r 1.20, 4.94). */
	inline constexpr double DomeRadius = 4.3377;
	inline constexpr double DomeCentreZ = 0.7716;
	inline constexpr double DomeSpringingR = 3.200;
	inline constexpr double EyeRadius = 1.200;
	inline constexpr double EyeZ = 4.940;
	/** The coffer rings' edges on the cap, (r, z), from the springing's plain band to the eye's. */
	inline constexpr double DomeRingEdges[][2] = {{3.047, 3.859}, {2.463, 4.342}, {1.893, 4.674}, {1.389, 4.881}};
	inline constexpr int32 DomeRings = 3;
	inline constexpr double DomeStepScales[DomeRings] = {1.0, 0.9, 0.8};
	/** 24 round in 15° cells from φ 7.5°. */
	inline constexpr int32 DomeRound = 24;
	inline constexpr double DomeFirstRibDeg = 7.5;
	/** The eye's bead, turned round the rim, (r, z). */
	inline constexpr double BeadTop[2] = {1.190, 4.960};
	inline constexpr double BeadFoot[2] = {1.190, 4.880};
	inline constexpr double BeadOuter = 1.220;

	// ---- §3.3, §3.4 the ring beam and the tribune's order -------------------------------------------------------------------
	inline constexpr double RingRadius = 3.400;
	inline constexpr int32 RingColumns = 12;
	inline constexpr double RingFirstColumnDeg = 15.0;
	/** The ring beam carries the entablature on both faces: architrave faces r 3.2583 and 3.5417. */
	inline constexpr double RingInnerFace = 3.2583;
	inline constexpr double RingOuterFace = 3.5417;

	// ---- §3.4 the Ionic order after Vignola, D 0.34 -----------------------------------------------------------------------
	/** The column's square plinth, 0.47, from 0.01 under the floor to 0.055. */
	inline constexpr double PlinthSide = 0.470;
	inline constexpr double PlinthTop = 0.055;
	/** The turned base, the shaft (r 0.170, straight to 1.0983, then entasis to r 0.14167 at 2.865) and the echinus. */
	inline constexpr double ShaftRadius = 0.170;
	inline constexpr double ShaftTopRadius = 0.14167;
	inline constexpr double ShaftFoot = 0.215;
	inline constexpr double EntasisFrom = 1.0983;
	inline constexpr double ShaftTop = 2.865;
	/** The entasis is drawn at 12 stations, each under a millimetre from the curve r(t) = 0.170 − 0.02833 (1 − cos πt/2). */
	inline constexpr int32 EntasisStations = 12;
	inline constexpr double CapitalTop = 2.965;
	/** The canalis block 0.31 across the volutes × 0.34, z 2.945–3.04; the plate 0.47 × 0.32, z 3.005–3.05. */
	inline constexpr double CanalisAcross = 0.310;
	inline constexpr double CanalisDeep = 0.340;
	inline constexpr double CanalisFoot = 2.945;
	inline constexpr double CanalisTop = 3.040;
	inline constexpr double PlateAcross = 0.470;
	inline constexpr double PlateDeep = 0.320;
	inline constexpr double PlateFoot = 3.005;
	inline constexpr double PlateTop = 3.050;
	/** The bolsters about the front-to-back axis at ±0.155, z 2.945, r 0.075, ±0.19 long, waisted to 0.056, belted 0.064. */
	inline constexpr double BolsterOffset = 0.155;
	inline constexpr double BolsterRadius = 0.075;
	inline constexpr double BolsterHalf = 0.190;
	inline constexpr double BolsterWaist = 0.056;
	inline constexpr double BolsterBelt = 0.064;
	inline constexpr double BolsterBeltHalf = 0.030;
	/**
	 * The volute: a logarithmic spiral fillet 12 × 16 mm (12 wide; 16 deep, 4 of them set into the face and 12 proud, as
	 * built), 2.5 turns from R 0.069 to 0.0193, round an eye R 0.018.
	 */
	inline constexpr double VoluteOuter = 0.069;
	inline constexpr double VoluteInner = 0.0193;
	inline constexpr double VoluteTurns = 2.5;
	inline constexpr double FilletWidth = 0.012;
	inline constexpr double FilletProud = 0.012;
	inline constexpr double FilletSet = 0.004;
	inline constexpr double VoluteEye = 0.018;
	/** The eye's boss, 14 mm proud (as built). */
	inline constexpr double VoluteEyeProud = 0.014;
	/** The spiral is drawn at 24 stations a turn, each chord within a millimetre of the curve at its outer turn. */
	inline constexpr int32 VoluteStationsPerTurn = 24;
	/** The abacus 0.37 square, z 3.035–3.07; its edge (0, 0) (0, 0.012) round to (0.015, 0.027) convex (0.015, 0.035). */
	inline constexpr double AbacusSide = 0.370;
	inline constexpr double AbacusFoot = 3.035;
	inline constexpr double AbacusTop = 3.070;
	inline constexpr double AbacusEdge = 0.015;

	/** Gallery pairs x ±2.60 at their centres ± 0.50; the screen x ±1.10 at y 7.0283, facing south. */
	inline constexpr double PairX = 2.600;
	inline constexpr double PairCentres[] = {25.100, 19.100, 13.100};
	inline constexpr double PairHalf = 0.500;
	inline constexpr double ScreenColumnX = 1.100;
	inline constexpr double ScreenColumnY = 7.0283;

	// ---- §3.4 the pilasters ------------------------------------------------------------------------------------------------
	inline constexpr double PilasterProud = 0.070;
	inline constexpr double PilasterHalf = 0.170;
	inline constexpr double PilasterFoot = 0.200;
	inline constexpr double PilasterTop = 2.900;
	inline constexpr double PilasterNeckFoot = 2.865;
	inline constexpr double PilasterNeckTop = 2.945;
	/**
	 * The pilaster's capital (§3.4): its canalis block 0.02 in front of the shaft, the flat volute discs 0.01 thick on it,
	 * and the abacus 0.40 wide standing 0.01 proud of the canalis.
	 */
	inline constexpr double PilasterCanalisProud = 0.090;
	inline constexpr double PilasterDiscProud = 0.100;
	inline constexpr double PilasterAbacusHalf = 0.200;
	inline constexpr double PilasterAbacusProud = 0.100;
	inline constexpr double GalleryPilasterY[] = {30.850, 7.270};
	inline constexpr double TribunePilasterDeg[] = {0.0, 61.38, 118.62, 180.0, 240.0, 300.0};

	// ---- §3.4 the entablature ---------------------------------------------------------------------------------------------
	/** 0.72 over the architrave's soffit at 3.06: the ledge at 3.70, the lip's top 3.78. */
	inline constexpr double ArchitraveSoffit = 3.060;
	/** Against a wall the architrave's face stands 0.07 proud and its back sinks 0.01: BackA −0.08. */
	inline constexpr double WallBackA = -0.080;
	/** Over each gallery column pair a ressaut, its face 0.2583 off the wall for the pair's y ± 0.72. */
	inline constexpr double RessautFace = 0.2583;
	inline constexpr double RessautHalf = 0.720;
	/** Behind each ressaut a solid filler from the wall, z 3.07 to 3.685, the pair's y ± 0.65. */
	inline constexpr double RessautFillerHalf = 0.650;
	inline constexpr double RessautFillerFoot = 3.070;
	inline constexpr double RessautFillerTop = 3.685;
	/** Across the screen its face stands at y 7.17. */
	inline constexpr double ScreenEntablatureY = 7.170;
	/** The tribune's run: face r 6.53. */
	inline constexpr double TribuneEntablatureR = 6.530;
	/** Dentils 0.034 wide × 0.040 deep × 0.048 high (z 3.516–3.564), pitch 0.052, on the band 0.035–0.075 out; 0.06 clear of a run's ends. */
	inline constexpr double DentilWidth = 0.034;
	inline constexpr double DentilPitch = 0.052;
	inline constexpr double DentilFoot = 3.516;
	inline constexpr double DentilTop = 3.564;
	inline constexpr double DentilBand = 0.035;
	inline constexpr double DentilProud = 0.040;
	inline constexpr double DentilMargin = 0.060;
	/** The dentils' grid, 4 mm: eight samples across a dentil, four across a gap. */
	inline constexpr double DentilGrid = 0.004;

	// ---- §3.4 the wall mouldings, panels and consoles ---------------------------------------------------------------------
	/** The keystone at each niche's crown, from 0.04 below it into the architrave's soffit, widening 0.10 → 0.15, 0.055 proud. */
	inline constexpr double KeystoneUnder = 0.040;
	inline constexpr double KeystoneFootWidth = 0.100;
	inline constexpr double KeystoneTopWidth = 0.150;
	inline constexpr double KeystoneProud = 0.055;
	inline constexpr double KeystoneTop = 3.070;
	/** Framed panels 0.80 × 1.90 (z 0.85–2.75), 0.02 proud: the gallery's at bay centre ± 1.60, the tribune's at each niche ± 15°. */
	inline constexpr double PanelWidth = 0.800;
	inline constexpr double PanelFoot = 0.850;
	inline constexpr double PanelTop = 2.750;
	inline constexpr double PanelProud = 0.020;
	inline constexpr double GalleryPanelOffset = 1.600;
	inline constexpr double TribunePanelOffsetDeg = 15.0;
	/** The lunette panel 2.60 × 0.80 (z 3.98–4.78) on the lintel's face, centred on the axis. */
	inline constexpr double LunetteWidth = 2.600;
	inline constexpr double LunetteFoot = 3.980;
	inline constexpr double LunetteTop = 4.780;
	/** The console: a quadrant bracket 0.36 wide × 0.30 proud × 0.41 high, top 1.55; the quarter ellipse 0.21 × 0.36 about (0.06, 1.50). */
	inline constexpr double ConsoleFoot = 1.140;
	inline constexpr double ConsoleTop = 1.550;
	inline constexpr double ConsoleCurveA = 0.060;
	inline constexpr double ConsoleCurveB = 1.500;
	inline constexpr double ConsoleCurveHigh = 0.360;

	// ---- §3.5 the carriers ------------------------------------------------------------------------------------------------
	/** The niche carriers' moulding: base 0.015 proud to 0.045, cap 0.02 proud from H − 0.03. */
	inline constexpr double NicheCarrierBase = 0.015;
	inline constexpr double NicheCarrierBaseTop = 0.045;
	inline constexpr double NicheCarrierCap = 0.020;
	inline constexpr double NicheCarrierCapHigh = 0.030;
	/** The axis and ambulatory plinths' moulding: base 0.03 proud to 0.10, cap 0.03 proud from H − 0.075. */
	inline constexpr double PlinthBase = 0.030;
	inline constexpr double PlinthBaseFillet = 0.060;
	inline constexpr double PlinthBaseTop = 0.100;
	inline constexpr double PlinthCap = 0.030;
	inline constexpr double PlinthCapHigh = 0.075;
	inline constexpr double PlinthCapFillet = 0.020;
	/** The herms: a moulded plinth 0.40 square × 0.14, the shaft 0.26 square at 0.13 widening to 0.32 at 1.17, the cap 0.09 from 1.16. */
	inline constexpr double HermPlinthHigh = 0.140;
	inline constexpr double HermShaftFoot = 0.130;
	inline constexpr double HermShaftFootSide = 0.260;
	inline constexpr double HermShaftTop = 1.170;
	inline constexpr double HermShaftTopSide = 0.320;
	inline constexpr double HermCapFoot = 1.160;
	/** The centrepiece's pedestal, turned: Ø 2.20 × 0.30. */
	inline constexpr double CentreRadius = 1.100;

	/** What stands on a carrier (§8), and so how the room builds it. */
	enum class ECarrier : uint8
	{
		NichePlinth,
		NichePedestal,
		Console,
		Herm,
		Plinth,
		Pedestal,
	};

	/** A carrier and its Mount anchor (§2, §8): the anchor's origin is the carrier's top centre, its x the way the work faces. */
	struct FCarrierPlan
	{
		const TCHAR* Anchor;
		ECarrier Kind;
		double X;
		double Y;
		double Top;
		/** The facing, φ in degrees from +x towards +y; negative: towards O. */
		double FacingDeg;
		/** The top's width across the facing and its depth along it. */
		double Width;
		double Depth;
	};

	inline constexpr double TowardsO = -1.0;

	inline constexpr FCarrierPlan Carriers[] = {
		{TEXT("gallery.niche.e1"), ECarrier::NichePedestal, 3.247, 28.100, 0.57, 180.0, 0.84, 0.70},
		{TEXT("gallery.niche.e2"), ECarrier::NichePlinth, 3.315, 22.100, 0.57, 180.0, 0.61, 0.54},
		{TEXT("gallery.niche.e3"), ECarrier::NichePedestal, 3.250, 16.100, 0.57, 180.0, 0.96, 0.60},
		{TEXT("gallery.niche.e4"), ECarrier::NichePedestal, 3.267, 10.100, 0.57, 180.0, 0.88, 0.63},
		{TEXT("gallery.niche.w1"), ECarrier::NichePedestal, -3.226, 28.100, 0.57, 0.0, 0.75, 0.80},
		{TEXT("gallery.niche.w2"), ECarrier::NichePedestal, -3.229, 22.100, 0.57, 0.0, 0.94, 0.66},
		{TEXT("gallery.niche.w3"), ECarrier::NichePlinth, -3.315, 16.100, 0.57, 0.0, 0.61, 0.52},
		{TEXT("gallery.niche.w4"), ECarrier::NichePlinth, -3.315, 10.100, 0.57, 0.0, 0.68, 0.50},
		{TEXT("gallery.bust.e1s"), ECarrier::Console, 2.850, 29.700, 1.55, 180.0, 0.36, 0.30},
		{TEXT("gallery.bust.e1n"), ECarrier::Console, 2.850, 26.500, 1.55, 180.0, 0.36, 0.30},
		{TEXT("gallery.bust.e2s"), ECarrier::Console, 2.850, 23.700, 1.55, 180.0, 0.36, 0.30},
		{TEXT("gallery.bust.e2n"), ECarrier::Console, 2.850, 20.500, 1.55, 180.0, 0.36, 0.30},
		{TEXT("gallery.bust.e3s"), ECarrier::Console, 2.850, 17.700, 1.55, 180.0, 0.36, 0.30},
		{TEXT("gallery.bust.e3n"), ECarrier::Console, 2.845, 14.500, 1.55, 180.0, 0.36, 0.31},
		{TEXT("gallery.bust.e4s"), ECarrier::Console, 2.850, 11.700, 1.55, 180.0, 0.36, 0.30},
		{TEXT("gallery.bust.e4n"), ECarrier::Console, 2.850, 8.500, 1.55, 180.0, 0.58, 0.30},
		{TEXT("gallery.bust.w1s"), ECarrier::Console, -2.850, 29.700, 1.55, 0.0, 0.36, 0.30},
		{TEXT("gallery.bust.w1n"), ECarrier::Console, -2.850, 26.500, 1.55, 0.0, 0.36, 0.30},
		{TEXT("gallery.bust.w2s"), ECarrier::Console, -2.840, 23.700, 1.55, 0.0, 0.36, 0.32},
		{TEXT("gallery.bust.w2n"), ECarrier::Console, -2.850, 20.500, 1.55, 0.0, 0.36, 0.30},
		{TEXT("gallery.bust.w3s"), ECarrier::Console, -2.850, 17.700, 1.55, 0.0, 0.36, 0.30},
		{TEXT("gallery.bust.w3n"), ECarrier::Console, -2.850, 14.500, 1.55, 0.0, 0.36, 0.30},
		{TEXT("gallery.bust.w4s"), ECarrier::Console, -2.850, 11.700, 1.55, 0.0, 0.36, 0.30},
		{TEXT("gallery.bust.w4n"), ECarrier::Console, -2.850, 8.500, 1.55, 0.0, 0.36, 0.30},
		{TEXT("gallery.herm.w"), ECarrier::Herm, -2.450, 30.830, 1.25, 270.0, 0.40, 0.40},
		{TEXT("gallery.herm.e"), ECarrier::Herm, 2.450, 30.830, 1.25, 270.0, 0.40, 0.40},
		{TEXT("gallery.plinth.2"), ECarrier::Plinth, 0.000, 22.100, 0.55, 0.0, 2.30, 1.04},
		{TEXT("gallery.plinth.3"), ECarrier::Plinth, 0.000, 16.100, 0.55, 180.0, 2.30, 1.04},
		{TEXT("tribune.niche.n"), ECarrier::NichePedestal, 0.000, -6.960, 0.53, 90.0, 0.87, 0.81},
		{TEXT("tribune.niche.ne"), ECarrier::NichePedestal, 5.807, -3.352, 0.53, 150.0, 1.42, 0.83},
		{TEXT("tribune.niche.nw"), ECarrier::NichePlinth, -6.043, -3.489, 0.53, 30.0, 0.70, 0.58},
		{TEXT("tribune.niche.se"), ECarrier::NichePedestal, 5.798, 3.347, 0.53, 210.0, 1.24, 1.06},
		{TEXT("tribune.niche.sw"), ECarrier::NichePedestal, -6.029, 3.481, 0.51, 330.0, 0.88, 0.80},
		{TEXT("tribune.bust.n-a"), ECarrier::Console, -1.664, -6.211, 1.55, 75.0, 0.36, 0.30},
		{TEXT("tribune.bust.n-b"), ECarrier::Console, 1.664, -6.211, 1.55, TowardsO, 0.36, 0.30},
		{TEXT("tribune.bust.ne-a"), ECarrier::Console, 4.511, -4.511, 1.55, 135.0, 0.39, 0.40},
		{TEXT("tribune.bust.ne-b"), ECarrier::Console, 6.206, -1.663, 1.55, 165.0, 0.36, 0.31},
		{TEXT("tribune.bust.nw-a"), ECarrier::Console, -6.211, -1.664, 1.55, 15.0, 0.36, 0.30},
		{TEXT("tribune.bust.nw-b"), ECarrier::Console, -4.547, -4.547, 1.55, 45.0, 0.36, 0.30},
		{TEXT("tribune.bust.se-a"), ECarrier::Console, 6.211, 1.664, 1.55, TowardsO, 0.36, 0.30},
		{TEXT("tribune.bust.se-b"), ECarrier::Console, 4.547, 4.547, 1.55, TowardsO, 0.36, 0.30},
		{TEXT("tribune.bust.sw-a"), ECarrier::Console, -4.547, 4.547, 1.55, TowardsO, 0.36, 0.30},
		{TEXT("tribune.bust.sw-b"), ECarrier::Console, -6.211, 1.664, 1.55, TowardsO, 0.36, 0.30},
		{TEXT("tribune.plinth.e"), ECarrier::Plinth, 5.100, 0.000, 0.50, 180.0, 0.90, 0.90},
		{TEXT("tribune.plinth.w"), ECarrier::Plinth, -5.100, 0.000, 0.50, 0.0, 1.15, 0.90},
		{TEXT("tribune.centre"), ECarrier::Pedestal, 0.000, 0.000, 0.30, 90.0, 2.20, 2.20},
	};

	// ---- §3.2 the floors -----------------------------------------------------------------------------------------------------
	/** The gallery's carpet x ±2.10 from y 7.50 to 30.70; its end panels 0.50 deep; each bay's square 4.20 with a frame of 0.15. */
	inline constexpr double CarpetHalf = 2.100;
	inline constexpr double CarpetSouthY = 30.700;
	inline constexpr double CarpetNorthY = 7.500;
	inline constexpr double CarpetEndPanel = 0.500;
	inline constexpr double FloorFrame = 0.150;
	inline constexpr double BayDiscRadius = 1.550;
	inline constexpr double BayRingRadius = 1.650;
	/** Between the bays: three cells x −1.95…−0.65, −0.65…0.65, 0.65…1.95, 1.50 deep, each with a lozenge of half-diagonals 0.55. */
	inline constexpr double CellEdges[] = {-1.950, -0.650, 0.650, 1.950};
	inline constexpr double LozengeHalf = 0.550;
	/** The tribune's rings (r from, r to) and its sectors, bounded at φ 15° + 30°k. */
	inline constexpr double TribuneDisc = 1.500;
	inline constexpr double TribuneRing1 = 1.650;
	inline constexpr double TribuneRing2 = 2.900;
	inline constexpr double TribuneRing3 = 3.050;
	inline constexpr double TribuneRing4 = 3.800;
	inline constexpr double TribuneRing5 = 3.950;
	inline constexpr double TribuneRing6 = 6.150;
	inline constexpr double TribuneRing7 = 6.300;
	inline constexpr int32 FloorSectors = 12;
	inline constexpr double FloorFirstSectorDeg = 15.0;
	inline constexpr double InnerDiscRadius = 0.220;
	inline constexpr double InnerDiscAt = 2.275;
	inline constexpr double SectorBorder = 0.100;
	inline constexpr double OuterDiscRadius = 0.800;
	inline constexpr double OuterDiscAt = 5.050;
}
