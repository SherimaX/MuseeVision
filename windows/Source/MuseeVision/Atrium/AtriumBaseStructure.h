#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AtriumBaseStructure.generated.h"

class UBoxComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UProceduralMeshComponent;
class URectLightComponent;

/**
 * The Atrium's level 0 (Élan; Shared/Wings/Elan.swift, buildAtrium; rendering plan/renderings/07),
 * built natively from MuseePlan::Elan and MuseePlan::Atrium. Place it at the world origin: the
 * geometry is in plan coordinates (the Atrium's centre is x 54, y 0). AElanStructure keeps the drum's
 * misty glass, the ribs, the roof and the iris; AElanElevator the car, the landings and the tube.
 *
 * - The floor: pale polished marble in concentric rings (joints at r 4.58, 6.11, 7.63, 9.15, 10.68 and
 *   12.2 m) of radial slabs (12, 12, 24, 24, 36, 36, 48, 36 round, a joint on every pilaster's line),
 *   6 mm joints of the same marble a shade darker. It runs from 1 cm inside the Square's bronze lining
 *   of the car's shaft (r 2.29: under the landing's bronze shoe and sill) to the wall, into the LED
 *   cove, and through the west door to the export's 96-gon (r 14.3), which the Hall of Light's floor
 *   meets vertex for vertex. The bronze waiting ring (r 2.94–3.06) lies flush in it.
 * - The stone base: a 0.8 m wall of honed travertine from the floor to 6 m, with the 4 m door to the Hall
 *   of Light (square-headed at 4.5 m, its reveal lined in the same stone). At its foot a warm LED cove,
 *   9 cm deep and 14 cm high, round the whole drum but the door: a slim diffuser low on its back and a
 *   rect light behind each third of a bay. At 5.72 m a coping band 10 cm proud wraps round twelve
 *   pilasters (1.6 m wide, 0.64 m deep, on 30 cm plinths) and turns into their capitals; its top at
 *   exactly MuseePlan::Elan::BaseHeight (6 m) runs out to the wall's outer face, and the drum's misty
 *   glass (r 14) and the ribs (r 13.42–14) stand on it: no gap, and no face coplanar with theirs.
 * - Bronze: eleven bay letters A … K (Didot-like, 30 cm, cast 2 mm proud) at r 13.1, reading from the
 *   room towards each bay; the waiting mark (a half ring and a dot) west of the ring.
 * - The Starry Night's stele: a 6 cm low-iron glass slab 1.5 m wide, 2.2 m high (40 cm of clear glass over the frame), set 2 cm into a
 *   brushed brass foot (0.4 × 1.7 × 0.08 m, eased edges), like the Salon's for Impression, Sunrise.
 *   The painting (/Museum/Elan/starry_night, a work) stays where it hangs, on the glass's west face.
 *
 * Watertight: faces that meet share their vertices (the floor's rings, the cove, the wall's rows, the
 * coping's top and the outer face), or one runs 2 mm on into the solid it meets (pilasters into the
 * wall, the band's soffit, the jambs below the floor). UV0 is in metres: plan (x, y) on the floor and on
 * horizontal faces, (along, down) on walls. Collision: the floor (walkable), the walls, pilasters and
 * plinths as themselves; the stele as a box that blocks the visitor only (the look trace reaches the
 * painting).
 *
 * Light, at CoveKelvin (neutral 3800 K): the cove only (33 small rect lights, no shadows, facing out of
 * the cove onto the floor), none by the stele or in the bays. The geometry is rebuilt at BeginPlay as
 * well as on construction, unless it has been baked into static meshes (Geometry/MuseeBake.h; the floor,
 * the diffuser and the stele's foot stay procedural: their materials are made at play). At BeginPlay the
 * imported pieces it replaces are also retired (hidden, no collision, off the musee.building list),
 * whether or not Scripts/native.py has done so.
 */
