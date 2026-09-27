#include "CubeRoom.h"

#include "CubeParts.h"
#include "CubePlan.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CubeRoom)

FMuseeKey UCubeRoom::StageKey() const
{
	return FMuseeKey(Musee::Cube::Plan::Stage);
}

void UCubeRoom::Describe(FSiteDescription& Out) const
{
	using namespace Musee::Cube;
	Out.Stage = StageKey();
	Out.DisplayName = NSLOCTEXT("MuseeCube", "DisplayName", "The Cube");
	DrawBox(Out.Pieces);
	DrawPanels(Out.Pieces);
	DrawShaft(Out.Pieces);
	DrawRails(Out.Pieces);
	DrawCeiling(Out.Pieces);
	DrawIris(Out.Pieces);
	DrawGround(Out.Pieces);
	// The site's declarations come last: the enclosures name the pieces just drawn.
	DescribeSite(Out);
}
