#pragma once

#include "CoreMinimal.h"

/**
 * The Sphere's plan: every number the stage is built to, in metres in the stage's frame (the Atrium's: origin on the Élan
 * axis at the museum's floor datum; x east, y south, z up), taken from its design sheet, Docs/Stages/sphere.md (the
 * section is given with each group). The sheet's own sources are the as-built Élan (windows/, D-017), the Atrium's sheet
 * at the seam and the coordinator's rulings. Angles are compass bearings, clockwise from north, as the sheet gives them.
 * Finishes, light and motion are the schedules' (Data/Stages/sphere), never here.
 */
namespace Musee::Sphere::Plan
{
	/** The stage's key (§1). */
	inline constexpr const TCHAR* Stage = TEXT("sphere");

	// ---- §1 the centre, the footprint and the vertical extent -------------------------------------------------------------
	/** The Sphere's centre C is on the axis at this height: the equator on the Atrium's drum. */
	inline constexpr double CentreZ = 22.000;
	/** The plan outline: a 96-gon of this circumradius, first vertex at bearing 0 (inscribed 14.318, the drip ring's edge). */
	inline constexpr int32 FootprintSides = 96;
	inline constexpr double FootprintCircumradius = 14.326;
	/** The mating plane (sphere.down, the throat's foot on the Atrium's collar) and the gilt ball's top. */
	inline constexpr double PortZ = 7.715;
	inline constexpr double FootprintTop = 37.440;
	/**
	 * The volume turned about the axis (§1, volume; the seams): the neck column r 3.000 from the plane up to the seam
	 * sphere R 14.000 about C, which the Sphere keeps inside of (drawn inside it, its corners on it, to the museum's chord
	 * tolerance of 1 mm, MuseeFormMaking's Stations.h); out to the drum's head r 14.080 at the equator and up to the wash;
	 * down the wash to the drip ring's edge r 14.320 and up it to 22.656; the dome's envelope R 14.335 about C, drawn
	 * outside it, up to the boss r 0.600 at its crossing; and the top 37.450.
	 */
	inline constexpr double NeckColumn = 3.000;
	inline constexpr double SeamRadius = 14.000;
	inline constexpr double SeamToleranceMeters = 0.001;
	inline constexpr double DripEdge = 14.320;
	inline constexpr double DripTop = 22.656;
	inline constexpr double EnvelopeRadius = 14.335;
	inline constexpr double EnvelopeCrown = 0.600;
	inline constexpr double VolumeTop = 37.450;

	// ---- §2 the port and the well it receives (class iris.4600, atrium.md §2) ---------------------------------------------
	inline constexpr const TCHAR* PortClass = TEXT("iris.4600");
	inline constexpr const TCHAR* PortMates = TEXT("atrium.up");
	/** The class's well: the circle of this radius (less the rails' notches), from the plane up to the Atrium's well top. */
	inline constexpr double WellRadius = 2.300;
	inline constexpr double WellTop = 23.390;
	/** The port's depth: the neck, from the plane to the flange's top. */
	inline constexpr double NeckTop = 11.400;

	// ---- §3.1 the steel plate shell and §3.3 the image surface: spheres about C, open round the neck ----------------------
	inline constexpr double PlateInner = 13.940;
	inline constexpr double PlateOuter = 13.950;
	/** The image surface R 13.700 and the light-field panels' depth behind it, 60 mm (one continuous surface tonight). */
	inline constexpr double ImageInner = 13.700;
	inline constexpr double ImageOuter = 13.760;

	// ---- §3.2 the exterior dome: skin, drip ring, boss and ball (as built bPearlShell) ------------------------------------
	inline constexpr double SkinInner = 14.240;
	inline constexpr double SkinOuter = 14.300;
	/** The skin's foot, on the drip ring, and the crown hole the boss's spigot passes. */
	inline constexpr double SkinFoot = 22.625;
	inline constexpr double CrownHole = 0.490;
	/** The Atrium's entablature wash, which the ring is bedded on: its inner edge, and its fall outwards (atrium.md §3). */
	inline constexpr double WashInnerR = 14.080;
	inline constexpr double WashInnerZ = 22.580;
	inline constexpr double WashFallPerMeter = 0.0571;
	/** The drip ring: its flat to this radius, then a 50 mm half-round about (RingFlatOuter, RingRoundZ). */
	inline constexpr double RingFlatOuter = 14.2926;
	inline constexpr double RingRoundRadius = 0.025;
	inline constexpr double RingRoundZ = 22.600;
	/** The boss: its spigot through the crown hole, its rim with a 40 mm quarter-round, the cone to the ball's seat. */
	inline constexpr double BossBottom = 36.230;
	inline constexpr double SpigotRadius = 0.485;
	inline constexpr double BossRim = 0.550;
	inline constexpr double BossRound = 0.040;
	inline constexpr double BossShoulderR = 0.300;
	inline constexpr double BossShoulderZ = 36.420;
	inline constexpr double BossSeatR = 0.200;
	/** The gilt ball, in the boss's seat. */
	inline constexpr double BallRadius = 0.450;
	inline constexpr double BallCentreZ = 36.990;