UCLASS()
class MUSEEVISION_API AAtriumBaseStructure : public AActor
{
	GENERATED_BODY()

public:
	AAtriumBaseStructure();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	/**
	 * USD paths of the imported prims this actor replaces: the Atrium's floor, stone base, coping and
	 * pilasters, the bronze ring (and mark), the eleven bay letters and the Starry Night's stele. The
	 * painting (/Museum/Elan/starry_night) stays.
	 */
	UFUNCTION(BlueprintPure, Category = "Musee|Atrium")
	static TArray<FString> GetReplacedImportPrims();

	/** The bay letters' centres, A … K (world cm, on the floor). */
	UFUNCTION(BlueprintPure, Category = "Musee|Atrium")
	TArray<FVector> GetBayLetterPositions() const;

	/** The stele's glass (world cm). */
	UFUNCTION(BlueprintPure, Category = "Musee|Atrium")
	FBox GetSteleGlassBounds() const;

	/** The cove lights' centres (world cm), for checks. */
	UFUNCTION(BlueprintPure, Category = "Musee|Atrium")
	TArray<FVector> GetCoveLightPositions() const;

	/**
	 * Section 0: the marble slabs (and the lip round the car's shaft). 1: their joints. 2: the bronze waiting
	 * ring. Walkable. Left procedural by the bake (MuseeBake::NoBake): its materials are made at play.
	 */
	/** The twelve stone pilasters round the base. Off: the Élan's bone ribs (AElanStructure) come down to the
	 * floor in their place, and the coping band runs round unbroken. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	bool bPilasters = false;

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Floor;

	/** The stone base: wall faces, cove, door reveal, band and coping, pilasters, plinths. With collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Walls;

	/** The bay letters and the waiting mark (bronze, 2 mm proud). No collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Inlays;

	/** The cove's LED diffuser (its glow is made at play: left procedural by the bake). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> CoveLed;

	/** The stele's glass. No collision (SteleBlock). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Stele;

	/** The stele's brass foot (its brass is made at play: left procedural by the bake). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> SteleFoot;

	/** Keeps the visitor out of the stele; the look and camera traces pass (to the painting). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UBoxComponent> SteleBlock;

	/** The cove's lights: three to a bay, none in the door's bay. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<URectLightComponent>> CoveLights;

	/** Polished marble (the export's). Its world-grid joints are turned off at play: the rings are the joints. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> FloorMaterial;

	/** The stone base, coping and pilasters: honed travertine, the export's warm white. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> StoneMaterial;

	/** The waiting ring, the mark and the letters: the landing's gilt bronze. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> BronzeMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> GlassMaterial;

	/** The stele's foot: M_Brass_Brushed (materials.py, saved); or a dynamic instance of M_Metal with BrassColour and BrassRoughness. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> BrassMaterial;

	/** The floor's joints: M_Elan_Mirror_Joint (materials.py, saved: the marble a shade darker, duller). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> JointMaterial;

	/** The LED diffuser: M_Daylit held at full glow on a white image, CoveKelvin at CoveNits. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> GlowMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	FLinearColor BrassColour = FLinearColor(0.72f, 0.52f, 0.25f);

	UPROPERTY(EditAnywhere, Category = "Musee")
	float BrassRoughness = 0.34f;

	/** The joints' shade of the marble (its base colour times this). */
	UPROPERTY(EditAnywhere, Category = "Musee", meta = (ClampMin = "0.3", ClampMax = "1.0"))
	float JointShade = 0.8f;

	/** The cove's light: lumens per metre of wall, and colour. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float CoveLumensPerMetre = 160.f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float CoveKelvin = 3800.f;

	/** The diffuser's own glow (cd/m²). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float CoveNits = 600.f;

private:
	void Build();
	void ApplyMaterials(bool bInstances);
	/** The cove's lights and the stele's block (not geometry: placed whether or not the actor is baked). */
	void PlaceLights();
	void AddTags();
	void RetireImported();
};
