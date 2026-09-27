#pragma once

#include "CoreMinimal.h"
#include "SiteRoom.h"

#include "NympheasRoom.generated.h"

/**
 * The Nymphéas Oval (D-019), the museum's first room and the site plan's anchor: a white elliptical room under a linen
 * velarium and a glass dome, the four willows of the Orangerie's second room curved on its wall, and at its centre a
 * black pond whose coping carries a carved lily frieze. Touch the golden iris and the pond rises on four bronze posts,
 * uncovering a long stair down to the Reserve. Its design sheet is Docs/Stages/nympheas.md.
 *
 * Its frame: origin at the oval's centre on the finished floor; x east, y south, z up, true north. It declares, in that
 * frame and in metres:
 * - **ports** `nympheas.east` (doorway.3000x4800.arched on the passage's port face, x 11.90, meeting salon.west) and
 *   `nympheas.stair` (stairwell.straight.2400x3200.arched at the stair's foot, x 11.90, z -5.80, meeting reserve.stair),
 *   which serves the well under the oval;
 * - **anchors** `nympheas.basin` and `nympheas.guard-rails` (Body), `nympheas.golden-iris` (Use, and the Bed of its cast
 *   specimen), `nympheas.pond.water` (Water), `nympheas.pond.lilies` and `nympheas.pond.shelf.east|west` (Bed),
 *   `nympheas.wall.willows` (Wall), `nympheas.bench.b1`…`b4` and `nympheas.coping.seat` (Seat);
 * - **enclosures** `nympheas.oval` and `nympheas.well`, and the Glazed opening `nympheas.roof`.
 */
UCLASS()
class UNympheasRoom : public USiteRoom
{
	GENERATED_BODY()

public:
	virtual FMuseeKey StageKey() const override;
	virtual void Describe(FSiteDescription& Out) const override;
};
