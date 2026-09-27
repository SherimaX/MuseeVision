#pragma once

#include "CoreMinimal.h"

/**
 * The Rotunda's plan: every number the room is built to, in metres in the stage's frame (origin at the drum's centre on
 * the finished floor; x east, y south, z up; φ from +x towards +y), taken from its design sheet, Docs/Stages/rotunda.md
 * (the section is given with each group). The sheet's own sources are the as-built Rotunda and sun clock (windows/,
 * D-017) and the Story (D-018). Numbers on the sun clock's drum are in the drum's frame z_d, which is the stage's with
 * the drum lowered. Finishes, light, hanging and motion are the schedules' (Data/Stages/rotunda), never here.
 */
namespace Musee::Rotunda::Plan
{
	/** The stage's key (§1). */
	inline constexpr const TCHAR* Stage = TEXT("rotunda");

	/** One element of a profile as the sheet writes it: a point (A, B), or an arc about a centre from one angle to another. */
	struct FStep
	{
		bool bArc = false;
		double A = 0.0;
		double B = 0.0;
		double RadiusA = 0.0;
		double RadiusB = 0.0;
		double FromDeg = 0.0;
		double ToDeg = 0.0;
	};

	/** A point of a profile. */
	constexpr FStep P(double A, double B) { return FStep{false, A, B, 0.0, 0.0, 0.0, 0.0}; }

	/** A circular arc about (A, B). */
	constexpr FStep Arc(double A, double B, double Radius, double FromDeg, double ToDeg)
	{
		return FStep{true, A, B, Radius, Radius, FromDeg, ToDeg};
	}

	/** An elliptical arc about (A, B), radius RadiusA along A and RadiusB along B. */
	constexpr FStep Ellipse(double A, double B, double RadiusA, double RadiusB, double FromDeg, double ToDeg)
	{
		return FStep{true, A, B, RadiusA, RadiusB, FromDeg, ToDeg};
	}

	// ---- §1 identity, footprint and the well -----------------------------------------------------------------------
	/** The drum's inner face and outer face; the footprint is the outer circle. */
	inline constexpr double DrumInner = 10.000;
	inline constexpr double DrumOuter = 11.200;
	/** The drum's faces are built at 320 stations per turn (§3 build-ups, as built); every 22.5° reference point is one. */
	inline constexpr int32 DrumStations = 320;
	/** The room's vertical extent, and the well prism the stair serves (§1: r 7.60, z -6.40 to 0.00). */
	inline constexpr double FootprintBottom = -6.400;
	inline constexpr double FootprintTop = 21.000;
	inline constexpr double WellRadius = 7.600;
	inline constexpr double WellBottom = -6.400;
	inline constexpr double WellTop = 0.000;

	// ---- §2 ports: the doorway classes (width x crown) and the stair's portal --------------------------------------
	inline constexpr const TCHAR* WestEastClass = TEXT("doorway.3000x6000.arched");
	inline constexpr const TCHAR* NorthSouthClass = TEXT("doorway.4000x6500.arched");
	inline constexpr const TCHAR* StairClass = TEXT("stairwell.spiral.3600x4400.arched");
	inline constexpr double WestEastClear = 3.000;
	inline constexpr double NorthSouthClear = 4.000;
	inline constexpr double DoorSpringing = 4.500;
	/** The class's lining allowance each side (Site's; the drafts' 0.050), and the rough sill under the threshold. */
	inline constexpr double LiningAllowance = 0.050;
	inline constexpr double RoughSill = -0.050;
	/** The doors' plan angles: east, south, west, north. */
	inline constexpr double EastDeg = 0.0;
	inline constexpr double SouthDeg = 90.0;
	inline constexpr double WestDeg = 180.0;
	inline constexpr double NorthDeg = 270.0;
	/** A doorway through the drum: its port's name, plan angle, clear width, class and the port it is designed to meet. */
	struct FDoor
	{
		const TCHAR* Name;
		double Deg;
		double Clear;
		const TCHAR* Class;
		const TCHAR* Mates;
	};
	inline constexpr FDoor Doors[] = {{TEXT("east"), EastDeg, WestEastClear, WestEastClass, TEXT("hall-of-light.west")},
		{TEXT("south"), SouthDeg, NorthSouthClear, NorthSouthClass, TEXT("albion.north")},
		{TEXT("west"), WestDeg, WestEastClear, WestEastClass, TEXT("salon.east")},
		{TEXT("north"), NorthDeg, NorthSouthClear, NorthSouthClass, TEXT("chenghuai.south")}};
	/** The stair's port: its threshold on the landing, clear 3.600, arched from 2.600 above it, crown 4.400. */
	inline constexpr double LandingZ = -6.000;
	inline constexpr double StairPortClear = 3.600;
	inline constexpr double StairPortSpringing = 2.600;
	/** The portal onto the stair under the pocket: flat-headed, its lintel's underside (§2, §3.7). */
	inline constexpr double PortalHead = -3.680;

