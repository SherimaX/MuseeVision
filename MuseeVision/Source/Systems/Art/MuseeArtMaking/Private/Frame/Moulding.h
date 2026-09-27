#pragma once

#include "CoreMinimal.h"
#include "FormDrawing.h"
#include "FormProfile.h"
#include "MuseeFrame.h"

#include "Moulding.generated.h"

/**
 * The museum's picture-frame moulding, `Data/Art/Moulding.json`: the carved and gilt section a framer runs round a
 * canvas, in units of the moulding's width (A outward from the canvas's edge, B up from the canvas's face, drawn from
 * the outside edge inward and ending on the canvas's face at the sight lip), and the rule that sizes the width to the
 * canvas, as frame makers proportion a moulding to the picture.
 */
USTRUCT()
struct FArtMoulding
{
	GENERATED_BODY()

	/** The record, under Data. */
	static constexpr const TCHAR* Record = TEXT("Art/Moulding.json");

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
	/**
	 * The frame round a canvas as a Form drawing: the moulding swept round the canvas's edge and mitred at its corners,
	 * its lip resting on the canvas's face and its back on the wall, one closed solid. CanvasTopLeft is the canvas's
	 * top-left corner on the wall's face as the room sees it, x along the top edge, y down the left edge, z out of the
	 * wall; the drawing's surface is the frame's line in the finish schedule.
	 */
	[[nodiscard]] FFormDrawing FrameDrawing(const FArtMoulding& Moulding, const FVector2D& CanvasMeters, const FMuseeFrame& CanvasTopLeft,
		FName Name, FName Surface);
}
