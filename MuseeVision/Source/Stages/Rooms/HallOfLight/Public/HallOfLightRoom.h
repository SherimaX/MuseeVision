#pragma once

#include "CoreMinimal.h"
#include "SiteRoom.h"

#include "HallOfLightRoom.generated.h"

/**
 * The Hall of Light, the museum's photography hall: a glass hall 9 m wide under a shallow vault of pearl steel and glass,
 * running 28 m east from the Rotunda's drum to the Atrium's stone base between a birch meadow (north) and an orchard
 * (south). It hangs true-size exhibition facsimiles of 24 works, forward in time walking east: sixteen prints on mounts
 * on bronze stems, four autochromes in cassettes against the glass lit by the garden's daylight through a diffusing
 * backing, and four stereo cards on bronze readers over travertine stones. Its design sheet is
 * Docs/Stages/hall-of-light.md.
 *
 * Its frame: origin on the hall's axis, on the finished floor, on the Rotunda drum's outer face (museum = local
 * + (11.200, 0, 0)); x east, y south, z up, true north. It declares, in that frame and in metres:
 * - **ports** `hall-of-light.west` (doorway.3000x6000.arched at (0, 0, 0) on the Rotunda's drum, R 11.20 about local
 *   (-11.20, 0), meeting rotunda.east) and `hall-of-light.east` (doorway.4000x4500.square at (28, 0, 0) on the Atrium's
 *   base, R 14.80 about (42.80, 0), meeting atrium.west); both faces are concave onto the neighbours;
 * - **anchors** `hall-of-light.prints.north|south` and `hall-of-light.autochromes.north|south` (Wall),
 *   `hall-of-light.stereo.1`...`4` (Mount, the reader plates), and the gardens' beds (Bed): the grounds, the flower and
 *   bulb beds, `hall-of-light.birch.1`...`8`, `hall-of-light.orchard.1`...`10` and `hall-of-light.hedge.north|south`;
 * - **enclosures** `hall-of-light.hall` (Enclosed) and the gardens `hall-of-light.meadow` and `hall-of-light.orchard`
 *   (Outdoors); the Glazed openings `hall-of-light.wall.north|south`, `hall-of-light.vault` and `hall-of-light.lunette`,
 *   and the step lights' twenty housings in the plinths.
 */
UCLASS()
class UHallOfLightRoom : public USiteRoom
{
	GENERATED_BODY()

public:
	virtual FMuseeKey StageKey() const override;
	virtual void Describe(FSiteDescription& Out) const override;
};
