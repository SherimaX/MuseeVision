#include "GroundsParts.h"

/**
 * The terrain (§3.2): a flat solid of topsoil over the whole footprint, z -0.330 to -0.030 (0.30 of it, as the Reserve's
 * sheet expects under it), made as one Nanite mesh and no Landscape; its top takes Nature's lawn. The sheet cuts it by
 * rule: the footprint less every received building's volume and less every piece of the Grounds set into it. The soil
 * gives way to every building the site plan stands on the grounds whose volume passes through it (a prism reaching
 * between its bottom and its top: the Rotunda's room and its well, a hall's gardens, a pond), so it never fills their
 * floors or their wells, and to the canal and the round basin down to their bluestone floors; Form joins the outlines
 * where the buildings stand side by side. The pieces set into it (the walks, the parvis, the kerbs) wait for a laid
 * surface cut round what is set into it.
 */
namespace Musee::Grounds
{
	void DrawTerrain(TArray<FFormDrawing>& Out, TConstArrayView<FSitePrism> Received)
	{
		const double Half = Plan::FootprintHalf;
		TArray<FFormProfile> Holes = {Rectangle({Plan::CanalWestX, Plan::CanalNorthY}, {Plan::CanalEastX, Plan::CanalSouthY}),
			Circle(At(Plan::RondPoint), Plan::BasinOuterRadius)};
		for (const FSitePrism& Building : Received)
		{
			if (Building.BottomMeters < Plan::LawnTop && Building.TopMeters > Plan::SoilBottom && Building.OutlineMeters.Num() >= 3)
			{
				Holes.Add(Polygon(Building.OutlineMeters));
			}
		}
		Out.Add(Laid(Pieces::Terrain, TEXT("terrain"), Rectangle({-Half, -Half}, {Half, Half}), MoveTemp(Holes), Plan::SoilBottom, Plan::LawnTop,
			&Musee::Collision::Walkable));
	}
}
