#include "Stages/MaterialLookup.h"

// Matter's resolver is written beside Studio's make (order B-MATTER-01) and joins it on merge; until then a build has
// no resolver and every material is a finding.
#if __has_include("MatterResolve.h")
#include "MatterResolve.h"
#define MUSEE_STUDIO_WITH_MATTER_RESOLVE 1
#else
#define MUSEE_STUDIO_WITH_MATTER_RESOLVE 0
#endif

namespace Musee::Studio
{
	FMaterialResolver MatterResolver()
	{
#if MUSEE_STUDIO_WITH_MATTER_RESOLVE
		return [](FName PaletteKey, FName Working, FName History) { return Musee::Matter::Resolve(PaletteKey, Working, History); };
#else
		return [](FName, FName, FName) -> UMaterialInterface* { return nullptr; };
#endif
	}

	bool HasMatterResolver()
	{
		return MUSEE_STUDIO_WITH_MATTER_RESOLVE != 0;
	}
}
