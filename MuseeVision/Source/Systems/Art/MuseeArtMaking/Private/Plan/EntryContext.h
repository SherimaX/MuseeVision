#pragma once

#include "ArtHangReading.h"
#include "CoreMinimal.h"
#include "MatterFinishSchedule.h"
#include "Objects/HangingRecords.h"
#include "Ware/WareMesh.h"

namespace Musee::ArtMaking
{
	/** What the checks of a plan's entries read once: Care.json's keys, the stage's finishes, the turned forms and the books. */
	struct FEntryContext
	{
		FCareKeys Care;
		TOptional<FMatterFinishSchedule> Finishes;
		FString FinishesWhy;
		TArray<FArtTurned> Turned;
		FString TurnedWhy;
		TArray<FArtBook> Books;
		FString BooksWhy;
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
}
