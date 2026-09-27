#include "NympheasRoom.h"

#include "NympheasParts.h"
#include "NympheasPlan.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(NympheasRoom)

FMuseeKey UNympheasRoom::StageKey() const
{
	return FMuseeKey(Musee::Nympheas::Plan::Stage);
}

void UNympheasRoom::Describe(FSiteDescription& Out) const
{
	using namespace Musee::Nympheas;
	Out.Stage = StageKey();
	Out.DisplayName = NSLOCTEXT("MuseeNympheas", "DisplayName", "The Nymphéas Oval");
	DescribeSite(Out);
	DrawShell(Out.Pieces);
	DrawFloors(Out.Pieces);
	DrawWell(Out.Pieces);
	DrawPond(Out.Pieces);
	DrawFittings(Out.Pieces);
}
