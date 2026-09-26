#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CubeStructure.generated.h"

class UBoxComponent;
class UMaterialInterface;
class UPostProcessComponent;
class UProceduralMeshComponent;
class URectLightComponent;

/**
 * Élan's level −1, the Cube (plan/boards/ElanCube; Cube/CubePlan.h), built natively. It replaces the Square
 * (ASquareStructure, no longer placed). Place it at the Atrium's centre (plan 54, 0, 0): the geometry is in the
 * Atrium's frame, x east, y south, z the height above the Atrium floor.
 *
 * - The room: a cube 28 m each way, its floor at −36 m, its centre 22 m under the Atrium floor. Its six faces are
 *   light-field panels, 0.7 m, 40 by 40 to a face: black, with a fine seam between panels and a slight sheen
 *   (M_CubePanel, Scripts/journeys.py). At rest only the faint grid shows; during a journey the panels "become" the
 *   scene, panel by panel (MPC_Cube Reveal: the panels dissolve and the world beyond shows through).
 * - Behind them a 1.2 m service void, then a concrete box 1 m thick all round (closed, so no light reaches the room
 *   but the car's own).
 * - The shaft: from the Atrium floor's slab (a bronze lining through it) a glass tube 4.9 m across, 60 mm laminated
 *   low-iron glass in four tiers between bronze ring frames, down through the ground to the box's roof. Behind the
 *   glass, 15 cm away across a lit cavity, the ground's cut face: a little crushed stone under the slab, dark topsoil,
 *   brown subsoil, clay-with-flints, then chalk with bands of flint nodules and an ammonite. LED lines in the ring
 *   frames graze the face from above. Below the glass a dark bronze sleeve goes through the roof and the void to the
 *   iris.
 * - The iris in the ceiling (the elevator's: UElanCubeWay) sits in a gilt soffit ring, r 2.46 to 4.12, that hides
 *   its open blades, in a housing in the void.
 * - Two T-section guide rails, north and south, run down the shaft for the car's roller guides.
 *
 * Collision: the floor's face (should anything fall), nothing else (only the car goes here). The geometry is rebuilt
 * at BeginPlay as well as on construction, unless baked (Geometry/MuseeBake.h).
 */
UCLASS()
class MUSEEVISION_API ACubeStructure : public AActor
{
	GENERATED_BODY()

public:
	ACubeStructure();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	/** The imported prims it replaces: the Square's (all of /Museum/Elan/The_Square and its five lamps). */
	UFUNCTION(BlueprintPure, Category = "Musee|Cube")
	static TArray<FString> GetReplacedImportPrims();

	/** During a journey the box, the shaft and the ground are hidden (the panels dissolve by MPC_Cube); after, back. */
	void SetJourneyMode(bool bOn);

	/**
	 * The eye at rest in the room: 0 while the lit car comes down (the exposure adapts in RoomMinEV … RoomMaxEV), 1 once
	 * its glass has cleared at the centre (fixed at RestEV, a dark-adapted eye: the faint grid, the car's glimmer on its
	 * floor and rail). `musee.CubeRestEV <ev>` overrides it for tests.
	 */
	void SetRest(float Amount);

	/** The panel faces (section 0), the floor's face colliding. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Panels;

	/** The concrete box, all six sides 1 m thick, its roof pierced for the shaft. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Box;

	/** The shaft's glass (translucent). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Glass;

	/** Metal: 0 gilt (the soffit ring, the frames' faces), 1 dark bronze (the lining, the sleeve, the housing), 2 the rails' steel. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Metal;

	/** The ground's cut face behind the glass, its flints and the ammonite (vertex colours: the layers). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Ground;

	/** The room's volume (the panels' inner faces): its exposure (RoomPost) holds while the eyes are in it. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UBoxComponent> Room;

	/**
	 * In the room at rest the eye adapts as it would in a dark hall (a narrow range, no compensation curve): the faint
	 * grid stays faint and the car's glimmer is not blown to white.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UPostProcessComponent> RoomPost;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float RoomMinEV = 3.5f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float RoomMaxEV = 9.f;   // (the lit car coming down: the eye adapts to it, and the grid fades until it has cleared)

	/** The eye's exposure at rest, the car cleared (EV100; PROPOSAL § 5 step 2: about 2). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float RestEV = 2.f;

	/** The LED lines in the cavity behind the glass, eight to a ring frame, grazing the ground's face. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<URectLightComponent>> CavityLights;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> PanelMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> PanelFallbackMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> ConcreteMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> GlassMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> GiltMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> DarkBronzeMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> RailMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> GroundMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> GroundFallbackMaterial;

	/** Each LED line in the cavity (lm). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float CavityLumens = 420.f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float CavityKelvin = 4000.f;

private:
	void Build();
	void ApplyMaterials();
	void PlaceLights();
	void AddTags();

	bool bJourneyMode = false;
	float AppliedRest = -1.f;
	float AppliedRestEV = -100.f;
};
