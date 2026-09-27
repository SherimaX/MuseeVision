#pragma once

#include "CoreMinimal.h"
#include "SiteRoom.h"

#include "GroundsRoom.generated.h"

/**
 * The Grounds, the museum's site under its own sky: a French formal front south of the Salon's portico (a stone parvis,
 * a gravel cour round a 30 m still canal, two box-edged lawn panels, osmanthus allées, a yew exedra and a birch avenue
 * running south), a gravel walk east that bends round Albion's south end to a rond-point with a round basin on the
 * Élan's meridian, a path north to a gravel terrace round the Élan's socle, lawn up to every wall, groves round the
 * buildings and ten bronze lamp standards. It is seen from the rooms, never entered: it has no port. Its design sheet is
 * Docs/Stages/grounds.md.
 *
 * Its frame is the museum's plan frame itself: origin under the gilt sun at the Rotunda's centre, z 0 the ground floor's
 * finished level; x east, y south, z up, true north. The site plan places it at the anchor's datum (a "placed" entry, not
 * a mating). It declares, in that frame and in metres:
 * - a **Ground footprint**, the 800 m square, from the soil's bottom (-0.33) to the tallest birch (13.0);
 * - no **ports**;
 * - **anchors** `grounds.canal.water` and `grounds.rond-point.water` (Water), `grounds.lamp.01`...`10` (Mount, the globes'
 *   centres), `grounds.lawn`, the hedges and the 37 trees (Bed), and `grounds.canal.coping.seat` and
 *   `grounds.rond-point.coping.seat` (Seat, a frame every 0.80 along each coping's seat line);
 * - the **enclosure** `grounds.open` (Outdoors).
 */
UCLASS()
class UGroundsRoom : public USiteRoom
{
	GENERATED_BODY()

public:
	virtual FMuseeKey StageKey() const override;
	virtual void Describe(FSiteDescription& Out) const override;
};
