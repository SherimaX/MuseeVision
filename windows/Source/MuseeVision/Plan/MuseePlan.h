#pragma once

#include "CoreMinimal.h"

/**
 * Numbers from the plan, ported from the Swift build (Shared/Plan/MuseumPlan.swift and the wing
 * files). The plan is in metres with the Rotunda's centre at the origin, x east, plan y south and
 * height up. That frame is left-handed like Unreal's, so the mapping has no flips:
 *
 *   Unreal X = x × 100 (east), Unreal Y = plan y × 100 (south), Unreal Z = height × 100 (up).
 *
 * Swift (RealityKit) is y-up: its (x, y, z) is plan (x, height, plan y).
 */
namespace MuseePlan
{
	constexpr double Cm = 100.0;

	/** Plan (x east, y south, height) in metres → Unreal centimetres. */
	inline FVector At(double X, double PlanY, double Height = 0.0) { return FVector(X * Cm, PlanY * Cm, Height * Cm); }

	/** A RealityKit direction or position (x east, y up, z south) → Unreal axes, unscaled. */
	inline FVector FromRealityKit(double X, double Y, double Z) { return FVector(X, Z, Y); }

	constexpr double EyeHeight = 1.6;

	/** Every visit begins on the gilt sun at the centre of the Rotunda, facing the west door. */
	inline FVector SpawnFeet() { return At(0, 0, 0); }
	constexpr double SpawnYawDegrees = 180.0;

	namespace Rotunda
	{
		constexpr double Radius = 10;            // Ø 20 m drum
		constexpr double DrumHeight = 10;        // springing of the dome
		constexpr double OculusRadius = 2.5;     // Ø 5 m eye
		constexpr double LatticeHeight = 20.3;   // the steel-and-glass lattice and its bronze node
		constexpr double ClockRadius = 7;        // Ø 14 m sun clock
		constexpr double ShadowRadius = 6.78;    // where the node's point of shadow falls on the hour

		constexpr double Wall = 1.2;             // drum wall thickness
		constexpr int32 Pilasters = 16;          // at 11.25° + k·22.5°, framing the doors and niches
		constexpr double PilasterWidth = 0.9;
		constexpr double PilasterDepth = 0.45;
		constexpr double PilasterHeight = 9.47;  // base, shaft and capital, up to the entablature
		/** Doors (clear width in the drum wall): W and E 3 m, N and S 4 m, round-arched, springing at 4.5 m. */
		constexpr double DoorWidthWestEast = 3;
		constexpr double DoorWidthNorthSouth = 4;
		constexpr double DoorSpring = 4.5;
		/** Niches on the diagonals: 2 m wide, sill 2.0 m, springing 4.67 m, half-round in plan. */
		constexpr double NicheWidth = 2;
		constexpr double NicheSill = 2;
		constexpr double NicheSpring = 4.67;
		/** Five rings of 28 coffers, 0.45 m deep, between 4° and 64° above the springing. */
		constexpr int32 CofferRings = 5;
		constexpr int32 CoffersPerRing = 28;
		constexpr double CofferDepth = 0.45;
		constexpr double CofferFromDegrees = 4;
		constexpr double CofferToDegrees = 64;
		/** The bronze node in the lattice, whose point of shadow keeps the hour. */
		constexpr double NodeRadius = 0.12;
		constexpr double NodeHeight = 20.25;
		/** The passages out of the W and N doors run to Bay 1's end wall (x −13.4) and the court's south wall (y −13.5). */
		constexpr double WestPassageEnd = 13.4;
		constexpr double NorthPassageEnd = 13.5;
	}

