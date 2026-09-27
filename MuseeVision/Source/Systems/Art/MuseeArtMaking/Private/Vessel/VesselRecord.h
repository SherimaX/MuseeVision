#pragma once

#include "CoreMinimal.h"

#include "VesselRecord.generated.h"

/**
 * The carved stand a vessel stands on where its plan says `stand` (`Data/Art/Vessels.json`, from the stage's sheet): a
 * drum whose radius is so many times the vessel's foot, low under a small vessel and higher under one of TallMeters or
 * more, in its wood's palette finish.
 */
USTRUCT()
struct FArtVesselStand
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	double FootRadii = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double LowMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double HighMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double TallMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired))
	FName Palette;

	UPROPERTY(meta = (MuseeRequired))
	FName Working;

	UPROPERTY(meta = (MuseeRequired))
	FName History;
};

/**
 * A vessel's size as its museum records it: its height, and where published its widest diameter, its foot's diameter,
 * and for an oval vessel its mouth's and foot's two axes [across, front to back]. Zero where the museum publishes none.
 */
USTRUCT()
struct FArtVesselMuseum
{
	GENERATED_BODY()

	UPROPERTY(meta = (Units = "Meters"))
	double HeightMeters = 0.0;

	UPROPERTY(meta = (Units = "Meters"))
	double DiameterMeters = 0.0;

	UPROPERTY(meta = (Units = "Meters"))
	double FootMeters = 0.0;

	UPROPERTY()
	TArray<double> MouthAxesMeters;

	UPROPERTY()
	TArray<double> FootAxesMeters;

	/** Whether the museum records an oval's mouth, whose two axes the turned section is scaled to. */
	[[nodiscard]] bool IsOval() const { return MouthAxesMeters.Num() == 2; }
};

/** How much of the outside a view shares with another view, and how well their paintings correlate there. */
USTRUCT()
struct FArtVesselOverlap
{
	GENERATED_BODY()

	/** The other view's photograph (its file name). */
	UPROPERTY(meta = (MuseeRequired))
	FString Image;

	/** The share of the unwrapped outside both views see (weighted by how squarely each sees it). */
	UPROPERTY()
	double Share = 0.0;

	UPROPERTY()
	double Correlation = 0.0;
};

/**
 * One photograph of a painted vessel, and the camera that took it as its derivation found it (MuseeVision/Scripts/
 * vessels.py): a pinhole standing at a bearing round the vessel (from its front towards its right), so far from its
 * axis and so high above its foot, pitched down by so much, with its focal length and its optical axis's place in the
 * photograph in pixels. A point of the vessel's frame (x its right, y its front, z up from its foot, metres) falls on
 * the photograph where that camera sees it.
 */
USTRUCT()
struct FArtVesselView
{
	GENERATED_BODY()

	/** The photograph, from the repository's root (`assets/ceramics/<work>/<file>`), read from the collection. */
	UPROPERTY(meta = (MuseeRequired))
	FString Image;

	UPROPERTY(meta = (Units = "Degrees"))
	double AzimuthDegrees = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double DistanceMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double HeightMeters = 0.0;

	UPROPERTY(meta = (Units = "Degrees"))
	double PitchDegrees = 0.0;

	UPROPERTY(meta = (MuseeRequired))
	double FocalPixels = 0.0;

	/** The optical axis's place in the photograph [u, v], pixels from its top-left corner. */
	UPROPERTY(meta = (MuseeRequired))
	TArray<double> AxisPixels;

	/** How far the section's image through this camera strays from the photograph's silhouette (rms, pixels). */
	UPROPERTY()
	double FitPixels = 0.0;

	/**
	 * How its bearing was found: `fitted` (the view the section is derived from, at bearing 0), `registered` (its
	 * painting laid on the first view's, directly or through others it overlaps) or `unverified` (its painting shares
	 * nothing with the first view's, so its bearing against it is not in the photographs).
	 */
	UPROPERTY(meta = (MuseeRequired))
	FString Placement;

