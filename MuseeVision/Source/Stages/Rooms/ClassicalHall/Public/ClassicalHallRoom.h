#pragma once

#include "CoreMinimal.h"
#include "SiteRoom.h"

#include "ClassicalHallRoom.generated.h"

/**
 * The Classical Hall, level −1 under the Rotunda: a barrel-vaulted sculpture gallery after the Vatican's Braccio Nuovo,
 * four laylit bays of plaster casts in red niches and on consoles, and through a screen of two Ionic columns a round
 * domed tribune after the Sala delle Muse, its ring of twelve columns under a coffered annular vault and a coffered
 * dome with a laylit eye. The only way in is the Rotunda's spiral stair, whose well this stage receives. Its design
 * sheet is Docs/Stages/classical-hall.md.
 *
 * Its frame: origin at the tribune's centre on its finished floor, the axis of the centrepiece under the eye; x east,
 * y south, z up, true north. It declares, in that frame and in metres:
 * - **port** `classical-hall.stair` (stairwell.spiral.3600x4400.arched on the cylindrical port face at y 31.70, facing
 *   south, meeting rotunda.stair), which receives the Rotunda's well: the cylinder r 7.60 about (0, 39.30);
 * - **anchors** 46 `Mount`s, one at the top centre of every carrier of a work in the round: the niches, consoles,
 *   herms and plinths of the gallery and the tribune, and the centrepiece's pedestal;
 * - **enclosure** `classical-hall.interior`, whose one opening is the port, with the Housings of its five laylights.
 */
UCLASS()
class UClassicalHallRoom : public USiteRoom
{
	GENERATED_BODY()

public:
	virtual FMuseeKey StageKey() const override;
	virtual void Describe(FSiteDescription& Out) const override;
};
