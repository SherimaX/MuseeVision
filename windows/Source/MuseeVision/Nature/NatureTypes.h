#pragma once

#include "CoreMinimal.h"
#include "NatureTypes.generated.h"

/** The four seasons (Shared/Core/Garden.swift, Season): the 24 solar terms in sixes, from Lichun. */
UENUM(BlueprintType)
enum class EMuseeNatureSeason : uint8
{
	Spring,
	Summer,
	Autumn,
	Winter
};

/** The trees of the gardens (Hall of Light, the Chinese Wing). */
UENUM(BlueprintType)
enum class EMuseeTreeSpecies : uint8
{
	/** Silver birch: white bark with dark lenticels, pendulous fine twigs, small triangular leaves. */
	Birch,
	/** A pruned orchard tree, cherry (FruitVariant 0) or apple (1): blossom, leaves, fruit, bare wood. */
	OrchardFruit,
	/** Prunus mume: gnarled, dark, zigzag wood; flowers on the bare wood at the turn of the year. */
	ChinesePlum,
	/** Sweet osmanthus: a dense evergreen small tree, glossy leaves, tiny cream-orange flowers in autumn. */
	Osmanthus,
	/** A clump of Phyllostachys: segmented culms with nodes, paired branches and fine leaf sprays. */
	Bamboo
};

/** The pond plants. */
UENUM(BlueprintType)
enum class EMuseeWaterPlant : uint8
{
	/** Nymphaea: floating pads with the radial slit, flowers opening in rings on the water. */
	WaterLily,
	/** Nelumbo: round peltate leaves, floating and held above the water, flowers on tall stalks. */
	Lotus
};

UENUM(BlueprintType)
enum class EMuseePottedPlantKind : uint8
{
	/** Cymbidium: arching strap leaves and slender flower spikes (flowers at Chunfen). */
	Orchid,
	/** A mounded chrysanthemum with many small flower heads (flowers at Shuangjiang). */
	Chrysanthemum
};

/** One water plant: a pad (or leaf) where the plan puts it, and whether it flowers. */
USTRUCT(BlueprintType)
struct FMuseeLilySpec
{
	GENERATED_BODY()

	/** Metres from the actor, x east, y south. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	FVector2D Position = FVector2D::ZeroVector;

	/** The pad's radius (the lotus leaf's), metres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	float Radius = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	bool bFlower = false;

	/** The golden lily (the pond's switch): a yellow Nymphaea that glows a little. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	bool bGolden = false;

	/** The lotus flower's size (the plan's ellipsoid diameter), metres; 0 = none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	float FlowerSize = 0.f;
};

/** Meadow flowers or bulbs of one colour at plotted points (Shared/Plan/HallOfLightPlanting.swift). */
USTRUCT(BlueprintType)
struct FMuseeFlowerBed
{
	GENERATED_BODY()

	/** The plan's colour, 0xRRGGBB; it also chooses the flower. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	int32 Colour = 0xA08AB0;

	/** The north meadow (true) or the orchard's spring bulbs (false). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	bool bMeadow = true;

	/** Metres from the actor, x east, y south. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	TArray<FVector2D> Points;
};
