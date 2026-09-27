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

	/**
	 * A stage's fabric: its description, the ports the site plan mates, and every piece made for it, with the records the
	 * making read.
	 */
	struct FStageFabric
	{
		FSiteDescription Description;
		/** The stage's ports joined to a built neighbour as Site solves the plan, in the description's order; the rest are blind. */
		TArray<FMuseeKey> MatedPorts;
		TArray<FFormMadePiece> Pieces;
		FMuseeDependencySet Dependencies;
	};

	/**
	 * The stage contract's first three steps and the shell check: describe the stage (a Ground stage with the buildings the
	 * site plan stands on it), solve the site plan for the ports it
	 * mates and add a blind for each it leaves unmated, make every drawing, gather the pieces the systems add, and check
	 * each enclosure's pieces are closed solids (Form.Sealed). Everything found wrong is appended to OutFindings.
	 */
	FStageFabric MakeFabric(const USiteRoom& Room, const FStageMakeOptions& Options, const FString& ScheduleDir,
		TArray<FMuseeFinding>& OutFindings);

	/** What the site plan reads of every loaded room: its ports, which the plan mates, and its footprint, which it places. */
	struct FSiteRooms
	{
		TMap<FMuseeKey, TArray<FSitePort>> Ports;
		TMap<FMuseeKey, FSiteFootprint> Footprints;
	};

	/** Every loaded room's ports and footprint, by stage, each room described once. */
	FSiteRooms DescribeRooms(FMuseeDependencySet& Dependencies);
}
