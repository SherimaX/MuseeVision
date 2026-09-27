#pragma once

#include "CoreMinimal.h"

/**
 * The Atrium's plan: every number the drum is built to, in metres in the stage's frame (origin on the Élan axis on the
 * marble's face; x east, y south, z up, true north), taken from its design sheet, Docs/Stages/atrium.md (the section is
 * given with each group). The sheet's own sources are the as-built Élan (windows/, D-017) and the approved boards.
 * Bearings are compass bearings, clockwise from north; a point at bearing b and radius r is (r sin b, -r cos b), and a
 * plan angle (from +x towards +y, the angle Form turns by) is the bearing less 90 degrees. Finishes, light, hanging and
 * motion are the schedules' (Data/Stages/atrium), never here.
 */
namespace Musee::Atrium::Plan
{
	/** The stage's key (§1). */
	inline constexpr const TCHAR* Stage = TEXT("atrium");

	/** One step of a drawn section or outline, (A, B) in metres: a line to a point, or a circular arc about a centre. */
	struct FStep
	{
		double A = 0.0;
		double B = 0.0;
		double Radius = 0.0;
		double FromDeg = 0.0;
		double ToDeg = 0.0;
		bool bArc = false;
	};

	/** A line to (A, B). */
	constexpr FStep To(double A, double B) { return FStep{A, B, 0.0, 0.0, 0.0, false}; }

	/** A circular arc about (A, B), from one angle to another (0 = +A, 90 = +B), either way round. */
	constexpr FStep Arc(double A, double B, double Radius, double FromDeg, double ToDeg) { return FStep{A, B, Radius, FromDeg, ToDeg, true}; }

	// ---- the setting-out (the sheet's head): ribs and fins at 15° + 30°k, bays and mullions at 30°k, the door west ----
	inline constexpr int32 Ribs = 12;
	inline constexpr double RibFirstBearing = 15.0;
	inline constexpr double BayStep = 30.0;
	inline constexpr double DoorBearing = 270.0;

	// ---- §1 the footprint: the portal's front and the arc R 15.400, clockwise from G1; its height ------------------------
	inline constexpr double PortalFrontX = -15.000;
	inline constexpr double PortalHalfWidth = 6.050;
	inline constexpr double FootprintRadius = 15.400;
	inline constexpr double FootprintBottom = -0.500;
	inline constexpr double FootprintTop = 22.580;
	/** The footprint's arc is set out at 720 stations of the full turn, as the oval's. */
	inline constexpr int32 FootprintStations = 720;
	/**
	 * The volume turned about the axis (§1, the seams) keeps to the Atrium's side of the seam sphere, drawn outside it with
	 * sides that leave it by at most the museum's chord tolerance, 1 mm (MuseeFormMaking's Stations.h). Inside the seam,
	 * the neck column r 3.000 is the Sphere's from the collar's top (UpPlane) up to the sphere.
	 */
	inline constexpr double SeamToleranceMeters = 0.001;
	inline constexpr double NeckColumn = 3.000;

	// ---- §2 the ports ----------------------------------------------------------------------------------------------------
	/** atrium.west: the base's face r 14.800 on the axis; its wall 0.800 deep (r 14.000 → 14.800). */
	inline constexpr double BaseInner = 14.000;
	inline constexpr double BaseOuter = 14.800;
	/** doorway.4000x4500.square: clear 4.000, square head at 4.500 (MuseePlan.h:171). */
	inline constexpr double DoorClear = 4.000;
	inline constexpr double DoorHead = 4.500;
	/** The rough opening the Atrium builds (a = 0.050 round the class): jambs y ±2.050, head 4.550, sill -0.050. */
	inline constexpr double RoughHalf = 2.050;
	inline constexpr double RoughHead = 4.550;
	inline constexpr double RoughSill = -0.050;
	/** The vertical ports' planes: atrium.down on the slab's soffit, atrium.up on the collar's top (§2). */
	inline constexpr double DownPlane = -0.500;
	inline constexpr double UpPlane = 7.715;
	/** Their depths: the slab and marble (0.500), the collar (7.455 → 7.715). */
	inline constexpr double DownDepth = 0.500;
	inline constexpr double UpDepth = 0.260;
	/** iris.4600: the served well's clear radius, and the wells' far ends below and above. */
	inline constexpr double WellRadius = 2.300;
	inline constexpr double WellBottom = -23.800;
	inline constexpr double WellTop = 23.390;

