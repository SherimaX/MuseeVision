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
	 *   photograph shows there, divided channel by channel by that photograph's white there: the glaze's hue, at the
	 *   lightness the glaze itself shows round that place (the WhiteQuantile of the photograph's glaze there, so a
	 *   gallery's light falling off down a vase, or a cup's wall darkening as it turns under, is taken off and the
	 *   painting keeps its darkness against its own ground); the glaze is white in the image, and its colour is Matter's.
	 *   A view sees a place where it faces the camera at all, obliquely too, its line of sight passes no other part of the
	 *   vessel, and it shows in the photograph inside the vessel's silhouette by the camera's own miss and an edge's blur
	 *   (EdgePixels); its weight is the square of how squarely it sees it, coming in over ClearanceMeters as its sight
	 *   clears the silhouette's margin and the vessel (a highlight lends little);
	 * - two views whose paintings lie further apart where they overlap than BlendMeters (the record's OffsetMeters) are not
	 *   blended, which would show the painting twice: each keeps its own side of the overlap, meeting the other at a seam
	 *   run where their paintings agree best, cross-faded over SeamMeters;
	 * - where the views' summed weight falls short of FullWeight (only towards the edge of what they see, as they see it
	 *   more and more askance), the colour runs smoothly to the fill: where a band of ornament runs, the band's own repeats
	 *   carried round, cross-faded by how well each is seen; elsewhere the glaze, white; so there is no seam;
	 * - the underside and the inside, which no view is composed onto, are the glaze.
	 * Deterministic: the same views and record give the same pixels.
	 */
	void ComposeDecoration(const FArtVesselComposition& How, const FArtVessel& Vessel, TConstArrayView<FVector2D> Sized,
		TConstArrayView<FVesselPicture> Views, FIntPoint Pixels, FImage& Out, FDecorationStats& OutStats);

	/** How far apart two views' paintings lie where the composition blends them, and over how many blocks it was measured. */
	struct FBlendOffset
	{
		int32 First = INDEX_NONE;
		int32 Second = INDEX_NONE;
		double Meters = 0.0;
		int32 Blocks = 0;
	};

	/**
	 * Where a painted vessel's composition blends two of its views (Views in the record's order, each at its record's
	 * camera), how far apart their paintings lie there: over the registration's grid, each view weighted as the
	 * composition weighs it (with its seams, unless bSeams is false), blocks where the lesser of the two weights is at least
	 * a quarter of the greater, each laid on the other view by the rise and turn at which its detail correlates best; the
	 * median offset of the blocks that lie on the other at 0.8 or better. A pair not listed is blended nowhere measurable.
	 * Blended further apart than BlendMeters, the painting shows twice.
	 */
	[[nodiscard]] TArray<FBlendOffset> BlendOffsets(const FArtVesselComposition& How, const FArtVesselRegistration& Registration, const FArtVessel& Vessel,
		TConstArrayView<FVesselPicture> Views, bool bSeams = true);

	/**
	 * How dark a view's painting is against its white, in the photograph and as composed, band by band along the outside
	 * (Bands of equal length): over the texels the view sees at least WhiteSquareness squarely, in windows of the band a
	 * thirtieth of the turn wide (so the light falling across the photograph is not taken for its painting), the 20th
	 * percentile of their lightness over the 90th, the photograph's and the decoration's, each the median over the band's
	 * windows; the share of those texels the decoration gives pure white (a white fitted too dark clips the glaze and
	 * pales the painting), and the share the photograph itself shows saturated (every channel at 250 or over: its own
	 * white clipped, which no division brings back).
	 */
	struct FContrastBand
	{
		double FromMeters = 0.0;
		double ToMeters = 0.0;
		double Photograph = 0.0;
		double Composed = 0.0;
		double Clipped = 0.0;
		double Saturated = 0.0;
		int32 Samples = 0;
		int32 Windows = 0;
	};

	[[nodiscard]] TArray<FContrastBand> PaintingContrast(const FArtVesselComposition& How, const FArtVessel& Vessel, const FVesselPicture& View,
		TConstArrayView<FVector2D> Sized, const FImage& Decoration, int32 Bands);

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
