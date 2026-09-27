#pragma once

#include "CoreMinimal.h"
#include "Objects/HangingRecords.h"

#include "HandscrollRecords.generated.h"

/**
 * How Art makes the fittings of a Chinese handscroll (`Data/Art/Handscroll.json`), the parts every handscroll's mount
 * shares whatever its length: the half-round stave (tiangan) at its outer end, round which its wrapper (baoshou) is
 * wound; the roller (zhou) at its inner end that its length is wound on, and the knobs (zhoutou) on the roller's ends.
 * The silk's thickness is the hanging scroll's (`Data/Art/Scrolls.json`): both are silk and paper on the same backing.
 */
USTRUCT()
struct FArtHandscrollFittings
{
	GENERATED_BODY()

	/** The record, under Data. */
	static constexpr const TCHAR* Record = TEXT("Art/Handscroll.json");

	/** The stave: a half-round batten across the outer end, its flat under the silk, of this radius. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double StaveRadiusMeters = 0.0;

	/** The roller the scroll is wound on, round, of this radius. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double RollerRadiusMeters = 0.0;

	/** Each knob: a turned end of this radius, standing this far past the scroll's edge on the roller's axis. */
	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double KnobRadiusMeters = 0.0;

	UPROPERTY(meta = (MuseeRequired, Units = "Meters"))
	double KnobLengthMeters = 0.0;

	/** The wrapper's length, as turns round the scroll rolled whole: once round it, and the overlap its tie holds. */
	UPROPERTY(meta = (MuseeRequired))
	double WrapperTurns = 0.0;

	/**
	 * How far a photograph's proportion may stray from the catalogue's and still be taken as the whole sheet, as a share
	 * (a catalogue's rounding, a sheet's wandering edges, a scan flattened from a scroll that never lies quite flat).
	 */
	UPROPERTY(meta = (MuseeRequired))
	double ProportionTolerance = 0.0;

	/** The stave's and the roller's wood, as the palette finishes it. */
	UPROPERTY(meta = (MuseeRequired))
	FArtFinishSpec Stave;

	UPROPERTY(meta = (MuseeRequired))
	FArtFinishSpec Roller;
};

/**
 * The window of one photograph that shows the work, the rest being what the photograph caught beside it (the mount's
 * silk, a separator, a frontispiece, a neighbouring sheet): shares of its width from its left edge, and of its height from
 * its top edge. A side left out is the photograph's own edge.
 */
USTRUCT()
struct FArtHandscrollWindow
{
	GENERATED_BODY()

	/** The photograph's number, `<work>-<image>` (1 for a work shown from its one photograph). */
	UPROPERTY(meta = (MuseeRequired))
	int32 Image = 0;

	UPROPERTY()
	double From = 0.0;

	UPROPERTY()
	double To = 1.0;

	UPROPERTY()
	double Top = 0.0;

	UPROPERTY()
	double Bottom = 1.0;
};

/**
 * A handscroll as its photographs show it (`Data/Art/Handscrolls.json`, one record a work): the order its numbered
 * photographs are read in, from its beginning at the right to its end at the left, established from the photographs
 * themselves, with that evidence; and where a photograph shows more than the work, the share of it that is the work.
 * A work shown from its one photograph gives no order.
 */
USTRUCT()
struct FArtHandscroll
{
	GENERATED_BODY()

	/** The record, under Data. */
	static constexpr const TCHAR* Record = TEXT("Art/Handscrolls.json");

	UPROPERTY(meta = (MuseeRequired))
	FString Work;

	/** The photographs' numbers in reading order, right to left: the first is the scroll's beginning. */
	UPROPERTY()
	TArray<int32> ReadingOrder;

	UPROPERTY()
	TArray<FArtHandscrollWindow> Windows;

	/**
	 * The mount's parts before the work, at its right end, as the museum's record measures them along the scroll: the
	 * separator (前隔水) of damask next to the work, and the frontispiece (引首) beyond it, on its own paper. Nought where
	 * the record gives none. What of them the stage doesn't lay open past the work's band and lead is wound on the stave.
	 */
	UPROPERTY(meta = (Units = "Meters"))
	double SeparatorMeters = 0.0;

	UPROPERTY(meta = (Units = "Meters"))
	double FrontispieceMeters = 0.0;

	/** How the order and the windows were established, from the photographs and the scroll's composition. */
	UPROPERTY(meta = (MuseeRequired))
	FString Evidence;
};
