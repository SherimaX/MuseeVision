#pragma once

#include "CoreMinimal.h"
#include "Vessel/VesselRecord.h"

namespace Musee::ArtMaking
{
	/**
	 * How a vessel's section is scaled to the size its plan shows it, [across, up]. A section derived from the vessel's
	 * own photographs (grade `derived`) is its true shape at its museum's height: it is scaled by one factor, the plan's
	 * height over its own, so its proportion is the photographs' (the plan gives its width as the section's, which make
	 * checks). A section traced from a photograph of a vessel of its type is scaled to the plan's width and height.
	 */
	[[nodiscard]] FVector2D SectionScale(const FArtVessel& Vessel, const FVector2D& SizeMeters);

	/**
	 * A vessel's section at the size its plan shows it (SectionScale), its top the plan's height above the foot's plane
	 * (the record's height 0, where the section's own foot or its feet stand). Point for point the record's, so a point of
	 * one is the same place on the vessel as the same point of the other.
	 */
	[[nodiscard]] TArray<FVector2D> SizedSection(const FArtVessel& Vessel, const FVector2D& SizeMeters);

	/** How near a derived section's height, and its plan's, come to its museum's: half a millimetre (the section's rounding). */
	inline constexpr double SectionHeightToleranceMeters = 0.0005;

	/** How near a derived vessel's plan's width comes to its section's: the plan gives sizes to the millimetre. */
	inline constexpr double PlanWidthToleranceMeters = 0.001;

	/**
	 * Whether a derived vessel is the size its museum records and its plan shows: its section's height its museum's, its
	 * plan's height its museum's and its width its section's, and its section's widest diameter (an oval's across its
	 * mouth's long axis) within DiameterTolerance of its museum's, or its record says why not (its Departure). Empty if
	 * so, or if it isn't derived; else what is wrong.
	 */
	[[nodiscard]] FString SizeFault(const FArtVessel& Vessel, const FVector2D& PlanMeters, double DiameterTolerance);

	/** The radius of a section's foot: its widest point at its lowest height. */
	[[nodiscard]] double FootRadius(TConstArrayView<FVector2D> Points);

	/**
	 * A turned vessel's surface laid out flat, as its mesh's UV0 and its decoration's image share it: U round the axis,
	 * metres along the widest parallel from the front (+y) towards the right (+x); V along the section, metres from its
	 * first point (the axis under the foot) out along the underside, up the outside, over the lip and down the inside.
	 */
	struct FVesselLayout
	{
		/** The widest radius, whose parallel U is measured along. */
		double ReferenceRadius = 0.0;

		/** Each section point's distance along the section from its first, metres. */
		TArray<double> Lengths;

		/** The layout's size: round the widest parallel, and along the whole section (metres). */
		[[nodiscard]] FVector2D SizeMeters() const
		{
			return FVector2D(2.0 * UE_DOUBLE_PI * ReferenceRadius, Lengths.IsEmpty() ? 0.0 : Lengths.Last());
		}
	};

	[[nodiscard]] FVesselLayout LayoutOf(TConstArrayView<FVector2D> Section);

	/**
	 * Where a distance along the section falls: the segment it lies on (from point Segment to Segment + 1) and how far
	 * along it, 0 to 1.
	 */
	struct FSectionPlace
	{
		int32 Segment = 0;
		double Along = 0.0;
	};

	[[nodiscard]] FSectionPlace PlaceAlong(const FVesselLayout& Layout, double LengthMeters);

	/**
	 * Whether a closed section is a solid when turned: no two of its segments cross (a surface through a surface), and,
	 * where it says where its outside runs ([first, last] points), its wall is nowhere thinner than half the wall it was
	 * drawn with (no point of its inside nearer its underside and outside than that). Empty if so; else what is wrong.
	 */
	[[nodiscard]] FString SectionFault(TConstArrayView<FVector2D> Section, TConstArrayView<int32> Outside, double WallMeters);

	/** The section's point at a place, and the outward normal of its segment there ([radius, height] components). */
	[[nodiscard]] FVector2D PointAt(TConstArrayView<FVector2D> Section, const FSectionPlace& Place);
	[[nodiscard]] FVector2D NormalOf(TConstArrayView<FVector2D> Section, int32 Segment);
}
