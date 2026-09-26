#pragma once

#include "CoreMinimal.h"
#include "Plan/MuseePlan.h"

/**
 * The Élan Cube (plan/boards/ElanCube, canvas/ElanCube.dc.html; plan/README.md "Élan, level −1"): the Sphere's twin,
 * mirrored in the Atrium floor. It replaces the Square (Source/MuseeVision/Square, kept but no longer placed).
 *
 * Metres, in the Atrium's frame: x east and y south from the Élan's axis (plan 54, 0), z the height above the Atrium
 * floor (so the Cube's centre is at z −22, as the Sphere's is at +22). The section on the board, read from its SVG
 * (9 px to the metre): the panel faces 28 m each way, floor −36, ceiling −8; a 1.2 m service void behind them; a
 * concrete box 1 m thick outside that (its roof's top at −5.8); a glass shaft 4.9 m across down through soil
 * (to −1.5), clay (to −4) and chalk; an iris in the ceiling; a gilt mast hung from it; the car's eyes stopping at
 * the exact centre.
 */
namespace CubePlan
{
	namespace E = MuseePlan::Elan;
	constexpr double Cm = MuseePlan::Cm;

	// The room: six faces of light-field panels, 0.7 m, 40 by 40 to a face (9,600 in all).
	constexpr double Half = 14.0;
	constexpr double Ceiling = -8.0;
	constexpr double Floor = -36.0;
	constexpr double Centre = -22.0;
	constexpr double Panel = 0.7;
	constexpr int32 PanelsPerEdge = 40;

	// The service void and the concrete box.
	constexpr double Void = 1.2;
	constexpr double Wall = 1.0;
	constexpr double BoxIn = Half + Void;              // 15.2: the box's inner faces
	constexpr double BoxOut = BoxIn + Wall;            // 16.2
	constexpr double RoofSoffit = Ceiling + Void;      // −6.8
	constexpr double RoofTop = RoofSoffit + Wall;      // −5.8
	constexpr double BaseTop = Floor - Void;           // −37.2
	constexpr double BaseBottom = BaseTop - Wall;      // −38.2

	// The car stops with the visitor's eyes at the exact centre; it never lands.
	constexpr double CarStop = Centre - MuseePlan::EyeHeight;   // −23.6

	// The shaft: the Atrium floor's structure, then the glass through the ground, then a dark bronze sleeve through
	// the box's roof and the void to the iris.
	constexpr double SlabBottom = -0.5;                // the Atrium floor: marble, screed and its concrete slab
	constexpr double LiningIn = 2.30, LiningOut = 2.56; // the bronze lining through the slab (the Atrium floor's lip ends at 2.29)
	constexpr double GlassIn = 2.45, GlassOut = 2.51;  // 60 mm laminated low-iron glass
	constexpr double EarthR = 2.66;                    // the cut face of the ground, 15 cm behind the glass (a lit cavity)
	constexpr double SoilBottom = -1.5;                // topsoil and subsoil
	constexpr double ClayBottom = -4.0;                // clay-with-flints
	constexpr double GlassTop = SlabBottom, GlassBottom = RoofTop;   // the glass: −0.5 … −5.8, in four tiers
	constexpr int32 GlassTiers = 4;
	constexpr int32 GlassPanesRound = 4;               // each tier in four curved panes, joints on the diagonals
	constexpr double FrameIn = 2.41, FrameOut = GlassIn;   // the bronze ring frames between tiers (the glass sits outside them)
	constexpr double FrameHalfHeight = 0.03;
	constexpr double SleeveIn = GlassIn, SleeveOut = 2.75;

	// The iris in the ceiling (the Élan kit's iris: seven blades, open into a ring round the car's way). A gilt
	// soffit ring hides the open blades; the blades lie just over it.
	constexpr double SoffitIn = 2.46, SoffitOut = 4.12;
	constexpr double SoffitThick = 0.03;
	constexpr double IrisTop = Ceiling + SoffitThick + 0.028;   // the top blade's face (the stack runs 26 mm down from it)
	constexpr double HousingOut = 4.30, HousingTop = -7.60;
	/** The iris closed round the mast's head: the blades leave a gap of r 0.197 m (Scripts: iris.py, the exact union). */
	constexpr float IrisClampOpen = 0.378f;

	// The guide rails in the shaft: north and south, T-section, from under the lining to the iris.
	constexpr double RailTip = 2.265, RailBlade = 0.016, RailFootIn = 2.385, RailFootOut = GlassIn - 0.005, RailFootWidth = 0.14;
	constexpr double RailTop = SlabBottom - 0.02, RailBottom = IrisTop + 0.03;

	// The mast: a spiral-band column (a Spiralift) that the car's crown feeds out, clamped at its head by the iris.
	constexpr double MastR = 0.17;
	constexpr double HeadR = 0.26, GrooveR = 0.195;

	// The ammonite in the chalk, and the flint bands.
	constexpr double AmmoniteZ = -4.95;
	constexpr double AmmoniteBearing = 58.0;           // degrees clockwise from north
	constexpr double AmmoniteDiameter = 0.36;

	/** Plan angle (from east, towards south) of a compass bearing (clockwise from north). */
	inline double PlanAngle(double BearingDeg) { return FMath::DegreesToRadians(BearingDeg - 90.0); }
	/** The Cube's centre in the world (cm). */
	inline FVector WorldCentre() { return E::Centre(Centre); }
}