	// ---- §3.1 floors: the fixed ring and the dial ------------------------------------------------------------------
	/** Every Crema Marfil floor is a laid finish of 30 mm over its core. */
	inline constexpr double FinishDepth = 0.030;
	/** The floor structure's underside. */
	inline constexpr double FloorBed = -0.550;
	/**
	 * The fixed ring, r 7.000 to the drum's face r 10.000. The sheet runs it to 10.040 under the shadow gap; here the
	 * 40 mm strip under the gap is the drum's plinth course, so the ring stops on r 10.000 all round, doorways included.
	 */
	inline constexpr double RingInner = 7.000;
	inline constexpr double RingOuter = 10.000;

	/** A course of radial slabs: its radii, its slab count, and the offset of its first joint. */
	struct FCourse
	{
		double Inner;
		double Outer;
		int32 Slabs;
		double OffsetDeg;
	};
	inline constexpr FCourse Courses[] = {{7.00, 7.75, 48, 0.0}, {7.75, 8.95, 32, 0.0}, {8.95, RingOuter, 64, 2.8125}};

	/**
	 * The motto on the middle course: HORAS · NON · NUMERO · NISI · SERENAS in cast bronze capitals of Centaur, cap 0.26 on
	 * the baseline r 8.53, tops towards the dial, centred on φ 90° (south) and read from the ring facing the dial; tracking
	 * 0.16 cap, 8 mm proud, 2.5 mm bevel; each stop a gap of 1.15 cap holding a gilt triangle, point down, ±0.12 cap at 0.60
	 * cap, its point at 0.36 cap and its apex 8 mm high.
	 */
	inline constexpr const TCHAR* MottoWords[] = {TEXT("HORAS"), TEXT("NON"), TEXT("NUMERO"), TEXT("NISI"), TEXT("SERENAS")};
	inline constexpr const TCHAR* MottoFace = TEXT("centaur.capitals");
	inline constexpr double MottoCap = 0.260;
	inline constexpr double MottoBaseline = 8.530;
	inline constexpr double MottoCentreDeg = 90.0;
	inline constexpr double MottoTracking = 0.16;
	inline constexpr double MottoWordSpace = 0.6;
	inline constexpr double MottoProud = 0.008;
	inline constexpr double MottoBevel = 0.0025;
	inline constexpr double MottoStopGap = 1.15;
	inline constexpr double MottoStopHalfWidth = 0.12;
	inline constexpr double MottoStopTop = 0.60;
	inline constexpr double MottoStopPoint = 0.36;
	inline constexpr double MottoStopApex = 0.008;

	/** The dial: the lifting drum's top, r 0 to 6.990, z_d 0; ring and dial meet with 10 mm clear. */
	inline constexpr double DialRadius = 6.990;

	/** A band of the dial and its surface (§3.1). */
	struct FBand
	{
		double Inner;
		double Outer;
		const TCHAR* Surface;
	};
	inline constexpr FBand DialBands[] = {{0.00, 1.40, TEXT("dial.field.a")}, {1.40, 1.52, TEXT("dial.band.red")},
		{1.52, 3.20, TEXT("dial.field.b")}, {3.20, 4.90, TEXT("dial.field.a")}, {4.90, 6.30, TEXT("dial.field.b")},
		{6.30, 6.38, TEXT("dial.band.black")}, {6.38, DialRadius, TEXT("dial.field.a")}};

	/** The bronze fillets on the band joints, each set in a groove 2 mm deep: 10 wide, 2 proud, 0.8 bevel. */
	inline constexpr double FilletRadii[] = {1.40, 1.52, 6.30, 6.38};
	inline constexpr double FilletWidth = 0.010;
	inline constexpr double FilletProud = 0.002;
	inline constexpr double FilletBevel = 0.0008;
	inline constexpr double InlayGroove = 0.002;

	/**
	 * The gilt sun's boss, a lathe (r, z): a foot 2 mm in its groove, a shoulder rolled over r 0.020 about
	 * (0.265, 0.035), then the dome, a sphere R 1.1854 whose top is z 0.085.
	 */
	inline constexpr double BossRadius = 0.285;
	inline constexpr double BossShoulder = 0.035;
	inline constexpr double BossRoll = 0.020;
	inline constexpr double BossDomeRadius = 1.1854;
	inline constexpr double BossTop = 0.085;
	/** The plain bead ring on the dome (r 0.005 at r 0.19) and the plain foot torus (r 0.014 at r 0.30, z 0.012). */
	inline constexpr double BeadRadius = 0.005;
	inline constexpr double BeadAt = 0.190;
	inline constexpr double FootTorusRadius = 0.014;
	inline constexpr double FootTorusAt = 0.300;
	inline constexpr double FootTorusZ = 0.012;

