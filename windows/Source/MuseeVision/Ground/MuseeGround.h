#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MuseeGround.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

/**
 * The site's ground: the imported 800 m grass plane (/Museum/HallOfLight/Ground), rebuilt natively with
 * openings wherever a way goes down through it: the Rotunda's sun-clock stair shaft (r 7.62 m, just outside
 * the shaft's outer face at 7.6), the Élan car's shaft under the Atrium (r 2.6 m round (54, 0): the Atrium's
 * floor stops at r 2.29), and the lily pond's opening in the Nymphéas oval, down the long stair to the Reserve
 * (x −90.45 … −82.85, y ±1.25: the oval floor's opening is x −90.4 … −82.9, y ±1.2). Each hole is a little
 * larger than the floor's own, so its edge stays under the floor. A constrained Delaunay triangulation of the
 * square with the holes cut out, plus a 20 m lattice of points so no triangle is hundreds of metres long.
 * Place it at the world origin. UVs in metres.
 */
UCLASS()
class MUSEEVISION_API AMuseeGround : public AActor
{
	GENERATED_BODY()

public:
	AMuseeGround();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** The imported plane it replaces. */
	UFUNCTION(BlueprintPure, Category = "Musee")
	static TArray<FString> GetReplacedImportPrims();

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Ground;

	/** Half the side of the square plane (cm). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float HalfSize = 40000.f;

	/** The opening over the stair shaft (cm), just outside the shaft's outer face at 760. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float HoleRadius = 762.f;

	/** The opening over the Élan car's shaft (cm), round (54 m, 0). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float ElanHoleRadius = 260.f;

	/** Height of the plane (cm): as the imported one, just under the floors. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float Height = -3.f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> GroundMaterial;

private:
	void Build();
};
