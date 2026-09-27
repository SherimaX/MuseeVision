#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "MatterMake.h"

class FAutomationTestBase;
class UMaterialInstanceConstant;
class UMaterialInterface;
class UMaterialParameterCollection;

/**
 * Matter's masters and instances made in memory for tests, as the make step makes them but never saved: the museum
 * Moment's collection, every master over it, and instances to compose.
 */
namespace Musee::Matter::TestMasters
{
	/** The museum Moment's collection, made in memory as the make step makes it. */
	UMaterialParameterCollection* TransientMoment();

	/** A master's asset name, for messages. */
	FString KindText(const FMasterKind& Kind);

	/** Every master, built in memory over Moment; a master that does not build is reported on Test. */
	TArray<FMadeMaster> TransientMasters(FAutomationTestBase& Test, UMaterialParameterCollection& Moment);

	/** The made master of a kind, or nullptr. */
	UMaterialInterface* FindIn(const TArray<FMadeMaster>& Masters, const FMasterKind& Kind);

	/** Retires masters made in memory. */
	void Retire(const TArray<FMadeMaster>& Masters);

	/** An empty instance in the transient package. */
	UMaterialInstanceConstant* TransientInstance();
}

#endif
