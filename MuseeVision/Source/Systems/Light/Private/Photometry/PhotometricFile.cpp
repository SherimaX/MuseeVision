#include "Photometry/PhotometricFile.h"

#include "Misc/FileHelper.h"

namespace Musee::Light
{
	namespace
	{
		/** LM-63's photometric type for a Type C table. */
		constexpr int32 TypeC = 1;

		/** LM-63's units type for dimensions in feet, and in metres. */
		constexpr int32 FeetUnits = 1;
		constexpr int32 MeterUnits = 2;

		/** A foot is 0.3048 m exactly (the international foot of 1959). */
		constexpr double MetersPerFoot = 0.3048;

		/** The numbers on the two lines after TILT=NONE, before the angles (LM-63-2002, 5.2 to 5.14). */
		constexpr int32 HeaderNumbers = 13;

		/** A full turn, and half of one, in degrees. */
		constexpr double FullTurnDegrees = 360.0;
		constexpr double HalfTurnDegrees = 180.0;

		/** The line that says the lamp is measured in the luminaire as it hangs, with no tilt table. */
		const TCHAR* const TiltNone = TEXT("TILT=NONE");
		const TCHAR* const TiltPrefix = TEXT("TILT=");

		/** The numbers after the TILT line, or the reason they aren't all numbers. */
		TValueOrError<TArray<double>, FString> NumbersAfterTilt(const FString& Text)
		{
			TArray<FString> Lines;
			Text.ParseIntoArrayLines(Lines, false);
			int32 Tilt = INDEX_NONE;
			for (int32 Index = 0; Index < Lines.Num() && Tilt == INDEX_NONE; ++Index)
			{
				if (Lines[Index].TrimStartAndEnd().StartsWith(TiltPrefix)) { Tilt = Index; }
			}
			if (Tilt == INDEX_NONE) { return MakeError(TEXT("it has no TILT line")); }
			if (Lines[Tilt].TrimStartAndEnd() != TiltNone) { return MakeError(TEXT("its lamp is tilted in the luminaire (only TILT=NONE is read)")); }
			TArray<double> Numbers;
			for (int32 Index = Tilt + 1; Index < Lines.Num(); ++Index)
			{
				TArray<FString> Words;
				Lines[Index].Replace(TEXT(","), TEXT(" ")).ParseIntoArrayWS(Words);
				for (const FString& Word : Words)
				{
					if (Word == TEXT("END")) { continue; }
					if (!Word.IsNumeric()) { return MakeError(FString::Printf(TEXT("'%s' after its TILT line isn't a number"), *Word)); }
					Numbers.Add(FCString::Atod(*Word));
				}
			}
			return MakeValue(MoveTemp(Numbers));
		}

		/** Whether angles rise, as LM-63 lists them. */
		bool Rising(const TArray<double>& Angles)
		{
			for (int32 Index = 1; Index < Angles.Num(); ++Index)
			{
				if (Angles[Index] < Angles[Index - 1]) { return false; }
			}
			return true;
		}

		/**
		 * The horizontal angles a measured angle stands for round the whole axis, by the symmetry LM-63 gives the range the
		 * file measures (LM-63-2002, 5.16): 0° to 90°, each quadrant; 0° to 180°, each side of the 0°–180° plane; 90° to
		 * 270°, each side of the 90°–270° plane; 0° to 360°, itself.
		 */
		TArray<double, TInlineAllocator<4>> Images(const FPhotometricFile& File, double Horizontal)
		{
			const double First = File.HorizontalDegrees[0];
			const double Last = File.HorizontalDegrees.Last();
			if (First == 0.0 && Last == 90.0)
			{
				return {Horizontal, HalfTurnDegrees - Horizontal, HalfTurnDegrees + Horizontal, FullTurnDegrees - Horizontal};
			}
			if (First == 0.0 && Last == HalfTurnDegrees) { return {Horizontal, FullTurnDegrees - Horizontal}; }
			if (First == 90.0 && Last == 270.0) { return {Horizontal, FMath::Fmod(FullTurnDegrees + HalfTurnDegrees - Horizontal, FullTurnDegrees)}; }
			return {Horizontal};
		}

