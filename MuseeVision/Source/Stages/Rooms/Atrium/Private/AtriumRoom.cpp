#include "AtriumRoom.h"

#include "AtriumParts.h"
#include "AtriumPlan.h"
#include "SiteDescription.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AtriumRoom)

FMuseeKey UAtriumRoom::StageKey() const
{
	return FMuseeKey(Musee::Atrium::Plan::Stage);
}

void UAtriumRoom::Describe(FSiteDescription& Out) const
{
	using namespace Musee::Atrium;
	Out.Stage = StageKey();
	Out.DisplayName = NSLOCTEXT("MuseeAtrium", "DisplayName", "The Atrium");
	DrawMasonry(Out.Pieces);
	DrawFloor(Out.Pieces);
	DrawGlazing(Out.Pieces);
	DrawSteel(Out.Pieces);
	DrawCanopyGrid(Out.Pieces);
	DrawLift(Out.Pieces);
	DrawCar(Out.Pieces);
	DrawStele(Out.Pieces);
	// The site's list of the hall's bounding pieces reads the pieces, so it comes after them.
	DescribeSite(Out);
}