	/**
	 * The Chinese Wing, 四時園 the Garden of the Four Seasons (Shared/Wings/ChineseWing.swift, ChinesePlan;
	 * plan/README.md; AChineseWingStructure). South of the Rotunda: a vestibule off its south door (whose
	 * passage the Rotunda stops at the drum's outer face), a moon gate Ø 3.6 m in the court's north wall,
	 * and a whitewashed cloister on three sides of an 11 m garden, under single-slope tile roofs that meet
	 * in valleys at the south corners and turn up where the west and east walks end at the north wall.
	 */
	namespace ChineseWing
	{
		constexpr double X0 = -10, X1 = 10, Y0 = 13.5, Y1 = 33.5;   // the court walls' inner faces
		constexpr double Wall = 0.6;                                 // outward: outer faces x ±10.6, y 12.9 and 34.1
		constexpr double WallHeight = 6;
		/** The vestibule, from the drum to the north wall's outer face: 4.8 m wide, a flat ceiling at 4.4 m. */
		constexpr double VestibuleHalfWidth = 2.4, VestibuleCeiling = 4.4;
		constexpr double MoonGateRadius = 1.8, MoonGateCentreHeight = 1.55;
		/** The sun clock (the Rotunda's floor, imported): a 128-gon of radius 10.3 m; the vestibule floor meets its edge. */
		constexpr double SunClockRadius = 10.3;
		constexpr int32 SunClockSides = 128;
		/** The colonnade: the west and east lines at x ±5.5, the south one at y 29; the eaves 0.6 m beyond. */
		constexpr double ColumnLineX = 5.5, ColumnLineY = 29.0;
		constexpr double EaveX = 4.9, EaveY = 28.4;
		constexpr double ColumnsAlongY[7] = {15.8, 18.0, 20.2, 22.4, 24.6, 26.8, 29.0};
		constexpr double ColumnsAlongX[6] = {-5.5, -3.3, -1.1, 1.1, 3.3, 5.5};
		/** The garden (gravel) between the colonnades, north to the moon terrace. */
		constexpr double GardenY0 = 18.0;
		/** The roofs' boarding, from the wall's inner face (distance d) to the eave (ChinesePlan.roofD, roofUnder). */
		constexpr double RoofProfileD[17] = {0, 0.32, 0.64, 0.96, 1.28, 1.60, 1.91, 2.23, 2.55, 2.87, 3.19, 3.51, 3.83, 4.15, 4.47, 4.78, 5.10};
		constexpr double RoofUnderside[17] = {5.04, 4.87, 4.69, 4.52, 4.36, 4.20, 4.06, 3.92, 3.80, 3.68, 3.58, 3.50, 3.43, 3.38, 3.36, 3.36, 3.40};
		/** The pond's edge (plan x, y); the water lies 0.3 m down, its edge 0.12 m in towards the centroid. */
		constexpr double Pond[28][2] = {{3.66, 20.90}, {3.23, 21.21}, {2.66, 21.53}, {2.47, 21.90}, {2.22, 22.22}, {1.56, 22.34},
			{0.76, 22.39}, {0.00, 22.48}, {-0.75, 22.50}, {-1.47, 22.31}, {-2.10, 22.05}, {-2.56, 21.83}, {-2.92, 21.57},
			{-3.26, 21.25}, {-3.47, 20.90}, {-3.50, 20.54}, {-3.30, 20.20}, {-2.74, 19.93}, {-2.03, 19.72}, {-1.35, 19.59},
			{-0.65, 19.52}, {0.05, 19.44}, {0.73, 19.41}, {1.39, 19.42}, {2.00, 19.53}, {2.53, 19.83}, {2.99, 20.20}, {3.45, 20.55}};
		constexpr double WaterDepth = 0.3, WaterInset = 0.12;
		constexpr double EdgingStones[14][2] = {{3.66, 20.90}, {2.66, 21.53}, {2.22, 22.22}, {0.76, 22.39}, {-0.75, 22.50},
			{-2.10, 22.05}, {-2.92, 21.57}, {-3.47, 20.90}, {-3.30, 20.20}, {-2.03, 19.72}, {-0.65, 19.52}, {0.73, 19.41},
			{2.00, 19.53}, {2.99, 20.20}};
		/** The five ceramics' plinths on the west walk (their tops carry the works at 0.9 m). */
		constexpr double CeramicsX = -8.2;
		constexpr double CeramicsY[5] = {18.50, 21.25, 24.00, 26.75, 29.50};
		constexpr double PlinthRadius = 0.45, PlinthTop = 0.9;
		/** The Thousand Li's table case on the east walk: the silk lies at 0.82 m on its bed at 0.8 m. */
		constexpr double HandscrollX0 = 9.0, HandscrollX1 = 9.9, HandscrollY0 = 16.8, HandscrollY1 = 30.2, HandscrollBed = 0.8;
		/** The Orchid Pavilion Preface's table case in the NW corner: its mount lies at 0.851 m. */
		constexpr double OrchidX0 = -9.3, OrchidX1 = -7.3, OrchidY0 = 13.9, OrchidY1 = 14.7, OrchidBed = 0.84;
		/** The hanging scrolls' cords start at the picture rail, 3.45 m up the south wall, 5 cm out from it. */
		constexpr double PictureRailHeight = 3.45;
		/** The bamboo beds against the north wall, and the two stone benches on the moon terrace. */
		constexpr double BambooBedX0 = 2.3, BambooBedX1 = 5.3, BambooBedY1 = 14.35;
		constexpr double BenchX0[2] = {-3.7, 1.9};
		constexpr double BenchLength = 1.8, BenchY0 = 17.05, BenchY1 = 17.5, BenchHeight = 0.45;
	}

	/**
	 * The sun clock that lifts (ASunClock, Source/MuseeVision/SunClock). The dial (r 0–7) is the top of
	 * a hollow marble drum: lowered, it is the Rotunda's floor; raised 3.5 m, it is a tempietto with a
	 * round-arched door to the west, and inside it a spiral stair goes down 6 m round an open well to a
	 * landing, which opens north into the room beyond. The fixed ring round it (r 7–10.3) carries the
	 * motto. Heights are from the Rotunda's floor (0); radii from its centre.
	 */
	namespace SunClock
	{
		constexpr double DialRadius = Rotunda::ClockRadius;   // 7: the fixed ring's inner edge, and the shaft's
		constexpr double DrumOuter = 6.99;                   // the drum's widest (cornice, plinth): 1 cm clear
		constexpr double DrumWallFace = 6.90;                // its wall face, between the pilasters
		constexpr double DrumWallInner = 6.545;
		constexpr double Lift = 3.5;                         // raised: the dial 3.5 m up, the door's sill on the floor
		constexpr double LiftSeconds = 8;
		constexpr double DoorWidth = 2.6;                    // west, at the wall face
		constexpr double DoorHeight = 2.95;
		constexpr double RingOuter = 10.3;                   // the fixed ring runs under the Rotunda's wall
		/** The shaft: the stair's wall face (r 6.48), the drum's pocket (r 6.54–7.0, down to −3.58) and the masonry to r 7.6. */
		constexpr double StairWall = 6.48;
		constexpr double PocketInner = 6.54;                 // 5 mm inside the drum's wall as it slides past
		constexpr double PocketFloor = -3.58;
		constexpr double ShaftOuter = 7.6;                   // the joint with the room beyond: that room starts here
		constexpr double Floor = -6.0;                       // the landing at the stair's foot
		constexpr double WellRadius = 2.2;                   // the open well (the stair's inner string's face)
		constexpr int32 Risers = 40;                         // 40 × 150 mm
		constexpr double Riser = 0.15;
		/**
		 * The port north (plan −y, centred on x 0) at the shaft's outer face (r 7.6): 3.6 m wide, 4.4 m high,
		 * round-arched (springing at −3.4, crown −1.6), sill on the landing (−6). Its jambs are the planes
		 * x = ±1.8 and its intrados the half-cylinder of radius 1.8 about the axis (x 0, z −3.4), so a passage
		 * beyond can continue them. Inside r 7.05 the drum's pocket passes over it: there the port is closed
		 * by a tympanum (r 7.05) over a lintel at −3.68, and the portal onto the stair is 3.6 × 2.32 m.
		 */
		constexpr double PortWidth = 3.6;
		constexpr double PortHeight = 4.4;
		constexpr double PortSpring = Floor + PortHeight - PortWidth / 2;   // −3.4
		constexpr double PortTympanum = 7.05;
		constexpr double PortLintel = -3.68;
	}

