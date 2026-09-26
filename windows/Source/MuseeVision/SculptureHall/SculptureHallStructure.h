#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SculptureHallStructure.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;

/**
 * The Sculpture Hall's architecture, built natively (Shared/Wings/SculptureHall.swift, SculptureHallPlan;
 * MuseePlan::SculptureHall) in place of the export's single skins. Place it at the world origin with no
 * rotation or scale: it is built in plan coordinates (x east, plan y south, height up; the Rotunda's centre
 * is the origin).
 *
 * - One court 16.8 m square inside (x ±8.4, y −30.9 … −14.1), vein-cut travertine ashlar walls 0.6 m thick, closed on
 *   both faces, standing 5 cm into the ground and rising to a parapet under a moulded travertine coping
 *   (10.45–10.56 m). A travertine skirting (18 cm), two string beads (3.2 and 6.4 m) and a plaster cornice
 *   (9.30–9.65 m: ovolo, corona, cyma) run round the room.
 * - The door on the axis of the south wall: 4 m, round-arched, springing 4.5 m, 48 stations per semicircle,
 *   the Rotunda's own, so its reveal meets the Rotunda's north passage (which ends at y −13.5, the wall's outer
 *   face) vertex for vertex. A moulded travertine architrave (0.30 m) on the court side; the skirting and the
 *   string beads die into it.
 * - The ceiling at 9.65 m round the laylight's opening (x ±7, y −29.7 … −15.3), a moulded plaster frame round
 *   the opening, and the well up to the diffuser at 9.97 m: the sky-lit panel (MI_light_grid_FFF3DA) under a
 *   grid of slim pearl steel bars on the panel's own 1 m lines. The diffuser casts shadows, so no sun reaches
 *   the court directly: daylight comes in only as the laylight's glow.
 * - Above: a slab to a roof deck at 10.30 m, a kerb round the well to 10.5 m, and a hipped glass roof over it
 *   (ridge 12.3 m) on steel rafters, hips, a ridge and an eaves gutter. The envelope is sealed; the chamber
 *   over the diffuser is open to the sky only through the glass.
 * - Polished travertine floor: the court, the door's reveal, and the Rotunda's north passage up to the sun
 *   clock's edge (its 128-gon at r 10.3 m), so the two floors meet edge to edge.
 * - The ten works' plinths, as the plan sizes them (tops unchanged), with a recessed toe; The Dance's stands
 *   4 cm off the west wall, clear of the skirting. The bench opposite The Dance: a travertine slab on two
 *   blocks. The works themselves (the scans) are not this actor's.
 *
 * Solids overlap rather than meet: wall faces run 5 cm below the floor and up into the coping; a face that
 * stops runs on into the solid it meets or shares its edge. UV0 is in metres (walls along the face and down,
 * floors and ceilings plan x, y; the diffuser plan x, y offset by half a grid line, so the panel's printed lines
 * fall under the bars).
 *
 * The geometry is rebuilt at BeginPlay as well as on construction, so the map never shows an older build.
 *
 * Development builds only: -MuseeSculptureHall previews it in the game before it is placed (spawned at the origin,
 * the pieces it replaces hidden); -MuseeSculptureDump=<file> writes its triangles for the geometry checks.
 */
UCLASS()
class MUSEEVISION_API ASculptureHallStructure : public AActor
{
	GENERATED_BODY()

public:
	ASculptureHallStructure();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/**
	 * USD paths of the imported Sculpture Hall prims this actor replaces: the court's walls, mouldings, floor,
	 * ceiling, laylight, bench and plinths. Retire them (and their collision) when it is placed. (The passage
	 * is the Rotunda's; the laylight's stand-in spot is relight.py's to remove; the scans stay.)
	 */
	UFUNCTION(BlueprintPure, Category = "Musee|SculptureHall")
	static TArray<FString> GetReplacedImportPrims();

	/** The laylight's glowing panel (the diffuser, x ±7, y −29.7 … −15.3 at 9.97 m), world cm, zero height. */
	UFUNCTION(BlueprintPure, Category = "Musee|SculptureHall")
	FBox GetLaylightPanel() const;

	/**
	 * Where a rect light for the laylight belongs (world cm): the panel's clear area inside the perimeter bars,
	 * 1 cm under the bars (9.89 m), facing down.
	 */
	UFUNCTION(BlueprintPure, Category = "Musee|SculptureHall")
	FBox GetLaylightLightBox() const;

	/** The share of the panel's area that the bars leave open (for the rect light's candela). */
	UFUNCTION(BlueprintPure, Category = "Musee|SculptureHall")
	static float GetLaylightOpenFraction();

	/** Section 0: the court's walls (inner faces, the door's reveal). 1: the exterior (outer faces, coping, kerb). With collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Walls;

	/** The skirting, the string beads and the door's architrave. With collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Trim;

	/** Section 0: the cornice. 1: the laylight's frame. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Mouldings;

	/** The ceiling and the laylight's well. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Ceiling;

	/** Section 0: the roof deck. 1: the glass roof's steel (gutter, rafters, hips, ridge). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Roof;

	/** The glass roof's panes (casts no shadow). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> RoofGlass;

	/** The laylight's diffuser: sky-lit (M_Daylit); casts shadows, so the sun stops at it. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Laylight;

	/** The bars under the diffuser. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> LaylightBars;

	/** The floor (court, reveal, passage). With collision (walkable). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Floor;

	/** Section 0: the plinths. 1: the bench. With collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Furniture;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> WallMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> ExteriorMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> TrimMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> CorniceMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> CeilingMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> RoofDeckMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> RoofSteelMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> GlassMaterial;

	/** The sky-lit panel (relight.py's LAYLIGHTS lights it; MuseeSky drives its Daylight). */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> LaylightMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> LaylightBarMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> FloorMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> PlinthMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> BenchMaterial;

private:
	void Build();
	void ApplyMaterials();
	void AddTags();
};
