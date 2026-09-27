#include "MuseeStageDresser.h"

namespace
{
	struct FDresserEntry
	{
		FName System;
		int32 Order = 0;
		FMuseeStageDresser Dresser;
	};

	/** The registered dressers, kept sorted by Order (then by system name, so the order never depends on load order). */
	TArray<FDresserEntry>& Dressers()
	{
		static TArray<FDresserEntry> Entries;
		return Entries;
	}
}

void Musee::RegisterStageDresser(FName System, int32 Order, FMuseeStageDresser Dresser)
{
	TArray<FDresserEntry>& Entries = Dressers();
	Entries.RemoveAll([System](const FDresserEntry& E) { return E.System == System; });
	Entries.Add({ System, Order, MoveTemp(Dresser) });
	Entries.StableSort([](const FDresserEntry& A, const FDresserEntry& B)
	{
		return A.Order != B.Order ? A.Order < B.Order : A.System.LexicalLess(B.System);
	});
}

void Musee::UnregisterStageDresser(FName System)
{
	Dressers().RemoveAll([System](const FDresserEntry& E) { return E.System == System; });
}

void Musee::DressStage(const FMuseeStageContext& Stage, TArray<FMuseeFinding>& OutFindings)
{
	for (const FDresserEntry& Entry : Dressers())
	{
		Entry.Dresser(Stage, OutFindings);
	}
}

TArray<FName> Musee::StageDressers()
{
	TArray<FName> Names;
	for (const FDresserEntry& Entry : Dressers())
	{
		Names.Add(Entry.System);
	}
	return Names;
}