	/**
	 * Élan (Shared/Wings/Elan.swift, ElanPlan). The Atrium is a 28 m circle; the Sphere sits in
	 * it like a ball in a cup: its radius is the Atrium's, its equator on the top of the glass
	 * drum at 22 m, and its underside is the Atrium's ceiling.
	 */
	namespace Elan
	{
		constexpr double CentreX = 54, CentreY = 0;
		constexpr double Radius = 14;
		constexpr double Wall = 0.8;
		constexpr double BaseHeight = 6;
		constexpr double DoorWidth = 4, DoorHeight = 4.5;
		constexpr double CarRadius = 2.2, CarHeight = 2.6;
		constexpr double CarDoorHalfAngleDegrees = 24;
		constexpr double WaitRing = 3.0;
		constexpr double SquareFloor = -9;
		constexpr double SquareCeiling = -1.5;
		/**
		 * The Square (level −1, Elan.swift buildSquare): a 28 m square under the Atrium, 7.5 m clear,
		 * rammed-earth walls 1.2 m thick outside it. Partitions 0.6 m thick on the Lo Shu grid lines
		 * (±28/6 m from the centre) with 2.6 × 3.0 m doorways; 1.2 m columns at the crossings under
		 * 2.8 m capitals 0.7 m deep. The glass floor at 5 (R 4.2 m) over a round pit 7 m deep; the
		 * rim round it is floor to r 4.9 (inside the cross of pale stone). The ceiling's shaft
		 * opening is r 2.8, lined by a bronze ring from r 2.3 to 3.0; the ring of light at r 13.8–14.
		 */
		constexpr double SquareHalf = 14;
		constexpr double SquareWall = 1.2;
		constexpr double SquareGrid = SquareHalf / 3;                   // 4.667: the grid lines, from the centre
		constexpr double SquarePartition = 0.6;
		constexpr double SquareDoorWidth = 2.6, SquareDoorHeight = 3.0;
		constexpr double SquareColumn = 1.2;
		constexpr double SquareCapital = 2.8, SquareCapitalDepth = 0.7;
		constexpr double SquareGlassRadius = 4.2;
		constexpr double SquareRimRadius = 4.9;
		constexpr double SquarePitDepth = 7;
		constexpr double SquareShaftOpening = 2.8;
		constexpr double SquareRingIn = 2.3, SquareRingOut = 3.0;
		constexpr double SquareLightRingIn = 13.8, SquareLightRingOut = 14.0;
		constexpr double SquareHeight = SquareCeiling - SquareFloor;   // 7.5 clear
		constexpr double SphereRadius = Radius;
		constexpr double SphereCentreHeight = 22;                       // the equator, on the drum
		constexpr double SouthPole = SphereCentreHeight - SphereRadius; // 8
		constexpr double RingRadius = 3.0;                              // the opening over the car
		constexpr double TopFloor = SphereCentreHeight - EyeHeight;     // eyes at the centre
		constexpr double Speed = 1.5;                                   // m/s, no skip
		constexpr double Accel = 0.75;

		/** Height of the Sphere's underside (the Atrium's ceiling) at radius r from the axis. */
		inline double Underside(double R) { return SphereCentreHeight - FMath::Sqrt(SphereRadius * SphereRadius - R * R); }
		/** The rim of the opening at the south pole (≈ 8.33 m). */
		inline double RingHeight() { return Underside(RingRadius); }
		/** Where the car waits: just inside the Sphere, over the opening. */
		inline double Home() { return RingHeight() + 0.3; }

		inline FVector Centre(double Height = 0) { return At(CentreX, CentreY, Height); }
	}

