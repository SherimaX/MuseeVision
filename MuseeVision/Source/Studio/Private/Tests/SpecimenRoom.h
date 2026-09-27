#pragma once

#include "CoreMinimal.h"
#include "SiteRoom.h"

#include "SpecimenRoom.generated.h"

/**
 * Studio's specimen stage, `specimen-room`: the smallest room that runs every step of a stage's make. Two walls meet
 * at a corner over a floor slab; the west wall has an arched door, whose port the site plan never mates, so Site closes
 * it with a blind; and a turned post rides on a body. It is made only by the tests (under /Game/Generated/Studio), and
 * the site plan never names it, so a make of the museum leaves it out.
 */
UCLASS()
class USpecimenRoom : public USiteRoom
{
	GENERATED_BODY()

public:
	virtual FMuseeKey StageKey() const override;
	virtual void Describe(FSiteDescription& Out) const override;
};
