#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ElanCar.generated.h"

class UBoxComponent;
class UMaterialInstanceDynamic;
class UPrimitiveComponent;
class UProceduralMeshComponent;
class URectLightComponent;

/**
 * The Élan glass car, built natively after Apple's round glass elevators and the Atrium rendering
 * (plan/renderings/07): a cylinder of clear low-iron glass, Ø 4.4 m and 2.6 m high, in four large
 * curved panes of 78° (their clear joints never over a landing's), closed by two curved leaves that
 * slide round the cylinder either side of the west door. Slim bronze rings top and bottom (7 cm), a
 * clear glass roof with a slim warm ring of light under its rim that lights the floor, a lightly
 * frosted glass floor, a slim glass handrail on bronze brackets, and a bronze collar under the floor
 * where the mast meets it.
 *
 * Its frame is the car's floor level on the axis (X east, Y south, Z up); AElanElevator moves it.
 * Every pane is its own primitive, so the renderer sorts the glass pane by pane (nearest last) and
 * never flips two concentric walls. Collision is explicit and simple: a convex floor (the visitor's
 * moving base), thin boxes round the walls with the door gap, and boxes on each leaf that slide with
 * it. The glass ignores the look trace (placards read through it), as the imported glass did.
 */
UCLASS(ClassGroup = Musee)
class MUSEEVISION_API UElanCar : public USceneComponent
{
	GENERATED_BODY()

public:
	UElanCar();

	/** Make the parts (once, at BeginPlay). */
	void Build();

	/** The car's floor level (m) and its speed (m/s, for the visitor riding it). */
	void Place(double FloorHeight, double Speed);
	/** 0 closed … 1 open (already eased). */
	void SetDoors(float Open);
	/** 0 … 1: at the top the glass fades to a rail and a floor. */
	void SetDim(float Amount);
	/** 0 … 1: the canopy brightens a little while the car runs. */
	void SetGlow(float Amount);
	/** Élan Cube: the light left on in the cleared car (a fraction of its full light): enough to find the floor and the rail. */
	void SetGlimmer(float Fraction);

	/** The floor the visitor stands on (their movement base while riding). */
	UPrimitiveComponent* FloorBody() const;
	int32 ColliderCount() const { return Colliders.Num(); }

	/**
	 * The slim luminous ring under the glass roof's rim (cd/m²). A glow, not a lamp: brighter, it clips
	 * under the Square's exposure and its edge shimmers. The rect light does the lighting.
	 */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float RingNits = 250.f;

	/** The roof's light onto the floor (lm), a neutral white. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float LightLumens = 3500.f;

	/** Neutral (the white balance is daylight's): at 3100 K the car filled the pearl neck with amber on the way up. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float LightKelvin = 5000.f;

	/** Clear low-iron glass: how much it takes out of the view behind, and how strongly it reflects. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float GlassOpacity = 0.13f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float GlassSpecular = 0.9f;

private:
	void ApplyGlow();

	/** Panes, leaves and the glass roof: they fade at the top. */
	UPROPERTY(Transient) TArray<TObjectPtr<UPrimitiveComponent>> FadingGlass;
	/** The top ring and the ring of light: hidden once the glass has faded. */
	UPROPERTY(Transient) TArray<TObjectPtr<UPrimitiveComponent>> DimHidden;
	UPROPERTY(Transient) TArray<TObjectPtr<USceneComponent>> LeafPivots;
	UPROPERTY(Transient) TArray<TObjectPtr<UBoxComponent>> Colliders;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> FloorMesh;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> CanopyMesh;
	UPROPERTY(Transient) TObjectPtr<URectLightComponent> CarLight;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> ClearGlass;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> RailGlass;
	bool bUnderground = false;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> FloorGlass;   // the frosted floor (Élan Cube: its diffuse goes at the stop)
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> RingGlow;
	/** Élan Cube: cleared in the dark room, the floor's frost and the rail read by what they scatter of the car's glimmer
	 * (M_CubeFrost, lit translucency; thin glass has no diffuse to show them by). */
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> RailMesh;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> FrostFloor;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> FrostRail;
	bool bFrostShown = false;

	bool bBuilt = false;
	float AppliedOpen = -1.f;
	float AppliedDim = -1.f;
	float AppliedGlow = 0.f;
	float Glimmer = 0.03f;
};

/**
 * A landing enclosure round the shaft (the Atrium's and the Square's): the same glass as the car,
 * on a circle 25 cm outside it (R 2.45 m), so the two walls never come near enough to fight. Five
 * panes, two curved leaves that open only when the car is here, a bronze shoe and head ring, and
 * a curved bronze sill across the door. Over it a clear glass tube carries the car's way on up, with
 * thin bronze rings about every 2.6 m: at the Atrium to a gilt collar under the ring round the
 * Sphere's opening (the rendering's tube), at the Square to the ceiling.
 */
UCLASS(ClassGroup = Musee)
class MUSEEVISION_API UElanLanding : public USceneComponent
{
	GENERATED_BODY()

public:
	UElanLanding();

	/**
	 * Make the parts: the enclosure Height (m) high. TubeTop > Height carries the glass tube on up to
	 * TubeTop, ending in a gilt collar out to CollarRadius (under the ring round the Sphere's opening),
	 * or, with CollarRadius 0, in a ring into a ceiling.
	 */
	void Build(double Height, double TubeTop, double CollarRadius);

	/** 0 closed … 1 open (already eased). */
	void SetDoors(float Open);
	/** Whether the look trace stops on it (so clicking the glass calls the car). */
	void SetCallable(bool bCall);

	int32 ColliderCount() const { return Colliders.Num(); }

	UPROPERTY(EditAnywhere, Category = "Musee")
	float GlassOpacity = 0.09f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float GlassSpecular = 0.9f;

private:
	UPROPERTY(Transient) TArray<TObjectPtr<UPrimitiveComponent>> Panes;
	UPROPERTY(Transient) TArray<TObjectPtr<USceneComponent>> LeafPivots;
	UPROPERTY(Transient) TArray<TObjectPtr<UBoxComponent>> Colliders;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> ClearGlass;

	bool bBuilt = false;
	bool bCallableNow = false;
	float AppliedOpen = -1.f;
};