	/**
	 * Élan's exterior (Source/MuseeVision/ElanExterior, AElanExteriorStructure): the tall domed hall
	 * seen from the Hall of Light and the courts. Radii from the Élan's axis (54, 0), heights from the
	 * ground. Every skin stands outside the interior's surfaces (AElanStructure: the drum's glass at
	 * r 14, the Sphere at R 14 about h 22; the imported stone base r 14–14.8 to h 6 with its coping).
	 */
	namespace ElanExterior
	{
		// The plinth (0–6 m): travertine ashlar over the imported base, a moulded base and a coping.
		constexpr double PlinthFace = 14.88;           // the ashlar face (the imported base's face: r 14.8)
		constexpr double PlinthSocle = 15.30;          // the base moulding's plain socle, h −0.1 … 0.4
		constexpr double PlinthCorona = 15.40;         // the coping's corona, h 5.48 … 5.86
		constexpr double PlinthTop = 6.20;             // the coping's wash where it meets the drum (6.05 at its lip)
		// The drum (6–22 m): its misty glass, a clear outer pane, and the order of fins and mullions.
		constexpr double DrumSkin = 14.03;             // the exterior's misty glass (the interior's at r 14)
		constexpr double DrumGlaze = 14.07;            // a clear pane over it (reflections)
		constexpr double OrderRoot = 14.05;            // where the fins, mullions and transoms start
		constexpr int32 Fins = 12;                     // on the interior ribs' bearings, 15° + 30°k from north
		constexpr double FinWidth = 0.34;
		constexpr double MullionWidth = 0.14;          // one in each bay, on its centre line
		constexpr double FirstTransom = 10.0, TransomStep = 4.0;   // transoms at h 10, 14, 18
		// The entablature at the equator (the Sphere's widest), stone, h 20.9 … 22.58, out to r 15.7.
		constexpr double CorniceBottom = 20.90, CorniceTop = 22.58, CorniceProjection = 15.70;
		// The dome (the Sphere's upper half): misty pearl skin, a clear pane, a lamella lattice of pearl steel.
		constexpr double DomeSkin = 14.20;             // about h 22 (the Sphere's own top: R 14)
		constexpr double DomeGlaze = 14.25;
		constexpr double LatticeInner = 14.29;         // the lattice's members stand 4 cm off the pane
		// The crown: a gilt compression ring round a lantern, a pearl cap and a gilt ball.
		constexpr double CrownTop = 36.60, LanternTop = 39.90, FinialTop = 40.84;
		/**
		 * The portal where the Hall of Light meets the plinth (world plan x): a stone frontispiece west of
		 * the drum, its opening the hall's section with room to spare (walls |y| ≤ 4.5 on kerbs to 4.8,
		 * vault R 5.3 about h 2.2); its east face on r 14.05 about the Élan's axis.
		 */
		constexpr double PortalFrontX = 39.0;
		constexpr double PortalHalfWidth = 6.05;
		constexpr double PortalClearHalfWidth = 4.95;
		constexpr double PortalClearRadius = 5.80;     // about h 2.2 (the hall's vault's centre): crown 8.0
		constexpr double PortalTop = 9.50;
	}

	/**
	 * The Hall of Light (Shared/Wings/HallOfLight.swift, HallOfLightPlan; plan/README.md). A glass
	 * hall 9 m wide from the Rotunda's drum (outer face r 11.2) to the Atrium's stone base (outer
	 * face r 14.8 about x 54): glass walls on a stone plinth to the eaves at 5 m, and a shallow vault
	 * of steel and glass (R 5.3 m about h 2.2, crown 7.5 m) springing from them. Posts every 3 m,
	 * arches every 1.5 m, eight panels across with a cross in each.
	 */
	namespace HallOfLight
	{
		constexpr double HalfWidth = 4.5;                  // the glass line
		constexpr double EavesHeight = 5.0;                // the walls' head and the vault's springing
		constexpr double PlinthHeight = 0.3;
		constexpr double VaultRadius = 5.3;
		constexpr double VaultCentreHeight = 2.2;          // crown 7.5 m
		constexpr double TransomHeight = 3.2;              // above the prints, so the garden shows over them
		constexpr double FirstPost = 11, PostStep = 3;     // posts at x 11 … 38
		constexpr int32 Posts = 10;
		constexpr double FirstArch = 12.5, ArchStep = 1.5; // arches at x 12.5 … 39.5 (at x 11 the drum is in the way)
		constexpr int32 Arches = 19;

		/** What the hall meets: the Rotunda's drum and sun clock, the Atrium's stone base, drum and floor. */
		constexpr double RotundaOuterRadius = Rotunda::Radius + Rotunda::Wall;   // 11.2
		constexpr double SunClockRadius = 10.3;            // the sun clock's 128-gon (Rotunda.swift)
		constexpr int32 SunClockSegments = 128;
		constexpr double AtriumOuterRadius = Elan::Radius + Elan::Wall;          // 14.8: the stone base, to 6 m
		constexpr double AtriumDrumRadius = Elan::Radius;                        // 14: the misty glass above it
		constexpr double AtriumFloorRadius = 14.3;         // the Atrium floor's 96-gon (Elan.swift)
		constexpr int32 AtriumFloorSegments = 96;
		constexpr double RotundaDoorHalfWidth = Rotunda::DoorWidthWestEast / 2;
		constexpr double AtriumDoorHalfWidth = Elan::DoorWidth / 2;

		/** The four stereo stones (x, plan y), off the axis; their readers face it at 1.32 m. */
		constexpr double StereoStones[4][2] = {{17, -2.4}, {23, 2.4}, {29, -2.4}, {35, 2.4}};
		constexpr double StereoStoneRadius = 0.35, StereoStoneHeight = 1.0, StereoReaderHeight = 1.32;
		/** Flush round viewing stones on the axis, between the stereo stones. */
		constexpr double ViewingStones[5] = {14, 20, 26, 32, 38};
		/** The prints' bays (x of each pair); from FirstAutochrome on, autochromes set into the glass. */
		constexpr double PrintBays[10] = {12.5, 15.5, 18.5, 21.5, 24.5, 27.5, 30.5, 33.5, 36.5, 38.95};
		constexpr int32 FirstAutochrome = 8;
		/** The prints' pale mounts (Swift): 1.0 × 0.9 m at h 1.15–2.05, their backs 0.12 m inside the glass. */
		constexpr double MountHalfWidth = 0.5, MountBottom = 1.15, MountTop = 2.05, MountBack = 4.38;
	}

