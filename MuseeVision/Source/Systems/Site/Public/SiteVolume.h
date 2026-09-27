#pragma once

#include "CoreMinimal.h"
#include "MuseeFinding.h"
#include "MuseeKey.h"
#include "SiteFootprint.h"

struct FSiteLayout;

namespace Musee::Site
{
	/**
	 * Two regions overlap when a straight line parallel to the first axis runs through both for more than this: the loop
	 * tolerance, a millimetre. Regions drawn each on its own side of a seam touch there and overlap by nothing.
	 */
	inline constexpr double OverlapToleranceMeters = 0.001;

	/**
	 * How far two closed polygons overlap: the longest stretch of a line parallel to the first axis (x in plan, r in a
	 * turned profile) that lies inside both, metres; zero for polygons that only touch. Each polygon is simple, and is read
	 * by the even-odd rule.
	 */
	[[nodiscard]] MUSEESITE_API double OverlapMeters(TConstArrayView<FVector2D> A, TConstArrayView<FVector2D> B);

	/**
	 * Site.NoOverlap over the placed stages: no two Building volumes share space. Two stages whose prisms overlap in plan
	 * and in height stand apart only when both are turned about one axis (within the loop tolerance) and their turned
	 * profiles meet only on their seams, as stages stacked on one axis do (the Cube, the Atrium, the Sphere). Each overlap
	 * is a finding drawn by the later of the two stages by key.
	 */
	[[nodiscard]] MUSEESITE_API TArray<FMuseeFinding> CheckOverlaps(const TMap<FMuseeKey, FSiteFootprint>& Footprints, const FSiteLayout& Layout);

	/**
	 * The prisms of every Building the layout places, moved into the frame of the placed stage Receiver: what a Ground
	 * stage receives (FSiteDescription::ReceivedPrisms). Empty when the receiver isn't placed. In the order of the
	 * buildings' keys.
	 */
	[[nodiscard]] MUSEESITE_API TArray<FSitePrism> PlacedPrisms(const FMuseeKey& Receiver, const TMap<FMuseeKey, FSiteFootprint>& Footprints,
		const FSiteLayout& Layout);
}
