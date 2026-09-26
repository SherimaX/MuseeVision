#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "JourneyScene.generated.h"

class AElanJourney;
class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UPrimitiveComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * One journey's world (Cube/ElanJourney): made when the journey opens, its places built and torn down round the car
 * as the day goes on. The components belong to the director actor (so they tick, render and die with it); a scene
 * keeps the ones a place made and destroys them when the place closes.
 *
 * Positions: metres in the car's frame, the visitor's eyes at the origin (x east, y south, z up), turned to the place's
 * scene yaw by the director; ToWorld() puts them in the world (the Cube's centre).
 */
UCLASS(Abstract)
class MUSEEVISION_API UJourneyScene : public UObject
{
	GENERATED_BODY()

public:
	/** Load what the journey needs and make what lasts through it. */
	virtual void Setup(AElanJourney* InDirector);
	/** Build place Index (0 … 5). */
	virtual void OpenPlace(int32 Index) {}
	/** Each frame of a place: the place's local hour, how far through it (0 … 1), the veil (0 clear … 1 thick). */
	virtual void TickPlace(float DeltaSeconds, double LocalHour, float Fraction, float Veil) {}
	/** Tear the place down (its components). */
	virtual void ClosePlace();
	/** Tear everything down. */
	virtual void Teardown();
	/** The place's sky, before it is set (the sea: caustics on the sun, the light's colour at depth). */
	virtual void AdjustSky(struct FMuseeSkyOverride& Sky, int32 Place) {}
	/** Whether its assets are there (the scripts have built them). */
	virtual bool IsAvailable() const { return false; }

	/** The world position (cm) of a point in the car's frame (m), the eyes at the origin. */
	FVector ToWorld(const FVector& Metres) const;

protected:
	/** A component on the director, attached to its root (at the eyes); kept for the place, or for the journey. */
	template <class T>
	T* Make(const TCHAR* Name, bool bForJourney = false);
	UStaticMeshComponent* MeshAt(UStaticMesh* Mesh, const FTransform& MetresTransform, UMaterialInterface* Material = nullptr, bool bForJourney = false);
	UInstancedStaticMeshComponent* Instances(UStaticMesh* Mesh, UMaterialInterface* Material = nullptr, bool bForJourney = false);
	void Forget(USceneComponent* Component);

	UPROPERTY(Transient) TObjectPtr<AElanJourney> Director;
	UPROPERTY(Transient) TArray<TObjectPtr<USceneComponent>> PlaceParts;
	UPROPERTY(Transient) TArray<TObjectPtr<USceneComponent>> JourneyParts;

private:
	USceneComponent* Register(USceneComponent* Component, bool bForJourney);
	UObject* DirectorObject() const;
};

template <class T>
T* UJourneyScene::Make(const TCHAR* Name, bool bForJourney)
{
	UObject* Outer = DirectorObject();
	T* C = NewObject<T>(Outer, MakeUniqueObjectName(Outer, T::StaticClass(), FName(Name)));
	Register(C, bForJourney);
	return C;
}
