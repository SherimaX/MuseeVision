#pragma once

#include "CoreMinimal.h"
#include "SiteRoom.h"

#include "SphereRoom.generated.h"

/**
 * The Sphere (D-012), the Future's stage: a ball 28 m across sitting in the Atrium like a ball in a cup, its upper half
 * the Élan's pearl dome with a gilt ball on top. Inside, the building is gone: the real sky over the museum's city is
 * shown all round on the image surface, below the feet too. The Atrium's glass car climbs through a pearl neck (the
 * airlock, sealed above by the neck iris), then on the Sphere's gilt mast to a stone platform at the exact centre.
 * Its design sheet is Docs/Stages/sphere.md.
 *
 * Its frame is the Atrium's: origin on the Élan axis at the museum's floor datum; x east, y south, z up, true north. The
 * Sphere's centre is (0, 0, 22.000). It declares, in that frame and in metres:
 * - **port** `sphere.down` (iris.4600, a vertical port on the plane z 7.715 facing down, meeting atrium.up), which
 *   receives the Atrium's well;
 * - **anchors** the mast's head and foot, its four bearing dogs and the fourteen blades of the neck and platform irises
 *   (Body), `sphere.call` (Use) and `sphere.image.mount` (Mount, the image luminaire at the centre);
 * - **enclosures** `sphere.interior` and `sphere.neck`, and the Housings of the two irises' pockets and the dogs.
 */
UCLASS()
class USphereRoom : public USiteRoom
{
	GENERATED_BODY()

public:
	virtual FMuseeKey StageKey() const override;
	virtual void Describe(FSiteDescription& Out) const override;
};
