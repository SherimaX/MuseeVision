#pragma once

#include "CoreMinimal.h"

namespace Musee::ArtMaking
{
	/**
	 * One photograph of a handscroll's work: its size in pixels, and the window of it that is the work, as shares of its
	 * width (From, To, from its left edge) and of its height (Top, Bottom, from its top edge). What lies outside the window
	 * is the mount, or a neighbouring sheet, caught in the photograph, and never lies on the work's sheet.
	 */
	struct FScrollImage
	{
		FIntPoint Pixels = FIntPoint::ZeroValue;
		double From = 0.0;
		double To = 1.0;
		double Top = 0.0;
		double Bottom = 1.0;
	};

	/**
	 * Where one photograph lies on the scroll's sheet: the stretch of the sheet it covers, measured from the sheet's left
	 * edge; where that stretch's left edge falls in the photograph, and how far the sheet's top edge stands below the
	 * photograph's; and the photograph's size, laid at its own proportion.
	 */
	struct FScrollPanel
	{
		/** The photograph's place in the left-to-right order the layout was given. */
		int32 Image = 0;
		double AlongMeters = 0.0;
		double LengthMeters = 0.0;
		double ImageLeftMeters = 0.0;
		double ImageTopMeters = 0.0;
		FVector2D ImageMeters = FVector2D::ZeroVector;
	};

	/**
	 * A work's photographs joined end to end across its sheet, each at its own proportion, never stretched: the panels,
	 * left to right; how far the joined windows' proportion strays from the sheet's (their length over the sheet's at one
	 * height, less one); and how much of the windows is cut from each edge of the sheet (from the two ends when they are
	 * the longer, from the head and foot when shorter).
	 */
	struct FScrollLayout
	{
		TArray<FScrollPanel> Panels;
		double Stray = 0.0;
		double CutMeters = 0.0;
	};

	/**
	 * Lays a work's photographs, given left to right as the scroll lies before its reader, across a sheet of Sheet's size
	 * (metres): their windows joined at one height, each photograph at its own proportion, scaled together as little as
	 * covers the whole sheet and centred on it, so what overruns it is cut evenly from its two opposite edges. Empty when
	 * there is nothing to lay, or a window is empty.
	 */
	[[nodiscard]] FScrollLayout LayScroll(TConstArrayView<FScrollImage> LeftToRight, const FVector2D& Sheet);
}
