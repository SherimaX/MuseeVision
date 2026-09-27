#include "MuseeMatterMakeCommandlet.h"

#include "HAL/PlatformTime.h"
#include "MatterLog.h"
#include "MatterMake.h"

UMuseeMatterMakeCommandlet::UMuseeMatterMakeCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UMuseeMatterMakeCommandlet::Main(const FString& Params)
{
	const double Start = FPlatformTime::Seconds();
	const Musee::Matter::FMatterMakeReport Report = Musee::Matter::MakeMatter(true);
	for (const FString& Line : Report.Lines)
	{
		UE_LOG(LogMuseeMatter, Display, TEXT("%s"), *Line);
	}
	for (const FString& Problem : Report.Problems)
	{
		UE_LOG(LogMuseeMatter, Error, TEXT("%s"), *Problem);
	}
	UE_LOG(LogMuseeMatter, Display, TEXT("MuseeMatterMake: %d written, %d unchanged, %d problems, in %.1f s."),
		Report.Written, Report.Unchanged, Report.Problems.Num(), FPlatformTime::Seconds() - Start);
	return Report.Succeeded() ? 0 : 1;
}
