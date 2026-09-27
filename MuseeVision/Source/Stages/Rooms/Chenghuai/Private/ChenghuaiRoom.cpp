#include "ChenghuaiRoom.h"

#include "ChenghuaiPlan.h"
#include "ChenghuaiTrades.h"
#include "SiteDescription.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ChenghuaiRoom)

FMuseeKey UChenghuaiRoom::StageKey() const
{
	return FMuseeKey(Musee::Chenghuai::Plan::Stage);
}

void UChenghuaiRoom::Describe(FSiteDescription& Out) const
{
	using namespace Musee::Chenghuai;
	Out.Stage = StageKey();
	Out.DisplayName = NSLOCTEXT("MuseeChenghuai", "DisplayName", "澄懷 Chenghuai");
	DrawBuildings(Out.Pieces);
	DrawGates(Out.Pieces);
	DrawWalls(Out.Pieces);
	DrawCourts(Out.Pieces);
	DrawGarden(Out.Pieces);
	DrawRockery(Out.Pieces);
	DrawCases(Out.Pieces);
	DrawFurniture(Out.Pieces);
	// The site's declarations last: the enclosures name the pieces that bound them.
	DescribeSite(Out);
}
