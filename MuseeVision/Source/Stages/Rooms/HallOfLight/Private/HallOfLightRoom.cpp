#include "HallOfLightRoom.h"

#include "HallOfLightParts.h"
#include "HallOfLightPlan.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(HallOfLightRoom)

FMuseeKey UHallOfLightRoom::StageKey() const
{
	return FMuseeKey(Musee::HallOfLight::Plan::Stage);
}

void UHallOfLightRoom::Describe(FSiteDescription& Out) const
{
	using namespace Musee::HallOfLight;
	Out.Stage = StageKey();
	Out.DisplayName = NSLOCTEXT("MuseeHallOfLight", "DisplayName", "The Hall of Light");
	DrawMasonry(Out.Pieces);
	DrawLinings(Out.Pieces);
	DrawWalls(Out.Pieces);
	DrawVault(Out.Pieces);
	DrawBronze(Out.Pieces);
	// The site's declarations come last: the hall's enclosure names the pieces just drawn.
	DescribeSite(Out);
}
