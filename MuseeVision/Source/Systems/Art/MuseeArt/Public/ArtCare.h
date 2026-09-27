#pragma once

#include "ArtLightDose.h"
#include "CoreMinimal.h"

/**
 * The museum's care classes (Data/Art/Care.json): conservation lighting limits by medium, the only lux-by-medium
 * numbers in the museum. A hung work names its class; Light's checks hold the work to it.
 *
 * Care.json also lists the reproductions the museum may hang in an original's place (the Hall of Light's exhibition
 * facsimiles). A reproduction keeps its original's care class, so its record says what the original needs, but one
 * that is replaced as it fades rather than conserved is not held to the class's limits: care accepts it under any light.
 */
namespace Musee::Art
{
	/** The care classes' keys (`oil-and-tempera`, `works-on-paper`...), or why Care.json can't be read. */
	[[nodiscard]] MUSEEART_API TValueOrError<TArray<FName>, FString> CareClasses();

	/** The reproductions' keys (`exhibition-facsimile`), or why Care.json can't be read. */
	[[nodiscard]] MUSEEART_API TValueOrError<TArray<FName>, FString> Reproductions();

	/**
	 * The words a label puts before the original's holder when a reproduction hangs ("Exhibition facsimile after the
	 * original in"), or why there are none: Care.json doesn't read or lists no such reproduction.
	 */
	[[nodiscard]] MUSEEART_API TValueOrError<FString, FString> ReproductionCredit(FName Reproduction);

	/**
	 * Why a work's light is outside its care, one clause each, or none when it is within: its illuminance, annual
	 * exposure and ultraviolet against its class's limits (a limit Care.json leaves unset binds nothing). What hangs is
	 * the original when Reproduction is none; a reproduction that isn't held to care is within under any light. Fails
	 * when Care.json doesn't read or doesn't know the class or the reproduction.
	 */
	[[nodiscard]] MUSEEART_API TValueOrError<TArray<FString>, FString> CareFaults(FName CareClass, FName Reproduction, const FArtLightDose& Dose);
}
