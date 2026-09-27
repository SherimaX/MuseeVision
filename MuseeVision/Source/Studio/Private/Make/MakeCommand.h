#pragma once

#include "CoreMinimal.h"

namespace Musee::Studio
{
	/** Registers musee.studio.make with Foundation's commands (and so the console), while Studio is loaded. */
	void RegisterMakeCommand();
	void UnregisterMakeCommand();
}
