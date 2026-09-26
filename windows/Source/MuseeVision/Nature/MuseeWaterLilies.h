#pragma once

#include "CoreMinimal.h"
#include "Nature/MuseeNatureActor.h"
#include "MuseeWaterLilies.generated.h"

class USphereComponent;

/**
 * Water plants on a pond, with the actor at the water's surface.
 *
 * WaterLily (the Nymphéas oval's pond, Plan.Oval.lilies; Shared/Wings/Reserve.swift, buildPond):
 * Nymphaea pads, round with the radial slit, a little cupped, some with an upturned rim, in
 * colonies that overlap; flowers of layered pointed petals opening in rings over green sepals, a
 * yellow stamen crown and stigma disc, sitting on the water; closed buds. Pink and white, and the
 * golden lily (PondPlan.goldenLily), which glows a little. The actor is attached to the lifting
 * pond (/Museum/Reserve/Lily_pond), so it rises with it; GoldenTarget lets the look trace find
 * the golden lily (the pond's switch).
 *
 * Lotus (the Chinese garden's pond, ChinesePlan.lotus): Nelumbo leaves, round and peltate, some
 * floating and some held above the water on stalks, cupped with wavy rims; pink flowers on tall
 * stalks with a seed-head receptacle, and buds. It follows the solar term as the Swift does:
 * leaves from Lixia to Lidong (withered from Shuangjiang), flowers from Mangzhong to Chushu,
 * seed heads after.
 */
UCLASS()
class MUSEEVISION_API AMuseeWaterLilies : public AMuseeNatureActor
{
	GENERATED_BODY()

public:
	AMuseeWaterLilies();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	EMuseeWaterPlant Kind = EMuseeWaterPlant::WaterLily;

	/** The plants where the plan puts them (metres from the actor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	TArray<FMuseeLilySpec> Plants;

	/** Keep the extra pads inside the basin: an ellipse about the actor (radii, metres); 0 = no limit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	FVector2D BasinRadii = FVector2D::ZeroVector;

	/** Or inside this outline (metres from the actor), e.g. the Chinese pond's edge. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	TArray<FVector2D> PondOutline;

	/** How many extra pads grow round each plant (the colony), relative to the usual. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature", meta = (ClampMin = "0", ClampMax = "3"))
	float Fullness = 1.f;

	/** The golden lily, for the look trace (visibility only; visitors walk through it). */
	UPROPERTY(VisibleAnywhere, Category = "Nature")
	TObjectPtr<USphereComponent> GoldenTarget;

protected:
	virtual void BuildPlant() override;
	virtual bool IsMovablePlant() const override { return true; }
};
