#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Visitor/MuseeInteractable.h"
#include "SunClock.generated.h"

class AMuseeCharacter;
class UMaterialInterface;
class UProceduralMeshComponent;
class USpotLightComponent;
class UPointLightComponent;

/**
 * The Rotunda's sun clock, native (MuseePlan::SunClock; the geometry in SunClockBuild.h). Place it at
 * the origin (the Rotunda's centre); it replaces the imported floor disc (GetReplacedImportPrims).
 *
 * The floor: a fixed ring of pale marble slabs (r 7–10.3) with the motto HORAS · NON · NUMERO · NISI
 * · SERENAS in bronze capitals, read from the ring; inside it the dial (r 0–7): marble bands, a
 * bronze hour ring on the shadow's circle (r 6.78), hour lines and minute ticks from VI through XII to
 * VI, bronze numerals, and a sunburst in relief at the centre, all cast 2–8 mm proud with bevels.
 *
 * The dial is the top of a marble drum. "Raise the sun clock" (from the ring) lifts it 3.5 m in 8 s:
 * a tempietto with fluted pilasters and a round-arched door facing the Salon, and inside it a wide
 * spiral stair (40 risers of 150 mm, a mid landing) down 6 m round an open well to a landing with a
 * marble rosette, which opens north (a port 3.6 × 4.4 m at r 7.6) into the classical hall. "Lower the
 * sun clock" (from the ring, outside the drum) settles it again. It never lowers while you are inside
 * it or below, and never lifts you: on the dial there is nothing to press.
 *
 * Its light (3800 K, all components of this actor, none by any art): twelve spots concealed in the
 * drum's coffers over the treads, one down the well onto the rosette, and a soft fill without a fitting
 * high in the well for the coffers and walls; they rise with the drum and are dark while it is down.
 * Daylight comes in through the door.
 *
 * The mechanism. Lowered, the drum hangs in its pocket (r 6.54–7.0, down to −3.58): its wall slides
 * between the stair's wall (the sleeve, r 6.48–6.54) and the pocket's face with 5 and 10 mm to spare, the
 * sleeve's top standing in a groove of the bronze rim, and the dial's slab (its ribs at −0.13, its rim at
 * −0.14) covers the stair: the top landing (−0.15) and the string's top (never above −0.14) pass under
 * it. Everything else that stands on the stair must too, so the balustrade is made in two kinds:
 *   - Below the split (tread 8's nosing, the handrail's top there at −0.23) it is fixed; so is the wall's
 *     handrail, which starts a tread higher (its top at −0.18), returned to the wall on a rose. The top
 *     flight's wall side has no rail: the well side guards it.
 *   - Along the upper flight's well side and across the top landing's back edge it retracts: each panel
 *     (standards, balusters, rings, handrail, a carrier bar at its foot) stands in the slot of a hollow
 *     housing hung on the stair, 1.2 m deep: marble sides (sunk fields where they are seen) bound in
 *     bronze, a cap round the slot, a shoe, end plates (along the string, r 2.07–2.20, its top the
 *     string's; behind the landing, the landing's). While the dial is down the panel sits 0.94 m lower,
 *     wholly inside, its handrail just under the slot's lips; it is driven from the lift through a cam
 *     with a dwell at each end: it stays in its housing until the dial is 1.2 m up, rises with the drum
 *     over the next 0.94 m (the ribs 1.24 m over its handrail all the way) and then stands. Lowering,
 *     it is home before the drum comes near, so nothing rides under the moving ceiling.
 * SunClockChecks.cpp tests every moving part (the drum, its bronze, the lights' fittings, the panels)
 * against the fixed ones, triangle against triangle, down, part-way and up.
 *
 * Collision: the ring, the dial and the drum (it moves with the drum), the shaft, the stair's string
 * and underside, and a hidden ramp through the treads' middles that the visitor walks on; hidden
 * fences along the balustrade (the retracting panels' in their plane, moving with them).
 *
 * The bake (Geometry/MuseeBake.h) may turn the fixed parts (ring, shaft, stair, the fixed rails and
 * the housings) into static meshes; the drum, its bronze, the retracting panels and the hidden
 * walkway stay procedural.
 */
UCLASS()
class MUSEEVISION_API ASunClock : public AActor, public IMuseePromptProvider
{
	GENERATED_BODY()

public:
	ASunClock();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** USD paths of the imported prims this actor replaces: the Rotunda's floor disc. Retire them (and their collision). */
	UFUNCTION(BlueprintPure, Category = "Musee|SunClock")
	static TArray<FString> GetReplacedImportPrims();