	/** The other views it overlaps, as placed. */
	UPROPERTY()
	TArray<FArtVesselOverlap> Overlaps;

	[[nodiscard]] static bool IsPlacement(const FString& Word)
	{
		return Word == TEXT("fitted") || Word == TEXT("registered") || Word == TEXT("unverified");
	}
};

/**
 * A band of ornament running round a painted vessel (a scroll, a row of panels): between two heights up its outside,
 * repeating every so many degrees. Where no view sees the band, its own repeat is carried round from where one does. A
 * band whose repeat isn't known (read on its photographs, or measured and trusted) isn't listed: where no view sees it,
 * the glaze is left.
 */
USTRUCT()
struct FArtVesselBand
{
	GENERATED_BODY()

	UPROPERTY()
	FString Name;

	UPROPERTY(meta = (Units = "Meters"))
	double FromMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double ToMeters = 0.0;

	/** The band's repeat round the axis: a whole number of repeats to the turn. */
	UPROPERTY(meta = (MuseeRequired, Units = "Degrees"))
	double PeriodDegrees = 0.0;

	/** How the repeat is known: read by eye on the photographs (with the measured repeat beside it), a plain line, or measured. */
	UPROPERTY()
	FString Reading;
};

/** One foot, as its plan: a closed outline [across, front to back] in metres from the axis, anticlockwise from above. */
USTRUCT()
struct FArtVesselFoot
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	TArray<FVector2D> Outline;
};

/**
 * The feet a vessel stands on where it isn't thrown with a foot ring (the Ru basin's four cloud-scroll feet, traced
 * from its base's photograph): each its outline stood up to their height. Its section stands on them: its lowest point
 * is their height.
 */
USTRUCT()
struct FArtVesselFeet
{
	GENERATED_BODY()

	UPROPERTY(meta = (Units = "Meters"))
	double HeightMeters = 0.0;

	UPROPERTY()
	TArray<FArtVesselFoot> Each;
};

/** A photograph of a vessel its derivation read and left out, and why. */
USTRUCT()
struct FArtVesselUnused
{
	GENERATED_BODY()

	UPROPERTY()
	FString Image;

	UPROPERTY()
	FString Why;
};

/**
 * A vessel's turned section: [radius, height] in metres from the centre of its foot, out along its underside, up its
 * outside, over its lip and down its inside to the axis again, the solid on the left. Turned about its axis it is the
 * vessel's closed solid.
 *
 * A section the stage's sheet traced from a photograph of a vessel of its type gives only that. One derived from the
 * vessel's own photographs (grade `derived`) says how (its derivation), what the museum records of its size and where
 * its outside runs in the section ([first, last] points); a painted vessel's also gives the views its decoration is
 * composed from, how much of its outside they see, its bands of ornament, and the photographs left out.
 */
USTRUCT()
struct FArtVessel
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	FString Work;

	UPROPERTY()
	FString Source;

	UPROPERTY()
	FString Grade;

	UPROPERTY()
	FString Derivation;

	UPROPERTY()
	FArtVesselMuseum Museum;

	/**
	 * Why a derived section's widest diameter stands further from the museum's than the record's DiameterTolerance: what
	 * the photographs show against the published figure. Empty where it doesn't.
	 */
	UPROPERTY()
	FString Departure;

	UPROPERTY(meta = (MuseeRequired))
	TArray<FVector2D> Section;

	UPROPERTY()
	TArray<int32> Outside;

	/** The wall its section was drawn with (its inside that far in from its outside): its lip's, where read. */
	UPROPERTY(meta = (Units = "Meters"))
	double WallMeters = 0.0;

	UPROPERTY()
	TArray<FArtVesselView> Views;

	UPROPERTY()
	double Coverage = 0.0;

	UPROPERTY()
	TArray<FArtVesselBand> Bands;

	UPROPERTY()
	TArray<FArtVesselUnused> Unused;

	UPROPERTY()
	FArtVesselFeet Feet;

	/** Whether its decoration is composed from its views: a painted vessel. */
	[[nodiscard]] bool IsPainted() const { return !Views.IsEmpty(); }
};

