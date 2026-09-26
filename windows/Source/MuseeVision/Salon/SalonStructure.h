#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SalonStructure.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

/** A line where the lighting script places a strip of light: a slot or cove in the Salon's architecture. */
USTRUCT(BlueprintType)
struct FSalonLightLine
{
	GENERATED_BODY()

	/** The line's ends, world cm (the middle of the slot, just above its floor, or just under its ceiling). */
	UPROPERTY(BlueprintReadOnly, Category = "Musee")
	FVector Start = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Musee")
	FVector End = FVector::ZeroVector;

	/** Which way the light faces (unit, world). */
	UPROPERTY(BlueprintReadOnly, Category = "Musee")
	FVector Facing = FVector::UpVector;

	/**
	 * VaultCove: the trough behind the cornice's lip at the vault's springing (washes the coffers).
	 * SilkTop: the slot under the cornice (washes the silk down). SilkBase: the slot in the skirting (washes it up).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Musee")
	FName Kind;

	/** 1–5: the bay; 0: the Manet cabinet. */
	UPROPERTY(BlueprintReadOnly, Category = "Musee")
	int32 Room = 0;

	/** Clear width of the slot, cm. */
	UPROPERTY(BlueprintReadOnly, Category = "Musee")
	float SlotWidth = 0.f;
};

/**
 * Salon Impression, built natively (Shared/Wings/Salon.swift, Plan.Salon, Plan.Oval; the Salon boards; look targets
 * 01–03) in place of the export's coarse meshes. Place it at the world origin with no rotation or scale: it is built in
 * plan coordinates (x east, plan y south, height up; the Rotunda's centre is the origin).
 *
 * - Five bays from Bay 1's end wall (x −14) to Bay 5's far wall (x −74), 14 m wide. Walls 0.6 m thick, closed on both
 *   faces: silk from a travertine skirting (with a light slot) to a moulded plaster cornice at the 6.5 m springing,
 *   whose lip hides a light trough that washes the vault. Travertine piers (1.2 × 2.5 m) with moulded bases and
 *   imposts carry transverse arches (9 m clear) with archivolts.
 * - A coffered barrel vault (R 7 m, crown 13.5 m): 19 rows round × 8 per bay of triple-stepped coffers, and in the crown
 *   of each bay a glazed lantern 2.3 × 1.16 m (clear glass, a steel grid) under a sealed flat roof that is open only
 *   over the glass.
 * - Polished travertine floors with flush Ø 0.9 m viewing stones (honed travertine, bronze ring) where the plan puts them.
 * - Round-arched doors (3 m, springing 3.3 m, 64 segments) with travertine architraves: from the Rotunda passage, to
 *   the Manet cabinet and on through a vaulted passage to the Nymphéas oval.
 * - The Manet cabinet (8 × 8 m, 5.5 m): silk walls, a coved cornice, a laylit ceiling, its own roof.
 * - The Nymphéas oval (22 × 15 m inside, walls 5 m): lime plaster, a deep cove up to a luminous velarium, the pond's
 *   floor opening left exactly as the plan has it. (Its four pond benches are AMuseeFurniture's, Furniture/.)
 *
 * Imported parts it replaces (hide them): GetReplacedImportPrims().
 */
UCLASS()
class MUSEEVISION_API ASalonStructure : public AActor
{
	GENERATED_BODY()

public:
	ASalonStructure();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** The vault's cove troughs at the springing: both long walls of every bay, and the two end walls. World cm. */
	UFUNCTION(BlueprintCallable, Category = "Musee|Salon")
	TArray<FSalonLightLine> GetCoveLines() const;

	/** Every light slot: the vault coves, and the slots at the top and foot of the silk (bays and cabinet). World cm. */
	UFUNCTION(BlueprintCallable, Category = "Musee|Salon")
	TArray<FSalonLightLine> GetLightLines() const;

	/** The five lanterns' glass openings, from the glass up to the roof top. World cm. */
	UFUNCTION(BlueprintCallable, Category = "Musee|Salon")
	TArray<FBox> GetLanternOpenings() const;

	/** The centres of the viewing stones, on the floor. World cm. */
	UFUNCTION(BlueprintCallable, Category = "Musee|Salon")
	TArray<FVector> GetViewingStoneCentres() const;

	/** Plan metres: bay b's (0–4) lantern opening, x0, y0 (north), x1, y1 (south), and the glass's height (m). */
	static void LanternRect(int32 Bay, double& X0, double& Y0, double& X1, double& Y1, double& GlassZ);

	/** Plan metres: the diffusers' undersides (bay 2's opal glass, bay 4's muslin at its lowest), m. */
	static double DiffuserZ(int32 Bay);

	/** Plan metres: the oval's velarium, its centre and half extents (x, y) and its height at the rim. */
	static void VelariumRect(double& CX, double& HalfX, double& HalfY, double& Z);

