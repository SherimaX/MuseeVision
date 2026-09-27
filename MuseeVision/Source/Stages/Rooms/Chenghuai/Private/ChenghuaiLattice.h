#pragma once

#include "ChenghuaiFrame.h"
#include "ChenghuaiHall.h"
#include "ChenghuaiPlan.h"
#include "CoreMinimal.h"

/**
 * Chenghuai's lattices: the fronts' panels of bars over paper (§3.6) and the leak windows' grilles in the walls (§3.5). A
 * lattice is one slab of wood or fired clay pierced by its openings, so it is drawn as the panel's outline with every
 * opening a hole: the openings are found exactly from the pattern's bars (the axis-square patterns by their own grid, the
 * ice-crack by its cuts, the hexagons by clipping), and the coins' interlocking rings are traced on a fine grid.
 */
namespace Musee::Chenghuai
{
	/** Closed loops in a panel's own (a, b), metres. */
	using FLoops = TArray<TArray<FVector2D>>;

	/** The openings of a front's rectangular panel, W × H from its lower left corner, inside its rim, in a pattern. */
	FLoops PanelOpenings(ELattice Pattern, double Width, double Height, int32 Seed);

	/** A leak window's shape about its centre (a convex loop): a circle, a hexagon, an octagon turned 22.5°. */
	TArray<FVector2D> LeakShape(Plan::ELeak Leak, const FVector2D& Centre, double Radius);

	/** The openings of a leak window's lattice inside its rim. */
	FLoops LeakOpenings(Plan::ELeak Leak, TConstArrayView<FVector2D> Shape, const FVector2D& Centre, double Radius, int32 Seed);

	/**
	 * A lattice panel of a front standing in a building's frame: W across u from U, H up from Z, its bars LatticeDepth deep
	 * along v from V; the paper (laminated in glass) behind it on the room's side (+v) when bPaper.
	 */
	void DrawPanel(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* Surface, const FBuildingFrame& F, double U, double V, double Z,
		double Width, double Height, ELattice Pattern, bool bPaper);

	/**
	 * A partition door leaf (隔扇) standing in a building's frame, W across u from U, H up from Z, its face at V: stiles and
	 * five rails round a carved apron panel, a waist panel, a lattice head over paper and a top panel.
	 */
	void DrawLeaf(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* Surface, const FBuildingFrame& F, double U, double V, double Z,
		double Width, double Height, ELattice Pattern);
}
