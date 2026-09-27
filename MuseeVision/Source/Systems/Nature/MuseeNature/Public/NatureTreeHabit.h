#pragma once

#include "CoreMinimal.h"
#include "NatureBranching.h"

#include "NatureTreeHabit.generated.h"

/**
 * How a tree species grows, for the `tree` form: its crown's place and shape, its trunk (or trunks), its orders of
 * branching, and how its leaves and flowers are set on its twigs. The numbers are the species' own habit, after the
 * as-built museum's tree grower and the floras its records cite. The tree is grown to the height and crown its bed
 * draws, or to the species' own.
 */
USTRUCT()
struct MUSEENATURE_API FNatureTreeHabit
{
	GENERATED_BODY()

	/** The height of the crown's middle, and the crown's radii [across, vertical], as shares of the tree's height. */
	UPROPERTY()
	float CrownCentreShare = 0.7f;

	UPROPERTY()
	TArray<float> CrownRadiusShares;

	/** The trunk's radius at its foot, for a tree of the species' middle height; it goes with the square root of the height. */
	UPROPERTY(meta = (Units = "Meters"))
	float TrunkRadiusMeters = 0.1f;

	/** The trunk's radius at its top, as a share of its foot's. */
	UPROPERTY()
	float TrunkTipShare = 0.1f;

	/** The root flare: how much wider the trunk's foot is, as a share of its radius, dying away within its first metre. */
	UPROPERTY()
	float FlareShare = 0.3f;

	/** How the trunk leans, wanders and kinks as it rises. */
	UPROPERTY(meta = (Units = "Degrees"))
	float LeanDegrees = 2.0f;

	UPROPERTY(meta = (Units = "Degrees"))
	float WanderDegrees = 3.0f;

	UPROPERTY(meta = (Units = "Degrees"))
	float ZigzagDegrees = 0.0f;

	/** The trunk's length as a share of the tree's height. */
	UPROPERTY()
	float TrunkShare = 0.95f;

	/** How far the crown follows the trunk's top where the trunk leans (0 to 1). */
	UPROPERTY()
	float CrownFollowShare = 0.5f;

	/** How many trunks rise from the foot (a multi-stemmed osmanthus), spread apart by an angle. */
	UPROPERTY()
	int32 Stems = 1;

	UPROPERTY(meta = (Units = "Degrees"))
	float StemSpreadDegrees = 0.0f;

	/** Whether the scaffold limbs are spaced evenly round the trunk (a pruned orchard tree). */
	UPROPERTY()
	bool bEvenScaffolds = false;

	/** The finest twig's radius. */
	UPROPERTY(meta = (Units = "Meters"))
	float MinRadiusMeters = 0.002f;

	UPROPERTY()
	int32 TrunkSegments = 10;

	UPROPERTY()
	int32 TrunkSides = 12;

	/** The orders of branching, the scaffold limbs first. */
	UPROPERTY()
	TArray<FNatureBranching> Branching;

	/** The order whose twigs carry the leaves (1 the scaffold limbs), from this share along each. */
	UPROPERTY()
	int32 LeafOrder = 2;

	UPROPERTY()
	float LeafStartShare = 0.15f;

	/** How many leaves a tree of the species' middle height carries in full leaf; it goes with the crown's volume. */
	UPROPERTY()
	int32 LeavesPerPlant = 10000;

	/** The angle a leaf's stalk makes with its twig; how far it droops; how much its blade folds and curls. */
	UPROPERTY(meta = (Units = "Degrees"))
	float LeafAngleDegrees = 50.0f;

	UPROPERTY()
	float LeafDroop = 0.3f;

	UPROPERTY()
	float LeafFold = 0.25f;

	UPROPERTY()
	float LeafCurl = 0.06f;

	/** The order whose wood carries the flowers, in clusters of so many, on stalks this long. */
	UPROPERTY()
	int32 FlowerOrder = 2;

	UPROPERTY()
	TArray<int32> FlowersPerCluster;

	UPROPERTY(meta = (Units = "Meters"))
	float PedicelMeters = 0.01f;

	/** The share of flowers in the second of the species' flower colours. */
	UPROPERTY()
	float SecondColourShare = 0.0f;
};
