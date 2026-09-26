#pragma once

#include "CoreMinimal.h"
#include "Plan/MuseePlan.h"

/**
 * Albion, England 1848–1898, on the Rotunda's south door (plan/proposals/albion: the boards Main, Plan, Sections, Hang,
 * Details and Outside, and their .dc.html sources, whose SVG carries the dimensions used here). A court of iron and
 * glass on ten columns of polished British stone, after the Oxford University Museum of Natural History.
 *
 * Plan metres, the Rotunda's centre the origin, x east, plan y south, height up (MuseePlan). Heights from the court's
 * floor, which is the Rotunda's (0).
 *
 * - The court: 28.0 × 36.0 m inside (x ±14, y 17.2 … 53.2), walls 0.6 m. A nave of 12 m (x ±6) between aisles of 8,
 *   on column lines 1 … 7 every 6 m (y 17.2 … 53.2). Ten polished stone columns (lines 2 … 6, x ±6), half-columns
 *   against the end walls on lines 1 and 7.
 * - The columns: a moulded base 0.35 m, a polished shaft Ø 0.5 m to 5.5 m, a carved capital 5.5 … 6.2 m; above, a
 *   clustered iron shaft to 9.7 m under the girder that carries the valley gutter at 10.0 m. Pointed iron arches
 *   (springing 6.2 m) between the capitals, wrought-iron rings in their spandrels.
 * - Three pointed glass vaults on iron ribs, springing at 10.0 m: the nave's crown 17.0 m (radius 7.083 m, its centres
 *   1.083 m either side of the axis), the aisles' 15.2 m (radius 5.38 m). The glass lies on these arcs; the ribs hang
 *   below it (main ribs on the column lines, lighter ones every 1.5 m, purlins, glazing bars every 0.75 m).
 * - The walls: buff stone banded with red (a band 0.28 m every 1.2 m from 0.9 m), a slate skirting 0.2 m; pilasters
 *   on the column lines of the aisle walls, from which the aisle ribs spring. The side walls stop at 10.0 m inside; a
 *   parapet (to 10.9 m) and its gutter outside the glass. Six lancets a side, 1.4 × 4.0 m (sill 5.0, head 9.0 m),
 *   on the bays' centre lines.
 * - The ends: stone to 8.5 m in the nave, to 10.0 m in the aisles; above, iron-and-glass screens. The south screen
 *   carries the Tristram and Isolde band (twelve panels 1.6 × 1.7 m: 9.0 … 10.7 and 10.9 … 12.6 m, x ±4.8).
 * - The porch: 6.0 m wide inside (x ±3), from the drum to the court, walls 0.6 m (outer faces x ±3.6); the court door
 *   4.0 m, pointed (springing 3.3, crown 5.5 m).
 * - Floors: the nave in encaustic tiles laid on the diagonal; the aisles in York stone flags; the porch in encaustic
 *   tiles inside a black border.
 */
namespace AlbionPlan
{
	using MuseePlan::At;

	// ---------------------------------------------------------------- The court
	constexpr double X0 = -14.0, X1 = 14.0;            // the side walls' inner faces
	constexpr double Y0 = 17.2, Y1 = 53.2;             // the end walls' inner faces (north, south)
	constexpr double Wall = 0.6;                       // outer faces x ±14.6, y 16.6 and 53.8
	constexpr double NaveHalf = 6.0;                   // the arcade lines
	constexpr double Bay = 6.0;
	constexpr int32 Lines = 7;                         // column lines 1 … 7, y 17.2 … 53.2
	inline double LineY(int32 Line) { return Y0 + Bay * (Line - 1); }   // Line 1 … 7
	inline double BayCentreY(int32 BayIndex) { return Y0 + Bay * (BayIndex + 0.5); }   // bays 0 … 5

	// ---------------------------------------------------------------- Heights
	constexpr double Skirting = 0.20;                  // slate
	constexpr double FirstBand = 0.90, BandHeight = 0.30, BandPitch = 1.20;   // (a band is one 0.3 m course; three buff courses between)
	constexpr double Spring = 10.0;                    // the vaults' springing; the side walls' inner tops
	constexpr double Parapet = 10.9;                   // the parapets' tops (under the coping's weathering)
	constexpr double NaveStone = 8.5;                  // the end walls' stone in the nave, the screens above

	// ---------------------------------------------------------------- The columns
	constexpr double ColumnX = NaveHalf;
	constexpr double BaseTop = 0.35, BaseRadius = 0.36;
	constexpr double ShaftRadius = 0.25, ShaftTop = 5.5;
	constexpr double CapitalTop = 6.2, AbacusHalf = 0.45;
	constexpr double ClusterTop = 9.70;                // the clustered iron shaft's capital, under the girder
	constexpr double GirderBottom = 9.70, GirderTop = 10.0, GirderHalf = 0.20;