	/** The sun's 32 rays at 11.25°·i from r 0.24: 16 long (i even) to r 1.25, 72 wide, and 16 short flames to r 0.86, 60 wide. */
	inline constexpr int32 Rays = 32;
	inline constexpr double RayRoot = 0.240;
	inline constexpr double LongRayTip = 1.250;
	inline constexpr double LongRayWidth = 0.072;
	inline constexpr double LongRayTaper = 0.62;
	inline constexpr double ShortRayTip = 0.860;
	inline constexpr double ShortRayWidth = 0.060;
	inline constexpr double ShortRayTaper = 0.75;
	/** Each ray's taper is drawn through this many stations from root to tip. */
	inline constexpr int32 RayStations = 16;

	// ---- §3.2 the drum ------------------------------------------------------------------------------------------------
	/** The shadow gap at the drum's foot: z 0 to 0.150, recessed 0.040 (its back r 10.040). */
	inline constexpr double ShadowGapTop = 0.150;
	inline constexpr double ShadowGapDepth = 0.040;
	/** The inner face stops behind the entablature at z 10.00; the drum's top annulus is z 11.00, r 10.553 to 11.20. */
	inline constexpr double DrumWallTop = 10.000;
	inline constexpr double DrumTop = 11.000;

	/** The four niches on the diagonals: SE, SW, NW, NE. */
	inline constexpr double NicheDegrees[] = {45.0, 135.0, 225.0, 315.0};
	inline constexpr const TCHAR* NicheNames[] = {TEXT("se"), TEXT("sw"), TEXT("nw"), TEXT("ne")};
	/** 2.00 wide on the chord plane at r 10.00, sill 2.00, springing 4.67; the half-round back r 1.00 reaches r 11.00. */
	inline constexpr double NicheHalfWidth = 1.000;
	inline constexpr double NicheSill = 2.000;
	inline constexpr double NicheSpringing = 4.670;
	/** Where a cast stands on its sill: r 10.25 on the niche's axis (§8). */
	inline constexpr double CastRadius = 10.250;

	// ---- §3.3 the order -------------------------------------------------------------------------------------------------
	/** The sixteen pilasters, stepped clear of the doors so each abacus clears its jamb. */
	inline constexpr double PilasterDegrees[] = {11.776, 33.750, 56.250, 75.337, 104.663, 123.750, 146.250, 168.224, 191.776,
		213.750, 236.250, 255.337, 284.663, 303.750, 326.250, 348.224};
	inline constexpr double PilasterWidth = 0.900;
	inline constexpr double PilasterFace = 9.570;
	/** The shaft's plain foot to 0.60, flutes stopped square from 0.60 to 8.62, plain to the capital's top at 9.47. */
	inline constexpr double FlutesFrom = 0.600;
	inline constexpr double FlutesTo = 8.620;
	inline constexpr double CapitalTop = 9.470;
	/** 7 flutes of 0.085 between 6 fillets of 0.0275, with margins of 0.070; each 0.030·sin(πt) deep. */
	inline constexpr int32 Flutes = 7;
	inline constexpr double FluteMargin = 0.070;
	inline constexpr double FluteWidth = 0.085;
	inline constexpr double FluteFillet = 0.0275;
	inline constexpr double FluteDepth = 0.030;
	/** Each flute's sine is drawn through this many stations. */
	inline constexpr int32 FluteStations = 9;

	/** The Tuscan torus base, (a out, z), swept round the front and returns. */
	inline constexpr FStep PilasterBase[] = {P(0.10, 0.0), P(0.10, 0.18), P(0.03, 0.18), Arc(0.03, 0.24, 0.06, -90.0, 90.0), P(0.02, 0.30),
		Arc(0.02, 0.32, 0.02, 270.0, 180.0), P(0.0, 0.0)};
	/** The Roman Doric capital: its astragal, then the echinus and abacus, the abacus's top flat back to the wall. */
	inline constexpr FStep PilasterAstragal[] = {P(0.0, 8.90), Arc(0.0, 8.93, 0.03, -90.0, 90.0)};
	inline constexpr FStep PilasterCapital[] = {P(0.0, 9.14), P(0.025, 9.14), P(0.025, 9.17), Arc(0.025, 9.26, 0.09, -90.0, 0.0),
		P(0.115, 9.47), P(0.0, 9.47)};

