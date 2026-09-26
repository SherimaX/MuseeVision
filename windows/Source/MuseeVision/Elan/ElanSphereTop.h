#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ElanSphereTop.generated.h"

class UBoxComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPrimitiveComponent;
class UPointLightComponent;
class UProceduralMeshComponent;
class URectLightComponent;

/**
 * An iris: a camera's diaphragm in the horizontal plane (ElanKit: IrisBlades curved blades with round ends,
 * each on a pin in the seat ring). Open, the blades lie hidden in the ring round the car's way; closing, each
 * turns on its pin and sweeps in along a curve, and they shut in a spiral, the top blade just under the
 * ring's face. Used three times: in the tube's collar (the Atrium's side of the neck), in the floor ring at
 * the neck's mouth in the Sphere, and in the platform at the Sphere's centre (stone on top: closed, the
 * platform is a full disc to walk on).
 */
UCLASS(ClassGroup = Musee)
class MUSEEVISION_API UElanIris : public USceneComponent
{
	GENERATED_BODY()

public:
	UElanIris();

	/**
	 * The blades under this component's origin, stacked down. bWalkable: a floor to stand on while closed.
	 * LineMaterial: a hairline inlay along each blade's long edges on its top, so the shut iris reads as a spiral.
	 */
	void Build(UMaterialInterface* TopMaterial, UMaterialInterface* UnderMaterial, UMaterialInterface* LineMaterial, bool bWalkable);

	/** 0 closed … 1 open (linear; eased here). */
	void SetOpen(float Open);

private:
	UPROPERTY(Transient) TArray<TObjectPtr<USceneComponent>> Pivots;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> FloorBody;

	bool bBuilt = false;
	float AppliedOpen = -1.f;
};

/**
 * The neck: the airlock between the building and the Sphere. A dark bronze throat, coffered with gilt
 * rings and ribs and girdled by a ring of warm light, from the tube's collar under the Sphere's opening
 * (≈ 7.72 m) up to a bronze floor ring inside the Sphere (11.40 m), the Sphere's floor opening. An iris
 * seals each end: the car stops in between, the way behind closes, the world outside changes unseen,
 * and the way ahead opens.
 */
UCLASS(ClassGroup = Musee)
class MUSEEVISION_API UElanNeck : public USceneComponent
{
	GENERATED_BODY()

public:
	UElanNeck();

	void Build();
	/** 0 closed … 1 open: the Atrium's side (under the collar) and the Sphere's (the floor opening). */
	void SetIrises(float Bottom, float Top);

	UPROPERTY(EditAnywhere, Category = "Musee")
	float RingNits = 350.f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float LightLumens = 220.f;

private:
	UPROPERTY(Transient) TObjectPtr<UElanIris> BottomIris;
	UPROPERTY(Transient) TObjectPtr<UElanIris> TopIris;
	bool bBuilt = false;
};

/**
 * The platform at the Sphere's centre: a round floor of pale stone (r 2.36 to 5.5 m) level with the
 * car's floor at the stop, with a slim bronze balustrade and glass infill, lit from under its rail,
 * carried on eight bronze struts from the neck's floor ring below. The car stands in its middle; sent
 * down, it sinks out of view and the platform's iris closes over its way, flush: a full disc.
 * Collision: the floor, the balustrade, the iris while closed, and a guard round the car's way while it
 * is open with no car at the doors.
 */
UCLASS(ClassGroup = Musee)
class MUSEEVISION_API UElanPlatform : public USceneComponent
{
	GENERATED_BODY()

public:
	UElanPlatform();

	void Build();
	/** 0 closed … 1 open. */
	void SetIris(float Open);
	/** The guard round the car's way (on while the way is open and no car stands at the doors). */
	void SetGuard(bool bOn);
	/** Its lights, on while the visitor is in the Sphere. */
	void SetLit(bool bOn);

	UPROPERTY(EditAnywhere, Category = "Musee")
	float RailGlowNits = 400.f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float LightLumens = 380.f;

	/** The soft fill from high above the platform (lm). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float FillLumens = 3000.f;

private:
	UPROPERTY(Transient) TObjectPtr<UElanIris> Iris;
	UPROPERTY(Transient) TObjectPtr<URectLightComponent> Fill;
	UPROPERTY(Transient) TArray<TObjectPtr<UPointLightComponent>> Lamps;
	UPROPERTY(Transient) TArray<TObjectPtr<UBoxComponent>> Guard;
	UPROPERTY(Transient) TArray<TObjectPtr<UBoxComponent>> Colliders;
	UPROPERTY(Transient) TArray<TObjectPtr<UPrimitiveComponent>> Glass;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> ClearGlass;
	bool bBuilt = false;
	bool bGuardOn = true;
	bool bLitNow = true;
};
