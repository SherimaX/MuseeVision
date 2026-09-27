#include "MotionExtentSpec.h"

double FMotionExtentSpec::ClosedLength() const
{
	return Length.IsSound() ? FMath::Min(Length.Out[0], Length.Out[1]) : 0.0;
}

double FMotionExtentSpec::StageExtension(int32 Stage, double InLength) const
{
	if (!Telescope.IsValidIndex(Stage))
	{
		return 0.0;
	}
	double Before = 0.0;
	for (int32 Index = 0; Index < Stage; ++Index)
	{
		Before += Telescope[Index].Stroke;
	}
	return FMath::Clamp(InLength - ClosedLength() - Before, 0.0, Telescope[Stage].Stroke);
}

double FMotionExtentSpec::StageOffset(int32 Stage, double InLength) const
{
	double Offset = 0.0;
	for (int32 Index = 0; Index <= Stage && Telescope.IsValidIndex(Index); ++Index)
	{
		Offset += StageExtension(Index, InLength);
	}
	return Offset;
}

int32 FMotionExtentSpec::TurnsAt(double InLength) const
{
	// A turn is whole once a thousandth of a millimetre of it has left the housing: below that the column is at rest.
	constexpr double FormedMeters = 1.0e-6;
	return Turns.IsSet() && InLength > FormedMeters ? FMath::CeilToInt32((InLength - FormedMeters) / Turns.Pitch) : 0;
}

FName FMotionExtentSpec::StagePiece(int32 Stage) const
{
	return FName(FString::Printf(TEXT("%s.stage-%d"), *Key.ToString(), Stage + 1));
}

FName FMotionExtentSpec::TurnPiece() const
{
	return FName(Key.ToString() + TEXT(".turn"));
}