	/** The entablature (plain frieze, cavetto crown), a ring (r, z) from the dome's springing down to the wall. */
	inline constexpr FStep Entablature[] = {P(9.9875, 10.500), P(9.220, 10.500), P(9.220, 10.460), Arc(9.220, 10.400, 0.060, 90.0, 0.0),
		P(9.280, 10.240), P(9.420, 10.240), P(9.420, 10.210), Arc(9.510, 10.210, 0.090, 180.0, 270.0), P(9.510, 9.840), P(9.430, 9.840),
		P(9.430, 9.820), Arc(9.480, 9.820, 0.050, 180.0, 270.0), P(9.480, 9.630), P(9.500, 9.630), P(9.500, 9.470),
		P(DrumInner, 9.470), P(DrumInner, DrumWallTop)};

	/** The door archivolts (two fascias and a roll), (s out from the clear intrados, p proud), springing to springing. */
	inline constexpr FStep DoorArchivolt[] = {P(0.30, 0.0), P(0.30, 0.10), Arc(0.27, 0.10, 0.03, 0.0, 90.0), P(0.20, 0.13), P(0.20, 0.10),
		P(0.09, 0.10), P(0.09, 0.08), P(0.0, 0.08), P(0.0, 0.0)};
	/** The niche surrounds (two steps and a roll), (s out from the niche's edge, p proud), up each jamb and round the head. */
	inline constexpr FStep NicheSurround[] = {P(0.20, 0.0), P(0.20, 0.05), Arc(0.17, 0.05, 0.03, 0.0, 90.0), P(0.07, 0.08), P(0.07, 0.06),
		P(0.0, 0.06), P(0.0, 0.0)};
	/** The niche sill ledge (d proud of the chord plane, z), ±1.26 about the axis, its ends capped. */
	inline constexpr FStep NicheSillLedge[] = {P(0.0, 1.84), P(0.13, 1.84), Arc(0.13, 1.87, 0.03, -90.0, 0.0), P(0.16, 2.00), P(0.0, 2.00)};
	inline constexpr double SillHalfLength = 1.260;

	/**
	 * A moulding on the drum's curved face is laid as voussoirs, each a short sweep in the plane tangent to the face at
	 * its middle: no longer than this across the face, so no voussoir stands more than 2 mm off the face (a chord of
	 * 0.40 on r 10.00 has a sagitta of 2 mm).
	 */
	inline constexpr double VoussoirMeters = 0.400;

	// ---- §3.4 the dome -------------------------------------------------------------------------------------------------
	/** The inner sphere R 10.00 about (0, 0, 10.00); the coffers' panels on R 10.45; the shell's outer sphere R 10.60. */
	inline constexpr double DomeCentreZ = 10.000;
	inline constexpr double DomeRadius = 10.000;
	inline constexpr double PanelRadius = 10.450;
	inline constexpr double ShellRadius = 10.600;
	/** The coffers run from 4° to 64° of elevation, 5 rings of 12° by 28 cells. */
	inline constexpr double CofferFromDeg = 4.0;
	inline constexpr int32 CofferRings = 5;
	inline constexpr double CofferRingDeg = 12.0;
	inline constexpr int32 CofferCells = 28;
	/** A coffer's steps: the rib, the step and the bevel as fractions of its cell, and the depths of its three sinkings. */
	inline constexpr double RibOfCell = 0.11;
	inline constexpr double StepOfCell = 0.045;
	inline constexpr double BevelOfCell = 0.05;
	inline constexpr double FirstSinking = 0.12;
	inline constexpr double SecondSinking = 0.26;
	inline constexpr double PanelSinking = PanelRadius - DomeRadius;
	/** The eye's curb (r, z) inside the upper band's end (r 3.000): a drip, the curb's inner face r 2.50, its top z 20.45. */
	inline constexpr double EyeCurbRadius = 3.000;
	inline constexpr double CurbPoints[][2] = {{3.000, 19.420}, {2.750, 19.420}, {2.750, 19.580}, {2.500, 19.580}, {2.500, 20.450}, {2.900, 20.450}};
	/** The shell's outer sphere stops at r 2.90, where the curb rises over it. */
	inline constexpr double ShellTopRadius = 2.900;
	/**
	 * The dome's covering: milled lead on the shell line, Code 8 sheet, 3.55 mm (BS EN 12588:2006, table 2), laid over
	 * the curb's top.
	 */
	inline constexpr double LeadMeters = 0.00355;

