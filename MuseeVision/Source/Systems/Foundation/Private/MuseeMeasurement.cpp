#include "MuseeMeasurement.h"

namespace Musee
{
	namespace
	{
		FString UnitSymbol(EUnit Unit)
		{
			return Unit == EUnit::Unspecified ? FString() : FString(TEXT(" ")) + FUnitConversion::GetUnitDisplayString(Unit);
		}
	}
}

FMuseeMeasurement FMuseeMeasurement::At(const FMuseeKey& InSubject, const FMuseeKey& InFrameKey, const FVector& InLocationMeters) const
{
	FMuseeMeasurement Measurement = *this;
	Measurement.Subject = InSubject;
	Measurement.FrameKey = InFrameKey;
	Measurement.LocationMeters = InLocationMeters;
	return Measurement;
}

bool FMuseeMeasurement::HasExpectation() const
{
	return FMath::IsFinite(ExpectedMinimum) || FMath::IsFinite(ExpectedMaximum);
}

bool FMuseeMeasurement::IsExpected() const
{
	return ExpectedMinimum <= Value && Value <= ExpectedMaximum;
}

FString FMuseeMeasurement::ToString() const
{
	const FString Symbol = Musee::UnitSymbol(Unit);
	FString Text = FString::Printf(TEXT("%s: %g%s"), *Label, Value, *Symbol);
	if (HasExpectation())
	{
		Text += FString::Printf(TEXT(" (expected %g to %g%s)"), ExpectedMinimum, ExpectedMaximum, *Symbol);
	}
	if (!Subject.IsNone())
	{
		Text += FString::Printf(TEXT(" on %s"), *Subject.ToString());
	}
	if (!FrameKey.IsNone())
	{
		Text += FString::Printf(TEXT(" at (%.3f, %.3f, %.3f) m in %s"), LocationMeters.X, LocationMeters.Y, LocationMeters.Z, *FrameKey.ToString());
	}
	return Text;
}
