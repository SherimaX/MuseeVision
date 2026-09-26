#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChenghuaiStructure.generated.h"

class UMaterialInterface;
class UMaterialParameterCollection;
class UProceduralMeshComponent;
class URectLightComponent;
class UPostProcessComponent;
class UCurveFloat;

/**
 * 澄懷 Chenghuai (plan/proposals/chenghuai): the three-court Beijing house on the Rotunda's north door and the Suzhou
 * garden beside it, built natively from Chenghuai/ChenghuaiPlan.h. Place it at the world origin with no rotation or
 * scale: it is built in plan coordinates (x east, plan y south, height up).
 *
 * - The porch from the Rotunda's north door to the gate (广亮大门, five steps up); the entry yard and its screen wall
 *   (影壁 · 澄懷觀道); the front court and the front row (倒座房 · 臨池); the court wall and the festooned gate (垂花门);
 *   the inner court with the side halls (厢房 · 天青, 昌南), the main hall (正房 · 澄懷堂) and its ear rooms (耳房 · 清閟,
 *   停雲), joined by the covered galleries (抄手游廊); the rear court and the rear row (后罩房 · 舒卷). Qing official-style
 *   carpentry on stone-kerbed platforms, grey brick, grey tiles, iron-oxide red and Suzhou-style painting by rank
 *   (ChenghuaiHall.h).
 * - The garden (臥遊) east of the house through the moon gate: the covered walk along the house's wall with its lattice
 *   windows and engraved stones, the pond, the flower hall (花厅 · 林泉), the water pavilion (水榭 · 知魚), the zigzag
 *   bridge, the rockery (假山) with its hexagonal pavilion (亭 · 見山), white walls, chestnut timber.
 * - The display furniture: the cases cut to the works, the tables, the carved screens (花罩).
 *
 * The works (scrolls, handscrolls, ceramics), the plants and the plaques' texts are placed by Scripts/chenghuai.py as
 * their own actors. Lights: the paper windows glow by day (their light follows MPC_Musee's Daylight); discreet lamps in
 * the cases' canopies; warm lanterns under the eaves at night.
 *
 * Solids overlap or share vertices where they meet, every one closed; UV0 is in metres (the painted timber's in the
 * atlas). The geometry is rebuilt at BeginPlay as well as on construction, unless it has been baked into static meshes
 * (Geometry/MuseeBake.h). It carries musee.bakeable (it ticks only to follow the daylight); the water and the guards
 * stay procedural.
 */
UCLASS()
class MUSEEVISION_API AChenghuaiStructure : public AActor
{
	GENERATED_BODY()

public:
	AChenghuaiStructure();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** USD paths of the imported prims this wing replaces (the Sculpture Hall's, all retired). */
	UFUNCTION(BlueprintPure, Category = "Musee|Chenghuai")
	static TArray<FString> GetReplacedImportPrims();

	/** The material slots' names, in part order (the MI_Ch_<Name> instances Scripts/chenghuai.py makes). */
	UFUNCTION(BlueprintPure, Category = "Musee|Chenghuai")
	static TArray<FString> GetPartNames();

	/** Writes the geometry as OBJ (centimetres), a group per solid "part|assembly|solid|closed", for the checks. */
	UFUNCTION(BlueprintCallable, Category = "Musee|Chenghuai")
	static bool WriteGeometryObj(const FString& Path);

	/** The pond's planting edge (plan metres, x east, y south): the water's outline 0.12 m inside the bank, for the lotus. */
	UFUNCTION(BlueprintPure, Category = "Musee|Chenghuai")
	static TArray<FVector2D> GetPondOutline();

	/** Masonry, stone and floors (collision). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Structure;

	/** Timber, paper, brass, plaques (collision). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Frame;

	/** Tiles, their bed and the ridges (no collision). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Roof;

	/** The cases' glass (no collision; the cases' frames block). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Glass;

	/** The pond's water (Single Layer Water; never Nanite). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Water;

	/** The visitor's guards round the pond, the rockery's edges and the cases: collision only, not drawn. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Guard;

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<URectLightComponent>> Lamps;

	/**
	 * The courts and the garden are outdoors: a photographer's exposure for sunlit brick and tile, not the museum's
	 * high-key one (its bias and curve lift views metering EV100 11.5–13.5 by up to 1.7 stops, which washed the courts
	 * out). While the camera stands on the wing under open sky (a trace down finds this actor, one up finds no roof but
	 * the trees), this unbound volume blends in the grounds' curve (AMuseeLandscape's, EV100 → stops, no bias on top);
	 * in the rooms, the galleries and under the eaves it fades out.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Musee|Light")
	TObjectPtr<UPostProcessComponent> CourtExposure;

	UPROPERTY(VisibleAnywhere, Category = "Musee|Light")
	TObjectPtr<UCurveFloat> CourtExposureCurve;

	/**
	 * The rooms are lit for silk and paper (50 lux, Hang board) through mulberry paper: hushed, as the renderings have
	 * them, not the galleries' high key. Inside a room this volume sets the museum's bias down by RoomExposureStops.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Musee|Light")
	TObjectPtr<UPostProcessComponent> RoomExposure;

	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float RoomExposureStops = -0.9f;   // (-0.7 high-key and pink; -1.4 gloomy in the ear rooms: the renderings' rooms are calm, not dark)

	/** One material per part (GetPartNames order); a missing one falls back to Fallbacks[part], then to M_Plaster. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TArray<TSoftObjectPtr<UMaterialInterface>> PartMaterials;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TArray<TSoftObjectPtr<UMaterialInterface>> Fallbacks;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> WaterMaterial;

	/** "Daylight" (0 at night … 1 at noon), from AMuseeSky. */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	TSoftObjectPtr<UMaterialParameterCollection> DaylightParameters;

	/** The case lamps' and lanterns' colour (warm, as the museum's lamps). */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float LampKelvin = 3500.f;

	/** The paper windows' glow by day: the sky's colour through paper. */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float WindowKelvin = 6300.f;   // (5600 K under the museum's 6600 K balance turned the rooms peach)

	/** Scales every lamp (1: the design's lux). */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float LampScale = 1.f;

	/** Scales the windows' daylight glow. */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float WindowScale = 1.f;

private:
	void Build();
	void ApplyMaterials();
	void PlaceLights();
	void UpdateLights(float Daylight);
	void AddTags();

	void UpdateCourtExposure(float DeltaSeconds);

	float LastDaylight = -1.f;
	float OutdoorWeight = 0.f;
	float RoomWeight = 0.f;
	bool bWasOutdoor = false;
};
