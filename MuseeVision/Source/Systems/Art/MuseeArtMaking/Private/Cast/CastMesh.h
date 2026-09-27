#pragma once

#include "Cast/ScanMesh.h"
#include "CoreMinimal.h"

class FFormMesh;

namespace Musee::ArtMaking
{
	/**
	 * How a scan stands as its work: the scale that gives it its true height, and the point of the scan (in its own unit)
	 * that stands on its carrier's top centre: the centre of its base slice, at its lowest point. Measured at its true
	 * size, in metres: its width across the front (x), its depth (y), and its base slice's width and depth.
	 */
	struct FCastFit
	{
		double Scale = 0.0;
		FVector Foot = FVector::ZeroVector;
		double WidthMeters = 0.0;
		double DepthMeters = 0.0;
		FVector2D BaseMeters = FVector2D::ZeroVector;

		[[nodiscard]] bool IsValid() const { return Scale > 0.0; }

		/** A point of the scan in the work's own frame, metres: its foot at the origin. */
		[[nodiscard]] FVector Place(const FVector3f& Point) const { return (FVector(Point) - Foot) * Scale; }
	};

	/**
	 * Fits a scan to its true height (metres, above its lowest point): its scale, and its foot, the centre of the box of
	 * its points within BaseSliceMeters of its lowest at that height. Invalid for a scan with no height.
	 */
	[[nodiscard]] FCastFit FitCast(const FArtScanMesh& Scan, double HeightMeters, double BaseSliceMeters);

	/** A part of a cast in another material than its body: the band of heights above its foot, metres, it covers. */
	struct FCastBand
	{
		double FromMeters = 0.0;
		double ToMeters = 0.0;
	};

	/**
	 * Builds a cast to the Form kernel's contract in the work's own frame (its foot at the origin, x to the viewer's right,
	 * y its front, z up; metres): OutParts[0] its body, OutParts[k] the triangles whose centre stands in Bands[k - 1]. A
	 * scan carries no drawing's UVs, and plaster, marble and bronze have no direction of their own, so UV0 is a box
	 * projection in metres: each triangle is projected along the axis its face turns most towards. Every point keeps the
	 * scan's own smooth normal; a point without one takes its face's.
	 */
	void BuildCast(const FArtScanMesh& Scan, const FCastFit& Fit, TConstArrayView<FCastBand> Bands, TArray<FFormMesh>& OutParts);
}