	// ---- §3.5 the eye --------------------------------------------------------------------------------------------------
	/** The lattice's ring against the curb, r 2.42 to 2.50, z 20.22 to 20.38. */
	inline constexpr double LatticeRingInner = 2.420;
	inline constexpr double LatticeRingOuter = 2.500;
	inline constexpr double LatticeRingBottom = 20.220;
	inline constexpr double LatticeRingTop = 20.380;
	/** Bars in three directions, 7 each at lateral offsets 0.80·k, 0.05 wide by 0.10 deep, each running 0.04 into the ring. */
	inline constexpr double BarDegrees[] = {0.0, 60.0, 120.0};
	inline constexpr int32 BarsEachSide = 3;
	inline constexpr double BarPitch = 0.800;
	inline constexpr double BarWidth = 0.050;
	inline constexpr double BarBottom = 20.250;
	inline constexpr double BarTop = 20.350;
	inline constexpr double BarsReach = LatticeRingInner + 0.040;
	/** The laminated low-iron glass, 2 × 5 mm on a 0.76 mm PVB interlayer, underside z 20.38 on the ring. */
	inline constexpr double GlassRadius = 2.500;
	inline constexpr double GlassBottom = 20.380;
	inline constexpr double GlassMeters = 0.005 + 0.005 + 0.00076;
	/** The node, the sun clock's nodus: a sphere r 0.12 about (0, 0, 20.25), enclosing the bars' crossing. */
	inline constexpr double NodeRadius = 0.120;
	inline constexpr double NodeZ = 20.250;

	// ---- §3.6 the sun clock's drum (z_d) ------------------------------------------------------------------------------
	/** The drum's lift, lowered to raised. */
	inline constexpr double DrumTravel = 3.500;
	/** The wall: face r 6.900, inner face r 6.545; the foot and plinth z_d -3.56 to -3.50, r to 6.990. */
	inline constexpr double SunWallFace = 6.900;
	inline constexpr double SunWallInner = 6.545;
	inline constexpr double SunFoot = -3.560;
	inline constexpr double SunSill = -3.500;
	/** The dial's finish lies from z_d -0.030 to 0; the drum's deck under it shows as the coffers' panels at -0.100. */
	inline constexpr double DeckPanel = -0.100;
	/** The rim's top line runs from (6.190, -0.090) to (6.545, -0.060), the groove's roof. */
	inline constexpr double RimInner = 6.190;
	inline constexpr double RimTopInner = -0.090;
	inline constexpr double RimGroove = -0.060;

	/** The base moulding (r, z_d), round the drum except across the door. */
	inline constexpr FStep SunBase[] = {P(6.990, -3.500), P(6.990, -3.300), P(6.955, -3.300), Arc(6.955, -3.265, 0.035, -90.0, 90.0),
		P(6.925, -3.230), P(6.925, -3.215), P(6.915, -3.215), Arc(6.915, -3.200, 0.015, -90.0, 90.0), P(6.905, -3.175), P(6.905, -3.100),
		P(6.900, -3.080), P(6.900, -3.500)};
	/** The entablature (r, z_d), all round; its corona is the dial's edge, whose top 30 mm is the dial's outer band. */
	inline constexpr FStep SunEntablature[] = {P(6.900, -0.450), P(6.930, -0.450), P(6.930, -0.395), P(6.940, -0.390), P(6.940, -0.335),
		P(6.952, -0.330), P(6.952, -0.315), P(6.915, -0.315), P(6.915, -0.195), Arc(6.915, -0.150, 0.045, -90.0, 0.0), P(6.990, -0.150),
		P(6.990, -FinishDepth), P(6.900, -FinishDepth)};
	/** The bronze rim under the dial's edge (r, z_d); its groove r 6.46 to 6.545 takes the sleeve's top when lowered. */
	inline constexpr FStep SunRim[] = {P(RimInner, RimTopInner), P(6.190, -0.125), Arc(6.205, -0.125, 0.015, 180.0, 270.0), P(6.320, -0.140),
		P(6.330, -0.132), P(6.340, -0.140), P(6.460, -0.140), P(6.460, RimGroove), P(SunWallInner, RimGroove)};
	/** The gilt pendant boss over the well (r, z_d): a low dome, a bead, a flare and a roll into the soffit. */
	inline constexpr FStep SunPendant[] = {P(0.0, -0.265), Ellipse(0.0, -0.225, 0.110, 0.040, -90.0, 0.0), Arc(0.125, -0.225, 0.015, 180.0, 0.0),
		P(0.140, -0.215), P(0.240, -0.190), P(0.340, -0.170), P(0.400, -0.158), Arc(0.400, -0.148, 0.010, -90.0, 90.0), P(0.400, -0.120),
		P(0.0, -0.120)};

