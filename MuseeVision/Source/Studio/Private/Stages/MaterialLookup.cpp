#include "Stages/MaterialLookup.h"

#include "MatterResolve.h"

namespace Musee::Studio
{
	FMaterialResolver MatterResolver()
	{
		return [](FName PaletteKey, FName Working, FName History) { return Musee::Matter::Resolve(PaletteKey, Working, History); };
	}

	bool HasMatterResolver()
	{
		return true;
	}
}
