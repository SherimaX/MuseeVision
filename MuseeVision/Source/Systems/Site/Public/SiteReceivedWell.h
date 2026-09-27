#pragma once

#include "CoreMinimal.h"
#include "MuseeFinding.h"
#include "SitePort.h"
#include "SitePortClass.h"

/** A made piece of a stage as Site's well check reads it: its name, and its triangles' corners in threes, in the stage's frame, metres. */
struct FSiteWellPiece
{
	FName Name;
	TArray<FVector> TrianglesMeters;
};

namespace Musee::Site
{
	/** The part of its port's class a piece is in a well the stage receives (FSiteWell::Parts), or none. */
	[[nodiscard]] MUSEESITE_API FName PartOf(const FSiteWell& Well, FName Piece);

	/**
	 * Site.RoomStaysInside for a well a stage receives. The well is shaped as the port's class draws it: the circle of the
	 * class's well radius about the prism's centre, less its notches, from the prism's bottom to its top. Every piece that
	 * reaches into that section by more than the loop tolerance must be a part the stage names (FSiteWell::Parts) of a kind
	 * the class admits (the rails, the irises, the mast, the dogs, the sill); every other piece in the well, and every part
	 * the class doesn't admit, is a finding drawn by the stage, since the serving stage's car runs there.
	 */
	[[nodiscard]] MUSEESITE_API TArray<FMuseeFinding> CheckReceivedWell(const FSitePort& Port, const FSitePortClass& Class,
		TConstArrayView<FSiteWellPiece> Pieces);
}
