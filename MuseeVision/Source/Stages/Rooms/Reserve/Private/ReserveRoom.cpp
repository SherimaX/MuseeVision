#include "ReserveRoom.h"

#include "ReserveParts.h"
#include "ReservePlan.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ReserveRoom)

FMuseeKey UReserveRoom::StageKey() const
{
	return FMuseeKey(Musee::Reserve::Plan::Stage);
}

void UReserveRoom::Describe(FSiteDescription& Out) const
{
	using namespace Musee::Reserve;
	Out.Stage = StageKey();
	Out.DisplayName = NSLOCTEXT("MuseeReserve", "DisplayName", "The Reserve");
	DescribeSite(Out);
	DrawShell(Out.Pieces);
	DrawFloors(Out.Pieces);
	DrawPiers(Out.Pieces);
	DrawVaults(Out.Pieces);
	DrawTracks(Out.Pieces);
	DrawRacks(Out.Pieces);
	DrawLuminaires(Out.Pieces);
	DrawFittings(Out.Pieces);
}
