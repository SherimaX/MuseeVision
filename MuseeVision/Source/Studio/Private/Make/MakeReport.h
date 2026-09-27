#pragma once

#include "Cast/Caster.h"
#include "CoreMinimal.h"
#include "Make/MadePackage.h"
#include "Make/MakeMeasures.h"
#include "MuseeFinding.h"

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
		/** What the make found wrong with what it made: unmade drawings, open shells, unresolved materials. */
		TArray<FMuseeFinding> Findings;

		int32 Count(EMadeOutcome Outcome) const;
		/** True when every package was made or left unchanged and nothing was found wrong. */
		bool Succeeded() const { return Count(EMadeOutcome::Failed) == 0 && Findings.IsEmpty(); }
		double Seconds() const;

		/** Adds another make's packages, steps, casting and findings to this one. */
		void Append(const FMakeReport& Other);

		/** Writes the report to LogMuseeStudio: the summary, then every package written or failed, then every finding. */
		void Log() const;
	};
}
