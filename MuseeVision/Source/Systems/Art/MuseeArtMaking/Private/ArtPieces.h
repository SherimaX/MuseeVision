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

	/** The name of a stained-glass panel's glass: `glass.<id>`, or `glass.<id>.<index>` for one of a work's several panels. */
	[[nodiscard]] FName GlassPieceName(const FArtHangEntry& Entry, bool bSplit);

	/**
	 * The word an object Art makes is named by, from its support: `canvas`, `glass`, `paper` (a print, a card), `plate` (a
	 * daguerreotype), `ware`; `work` for any other.
	 */
	[[nodiscard]] const TCHAR* ObjectWord(FName Support);

	/**
	 * The name of an object Art makes: `<word>.<id>`, or `<word>.<id>.<index>` for one of a work's several objects. Its
	 * face (a canvas's paint, a print, a plate, ware's decorated faces) is that name; its other parts are `<name>.<part>`
	 * (`linen`, `frame`, `mount`, `glazing`, `ground`).
	 */
	[[nodiscard]] FString ObjectName(const FArtHangEntry& Entry, bool bSplit);
	[[nodiscard]] FName PartName(const FArtHangEntry& Entry, bool bSplit, const TCHAR* Part);

	/**
	 * The name a stage gives the piece it builds for a work (`"built": "form"`): `work.<id>`, or `work.<id>.<index>` for
	 * one of a work's several objects. A work built of several pieces names each `<that name>.<part>`; Art's caption is
	 * the component they all hang from.
	 */
	[[nodiscard]] FName BuiltPieceName(const FArtHangEntry& Entry, bool bSplit);

	/**
	 * Art's piece source: reads the stage's Hang.json (a stage without one hangs nothing) and makes each work as a real
	 * object at its anchor:
	 * - a canvas at its Wall anchor: a stretched canvas at its real size, its face a piece in the work's own paint
	 *   (Musee::Art::PaintMaterial, with the paint's working and history) showing its part of the work's image or its own,
	 *   its tacking edges and back a piece of linen, and, where the plan frames it, a moulded frame in the finish of the
	 *   frame's surface;
	 * - a stained-glass panel in its Glazing anchor (D-014: in Form's opening): a pane of the glass its `wall` surface
	 *   names, the image fired into it, its layer the work's own (`paint.<id>[.<index>]`) over that glass's finish;
	 * - a print at its Wall anchor: the print on its mount, under its glazing, in its frame (ObjectMaking.h);
	 * - a card or a daguerreotype's plate on its reader, a Mount anchor, under its glazing;
	 * - a dish or a vessel standing at its centre on its Mount anchor, turned from its photograph.
	 * A work the stage builds (`"built": "form"`) is left to the stage. Anything the plan gets wrong is a finding and
	 * that work isn't made.
	 */
	void GatherArt(const FMuseeStageContext& Stage, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings);

	/**
	 * Art's dresser: gives each work its label and care class, for the visitor's hands and Light's checks. Each object's
	 * pieces (a canvas's face, linen and frame; a panel's glass; the pieces a stage built for the work, named by
	 * BuiltPieceName) hang from a UArtWorkComponent carrying the label and class, set where the object's first piece is,
	 * so a look at any of them finds the label (the visitor looks up from what it sees); each piece carries the tags
	 * `musee.work:<id>` and `musee.care:<class>`. A built work whose pieces the stage didn't make is a finding.
	 */
	void DressWorks(const FMuseeStageContext& Stage, TArray<FMuseeFinding>& OutFindings);
}
