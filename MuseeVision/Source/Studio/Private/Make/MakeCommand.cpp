#include "Make/MakeCommand.h"

#include "MuseeCommand.h"
#include "Stages/StageMake.h"

namespace Musee::Studio
{
	namespace
	{
		/** The command's name, owned by Studio. */
		const TCHAR* const MakeCommandName = TEXT("musee.studio.make");

		bool RunMake(const TArray<FString>& Args, UWorld*, FOutputDevice& Output)
		{
			if (Args.Num() != 1)
			{
				Output.Log(TEXT("musee.studio.make takes one name: a stage's key, site, or all."));
				return false;
			}
			FMakeReport Report;
			FString Why;
			if (!MakeByName(Args[0], Report, Why))
			{
				Output.Log(Why);
				return false;
			}
			Report.Log();
			Output.Logf(TEXT("musee.studio.make %s: %d written, %d unchanged, %d failed, %d findings (see LogMuseeStudio)."), *Args[0],
				Report.Count(EMadeOutcome::Written), Report.Count(EMadeOutcome::Unchanged), Report.Count(EMadeOutcome::Failed), Report.Findings.Num());
			return true;
		}
	}

	void RegisterMakeCommand()
	{
		FMuseeCommand::Register({FName(MakeCommandName),
			TEXT("musee.studio.make <stage|site|all>: makes a stage's level, the museum level, or every stage then the museum, writing only what changed."),
			&RunMake});
	}

	void UnregisterMakeCommand()
	{
		FMuseeCommand::Unregister(FName(MakeCommandName));
	}
}