	/** The door, west: radial jambs at ±10.860°, sill z_d -3.50, springing -1.85, 2.60 wide at the face. */
	inline constexpr double SunDoorDeg = 180.0;
	inline constexpr double SunDoorHalfDeg = 10.860;
	inline constexpr double SunDoorSpringing = -1.850;
	/** Its archivolt's centre line 1.355 from the axis, down both jambs to z_d -3.10; (across, proud). */
	inline constexpr double SunArchivoltRadius = 1.355;
	inline constexpr double SunArchivoltFoot = -3.100;
	inline constexpr FStep SunArchivolt[] = {P(0.045, -0.010), P(0.045, 0.036), Arc(0.033, 0.036, 0.012, 0.0, 180.0), P(0.010, 0.036),
		P(0.010, 0.030), P(-0.045, 0.030), P(-0.045, -0.010)};

	/** The twelve pilasters at 15° + 30°·k; each part's width, proud of the face on its axis, and heights (z_d). */
	inline constexpr int32 SunPilasters = 12;
	inline constexpr double SunPilasterFirstDeg = 15.0;
	struct FPilasterPart
	{
		const TCHAR* Name;
		double Width;
		double Proud;
		double Bottom;
		double Top;
		bool bFluted;
	};
	inline constexpr FPilasterPart SunPilasterParts[] = {{TEXT("pedestal"), 0.50, 0.075, -3.10, -2.98, false},
		{TEXT("shaft"), 0.44, 0.055, -2.98, -0.62, true}, {TEXT("necking"), 0.47, 0.064, -0.62, -0.56, false},
		{TEXT("echinus"), 0.49, 0.071, -0.56, -0.53, false}, {TEXT("abacus"), 0.51, 0.080, -0.53, -0.45, false}};
	/** The shaft's 5 flutes of 0.0632 between fillets of 0.016, margins 0.03, each a circular arc sagging 0.018. */
	inline constexpr int32 SunFlutes = 5;
	inline constexpr double SunFluteWidth = 0.0632;
	inline constexpr double SunFluteFillet = 0.016;
	inline constexpr double SunFluteMargin = 0.030;
	inline constexpr double SunFluteSag = 0.018;

	/** The soffit (the stair's ceiling): ring ribs at z_d -0.13, each coffer sunk to a 40 mm frame at -0.115, panel -0.100. */
	inline constexpr double SoffitRib = -0.130;
	inline constexpr double SoffitFrame = -0.115;
	inline constexpr double SoffitFrameWidth = 0.040;
	/** The soffit's plate reaches in under the rim, which hangs from r 6.190. */
	inline constexpr double SoffitOuter = 6.200;
	/** A ring of coffers: its radii, its count, and its radial ribs' width in degrees. */
	struct FCofferRing
	{
		double Inner;
		double Outer;
		int32 Coffers;
		double RibDeg;
	};
	inline constexpr FCofferRing SoffitCoffers[] = {{0.55, 1.25, 8, 6.5625}, {1.35, 2.25, 12, 2.8125}, {2.35, 3.45, 16, 1.875},
		{3.55, 4.75, 24, 0.9375}, {4.85, RimInner, 32, 0.9375}};

	// ---- §3.7 the shaft ---------------------------------------------------------------------------------------------------
	/** The sleeve (the stair's wall): face r 6.480, back r 6.540, from the landing to its top z -0.070. */
	inline constexpr double SleeveFace = 6.480;
	inline constexpr double SleeveBack = 6.540;
	inline constexpr double SleeveTop = -0.070;
	/** The pocket the drum's wall slides into: r 6.54 to 7.00, its floor z -3.58, open to z 0. */
	inline constexpr double PocketOuter = 7.000;
	inline constexpr double PocketFloor = -3.580;
	/** The masonry to the port face r 7.60; the port's arched reveal runs in to r 7.05, where the tympanum closes it. */
	inline constexpr double RevealInner = 7.050;
	/** The portal's jambs are the planes x = ±1.80. */
	inline constexpr double PortalHalfWidth = 1.800;
	/** The eight ram bores, Ø 0.36 on r 6.77 at 22.5° + 45°·k, from the pocket floor down to z -5.90. */
	inline constexpr int32 Bores = 8;
	inline constexpr double BoreRadius = 6.770;
	inline constexpr double BoreDiameter = 0.360;
	inline constexpr double BoreFirstDeg = 22.5;
	inline constexpr double BoreBottom = -5.900;
	/**
	 * A ram's outer barrel in its bore: 0.30, leaving 30 mm round it in the 0.36 bore. The rams' stages and pressures are
	 * Motion's drive; the barrel stands on the bore's bottom with its head under the drum's foot.
	 */
	inline constexpr double RamDiameter = 0.300;
	/** The foundation under the landing, down to the well prism's floor. */
	inline constexpr double FoundationBottom = WellBottom;

