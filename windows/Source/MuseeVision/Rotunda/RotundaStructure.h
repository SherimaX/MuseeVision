#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RotundaStructure.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

/**
 * The Rotunda's architecture, built from MuseePlan::Rotunda (the design first drawn in
 * Shared/Wings/Rotunda.swift, buildRotunda and buildPassage). Place it at the Rotunda's centre.
 *
 * A solid drum wall 1.2 m thick: inner and outer faces, a 150 mm shadow gap at the floor, jambs and
 * arched soffits through the four doors (running on as the passages west to the Salon and north to the
 * Sculpture Hall), four half-round niches with half-dome heads. Sixteen fluted pilasters with bases and
 * capitals; an entablature (architrave, frieze, cornice) at the springing; archivolts, imposts, niche
 * surrounds and sills. The dome: 5 rings × 28 stepped coffers, a stepped curb round the Ø 5 m eye, its
 * outer shell and the drum's top; in the eye, the steel lattice, its glass and the bronze node.
 *
 * Surfaces that meet share their vertices, or one sinks a centimetre into the other, so no light gets
 * through a seam: the only view of the sky is through the eye's glass.
 */
UCLASS()
class MUSEEVISION_API ARotundaStructure : public AActor
{
	GENERATED_BODY()

public:
	ARotundaStructure();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** The drum wall, its reveals and niches, and the two passages: travertine, with collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Walls;

	/** Section 0: the pilasters. Section 1: entablature, archivolts, imposts, niche surrounds and sills. With collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Mouldings;

	/** Section 0: the coffered dome and the eye's curb, inside. Section 1: the drum's top and the dome's shell, outside. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Dome;

	/** Section 0: the steel lattice. Section 1: its glass. Section 2: the bronze node. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Oculus;

	/** The drum, the passages and the outside of the dome. UVs are in metres. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> WallMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> PilasterMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> MouldingMaterial;

	/** The dome and its coffers, inside. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> DomeMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> SteelMaterial;

	/** The eye's glass: thin, clear and translucent. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> GlassMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> NodeMaterial;

private:
	void Build();
};
