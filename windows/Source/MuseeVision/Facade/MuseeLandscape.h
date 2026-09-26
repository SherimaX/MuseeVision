#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Nature/NatureTypes.h"
#include "MuseeLandscape.generated.h"

class UCurveFloat;
class UInstancedStaticMeshComponent;
class UPostProcessComponent;
class UMaterialInterface;
class UMaterialParameterCollection;
class UPointLightComponent;
class UProceduralMeshComponent;

/** One tree of the grounds, for Scripts/nature.py (place_grounds) and the preview: an AMuseeTree's settings. */
USTRUCT(BlueprintType)
struct FMuseeGroundsTree
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Musee")
	FString Name;

	/** Plan metres (x east, y south); the trunk's foot on the ground. */
	UPROPERTY(BlueprintReadOnly, Category = "Musee")
	FVector2D Position = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Musee")
	EMuseeTreeSpecies Species = EMuseeTreeSpecies::Birch;

	UPROPERTY(BlueprintReadOnly, Category = "Musee")
	float TreeHeight = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Musee")
	float CrownHeight = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Musee")
	FVector2D CrownRadii = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Musee")
	float LeafDensity = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Musee")
	int32 Seed = 1;

	/** Bamboo: the culms, metres from Position (x east, y south); empty for a tree. */
	UPROPERTY(BlueprintReadOnly, Category = "Musee")
	TArray<FVector2D> Culms;

	/** The plan box the plant keeps inside (metres from Position; AMuseeTree::Keep); invalid = no limit. */
	UPROPERTY(BlueprintReadOnly, Category = "Musee")
	FBox2D Keep = FBox2D(ForceInit);
};

/**
 * The museum's grounds (MuseePlan::Grounds), laid out as a French formal garden before the portico on the Salon's south
 * front. Place it at the world origin with no rotation or scale (plan coordinates, metres).
 *
 * - The parvis: honed travertine slabs from the podium's gravel strip to the cour, the portico's steps standing on it.
 * - The cour: gravel round a long still canal on the axis (x −44, y 27 … 57: a raised travertine coping, a dark stone
 *   floor, still water that holds the portico's reflection), two lawn panels (tapis verts) edged with clipped box and
 *   clipped balls at their corners, walks for the allées outside them, a cross walk at the far end closed by a
 *   semicircular yew exedra open on the axis.
 * - The avenue south along the axis; the walk east (y 40 … 44) past the Chinese Wing to a rond-point with a round basin
 *   on the Élan's meridian, and a path north from it to the gravel terrace round the Élan's plinth.
 * - Lamp standards along the canal and at the parvis's corners: bronze posts with opal globes that glow, and at night
 *   (MPC_Musee's Daylight) a warm light in each, on lighting channel 1 only (the exterior's).
 * - Stone kerbs along the walks.
 * - Clipped hedges of real leaves (Nature/MuseeHedge): box round the lawn panels and its clipped balls, the yew exedra,
 *   and the Hall of Light's garden hedges along its long sides (yew; the import's plain boxes are hidden by
 *   Scripts/lawn.py). Their leaf shells are instanced Nanite modules (PlantHedges, transient, never baked) over dark
 *   procedural cores a few centimetres inside the clipped surface (collision; the shade inside the hedge).
 *
 * The trees (allées of round-headed osmanthus in the parterre, birches along the avenue and the walk east, groves round
 * the building, bamboo against the Chinese Wing's white outer walls) are AMuseeTree actors placed by Scripts/nature.py (place_grounds) from GetTreePlacements. Nothing here
 * reaches the Hall of Light's meadow and orchard (x 11 … 41, |y| < 13.8) but its two hedges (y ±14).
 *
 * Collision: the paving, gravel, kerbs, copings and the hedges' cores (walkable where flat); none on the water or the
 * leaves. musee.Hedges 0/1 hides and shows the leaves (for A/B timing). The geometry is rebuilt at BeginPlay as well as on construction, unless baked (Geometry/MuseeBake.h). It
 * carries musee.bakeable (nothing moves; the leaves carry no wind; it ticks only to dim its lamps).
 */
