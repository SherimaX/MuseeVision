#pragma once

#include "CoreMinimal.h"
#include "SiteRoom.h"

#include "CubeRoom.generated.h"

/**
 * The Cube, Élan's room 22 m under the Atrium floor (D-012): a sealed room 28 m each way whose six inner faces are
 * light-field panels, reached down a glass shaft through the site's soil, clay and chalk. The Atrium's glass car comes
 * down the shaft, the ceiling iris opens, closes round the head of a gilt mast, and the car sinks on the mast until the
 * visitor's eyes are at the room's exact centre; it never lands. There the museum's first work, II · Sea Light, performs
 * through the panels. Its design sheet is Docs/Stages/cube.md.
 *
 * Its frame is the Atrium's: origin on the Élan axis at the museum's floor datum (museum = local + (54, 0, 0)); x east,
 * y south, z up, true north. The room's centre is (0, 0, -22). It declares, in that frame and in metres:
 * - **one port** `cube.up` (iris.4600, the plane z -0.500 facing up, meeting atrium.down), whose well from the plane to
 *   z -23.800 it receives: it builds there only the rails in the class's notches and its ceiling iris;
 * - **anchors** `cube.up.iris.blade.1`…`7` (Body, each at its pin on its top face), `cube.performance` (Use, the work's
 *   control at the car's `cube` stop) and `cube.panels.ceiling|floor|north|south|east|west` (Mount, the panel faces);
 * - **enclosures** `cube.room`, `cube.shaft` and `cube.shaft.cavity`, the Port opening `cube.up` and the Housing
 *   `cube.up.iris.housing`. It has no daylight and no sky.
 */
UCLASS()
class UCubeRoom : public USiteRoom
{
	GENERATED_BODY()

public:
	virtual FMuseeKey StageKey() const override;
	virtual void Describe(FSiteDescription& Out) const override;
};
