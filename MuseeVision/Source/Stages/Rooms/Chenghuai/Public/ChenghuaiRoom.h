#pragma once

#include "CoreMinimal.h"
#include "SiteRoom.h"

#include "ChenghuaiRoom.generated.h"

/**
 * 澄懷 Chenghuai, on the Rotunda's north door (D-001): a three-court Beijing house (三进四合院) with a Suzhou garden (臥遊)
 * beside it to the east. Grey brick rubbed or jointed by rank, white stone axe-dressed, red-lacquered and painted timber
 * under grey clay tiles; the garden white-walled, its timber chestnut, round a pond, a rockery and a zigzag bridge. One
 * door in and out, one loop of sixteen stops. Its design sheet is Docs/Stages/chenghuai.md.
 *
 * Its frame: origin on the gate's axis (the Rotunda's north-south axis) on the site's south outer face, at the Rotunda's
 * floor; x east, y south, z up, true north. It declares, in that frame and in metres:
 * - its one **port** `chenghuai.south` (doorway.4000x6500.arched on the drum's face at (0, 5.80), meeting rotunda.north);
 * - the **anchors** of its hang (the scroll and ceramic cases, the handscroll beds, the screen wall's heart, the engraved
 *   stones, the furniture), its planting (the pond, the fish bowl, the court beds, pits and tubs, the garden's trees,
 *   bamboo and ground) and the pavilion's four seats;
 * - fifteen **enclosures** (the porch and the garden outdoors, four courts, nine rooms), each room's paper lattice as a
 *   Glazed opening, each court's top and the doors between its spaces as Open openings.
 */
UCLASS()
class UChenghuaiRoom : public USiteRoom
{
	GENERATED_BODY()

public:
	virtual FMuseeKey StageKey() const override;
	virtual void Describe(FSiteDescription& Out) const override;
};