	/**
	 * The Sculpture Hall (Shared/Wings/SculptureHall.swift, SculptureHallPlan): one top-lit court
	 * north of the Rotunda, 16.8 m square inside, walls 0.6 m thick, a laylight under a glass roof.
	 * The door from the Rotunda's north passage (which ends at the south wall's outer face,
	 * Rotunda::NorthPassageEnd) is on the axis: 4 m, round-arched, springing 4.5 m (crown 6.5 m).
	 */
	namespace SculptureHall
	{
		constexpr double West = -8.4, East = 8.4;        // the court's inner faces
		constexpr double North = -30.9, South = -14.1;
		constexpr double Wall = 0.6;                      // the outer faces: x ±9, y −31.5 and −13.5
		constexpr double CeilingHeight = 9.65;
		constexpr double DoorWidth = 4, DoorSpring = 4.5;
		/** The laylight's opening in the ceiling; its diffuser at LaylightHeight. */
		constexpr double LaylightWest = -7, LaylightEast = 7, LaylightNorth = -29.7, LaylightSouth = -15.3;
		constexpr double LaylightHeight = 9.97;
		/** The sun clock (the Rotunda's floor, imported): a 128-gon of radius 10.3 m; the passage's floor meets its edge. */
		constexpr double SunClockRadius = 10.3;
		constexpr int32 SunClockSides = 128;
	}

	/**
	 * The Atrium's level 0 (Shared/Wings/Elan.swift, buildAtrium; AAtriumBaseStructure): the floor,
	 * the stone base under the drum's glass, the waiting ring and mark, the bay letters, and the glass
	 * stele that carries the Starry Night. About Elan::Centre(); plan angles from east towards south.
	 */
	namespace Atrium
	{
		/** The export's floor: a 96-gon of radius 14.3 m. The Hall of Light's floor meets it in the west door. */
		constexpr double FloorRadius = 14.3;
		constexpr int32 FloorSegments = 96;
		/** The car's shaft through the floor: the floor stops 1 cm inside the Square's bronze lining (r 2.3). */
		constexpr double FloorInnerRadius = 2.29;
		/** The bronze ring where you wait for the car, and the mark west of it (a half ring and a dot). */
		constexpr double WaitRingIn = 2.94, WaitRingOut = 3.06;
		constexpr double WaitMarkX = 50.69, WaitMarkIn = 0.38, WaitMarkOut = 0.46;
		constexpr double WaitDotX = 50.35, WaitDotRadius = 0.14;
		/** The Starry Night's glass stele, north of the axis: its west face (the painting's side), y and size. */
		constexpr double SteleWestX = 50.16, SteleY = -1.90, SteleHalfWidth = 0.75, SteleTop = 2.2;   // 40 cm of glass over the frame
		/** Eleven bays for new art, A … K clockwise from bearing 300°: a bronze letter at the foot of each. */
		constexpr double BayLetterRadius = 13.1;
		constexpr double FirstBayBearing = 300, BayStepDegrees = 30;
		constexpr int32 BayLetters = 11;
	}

	/**
	 * The long stair from the Nymphéas oval's pond down to the Reserve (Shared/Wings/Reserve.swift,
	 * PondPlan and buildStair; AReserveStairStructure). 36 risers of 5.8/36 m and goings of 0.3389 m,
	 * running east from the first nosing (x −90) to the last riser (x −78.1385); the shaft 2.4 m wide
	 * (y ±1.2) from the pond's opening in the oval floor (x −90.4 … −82.9, soffit h −0.4 beyond it) to
	 * the Reserve's west wall (outer face x −74.6), the bottom landing reaching into the Reserve's floor
	 * notch (to x −73.7) at h −5.8.
	 */
	namespace LongStair
	{
		constexpr double OpeningX0 = -90.4, OpeningX1 = -82.9;
		constexpr double HalfWidth = 1.2;
		constexpr double FirstNosingX = -90.0;
		constexpr double Going = 0.3389;
		constexpr int32 Risers = 36;
		constexpr double Depth = 5.8;
		constexpr double Riser = Depth / Risers;
		constexpr double LastRiserX = FirstNosingX + Going * (Risers - 1);
		constexpr double ReserveWallOuterX = -74.6;
		constexpr double ShaftEnd = -74.0;
		constexpr double LandingEnd = -73.7;
		constexpr double SoffitHeight = -0.4;
	}

	/**
	 * The Reserve (AReserveStructure, AReserveRacks; Shared/Wings/Reserve.swift, ReservePlan, rebuilt as a grand
	 * brick undercroft): a nave 10 m between the pier lines and an aisle either side, 24 m wide inside, 60 m long,
	 * eight bays of 7.5 m. Floor at FloorZ; heights inside the Reserve (Spring, rises, racks) are above that floor.
	 * The long stair arrives through the arched west door on the axis (LongStair).
	 */
	namespace Reserve
	{
		constexpr double FloorZ = -5.8;
		constexpr double X0 = -74.0, X1 = -14.0;          // the end walls' inner faces
		constexpr double HalfWidth = 12.0;                 // the side walls' inner faces
		constexpr double Wall = 0.6;                       // outer faces x −74.6, −13.4, y ±12.6
		/** The box's top: 5 cm under the Salon's floor inside its outer faces (5 cm in from them), 0.3 m under the lawn beyond. */
		constexpr double SalonTopZ = -0.05, LawnTopZ = -0.33;
		constexpr double SalonTopX0 = -74.55, SalonTopX1 = -13.45, SalonTopHalf = 7.55;
		/** The piers: brick, 0.7 m square with chamfered arrises, on the two arcade lines, every 7.5 m. */
		constexpr double PierY = 5.0, PierHalf = 0.35;
		constexpr double FirstPierX = -66.5, BayPitch = 7.5;
		constexpr int32 PierLines = 7, Bays = 8;
		/** Every arch springs from the imposts' tops; the nave's vault rises higher than the aisles'. */
		constexpr double Spring = 4.0;
		constexpr double NaveRise = 1.55, AisleRise = 1.25;   // arch bands' crowns 5.55 and 5.25 m; the webs 6 cm over them

