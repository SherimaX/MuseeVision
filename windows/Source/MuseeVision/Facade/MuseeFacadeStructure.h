#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MuseeFacadeStructure.generated.h"

class UMaterialInterface;
class UMaterialParameterCollection;
class UProceduralMeshComponent;
class USpotLightComponent;

/**
 * The museum's exterior dress (MuseePlan::Facade): the stone wings clad as one classical building, after the
 * Altes Museum, the Glyptotek and the Petit Palais, with the Élan's pearl dome as its crown. Place it at the world
 * origin with no rotation or scale: it is built in plan coordinates (x east, plan y south, height up, metres).
 *
 * - A rusticated podium 2 m high runs unbroken round the whole stone complex: the Sculpture Hall, the Rotunda's drum,
 *   the passages, the Salon with the Manet cabinet, and the Nymphéas oval (joined to the Salon by a short new wall, the
 *   hyphen, |y| 5.5 m). A moulded base course, two courses of banded rustication with V joints on the stone's own
 *   0.6 m coursing, a cap; a gravel strip with a stone edge at its foot.
 * - On it, each volume carries one Ionic order after Vignola (column 9 D, entablature 2.25 D, cornice with dentils):
 *   the Salon a giant order (D 1.1 m, entablature to 14.4 m, over its roof), the Sculpture Hall D 0.79, the drum
 *   D 0.83 with an attic to 12.15 m, the oval D 0.48, the cabinet D 0.4. Fluted pilasters with Attic bases and Ionic
 *   capitals (volutes with their spiral fillets); vein-cut ashlar between them, 0.2 m over the existing faces.
 * - Blind glazed windows in the bays: round-arched on the Salon, the Sculpture Hall, the drum and the cabinet, oculi on
 *   the oval: moulded architraves, keystones, sills on consoles, aprons, bronze glazing bars over dark backing, the
 *   Salon's with a round tablet over each. The openings are cut in the skin only (0.14 m deep), never into a wall.
 * - Balustrades with pedestals over the pilasters and urns at the corners (the Salon, the Sculpture Hall, the oval, the
 *   cabinet); the drum an attic.
 * - The entrance front, the Salon's south side: a hexastyle Ionic portico (the Salon's order, columns at x −54 … −34,
 *   24 flutes, entasis) on the podium, eleven steps between cheek walls with urns, a coffered ceiling, a pediment with
 *   raking cornices and a tablet in its tympanum, a gabled roof; under it a bronze door and four windows. On its frieze,
 *   in gilt bronze Roman capitals: MVSÉE · VISION.
 *
 * At night (MPC_Musee's Daylight) warm floodlights come up: the portico's columns grazed from their feet, washes on the
 * pediment and the bronze door, and uplights on the pilasters of the south front, the drum, the Sculpture Hall's corners
 * and the oval. They are on lighting channel 1 only (the exterior's, as the Élan's), so they light nothing inside; the
 * dress receives channels 0 (the sun) and 1.
 *
 * Everything stands outside the existing outer faces and runs a few centimetres into the walls it meets; nothing
 * reaches an interior (doors between wings are interior: the passages keep their shells, the Hall of Light stays glass,
 * the Chinese Wing keeps its whitewashed walls and tiled copings, the Élan's exterior is its own). Solids overlap
 * rather than leave cracks; UV0 is in metres. Collision on the stone (the podium, steps and portico are walkable);
 * none on the glass. The geometry is rebuilt at BeginPlay as well as on construction, unless the actor has been baked
 * (Geometry/MuseeBake.h). It carries musee.bakeable: it ticks only to dim its own lights; nothing moves.
 *
 * Development builds: musee.Facade.Preview spawns it (and the grounds, AMuseeLandscape, with their trees) for one run
 * without saving the map (Facade/FacadePreview.cpp).
 */
UCLASS()
class MUSEEVISION_API AMuseeFacadeStructure : public AActor
{
	GENERATED_BODY()

public:
	AMuseeFacadeStructure();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** The imported prims it replaces: none (it dresses the native rooms' outer faces). */
	UFUNCTION(BlueprintPure, Category = "Musee|Facade")
	static TArray<FString> GetReplacedImportPrims();

	/** Triangles in the last build (all sections). */
	UFUNCTION(BlueprintPure, Category = "Musee|Facade")
	int32 GetTriangleCount() const { return TriangleCount; }

	/**
	 * Sections: 0 the ashlar skin, 1 honed stone (podium, steps, floors, roofs), 2 the pilasters' and columns' shafts,
	 * 3 carved stone (bases, capitals, entablatures, frames, balustrades, urns), 4 bronze (the door, glazing bars),
	 * 5 the windows' dark backing, 6 the gravel strip at the foot of the walls, 7 the base (podium, steps, the portico's
	 * floor) in a greyer stone, 8 the inscription's gilt letters. With collision (not the letters).
	 */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Stone;

	/** The windows' glass: no collision, no shadow. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Glass;

	/** The night's floodlights (lighting channel 1 only), placed by FacadeBuild::NightLights. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<USpotLightComponent>> Floodlights;

	/** MPC_Musee: its Daylight (0 at night … 1 at noon) brings the floodlights up at dusk. */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	TSoftObjectPtr<UMaterialParameterCollection> Parameters;

	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	bool bNightLights = true;

	/** Warm white, a little warmer than the Élan's 3800 K, as old sodium-free stone lighting. */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float LightKelvin = 3300.f;

	/** Scales every floodlight's candela (1: 30–70 lux on the stone). */
	UPROPERTY(EditAnywhere, Category = "Musee|Light", meta = (ClampMin = "0"))
	float FloodlightScale = 1.f;

	/** Ray-traced (MegaLights) shadows for the floodlights, so a cornice shades what is above it. */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	bool bFloodlightShadows = true;

	/** Let MegaLights sample the floodlights (off: shaded on the deferred path, steady; the flicker audit). */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	bool bFloodlightsMegaLights = false;

	/** Daylight at which the lights start to come up, and at which they are off. */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float NightLightsFadeFrom = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float NightLightsOffAt = 0.35f;

	/** The wall fields: vein-cut travertine ashlar (per-block texture); M_Travertine_Honed if it is missing. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> AshlarMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> StoneMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> ShaftMaterial;

	/** Mouldings, capitals, balusters: fine honed stone without coursing joints; M_Plaster_Moulding if it is missing. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> CarvedMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> BronzeMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> BackingMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> GravelMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> GlassMaterial;

	/** The inscription on the portico's frieze (MVSÉE · VISION): gilt bronze letters. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> GiltMaterial;

	/** The podium, the steps and the portico's floor: a warm grey stone that grounds the cream walls. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> BaseMaterial;

	/**
	 * Cover the Rotunda's dome in stone (step rings on the attic, a smooth dome, a ring round the eye), 20 cm outside its
	 * shell: its crackle-veined marble reads as a camouflage pattern from outside. Off leaves the dome as it is.
	 */
	UPROPERTY(EditAnywhere, Category = "Musee|Build")
	bool bCoverRotundaDome = true;

private:
	void Build();
	void ApplyMaterials();
	void AddTags();
	void PlaceLights();
	void SetNight(float Level, float Daylight);

	int32 TriangleCount = 0;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollection> LoadedParameters;

	float NightLevel = 0.f;
	float LastDaylight = -1.f;
};