		/**
		 * The flux the table sends into the horizontal directions Keep accepts (angles in degrees, 0 to 360), in lumens,
		 * integrated cell by cell over the sphere with the file's symmetry. A table of one horizontal angle is the same all
		 * round the axis.
		 */
		double FluxWhere(const FPhotometricFile& File, TFunctionRef<bool(double)> Keep)
		{
			const TArray<double>& Vertical = File.VerticalDegrees;
			const TArray<double>& Horizontal = File.HorizontalDegrees;
			double Lumens = 0.0;
			if (Horizontal.Num() == 1)
			{
				// The same all round: the share of the turn Keep accepts, counted a degree at a time.
				double Share = 0.0;
				for (int32 Degree = 0; Degree < static_cast<int32>(FullTurnDegrees); ++Degree)
				{
					Share += Keep(Degree + 0.5) ? 1.0 : 0.0;
				}
				for (int32 V = 0; V + 1 < Vertical.Num(); ++V)
				{
					const double Band = FMath::Cos(FMath::DegreesToRadians(Vertical[V])) - FMath::Cos(FMath::DegreesToRadians(Vertical[V + 1]));
					Lumens += 0.5 * (File.At(0, V) + File.At(0, V + 1)) * Band;
				}
				return Lumens * 2.0 * UE_DOUBLE_PI * Share / FullTurnDegrees;
			}
			for (int32 H = 0; H + 1 < Horizontal.Num(); ++H)
			{
				const double Middle = 0.5 * (Horizontal[H] + Horizontal[H + 1]);
				double Share = 0.0;
				for (const double Image : Images(File, Middle))
				{
					Share += Keep(Image) ? 1.0 : 0.0;
				}
				const double Wedge = FMath::DegreesToRadians(Horizontal[H + 1] - Horizontal[H]);
				for (int32 V = 0; V + 1 < Vertical.Num(); ++V)
				{
					const double Band = FMath::Cos(FMath::DegreesToRadians(Vertical[V])) - FMath::Cos(FMath::DegreesToRadians(Vertical[V + 1]));
					const double Mean = 0.25 * (File.At(H, V) + File.At(H, V + 1) + File.At(H + 1, V) + File.At(H + 1, V + 1));
					Lumens += Share * Mean * Band * Wedge;
				}
			}
			return Lumens;
		}
	}

	double FPhotometricFile::PeakCandela() const
	{
		double Peak = 0.0;
		for (const double Value : Candela)
		{
			Peak = FMath::Max(Peak, Value);
		}
		return Peak;
	}

	FPhotometricFile::FDirection FPhotometricFile::PeakDirection() const
	{
		FDirection Peak;
		double Greatest = -1.0;
		for (int32 H = 0; H < HorizontalDegrees.Num(); ++H)
		{
			for (int32 V = 0; V < VerticalDegrees.Num(); ++V)
			{
				if (At(H, V) > Greatest)
				{
					Greatest = At(H, V);
					Peak = FDirection{VerticalDegrees[V], HorizontalDegrees[H]};
				}
			}
		}
		return Peak;
	}

	double FPhotometricFile::FluxLumens() const
	{
		return FluxWhere(*this, [](double) { return true; });
	}

	double FPhotometricFile::FluxTowardsZeroPlaneLumens() const
	{
		return FluxWhere(*this, [](double Horizontal) { return FMath::Cos(FMath::DegreesToRadians(Horizontal)) > 0.0; });
	}

	TOptional<double> FPhotometricFile::RoundOpeningRadiusMeters() const
	{
		if (OpeningMeters.X < 0.0 && OpeningMeters.Y < 0.0) { return -0.5 * OpeningMeters.X; }
		return {};
	}

	TValueOrError<FPhotometricFile, FString> ParsePhotometricFile(const FString& Text)
	{
		TValueOrError<TArray<double>, FString> Read = NumbersAfterTilt(Text);
		if (Read.HasError()) { return MakeError(Read.StealError()); }
		const TArray<double>& Numbers = Read.GetValue();
		if (Numbers.Num() < HeaderNumbers) { return MakeError(TEXT("its header after the TILT line is short")); }
		const double Multiplier = Numbers[2];
		const int32 VerticalCount = FMath::RoundToInt32(Numbers[3]);
		const int32 HorizontalCount = FMath::RoundToInt32(Numbers[4]);
		const int32 Units = FMath::RoundToInt32(Numbers[6]);
		const double BallastFactor = Numbers[10];
		if (VerticalCount < 2 || HorizontalCount < 1) { return MakeError(TEXT("its table has too few angles")); }
		if (Units != FeetUnits && Units != MeterUnits) { return MakeError(TEXT("its units are neither feet nor metres")); }
		if (Numbers.Num() != HeaderNumbers + VerticalCount + HorizontalCount + VerticalCount * HorizontalCount)
		{
			return MakeError(FString::Printf(TEXT("it holds %d numbers after its TILT line, not the %d its %d by %d table needs"), Numbers.Num(),
				HeaderNumbers + VerticalCount + HorizontalCount + VerticalCount * HorizontalCount, VerticalCount, HorizontalCount));
		}
		FPhotometricFile File;
		File.PhotometricType = FMath::RoundToInt32(Numbers[5]);
		const double Meters = Units == FeetUnits ? MetersPerFoot : 1.0;
		File.OpeningMeters = FVector(Numbers[7], Numbers[8], Numbers[9]) * Meters;
		File.InputWatts = Numbers[12];
		int32 Next = HeaderNumbers;
		File.VerticalDegrees.Append(&Numbers[Next], VerticalCount);
		Next += VerticalCount;
		File.HorizontalDegrees.Append(&Numbers[Next], HorizontalCount);
		Next += HorizontalCount;
		if (!Rising(File.VerticalDegrees) || !Rising(File.HorizontalDegrees)) { return MakeError(TEXT("its angles don't rise")); }
		File.Candela.Reserve(VerticalCount * HorizontalCount);
		for (; Next < Numbers.Num(); ++Next)
		{
			File.Candela.Add(Numbers[Next] * Multiplier * BallastFactor);
		}
		return MakeValue(MoveTemp(File));
	}

	TValueOrError<FPhotometricFile, FString> ReadPhotometricFile(const FString& Path)
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *Path)) { return MakeError(FString::Printf(TEXT("%s can't be read"), *Path)); }
		return ParsePhotometricFile(Text);
	}
}
