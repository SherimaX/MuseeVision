#pragma once

#include "CoreMinimal.h"
#include "SiteRoom.h"

#include "SpecimenRoom.generated.h"

/**
 * Studio's specimen stage, `specimen-room`: the smallest room that runs every step of a stage's make. Two walls meet
 * at a corner over a floor slab; the west wall has an arched door, whose port the test plan leaves unmated, so Site closes
 * it with a blind, or mates with the specimen annex, so Site joins them; a turned post rides on a body, and its cap on a
 * body riding that one; and two identical studs, which stop only the body, are cast once. It is made only by the tests
 * (under /Game/Generated/Specimens), and the site plan never names it, so a make of the museum leaves it out.
 */
UCLASS()
class USpecimenRoom : public USiteRoom
{
	GENERATED_BODY()

public:
	virtual FMuseeKey StageKey() const override;
	virtual void Describe(FSiteDescription& Out) const override;
};