	// ---- §3.1 structure ----------------------------------------------------------------------------------------------------
	inline constexpr double SlabInner = 2.560;
	inline constexpr double SlabBottom = -0.500;
	inline constexpr double SlabTop = -0.030;

	// ---- §3.2 masonry: the base, the cove, the coping band ------------------------------------------------------------------
	inline constexpr double BaseBottom = -0.500;
	inline constexpr double WallTop = 6.000;
	/** The cove (Housing atrium.cove): r 14.000 → 14.090, z 0.000 → 0.140, square ends 0.350 short of the jambs (y ±2.350). */
	inline constexpr double CoveBack = 14.090;
	inline constexpr double CoveTop = 0.140;
	inline constexpr double CoveEndHalf = 2.350;
	/** The coping band, lathed between the ribs, stopping 5 mm short of their side faces. */
	inline constexpr FStep CopingBand[] = {To(13.900, 5.720), To(14.000, 5.720), To(14.000, 6.000), To(13.930, 6.000), To(13.900, 5.970)};
	/** Joints kept between steelwork and what it passes: the ribs 5 mm off stone and glass, the grid 5 mm short of a rib. */
	inline constexpr double SteelClearance = 0.005;

	/**
	 * The exterior dress, lathed outside the west sector (§3.2), from its foot up the socle, torus and scotia, the ashlar
	 * face, the ovolo, corona and bead, the wash, and back along the wall top to its bed on the base's face (r 14.800).
	 */
	inline constexpr FStep Dress[] = {To(14.800, -0.100), To(15.300, -0.100), To(15.300, 0.400), To(15.260, 0.440), To(15.140, 0.440),
		Arc(15.140, 0.540, 0.100, -90.0, 90.0), To(15.080, 0.640), To(15.080, 0.680), Arc(15.080, 0.860, 0.180, -90.0, -180.0),
		To(14.880, 0.880), To(14.880, 5.200), To(14.940, 5.200), To(14.940, 5.280), Arc(14.940, 5.480, 0.200, -90.0, 0.0),
		To(15.400, 5.480), To(15.400, 5.860), To(15.360, 5.860), To(15.360, 5.890), Arc(15.240, 5.890, 0.120, 0.0, 90.0),
		To(15.200, 6.010), To(15.200, 6.050), To(14.080, 6.200), To(14.080, 6.000), To(14.800, 6.000)};
	/** The dress stops where its bed on the base's face (r 14.800) meets the portal's side (|y| 6.050). */
	inline constexpr double DressBed = 14.800;

	/**
	 * The entablature at the equator (§3.2), lathed all round, split at the corona's soffit (22.050): below it the frieze
	 * and ovolo, notched round each fin (±0.170 tangential, 20.900 → 22.050); above it the corona, bead and wash.
	 */
	inline constexpr FStep EntablatureLower[] = {To(14.080, 20.900), To(14.620, 20.900), To(14.620, 21.100), To(14.700, 21.100),
		To(14.700, 21.340), To(14.760, 21.340), To(14.760, 21.400), To(14.660, 21.400), To(14.660, 21.840), To(14.700, 21.840),
		Arc(14.700, 22.050, 0.210, -90.0, 0.0), To(14.080, 22.050)};
	inline constexpr FStep EntablatureUpper[] = {To(14.080, 22.050), To(15.700, 22.050), To(15.700, 22.300), To(15.660, 22.300),
		To(15.660, 22.330), Arc(15.520, 22.330, 0.140, 0.0, 90.0), To(15.480, 22.470), To(15.480, 22.500), To(14.080, 22.580)};
	inline constexpr double FinNotchHalf = 0.170;

