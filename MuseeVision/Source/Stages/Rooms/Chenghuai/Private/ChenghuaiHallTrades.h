#pragma once

#include "ChenghuaiHall.h"
#include "CoreMinimal.h"

/**
 * The Qing hall's trades, each drawing its part of one building from the spec and its section: the platform and columns
 * (§3.2), the timber frame (§3.3), the roof (§3.4), the gables and back wall (§3.5) and the front (§3.6).
 */
namespace Musee::Chenghuai::Hall
{
	/** A piece's name under its building: <building>.<Part>. */
	FString Named(const FHallSpec& Spec, const FString& Part);

	/** Whether a hard gable stands at the U0 (or U1) end. */
	bool HasGable(const FHallSpec& Spec, bool bAtU1);

	/** The column axes along u that stand free (the ends stand in the gables, unless that end is open). */
	TArray<double> FreeColumns(const FHallSpec& Spec);

	/** The surface of the painted members (lintels, boards, beams, purlins): painted by rank, else the timber's own. */
	const TCHAR* PaintedOr(const FHallSpec& Spec, const TCHAR* Plain);

	void DrawPlatform(TArray<FFormDrawing>& Out, const FHallSpec& Spec);
	void DrawColumns(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FHallSection& Section);
	void DrawTimber(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FHallSection& Section);
	void DrawRoof(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FHallSection& Section);
	void DrawGables(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FHallSection& Section);
	void DrawBackWall(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FHallSection& Section);
	void DrawFront(TArray<FFormDrawing>& Out, const FHallSpec& Spec, const FHallSection& Section);

	/** The room's Glazed lattice and its open door pairs, facing out through the front. */
	void DrawFrontOpenings(const FHallSpec& Spec, TArray<FFormOpening>& Out);
}

namespace Musee::Chenghuai
{
	/**
	 * A column (§3.2) at (u, v) of a building's frame: its base stone (柱顶石) 1.9 D square, the turned drum (鼓镜) on it, and
	 * the shaft from the floor + 0.06 to its top, tapering 1 per mille of its height over its diameter.
	 */
	void DrawColumn(TArray<FFormDrawing>& Out, const FString& Name, const FBuildingFrame& F, double U, double V, double Floor, double Top,
		double Diameter, const TCHAR* Timber, double BaseSide = 0.0);
}