		inline double BayCentreX(int32 Bay) { return FirstPierX - BayPitch / 2 + BayPitch * Bay; }
		inline double PierX(int32 Line) { return FirstPierX + BayPitch * Line; }
		/** The aisles' centre lines (between the piers' faces and the walls). */
		constexpr double AisleCentreY = (PierY + PierHalf + HalfWidth) / 2;   // 8.675

		/**
		 * The sliding picture screens (Reserve; AReserveRacks: "racks" in the code, "screens" to the visitor), in the
		 * aisles, edge-on to the nave: three per bay (x bay centre −1.9, 0, +1.9), each an oak-framed panel faced in
		 * linen, 5.0 m long, its top 3.40 m up, top-hung on two trolleys from its own bronze track overhead (underside
		 * TrackZ0) and kept upright by a guide slot in the floor. At home it stands 0.35 m off the side wall; pulled out it
		 * glides 5.65 m along its track into the nave (to 1 m off the axis). Both aisles, bays 1–6 (the first six north
		 * screens are A–F, the lettered ones); in bays 7 and 8 stand the plan chests, round the easel at the east end.
		 */
		constexpr double RackLength = 5.0, RackHeight = 3.40;
		constexpr double ScreenBottom = 0.07;                  // the screen's underside (its guide fins run in the floor slot)
		constexpr double ScreenHalf = 0.05, PostHalf = 0.08;   // the panel's and its end posts' half thickness
		constexpr double TrackZ0 = 3.58, TrackZ1 = 3.70;       // the overhead track's underside and top
		constexpr double RackHomeCentre = HalfWidth - 0.35 - RackLength / 2;   // |y| 9.15: 6.65 … 11.65
		constexpr double RackGlide = RackHomeCentre - RackLength / 2 - 1.0;    // 5.65: out, |y| 1.0 … 6.0
		constexpr double RackOffsets[3] = {-1.9, 0.0, 1.9};
		constexpr int32 NorthRackBays = 6, SouthRackBays = 6;
		constexpr int32 RacksPerBay = 3;
		constexpr int32 RackCount = RacksPerBay * (NorthRackBays + SouthRackBays);   // 36
		constexpr int32 LetteredRacks = 6;                                         // A–F: the first six north racks
		/** The works hang on either face, centred 1.55 m up, their backs 6.5 cm off the screen's plane (2 cm off the linen), 35 cm apart (frames 9 cm). */
		constexpr double WorkHeight = 1.55, WorkOff = 0.065, WorkGap = 0.35, WorkFrame = 0.09;

		struct FRackSlot { double X; bool bNorth; int32 Index; };
		/** Rack K (0 … RackCount − 1): north racks west to east, then south racks. */
		inline FRackSlot Rack(int32 K)
		{
			const bool bNorth = K < RacksPerBay * NorthRackBays;
			const int32 Local = bNorth ? K : K - RacksPerBay * NorthRackBays;
			return {BayCentreX(Local / RacksPerBay) + RackOffsets[Local % RacksPerBay], bNorth, Local};
		}
		/** A rack's home centre (plan y): its aisle end towards the axis. */
		inline double RackHomeY(bool bNorth) { return bNorth ? -RackHomeCentre : RackHomeCentre; }

		/** The thirteen unhung works (the board SalonReserve): screens A–E (bEast: on the screen's east face), the Degas pastels in the plan chests. */
		struct FRackWork { const TCHAR* Letter; const TCHAR* Id; double W, H; bool bEast; };
		inline constexpr FRackWork RackWorks[11] = {
			{TEXT("A"), TEXT("caillebotte-pont-europe"), 1.80, 1.25, true}, {TEXT("A"), TEXT("caillebotte-young-man-window"), 0.81, 1.16, false},
			{TEXT("B"), TEXT("degas-place-de-la-concorde"), 1.18, 0.78, false}, {TEXT("B"), TEXT("degas-orchestra-opera"), 0.46, 0.57, true},
			{TEXT("C"), TEXT("renoir-cirque-fernando"), 0.99, 1.31, false}, {TEXT("C"), TEXT("renoir-two-sisters"), 0.81, 1.01, false},
			{TEXT("C"), TEXT("renoir-the-swing"), 0.73, 0.92, false},
			{TEXT("D"), TEXT("pissarro-red-roofs"), 0.66, 0.55, false}, {TEXT("D"), TEXT("pissarro-avenue-opera-snow"), 0.81, 0.65, true},
			{TEXT("E"), TEXT("sisley-bridge-villeneuve"), 0.65, 0.50, false}, {TEXT("E"), TEXT("sisley-snow-louveciennes"), 0.51, 0.61, true},
		};
		/** The rack pulled out when the visit begins (the rendering: the three Renoirs in the nave). */
		constexpr const TCHAR* InitialRack = TEXT("C");

