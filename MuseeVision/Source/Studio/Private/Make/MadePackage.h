#pragma once

#include "CoreMinimal.h"

namespace Musee::Studio
{
	/** What a make did to one package. */
	enum class EMadeOutcome : uint8
	{
		/** Its stored input hash matched: the package on disk was left as it was. */
		Unchanged,
		/** Its inputs changed or it was new: it was made and saved. */
		Written,
		/** It could not be made; Why says what stopped it. */
		Failed,
	};

	/** One package a make touched, and what became of it. */
	struct FMadePackage
	{
		FString PackageName;
		EMadeOutcome Outcome = EMadeOutcome::Failed;
		FString Why;
	};
}
