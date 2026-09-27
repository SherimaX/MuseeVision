#pragma once

#include "CoreMinimal.h"
#include "MatterTriple.h"

#include "MatterSheet.generated.h"

/**
 * A thin sheet that scatters the light it passes (Sheet.ush): a linen stretched as a velarium or a scrim, an
 * opal-flashed pane. Light that enters one face leaves by either face, scattered; what leaves by the far face is the
 * sheet's diffuse transmittance, measured with its reflectance in an integrating sphere (CIE 130-1998).
 */
USTRUCT()
struct MUSEEMATTER_API FMatterSheet
{
	GENERATED_BODY()

	/** The sheet's diffuse transmittance at the primaries (unit 1, linear Rec.709). */
	UPROPERTY()
	FMatterTriple Transmittance;
};