	// ---- §3.2 the portal (after the Pantheon's intermediate block) ------------------------------------------------------------
	inline constexpr double PortalBottom = -0.100;
	inline constexpr double PortalTop = 9.500;
	/** Its opening: jambs y ±4.950 from -0.100 to 5.221, then an arch R 5.800 about (y 0, z 2.200), crown 8.000. */
	inline constexpr double PortalJamb = 4.950;
	inline constexpr double PortalArchRadius = 5.800;
	inline constexpr double PortalArchCentreZ = 2.200;
	/** The drum's outer face the portal's back meets above the wall top (r 14.050). */
	inline constexpr double PortalBackAbove = 14.050;
	/** The cornice, (a out of the face, z), swept along the portal's front and returns, mitred. */
	inline constexpr FStep Cornice[] = {To(0.000, 8.750), To(0.050, 8.750), To(0.050, 8.820), Arc(0.050, 9.000, 0.180, -90.0, 0.0),
		To(0.450, 9.000), To(0.450, 9.320), To(0.410, 9.320), To(0.410, 9.350), Arc(0.300, 9.350, 0.110, 0.0, 90.0), To(0.260, 9.460),
		To(0.260, 9.500), To(0.000, 9.500), To(-0.020, 9.480), To(-0.020, 8.770)};
	inline constexpr double CorniceReturnX = -12.680;

	// ---- §3.3 the floor: marble rings of radial slabs, 6 mm joints; the lip's notches; the waiting ring and mark ---------------
	inline constexpr double MarbleBottom = -0.030;
	inline constexpr double FloorJoint = 0.006;
	/** The rings' radii, from the lip (2.290) to the cove's back (14.090); the waiting ring lies between rings 1 and 2. */
	inline constexpr double RingRadii[] = {2.290, 2.940, 3.060, 4.583, 6.107, 7.630, 9.153, 10.677, 12.200, 14.090};
	/** Slabs in each marble ring (rings 1 to 8); the waiting ring (index 1 above) is bronze. */
	inline constexpr int32 RingSlabs[] = {12, 12, 24, 24, 36, 36, 48, 36};
	/** The lip's and the lining's notches for the rails (bearings 0° and 180°): the blade's slot and the foot's. */
	inline constexpr double BladeSlotHalf = 0.008;
	inline constexpr double BladeSlotOut = 2.385;
	inline constexpr double FootSlotHalf = 0.070;
	inline constexpr double FootSlotOut = 2.430;
	inline constexpr double RailBearings[] = {0.0, 180.0};
	/** The waiting mark: a half ring about (-3.310, 0) on its west half, and a dot at (-3.650, 0), 2 mm proud. */
	inline constexpr double MarkCentreX = -3.310;
	inline constexpr double MarkInner = 0.380;
	inline constexpr double MarkOuter = 0.460;
	inline constexpr double DotX = -3.650;
	inline constexpr double DotRadius = 0.140;
	inline constexpr double MarkBottom = -0.010;
	inline constexpr double MarkTop = 0.002;

	// ---- §3.4 glazing ---------------------------------------------------------------------------------------------------------
	/** The inner leaf r 14.000 → 14.024, z 6.000 → 21.180; panes 15° wide; tiers split at 10, 14, 18; 8 mm butt joints. */
	inline constexpr double InnerLeafOuter = 14.024;
	inline constexpr double CanopyMeetsDrum = 21.180;
	inline constexpr double PaneBearingStep = 15.0;
	inline constexpr double Transoms[] = {10.000, 14.000, 18.000};
	inline constexpr double PaneJoint = 0.008;
	/** The head closer: r 14.000 → 14.080, z 21.180 → 21.200. */
	inline constexpr double CloserOuter = 14.080;
	inline constexpr double CloserTop = 21.200;
	/** The outer leaf r 14.050 → 14.074, z 6.000 → 20.950; the order's root is its outer face. */
	inline constexpr double OuterLeafInner = 14.050;
	inline constexpr double OuterLeafOuter = 14.074;
	inline constexpr double OuterLeafTop = 20.950;
	/** The canopy: the Sphere's underside, upper face R 14.000 on the seam, room face R 14.024, about (0, 0, 22.000). */
	inline constexpr double SphereCentreZ = 22.000;
	inline constexpr double SeamRadius = 14.000;
	inline constexpr double CanopyRadius = 14.024;

