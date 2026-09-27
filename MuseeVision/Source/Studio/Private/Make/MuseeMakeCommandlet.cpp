#include "Make/MuseeMakeCommandlet.h"

#include "Make/Make.h"
#include "ShaderCompiler.h"
#include "Tests/ProofPlan.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MuseeMakeCommandlet)

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
	Musee::Studio::FMakePlan Plan = Musee::Studio::ProofPlan();
	const Musee::Studio::FMakeReport Report = Musee::Studio::Make(Plan);
	Report.Log();
	// The shader-job cache's queries and hits over the whole run, from the engine's own summary.
	if (GShaderCompilingManager)
	{
		GShaderCompilingManager->PrintStats();
	}
	return Report.Succeeded() ? 0 : 1;
}
