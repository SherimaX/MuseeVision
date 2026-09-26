#include "Visitor/MuseeWorld.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

const FName MuseeWorld::BuildingTag(TEXT("musee.building"));

void MuseeWorld::SetBuildingHidden(UWorld* World, bool bHidden)
{
	if (!World) { return; }
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->Tags.Contains(BuildingTag)) { It->SetActorHiddenInGame(bHidden); }
	}
}

void MuseeWorld::FindParts(UWorld* World, const FString& PartTag, TArray<AActor*>& Out)
{
	if (!World) { return; }
	const FName Tag(*(TEXT("part:") + PartTag));
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->Tags.Contains(Tag)) { Out.Add(*It); }
	}
}

AActor* MuseeWorld::FindPrim(UWorld* World, const FString& PrimPath)
{
	if (!World) { return nullptr; }
	const FName Tag(*(TEXT("prim:") + PrimPath));
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (It->Tags.Contains(Tag)) { return *It; }
	}
	return nullptr;
}

namespace
{
	TMap<TWeakObjectPtr<const AActor>, TWeakObjectPtr<AActor>> GUses;
}

void MuseeWorld::RegisterUse(AActor* Proxy, AActor* Target)
{
	if (Proxy && Target) { GUses.Add(Proxy, Target); }
}

AActor* MuseeWorld::UseTarget(const AActor* Proxy)
{
	const TWeakObjectPtr<AActor>* Found = GUses.Find(Proxy);
	return Found ? Found->Get() : nullptr;
}