		/**
		 * The viewing easel at the east end, on the axis (AReserveStructure's): an oak H-frame, its uprights' front faces at
		 * EaselFrontX, a ledge whose top is EaselLedge. A painting on it faces west, its back on the uprights, its frame on
		 * the ledge. The visit begins with Le Pont de l'Europe on it.
		 */
		constexpr double EaselFrontX = -15.05, EaselLedge = 0.80, EaselHalfSpan = 0.62;
		constexpr double EaselWorkX = EaselFrontX - 0.005;
		constexpr const TCHAR* EaselWork = TEXT("caillebotte-pont-europe");
		/** A work's centre height on the easel: its frame stands on the ledge. */
		inline double EaselWorkHeight(double H) { return EaselLedge + WorkFrame + H / 2; }
		/** The plan chests: an island of two chests back to back in each aisle's last two bays. */
		constexpr int32 ChestBays[2] = {6, 7};
		constexpr double ChestHalfX = 1.1, ChestDepth = 1.05, ChestTop = 0.90;
		/** The Degas pastels lie on the south islands' tops (The Tub west, The Star east). */
		constexpr const TCHAR* ChestWorks[2] = {TEXT("/Museum/Reserve/The_Reserve/Canvas_2"), TEXT("/Museum/Reserve/The_Reserve/Canvas")};
	}

	/**
	 * The classical hall under the Rotunda (AClassicalHallStructure): reached from the spiral stair's
	 * shaft (r 7.0, wall to r 7.6, floor at −6) through a round-arched opening facing north, 3.6 m wide
	 * and 4.4 m high. North of it a barrel-vaulted sculpture gallery 6 m wide and 24 m long (four bays of
	 * paired Ionic columns, statue niches, a coffered vault with four laylights) opens through a screen
	 * of two columns into a round tribune 13.2 m across: an ambulatory under a coffered annular vault
	 * round a ring of twelve columns that carries a coffered dome with a laylit eye over the centrepiece.
	 *
	 * Heights are above the floor (h), which is at FloorZ; nothing inside rises above CeilingLimitZ and
	 * the closed masonry box stops at RoofZ, under the ground floor's slabs.
	 */
	namespace ClassicalHall
	{
		constexpr double FloorZ = -6.0;
		constexpr double CeilingLimitZ = -0.62;          // the laylights' diffusers, the highest inner surfaces
		constexpr double RoofZ = -0.55;                  // the top of the masonry box
		constexpr double ShellBottomZ = -6.4;

		/** The joint with the stair's shaft: its outer face, and the opening in it (the shaft's builder makes the reveal). */
		constexpr double ShaftOuterRadius = 7.6;
		constexpr double OpeningHalfWidth = 1.8, OpeningSpring = 2.6, OpeningHeight = 4.4;
		/** The hall's passage starts 5 cm inside the shaft's wall, 1 cm inside the opening's faces (a 1 cm threshold step). */
		constexpr double LipRadius = 7.55;
		constexpr double PassageInset = 0.01;

		/** The gallery: x ±3, from its south wall to the screen at the tribune, four 6 m bays. */
		constexpr double GalleryHalfWidth = 3.0;
		constexpr double GallerySouthY = -8.2, GalleryNorthY = -32.2;
		constexpr int32 GalleryBays = 4;
		constexpr double BayLength = 6.0;

		/** The Ionic order: columns 0.34 m, 9 diameters to the abacus; the entablature's ledge and lip. */
		constexpr double ColumnDiameter = 0.34;
		constexpr double ColumnHeight = 3.06;
		constexpr double LedgeHeight = 3.70, CorniceLipHeight = 3.78;
		constexpr double ColumnFromWall = 0.40, PairSpacing = 1.0;

		/** The vaults' crowns (ribs), the coffers' depth and the laylights' diffusers, above the floor. */
		constexpr double VaultCrown = 5.15, CofferDepth = 0.18, LaylightHeight = CeilingLimitZ - FloorZ;

		/** The tribune: 0.5 m of wall north of the screen, then the round room. */
		constexpr double TribuneRadius = 6.6;
		constexpr double TribuneCentreY = GalleryNorthY - 0.5 - TribuneRadius;   // −39.3
		constexpr double RingRadius = 3.4;
		constexpr int32 RingColumns = 12;
		constexpr double OculusRadius = 1.2;

		inline FVector TribuneCentre(double Height = 0) { return At(0, TribuneCentreY, FloorZ + Height); }
	}

