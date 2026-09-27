#include "ChenghuaiPlanHalls.h"
#include "ChenghuaiTrades.h"

/**
 * The building table drawn (§3.1): the house's halls with their east twins, the galleries round the inner court, the
 * 垂花门's two roofs, and the garden's covered walk, flower hall and water pavilion, each by the one composition.
 */
namespace Musee::Chenghuai
{
	TArray<FHallSpec> Buildings()
	{
		using namespace Plan;
		const FHallSpec Side = SideHall();
		const FHallSpec Ear = EarRoom();
		TArray<FHallSpec> All = {FrontRow(), GateHall(), Side, MirroredAbout(Side, CourtAxis, TEXT("changnan-hall"), TEXT("changnan")), MainHall(),
			Ear, MirroredAbout(Ear, CourtAxis, TEXT("tingyun-room"), TEXT("tingyun")), RearRow()};
		for (const FHallSpec& Gallery : {GalleryG1(), GalleryG2(), GalleryG3(), GalleryG4()})
		{
			All.Add(Gallery);
			All.Add(MirroredAbout(Gallery, CourtAxis, Gallery.Name.Replace(TEXT(".west"), TEXT(".east")), FString()));
		}
		All.Append({ChuihuaRoof(true), ChuihuaRoof(false), CoveredWalk(), FlowerHall(), WaterPavilion()});
		return All;
	}

	void DrawBuildings(TArray<FFormDrawing>& Out)
	{
		for (const FHallSpec& Spec : Buildings())
		{
			DrawHall(Out, Spec);
		}
	}
}