	// ---- §3.5 steelwork ---------------------------------------------------------------------------------------------------------
	/** The rib's back line: up the wall at r 13.995 to 9.003, the turn R 3.000 about (10.995, 9.003) to 130.230°, then R 14.024 under the Sphere to the ring. */
	inline constexpr double RibBack = 13.995;
	inline constexpr double RibTurnZ = 9.003;
	inline constexpr double RibTurnCentreR = 10.995;
	inline constexpr double RibTurnRadius = 3.000;
	inline constexpr double RibTurnEndDeg = 130.230;
	/** Its section: 0.420 deep from the back into the room, 0.300 wide, with 30 mm chamfers on its room-side arrises. */
	inline constexpr double RibDepth = 0.420;
	inline constexpr double RibHalf = 0.150;
	inline constexpr double RibChamfer = 0.030;
	/** The shoe: a sleeve 0.435 deep and ±0.165 wide round the rib's foot, z 0.000 → 0.360. */
	inline constexpr double ShoeDepth = 0.435;
	inline constexpr double ShoeHalf = 0.165;
	inline constexpr double ShoeTop = 0.360;
	/** The compression ring, lathed, and its bore (the neck column r 3.000). */
	inline constexpr FStep CompressionRing[] = {To(3.000, 7.745), To(3.600, 7.890), To(3.600, 8.446), To(3.000, 8.301)};
	inline constexpr double RingOuter = 3.600;
	/** The hangers: twelve 10 mm plates, radial r 3.150 → 3.450 at the rib bearings, from the collar's top to the ring's soffit. */
	inline constexpr double HangerInner = 3.150;
	inline constexpr double HangerOuter = 3.450;
	inline constexpr double HangerThickness = 0.010;

	/** The canopy grid: members 60 wide × 120 deep below the room face; nodes on 20 rings, (r, z) from the rim inwards. */
	inline constexpr double MemberWidth = 0.060;
	inline constexpr double MemberDepth = 0.120;
	inline constexpr double GridRings[][2] = {{13.795, 19.478}, {13.573, 18.474}, {13.285, 17.507}, {12.936, 16.585}, {12.537, 15.715},
		{12.095, 14.901}, {11.619, 14.148}, {11.120, 13.455}, {10.604, 12.822}, {10.079, 12.249}, {9.552, 11.732}, {9.030, 11.270},
		{8.515, 10.857}, {8.014, 10.491}, {7.087, 9.898}, {6.232, 9.437}, {5.458, 9.082}, {4.765, 8.810}, {4.149, 8.604}, {3.600, 8.446}};
	/** Rings 1 to 14 carry 72 nodes, 5° apart; rings 15 to 20, 36 nodes 10° apart; ring 1's first node at bearing 90°. */
	inline constexpr int32 DenseRings = 14;
	inline constexpr int32 DenseNodes = 72;
	inline constexpr int32 SparseNodes = 36;
	inline constexpr double FirstNodeBearing = 90.0;

