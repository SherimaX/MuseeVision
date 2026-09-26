#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HallOfLightStructure.generated.h"

class UMaterialInterface;
class UMaterialParameterCollection;
class UProceduralMeshComponent;
class USpotLightComponent;

/**
 * The Hall of Light's architecture (plan/README.md; Shared/Wings/HallOfLight.swift, buildHallOfLight),
 * built natively from MuseePlan::HallOfLight. Place it at the world origin: the geometry is in plan
 * coordinates (the Rotunda's centre is the origin).
 *
 * A glass hall 9 m wide from the Rotunda's drum to the Atrium's stone base, between the birch meadow
 * (north) and the orchard (south). A honed travertine plinth 0.3 m high carries the glass walls, with
 * a stone planter edge outside it along the gardens. Pearl-white steel: I-section posts every 3 m with
 * a transom at 3.2 m and a glazing shoe on the plinth, a box eave at 5 m; above it the shallow vault
 * (R 5.3 m, crown 7.5 m) of I-section arches every 1.5 m, eight rows of purlins and a cross of flat
 * bars in every panel, the glass carried over them. At the ends the vault and the walls run into the
 * drums on steel collars: into the Rotunda's marble, and at the Atrium into its stone base below 6 m
 * and up to its misty glass above (a purlin lies along the coping where they change). A polished
 * travertine floor runs through both doors' reveals to the sun clock (under a bronze threshold) and the
 * Atrium's floor, with five flush viewing stones on the axis (rendering 06). Plaster architraves round both doors. Four round
 * stereo stones with bronze stems under their readers; each print's mount stands on two bronze stems
 * on the plinth; the autochromes are set into the glass between bronze glazing bars.
 *
 * Light: by day the sun and the sky come in through the glass (this actor adds none). At night the hall
 * has its own soft light at LightKelvin: concealed uplights on the eaves wash the lattice from the
 * springing, and low path lights in the plinth, under the posts (never by a print), light the floor.
 * They follow MPC_Musee's Daylight: full at night, off by day. The prints' accent spots are
 * Scripts/gallery_lights.py's.
 *
 * Watertight by construction: every member is a closed solid; surfaces that meet share vertices or one
 * runs 1–5 cm into the solid it meets (glass into the plinth, the eaves and the drums, the floor under the
 * plinths and into the drums' walls); no two faces lie closer than 1 cm unless they share their vertices.
 * UV0 in metres. check_hall_of_light.py (beside this file) is a Python port of the geometry that checks it.
 * The geometry is rebuilt at BeginPlay as well as on construction, so the map never shows an older build.
 */
UCLASS()
class MUSEEVISION_API AHallOfLightStructure : public AActor
{
	GENERATED_BODY()

public:
	AHallOfLightStructure();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * USD paths of the imported prims this actor replaces: the hall's floor, plinths, glass and lattice
	 * (posts, eaves and vault), and the stereo stones. Retire them (and their collision) when it is
	 * placed. The prints, their mounts and edges, the stereographs and their readers, the trees, the
	 * flowers, the Ground and the Hedges stay.
	 */
	UFUNCTION(BlueprintPure, Category = "Musee|HallOfLight")
	static TArray<FString> GetReplacedImportPrims();

	/** The flush viewing stones' centres on the axis (world cm, on the floor). */
	UFUNCTION(BlueprintPure, Category = "Musee|HallOfLight")
	TArray<FVector> GetViewingStoneCentres() const;

	/** How strongly the night lights are on now: 0 by day … 1 at night. */
	UFUNCTION(BlueprintPure, Category = "Musee|HallOfLight")
	float GetNightLevel() const { return NightLevel; }

	/** Section 0: the polished floor. 1: the viewing stones. 2: their bronze rings. Walkable. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Floor;

	/** Section 0: plinths, planter edges and stereo stones (honed travertine). 1: the doors' architraves. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Stone;

	/** The glass walls' steel: posts, transoms, glazing shoes, end posts and eaves. With collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Frame;

	/** The vault's steel: arches, purlins, crosses and the collars at the drums. No collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Lattice;

	/** The walls' and the vault's glass: blocks the visitor, the look trace passes through. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Glass;

	/** The bronze: the prints' stems, the stereo readers' stems, the autochromes' glazing bars, the path lights' louvres. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Bronze;

	/** On the eaves, ten a side midway between arches (x 11.75 … 38.75), washing the lattice across the vault. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<USpotLightComponent>> SpringingUplights;

	/** Low in the plinth under each post (never under a print), on both sides, across the floor. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<USpotLightComponent>> PathLights;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> FloorMaterial;

	/** The plinths, planter edges, stereo stones and viewing stones. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> StoneMaterial;

	/** The doors' architraves. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> MouldingMaterial;

	/** Walls' frame and lattice: M_RibPearl (the Atrium's pearl-white steel); M_RibSteel for the Rotunda's dark steel. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> SteelMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> GlassMaterial;

	/** M_Bronze_Statuary (materials.py, saved); or M_Metal, given BronzeColour and BronzeRoughness at run time. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> BronzeMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	FLinearColor BronzeColour = FLinearColor(0.10f, 0.061f, 0.032f);

	UPROPERTY(EditAnywhere, Category = "Musee")
	float BronzeRoughness = 0.4f;

	/** MPC_Musee: its Daylight (0 at night … 1 at noon) turns the night lights down by day. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialParameterCollection> Parameters;

	/** Colour temperature of every lamp: neutral. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float LightKelvin = 3800.f;

	/** Each uplight (cd on its axis): the pearl lattice across the vault at about 70 lux, a soft glowing canopy. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float UplightCandela = 2500.f;

	/**
	 * Each path light (cd on its axis), 14 cm over the floor: a pool of about 150 lux half a metre out from the
	 * plinth, 20 lux at a metre; the prints (1.5 m along the wall from them) get under 5 lux (their accent spots give 70).
	 */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float PathLightCandela = 150.f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	bool bUplightShadows = false;

	UPROPERTY(EditAnywhere, Category = "Musee")
	bool bPathLightShadows = true;

	/** Daylight at which the night lights start to fade, and at which they are off. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float NightLightsFadeFrom = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float NightLightsOffAt = 0.35f;

private:
	void Build();
	void ApplyMaterials();
	void PlaceLights();
	void AddTags();
	void SetNightLevel(float Level);

	float NightLevel = 1.f;
};
