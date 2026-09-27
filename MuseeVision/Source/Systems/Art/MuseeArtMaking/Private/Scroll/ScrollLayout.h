#pragma once

#include "CoreMinimal.h"

namespace Musee::ArtMaking
{
	/** One photograph of a handscroll's work: its size in pixels, and the share of its width that is the work. */
	struct FScrollImage
	{
		FIntPoint Pixels = FIntPoint::ZeroValue;
		double From = 0.0;
		double To = 1.0;
	};

	/**
	 * Where one photograph lies on the scroll's sheet: the stretch of the sheet it covers, measured from the sheet's left
	 * edge, and where that stretch's left edge falls in the photograph, which is laid at its own proportion at ImageMeters.
	 */
	struct FScrollPanel
	{
		/** The photograph's place in the left-to-right order the layout was given. */
		int32 Image = 0;
		double AlongMeters = 0.0;
		double LengthMeters = 0.0;
		double ImageLeftMeters = 0.0;
		FVector2D ImageMeters = FVector2D::ZeroVector;
	};

	/**
	 * A work's photographs joined end to end across its sheet, each at its own proportion, never stretched: the panels,
	 * left to right; how far the photographs' top edge stands above the sheet's; how far the joined photographs' proportion
	 * strays from the sheet's (their length over the sheet's at one height, less one); and how much of the photographs is
	 * cut from each edge of the sheet (from the two ends when they are the longer, from the head and foot when shorter).
	 */
	struct FScrollLayout
	{
		TArray<FScrollPanel> Panels;
		double ImageTopMeters = 0.0;
		double Stray = 0.0;
		double CutMeters = 0.0;
	};

	/**
	 * Lays a work's photographs, given left to right as the scroll lies before its reader, across a sheet of Sheet's size
	 * (metres): joined at one height, each at its own proportion, scaled together as little as covers the whole sheet and
	 * centred on it, so what overruns it is cut evenly from its two opposite edges. Empty when there is nothing to lay.
	 */
	[[nodiscard]] FScrollLayout LayScroll(TConstArrayView<FScrollImage> LeftToRight, const FVector2D& Sheet);
}
