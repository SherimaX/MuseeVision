#pragma once

#include "CoreMinimal.h"
#include "SiteRoom.h"

#include "ReserveRoom.generated.h"

/**
 * The Reserve (D-020), the museum's picture store: a brick undercroft 60 m long under the Salon, reached only by the
 * Nymphéas oval's stair when the golden iris has raised the pond. Groin vaults of Burgundy brick spring from travertine
 * imposts over a nave and two aisles; in the aisles stand 36 brass racks hung from overhead tracks, each drawn out into
 * the nave by its pull, one at a time, with the works hung on its woven mesh; at the east end an oak easel holds Le Pont
 * de l'Europe, and the Degas pastels lie on the plan chests. Its design sheet is Docs/Stages/reserve.md.
 *
 * Its frame: origin on the axis midway between the end walls' inner faces, on the floor; x east, y south, z up, true
 * north. It declares, in that frame and in metres:
 * - **port** `reserve.stair` (stairwell.straight.2400x3200.arched, the west door at x -30.60, meeting nympheas.stair),
 *   which receives the oval's well;
 * - **anchors** the racks `reserve.screen-<name>` (Body) and their pulls `reserve.screen-<name>.pull` (Use), the racks'
 *   faces and the easel (Wall), the two south chests' felts (Mount);
 * - **enclosure** `reserve.undercroft`, whose one opening is the port.
 */
UCLASS()
class UReserveRoom : public USiteRoom
{
	GENERATED_BODY()

public:
	virtual FMuseeKey StageKey() const override;
	virtual void Describe(FSiteDescription& Out) const override;
};
