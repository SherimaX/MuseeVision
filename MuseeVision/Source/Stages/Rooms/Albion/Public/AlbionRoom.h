#pragma once

#include "CoreMinimal.h"
#include "SiteRoom.h"

#include "AlbionRoom.generated.h"

/**
 * Albion, the British court on the Rotunda's south door (D-001): England 1848 to 1898, a museum court of iron and glass
 * on ten columns of polished British stone, after the Oxford University Museum. Under three pointed glass vaults on iron
 * ribs, the Pre-Raphaelites hang on banded walls of Bath and Mansfield stone, truth to nature walked south down the west
 * aisle and beauty north up the east, the legend of the Holy Grail at the south end under the Tristram and Isolde band.
 * Its design sheet is Docs/Stages/albion.md.
 *
 * Its frame: origin at the court's centre on the nave's axis and on column line 4, at the finished floor; x east, y
 * south, z up, true north. It declares, in that frame and in metres:
 * - the **port** `albion.north` (doorway.4000x6500.arched on the flat port plane y -24.00, facing north, meeting
 *   rotunda.south), its only one: the court is a dead end by design;
 * - **anchors** `albion.wall.west|east|south|north` (Wall), `albion.case.w3|e3|w5|e5` and `albion.lectern` (Mount),
 *   `albion.band.t1`…`t12` and `albion.lancet.w1`…`w6`, `e1`…`e6` (Glazing), and the benches' eight Seats;
 * - the **enclosure** `albion.court` with its porch, and its Glazed openings: the three vaults, the nave's screens, the
 *   four gables, the band and the twelve lancets.
 */
UCLASS()
class UAlbionRoom : public USiteRoom
{
	GENERATED_BODY()

public:
	virtual FMuseeKey StageKey() const override;
	virtual void Describe(FSiteDescription& Out) const override;
};