	/**
	 * The exterior dress of the stone wings (AMuseeFacadeStructure, Source/MuseeVision/Facade): a rusticated podium
	 * round the whole stone complex, an Ionic order (Vignola: column 9 D, entablature 2.25 D) on each volume, blind
	 * glazed windows, balustrades, and the portico on the Salon's south front. Everything stands outside the existing
	 * outer faces listed here (the wings' own numbers: SalonStructure.cpp, SculptureHallStructure.cpp, RotundaStructure.cpp,
	 * ChineseWingGeometry.cpp, HallOfLightStructure.cpp). Plan metres; heights from the ground floor (0).
	 */
	namespace Facade
	{
		// ---- The existing outer faces
		constexpr double SalonEast = -13.4, SalonWest = -74.6, SalonHalf = 7.6;   // the Salon's walls (0.6 m)
		constexpr double SalonRoofTop = 14.05;                                     // its roof slab (13.85–14.05)
		constexpr double CabinetEast = -15.6, CabinetWest = -24.4, CabinetNorth = -16.0, CabinetRoof = 6.3;
		constexpr double OvalX = -86.5, OvalA = 11.0, OvalB = 7.5, OvalWall = 0.6, OvalRoof = 7.2;   // inner semi-axes; outer = offset by the wall
		constexpr double HyphenHalf = 5.5;             // the new wall closing the gap between the Salon and the oval, |y| 5.5
		constexpr double WestPassageHalf = 1.8, WestPassageTop = 6.3;     // the Rotunda's passage to the Salon (its shell)
		constexpr double NorthPassageHalf = 2.3, NorthPassageTop = 6.8;   // … and to the Sculpture Hall
		constexpr double VestibuleHalf = 3.65, VestibuleTop = 7.5;        // Albion's porch on the south door (its walls, its lead flat)
		constexpr double CourtNorthFace = 12.9;                           // the Chinese court's north wall, outer face
		constexpr double DrumRadius = 11.2, DrumTop = 11.0;               // the Rotunda's outer face; the dome's shell starts at 11 m
		constexpr double DomeShellRadius = 10.6, DomeShellCentre = 10.0, DomeCurbTop = 20.45;
		constexpr double DomeCoverClearance = 0.5;     // the stone cover over it (bCoverRotundaDome): R 11.1
		constexpr double HallKerb = 4.8;                                  // the Hall of Light's planter kerbs, |y|
		constexpr double SculptureWest = -9.0, SculptureEast = 9.0, SculptureNorth = -31.5, SculptureSouth = -13.5;

		// ---- The dress
		constexpr double Podium = 2.0;                 // the rusticated podium's top: every order stands on it
		constexpr double Skin = 0.20;                  // the ashlar skin's thickness over the existing faces
		constexpr double WindowBack = 0.06;            // the blind windows' bronze backing, out from the existing face
		/** Each volume's order: the lower diameter D and the entablature's top (base 2.0 + 11.25 D). */
		constexpr double SalonD = 1.10;                // entablature 11.9 … 14.375, over the roof slab
		constexpr double SculptureD = 0.787;           // 9.08 … 10.85, over the coping (10.56)
		constexpr double DrumD = 0.827;                // 9.44 … 11.3, the attic to 12.15 over it
		constexpr double OvalD = 0.484;                // 6.36 … 7.45, over the oval's roof (7.2)
		constexpr double CabinetD = 0.40;              // 5.6 … 6.5, over the cabinet's roof (6.3)
		constexpr double DrumAtticTop = 12.15;
		inline double EntablatureTop(double D) { return Podium + 11.25 * D; }
		inline double EntablatureBottom(double D) { return Podium + 9.0 * D; }

		/** The portico on the Salon's south front: six Ionic columns (the Salon's order) on the podium, a pediment, steps. */
		constexpr double PorticoAxisX = -44.0;
		constexpr double PorticoColumnY = 12.0;        // the columns' centres
		constexpr double PorticoPitch = 4.0;           // six columns, x −54 … −34 (the pilasters' 4 m rhythm)
		constexpr double PorticoFront = 12.9;          // the stylobate's front, where the steps begin
		constexpr double PorticoStepGoing = 0.40;      // 11 risers of 2.0 / 11 m
		constexpr int32 PorticoRisers = 11;
		constexpr double PorticoStepsEnd = PorticoFront + PorticoRisers * PorticoStepGoing;   // 17.3
		constexpr double PorticoHalf = 11.6;           // the steps and stylobate, x −55.6 … −32.4
		constexpr double PorticoCheek = 1.2;           // the cheek walls either side of the steps
		constexpr double PedimentPitchDegrees = 13.5;
	}

	/**
	 * The grounds (AMuseeLandscape, Source/MuseeVision/Facade): the forecourt south of the portico with its canal, the
	 * parterre, the avenue, the walk east to the Élan and its terrace; the trees are placed by Scripts/nature.py
	 * (place_grounds) from AMuseeLandscape::GetTreePlacements.
	 */
	namespace Grounds
	{
		constexpr double AxisX = Facade::PorticoAxisX;
		/** The parvis in front of the steps (stone), and the gravel cour round the canal. */
		constexpr double ParvisY0 = Facade::PorticoStepsEnd, ParvisY1 = 23.0, ParvisHalf = 16.0;
		/** The canal: a still basin on the axis, its coping 0.45 m high, the water 0.15 m under it. */
		constexpr double CanalHalf = 4.5, CanalY0 = 27.0, CanalY1 = 57.0;
		constexpr double CopingWidth = 0.5, CopingTop = 0.45, WaterLevel = 0.30, BasinFloor = -0.25;
		/** The parterre: lawn panels either side of the canal edged with box, the allée walks outside them. */
		constexpr double PanelInner = 7.5, PanelOuter = 14.0;          // |x − axis|
		constexpr double WalkInner = 16.0, WalkOuter = 20.0;           // the allées' walks
		constexpr double CourY1 = 62.0;                                // the cross walk at the canal's far end: 58 … 62
		/** The avenue south along the axis; the walk east to the Élan (y 40 … 44) and the terrace round it. */
		constexpr double AvenueHalf = 4.0, AvenueY1 = 130.0;
		constexpr double EastWalkY0 = 40.0, EastWalkY1 = 44.0;
		constexpr double EastWalkX1 = Elan::CentreX;                   // … to the rond-point on the Élan's meridian
		constexpr double RondPointY = 42.0, RondPointRadius = 7.0, RondBasinRadius = 3.0;
		/** The terrace round the Élan's plinth (its socle's face at r 15.3), from the north-west round by the east to the south-west. */
		constexpr double ElanTerraceIn = 15.32, ElanTerraceOut = 19.4, ElanTerraceFromDegrees = -125.0, ElanTerraceToDegrees = 125.0;
	}
}