	/** The drum's order, outdoors, rooted on the outer leaf's face; B the seat: 6.200 on the wash, 9.500 on the portal within ±25° of west. */
	inline constexpr double SeatOnWash = 6.200;
	inline constexpr double SeatOnPortal = 9.500;
	inline constexpr double PortalSector = 25.0;
	/** A fin (the elliptical head about (15.600, 19.400), radii 0.800 by 2.680), cut level at the corona's soffit. */
	inline constexpr double FinFoot = 0.800;
	inline constexpr double FinFootOut = 15.000;
	inline constexpr double FinWaistR = 14.800;
	inline constexpr double FinWaistZ = 19.400;
	inline constexpr double FinHeadCentreR = 15.600;
	inline constexpr double FinHeadRadiusR = 0.800;
	inline constexpr double FinHeadRadiusZ = 2.680;
	inline constexpr double FinHeadCut = 22.050;
	inline constexpr double FinHalf = 0.170;
	/** A pedestal under each fin: out to 15.100, 0.780 high, its top chamfered in to 15.030 at 0.850. */
	inline constexpr FStep Pedestal[] = {To(OuterLeafOuter, 0.000), To(15.100, 0.000), To(15.100, 0.780), To(15.030, 0.850), To(OuterLeafOuter, 0.850)};
	inline constexpr double PedestalHalf = 0.280;
	inline constexpr double PedestalHeight = 0.850;
	/** A mullion at 30°k: out to 14.450, from the seat to 20.950. */
	inline constexpr double MullionOut = 14.450;
	inline constexpr double MullionTop = 20.950;
	inline constexpr double MullionHalf = 0.070;
	/** The transoms: rings at 10, 14 and 18 m, 0.100 high, out to 14.300, between the verticals. */
	inline constexpr double TransomOut = 14.300;
	inline constexpr double TransomHalfHeight = 0.050;

	// ---- §3.6 the lift installation ---------------------------------------------------------------------------------------------
	/** The lining: a shell r 2.300 → 2.560, z -0.500 → -0.030, slotted for the rails. */
	inline constexpr double LiningInner = 2.300;
	inline constexpr double LiningOuter = 2.560;
	/** The guide rails: T-section, blade r 2.265 → 2.385 (16 mm), foot r 2.385 → 2.425 (140 mm), from -0.490 to 7.425. */
	inline constexpr double RailBladeIn = 2.265;
	inline constexpr double RailFootIn = 2.385;
	inline constexpr double RailFootOut = 2.425;
	inline constexpr double RailBottom = -0.490;
	inline constexpr double RailTop = 7.425;
	/**
	 * The rails' brackets, 180 × 100, r 2.421 → 2.450, fixing the foot to the landing's head ring (its top, 3.000) and the
	 * tube's ring frame (its top, 5.3775): each stands on the ring it fixes to, never through glass.
	 */
	inline constexpr double BracketIn = 2.421;
	inline constexpr double BracketOut = 2.450;
	inline constexpr double BracketHalfWidth = 0.090;
	inline constexpr double BracketHeight = 0.100;
	inline constexpr double BracketLevels[] = {3.000, 5.3775};

