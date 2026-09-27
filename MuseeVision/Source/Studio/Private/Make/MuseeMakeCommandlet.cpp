#include "Make/MuseeMakeCommandlet.h"

#include "Misc/Parse.h"
#include "ShaderCompiler.h"
#include "Stages/StageMake.h"
#include "StudioLog.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MuseeMakeCommandlet)

namespace
{
	/** The commandlet's exit codes: made cleanly, made with failures or findings, or asked for nothing it can make. */
	constexpr int32 MadeCleanly = 0;
	constexpr int32 MadeWithFindings = 1;
	constexpr int32 NothingToMake = 2;
}

UMuseeMakeCommandlet::UMuseeMakeCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
	ShowErrorCount = true;
}

int32 UMuseeMakeCommandlet::Main(const FString& Params)
{
	FString Name;
	if (!FParse::Value(*Params, TEXT("Stage="), Name))
	{
		UE_LOG(LogMuseeStudio, Error, TEXT("MuseeMake needs -Stage=<stage key|site|all>."));
		return NothingToMake;
	}
	Musee::Studio::FMakeReport Report;
	FString Why;
	if (!Musee::Studio::MakeByName(Name, Report, Why))
	{
		UE_LOG(LogMuseeStudio, Error, TEXT("MuseeMake: %s"), *Why);
		return NothingToMake;
	}
	Report.Log();
	// The shader-job cache's queries and hits over the whole run, from the engine's own summary.
	if (GShaderCompilingManager)
	{
		GShaderCompilingManager->PrintStats();
	}
	return Report.Succeeded() ? MadeCleanly : MadeWithFindings;
}
