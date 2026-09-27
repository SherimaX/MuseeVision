#pragma once

#include "CoreMinimal.h"
#include "ImageCore.h"
#include "Vessel/VesselSection.h"

namespace Musee::ArtMaking
{
	/**
	 * The camera a photograph of a vessel was taken with (FArtVesselView): a pinhole at a bearing round the vessel from
	 * its front towards its right, its distance from the axis and its height above the foot, pitched down, with its focal
	 * length and its optical axis's place in the photograph, both in the photograph's pixels; and how far the section's
	 * image through it strays from the photograph's silhouette (its fit, pixels).
	 */
	struct FVesselCamera
	{
		double AzimuthRadians = 0.0;
		double DistanceMeters = 0.0;
		double HeightMeters = 0.0;
		double PitchRadians = 0.0;
		double FocalPixels = 0.0;
		FVector2D AxisPixels = FVector2D::ZeroVector;
		double FitPixels = 0.0;

		/** Where the camera stands, in the vessel's frame. */
		[[nodiscard]] FVector Position() const;

		/** Where a point of the vessel's frame falls on the photograph, pixels from its top-left corner; false behind it. */
		[[nodiscard]] bool Project(const FVector& Point, FVector2D& OutPixel) const;

		/** The direction, in the vessel's frame, of the line of sight from the camera through a place in the photograph. */
		[[nodiscard]] FVector Sight(const FVector2D& Pixel) const;
	};

	[[nodiscard]] FVesselCamera CameraOf(const FArtVesselView& View);

	/** One view to compose from: its photograph, 8-bit sRGB, and its camera. */
	struct FVesselPicture
	{
		FImage Image;
		FVesselCamera Camera;
	};

	/** What a composition did, for its checks: how much of the outside the views gave whole, and a band's repeat filled. */
	struct FDecorationStats
	{
		double Seen = 0.0;
		double Repeated = 0.0;
	};

	/** The decoration's size in pixels: the layout at the finest its best view resolves, within the record's bounds. */
	[[nodiscard]] FIntPoint DecorationPixels(const FArtVesselComposition& How, const FArtVessel& Vessel, const FVesselLayout& Layout);

	/**
	 * Composes a painted vessel's decoration in its layout (the sized section's, which its mesh's UV0 shares), 8-bit sRGB:
	 * - each texel of the outside is the point of the vessel's own section (the record's, at the museum's size, where its
	 *   views were fitted) at the same place, turned to its angle; every view that sees it lends the colour its
	 *   photograph shows there, divided channel by channel by that photograph's white there (its glaze's hue, at the
	 *   lightness a plane in the normal and in the view's squareness, fitted to its plain glaze, gives: its shading taken
	 *   off; the glaze is white in the image, and its colour is Matter's). A view sees a place where it faces the camera at all, obliquely too, its line
	 *   of sight passes no other part of the vessel, and it shows in the photograph inside the vessel's silhouette by the
	 *   camera's own miss and an edge's blur (EdgePixels); its weight is the square of how squarely it sees it, coming in
	 *   over ClearanceMeters as its sight clears the silhouette's margin and the vessel (a highlight lends little);
	 * - where the views' summed weight falls short of FullWeight (only towards the edge of what they see, as they see it
	 *   more and more askance), the colour runs smoothly to the fill: where a band of ornament runs, the band's own repeats carried round, cross-faded by how well
	 *   each is seen; elsewhere the glaze, white; so there is no seam;
	 * - the underside and the inside, which no view is composed onto, are the glaze.
	 * Deterministic: the same views and record give the same pixels.
	 */
	void ComposeDecoration(const FArtVesselComposition& How, const FArtVessel& Vessel, TConstArrayView<FVector2D> Sized,
		TConstArrayView<FVesselPicture> Views, FIntPoint Pixels, FImage& Out, FDecorationStats& OutStats);

	/** How much of the outside two placed views share, and how well their paintings correlate there (-1 where too little). */
	struct FViewOverlap
	{
		double Share = 0.0;
		double Correlation = -1.0;
	};

	/**
	 * The overlap of each pair of a painted vessel's views (Views in the record's order, each at its record's camera),
	 * as its derivation registers them (FArtVesselRegistration): Result[i][j] for views i and j.
	 */
	[[nodiscard]] TArray<TArray<FViewOverlap>> ViewOverlaps(const FArtVesselRegistration& How, const FArtVessel& Vessel,
		TConstArrayView<FVesselPicture> Views);

	/**
	 * Why a painted vessel's registered views don't stand where the evidence that placed them puts them: a registered view
	 * lists the overlaps that placed it; at its record's bearing it overlaps each view it lists by the share its record
	 * gives (within ShareTolerance of it) at a correlation no worse than its record's (by CorrelationTolerance), and
	 * overlaps no view it doesn't list by as much as LeastShare. A view turned from its place overlaps others by other
	 * shares, or its painting no longer lies on theirs. Empty if every one stands so.
	 */
	[[nodiscard]] TArray<FString> PlacementFaults(const FArtVesselRegistration& How, const FArtVessel& Vessel, TConstArrayView<FVesselPicture> Views);
}
