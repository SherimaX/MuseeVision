#pragma once

#include "CoreMinimal.h"
#include "Cube/JourneyScene.h"
#include "JourneyMatterhorn.generated.h"

class UProceduralMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Journey I, the Matterhorn (plan/boards/ElanCubeJourneys): a day on the mountain, dawn to night, in six places along
 * the Hörnli route. The land is swisstopo's (swissALTI3D, SWISSIMAGE; open data, Swiss OGD) and, beyond Switzerland,
 * the Copernicus DEM, built into Nanite tiles by Scripts/journey_terrain.py, journeys.py and MuseeJourneyLibrary.
 *
 * The tiles are in LV95 metres (the mesh's origin its tile's north-west corner, heights above the sea, the Earth's
 * curvature taken off relative to the summit). A place puts its point of view (the car's floor over the ground there)
 * at the eyes: the land hangs from a pivot at the eyes that tilts it by the curvature between the summit and the place
 * (a few hundredths of a degree: the far peaks sit as low as they do from there).
 *
 * Places: 1 Riffelsee at dawn (the lake just under the car), 2 inside a crevasse of the Theodul glacier, 3 beside the
 * Hörnli hut, 4 on the ridge by the Solvay hut, 5 the summit at sunset, 6 the summit at night.
 */
UCLASS()
class MUSEEVISION_API UJourneyMatterhorn : public UJourneyScene
{
	GENERATED_BODY()

public:
	virtual bool IsAvailable() const override;
	virtual void Setup(AElanJourney* InDirector) override;
	virtual void OpenPlace(int32 Index) override;
	virtual void TickPlace(float DeltaSeconds, double LocalHour, float Fraction, float Veil) override;
	virtual void Teardown() override;

	struct FView
	{
		double E = 0, N = 0;     // LV95
		double Floor = 0;        // the car's floor, curvature taken off as in the tiles (m)
		bool bLand = true;       // the land shows (not in the crevasse)
	};
	static const FView& View(int32 Index);

private:
	void BuildLand();
	void PlaceLand(const FView& V);
	void BuildLake();
	void BuildRidge();
	void BuildBoulders();

	UPROPERTY(Transient) TObjectPtr<USceneComponent> Pivot;
	UPROPERTY(Transient) TObjectPtr<USceneComponent> Land;
	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Tiles;
	UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> Lake;
	UPROPERTY(Transient) TObjectPtr<class UInstancedStaticMeshComponent> Lamps;
	UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> LampGlow;
	/** Snow falling in the Cube (the last place's end) and spindrift blowing off the ridge (place 4). */
	UPROPERTY(Transient) TObjectPtr<class UInstancedStaticMeshComponent> Flakes;
	TArray<FVector4> FlakeState;   // position (m, the eyes' frame) and a random
	void MoveFlakes(float DeltaSeconds, float Amount, const FVector& Wind);
	int32 Current = -1;
	int32 FarFieldWas = -1;
};
