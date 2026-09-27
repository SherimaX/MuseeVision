#include "GroundsParts.h"

/**
 * The terrain (§3.2): a flat solid of topsoil over the whole footprint, z -0.330 to -0.030 (0.30 of it, as the Reserve's
 * sheet expects under it), made as one Nanite mesh and no Landscape; its top takes Nature's lawn. The sheet cuts it by
 * rule: the footprint less every received building's volume and less every piece of the Grounds set into it. Site hands
 * a Ground stage no received outlines and Form has no cut of one solid by others, so the soil is drawn over the whole
 * footprint, pierced only where water would show it: the canal and the round basin, down to their bluestone floors.
 */
namespace Musee::Grounds
{
	void DrawTerrain(TArray<FFormDrawing>& Out)
	{
		const double Half = Plan::FootprintHalf;
		const TArray<FFormProfile> Basins = {Rectangle({Plan::CanalWestX, Plan::CanalNorthY}, {Plan::CanalEastX, Plan::CanalSouthY}),
			Circle(At(Plan::RondPoint), Plan::BasinOuterRadius)};
		Out.Add(Laid(Pieces::Terrain, TEXT("terrain"), Rectangle({-Half, -Half}, {Half, Half}), Basins, Plan::SoilBottom, Plan::LawnTop,
			&Musee::Collision::Walkable));
	}
}
