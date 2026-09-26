#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlbionLamps.generated.h"

class UBoxComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMaterialParameterCollection;
class UPointLightComponent;
class UPostProcessComponent;
class UProceduralMeshComponent;

/**
 * Albion's night light: electroliers of the 1880s–90s, as a museum court was first lit by electricity (the gasolier's
 * form, a turned bronze body with scrolled arms, carrying incandescent lamps in opal globes). Plan metres (AlbionPlan).
 *
 * - Twelve electroliers, one in each bay of the two arcades (x ±6, the bays' centres), hung from the crown of the
 *   arcade's iron arch (its soffit flange at 9.54 m) by a clamp and a chain to a turned stem; the body at 5.85 m carries
 *   six scrolled arms, each ending in a spun gallery that holds a cased opal globe (Ø 17 cm, M_GlobeLamp, as the
 *   Reserve's pendants) hanging below it; a corona ring ties the arms. Nothing hangs over the nave's axis: the view from
 *   the gilt sun to the tapestry stays clear, and between the arcades the fittings light the nave and the aisles alike.
 * - One smaller electrolier (four arms) in the porch, from its oak ceiling (6.9 m).
 * - Each fitting's light is one point light at its globes' centre (2900 K, the museum's incandescent lamp; the globes'
 *   lumens summed; its source about a globe's size), shadowed by the building. They and the globes' glow come on at dusk
 *   (MPC_Musee Daylight) and go out by day. The chains and stems cast shadows (in the sun too); the bodies, arms and
 *   galleries round the lamp don't (they would shadow their own light).
 * - At night the court is balanced towards its lamps (3900 K) and metered as the rest of the museum, by
 *   a post-process volume over the court and porch that fades in with the night (by day exposure.py's zone holds it).
 *
 * Placed at the origin by Scripts/native.py (musee.nobake: the fittings stay procedural so their globes can dim).
 */
UCLASS()
class MUSEEVISION_API AAlbionLamps : public AActor
{
	GENERATED_BODY()

public:
	AAlbionLamps();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** The chains, clamps and stems (cast shadows). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Hangers;

	/** The bodies, arms, corona rings and galleries (no shadow: they sit round their own light). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Bodies;

	/** The opal globes (lit at night; no shadow). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Globes;

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TArray<TObjectPtr<UPointLightComponent>> Lights;

	/** The court and porch, for the night's white balance. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UBoxComponent> Court;

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UPostProcessComponent> NightBalance;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> BronzeMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> GlobeMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialParameterCollection> DaylightParameters;

	/** One lamp's lumens (a 60 W-equivalent incandescent in its opal globe) and its colour. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float LampLumens = 520.f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float LampKelvin = 2900.f;

	/** The white balance at night, towards the 2900 K lamps (warmer honey stone than the Reserve's brick: a little further). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float NightWhiteTemp = 3900.f;

	/** At night the court meters as the rest of the museum does (setup_project.EXPOSURE_BIAS; by day Albion's exposure
	 *  zone, exposure.py, holds its sunlit glass court down). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float NightExposureBias = 0.9f;

	/** The lamps' world positions (cm), for the checks. */
	UFUNCTION(BlueprintPure, Category = "Musee|Albion")
	TArray<FVector> GetLampPositions() const;

private:
	void Build();
	void SetNight(float Night);
	float LastNight = -1.f;
	bool bLastNear = true;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GlobeInstance;
};