	// ---- §3.4 the neck: throat, courses, ribs ----------------------------------------------------------------------------
	/** The throat, a welded double skin, from the plane to the mouth (the flange's underside, the mast's foot plane). */
	inline constexpr double ThroatBore = 2.550;
	inline constexpr double ThroatOuter = 2.980;
	inline constexpr double Mouth = 11.150;
	/** Six gilt courses on the bore, 24 mm tall at these centres (eight of 0.42935 from 7.7152, less the one by the light). */
	inline constexpr double CourseInner = 2.532;
	inline constexpr double CourseHeight = 0.024;
	inline constexpr double CourseCentres[] = {8.1446, 8.5739, 9.0033, 9.8620, 10.2913, 10.7207};
	/** Everything on the bore stops this short of what it meets (the rail brackets, the light channel). */
	inline constexpr double StopClear = 0.010;
	/** Sixteen gilt ribs on the bore, stopped short of the light channel (9.550 … 9.650). */
	inline constexpr int32 Ribs = 16;
	inline constexpr double RibFirstBearing = 11.25;
	inline constexpr double RibInner = 2.526;
	inline constexpr double RibWidth = 0.022;
	inline constexpr double RibBreakBottom = 9.540;
	inline constexpr double RibBreakTop = 9.660;

	// ---- §3.4 the flange (the mouth and the neck iris's seat) and the iris pockets -----------------------------------------
	/** The seats' bore: a 96-gon of this circumradius, its sides centred on bearings 3.75°k (flange, platform deck, seat ring). */
	inline constexpr int32 SeatSides = 96;
	inline constexpr double SeatCircumradius = 2.460;
	inline constexpr double FlangeOuter = 4.500;
	/** The neck iris's pocket, open to the bore under the flange's 3 mm top plate. */
	inline constexpr double NeckPocketBottom = 11.369;
	inline constexpr double NeckPocketTop = 11.397;
	inline constexpr double PocketOuter = 4.100;
	/** The pins the blades turn on, through each pocket. */
	inline constexpr double PinDiameter = 0.060;

	// ---- §2, §3.4, §3.5 the class's iris (iris.4600, atrium.md §2): the neck iris and the platform iris --------------------
	/** Seven blades, each a 110° arc of the middle circle, 1.250 wide with round ends, 2 mm thick at a 4 mm pitch. */
	inline constexpr int32 IrisBlades = 7;
	inline constexpr double IrisMiddle = 3.095;
	inline constexpr double IrisWidth = 1.250;
	inline constexpr double IrisArcDeg = 110.0;
	inline constexpr double IrisBlade = 0.002;
	inline constexpr double IrisPitch = 0.004;
	/** Pins at this radius on plan angles 360°k/7 (pin 0 due east); each blade turns this far on its pin, shut to open. */
	inline constexpr double IrisPin = 3.745;
	inline constexpr double IrisSwingDeg = 52.0;
	/** The top blade's top face of each iris. */
	inline constexpr double NeckIrisTop = 11.396;
	inline constexpr double PlatformIrisTop = 20.396;

	// ---- §3.4 the rails in the neck (the Atrium's section) and their brackets ----------------------------------------------
	inline constexpr double RailBearings[] = {0.0, 180.0};
	inline constexpr double RailBladeIn = 2.265;
	inline constexpr double RailFootIn = 2.385;
	inline constexpr double RailFootOut = 2.425;
	inline constexpr double RailBladeHalf = 0.008;
	inline constexpr double RailFootHalf = 0.070;
	inline constexpr double RailBottom = 7.745;
	inline constexpr double RailTop = 11.340;
	/** Brackets 180 wide × 100 tall: four to the throat's bore and one to the flange's bore. */
	inline constexpr double BracketHalf = 0.090;
	inline constexpr double BracketHeight = 0.100;
	inline constexpr double ThroatBracketCentres[] = {7.900, 8.900, 9.900, 10.900};
	inline constexpr double FlangeBracketCentre = 11.250;

