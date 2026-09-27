#pragma once

#include "CoreMinimal.h"
#include "FormMadePiece.h"
#include "MuseeFinding.h"
#include "MuseeStageDresser.h"

namespace Musee::ArtMaking
{
	/** Art's pieces are gathered after Nature's (order 30), and its works dressed after Nature's behaviour (order 30). */
	inline constexpr int32 PieceSourceOrder = 30;
	inline constexpr int32 DresserOrder = 30;

	/** The names of a hung work's two pieces: its painted face, `canvas.<id>`, and its linen, `canvas.<id>.linen`. */
	[[nodiscard]] FName FacePieceName(const FString& WorkId);
	[[nodiscard]] FName LinenPieceName(const FString& WorkId);

	/**
	 * Art's piece source: reads the stage's Hang.json (a stage without one hangs nothing) and makes each work as a real
	 * object at its Wall anchor: a stretched canvas at its real size, its face a piece in the work's own paint
	 * (Musee::Art::PaintMaterial), its tacking edges and back a piece of linen. Anything the plan gets wrong is a finding
	 * and that work isn't made.
	 */
	void GatherArt(const FMuseeStageContext& Stage, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings);

	/**
	 * Art's dresser: gives each hung work's face its label and care class (a UArtWorkComponent on it, and the tags
	 * `musee.work:<id>` and `musee.care:<class>` on both its pieces), for the visitor's hands and Light's checks.
	 */
	void DressWorks(const FMuseeStageContext& Stage, TArray<FMuseeFinding>& OutFindings);
}
