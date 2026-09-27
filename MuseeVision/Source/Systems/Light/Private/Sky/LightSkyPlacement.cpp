#include "Sky/LightSkyPlacement.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "LightSky.h"

bool ULightSkyPlacement::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	return World && World->IsGameWorld();
}

void ULightSkyPlacement::OnWorldBeginPlay(UWorld& World)
{
	Super::OnWorldBeginPlay(World);
	if (TActorIterator<ALightSky>(&World)) { return; }
	FActorSpawnParameters Spawn;
	Spawn.Name = TEXT("Sky");
	Spawn.ObjectFlags = RF_Transient;
	World.SpawnActor<ALightSky>(Spawn);
}
