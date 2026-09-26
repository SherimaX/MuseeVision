#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ElanExteriorStructure.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMaterialParameterCollection;
class UPointLightComponent;
class UProceduralMeshComponent;
class USpotLightComponent;

/** A pose to judge the exterior from: -MuseePose=X,Y,Feet,Yaw,Pitch -MuseeFov=Fov -ExecCmds="musee.Hour Hour". */
USTRUCT(BlueprintType)
struct FElanExteriorViewpoint
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Musee") FString Name;
	/** Plan metres (x east, y south) and the feet's height. */
	UPROPERTY(BlueprintReadOnly, Category = "Musee") double X = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Musee") double Y = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Musee") double Feet = 0;
	/** Degrees: yaw from east towards south (Unreal's), pitch up. */
	UPROPERTY(BlueprintReadOnly, Category = "Musee") double Yaw = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Musee") double Pitch = 0;
	/** Horizontal field of view (degrees) and the local hour (21.5: night). */
	UPROPERTY(BlueprintReadOnly, Category = "Musee") float Fov = 90.f;
	UPROPERTY(BlueprintReadOnly, Category = "Musee") float Hour = 15.f;
	UPROPERTY(BlueprintReadOnly, Category = "Musee") FString Note;

	/** "X,Y,Feet,Yaw,Pitch" for -MuseePose. */
	FString Pose() const;
	/** The game's arguments for this view: -MuseePose, -MuseeFov and the hour. */
	FString CommandLine() const;
};

/**
 * Élan's exterior: the Atrium and the Sphere seen from outside as a tall domed hall (plan/README.md,
 * "from outside it is a misty pearl on the drum"). Place it at the Élan's centre, (5400, 0, 0), no
 * rotation: the geometry is in metres about the drum's axis, the ground at 0.
 *
 * - The plinth (0–6 m): travertine ashlar over the imported stone base, on a moulded base (socle,
 *   torus, scotia) under a coping cornice (ovolo, corona, bead, a wash up to the drum).
 * - The drum (6–22 m): the misty glass (its own exterior layer, tuned for outdoors, 3 cm outside the
 *   interior's) behind a clear outer pane, framed by an order of twelve pearl-steel fins on the
 *   interior ribs' bearings (tapering shafts on pedestals, flaring at the top into brackets under the
 *   cornice), a slender mullion on each bay's centre line and three transoms (h 10, 14, 18).
 * - The equator (20.9–22.58 m): a stone entablature, architrave, frieze and cornice, from which the
 *   Sphere rises like a ball from a cup.
 * - The dome (22–36.3 m), bPearlShell (the default, 2026-09-26): the Sphere's upper half as one seamless opaque
 *   shell (the Sphere inside is a full projection surface, so its outer shell can't be glass): 6 cm pearl panels
 *   (ShellMaterial, MI_Ext_Pearl: a white ceramic-metal under a clear lacquer, the sky's reflections sliding over it)
 *   whose hairline joints follow meridians and parallels and read only close up; a hairline gilt ring where it rises
 *   out of the cornice (a drip that throws the shell's water clear of the cornice's wash); at the top a gilt boss and
 *   the gilt ball: a sphere on the Sphere. The earlier design (off): a misty pearl behind a clear pane under a
 *   lamella lattice of pearl steel, 24 meridian ribs and two rings, a gilt compression ring, a glazed lantern that
 *   glows at night, a cap and the ball.
 * - The portal (west): a travertine frontispiece that takes the Hall of Light's glass section into the
 *   plinth, like the Pantheon's intermediate block between the portico and the rotunda: an architrave
 *   round the opening, a keystone, a socle and a cornice returning to the drum.
 *
 * Light nothing inside. At night: the drum and the dome glow (M_MistyGlass × max(Daylight, the night
 * floor)); the lantern glows warm; uplights on the fins, washes from the cornice onto the dome, and a
 * light in the lantern on the crown, all at LightKelvin, following MPC_Musee's Daylight.
 *
 * Watertight by construction: every solid is closed (the mouldings' backs 2 cm inside what they stand
 * on), surfaces meet by running into each other, and no two faces lie closer than 1 cm unless they meet.
 * UV0 in metres. The geometry is rebuilt at BeginPlay as well as on construction, unless the actor has
 * been baked (Geometry/MuseeBake.h): then Stone, Steel, Gilt and Glaze are static meshes, and Skin, whose
 * glowing instances are made at BeginPlay, stays procedural (musee.nobake). The actor is musee.bakeable:
 * it ticks only to dim its own lights and the lantern's glass.
 */
UCLASS()
class MUSEEVISION_API AElanExteriorStructure : public AActor
{
	GENERATED_BODY()

public:
	AElanExteriorStructure();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** The best views of the exterior, by day and by night (plan metres; see FElanExteriorViewpoint). */
	UFUNCTION(BlueprintPure, Category = "Musee|Elan")
	static TArray<FElanExteriorViewpoint> GetViewpoints();

