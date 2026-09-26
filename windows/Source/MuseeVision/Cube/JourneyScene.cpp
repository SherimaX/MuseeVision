#include "Cube/JourneyScene.h"

#include "Cube/ElanJourney.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

void UJourneyScene::Setup(AElanJourney* InDirector)
{
	Director = InDirector;
}

UObject* UJourneyScene::DirectorObject() const
{
	return Director.Get();
}

FVector UJourneyScene::ToWorld(const FVector& Metres) const
{
	return Director ? Director->GetActorTransform().TransformPosition(Metres * 100.0) : Metres * 100.0;
}

USceneComponent* UJourneyScene::Register(USceneComponent* Component, bool bForJourney)
{
	if (!Component || !Director) { return Component; }
	Component->SetMobility(EComponentMobility::Movable);
	Component->SetupAttachment(Director->GetRootComponent());
	Component->RegisterComponent();
	(bForJourney ? JourneyParts : PlaceParts).Add(Component);
	return Component;
}

UStaticMeshComponent* UJourneyScene::MeshAt(UStaticMesh* Mesh, const FTransform& MetresTransform, UMaterialInterface* Material, bool bForJourney)
{
	if (!Mesh) { return nullptr; }
	UStaticMeshComponent* C = Make<UStaticMeshComponent>(TEXT("JourneyMesh"), bForJourney);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetStaticMesh(Mesh);
	if (Material) { C->SetMaterial(0, Material); }
	FTransform T = MetresTransform;
	T.SetTranslation(MetresTransform.GetTranslation() * 100.0);
	C->SetRelativeTransform(T);
	return C;
}

UInstancedStaticMeshComponent* UJourneyScene::Instances(UStaticMesh* Mesh, UMaterialInterface* Material, bool bForJourney)
{
	if (!Mesh) { return nullptr; }
	UInstancedStaticMeshComponent* C = Make<UInstancedStaticMeshComponent>(TEXT("JourneyInstances"), bForJourney);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetStaticMesh(Mesh);
	if (Material) { C->SetMaterial(0, Material); }
	return C;
}

void UJourneyScene::Forget(USceneComponent* Component)
{
	PlaceParts.Remove(Component);
	JourneyParts.Remove(Component);
}

void UJourneyScene::ClosePlace()
{
	for (USceneComponent* C : PlaceParts)
	{
		if (C) { C->DestroyComponent(); }
	}
	PlaceParts.Reset();
}

void UJourneyScene::Teardown()
{
	ClosePlace();
	for (USceneComponent* C : JourneyParts)
	{
		if (C) { C->DestroyComponent(); }
	}
	JourneyParts.Reset();
}
