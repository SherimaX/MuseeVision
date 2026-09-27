#include "FormPieceSource.h"

namespace
{
	struct FSourceEntry
	{
		FName System;
		int32 Order = 0;
		FFormPieceSource Source;
	};

	/** The registered sources, sorted by Order and then by system name, so the order never depends on load order. */
	TArray<FSourceEntry>& Sources()
	{
		static TArray<FSourceEntry> Entries;
		return Entries;
	}
}

void Musee::FormMaking::RegisterPieceSource(FName System, int32 Order, FFormPieceSource Source)
{
	TArray<FSourceEntry>& Entries = Sources();
	Entries.RemoveAll([System](const FSourceEntry& E) { return E.System == System; });
	Entries.Add({ System, Order, MoveTemp(Source) });
	Entries.StableSort([](const FSourceEntry& A, const FSourceEntry& B)
	{
		return A.Order != B.Order ? A.Order < B.Order : A.System.LexicalLess(B.System);
	});
}

void Musee::FormMaking::UnregisterPieceSource(FName System)
{
	Sources().RemoveAll([System](const FSourceEntry& E) { return E.System == System; });
}

void Musee::FormMaking::GatherPieces(const FMuseeStageContext& Stage, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
{
	for (const FSourceEntry& Entry : Sources())
	{
		Entry.Source(Stage, OutPieces, OutFindings);
	}
}
