#pragma once

#include "CoreMinimal.h"

class UMaterialInterface;

namespace Musee::Studio
{
	/** Turns a palette key, a working and a history into the material instance Matter made for them, or null. */
	using FMaterialResolver = TFunction<UMaterialInterface*(FName PaletteKey, FName Working, FName History)>;

	/**
	 * Matter's resolver, Musee::Matter::Resolve (MuseeMatter, MatterResolve.h). A build without it resolves nothing,
	 * so every piece's material is a finding: there is never a fallback material.
	 */
	FMaterialResolver MatterResolver();

	/** Whether this build carries Matter's resolver. */
	bool HasMatterResolver();
}
