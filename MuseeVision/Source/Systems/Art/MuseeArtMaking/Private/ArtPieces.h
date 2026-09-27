#pragma once

#include "ArtHangEntry.h"
#include "CoreMinimal.h"
#include "FormMadePiece.h"
#include "MuseeFinding.h"
#include "MuseeStageDresser.h"

namespace Musee::ArtMaking
{
	/** Art's pieces are gathered after Nature's (order 30), and its works dressed after Nature's behaviour (order 30). */
	inline constexpr int32 PieceSourceOrder = 30;
	inline constexpr int32 DresserOrder = 30;

	/**
	 * The name of a hung canvas: `canvas.<id>`, or `canvas.<id>.<index>` for one of a work's several canvases. Its pieces
	 * are its painted face (the canvas's own name), its linen (`<name>.linen`) and its frame (`<name>.frame`).
	 */
	[[nodiscard]] FString CanvasName(const FArtHangEntry& Entry, bool bSplit);
	[[nodiscard]] FName FacePieceName(const FArtHangEntry& Entry, bool bSplit);
	[[nodiscard]] FName LinenPieceName(const FArtHangEntry& Entry, bool bSplit);
	[[nodiscard]] FName FramePieceName(const FArtHangEntry& Entry, bool bSplit);

	/**
	 * Art's piece source: reads the stage's Hang.json (a stage without one hangs nothing) and makes each canvas as a real
	 * object at its Wall anchor: a stretched canvas at its real size, its face a piece in the work's own paint
	 * (Musee::Art::PaintMaterial, with the paint's working and history) showing its part of the work's image, its tacking
	 * edges and back a piece of linen, and, where the plan frames it, a moulded frame in the finish of the frame's
	 * surface. Anything the plan gets wrong is a finding and that work isn't made.
	 */
	void GatherArt(const FMuseeStageContext& Stage, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings);

	/**
	 * Art's dresser: gives each hung work its label and care class (a UArtWorkComponent on the face of its first canvas,
	 * and the tags `musee.work:<id>` and `musee.care:<class>` on every piece of every canvas), for the visitor's hands and
	 * Light's checks.
	 */
	void DressWorks(const FMuseeStageContext& Stage, TArray<FMuseeFinding>& OutFindings);
}
