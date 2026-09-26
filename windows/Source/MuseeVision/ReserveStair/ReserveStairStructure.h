#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReserveStairStructure.generated.h"

class UBoxComponent;
class UMaterialInterface;
class UProceduralMeshComponent;
class URectLightComponent;
class USpotLightComponent;

/**
 * The long stair from the Nymphéas oval's pond down to the Reserve, its shaft and its handrails
 * (Shared/Wings/Reserve.swift, PondPlan and buildStair), built natively from MuseePlan::LongStair.
 * Place it at the world origin: the geometry is in plan coordinates.
 *
 * - The stair: 36 risers of 161 mm and goings of 339 mm in honed travertine, running east from the pond's
 *   opening in the oval floor (x −90.4 … −82.9, y ±1.2) down to the Reserve's floor (h −5.8). Each tread
 *   is one stone with a 25 mm nosing, rounded on top and undercut beneath; the top landing lies flush
 *   with the oval's floor and fills the opening's west end; the bottom landing runs through the Reserve's
 *   arched west door into its floor's notch (to x −73.7), wall to wall in the reveal.
 * - The shaft: brick walls (y ±1.2) from the opening down to the landing, lined at the top by a 30 cm
 *   travertine curb round the opening (whose edges the Salon leaves to it) and its slab edge (x −82.9);
 *   beyond it a plaster soffit at h −0.4 (the oval floor's underside) to the Reserve's wall (x −74.6).
 *   Closed as the Reserve is: outer faces 0.4 m out, a top 1 cm under the oval's floor.
 * - Closed strings of travertine on both walls, 25 mm proud, 24 cm over the nosings, turning into a 15 cm
 *   skirting on the bottom landing.
 * - Handrails of bronze, Ø 50 mm with a flat underside that carries a warm LED line, 900 mm over the
 *   nosings on bronze brackets (a wall rose, an arm and a stem), eased round a knee onto the landing and
 *   returned into the wall at both ends. They start where they pass under the oval's floor (as in Swift:
 *   nothing may stand in the opening, where the pond settles).
 *
 * Walking: the treads have no collision of their own; a hidden ramp under them, through the middle of
 * every tread, carries the visitor smoothly from the oval's floor to the Reserve's (26°, well within
 * MuseeCharacter's 46°; no step at all, where MaxStepHeight 45 cm would allow the treads' 16 cm). The
 * walls collide. Round the opening's north, south and east edges an invisible guard (1 m, the visitor
 * only: the look trace, the camera and the pond pass) keeps a visitor on the oval floor from stepping
 * into the shaft beside the stair, whether the pond is up or down.
 *
 * Light, at LightKelvin (neutral 3800 K), no art in the shaft: under each handrail a line of light onto the
 * treads (the rail's diffuser and four rect lights per side), and two small bronze downlights on the
 * soffit over the lower flight and the landing.
 *
 * Watertight: the treads, risers and nosings share their profile's vertices along the flight and run
 * 2 mm into the strings; the strings run 2 mm into the walls, the walls 2 mm into the Reserve's and
 * under the landing. UV0 is in metres. The geometry is rebuilt at BeginPlay as well as on construction,
 * unless it has been baked into static meshes (Geometry/MuseeBake.h; the stair's stone, the LED lines and the
 * hidden walkway stay procedural). At BeginPlay the imported stair, shaft and handrails are retired
 * (hidden, no collision).
 */
UCLASS()
class MUSEEVISION_API AReserveStairStructure : public AActor
{
	GENERATED_BODY()

public:
	AReserveStairStructure();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	/**
	 * USD paths of the imported prims this actor replaces: the long stair, the stair shaft and the
	 * handrails. The pond's posts and rim lights (moving parts), the racks and plan chests stay.
	 */
	UFUNCTION(BlueprintPure, Category = "Musee|Reserve")
	static TArray<FString> GetReplacedImportPrims();

	/** The height (m) the visitor walks at over plan x on the stair (the ramp under the treads). */
	UFUNCTION(BlueprintPure, Category = "Musee|Reserve")
	static double GetWalkHeightAt(double PlanX);

	/** The top of tread (or landing) under plan x (m): the stone the visitor sees. */
	UFUNCTION(BlueprintPure, Category = "Musee|Reserve")
	static double GetTreadHeightAt(double PlanX);

	/**
	 * Section 0: treads, risers and nosings. 1: the landings. 2: the strings. 3: the travertine curb round the
	 * opening and its slab edge. No collision (Walkway; the walls below the curb). Left procedural by the bake:
	 * its stone is made at play.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Stair;

	/** Section 0: the brick walls. 1: the closed outside. 2: the plaster soffit. With collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Shaft;

	/** Bronze: handrails, brackets, downlights. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Rails;

	/** The LED lines under the rails and the downlights' lenses (their glow is made at play: left procedural by the bake). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> RailLeds;

	/** Hidden: the ramp the visitor walks on, with the landings. Walkable. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Walkway;

	/** The invisible guards round the opening (north, south, east): the visitor only. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<UBoxComponent>> Guards;

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<URectLightComponent>> RailLights;

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<USpotLightComponent>> Downlights;

	/** The shaft's brick (the Reserve's). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> BrickMaterial;

	/** Treads, strings, landings, curb: honed travertine (its world-grid joints off on treads and strings at play). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> StoneMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> PlasterMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> BronzeMaterial;

	/** The LED lines: M_Daylit held at full glow on a white image, LightKelvin at LedNits. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> GlowMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float LightKelvin = 3800.f;

	/** The handrails' light onto the treads, lumens per metre of rail. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float RailLumensPerMetre = 220.f;

	/** Each soffit downlight (lm). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float DownlightLumens = 900.f;

	/** The LED lines' own glow (cd/m²). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float LedNits = 500.f;

private:
	void Build();
	void ApplyMaterials(bool bInstances);
	void PlaceLights();
	/** The guards round the opening (not geometry: placed whether or not the actor is baked). */
	void PlaceGuards();
	void AddTags();
	void RetireImported();
};