	// ---- §3.5 the platform ----------------------------------------------------------------------------------------------
	inline constexpr double PlatformTop = 20.420;
	/** The deck, a steel box from the seats' bore: its soffit, its top under the stone and under the pocket, its outer face. */
	inline constexpr double DeckSoffit = 20.170;
	inline constexpr double DeckTop = 20.390;
	inline constexpr double PlatformPocketBottom = 20.369;
	inline constexpr double PlatformPocketTop = 20.397;
	inline constexpr double DeckOuter = 5.498;
	/** The stone ring over the deck, from the pocket's outer wall: 24 slabs of 15°, 1 mm joints at bearings 7.5° + 15°k. */
	inline constexpr int32 StoneSlabs = 24;
	inline constexpr double StoneFirstJoint = 7.5;
	inline constexpr double StoneJoint = 0.001;
	/** The seat ring, the pocket's lid: 16 cast segments, joints at bearings 11.25° + 22.5°k. */
	inline constexpr int32 SeatSegments = 16;
	inline constexpr double SeatFirstJoint = 11.25;
	/** The door sill across ±26° of the car's door (west), from this radius to the seats' bore. */
	inline constexpr double SillInner = 2.245;
	inline constexpr double SillFrom = 244.0;
	inline constexpr double SillTo = 296.0;
	/** The nosing, the balustrade's shoe, its 32 posts, 32 curved panes and the handrail on r 5.320. */
	inline constexpr double NosingOuter = 5.512;
	inline constexpr double NosingBottom = 20.360;
	inline constexpr double NosingTop = 20.423;
	inline constexpr double ShoeInner = 5.290;
	inline constexpr double ShoeOuter = 5.350;
	inline constexpr double ShoeTop = 20.480;
	inline constexpr double BalustradeRadius = 5.320;
	inline constexpr int32 Posts = 32;
	inline constexpr double PostFirstBearing = 5.625;
	inline constexpr double PostSide = 0.025;
	inline constexpr double PostTop = 21.442;
	inline constexpr double GlassInner = 5.3136;
	inline constexpr double GlassOuter = 5.3264;
	inline constexpr double GlassTop = 21.440;
	inline constexpr double PaneDeg = 10.873;
	inline constexpr double HandrailDiameter = 0.056;
	inline constexpr double HandrailZ = 21.470;
	/** The handrail light's gilt channel, inside the post line (Light's strip lies in it). */
	inline constexpr double ChannelInner = 5.270;
	inline constexpr double ChannelOuter = 5.292;
	inline constexpr double ChannelBottom = 21.425;
	inline constexpr double ChannelTop = 21.440;
	/** Eight struts, tube 168 × 10, from the flange's rim to under the deck, with end plates 300 across, 20 thick. */
	inline constexpr int32 Struts = 8;
	inline constexpr double StrutFirstBearing = 22.5;
	inline constexpr double StrutOuterDiameter = 0.168;
	inline constexpr double StrutWall = 0.010;
	inline constexpr double StrutFootR = 4.300;
	inline constexpr double StrutHeadR = 5.300;
	inline constexpr double StrutPlateDiameter = 0.300;
	inline constexpr double StrutPlate = 0.020;

	// ---- §3.6 the mast's footing: the foot ring and the four bearing dogs in their housings -------------------------------
	/** The foot ring round the column's base, drawn at the car's stop mast, 2 mm over the dogs. */
	inline constexpr double FootRingInner = 0.205;
	inline constexpr double FootRingOuter = 0.400;
	inline constexpr double FootRingBottom = 11.152;
	inline constexpr double FootRingTop = 11.210;
	/** Four CuSn10 dogs, 0.160 wide × 0.300 deep × 3.060 long, top face the foot plane, drawn retracted (tips at r 2.560). */
	inline constexpr double DogBearings[] = {45.0, 135.0, 225.0, 315.0};
	inline constexpr double DogWidth = 0.160;
	inline constexpr double DogDepth = 0.300;
	inline constexpr double DogLength = 3.060;
	inline constexpr double DogRetracted = 2.560;
	inline constexpr double DogExtended = 0.300;
	/**
	 * The dogs' housings: steel boxes 0.180 wide inside, from the bore through the throat and out under the flange,
	 * closed at the outer end; their floor 10.830 … 10.850 under the dog, their roof the flange's underside (and beyond
	 * the flange their own top plate). The sheet gives the floor's plate; the sides, end and top are the same plate.
	 */
	inline constexpr double HousingInside = 0.180;
	inline constexpr double HousingBottom = 10.830;
	inline constexpr double HousingOuterEnd = 5.650;
	inline constexpr double HousingPlate = 0.020;

	// ---- §2 anchors ------------------------------------------------------------------------------------------------------
	/** The platform way's centre, where the Sphere's use of the car stands. */
	inline constexpr double CallZ = PlatformTop;
}
