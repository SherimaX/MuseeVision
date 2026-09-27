#pragma once

#include "CoreMinimal.h"
#include "FormTypeface.h"

/**
 * A line of cast or cut letters set on a bed: a motto of bronze capitals let into a floor, a dedication along a frieze.
 * Each letter is one casting, straight as the founder makes it, set with its baseline on the line and its middle on it:
 * along the frame's x from its origin, or round a circle about the frame's origin, each letter square to the circle at its
 * own middle. The letters stand ProudMeters along the frame's z from their bed, their top edges bevelled by BevelMeters
 * (a founder's chamfer, measured across the face), and the line is centred on its place. A space moves the pen on by a
 * word space; an interpunct (U+00B7, the Roman inscription's stop between words) moves it on by a stop's gap and sets a
 * triangular stop in it, point down, a low pyramid rising to its apex; the typeface must hold every other character. The
 * whole line is one piece of as many closed solids as it has letters and stops, the stops in their own part.
 */
struct FFormLettering
{
	/** The stops' part, which the drawing may finish apart from the letters (a gilt stop among patinated capitals). */
	static constexpr const TCHAR* StopPart = TEXT("stop");

	/** The character set as a triangular stop. */
	static constexpr TCHAR Interpunct = TCHAR(0x00B7);

	/** The text, in the typeface's letters, spaces and interpuncts. */
	FString Text;

	/** The letters' outlines (Musee::Form::FindTypeface). */
	FFormTypeface Typeface;

	/** The cap height, metres. */
	double CapMeters = 0.0;

	/** The space between two letters of a word, and a word space, in cap heights. */
	double TrackingCaps = 0.0;
	double WordSpaceCaps = 0.0;

	/**
	 * A stop: the gap it stands in, its triangle's half-width at its top edge, the heights of that edge and of its point
	 * above the baseline, in cap heights; and the height of its apex above the bed, metres.
	 */
	double StopGapCaps = 0.0;
	double StopHalfWidthCaps = 0.0;
	double StopTopCaps = 0.0;
	double StopPointCaps = 0.0;
	double StopApexMeters = 0.0;

	/**
	 * Zero: the baseline runs along the frame's x, the letters' tops towards its y, and the line is centred on the origin.
	 * Otherwise the baseline is the circle of this radius about the frame's origin, and the line is centred on the plan
	 * angle CentreDegrees (from the frame's x towards its y).
	 */
	double BaselineRadiusMeters = 0.0;
	double CentreDegrees = 0.0;

	/**
	 * On a circle: the letters' tops point towards the centre, and the line reads the way the angle falls (as it does for
	 * someone standing outside the circle and facing its centre); otherwise their tops point away from it and the line
	 * reads the way the angle rises.
	 */
	bool bTopsInward = true;

	/** How far the letters stand from their bed, and the bevel round their faces, metres. */
	double ProudMeters = 0.0;
	double BevelMeters = 0.0;
};
