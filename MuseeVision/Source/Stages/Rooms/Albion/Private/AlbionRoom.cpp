#include "AlbionRoom.h"

#include "AlbionParts.h"
#include "AlbionPlan.h"
#include "SiteDescription.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AlbionRoom)

FMuseeKey UAlbionRoom::StageKey() const
{
	return FMuseeKey(Musee::Albion::Plan::Stage);
}

void UAlbionRoom::Describe(FSiteDescription& Out) const
{
	using namespace Musee::Albion;
	Out.Stage = StageKey();
	Out.DisplayName = NSLOCTEXT("MuseeAlbion", "DisplayName", "Albion");
	DrawMasonry(Out.Pieces);
	DrawOutside(Out.Pieces);
	DrawPorch(Out.Pieces);
	DrawFloors(Out.Pieces);
	DrawColumns(Out.Pieces);
	DrawArcade(Out.Pieces);
	DrawVaults(Out.Pieces);
	DrawRoof(Out.Pieces);
	DrawScreens(Out.Pieces);
	DrawLancets(Out.Pieces);
	DrawRainwater(Out.Pieces);
	DrawFittings(Out.Pieces);
	// The site's declarations come last: the enclosure is bounded by the pieces just drawn.
	DescribeSite(Out);
}
