#include "MotionControl.h"

#include "MotionMechanism.h"
#include "MuseeUnits.h"

UMotionControl::UMotionControl()
{
	InitSphereRadius(Musee::ToEngine(Musee::Meters(RadiusMeters)));
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	SetHiddenInGame(true);
	SetMobility(EComponentMobility::Movable);
}

void UMotionControl::Bind(UMotionMechanism* InMechanism, const FMuseeKey& InUseAnchor)
{
	BoundMechanism = InMechanism;
	UseAnchorKey = InUseAnchor;
}

FMuseeUse UMotionControl::Usage() const
{
	return BoundMechanism ? BoundMechanism->UsageAt(UseAnchorKey) : FMuseeUse();
}

void UMotionControl::Use()
{
	if (BoundMechanism)
	{
		BoundMechanism->UseAt(UseAnchorKey);
	}
}
