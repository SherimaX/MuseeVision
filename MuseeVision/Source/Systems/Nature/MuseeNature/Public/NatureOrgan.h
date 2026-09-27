#pragma once

#include "CoreMinimal.h"

#include "NatureOrgan.generated.h"

/**
 * One kind of organ a species grows (`leaf`, `flower`, `stem`, `eye`, `seed`), and the palette key of the living
 * tissue it is made of: a lily's pad is a waxy floating leaf, its petal a thin translucent sheet. The organ's own
 * tint travels with its vertices; the palette entry gives the tissue's optics.
 */
USTRUCT()
struct MUSEENATURE_API FNatureOrgan
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired))
	FName Organ;

	UPROPERTY(meta = (MuseeRequired))
	FName Material;
};
