#pragma once

#include "CoreMinimal.h"
#include "Templates/ValueOrError.h"

namespace Musee::Light
{
	/**
	 * A maker's photometric file as its laboratory measured it: an IESNA LM-63 file's candela table, the luminous
	 * intensity of the luminaire in each direction about it, in candelas. Light reads it for the numbers the table fixes
	 * (the flux it integrates to and its peak); the light profile the engine renders it with is imported from the same
	 * file by Studio.
	 *
	 * The table is Type C (LM-63's photometric type 1), the one museum luminaires are measured in: the vertical angle is
	 * measured from the luminaire's nadir, its optical axis, 0° to 180°, and the horizontal angle turns about that axis,
	 * 0° being the plane the maker marks on the luminaire.
	 */
	struct FPhotometricFile
	{
		/** LM-63's photometric type: 1 for Type C, 2 for Type B, 3 for Type A. */
		int32 PhotometricType = 0;

		/** The vertical angles the table is measured at, in degrees, rising. */
		TArray<double> VerticalDegrees;

		/** The horizontal angles, in degrees, rising. */
		TArray<double> HorizontalDegrees;

		/**
		 * The intensity at each pair of angles, in candelas, the file's multiplier and ballast factor applied: the vertical
		 * angles of the first horizontal angle, then of the next, as the file lists them.
		 */
		TArray<double> Candela;

		/**
		 * The luminous opening's width, length and height, in metres; a negative width and length mark a round opening of
		 * that diameter (LM-63-2002, 5.9).
		 */
		FVector OpeningMeters = FVector::ZeroVector;

		/** The input power the luminaire was measured at, in watts. */
		double InputWatts = 0.0;

		/** The intensity at the table's HorizontalIndex-th horizontal and VerticalIndex-th vertical angle, in candelas. */
		[[nodiscard]] double At(int32 HorizontalIndex, int32 VerticalIndex) const
		{
			return Candela[HorizontalIndex * VerticalDegrees.Num() + VerticalIndex];
		}

		/** The greatest intensity in the table, in candelas. */
		[[nodiscard]] double PeakCandela() const;

		/** A direction in the table's Type C frame: its vertical angle from the nadir and its horizontal angle, in degrees. */
		struct FDirection
		{
			double VerticalDegrees = 0.0;
			double HorizontalDegrees = 0.0;
		};

		/**
		 * The direction of the table's greatest intensity (the first the table lists, where several are equal): where the
		 * luminaire throws its light, the direction a designer aims.
		 */
		[[nodiscard]] FDirection PeakDirection() const;

		/**
		 * The luminous flux the table carries, in lumens: its intensity integrated over the whole sphere, cell by cell of
		 * the measured grid (the mean of each cell's four corners over the cell's solid angle), the horizontal range the file
		 * measures repeated round the axis by the symmetry LM-63 gives it (0° alone: all round; 0° to 90°: each quadrant;
		 * 0° to 180°: each side of the 0°–180° plane). Only a Type C table has this frame.
		 */
		[[nodiscard]] double FluxLumens() const;

		/**
		 * The flux the table sends into the half of the horizontal circle nearer 0° (within 90° of it), in lumens: where an
		 * asymmetric lens throws its light.
		 */
		[[nodiscard]] double FluxTowardsZeroPlaneLumens() const;

		/** The radius of a round luminous opening, in metres, or unset if the opening isn't round. */
		[[nodiscard]] TOptional<double> RoundOpeningRadiusMeters() const;
	};

	/**
	 * Reads an LM-63 file (the 1986, 1991, 1995 or 2002 edition) at Path. Only a file whose lamp is measured in the
	 * luminaire as it hangs (`TILT=NONE`) is read; the reason it can't be read otherwise.
	 */
	TValueOrError<FPhotometricFile, FString> ReadPhotometricFile(const FString& Path);

	/** Reads an LM-63 file from its text. */
	TValueOrError<FPhotometricFile, FString> ParsePhotometricFile(const FString& Text);
}
