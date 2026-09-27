#pragma once

#include "CoreMinimal.h"
#include "FormDrawing.h"
#include "FormProfile.h"
#include "MuseeFrame.h"

#include "Moulding.generated.h"

/**
 * A picture-frame moulding: the section a framer runs round what it frames, in units of the moulding's width (A outward
 * from the framed edge, B up from the face the frame's lip rests on, drawn from the outside edge inward and ending on
 * that face at the sight lip), and the rule that sizes the width to the work, as frame makers proportion a moulding to
 * the picture. The museum's moulding is the carved and gilt one, `Data/Art/Moulding.json`; a style Art draws has its own
 * record, `Data/Art/Mouldings/<style>.json` (the photograph's flat oak, the wallpapers' panel frame).
 */
USTRUCT()
struct FArtMoulding
{
	GENERATED_BODY()

	/** The museum's moulding's record, under Data. */
	static constexpr const TCHAR* Record = TEXT("Art/Moulding.json");

	/**
	 * The frame styles (a plan's `frameStyle`) that have their own record, `Art/Mouldings/<style>.json`: the photograph's
	 * flat oak, and the narrow panel frame round Morris's wallpapers.
	 */
	static constexpr const TCHAR* DrawnStyles[] = { TEXT("photograph"), TEXT("panel") };

	/**
	 * The record of the moulding a frame of this style is made in: the style's own, where Art draws it; the museum's
	 * moulding for a frame that names no style, and for the styles Art doesn't draw yet (Albion's reeded and Watts
	 * frames, tier 2).
	 */
	[[nodiscard]] static FString RecordFor(FName FrameStyle);

	/** The moulding's face above the canvas, in units of its width. */
	UPROPERTY(meta = (MuseeRequired))
	FFormProfile Face;

	/** The width is WidthBaseMeters + WidthPerLongerSide x the canvas's longer side, held between the least and most. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double WidthBaseMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired))
	double WidthPerLongerSide = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double LeastWidthMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double MostWidthMeters = 0.0;

	/** How far the rebate stands clear of the canvas's edge all round. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double RebateClearanceMeters = 0.0;

	/** The moulding's width round a canvas of this size (metres). */
	[[nodiscard]] double WidthFor(const FVector2D& CanvasMeters) const;
};

namespace Musee::ArtMaking
{
	/** Where a frame stands out of its wall: the face its lip rests on, and its back, each measured out of the wall (m). */
	struct FFrameDepth
	{
		/** The face under the lip: a canvas's painted face, or the glazing's over a mounted print. */
		double FaceMeters = 0.0;
		/** The frame's back: on the hanging gap behind a canvas, flush with a mount's back. */
		double BackMeters = 0.0;
	};

	/**
	 * The frame round a work as a Form drawing: the moulding swept round the framed edge and mitred at its corners, its
	 * lip resting on the face under it and its back at the depth given, one closed solid. TopLeft is the framed edge's
	 * top-left corner on the wall's face as the room sees it, x along the top edge, y down the left edge, z out of the
	 * wall; the drawing's surface is the frame's line in the finish schedule.
	 */
	[[nodiscard]] FFormDrawing FrameDrawing(const FArtMoulding& Moulding, const FVector2D& FramedMeters, const FMuseeFrame& TopLeft,
		const FFrameDepth& Depth, FName Name, FName Surface);
}
