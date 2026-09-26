#pragma once

#include "CoreMinimal.h"
#include "Nature/MuseeNatureActor.h"
#include "MuseeTree.generated.h"

class UCapsuleComponent;
class UBoxComponent;

/**
 * A tree grown from a seed (Shared/Core/Garden.swift, tree(); the gardens of Shared/Wings/HallOfLight.swift
 * and Shared/Wings/ChineseWing.swift). A trunk with taper, root flare and a slight curve; branches
 * in a phyllotactic spiral at the species' angles, bending up or drooping under their weight, down
 * to fine twigs; the scaffold branches reach for the crown's envelope (CrownHeight, CrownRadii:
 * the Swift crown's centre and radii), so the tree fills the same space as the design's crown.
 *
 * Bark is tapered tubes with smooth normals (more sides on the trunk, three on twigs). Leaves,
 * blossom and fruit follow the season (the Chinese species follow the solar term, as in the
 * Swift): leaf cards along the twigs, turned to the light, drawn by a masked two-sided foliage
 * material that lets the light through. The wind is in the materials: every vertex carries how
 * far it bends and its phase (see NatureMesh.h), so nothing moves on the CPU.
 *
 * Species: Birch, OrchardFruit (FruitVariant 0 cherry, 1 apple), ChinesePlum, Osmanthus, and
 * Bamboo, a clump of culms at Culms.
 */
UCLASS()
class MUSEEVISION_API AMuseeTree : public AMuseeNatureActor
{
	GENERATED_BODY()

public:
	AMuseeTree();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	EMuseeTreeSpecies Species = EMuseeTreeSpecies::Birch;

	/** Overall height, metres (0 = the species' own; bamboo: the mean culm height). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature", meta = (ClampMin = "0"))
	float TreeHeight = 0.f;

	/** Height of the crown's centre, metres (0 = the species' own). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature", meta = (ClampMin = "0"))
	float CrownHeight = 0.f;

	/** The crown's radii, horizontal and vertical, metres (0 = the species' own). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	FVector2D CrownRadii = FVector2D::ZeroVector;

	/** OrchardFruit: 0 cherry, 1 apple. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	int32 FruitVariant = 0;

	/** Bamboo: where the culms stand, metres from the actor (x east, y south). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	TArray<FVector2D> Culms;

	/**
	 * A plan box (metres from the actor; x east, y south) that the culms, branches and leaves stay inside above
	 * KeepFromHeight: a wall's face or an eave's edge less a margin. Invalid (the default): no limit.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	FBox2D Keep = FBox2D(ForceInit);

	/** Keep applies above this height, metres (0: the whole plant, as against a wall; ~3 m for a crown under an eave). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature", meta = (ClampMin = "0"))
	float KeepFromHeight = 0.f;

	/** Leaves, blossom and fruit relative to the species' own numbers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature", meta = (ClampMin = "0.1", ClampMax = "3"))
	float LeafDensity = 1.f;

	/** Visitors walk round the trunk (a capsule), or round the clump (a box). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	bool bBlockVisitors = true;

	UPROPERTY(VisibleAnywhere, Category = "Nature")
	TObjectPtr<UCapsuleComponent> TrunkCollision;

	UPROPERTY(VisibleAnywhere, Category = "Nature")
	TObjectPtr<UBoxComponent> ClumpCollision;

protected:
	virtual void BuildPlant() override;
};
