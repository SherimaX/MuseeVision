#pragma once

#include "CoreMinimal.h"
#include "Nature/MuseeNatureActor.h"
#include "MuseeMeadow.generated.h"

/**
 * The Hall of Light's meadow flowers and bulbs (Shared/Plan/HallOfLightPlanting.swift, placed by
 * buildHallGardens), and the meadow's grass. Each plotted point is a plant: stems with leaves and
 * flower heads of its bed's colour, the flower chosen by the colour: field scabious (violet),
 * musk mallow (pink), a terracotta helenium, buttercups (yellow); in the orchard's spring bulbs,
 * narcissi (white), tulips (pink) and daffodils (yellow). The season follows the Swift: in
 * summer the meadow flowers and the bulbs are over; in autumn half the meadow still flowers,
 * lower; in winter there are brown seed heads.
 *
 * Grass: tufts of blades (with a few flowering grass stems) scattered over GrassMin … GrassMax.
 */
UCLASS()
class MUSEEVISION_API AMuseeMeadow : public AMuseeNatureActor
{
	GENERATED_BODY()

public:
	AMuseeMeadow();

	/** The plotted beds (points in metres from the actor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	TArray<FMuseeFlowerBed> Beds;

	/** Where the grass grows, metres from the actor (x east, y south); an empty box = no grass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	FVector2D GrassMin = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	FVector2D GrassMax = FVector2D::ZeroVector;

	/** Grass tufts per square metre. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature", meta = (ClampMin = "0", ClampMax = "12"))
	float GrassDensity = 2.5f;

	/** Keep the grass off these boxes (min x, min y, max x, max y, metres from the actor): paths, tree bases. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	TArray<FVector4> GrassHoles;

protected:
	virtual void BuildPlant() override;
};
