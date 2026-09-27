#pragma once

#include "CoreMinimal.h"
#include "FormDrawing.h"
#include "FormMadePiece.h"
#include "MotionExtentSpec.h"
#include "MuseeFinding.h"
#include "MuseeStageDresser.h"

namespace Musee::MotionMaking
{
	/** Motion's parts are gathered before Nature's and Art's (order 10), as its dresser runs first. */
	inline constexpr int32 PieceSourceOrder = 10;

	/**
	 * Motion's piece source: reads the stage's Mechanisms.json (a stage without one has no machines) and draws the parts
	 * of every piece of variable length with Form's drawing: each stage of a telescope, drawn home (every stage's far
	 * end at the closed length, as a collapsed ram stands), and one turn of a column of turns, at its anchored end. The
	 * dresser then moves them.
	 */
	void GatherParts(const FMuseeStageContext& Stage, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings);

	/** The drawings of an extent's parts: a telescope's stages, or a column's one turn. */
	[[nodiscard]] TArray<FFormDrawing> DrawExtent(const FMotionExtentSpec& Extent);
}
