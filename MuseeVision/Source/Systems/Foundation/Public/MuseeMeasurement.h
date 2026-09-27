#pragma once

#include "CoreMinimal.h"
#include "Math/UnitConversion.h"
#include "MuseeKey.h"
#include "MuseeUnits.h"

#include <limits>

#include "MuseeMeasurement.generated.h"

/**
 * One reading of an instrument: a value in its unit, the range the museum expects of it, and where and on what it was
 * taken. A probe returns readings (the lux meter's illuminance at the reticle, next to its target); a check's finding
 * carries the reading that failed.
 */
USTRUCT()
struct MUSEEFOUNDATION_API FMuseeMeasurement
{
	GENERATED_BODY()

	/** What was measured, in plain words (`illuminance on the canvas`). */
	UPROPERTY()
	FString Label;

	/** The value, in Unit. */
	UPROPERTY()
	double Value = 0.0;

	/** The unit of Value and of the expected range. */
	UPROPERTY()
	EUnit Unit = EUnit::Unspecified;

	/** The least value expected; minus infinity where nothing is expected below. */
	UPROPERTY()
	double ExpectedMinimum = -std::numeric_limits<double>::infinity();

	/** The greatest value expected; infinity where nothing is expected above. */
	UPROPERTY()
	double ExpectedMaximum = std::numeric_limits<double>::infinity();

	/** The thing measured (a piece, a source, a surface), or the none key. */
	UPROPERTY()
	FMuseeKey Subject;

	/** The frame Location is given in: a stage's key when a stage was measured alone, `museum` for the whole. */
	UPROPERTY()
	FMuseeKey FrameKey;

	/** Where the reading was taken, in the frame FrameKey names. */
	UPROPERTY(meta = (Units = "Meters"))
	FVector LocationMeters = FVector::ZeroVector;

	/** A reading of a quantity. */
	template <EUnit TheUnit>
	[[nodiscard]] static FMuseeMeasurement Of(const FString& InLabel, Musee::TQuantity<TheUnit> Quantity)
	{
		FMuseeMeasurement Measurement;
		Measurement.Label = InLabel;
		Measurement.Value = Quantity.Get();
		Measurement.Unit = TheUnit;
		return Measurement;
	}

	/** The same reading, expected within [Minimum, Maximum] in its own unit. */
	template <EUnit TheUnit>
	[[nodiscard]] FMuseeMeasurement Expecting(Musee::TQuantity<TheUnit> Minimum, Musee::TQuantity<TheUnit> Maximum) const
	{
		ensureMsgf(TheUnit == Unit, TEXT("A reading's expected range is given in the reading's own unit."));
		FMuseeMeasurement Measurement = *this;
		Measurement.ExpectedMinimum = Minimum.Get();
		Measurement.ExpectedMaximum = Maximum.Get();
		return Measurement;
	}

	/** The same reading, taken on a thing at a place in a frame. */
	[[nodiscard]] FMuseeMeasurement At(const FMuseeKey& InSubject, const FMuseeKey& InFrameKey, const FVector& InLocationMeters) const;

	/** Whether the museum expects anything of the value. */
	[[nodiscard]] bool HasExpectation() const;

	/** Whether the value lies within its expected range (always, when nothing is expected). */
	[[nodiscard]] bool IsExpected() const;

	/** The reading in one line: `illuminance on the canvas: 180 lx (expected 150 to 250 lx) on ... at ...`. */
	[[nodiscard]] FString ToString() const;
};
