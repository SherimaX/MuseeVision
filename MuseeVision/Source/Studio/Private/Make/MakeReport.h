#pragma once

#include "Cast/Caster.h"
#include "CoreMinimal.h"
#include "Make/MadePackage.h"
#include "Make/MakeMeasures.h"

namespace Musee::Studio
{
	/** One step of a make, and how long it took. */
	struct FMakeStep
	{
		FString Name;
		double Seconds = 0.0;
	};

	/** What a make did and what it cost: every package it touched, each step's time, the casting and its DDC traffic. */
	struct FMakeReport
	{
		TArray<FMadePackage> Packages;
		TArray<FMakeStep> Steps;
		FCastStats Cast;
		/** The mesh builds' DDC traffic during the cast step: fetched (hits) against built (misses). */
		FDerivedDataTally DerivedData;
		double PeakPhysMemoryMB = 0.0;

		int32 Count(EMadeOutcome Outcome) const;
		bool Succeeded() const { return Count(EMadeOutcome::Failed) == 0; }
		double Seconds() const;

		/** Writes the report to LogMuseeStudio: the summary, then every package written or failed. */
		void Log() const;
	};
}
