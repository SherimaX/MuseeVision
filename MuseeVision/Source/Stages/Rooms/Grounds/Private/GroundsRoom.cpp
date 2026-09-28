#include "GroundsRoom.h"

#include "GroundsParts.h"
#include "GroundsPlan.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GroundsRoom)

FMuseeKey UGroundsRoom::StageKey() const
{
	return FMuseeKey(Musee::Grounds::Plan::Stage);
}

void UGroundsRoom::Describe(FSiteDescription& Out) const
{
	using namespace Musee::Grounds;
	Out.Stage = StageKey();
	Out.DisplayName = NSLOCTEXT("MuseeGrounds", "DisplayName", "The Grounds");
	DrawPaving(Out.Pieces);
	DrawGravel(Out.Pieces);
	DrawStrip(Out.Pieces);
	DrawKerbs(Out.Pieces);
	DrawBasins(Out.Pieces);
	DrawLamps(Out.Pieces);
	// The terrain after what is set into it: its soil gives way to each of them.
	DrawTerrain(Out.Pieces, Out.ReceivedPrisms);
	// The site's declarations come last: the enclosure names the terrain just drawn.
	DescribeSite(Out);
}