	/** USD paths of the imported Salon prims this actor replaces: hide them (and their collision) when it is placed. */
	UFUNCTION(BlueprintPure, Category = "Musee|Salon")
	static TArray<FString> GetReplacedImportPrims();

	/**
	 * Sections 0–4: the bays' silk (the walls, and since the interior update the panels on the faces of the piers that
	 * look into the bay). 5: the cabinet's silk. 6: travertine (piers, spandrels, reveals, outer faces). 7: the oval's
	 * plaster walls. 8: the transverse arches' voussoirs and keystones (jointless stone; the V-joints are geometry).
	 * With collision.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> WallsMesh;

	/** Travertine trim: skirtings, pier bases and imposts, archivolts, door architraves. With collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> TrimMesh;

	/** Section 0: the plaster cornices (Salon and cabinet). Section 1: the oval's cove. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> CorniceMesh;

	/** Section 0: the coffered vault, lunettes and lantern shafts; the cabinet's ceiling and laylight well. 1: the gilt rosettes, one in every coffer. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> VaultMesh;

	/** The roofs (Salon, cabinet, oval, the oval passage). Light-tight; open only over the lanterns' glass. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> RoofMesh;

	/**
	 * Section 0: the stone floors (travertine under every arch, in the reveals and passages, the oval). 1: the viewing
	 * stones. 2: bronze (their rings, and the flush strips where the oak meets the stone). 3: the oak Versailles parquet
	 * of the bays and the Manet cabinet. With collision (walkable).
	 */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> FloorMesh;

	/** Empty: the oval's four benches are AMuseeFurniture's (kept so a placed actor's component list doesn't change). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> BenchMesh;

	/** The lanterns' clear glass: bays 1, 3, 4 and 5 (casts no shadow, so the sun comes through). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> GlassMesh;

	/**
	 * The lanterns' diffusers, which stop the sun and pass its light on softly: section 0 bay 2's opal glass (in place of
	 * the clear panes), 1 bay 4's muslin blind, stretched on a bronze frame under the steel grid. They cast shadows.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> DiffuserMesh;

	/**
	 * The sky over Giverny, in each lantern's shaft above the glass: the cloud you see through the clear glass (and its
	 * shadow on the floor), as ASalonSky sets the weather; the real sky and sun come through where there is none. It dims
	 * the opal and the muslin too. Section 0: bays 1, 2, 4, 5; 1: bay 3 (under the leaves). No collision.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> VeilMesh;

	/** Over bay 3's lantern, on the roof: 0 the bronze pergola, 1 the vines' stems, 2 their leaves, 3 the travertine planters. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> PergolaMesh;

	/** The lanterns' steel grid. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> SteelMesh;

	/** Section 0: the oval's velarium. Section 1: the cabinet's laylight. Sky-lit (M_Daylit); no shadows. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> DaylitMesh;

	/** The brushed brass foot of the glass stele that carries Impression, Sunrise. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> SteleFootMesh;

	/** Silk, bays 1–5 (oxblood, Paris grey, sage, dusty rose, ochre). */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TArray<TSoftObjectPtr<UMaterialInterface>> BaySilkMaterials;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> CabinetSilkMaterial;

	/** Piers, arches, reveals and the outer faces. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> StoneMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> TrimMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> CorniceMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> VaultMaterial;

	/** The oval's lime-white plaster (walls and cove). */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> OvalPlasterMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> RoofMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> FloorMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> ViewingStoneMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> BronzeMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> BenchMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> GlassMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> SteelMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> VelariumMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> LaylightMaterial;

	/** The stele's foot: M_Brass_Brushed (materials.py, saved); or a dynamic instance of M_Metal with BrassColour and BrassRoughness. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> BrassMaterial;

	/** The interior update (the salon-interior proposal): the voussoirs' jointless stone, the oak parquet, the rosettes' gilt. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> VoussoirMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> ParquetMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> GiltMaterial;

	/** The lanterns: clear panes (rain on them), bay 2's opal glass, bay 4's muslin, the sky panels over the clear glass. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> LanternGlassMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> OpalMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> MuslinMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> SkyVeilMaterial;

	/** Bay 3's sky panel: the same sky, never quite closed, so the vine's leaves above it still show against an overcast sky. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> SkyVeilLeafMaterial;

	/** Bay 3's pergola: patinated bronze, the vines' bark and leaves, the planters' stone. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TArray<TSoftObjectPtr<UMaterialInterface>> PergolaMaterials;

	/** If an interior-update material above is missing (its script not yet run): these, so nothing shows the default. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> FallbackStoneMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	FLinearColor BrassColour = FLinearColor(0.72f, 0.52f, 0.25f);

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	float BrassRoughness = 0.34f;

	/** Viewing stones (plan metres), from the SalonPlan board. Each needs 0.6 m of clear floor round its centre. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TArray<FVector2D> ViewingStones;

private:
	void Build();
	void ApplyMaterials();
};
