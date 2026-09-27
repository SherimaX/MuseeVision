#pragma once

#include "CoreMinimal.h"
#include "FormMadePiece.h"
#include "MuseeFinding.h"
#include "MuseeStageDresser.h"

/**
 * A system that adds made pieces to a stage while it is being made: Nature grows the stage's plants and shapes its water
 * from the planting plan, Art builds its canvases from the hanging plan. Each reads its own schedule from the stage's
 * context and appends pieces; anything it can't make is a finding. The stage's own architecture comes first, from its
 * description; the sources add to it.
 */
using FFormPieceSource = TFunction<void(const FMuseeStageContext& Stage, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)>;

namespace Musee::FormMaking
{
	/** Registers a system's piece source (its making module's startup). Sources run in ascending Order. */
	MUSEEFORMMAKING_API void RegisterPieceSource(FName System, int32 Order, FFormPieceSource Source);

	/** Removes a system's piece source (its making module's shutdown). */
	MUSEEFORMMAKING_API void UnregisterPieceSource(FName System);

	/** Runs every registered source on a stage. Studio calls this once per stage it makes, after the stage's own pieces. */
	MUSEEFORMMAKING_API void GatherPieces(const FMuseeStageContext& Stage, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings);
}