	// ---------------------------------------------------------------- The arcade's iron arches (between the capitals)
	constexpr double ArcadeSpring = CapitalTop;
	constexpr double ArcadeHalfSpan = 2.75;            // springing 0.25 m off each column's centre
	constexpr double ArcadeCrown = GirderBottom;       // 9.7
	/** The two-centred arch through (±2.75, 6.2) and (0, 9.7): R = (h² + s²) / 2s with s = 2.75, h = 3.5. */
	constexpr double ArcadeRadius = (3.5 * 3.5 + 2.75 * 2.75) / (2.0 * 2.75);   // 3.602

	// ---------------------------------------------------------------- The vaults (the glass lies on these arcs)
	constexpr double NaveRadius = 7.0833333, NaveCentreOffset = NaveRadius - NaveHalf;   // centres at x ∓1.083
	constexpr double AisleHalfSpan = 4.0;
	constexpr double AisleRadius = 5.38, AisleCentreOffset = AisleRadius - AisleHalfSpan;  // 1.38
	constexpr double AisleCentreX = 10.0;              // the aisles' crowns, x ±10
	inline double NaveCrown() { return Spring + FMath::Sqrt(NaveRadius * NaveRadius - NaveCentreOffset * NaveCentreOffset); }        // 17.0
	inline double AisleCrown() { return Spring + FMath::Sqrt(AisleRadius * AisleRadius - AisleCentreOffset * AisleCentreOffset); }   // 15.2
	constexpr double RibPitch = 1.5;                   // a rib every 1.5 m, the column lines' heavier
	constexpr double BarPitch = 0.75;                  // glazing bars along the arcs (the panes' width)
	constexpr double PanePitch = 0.60;                 // the panes' length round the arc (the glass is faceted)

	// ---------------------------------------------------------------- Lancets (side walls), one a bay
	constexpr double LancetHalf = 0.70, LancetSill = 5.0, LancetHead = 9.0;
	/** The lancet's pointed head: two arcs of radius LancetArch from the springing at LancetSpring. */
	constexpr double LancetArch = 0.808;                // the board's 40.4 px at 50 px/m
	inline double LancetSpring() { return LancetHead - FMath::Sqrt(LancetArch * LancetArch - FMath::Square(LancetArch - LancetHalf)); }

	// ---------------------------------------------------------------- The south screen's stained band
	constexpr double BandX0 = -4.8, BandX1 = 4.8;      // six columns of 1.6 m
	constexpr double BandPanelW = 1.6;
	constexpr double LowerRow0 = 9.0, LowerRow1 = 10.7, UpperRow0 = 10.9, UpperRow1 = 12.6;
	/** The screens' mullions (x) and transoms (heights) in the nave. */
	constexpr double ScreenMullions[9] = {-6.0, -4.8, -3.2, -1.6, 0.0, 1.6, 3.2, 4.8, 6.0};
	constexpr double ScreenTransoms[6] = {8.5, 9.0, 10.7, 10.9, 12.6, 14.8};

	// ---------------------------------------------------------------- The porch and the doors
	constexpr double PorchHalf = 3.0, PorchOuterHalf = 3.6;
	constexpr double PorchCeiling = 6.9, PorchRoof = 7.45;     // a panelled ceiling under a lead flat
	constexpr double DrumOuter = MuseePlan::Rotunda::Radius + MuseePlan::Rotunda::Wall;   // 11.2
	constexpr double RotundaDoorHalf = MuseePlan::Rotunda::DoorWidthNorthSouth / 2;       // 2.0
	constexpr double RotundaDoorSpring = MuseePlan::Rotunda::DoorSpring;                   // 4.5
	constexpr double SunClockRadius = 10.3;            // the sun clock's 128-gon: the porch floor meets its edge
	constexpr int32 SunClockSides = 128;
	constexpr double CourtDoorHalf = 2.0, CourtDoorSpring = 3.3, CourtDoorCrown = 5.5;
	/** The court door's pointed head: R = (h² + s²) / 2s, s = 2, h = 2.2. */
	constexpr double CourtDoorRadius = (2.2 * 2.2 + 2.0 * 2.0) / (2.0 * 2.0);   // 2.21
	/** Ruskin's words over the court door, the court side: a band x ±4.6 at 6.5 … 7.5 m. */
	constexpr double RuskinHalf = 4.6, RuskinBottom = 6.5, RuskinTop = 7.5;

	// ---------------------------------------------------------------- Buttresses (outside)
	constexpr double ButtressDepth = 0.9, ButtressHalf = 0.5;   // on the side walls' column lines
	constexpr double ButtressTop = 9.9, ButtressSetOff = 4.8;

	// ---------------------------------------------------------------- The hang's furniture
	constexpr double BenchX = 3.2, BenchY = 40.0, BenchLength = 3.0, BenchDepth = 0.5, BenchHeight = 0.45;
	constexpr double CaseX = 10.4, CaseLength = 2.2, CaseDepth = 1.0, CaseTop = 0.95;
	constexpr double CaseY[2] = {29.2, 41.2};           // lines 3 and 5
	constexpr double LecternX = 12.2, LecternY = 20.2;  // Morris's bay (E1)
}