UCLASS()
class MUSEEVISION_API AMuseeLandscape : public AActor
{
	GENERATED_BODY()

public:
	AMuseeLandscape();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

	/** The imported prims it replaces: none. */
	UFUNCTION(BlueprintPure, Category = "Musee|Grounds")
	static TArray<FString> GetReplacedImportPrims();

	/** The grounds' trees (nature.py place_grounds places them as AMuseeTree; the preview spawns them). */
	UFUNCTION(BlueprintPure, Category = "Musee|Grounds")
	static TArray<FMuseeGroundsTree> GetTreePlacements();

	UFUNCTION(BlueprintPure, Category = "Musee|Grounds")
	int32 GetTriangleCount() const { return TriangleCount; }

	/** Instance the hedges' leaf modules (done at BeginPlay, and for the editor's viewport). */
	UFUNCTION(BlueprintCallable, Category = "Musee|Grounds")
	void PlantHedges();

	UFUNCTION(BlueprintCallable, Category = "Musee|Grounds")
	void SetHedgesShown(bool bShow);

	/** Sections: 0 stone paving, 1 gravel, 2 kerbs and copings, 3 the basins' floors. With collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Ground;

	/** The basins' still water (no collision, no shadow). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Water;

	/** The hedges' dark cores (collision), under their instanced leaves. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Hedges;

	/** Section 0: the lamp standards (bronze, collision). Section 1: their opal globes. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Lamps;

	/** A warm light in each globe at night (lighting channel 1 only). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<UPointLightComponent>> LampLights;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> LampMaterial;

	/** The globes: M_OpalGlobe (glowing warm glass). */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> GlobeMaterial;

	/** MPC_Musee: its Daylight lights the lamps at dusk. */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	TSoftObjectPtr<UMaterialParameterCollection> Parameters;

	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	bool bNightLights = true;

	/**
	 * Outdoors, a photographer's exposure for a sunlit garden, not the museum's high-key one: the global exposure lifts
	 * views that meter EV100 11.5–13.5 by up to 1.7 stops (right for the sky-lit rooms), and a view of lawn and hedges
	 * meters just there, so the grounds washed out. While the camera stands on the grounds under open sky (a trace
	 * down finds the landscape or the ground, one up finds no roof), this unbound volume blends in its own
	 * compensation curve (ExteriorExposureCurve, EV100 → stops, no bias on top); inside, it fades out.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Musee|Light")
	TObjectPtr<UPostProcessComponent> ExteriorExposure;

	UPROPERTY(VisibleAnywhere, Category = "Musee|Light")
	TObjectPtr<UCurveFloat> ExteriorExposureCurve;

	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	bool bExteriorExposure = true;

	UPROPERTY(EditAnywhere, Category = "Musee|Light", meta = (ClampMin = "0"))
	float LampCandela = 300.f;

	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float LampKelvin = 2800.f;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> PavingMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> GravelMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> KerbMaterial;

	/** The basins' floors: dark honed stone, so the water holds reflections. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> BasinMaterial;

	/** Still, dark, mirror-like water (the Chinese garden's pond); the lily pond's if it is missing. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> WaterMaterial;

	/** The hedges' cores: MI_Hedge_Core (bare twigs in the shade inside a clipped hedge). */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> HedgeCoreMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Hedges")
	bool bPreviewHedgesInEditor = true;

private:
	void Build();
	void ApplyMaterials();
	void AddTags();
	void PlaceLights();
	void SetNight(float Level, float Daylight);
	void ClearHedges();
	void UpdateExteriorExposure(float DeltaSeconds);

	float OutdoorWeight = 0.f;

	int32 TriangleCount = 0;

	UPROPERTY(Transient, DuplicateTransient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> HedgeLeaves;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollection> LoadedParameters;

	float LastDaylight = -1.f;
};