	/** Spans round the landing and the car, degrees clockwise from the door's centre line (bearing 270°). */
	struct FSpan
	{
		double From;
		double To;
	};
	/** The landing's panes r 2.450 → 2.465, z 0.060 → 2.940: pockets either side of the doorway, three large panes. */
	inline constexpr double LandingGlassIn = 2.450;
	inline constexpr double LandingGlassOut = 2.465;
	inline constexpr double LandingGlassBottom = 0.060;
	inline constexpr double LandingGlassTop = 2.940;
	inline constexpr FSpan LandingPanes[] = {{24.0, 48.0}, {48.0, 136.0}, {136.0, 224.0}, {224.0, 312.0}, {312.0, 336.0}};
	/** The doorway, ±24° about the centre line. */
	inline constexpr double LandingDoorHalf = 24.0;
	/** The landing's leaves r 2.375 → 2.390, z 0.008 → 2.940, each closed from the centre line to 25°. */
	inline constexpr double LandingLeafIn = 2.375;
	inline constexpr double LandingLeafOut = 2.390;
	inline constexpr double LandingLeafBottom = 0.008;
	inline constexpr double LeafSpan = 25.0;
	/** The leaves' gilt leading fins, 12 wide (along the leaf) by 25 deep (across it). */
	inline constexpr double FinWidth = 0.012;
	inline constexpr double FinDepth = 0.025;
	/** The shoe r 2.360 → 2.490, z -0.010 → 0.070, open across the doorway; the head ring r 2.360 → 2.490, z 2.930 → 3.000. */
	inline constexpr double LandingRingIn = 2.360;
	inline constexpr double LandingRingOut = 2.490;
	inline constexpr double LandingShoeBottom = -0.010;
	inline constexpr double LandingShoeTop = 0.070;
	inline constexpr double HeadRingBottom = 2.930;
	inline constexpr double HeadRingTop = 3.000;
	/** The sill r 2.245 → 2.515, z -0.030 → 0.008, across the door ±26°. */
	inline constexpr double SillIn = 2.245;
	inline constexpr double SillOut = 2.515;
	inline constexpr double SillTop = 0.008;
	inline constexpr double SillHalf = 26.0;
	/** The tube: the landing's glass in two tiers, six panes a tier (the landing's divisions and one over the door). */
	inline constexpr double TubeTiers[][2] = {{3.000, 5.3375}, {5.3775, 7.685}};
	inline constexpr FSpan TubePanes[] = {{-24.0, 24.0}, {24.0, 48.0}, {48.0, 136.0}, {136.0, 224.0}, {224.0, 312.0}, {312.0, 336.0}};
	/** The tube's ring frame r 2.410 → 2.480 between the tiers. */
	inline constexpr double RingFrameIn = 2.410;
	inline constexpr double RingFrameOut = 2.480;
	/** The collar, lathed, with the pocket its iris stows in (r 2.465 → 4.050, z 7.660 → 7.705). */
	inline constexpr FStep Collar[] = {To(2.465, 7.455), To(2.800, 7.455), To(2.800, 7.545), To(3.620, 7.545), To(3.620, 7.455),
		To(4.000, 7.455), To(4.020, 7.465), Arc(4.020, 7.545, 0.080, -90.0, 0.0), To(4.100, 7.715), To(2.465, 7.715), To(2.465, 7.705),
		To(4.050, 7.705), To(4.050, 7.660), To(2.465, 7.660)};
	inline constexpr double CollarSoffit = 7.455;
	inline constexpr double CollarOuter = 4.100;
	inline constexpr double PocketIn = 2.465;
	inline constexpr double PocketOut = 4.050;
	inline constexpr double PocketBottom = 7.660;
	inline constexpr double PocketTop = 7.705;

	/**
	 * The collar iris (iris.4600): seven blades, each a 110° arc of the middle circle r 3.095, 1.250 wide with round ends,
	 * 2 mm thick at a 4 mm pitch; pins at r 3.745, plan angles 360°k/7 (pin 0 due east), blade k 4k mm below the top
	 * (7.695). Drawn shut: turned 52° on its pin from its open place, towards increasing plan angle.
	 */
	inline constexpr int32 IrisBlades = 7;
	inline constexpr double IrisMiddle = 3.095;
	inline constexpr double IrisBladeWidth = 1.250;
	inline constexpr double IrisArcDeg = 110.0;
	inline constexpr double IrisPin = 3.745;
	inline constexpr double IrisTop = 7.695;
	inline constexpr double IrisPitch = 0.004;
	inline constexpr double IrisBlade = 0.002;
	inline constexpr double IrisSwingDeg = 52.0;