	/** How high the drum stands (m, 0 … 3.5). */
	UFUNCTION(BlueprintPure, Category = "Musee|SunClock")
	float GetLiftHeight() const;

	/** Fully up. */
	UFUNCTION(BlueprintPure, Category = "Musee|SunClock")
	bool IsRaised() const;

	/** The landing at the stair's foot, in front of the port (world cm, on the floor). */
	UFUNCTION(BlueprintPure, Category = "Musee|SunClock")
	FVector GetLandingPoint() const;

	/** Testing: sets the lift at once (0 down … 1 up), without the checks; part-way, it holds there (no prompts) until set again. */
	void SetLiftFraction(float Fraction);

	// IMuseePromptProvider: "Raise the sun clock" / "Lower the sun clock", from the ring.
	virtual void GetPrompts(const AMuseeCharacter* Visitor, TArray<FMuseeActionPrompt>& Out) const override;
	virtual void RunPrompt(AMuseeCharacter* Visitor, FName Id) override;

	static const FName RaisePromptId;   // sunclock.raise
	static const FName LowerPromptId;   // sunclock.lower

	/** The dial and the drum (they move: never baked). Sections: 0–3 the dial's stone (A, B, rosso, nero), 4 the drum's stone, 5 the soffit. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Drum;

	/** On the drum (never baked): 0 the bronze (inlay, numerals, rays, rim), 1 the gilt (the boss, the pendant). No collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> DrumBronze;

	/** The fixed ring: 0, 1 the slabs' two tones. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Ring;

	/** 0 the motto's bronze, 1 its gilt stops. No collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> RingBronze;

	/** 0 the masonry (the stair's wall, the pocket, the port), 1–3 the landing's floor (pale, rosso, nero), 4 the port's dressings (archivolt, transoms). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Shaft;

	/** 0 the treads (no collision), 1 the inner string, 2 the underside and ends. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Stair;

	/** The fixed rails: 0 the bronze (the balustrade below the split, the newel, the wall's rail); the retracting panels' housings, 1 their bronze tops (the slots), 2 their marble sides. No collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Rails;

	/** The retracting balustrade: 0 its bronze (PanelDrop down, in its housings, while the dial is down), 1 its hidden fence. Moves: never baked. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> TopRails;

	/** Hidden, collision only, never baked: 0 the ramp the visitor walks on down the stair, 1 the fence along the balustrade below the split. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Walkway;

	/** Concealed in the drum's coffers over the treads. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<USpotLightComponent>> CoveLights;

	/** Under the pendant boss, down the well onto the rosette. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<USpotLightComponent> WellLight;

	/**
	 * A soft fill with no fitting, high in the open well (it rises with the drum): the coffers, the pendant and
	 * the drum's walls, which the cove spots (pointing down) leave dark. The drum can carry no trough for a
	 * cove: lowered, everything inside its wall must clear the stair.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UPointLightComponent> FillLight;

	/** The dial's, the ring's and the treads' pale marble (UVs in metres), and its second tone. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> MarbleMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> MarbleAltMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> MarbleFallbackMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> RossoMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> NeroMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> DarkFallbackMaterial;

	/** The drum's wall (moves: its veining comes from its own UVs, no world-space joints). */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> DrumMaterial;

	/** The shaft's masonry, the stair's underside. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> WallMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> SoffitMaterial;

	/** Satin bronze (roughness about 0.4). */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> BronzeMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> GiltMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> GiltFallbackMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float LightKelvin = 3800.f;

	/** Each cove spot (cd): about 200 lux on the treads halfway down, 100 on the landing. */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float CoveCandela = 4500.f;

	/** The well's spot (cd): about 150 lux on the rosette, 9.4 m below. */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float WellCandela = 13000.f;

	/** The fill (cd): about 250 lux on the coffers over it, 30–40 on the drum's walls. */
	UPROPERTY(EditAnywhere, Category = "Musee|Light")
	float FillCandela = 1500.f;

private:
	enum class EState : uint8 { Down, Rising, Up, Lowering, Held };

	void Build();
	void ApplyMaterials();
	void PlaceLights();
	void Apply();
	void AddTags();

	/** The visitor's feet in this actor's frame (m). */
	FVector LocalFeet(const AMuseeCharacter* Visitor) const;
	bool OnRing(const AMuseeCharacter* Visitor) const;
	bool OnDial(const AMuseeCharacter* Visitor) const;
	/** In the drum's footprint, or below the floor: never lower then. */
	bool InsideOrBelow(const AMuseeCharacter* Visitor) const;

	EState State = EState::Down;
	double Progress = 0;   // 0 … 1 of the lift, before easing
};
