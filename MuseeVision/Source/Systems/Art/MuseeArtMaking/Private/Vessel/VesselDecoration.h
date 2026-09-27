#pragma once

#include "CoreMinimal.h"
#include "ImageCore.h"
#include "Vessel/VesselSection.h"

namespace Musee::ArtMaking
{
	/**
	 * The camera a photograph of a vessel was taken with (FArtVesselView): a pinhole at a bearing round the vessel from
	 * its front towards its right, its distance from the axis and its height above the foot, pitched down, with its focal
	 * length and its optical axis's place in the photograph, both in the photograph's pixels.
	 */
	struct FVesselCamera
	{
		double AzimuthRadians = 0.0;
		double DistanceMeters = 0.0;
		double HeightMeters = 0.0;
		double PitchRadians = 0.0;
		double FocalPixels = 0.0;
		FVector2D AxisPixels = FVector2D::ZeroVector;

		/** Where the camera stands, in the vessel's frame. */
		[[nodiscard]] FVector Position() const;

		/** Where a point of the vessel's frame falls on the photograph, pixels from its top-left corner; false behind it. */
		[[nodiscard]] bool Project(const FVector& Point, FVector2D& OutPixel) const;
	};

	[[nodiscard]] FVesselCamera CameraOf(const FArtVesselView& View);

	/** One view to compose from: its photograph, 8-bit sRGB, and its camera. */
	struct FVesselPicture
	{
		FImage Image;
		FVesselCamera Camera;
	};

	/** What a composition did, for its checks: the most a contributing view was stretched, and how much it saw. */
	struct FDecorationStats
	{
		/** The largest 1 / squareness of any view that lent a texel its colour: no view lends it past SquarenessCut. */
		double MostStretch = 0.0;
		/** The share of the outside's texels the views gave whole, and the share a band's repeat filled. */
		double Seen = 0.0;
		double Repeated = 0.0;
	};

	/** The decoration's size in pixels: the layout at the finest its best view resolves, within the record's bounds. */
	[[nodiscard]] FIntPoint DecorationPixels(const FArtVesselComposition& How, const FArtVessel& Vessel, const FVesselLayout& Layout);

	/**
	 * Composes a painted vessel's decoration in its layout (the sized section's, which its mesh's UV0 shares), 8-bit sRGB:
	 * - each texel of the outside is the point of the vessel's own section (the record's, at the museum's size, where its
	 *   views were fitted) at the same place, turned to its angle; each view that sees it at least SquarenessCut squarely
	 *   lends the colour its photograph shows there, divided channel by channel by that photograph's white there (its
	 *   glaze's hue, at the lightness a plane in the normal fitted to its plain glaze gives: its shading taken off; the
	 *   glaze is white in the image, and its colour is Matter's), with the weight the square of how far past the cut it
	 *   sees (a highlight lends little);
	 * - where the views' summed weight falls short of FullWeight, the colour runs smoothly to the fill: where a band of
	 *   ornament runs, the band's own repeats carried round, cross-faded by how well each is seen; elsewhere the glaze,
	 *   white; so there is no seam and no view stretched past the cut;
	 * - the underside and the inside, which no view is composed onto, are the glaze.
	 * Deterministic: the same views and record give the same pixels.
	 */
	void ComposeDecoration(const FArtVesselComposition& How, const FArtVessel& Vessel, TConstArrayView<FVector2D> Sized,
		TConstArrayView<FVesselPicture> Views, FIntPoint Pixels, FImage& Out, FDecorationStats& OutStats);
}
