#include "RotundaRoom.h"

#include "RotundaParts.h"
#include "RotundaPlan.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RotundaRoom)

FMuseeKey URotundaRoom::StageKey() const
{
	return FMuseeKey(Musee::Rotunda::Plan::Stage);
}

void URotundaRoom::Describe(FSiteDescription& Out) const
{
	using namespace Musee::Rotunda;
	Out.Stage = StageKey();
	Out.DisplayName = NSLOCTEXT("MuseeRotunda", "DisplayName", "The Rotunda");
	DescribeSite(Out);
	DrawDrum(Out.Pieces);
	DrawOrder(Out.Pieces);
	DrawDome(Out.Pieces);
	DrawEye(Out.Pieces);
	DrawFloors(Out.Pieces);
	DrawSunClock(Out.Pieces);
	DrawShaft(Out.Pieces);
	DrawStair(Out.Pieces);
}