	/** The landing's rosette: a pale eye, a black 32-gon disc, a 16-point star, a pale field, a black band and a red band. */
	inline constexpr double RosetteEye = 0.120;
	inline constexpr double RosetteDisc = 0.300;
	inline constexpr double RosetteLongPoint = 1.800;
	inline constexpr double RosetteShortPoint = 1.200;
	inline constexpr double RosetteField = 1.900;
	inline constexpr double RosetteBlack = 2.000;
	inline constexpr double RosetteRed = 2.060;
	inline constexpr int32 RosettePoints = 16;

	// ---- §3.8 the stair ------------------------------------------------------------------------------------------------
	/** 40 risers of 0.150, riser 1 from the raised drum's sill (z 0) to the top landing. */
	inline constexpr int32 Risers = 40;
	inline constexpr double Riser = 0.150;
	/** The top landing (tread 1) spans φ 194° to 164°; the mid landing is tread 20, turning 20°. */
	inline constexpr double TopLandingBackDeg = 194.0;
	inline constexpr double FirstNosingDeg = 164.0;
	inline constexpr int32 MidLandingTread = 20;
	inline constexpr double MidLandingDeg = 20.0;
	/** The other 37 treads share what the landings leave of 214°. */
	inline constexpr double TreadDeg = (214.0 - MidLandingDeg) / 37.0;
	/** Treads span r 2.35 to 6.48, running 10 mm into the string and the wall. */
	inline constexpr double TreadInner = 2.350;
	inline constexpr double TreadOuter = SleeveFace;
	inline constexpr double TreadInto = 0.010;
	/** The soffit is 0.40 under the pitch line, never below z -6.02. */
	inline constexpr double SoffitUnderPitch = 0.400;
	inline constexpr double SoffitFloor = -6.020;
	/** The inner string r 2.20 to 2.35: 0.12 over the pitch line, never above z -0.14, flush with the top landing. */
	inline constexpr double StringInner = 2.200;
	inline constexpr double StringOverPitch = 0.120;
	inline constexpr double StringCeiling = -0.140;
	/** The string runs half a degree past the last riser. */
	inline constexpr double StringPastDeg = 0.5;
	/** The nosing (forward of the riser line, up from the tread's top). */
	inline constexpr FStep Nosing[] = {P(0.0, -0.050), P(0.022, -0.040), Arc(0.022, -0.020, 0.020, -90.0, 90.0), P(0.0, 0.0)};
	/**
	 * The treads' and the string's surfaces are set out on a grid of 0.10: the soffit's helicoid, whose curvature is at most
	 * about 0.2 per metre here, is followed within 0.10^2 x 0.2 / 8 = 0.25 mm.
	 */
	inline constexpr double StairGrid = 0.100;

