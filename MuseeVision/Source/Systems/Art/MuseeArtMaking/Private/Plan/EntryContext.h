#pragma once

#include "ArtHangReading.h"
#include "Cast/CastRecord.h"
#include "CoreMinimal.h"
#include "MatterFinishSchedule.h"
#include "Support/ArtMedia.h"
#include "Vessel/VesselRecord.h"
#include "Objects/HandscrollRecords.h"
#include "Objects/HangingRecords.h"
#include "Ware/WareMesh.h"

namespace Musee::ArtMaking
{
	/**
	 * What the checks of a plan's entries read once: Care.json's keys, the stage's finishes, the turned forms, the books,
	 * the meshes works hang on, and for works in the round the media, how works are cast and how vessels are turned, every finish they name offered
	 * by the palette (RoundWhy says why they can't be read).
	 */
	struct FEntryContext
	{
		FCareKeys Care;
		TOptional<FMatterFinishSchedule> Finishes;
		FString FinishesWhy;
		TArray<FArtTurned> Turned;
		FString TurnedWhy;
		TOptional<FArtMedia> Media;
		TOptional<FArtCasts> Casts;
		TOptional<FArtVessels> Vessels;
		FString RoundWhy;
		TArray<FArtBook> Books;
		FString BooksWhy;
		/** A handscroll's fittings and the works' records (their photographs' reading order), and why not when unread. */
		TOptional<FArtHandscrollFittings> Handscroll;
		TArray<FArtHandscroll> Handscrolls;
		FString HandscrollWhy;
		TArray<FArtMesh> Meshes;
	};

	/** Whether the plan frames the object in a surface of the finish schedule (`"frame"` names one, not `none`). */
	[[nodiscard]] bool IsFramed(const FArtHangEntry& Entry);

	/**
	 * What is wrong with making one object on its own, one clause each: its size and place; for its support, the anchor
	 * it is made at and the surfaces it names (a canvas's frame; a panel's frame and wood; stained glass's glass; a
	 * print's mount, frame and glazing; a card's or plate's glazing; ware's kind, place and form; a tapestry's place off
	 * its wall; a hanging scroll's mount, knobs and cord; a book's or leaf's place, and a book's binding); and whether its
	 * id can name a layer.
	 */
	[[nodiscard]] TArray<FString> EntryFaults(const FMuseeStageContext& Stage, const FArtHangEntry& Entry, const FEntryContext& Context);

	/** The thickness of a pane of glass finished as this surface of the schedule (metres), from its working; zero if none. */
	[[nodiscard]] double PaneMeters(FName Surface, const FEntryContext& Context);

	/** The thickness of the glass a glazed object lies under (metres), from its glazing's working; zero if it has none. */
	[[nodiscard]] double GlazingMeters(const FArtHangEntry& Entry, const FEntryContext& Context);

	/**
	 * Whether glass is a transparency (an autochrome): on a Wall anchor in its window mat, standing in its frame against
	 * the wall and lit through the diffusing pane behind it, rather than stained glass leaded in a Glazing light.
	 */
	[[nodiscard]] bool IsTransparency(const FMuseeStageContext& Stage, const FArtHangEntry& Entry);

	/** A piece of turned ware's form: its kind and size from the plan, its window and outline from Data/Art/Turned.json. */
	[[nodiscard]] FWareSpec WareOf(const FArtHangEntry& Entry, const FEntryContext& Context);

	/** A book's binding, from Data/Art/Books.json, or null. */
	[[nodiscard]] const FArtBook* FindBook(const FString& Work, TConstArrayView<FArtBook> Books);

	/** The fittings and the works' records every handscroll is made with, read once when the plan holds a handscroll. */
	void ReadHandscrollsInto(const FArtHang& Hang, FEntryContext& Context);

	/**
	 * A handscroll's photographs laid across its sheet (Scroll/ScrollLayout.h): its one photograph, or its numbered ones
	 * (Musee::Art::FindSectionImages) joined in the reading order Data/Art/Handscrolls.json gives, right to left, so they lie
	 * left to right in the reverse; each at its own proportion, within the share of it that is the work. Faults, one
	 * clause each, where they can't be laid; a note, where they can, when their proportion strays from the catalogue's by
	 * more than the fittings' tolerance (laid to cover the sheet, what overruns it cut from its edges; never stretched).
	 */
	void PlanHandscroll(const FArtWork& Work, const FArtHangEntry& Entry, const FEntryContext& Context, FCanvasPlan& OutCanvas,
		TArray<FString>& OutFaults, TArray<FString>& OutNotes);
}
