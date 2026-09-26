#pragma once

#include "CoreMinimal.h"
#include "Chenghuai/ChenghuaiGeometry.h"

/**
 * The garden's stone (Garden board): the rockery (假山) of stacked bluestone with the path that climbs it to the
 * hexagonal pavilion (亭 · 見山) and comes down the other side, and the rocks that line the pond (湖石驳岸).
 *
 * The rockery is a field (height over the plan, stacked in courses of stone 0.3 m deep, each stone its own block and
 * tilt) meshed with surface nets; the path is cut into it as a ledge with steps of 15 cm. The pavilion stands on the
 * summit (2.6 m): six chestnut columns, lintels and hanging fretwork, seats with goose-neck backs (美人靠), a pyramidal
 * roof (攒尖) of butterfly tiles on six hips that sweep up at the corners (Suzhou's 发戗), a finial.
 */
namespace ChenghuaiRockery
{
	using ChenghuaiBuild::FChMeshes;
	using ChenghuaiKit::FPart;

	/** Rocks along a pond's edge: irregular stacked blocks from under the water to a little over the ground. */
	void EdgeRocks(FPart& Part, const TArray<FVector2D>& Edge, double Ground, double Water);

	/** The rockery, its path, guards and the pavilion on its summit. */
	void BuildRockery(FChMeshes& Out);

	/** The rockery's height at a plan point (0 off it): for placing plants and the visitor's checks. */
	double RockeryHeight(const FVector2D& P);
}
