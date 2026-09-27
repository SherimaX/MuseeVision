#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Editor.h"
#include "Misc/AutomationTest.h"

namespace Musee::Studio
{
	/** Leaves the editor on a fresh untitled level after a test has played a made level, so no made level stays open. */
	class FOpenUntitledLevel : public IAutomationLatentCommand
	{
	public:
		virtual bool Update() override
		{
			GEditor->CreateNewMapForEditing(/*bPromptUserToSave*/ false);
			return true;
		}
	};
}

#endif
