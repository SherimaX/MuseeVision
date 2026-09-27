#include "SalonRoom.h"

#include "SalonParts.h"
#include "SalonPlan.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SalonRoom)

FMuseeKey USalonRoom::StageKey() const
{
	return FMuseeKey(Musee::Salon::Plan::Stage);
}

void USalonRoom::Describe(FSiteDescription& Out) const
{
	using namespace Musee::Salon;
	Out.Stage = StageKey();
	Out.DisplayName = NSLOCTEXT("MuseeSalon", "DisplayName", "Salon Impression");
	DescribeSite(Out);
	DrawWalls(Out.Pieces);
	DrawPiers(Out.Pieces);
	DrawMouldings(Out.Pieces);
	DrawVault(Out.Pieces);
	DrawRosettes(Out.Pieces);
	DrawLanterns(Out.Pieces);
	DrawRoof(Out.Pieces);
	DrawCabinet(Out.Pieces);
	DrawFloors(Out.Pieces);
	DrawFittings(Out.Pieces);
}
