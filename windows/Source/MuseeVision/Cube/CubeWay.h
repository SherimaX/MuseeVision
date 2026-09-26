#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CubeWay.generated.h"

class UElanIris;
class USpotLightComponent;
class UMaterialInterface;
class UProceduralMeshComponent;

/**
 * The car's way into the Cube (Cube/CubePlan.h), the moving parts the elevator drives: the iris in the Cube's ceiling
 * (the Élan kit's seven-bladed iris, as in the neck), and the gilt mast the car hangs from below it.
 *
 * The mast is a spiral-band column (a Spiralift: two steel bands that interlock into a rigid tube as they leave a
 * drum, and part again going back in), clad in gilt. Its drum is the car's crown. Coming down the shaft on its rails
 * the car carries the mast's head on the crown; it stops with the head in the iris's plane, the iris closes on the
 * head's groove (the mast clamp: seven blades round a 0.39 m neck), and the crown then feeds the column out: the car
 * sinks on it to the centre. Going up, the column is taken back in until the head sits on the crown again, the iris
 * opens and lets it go, and the car climbs the rails.
 *
 * Its frame is the elevator's (the Atrium floor on the axis). Never baked.
 */
UCLASS(ClassGroup = Musee)
class MUSEEVISION_API UElanCubeWay : public USceneComponent
{
	GENERATED_BODY()

public:
	UElanCubeWay();

	void Build();

	/** The car's floor level (m) and the iris's opening (0 shut … 1 open; CubePlan::IrisClampOpen round the head). */
	void Apply(double CarY, float IrisOpen);

	/** The car's floor level at which the head's groove lies in the iris's plane. */
	static double ClampLevel();

	/** Hidden with the building (in the Sphere, and while a journey shows). */
	void SetShown(bool bShow);
	/** The mast and its head stay while the rest of the building is hidden (a journey). */
	void SetMastOnly(bool bOnly);
	/** While a piece plays the mast's lights go out (the sea is lit only where something moves). */
	void SetDark(bool bNow);

private:
	UPROPERTY(Transient) TObjectPtr<UElanIris> Iris;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Head;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Column;
	/** Four small spots recessed in the gilt soffit ring, aimed down at the mast: in the black room they are what shows
	 * the mast and the iris (on only while the car is in the Cube). */
	UPROPERTY(Transient) TArray<TObjectPtr<USpotLightComponent>> MastLights;
	bool bLightsOn = true;
	bool bBuilt = false;
	bool bShown = true;
	bool bMastOnly = false;
	bool bDark = false;
	double AppliedY = 1e9;
	float AppliedIris = -1.f;
};
