#pragma once

#include "CoreMinimal.h"

#include "SiteFootprint.generated.h"

/** What a stage's footprint is, which sets how it may meet the others. */
UENUM()
enum class ESiteFootprintKind : uint8
{
	/** A building: it never overlaps another building. */
	Building,
	/** The ground (one stage only, the grounds): it contains every building. */
	Ground,
};

/** One prism of a stage's volume: a plan outline between two heights, in the stage's frame (the Cube's box, and its shaft column above it). */
USTRUCT()
struct MUSEESITE_API FSitePrism
{
	GENERATED_BODY()

	/** The outline in plan, (x east, y south), metres, closed. */
	UPROPERTY()
	TArray<FVector2D> OutlineMeters;

	UPROPERTY(meta = (Units = "Meters"))
	double BottomMeters = 0.0;

	UPROPERTY(meta = (Units = "Meters"))
	double TopMeters = 0.0;
};

/**
 * The ground a stage stands on and the height it rises through, in its own frame: the volume it may build in. It is the
 * outline between the bottom and the top, or the stacked prisms it names. A stage stacked with others on one vertical axis
 * (the Cube under the Atrium under the Sphere) also gives its volume turned about that axis, as the region a profile
 * encloses in (r, z), so two stages whose prisms overlap can still be shown to meet only on their seams: the Sphere's
 * neck and bowl inside the Atrium's canopy.
 */
USTRUCT()
struct MUSEESITE_API FSiteFootprint
{
	GENERATED_BODY()

	UPROPERTY()
	ESiteFootprintKind Kind = ESiteFootprintKind::Building;

	/** The footprint's outline in plan, (x east, y south), metres, closed. */
	UPROPERTY()
	TArray<FVector2D> OutlineMeters;

	/** The lowest and highest points the stage builds to, metres. */
	UPROPERTY(meta = (Units = "Meters"))
	double BottomMeters = 0.0;

	UPROPERTY(meta = (Units = "Meters"))
	double TopMeters = 0.0;

	/** The stacked prisms the stage builds within, bottom up; empty when it is the one prism of the outline from bottom to top. */
	UPROPERTY()
	TArray<FSitePrism> Prisms;

	/**
	 * The volume turned about a vertical axis: a closed polygon in (r out from the axis, z up), metres, holding the
	 * section every half-plane from the axis cuts through the stage, and lying on the stage's own side of each seam (a
	 * curved seam drawn inside the stage, never across it). Empty when the stage isn't stacked on an axis.
	 */
	UPROPERTY()
	TArray<FVector2D> TurnedProfileMeters;

	/** The axis the profile turns about, (x east, y south) in the stage's frame, metres. */
	UPROPERTY(meta = (Units = "Meters"))
	FVector2D AxisMeters = FVector2D::ZeroVector;

	/** The prisms of the volume: the named ones, or the outline from bottom to top. */
	[[nodiscard]] TArray<FSitePrism> PrismsOrOutline() const;
};
