#include "SphereRoom.h"

#include "SphereParts.h"
#include "SpherePlan.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SphereRoom)

FMuseeKey USphereRoom::StageKey() const
{
	return FMuseeKey(Musee::Sphere::Plan::Stage);
}

void USphereRoom::Describe(FSiteDescription& Out) const
{
	using namespace Musee::Sphere;
	Out.Stage = StageKey();
	Out.DisplayName = NSLOCTEXT("MuseeSphere", "DisplayName", "The Sphere");
	DescribeSite(Out);
	DrawShells(Out.Pieces);
	DrawNeck(Out.Pieces);
	DrawPlatform(Out.Pieces);
	DrawMast(Out.Pieces);
}
