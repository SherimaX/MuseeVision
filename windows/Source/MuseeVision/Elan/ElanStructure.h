#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ElanStructure.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class URectLightComponent;

/**
 * The Atrium's drum, the Sphere, its glazed underside and the ribs (Shared/Wings/Elan.swift,
 * buildAtrium), built natively from ElanPlan's numbers. The Sphere (R 14 m, centre 22 m up) sits in
 * the Atrium like a ball in a cup: its equator on the top of the misty-glass drum, its underside the
 * Atrium's ceiling, down to the opening over the car at the south pole.
 *
 * The underside is the room's light, like the British Museum's Great Court roof: a bright glazed
 * canopy (its own material slot, brighter than the drum) under a triangulated grid of pearl-white
 * steel, two rings of area lights just under it (the laylights: MuseeSky scales them with the
 * daylight, as it does M_MistyGlass), and twelve pearl ribs of constant chamfered section, in bronze shoes
 * with a bronze line up each face, from the floor up the drum, arching under the Sphere to a
 * ring round the opening. The opening itself is the elevator's: its neck (Elan/ElanSphereTop) fills it
 * and seals it with irises; this actor's old iris is retired (kept, empty and hidden).
 *
 * The geometry is rebuilt at BeginPlay as well as on construction, so the saved map never shows an
 * older build. The imported Élan layer predates this design; Scripts/import_wing.py removes its old
 * vault (Misty_glass, Atrium_ribs, Iris).
 */
UCLASS()
class MUSEEVISION_API AElanStructure : public AActor
{
	GENERATED_BODY()

public:
	AElanStructure();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	/** Retired (the elevator's neck seals the opening now): keeps the old iris hidden. */
	void SetIrisOpen(bool bOpen);

	/** How deep the ring round the opening hangs under the Sphere (m): the elevator's tube meets it. */
	static constexpr double RingDepth = 0.58;

	/** The drum (section 0), the roof, i.e. the Sphere's underside (1), and the Sphere's top (2). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Glass;

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Ribs;

	/** The roof's steel grid, 5 cm under the glass. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Lattice;

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Iris;

	/** The laylights under the roof: two rings of twelve, at r ≈ 6 m and r ≈ 11 m. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<URectLightComponent>> RoofLights;

	/** Twelve concealed uplights on the coping (6 m), one grazing each rib's inner face, so the ribs read
	 *  pearl-white against the glowing roof (rendering 07) instead of as silhouettes. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<class USpotLightComponent>> RibUplights;

	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float RibUplightCandela = 240000.f;

	/** Misty glass: a fine white frit lit by the sky behind it (M_MistyGlass: Luminance, NightFloor). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> GlassMaterial;

	/** The roof's glass (the same material by default, brighter). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> RoofMaterial;

	/** The ribs, the ring and the roof's grid: pearl-white painted steel. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> RibMaterial;

	/** Retired with the old iris. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> IrisMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> BronzeMaterial;

	/** Full daylight luminance (cd/m²) of the roof and of the drum; both × max(Daylight, NightFloor). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float RoofNits = 900.f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float DrumNits = 250.f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float NightFloor = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	bool bRoofLightShadows = true;

private:
	void Build();
	void ApplyMaterials(bool bInstances);
	void PlaceRoofLights();
	void PlaceRibUplights();
	void AddLaylightTags();
	/** Turn off the older laylights inside the drum (Scripts/relight.py's ELAN_DRUM), if the map still has them. */
	void RetireDrumLights();

	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> RoofGlow;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> DrumGlow;
};
