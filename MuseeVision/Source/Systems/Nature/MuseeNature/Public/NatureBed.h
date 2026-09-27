#pragma once

#include "CoreMinimal.h"
#include "MuseeKey.h"
#include "NatureArc.h"
#include "NatureCast.h"
#include "NatureClip.h"
#include "NatureGround.h"
#include "NatureOutline.h"
#include "NatureTraining.h"

#include "NatureBed.generated.h"

/**
 * One planting in a stage's planting plan: a species grown in a Bed anchor, on the body that carries the bed (if it
 * moves). Each organ is made of its own living tissue (the species record's palette key for it: a lily's pad
 * `living.leaf-floating`, its petals `living.petal`), worked and aged as the entry says. A cast specimen is a single
 * plant grown to its species' form and cast in metal, every organ in the cast's material, working and history (the
 * golden iris, gilt bronze cast from Iris laevigata).
 *
 * Where the plants stand, in order: at each of its `points`, at its one `point` (a tree, a clipped ball), along its
 * `line` or `arc` (a hedge), over its `outline` (a ground cover: moss, a sward), or else round the anchor at the
 * planting's spacing. The ground under them, when the entry gives one, is Nature's too: the soil, lawn or gravel
 * surface over the outline.
 *
 * Every plan point a plan writes (a point, a line, a polygon's corner) is in the stage's frame, in metres; an ellipse is
 * centred on its anchor, its axes along the anchor's. The soil lies at the bed anchor's height.
 */
USTRUCT()
struct MUSEENATURE_API FNatureBed
{
	GENERATED_BODY()

	/** The Bed anchor: the soil's surface at the planting's centre. */
	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Bed;

	/** The species by its scientific name, as its record in Data/Nature/Species names it (`Nymphaea alba`). */
	UPROPERTY(meta = (MuseeRequired))
	FString Species;

	/** How many plants (clumps, crowns, colonies, culms) the bed holds. */
	UPROPERTY(meta = (MuseeRequired))
	int32 Count = 0;

	/** The planting distance between neighbouring plants; a planting of one plant stands at its anchor and needs none. */
	UPROPERTY(meta = (Units = "Meters"))
	float Spacing = 0.0f;

	/** The working and history the plants' tissues are resolved with (Data/Matter/Shading); none for a cast specimen. */
	UPROPERTY()
	FName Working;

	UPROPERTY()
	FName History;

	/** For a cast specimen, what it is cast in; every part of the plant takes it. */
	UPROPERTY()
	FNatureCast Cast;

	/** The body that carries the bed, or the none key (written empty or left out). */
	UPROPERTY()
	FMuseeKey Body;

	/** The ground Nature makes under the planting: its soil, lawn or gravel surface over the outline. */
	UPROPERTY()
	FNatureGround Ground;

	/** The bed's plan: the ground's extent, and the region a ground cover grows over. */
	UPROPERTY()
	FNatureOutline Outline;

	/**
	 * The stage's own piece a carpet lies on, in place of an outline: over its top, its faces that look straight up at its
	 * highest level (the grounds' lawn on its terrain, which the stage cuts round every building on it).
	 */
	UPROPERTY()
	FName On;

	/** Where each plant stands, [x, y]: count of them. */
	UPROPERTY()
	TArray<FVector2D> Points;

	/** Where its one plant stands, [x, y]. */
	UPROPERTY()
	TArray<double> Point;

	/**
	 * The line a hedge is planted along, [[x, y], [x, y], ...]: straight runs from point to point, mitred where they
	 * meet, its plants set out along it at the spacing.
	 */
	UPROPERTY()
	TArray<FVector2D> Line;

	/** Whether the line closes on itself (a hedge round a panel): it runs on from its last point back to its first. */
	UPROPERTY()
	bool Closed = false;

	/** An arc a hedge is planted along, in place of a line. */
	UPROPERTY()
	FNatureArc Arc;

	/** A tree's or a clump's height above the soil, as the plan draws it (the species' own when zero). */
	UPROPERTY(meta = (Units = "Meters"))
	float Height = 0.0f;

	/** The height of the middle of a tree's crown above the soil (the species' own share of its height when zero). */
	UPROPERTY(meta = (Units = "Meters"))
	float CrownCentre = 0.0f;

	/** A tree's crown's radii, [across, vertical] (the species' own when empty). */
	UPROPERTY()
	TArray<float> CrownRadii;

	/**
	 * How densely a tree is in leaf, as a share of the full leaf its species carries for a crown of its size (the
	 * species record's leavesPerPlant): above 1 for a close, well-fed crown (the allées' osmanthus), below it for an airy
	 * one (the birches). The species' own (1) when zero.
	 */
	UPROPERTY()
	float LeafDensity = 0.0f;

	/** For a clipped hedge, the section the shears keep; for a clipped ball, its diameter. */
	UPROPERTY()
	FNatureClip Clipped;

	/**
	 * For a sward, the height it is mown to: a mown lawn is a close carpet at that height over its region (as a moss is at
	 * its shoots' length), not tufts grown blade by blade. Zero for a sward left uncut.
	 */
	UPROPERTY(meta = (Units = "Meters"))
	float Mown = 0.0f;

	/** For a climber, the paths it is trained along: its stem first, then its canes. */
	UPROPERTY()
	TArray<FNatureTraining> Trained;

	UPROPERTY()
	FString Note;
};
