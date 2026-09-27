#pragma once

#include "CoreMinimal.h"
#include "SiteRoom.h"

#include "SalonRoom.generated.h"

/**
 * The Salon Impression (D-019: the five bays and the Manet cabinet): the long gallery of the Impressionists between the
 * Rotunda and the Nymphéas oval, five bays under one coffered barrel vault, each silked in its own colour and lit from
 * above by its own lantern, their gilt rosettes catching the light; transverse arches of travertine on silked piers
 * between them; and off bay 1 the Manet cabinet, green and dark, lit only by its laylight, where the 1874 case keeps the
 * first exhibition's catalogue. Its design sheet is Docs/Stages/salon.md.
 *
 * Its frame: origin on the axis in the plane of bay 1's east end wall's silk face, on the finished floor; x east, y
 * south, z up, true north. It declares, in that frame and in metres:
 * - **ports** `salon.east` (doorway.3000x6000.arched on the end wall's outer face, x 0.60, meeting rotunda.west; the
 *   wall steps down inside to the Salon's own 3.00 × 4.80 door) and `salon.west` (doorway.3000x4800.arched on the far
 *   wall's outer face, x -60.60, meeting nympheas.east);
 * - **anchors** the hanging walls of every bay and of the cabinet and the stele's glass (Wall), the 1874 case and the
 *   dancer's plinth (Mount), the eight benches (Seat), the pergola's four planters (Bed);
 * - **enclosures** `salon.bays` (with the Glazed openings `salon.bay1.lantern` … `salon.bay5.lantern`), `salon.cabinet`
 *   (its door `salon.cabinet.door`) and `salon.outdoors.roof`.
 */
UCLASS()
class USalonRoom : public USiteRoom
{
	GENERATED_BODY()

public:
	virtual FMuseeKey StageKey() const override;
	virtual void Describe(FSiteDescription& Out) const override;
};