	// ---- §3.6 the car (its frame: the floor datum on the axis, at the Atrium stop) --------------------------------------------
	inline constexpr double CarGlassIn = 2.185;
	inline constexpr double CarGlassOut = 2.200;
	inline constexpr double CarGlassBottom = 0.020;
	inline constexpr double CarGlassTop = 2.535;
	inline constexpr FSpan CarPanes[] = {{24.0, 92.0}, {92.0, 180.0}, {180.0, 268.0}, {268.0, 336.0}};
	inline constexpr double CarLeafIn = 2.135;
	inline constexpr double CarLeafOut = 2.150;
	inline constexpr double CarLeafBottom = 0.025;
	/** The base and top rings r 2.130 → 2.235. */
	inline constexpr double CarRingIn = 2.130;
	inline constexpr double CarRingOut = 2.235;
	inline constexpr double CarBaseRing[2] = {-0.070, 0.025};
	inline constexpr double CarTopRing[2] = {2.530, 2.600};
	/**
	 * The floor, a disc r ≤ 2.135, z -0.030 → 0.020; the roof, a clear disc inside the top ring with its underside at 2.575,
	 * its glass up to the crown's foot (2.583), which stands on it.
	 */
	inline constexpr double CarFloorRadius = 2.135;
	inline constexpr double CarFloor[2] = {-0.030, 0.020};
	inline constexpr double CarRoofSoffit = 2.575;
	/** The handrail: a glass rod Ø 36 on r 2.000 at 0.970, 30° → 330° from the door; brackets 12 × 12 at 60°k, r 2.000 → 2.196. */
	inline constexpr double HandrailRadius = 2.000;
	inline constexpr double HandrailHeight = 0.970;
	inline constexpr double HandrailDiameter = 0.036;
	inline constexpr double HandrailFrom = 30.0;
	inline constexpr double HandrailTo = 330.0;
	inline constexpr double HandrailBrackets[] = {60.0, 120.0, 180.0, 240.0, 300.0};
	inline constexpr double HandrailBracketOut = 2.196;
	inline constexpr double HandrailBracketSize = 0.012;
	/** The roller guides on both rings at bearings 0° and 180°: blocks r 2.229 → 2.290, ±0.045 tangential. */
	inline constexpr double GuideIn = 2.229;
	inline constexpr double GuideOut = 2.290;
	inline constexpr double GuideHalf = 0.045;
	/** The crown: a gilt drum r 0.340 from 2.583 to 2.780 on four arms at 45°, 135°, 225°, 315° (r 0.310 → 2.225, z 2.600 → 2.632). */
	inline constexpr double CrownRadius = 0.340;
	inline constexpr double CrownBottom = 2.583;
	inline constexpr double CrownTop = 2.780;
	inline constexpr double CrownArmBearings[] = {45.0, 135.0, 225.0, 315.0};
	inline constexpr double CrownArmIn = 0.310;
	inline constexpr double CrownArmOut = 2.225;
	inline constexpr double CrownArmBottom = 2.600;
	inline constexpr double CrownArmTop = 2.632;
	/** The sheet gives the arms' depth only; their width is the as-built 44 mm (windows ElanCar.cpp:171, half-width 0.022). */
	inline constexpr double CrownArmHalf = 0.022;
	/** The Cube mast's head on the crown's top: r 0.260, a groove r 0.195 from 0.070 to 0.110, the cap to 0.160. */
	inline constexpr FStep MastHead[] = {To(0.000, 0.000), To(0.260, 0.000), To(0.260, 0.070), To(0.195, 0.070), To(0.195, 0.110),
		To(0.260, 0.110), To(0.260, 0.160), To(0.000, 0.160)};
	/** The under-floor seat the Sphere's mast bears on: a drum r 0.200, z -0.160 → -0.030. */
	inline constexpr double SeatRadius = 0.200;
	inline constexpr double SeatBottom = -0.160;
	inline constexpr double SeatTop = -0.030;

	// ---- §2 anchors ---------------------------------------------------------------------------------------------------------
	/** The call: the waiting mark's centre. */
	inline constexpr double CallX = -3.310;
	/** The stele's Wall: its glass's west face at its north end, on the floor. */
	inline constexpr double SteleWestX = -3.840;
	inline constexpr double SteleNorthY = -2.650;
	/** The eleven bays A-K, centred on bearings 300° to 240° clockwise, each between its ribs' side faces on the base's inner face. */
	inline constexpr int32 Bays = 11;
	inline constexpr double FirstBayBearing = 300.0;

	// ---- §3.7 the stele ------------------------------------------------------------------------------------------------------
	inline constexpr double SteleGlass[2][3] = {{-3.840, -2.650, 0.060}, {-3.780, -1.150, 2.200}};
	inline constexpr double SteleFoot[2][3] = {{-4.010, -2.750, 0.000}, {-3.610, -1.050, 0.080}};
	/** The foot's top edges are eased 6 mm. */
	inline constexpr double SteleFootEase = 0.006;
}
