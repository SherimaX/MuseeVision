#pragma once

#include "CoreMinimal.h"

#include "NatureGround.generated.h"

/**
 * The ground a bed lies on, which Nature makes: the surface of its soil (bare, under moss, potted) or of its gravel, as a
 * full material spec, the palette key with its working and history (`accent.site-geology` / `topsoil.bare` /
 * `weathered`). A lawn is a bed of grass on its soil. Nature lays it flat at the bed anchor's height over the bed's
 * outline, less the water that lies in it.
 */
USTRUCT()
struct MUSEENATURE_API FNatureGround
{
	GENERATED_BODY()

	/** The palette key (Data/Matter/Palette.json). */
	UPROPERTY()
	FName Material;

	/** The working and history it is resolved with (Data/Matter/Shading). */
	UPROPERTY()
	FName Working;

	UPROPERTY()
	FName History;

	/** Whether the bed makes its ground at all: it names a material. */
	[[nodiscard]] bool IsSet() const { return !Material.IsNone(); }
};
