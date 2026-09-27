#pragma once

#include "CoreMinimal.h"
#include "FormMadePiece.h"
#include "MuseeDependencySet.h"
#include "MuseeFinding.h"
#include "SiteDescription.h"

class USiteRoom;

namespace Musee::Studio
{
	struct FStageMakeOptions;

	/** A stage's fabric: its description and every piece made for it, with the records the making read. */
	struct FStageFabric
	{
		FSiteDescription Description;
		TArray<FFormMadePiece> Pieces;
		FMuseeDependencySet Dependencies;
	};

	/**
	 * The stage contract's first three steps and the shell check: describe the stage, add a blind for each port the site
	 * plan leaves unmated, make every drawing, gather the pieces the systems add, and check each enclosure's pieces are
	 * closed solids (Form.Sealed). Everything found wrong is appended to OutFindings.
	 */
	FStageFabric MakeFabric(const USiteRoom& Room, const FStageMakeOptions& Options, const FString& ScheduleDir,
		TArray<FMuseeFinding>& OutFindings);

	/** The ports of every loaded room, by stage: what the site plan mates. */
	TMap<FMuseeKey, TArray<FSitePort>> PortsOfRooms(FMuseeDependencySet& Dependencies);
}
