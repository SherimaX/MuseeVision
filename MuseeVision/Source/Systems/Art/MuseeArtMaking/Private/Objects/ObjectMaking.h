#pragma once

#include "ArtHang.h"
#include "ArtHangReading.h"
#include "Cast/CastRecord.h"
#include "CoreMinimal.h"
#include "FormMadePiece.h"
#include "Frame/Moulding.h"
#include "MuseeFinding.h"
#include "MuseeStageDresser.h"
#include "Ware/WareMesh.h"

namespace Musee::ArtMaking
{
	/**
	 * What every object of a stage is made with: its stage and plan, each frame style's moulding, ware's forms and how
	 * works are cast, read once.
	 */
	struct FObjectMaking
	{
		const FMuseeStageContext& Stage;
		const FArtHang& Hang;
		TMap<FString, TValueOrError<FArtMoulding, FString>> Mouldings;
		TOptional<TValueOrError<FArtWareForms, FString>> WareForms;
		TOptional<TValueOrError<FArtCasts, FString>> CastRecord;

		/** The moulding a frame of this style is made in (FArtMoulding::RecordFor), or why it can't be read. */
		const TValueOrError<FArtMoulding, FString>& Moulding(FName FrameStyle);

		/** How ware is turned (Data/Art/Ware.json), or why it can't be read. */
		const TValueOrError<FArtWareForms, FString>& Forms();

		/** How works in the round are cast (Data/Art/Casts.json), or why it can't be read. */
		const TValueOrError<FArtCasts, FString>& Casts();
	};

	/** A piece Art makes: named, finished in its surface or in a material spec, colliding as a solid, drawn by Art. */
	[[nodiscard]] FFormMadePiece ArtPiece(FName Name, FName Surface, const FLayerFinish& Material, const FMuseeFrame& Frame, bool bNanite);

	/**
	 * The own frame of a work standing in the round on its Mount anchor: at its foot, its `centre`; y its front (the plan's
	 * facing, a bearing from the stage's +x towards +y, levelled on the bed); z the bed's up; x to the viewer's right as the
	 * viewer faces its front.
	 */
	[[nodiscard]] FMuseeFrame RoundFrame(const FArtHangEntry& Entry, const FMuseeFrame& Bed);

	/**
	 * A print on its mount on a Wall anchor: the mount, a board of its size and depth, its face on the anchor's plane; the
	 * print on it, at its place in the plan, a sheet of paper in the work's own layer; the glazing over the mount, lying on
	 * the print; and the frame round the mount, its lip on the glazing and its back flush with the mount's.
	 */
	void MakePrint(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces,
		TArray<FMuseeFinding>& OutFindings);

	/**
	 * A transparency (an autochrome) on a Wall anchor: its window mat, its face on the anchor's plane with the plate's
	 * window cut through it; the plate behind the window, a pane of its glass carrying the work's own layer; the diffusing
	 * pane behind the plate, through which the daylight behind lights it; the glazing on the mat; and the frame round the
	 * mat, its back flush with the backing's.
	 */
	void MakeTransparency(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces,
		TArray<FMuseeFinding>& OutFindings);

	/** A card or a daguerreotype's plate lying on its reader (a Mount anchor), centred on it, under its glazing. */
	void MakeCard(const FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces);

	/**
	 * A piece of turned ware standing at its centre on its Mount anchor, its front towards its facing: its decorated
	 * faces in the work's own layer over the support's lustre, and its ground in the support's plain glaze.
	 */
	void MakeWare(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces,
		TArray<FMuseeFinding>& OutFindings);

	/**
	 * A work cast from its scan, standing on its carrier's top (its Mount anchor), its front towards its facing: the scan
	 * scaled to the work's true height (the plan's size), its base slice centred on the plan's `centre`, in its medium's
	 * finish (Data/Art/Casts.json), and any part the record casts in another material (a dancer's tulle skirt) a piece of
	 * its own. A scan whose width at that height strays from the plan's is a finding, and the work isn't made.
	 */
	void MakeCast(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces,
		TArray<FMuseeFinding>& OutFindings);
}
