#pragma once

#include "CoreMinimal.h"
#include "SiteRoom.h"

#include "RotundaRoom.generated.h"

/**
 * The Rotunda (D-012), the anchor stage, where every visit begins on the gilt sun: a drum of honed Crema Marfil under
 * a coffered dome, lit from one glazed eye whose pearl lattice's node is the nodus of a sun clock laid in the floor.
 * Four doors lead to four chapters, four casts stand in the diagonal niches, and from the ring the whole dial rises
 * 3.50 m as a tempietto, uncovering a helical stair down to the Classical Hall. Its design sheet is
 * Docs/Stages/rotunda.md.
 *
 * Its frame: origin at the drum's centre on the finished floor (the top of the ring and of the lowered dial); x east,
 * y south, z up, true north. Its origin is the museum's. It declares, in that frame and in metres:
 * - **ports** `rotunda.west`, `.north`, `.south` and `.east` (arched doorways through the drum, their frames on the
 *   drum's outer face r 11.20) and `rotunda.stair` (the spiral stair's portal at the landing, r 7.60, z -6.00), which
 *   serves the well under the drum;
 * - **anchors** `rotunda.sun-clock.drum` and `rotunda.stair.balustrade.panels` (Body), `rotunda.sun-clock.control`
 *   (Use) and `rotunda.niche.ne|se|sw|nw.cast` (Mount);
 * - **enclosures** `rotunda.hall` and `rotunda.stair`, and the Glazed opening `rotunda.eye`.
 */
UCLASS()
class URotundaRoom : public USiteRoom
{
	GENERATED_BODY()

public:
	virtual FMuseeKey StageKey() const override;
	virtual void Describe(FSiteDescription& Out) const override;
};
