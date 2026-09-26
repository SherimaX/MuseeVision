#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChineseWingStructure.generated.h"

class UMaterialInterface;
class UMaterialParameterCollection;
class UProceduralMeshComponent;
class URectLightComponent;

/**
 * The Chinese Wing, 四時園 the Garden of the Four Seasons (plan/README.md; Shared/Wings/ChineseWing.swift), its
 * architecture built natively from MuseePlan::ChineseWing in place of the export's (GetReplacedImportPrims). Place it at
 * the world origin with no rotation or scale: it is built in plan coordinates (x east, plan y south, height up).
 *
 * - The vestibule from the Rotunda's south door (the drum's outer face, where the Rotunda stops) to the moon gate:
 *   plaster walls and ceiling at 4.4 m, a panel closing the door's arch above it, granite paving that meets the sun
 *   clock's edge. The moon gate (Ø 3.6 m) is lined with a grey stone ring proud of both faces, on a stone sill.
 * - The court's whitewashed walls (6 m, 0.6 m thick) under tiled copings: a corbel course, a small gabled roof of real
 *   tiles, a ridge, hips at the outer corners.
 * - The cloister on three sides: dark timber columns on grey stone drum bases along a granite kerb; eave purlins and
 *   beams with brackets (雀替) and hanging fretwork (挂落); tie beams rising to the wall, a middle purlin and a wall
 *   plate; rafters under the boarding; valley beams at the south corners. Single-slope roofs of grey pans and covers
 *   in courses, with drip and end tiles at the eaves, meet in valleys at the south corners; at the north wall the west
 *   and east eaves turn up 0.55 m. Seat rails (美人靠) with goose-neck backs fill the bays along the garden.
 * - The garden's gravel, the lotus pond's bank and basin (the water surface stays the export's, 0.3 m down, its edge on
 *   the bank), irregular edging stones reaching over the water; granite paving in the walks and on the moon terrace,
 *   two stone benches, the bamboo beds' kerbs.
 * - The ceramics' drum plinths (tops at 0.9 m), the handscroll's table case (bed at 0.8 m) and the Orchid Pavilion's
 *   (bed at 0.84 m), the picture rail on the south wall (3.45 m).
 *
 * The plants, the lotus and the Taihu rock (native nature actors), the works (scrolls, the handscroll's silk and
 * rollers, the ceramics, the stele and its inscription, the plaque) and the pond's water are not this actor's.
 *
 * Light, all at LightKelvin (3800 K), no fitting near the art: a hidden strip along each walk inside the column line
 * washes the eaves' boarding and rafters from below; a strip under each seat and each terrace bench lights the paving;
 * a soft panel in the vestibule's ceiling. By day (MPC_Musee "Daylight") the eaves and the path lights go out (the
 * court is open to the sky); the vestibule's stays on.
 *
 * Solids overlap or share vertices where they meet, every one closed; UV0 is in metres. The geometry is rebuilt at
 * BeginPlay as well as on construction, so the map never shows an older build, unless it has been baked into static
 * meshes (Geometry/MuseeBake.h): then it is left alone. It carries musee.bakeable (it ticks only to dim its lamps); the
 * pond's guard stays procedural (musee.nobake).
 */
UCLASS()
class MUSEEVISION_API AChineseWingStructure : public AActor
{
	GENERATED_BODY()

public:
	AChineseWingStructure();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * USD paths of the imported prims this actor replaces (the architecture of /Museum/ChineseWing, and the cloister's
	 * three lamps, SpotLight … SpotLight_3). Retire them (and their collision) when it is placed.
	 */
	UFUNCTION(BlueprintPure, Category = "Musee|ChineseWing")
	static TArray<FString> GetReplacedImportPrims();

	/** The columns' feet (world cm). */
	UFUNCTION(BlueprintPure, Category = "Musee|ChineseWing")
	TArray<FVector> GetColumnPositions() const;

	/** The moon gate's centre, in the north wall's middle (world cm). */
	UFUNCTION(BlueprintPure, Category = "Musee|ChineseWing")
	FVector GetMoonGateCentre() const;

	/**
	 * Writes the geometry as OBJ (centimetres, the plan's axes), a group per solid named "part|assembly|solid|closed",
	 * for the checks in Scripts (watertightness, normals, surfaces too close). True if written.
	 */
	UFUNCTION(BlueprintCallable, Category = "Musee|ChineseWing")
	static bool WriteGeometryObj(const FString& Path);

	/**
	 * With complex collision. 0: the court's walls (whitewash). 1: the vestibule (plaster). 2: the paving. 3: the gravel
	 * and the bamboo beds. 4: grey stone (the moon gate's ring and sill, the kerb, the column bases, the beds' kerbs).
	 * 5: the pond's bank and basin. 6: the edging stones. 7: the ceramics' plinths and the benches.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Structure;

	/** With complex collision. 0: dark timber (the frame, the boarding, the picture rail, the cases). 1: the seat rails. 2: the cases' beds. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Frame;

	/** No collision. 0: the tiles. 1: their mortar bed, the flashing, the copings and the corbel course. 2: the ridges and hips. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Roof;

	/** A low fence round the pond's edge: collision only (not drawn; the look trace passes). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> PondGuard;

	/** Hidden strips inside the column lines washing the eaves from below (west, east, south). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<URectLightComponent>> EaveLights;

	/** Under each seat rail's plank and each terrace bench, onto the paving. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<URectLightComponent>> PathLights;

	/** A soft panel in the vestibule's ceiling. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<URectLightComponent> VestibuleLight;

	// ---- Materials (the export's instances in /Game/Museum/Materials/USD; each falls back to the next if missing).

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> WallMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> VestibuleMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> PavingMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> GravelMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> GreyStoneMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> PondBankMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> EdgingStoneMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> PlinthMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> TimberMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> SeatRailMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> CaseBedMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> TileMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> MortarMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> RidgeMaterial;

	/** Stand-ins where a material above is missing, in the same order (whitewash for the vestibule, and so on). */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TArray<TSoftObjectPtr<UMaterialInterface>> FallbackMaterials;

	// ---- Light.

	/** "Daylight" (0 at night … 1 at noon), from AMuseeSky. */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	TSoftObjectPtr<UMaterialParameterCollection> DaylightParameters;

	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float LightKelvin = 3800.f;

	/** The eaves' wash at night, lumens per metre of walk (about 150–200 lux on the dark boarding: a soft glow under the roofs). */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float EaveLumensPerMetre = 800.f;

	/** How much of the eaves' wash stays on by day (0: off). */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float EaveDayFraction = 0.f;

	/** The path lights at night, lumens per metre (about 15–20 lux on the paving by the seats). */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float PathLumensPerMetre = 25.f;

	/** The vestibule's panel, day and night (about 40 lux on its floor). */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float VestibuleLumens = 600.f;

private:
	void Build();
	void ApplyMaterials();
	void PlaceLights();
	void UpdateLights(float Night);
	void AddTags();

	float LastNight = -1.f;
};