	/** How strongly the night lights are on now: 0 by day … 1 at night. */
	UFUNCTION(BlueprintPure, Category = "Musee|Elan")
	float GetNightLevel() const { return NightLevel; }

	/** Section 0: the plinth and the portal's body (StoneMaterial). 1: the entablature and the portal's mouldings. With collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Stone;

	/** Section 0: the drum's order (fins, pedestals, mullions, transoms). 1: the dome's lattice, or its pearl shell. 2: the lantern's colonnettes and cap. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Steel;

	/** The crown ring and the finial. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Gilt;

	/** Section 0: the drum's misty glass. 1: the dome's. 2: the lantern's glass. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Skin;

	/** Section 0: the drum's clear outer pane. 1: the dome's. No shadows. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Glaze;

	/** On the plinth's coping at each fin's foot, grazing it up to the cornice. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<USpotLightComponent>> FinUplights;

	/** On the cornice at each bay's centre, washing the dome's lattice. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<USpotLightComponent>> DomeWashes;

	/** In the lantern, on the crown (no shadows: the lantern's glass would hold it in). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UPointLightComponent> CrownLight;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> StoneMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> MouldingMaterial;

	/** Pearl-white steel, as the Atrium's ribs. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> SteelMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> GiltMaterial;

	/** M_MistyGlass (Luminance, NightFloor): each skin gets its own instance at play. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> SkinMaterial;

	/** Warm glass for the lantern (M_OpalGlobe: Luminance); M_MistyGlass if it isn't there. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> LanternMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> GlazeMaterial;

	/** MPC_Musee: its Daylight (0 at night … 1 at noon) turns the lights on at night and dims the lantern. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialParameterCollection> Parameters;

	/**
	 * The drum's misty glass seen from outside (cd/m²): at noon, and at night. Outdoors a sunlit wall
	 * is ~15 000 nits, so the drum reads as pale frosted glass by day, a lantern by night.
	 */
	UPROPERTY(EditAnywhere, Category = "Musee|Glow")
	float DrumDayNits = 4500.f;

	UPROPERTY(EditAnywhere, Category = "Musee|Glow")
	float DrumNightNits = 25.f;

	/** The dome: a pearl by day (brighter than the drum), a soft glow at night. */
	UPROPERTY(EditAnywhere, Category = "Musee|Glow")
	float DomeDayNits = 7000.f;

	UPROPERTY(EditAnywhere, Category = "Musee|Glow")
	float DomeNightNits = 45.f;

	/** The lantern's warm glass: pale by day, the crown's light at night. */
	UPROPERTY(EditAnywhere, Category = "Musee|Glow")
	float LanternDayNits = 4000.f;

	UPROPERTY(EditAnywhere, Category = "Musee|Glow")
	float LanternNightNits = 500.f;

	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float LightKelvin = 3800.f;

	/** Each fin's uplight (cd on its axis): ~40 lux on the fin 10 m up. */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float FinUplightCandela = 4000.f;

	/** Each dome wash (cd on its axis): ~50 lux on the lattice. */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float DomeWashCandela = 5000.f;

	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float CrownLightCandela = 600.f;

	/** Daylight at which the night lights start to fade, and at which they are off. */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float NightLightsFadeFrom = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float NightLightsOffAt = 0.35f;

	/** The frontispiece where the Hall of Light meets the plinth; off leaves the plinth round (and open over the hall). */
	UPROPERTY(EditAnywhere, Category = "Musee|Build")
	bool bPortal = true;

	/** Build the exterior's own misty glass over the drum (off: the interior's r 14 glass shows). */
	UPROPERTY(EditAnywhere, Category = "Musee|Build")
	bool bDrumSkin = true;

	/** The clear outer panes over the drum and the dome (reflections of the sky). */
	UPROPERTY(EditAnywhere, Category = "Musee|Build")
	bool bGlaze = true;

	/** The lantern, its cap and the ball (off: the crown ring alone, an oculus ring). The lattice dome's only. */
	UPROPERTY(EditAnywhere, Category = "Musee|Build")
	bool bLantern = true;

	/** The dome as one seamless opaque pearl shell (a gilt ring at its foot, a gilt ball at its top) in place of the
	 *  misty glass under a lattice with its lantern. */
	UPROPERTY(EditAnywhere, Category = "Musee|Build")
	bool bPearlShell = true;

	/** The shell's pearl panels (Scripts/exterior_materials.py: MI_Ext_Pearl); SteelMaterial if it isn't there. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> ShellMaterial;

private:
	void Build();
	void ApplyMaterials(bool bInstances);
	void PlaceLights();
	void AddTags();
	void SetNight(float Level, float Daylight);

	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> DrumGlow;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> DomeGlow;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> LanternGlow;
	UPROPERTY(Transient) TObjectPtr<UMaterialParameterCollection> LoadedParameters;

	float NightLevel = 0.f;
	float LastDaylight = -1.f;
};
