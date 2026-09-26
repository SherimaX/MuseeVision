#pragma once

#include "CoreMinimal.h"
#include "Cube/JourneyScene.h"
#include "JourneySea.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UProceduralMeshComponent;
class UStaticMesh;

/**
 * Journey II, Raja Ampat (plan/boards/ElanCubeJourneys): a day under the sea, morning to night, in six places, from
 * the surface at Piaynemo to the wall's edge of the deep, and back to the surface at night.
 *
 * The sea: its surface (a wide disc of waves, a mirror by Fresnel above and by total internal reflection below, with
 * Snell's window over the eyes) at the place's depth over the eyes; under it the light's path attenuated channel by
 * channel and the sea's own glow scattered in (M_RA_Underwater, a post-process); the sun's light netted by caustics
 * (a light function) and tinted by the water it has come through; god rays in the volumetric fog.
 *
 * The life: SeaGen's beds, corals, sponges, sea fans, mangroves and karst islets (Nanite, instanced) scattered on the
 * bottom by each place's rules; fishes in schools that circle, ring the car, hover over the corals or cruise past
 * (instances moved every frame, each swimming by its material's wave); reef mantas round their cleaning station;
 * blacktip sharks among the roots; a column of barracuda far out; a pygmy seahorse on the fan beside the car; at night
 * the walking shark on the reef flat and the plankton flashing in the dark.
 */
UCLASS()
class MUSEEVISION_API UJourneySea : public UJourneyScene
{
	GENERATED_BODY()

public:
	virtual bool IsAvailable() const override;
	virtual void Setup(AElanJourney* InDirector) override;
	virtual void OpenPlace(int32 Index) override;
	virtual void TickPlace(float DeltaSeconds, double LocalHour, float Fraction, float Veil) override;
	virtual void AdjustSky(struct FMuseeSkyOverride& Sky, int32 Place) override;
	virtual void ClosePlace() override;
	virtual void Teardown() override;

private:
	/** A group of fishes that move together (see Tick): a school, a ring, a hovering cloud, cruisers, a column. */
	struct FGroup
	{
		UInstancedStaticMeshComponent* Mesh = nullptr;
		int32 Kind = 0;            // 0 circling school, 1 ring round the car, 2 hovering, 3 cruising, 4 column, 5 walking
		FVector Centre = FVector::ZeroVector;   // metres, the eyes' frame
		double Radius = 5, RadiusSpread = 1, Height = 0, HeightSpread = 1, Speed = 0.5, Noise = 0.2;
		TArray<FVector4> Fish;     // per fish: radius offset, height offset, phase, speed factor
	};

	UStaticMesh* Thing(const TCHAR* Name) const;
	UInstancedStaticMeshComponent* Scatter(const TCHAR* Mesh, int32 Count, double RMin, double RMax, double ScaleMin, double ScaleMax,
										   TFunctionRef<bool(const FVector&)> Accept, uint32 Seed, bool bOnWall = false);
	FGroup& School(const TCHAR* Species, int32 Count, int32 Kind, const FVector& Centre, double Radius, double RadiusSpread, double Height,
				   double HeightSpread, double Speed, uint32 Seed, bool bForJourney = false);
	void MoveFish(float DeltaSeconds);
	void BuildSurface();

	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Surface;
	UPROPERTY(Transient) TObjectPtr<UInstancedStaticMeshComponent> Plankton;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> Caustics;
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> Underwater;
	TArray<FGroup> Groups;          // the place's
	TArray<FGroup> JourneyGroups;   // the journey's (the veil's shoal)
	int32 Current = -1;
	double Clock = 0;
	float CurrentVeil = 0.f;
};