/**
 * How a painted vessel's views are composed round it (Data/Art/Vessels.json): how squarely a view must see the surface
 * to lend it its colour, how a photograph's white and its highlights are known, and how finely the whole is composed.
 */
USTRUCT()
struct FArtVesselComposition
{
	GENERATED_BODY()

	/**
	 * The least cosine between the surface's normal and a view's line of sight at which the view lends its colour: its
	 * weight rises from nothing there, as the square of how far past it the view sees, to one face on.
	 */
	UPROPERTY(meta = (MuseeRequired))
	double SquarenessCut = 0.0;

	/** The cosine past which a view sees the surface squarely enough to measure its white there. */
	UPROPERTY(meta = (MuseeRequired))
	double WhiteSquareness = 0.0;

	/** The share of those samples darker than the view's white: its white is that quantile of their lightness. */
	UPROPERTY(meta = (MuseeRequired))
	double WhiteQuantile = 0.0;

	/**
	 * Where a view shows plain glaze, to which its shading is fitted: a sample no more saturated than this (the spread of
	 * its channels over its largest) and no darker than GlazeLightness of the view's white; the shading never below
	 * ShadeFloor of the white.
	 */
	UPROPERTY(meta = (MuseeRequired))
	double GlazeChroma = 0.0;

	UPROPERTY(meta = (MuseeRequired))
	double GlazeLightness = 0.0;

	UPROPERTY(meta = (MuseeRequired))
	double ShadeFloor = 0.0;

	/** A sample so many times its view's white is a highlight: the lamp's reflection in the glaze, not the painting. */
	UPROPERTY(meta = (MuseeRequired))
	double HighlightOverWhite = 0.0;

	/** A highlight's weight, as a share of a sample's, so another view that sees the surface without it wins. */
	UPROPERTY(meta = (MuseeRequired))
	double HighlightWeight = 0.0;

	/** The summed weight at which the views alone give the surface's colour; below it a band's repeat or the glaze fills in. */
	UPROPERTY(meta = (MuseeRequired))
	double FullWeight = 0.0;

	/** The finest the decoration is composed at, pixels a metre, and the most pixels it spans either way. */
	UPROPERTY(meta = (MuseeRequired))
	double MostPixelsPerMeter = 0.0;

	UPROPERTY(meta = (MuseeRequired))
	int32 MostPixels = 0;
};

/** Every turned vessel's section, the stand they stand on and how painted ones are composed (`Data/Art/Vessels.json`). */
USTRUCT()
struct FArtVessels
{
	GENERATED_BODY()

	/** The record, under Data. */
	static constexpr const TCHAR* Record = TEXT("Art/Vessels.json");

	/** A section turns smoothly through a traced point where it turns less than this, and keeps an arris where it turns more. */
	UPROPERTY(meta = (MuseeRequired, Units = "Degrees"))
	double CreaseDegrees = 0.0;

	/**
	 * How far a derived section's widest diameter may stand from its museum's, as a share of it, before its record must
	 * say why (its Departure): a published figure's rounding and a hand-thrown vessel's own out-of-round.
	 */
	UPROPERTY(meta = (MuseeRequired))
	double DiameterTolerance = 0.0;

	UPROPERTY(meta = (MuseeRequired))
	FArtVesselStand Stand;

	UPROPERTY(meta = (MuseeRequired))
	FArtVesselComposition Composition;

	UPROPERTY(meta = (MuseeRequired))
	TArray<FArtVessel> Vessels;

	[[nodiscard]] const FArtVessel* Find(const FString& Work) const
	{
		return Vessels.FindByPredicate([&Work](const FArtVessel& V) { return V.Work == Work; });
	}
};
