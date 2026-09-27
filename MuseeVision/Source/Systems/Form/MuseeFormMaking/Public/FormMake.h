#pragma once

#include "CoreMinimal.h"
#include "FormDrawing.h"
#include "FormMadePiece.h"
#include "FormShells.h"
#include "MuseeFinding.h"

/**
 * Form's making: the one place drawings become solid pieces. Studio hands it a stage's drawings; it cuts each into
 * pieces with the kernel (curves divided at the one chord tolerance, UVs in metres, tangents from the UVs, one crease
 * rule) and hands back pieces ready to cast. A drawing that can't be made is a finding named for its drawer, never a
 * silent gap or a stand-in shape.
 */
namespace Musee::FormMaking
{
	/** Makes one drawing into its pieces (one, or two for a glazed ceiling), appending them. */
	MUSEEFORMMAKING_API void Make(const FFormDrawing& Drawing, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings);

	/** Makes every drawing, in order. */
	MUSEEFORMMAKING_API void Make(TConstArrayView<FFormDrawing> Drawings, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings);

	/** Reads a made piece's skin as a shell: the input Form.Sealed judges (FFormShells). */
	[[nodiscard]] MUSEEFORMMAKING_API FFormPieceShell MeasureShell(const FFormMadePiece& Piece);
}
