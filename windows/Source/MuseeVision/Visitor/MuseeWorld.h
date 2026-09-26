#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * The imported building. Scripts/import_wing.py tags every actor it places with
 * "musee.building" (and "musee.wing:<Wing>"), so the moving parts and the Sphere can find them.
 */
namespace MuseeWorld
{
	extern const FName BuildingTag;

	/** Hide or show the whole building (inside the Sphere it is gone). */
	void SetBuildingHidden(UWorld* World, bool bHidden);

	/** Actors tagged "part:<Tag>" (the USD's museevision:movingPart), e.g. "pond", "elevator_car". */
	void FindParts(UWorld* World, const FString& PartTag, TArray<AActor*>& Out);

	/** An actor by its USD prim path tag "prim:/Museum/…". */
	AActor* FindPrim(UWorld* World, const FString& PrimPath);

	/** Clicking Proxy uses Target (e.g. the landing glass calls the elevator). */
	void RegisterUse(AActor* Proxy, AActor* Target);
	/** What clicking Proxy uses, if anything. */
	AActor* UseTarget(const AActor* Proxy);
}