	// ---- §3.9 fittings: the well balustrade, its housings, the wall rail -----------------------------------------------
	/** The well balustrade stands in the plane r 2.15, its heights measured plumb above the string's top. */
	inline constexpr double BalustradeRadius = 2.150;
	/** Three bars 25 high by 20 thick, their middles at these heights; the handrail's middle at 0.90. */
	inline constexpr double BarHeights[] = {0.0575, 0.5325, 0.8675};
	inline constexpr double BarHalfHigh = 0.0125;
	inline constexpr double BarThick = 0.020;
	inline constexpr double HandrailHeight = 0.900;
	/** The handrail: a soft bar 60 across by 45 high, a superellipse (cos t, sin t each raised to 0.7), at 24 stations. */
	inline constexpr double HandrailAcross = 0.060;
	inline constexpr double HandrailHigh = 0.045;
	inline constexpr double HandrailRoundness = 0.7;
	inline constexpr int32 HandrailStations = 24;
	/** Standards 30 square, at most 1.60 apart and at every kink, from 0.25 below the string's top to 10 mm under the rail. */
	inline constexpr double StandardSide = 0.030;
	inline constexpr double StandardPitch = 1.600;
	inline constexpr double StandardFoot = -0.250;
	inline constexpr double StandardUnderRail = 0.010;
	/** Each standard's bracket to the string: 24 wide, from 0.23 to 0.17 below the string's top. */
	inline constexpr double BracketWide = 0.024;
	inline constexpr double BracketBottom = -0.230;
	inline constexpr double BracketTop = -0.170;
	/** Balusters 14 square, about 0.110 apart between the bottom and middle bars, the first 0.040 in from a standard. */
	inline constexpr double BalusterSide = 0.014;
	inline constexpr double BalusterPitch = 0.110;
	inline constexpr double BalusterClear = 0.040;
	/**
	 * Rings between the middle and top bars, touching both on the rake (§3.9): the band's outer edge runs 2 mm into the bars,
	 * the band 12 wide and 12 thick; a boss r 0.022, 22 thick; four spokes 7 wide, 8 thick, at 45° + 90° k, running 2 mm
	 * into the band (SunClockRails.cpp:196-212, as built); the rings keep 10 mm clear of the standards.
	 */
	inline constexpr double RingBandOver = 0.002;
	inline constexpr double RingBand = 0.012;
	inline constexpr double RingThick = 0.012;
	inline constexpr double RingBossRadius = 0.022;
	inline constexpr double RingBossThick = 0.022;
	inline constexpr double SpokeWide = 0.007;
	inline constexpr double SpokeThick = 0.008;
	inline constexpr double SpokeInto = 0.002;
	inline constexpr double RingClear = 0.010;
	/**
	 * The run ends 1° past the last riser, at the newel: a post 56 square on the landing to 0.03 over the rail's middle, a cap
	 * 72 by 20, a ball r 0.045 sitting 3 mm into the cap (SunClockRails.cpp:450-459, as built).
	 */
	inline constexpr double BalustradePastDeg = 1.0;
	inline constexpr double NewelSide = 0.056;
	inline constexpr double NewelOverRail = 0.030;
	inline constexpr double NewelCapSide = 0.072;
	inline constexpr double NewelCapHigh = 0.020;
	inline constexpr double NewelBallRadius = 0.045;
	inline constexpr double NewelBallSeat = 0.003;
	/** The split: below tread 8's nosing the balustrade is fixed; above it, and behind the top landing, it retracts. */
	inline constexpr int32 SplitTread = 8;
	/** At home a retracting panel stands 0.94 below its place (Mechanisms.json, rotunda.stair.balustrade's travel). */
	inline constexpr double PanelHome = 0.940;
	/** Behind the top landing the panel's plane stands 0.07 beyond the back edge, in the middle of its 0.14 housing. */
	inline constexpr double BackPanelBehind = 0.070;
	inline constexpr double BackHousingBehind = 0.140;
	/** A panel's carrier bar at its foot, 24 thick, from 0.22 to 0.18 below the string's top; its standards stop 20 mm into it. */
	inline constexpr double CarrierBottom = -0.220;
	inline constexpr double CarrierTop = -0.180;
	inline constexpr double CarrierThick = 0.024;
	inline constexpr double StemInto = 0.020;
	/**
	 * The housings: U channels 1.20 deep under their tops, a 30 mm floor, 40 mm end blocks, a 76 mm slot; along the string
	 * r 2.07 to 2.20, their tops flush with the string's; a bronze cap 0.06 down round the slot and a shoe 0.05 up from the
	 * bottom, marble sides between.
	 */
	inline constexpr double HousingInner = 2.070;
	inline constexpr double HousingOuter = StringInner;
	inline constexpr double HousingDepth = 1.200;
	inline constexpr double HousingFloor = 0.030;
	inline constexpr double HousingEndBlock = 0.040;
	inline constexpr double HousingSlot = 0.076;
	inline constexpr double HousingCap = 0.060;
	inline constexpr double HousingShoe = 0.050;
	/** The well's housing stops a centimetre short of the split's standard; a panel keeps 5 mm clear of its end blocks. */
	inline constexpr double HousingShort = 0.010;
	inline constexpr double PanelClear = 0.005;
	/**
	 * The wall rail: a round bar Ø 0.050 at r 6.40, its middle 0.92 over the pitch line, from tread 7's nosing (its top at
	 * -0.18, under the lowered dial) to 4° past the last riser; returned level to the wall at its top round a bend of 0.05 and
	 * a centimetre into it, on a rose Ø 0.090, 12 proud; brackets every 1.25 from 0.30 along it, each a 16 mm square stem from
	 * 0.085 to 0.015 under the rail's middle and an arm 17 high from its foot a centimetre into the wall
	 * (SunClockRails.cpp:372-410, 488-507, as built).
	 */
	inline constexpr double WallRailRadius = 6.400;
	inline constexpr double WallRailDiameter = 0.050;
	inline constexpr double WallRailOverPitch = 0.920;
	inline constexpr int32 WallRailFromTread = 7;
	inline constexpr double WallRailPastDeg = 4.0;
	inline constexpr double WallReturnBend = 0.050;
	inline constexpr double RoseRadius = 0.045;
	inline constexpr double RoseProud = 0.012;
	inline constexpr double WallBracketPitch = 1.250;
	inline constexpr double WallBracketFirst = 0.300;
	inline constexpr double WallBracketSide = 0.016;
	inline constexpr double WallBracketFoot = -0.085;
	inline constexpr double WallBracketStemTop = -0.015;
	inline constexpr double WallBracketArmTop = -0.068;
}
