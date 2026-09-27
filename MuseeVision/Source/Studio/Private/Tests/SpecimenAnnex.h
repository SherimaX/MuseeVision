#pragma once

#include "CoreMinimal.h"
#include "SiteRoom.h"

#include "SpecimenAnnex.generated.h"

/**
 * Studio's second specimen stage, `specimen-annex`: a wall with the museum's standard arched door over a floor slab, its
 * port facing east to meet the specimen room's west door. The joined test plan mates the two, so the site make draws
 * the joint between them; no other plan names it.
 */
UCLASS()
class USpecimenAnnex : public USiteRoom
{
	GENERATED_BODY()

public:
	virtual FMuseeKey StageKey() const override;
	virtual void Describe(FSiteDescription& Out) const override;
};
