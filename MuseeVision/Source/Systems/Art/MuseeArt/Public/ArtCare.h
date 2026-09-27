#pragma once

#include "CoreMinimal.h"

/**
 * The museum's care classes (Data/Art/Care.json): conservation lighting limits by medium, the only lux-by-medium
 * numbers in the museum. A hung work names its class; Light's checks hold the work to it.
 */
namespace Musee::Art
{
	/** The care classes' keys (`oil-and-tempera`, `works-on-paper`...), or why Care.json can't be read. */
	[[nodiscard]] MUSEEART_API TValueOrError<TArray<FName>, FString> CareClasses();
}
