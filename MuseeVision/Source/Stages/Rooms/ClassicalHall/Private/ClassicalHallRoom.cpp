#include "ClassicalHallRoom.h"

#include "ClassicalHallParts.h"
#include "ClassicalHallPlan.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ClassicalHallRoom)

FMuseeKey UClassicalHallRoom::StageKey() const
{
	return FMuseeKey(Musee::ClassicalHall::Plan::Stage);
}

void UClassicalHallRoom::Describe(FSiteDescription& Out) const
{
	using namespace Musee::ClassicalHall;
	Out.Stage = StageKey();
	Out.DisplayName = NSLOCTEXT("MuseeClassicalHall", "DisplayName", "The Classical Hall");
	DrawWalls(Out.Pieces);
	DrawNiches(Out.Pieces);
	DrawFloors(Out.Pieces);
	DrawGalleryVault(Out.Pieces);
	DrawTribuneVaults(Out.Pieces);
	DrawOrder(Out.Pieces);
	DrawEntablatures(Out.Pieces);
	DrawMouldings(Out.Pieces);
	DrawCarriers(Out.Pieces);
	// The site's declarations last: the enclosure lists the shell's pieces the trades have drawn.
	DescribeSite(Out);
}
