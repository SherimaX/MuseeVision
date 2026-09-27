#pragma once

#include "CoreMinimal.h"
#include "Delegates/IDelegateInstance.h"

namespace Musee::Studio
{
	/**
	 * What the asset registry records for the assets Studio makes, from the moment Studio loads:
	 *  - each made asset's input hash, so a make can tell an unchanged asset on disk without loading it;
	 *  - for a made level, the fixed DateModified the engine gives procedurally saved worlds (FDateTime::MinValue)
	 *    rather than the clock. A made level is procedural in the engine's own sense (no one edits it by hand), and a
	 *    clock in the file would make every save of the same level differ.
	 */
	class FMadeAssetTags
	{
	public:
		void Register();
		void Unregister();

	private:
		FDelegateHandle InputHashTag;
		FDelegateHandle LevelDateTag;
	};
}
