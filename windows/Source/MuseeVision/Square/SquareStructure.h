#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SquareStructure.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class UProceduralMeshComponent;
class USpotLightComponent;

/**
 * Élan's level −1, the Square (plan/README.md; Shared/Wings/Elan.swift, buildSquare), built natively
 * from MuseePlan::Elan. Place it at the world origin: the geometry is in plan coordinates (the
 * Square's centre is the Atrium's, x 54, y 0; its floor at h −9).
 *
 * A 28 m square under the Atrium, 7.5 m clear, in rammed earth: the outer walls 1.2 m thick, the
 * partitions 0.6 m thick on the Lo Shu's grid lines, doorways 2.6 × 3.0 m in a clockwise pinwheel
 * (6 opens east into 1, 8 south into 3, 4 west into 9, 2 north into 7), four 1.2 m columns at the
 * crossings under 2.8 m capitals. The floor is the Lo Shu's nine squares (north up: 6 1 8 / 7 5 3 /
 * 2 9 4), pale stone in the cross of galleries (1, 3, 9, 7 and 5), dark in the corner rooms (6, 8, 4,
 * 2), each with its numeral cast in bronze. At 5, where the car lands, a clear glass disc (R 4.2 m,
 * a slim bronze frame) looks down into a round pit 7 m deep through the strata: soil, clay, chalk
 * with an ammonite, bedrock. The ceiling is a slab with the shaft's opening, lined in bronze (a ring
 * from r 2.3 to 3.0 and the lining up to the Atrium's floor), and the ring of light in it at r
 * 13.8–14, under the Atrium's circle.
 *
 * Light, all electric at LightKelvin (neutral 3800 K), all components of this actor, none of them a
 * fitting by the art: a spot down the shaft onto 5 (on through the glass into the pit); in each of
 * the four galleries six recessed wall-washers, two per wall, about 3 m out from it (walls about
 * 180 lux direct at 1–2.6 m, a little more with the pale floor's bounce) and a soft downlight over
 * the numeral; one dim, broad downlight in each dark room (about 20 lux); two lamps on the pit's axis
 * for the strata. The ring of light is self-lit (RingNits), not a lamp. The lights keep their level
 * by night and day (no musee.laylight tag).
 *
 * Solids overlap rather than meet: walls, partitions and columns stand 5 cm into the floor and 2 cm
 * into the ceiling slab; the floor, pit and glass share their edges vertex for vertex. UV0 is in
 * metres: floors and ceilings plan (x, y); every vertical face of rammed earth U = x + y (so the
 * lifts run on round every corner) and V = 7.5 − the height (the layering is one image the height of
 * the room); the pit's wall U = the arc from east, V = the depth (see stone_textures.py strata_pit).
 *
 * The geometry is rebuilt at BeginPlay as well as on construction, so the map never shows an older
 * build.
 */
UCLASS()
class MUSEEVISION_API ASquareStructure : public AActor
{
	GENERATED_BODY()

public:
	ASquareStructure();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	/**
	 * USD paths of the imported prims this actor replaces: the whole of /Museum/Elan/The_Square
	 * (floors, glass, strata pit, rammed earth, capitals, ceiling, ring of light, numerals) and the
	 * Square's five lamps (/Museum/Elan/SpotLight_3 … _7: the shaft spot and the four downlights).
	 * Retire them (and their collision) when it is placed. The landing screens are AElanElevator's.
	 */
	UFUNCTION(BlueprintPure, Category = "Musee|Square")
	static TArray<FString> GetReplacedImportPrims();

	/** The centre of the glass floor at 5, on the floor (world cm). */
	UFUNCTION(BlueprintPure, Category = "Musee|Square")
	FVector GetGlassCentre() const;

	/** Where each numeral lies, 1 to 9 (world cm, on the floor). */
	UFUNCTION(BlueprintPure, Category = "Musee|Square")
	TArray<FVector> GetNumeralPositions() const;

	/**
	 * With complex collision. Section 0: the rammed earth (outer walls, partitions, columns).
	 * 1: the pale floor (the cross, and the rim round the glass). 2: the dark rooms' floors.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Structure;

	/** The glass floor over the pit: walkable; the look trace passes through it. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> GlassFloor;

	/** The pit's round wall and its floor, in the strata. No collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Pit;

	/** Section 0: the ceiling slab (underside, top and edges). 1: the capitals. No collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Ceiling;

	/** The bronze: the numerals, the glass's frame, the ring round the shaft and its lining. No collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Bronze;

	/** The ring of light in the ceiling (self-lit, casts no shadow). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> LightRing;

	/** Down the shaft onto 5 and on through the glass into the pit. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<USpotLightComponent> ShaftLight;

	/** Recessed wall-washers, six to a gallery (1, 3, 9, 7), two per wall. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<USpotLightComponent>> WallWashers;

	/** A soft downlight over each gallery's numeral (1, 3, 9, 7). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<USpotLightComponent>> GalleryDownlights;

	/** One dim, broad downlight in each dark room (6, 8, 4, 2). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<USpotLightComponent>> DarkRoomLights;

	/** Two lamps on the pit's axis, under the glass (1.2 m and 4.6 m down), for the strata. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<UPointLightComponent>> PitLights;

	/** Rammed earth: UV0 U = x + y, V = 7.5 − height (m); M_RammedEarth (T_rammed_earth_lifts, TexScale 2.8). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> EarthMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> EarthFallbackMaterial;

	/** The pale stone of the cross. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> FloorMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> FloorFallbackMaterial;

	/** The dark rooms' stone. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> DarkFloorMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> DarkFloorFallbackMaterial;

	/** The ceiling slab and the capitals. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> CeilingMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> CeilingFallbackMaterial;

	/** The pit: M_Strata (T_strata_pit, laid out for this pit's UVs). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> StrataMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> StrataFallbackMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> GlassMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> BronzeMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> BronzeFallbackMaterial;

	/** The ring of light: M_Daylit, held at full glow (NightFloor 1), a white image, tinted to LightKelvin. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> RingMaterial;

	/** The ring of light's luminance (cd/m²). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float RingNits = 600.f;

	/** Colour temperature of every lamp (and of the ring). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float LightKelvin = 3800.f;

	/** Each wall-washer (cd on its axis): about 180 lux direct on the gallery walls at 1–2.6 m. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float WasherCandela = 11000.f;

	/** Each gallery downlight: about 70 lux under it, on top of the washers' spill. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float DownlightCandela = 4000.f;

	/** Each dark room's light: about 20 lux on the floor, 10–15 on the walls. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float DarkRoomCandela = 1800.f;

	/** The shaft's spot: about 400 lux on 5, 100 on the pit's floor. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float ShaftCandela = 31000.f;

	/** The pit's upper and lower lamps: the strata at 70–150 lux. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float PitUpperCandela = 2000.f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float PitLowerCandela = 1100.f;

private:
	void Build();
	void ApplyMaterials(bool bInstances);
	void PlaceLights();
	void AddTags();

	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> RingGlow;
};
