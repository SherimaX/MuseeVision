#pragma once

#include "CoreMinimal.h"

#include "NatureBranching.generated.h"

/**
 * One order of a tree's branching, as a species record describes it (the as-built museum's tree grower,
 * windows/Source/MuseeVision/Nature/MuseeTree.cpp, whose species were drawn after the Swift gardens): how many children a
 * branch of the order above bears and where along it, at what angle and phyllotaxis, how long and thick they grow, and
 * how they bend (curving, wandering, drooping under their weight, rising to the light, kinking at each node).
 */
USTRUCT()
struct MUSEENATURE_API FNatureBranching
{
	GENERATED_BODY()

	/** For the first order (the scaffold limbs): how many the trunk bears. */
	UPROPERTY()
	int32 Count = 0;

	/** For a deeper order: how many a metre of the parent bears. */
	UPROPERTY()
	float PerMeter = 0.0f;

	/** Where along the parent the children begin and end, as shares of its length. */
	UPROPERTY()
	float Start = 0.1f;

	UPROPERTY()
	float End = 1.0f;

	/** The angle from the parent's axis, at the top of the crown (and, for the first order, at its bottom). */
	UPROPERTY(meta = (Units = "Degrees"))
	float AngleDegrees = 45.0f;

	UPROPERTY(meta = (Units = "Degrees"))
	float AngleLowDegrees = 45.0f;

	/** How far the angle strays either way. */
	UPROPERTY(meta = (Units = "Degrees"))
	float AngleSpreadDegrees = 10.0f;

	/** The divergence round the parent between successive children (137.5 is the golden angle). */
	UPROPERTY(meta = (Units = "Degrees"))
	float PhyllotaxisDegrees = 137.5f;

	/**
	 * The first order's length as a share of the way to the crown's edge; a deeper order's as a share of its parent's,
	 * straying by the spread, and shorter towards the parent's tip by the taper.
	 */
	UPROPERTY()
	float LengthShare = 0.5f;

	UPROPERTY()
	float LengthSpread = 0.2f;

	UPROPERTY()
	float LengthTaper = 0.4f;

	/** A child's radius as a share of its parent's where it leaves it. */
	UPROPERTY()
	float RadiusShare = 0.5f;

	/** How far it curves over its length (upward positive), and wanders at each segment. */
	UPROPERTY(meta = (Units = "Degrees"))
	float CurveDegrees = 0.0f;

	UPROPERTY(meta = (Units = "Degrees"))
	float WanderDegrees = 5.0f;

	/** How far it droops under its weight, and rises towards the light, over its length. */
	UPROPERTY(meta = (Units = "Degrees"))
	float DroopDegrees = 0.0f;

	UPROPERTY(meta = (Units = "Degrees"))
	float RiseDegrees = 0.0f;

	/** The kink at each node, alternating (a jujube's or a plum's zigzag). */
	UPROPERTY(meta = (Units = "Degrees"))
	float ZigzagDegrees = 0.0f;

	/** How finely it is drawn: segments along it, sides round it. */
	UPROPERTY()
	int32 Segments = 4;

	UPROPERTY()
	int32 Sides = 4;
};
