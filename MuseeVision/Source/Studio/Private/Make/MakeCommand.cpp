#include "HAL/IConsoleManager.h"
#include "Make/Make.h"
#include "Tests/ProofPlan.h"

namespace Musee::Studio
{
	namespace
	{
		void RunMakeCommand()
		{
			FMakePlan Plan = ProofPlan();
			Make(Plan).Log();
		}

		/** The make inside the open editor: makes Studio's proof stage and its museum level, and logs the report. */
		FAutoConsoleCommand MakeCommand(
			TEXT("musee.studio.make"),
			TEXT("Makes Studio's proof stage and its museum level under /Game/Generated/Studio, writing only what changed, and logs the make report."),
			FConsoleCommandDelegate::CreateStatic(&RunMakeCommand));
	}
}
