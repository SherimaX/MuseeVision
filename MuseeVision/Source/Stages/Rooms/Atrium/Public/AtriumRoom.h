#pragma once

#include "CoreMinimal.h"
#include "SiteRoom.h"

#include "AtriumRoom.generated.h"

/**
 * The Atrium, Élan's stage at level 0 (D-012): a glass drum 28 m across on a 6 m travertine base, its misty double skin
 * rising to the equator at 22 m, and overhead the Sphere's glazed underside on a triangulated pearl grid, carried by
 * twelve pearl ribs to a ring round the car's way. At its centre a round glass car in a glass landing joins the Cube
 * 22 m below to the Sphere above, through the collar's iris and, below, the Cube's ceiling iris; beside the landing's
 * door The Starry Night stands on a glass stele. Its design sheet is Docs/Stages/atrium.md.
 *
 * Its frame: origin on the Élan axis on the marble's face; x east, y south, z up, true north. It declares, in that frame
 * and in metres:
 * - **ports** `atrium.west` (doorway.4000x4500.square on the base's face R 14.800, meeting hall-of-light.east), and the
 *   vertical ports `atrium.down` (the slab's soffit, -0.500, facing down, meeting cube.up) and `atrium.up` (the collar's
 *   top, 7.715, facing up, meeting sphere.down), class iris.4600, whose wells it serves;
 * - **anchors** `atrium.car`, `atrium.car.mast.head`, the car's and the landing's leaves and the collar iris's seven
 *   blades (Body), `atrium.call` and `atrium.car.panel` (Use), `atrium.stele` and the bays `atrium.bay.a`…`k` (Wall);
 * - **the enclosure** `atrium.hall`, the Glazed opening `atrium.drum`, and the housings `atrium.cove` and
 *   `atrium.collar.pocket`.
 */
UCLASS()
class UAtriumRoom : public USiteRoom
{
	GENERATED_BODY()

public:
	virtual FMuseeKey StageKey() const override;
	virtual void Describe(FSiteDescription& Out) const override;
};
