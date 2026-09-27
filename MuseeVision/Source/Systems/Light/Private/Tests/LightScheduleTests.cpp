#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/FileManager.h"
#include "LightLuminaire.h"
#include "Misc/Paths.h"
#include "MuseeRecords.h"
#include "Schedule/LightingRecords.h"

// The stages' lighting schedules through Light's own reader: every stage's Lighting.json reads, field for field, and
// every luminaire type a room names is a real product Light holds a record of, so no room waits on a type that doesn't
// exist and no field a sheet writes is dropped.
namespace Musee::Light::ScheduleTests
{
	inline constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** The stages' schedules, under Data. */
	constexpr const TCHAR* StagesFolder = TEXT("Stages");
	constexpr const TCHAR* LightingSchedule = TEXT("Lighting.json");

	TArray<FString> StageKeys()
	{
		TArray<FString> Stages;
		IFileManager::Get().FindFiles(Stages, *(Records::DataRoot() / StagesFolder / TEXT("*")), false, true);
		Stages.Sort();
		return Stages;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLightEveryScheduleReads, "Musee.Light.EveryScheduleReads", Flags)
	bool FLightEveryScheduleReads::RunTest(const FString& Parameters)
	{
		const TArray<FString> Stages = StageKeys();
		TestTrue(TEXT("There are stages to check"), !Stages.IsEmpty());
		int32 Named = 0;
		TSet<FString> Types;
		for (const FString& Stage : Stages)
		{
			const FString Path = Records::DataRoot() / StagesFolder / Stage / LightingSchedule;
			if (!FPaths::FileExists(Path))
			{
				AddError(FString::Printf(TEXT("%s has no lighting schedule."), *Stage));
				continue;
			}
			const TValueOrError<FLightSchedule, FString> Read = ReadRecord<FLightSchedule>(Path);
			if (Read.HasError())
			{
				AddError(FString::Printf(TEXT("%s: %s"), *Stage, *Read.GetError()));
				continue;
			}
			TestEqual(FString::Printf(TEXT("%s's schedule names its stage"), *Stage), Read.GetValue().Stage, Stage);
			for (const FLightScheduleLuminaire& Entry : Read.GetValue().Luminaires)
			{
				const TOptional<FMuseeKey> Key = FMuseeKey::Parse(Entry.Type);
				if (!Key.IsSet() || !FindLuminaire(Key.GetValue()))
				{
					AddError(FString::Printf(TEXT("%s/%s: the luminaire type '%s' of '%s' isn't in the catalogue."), *Stage, LightingSchedule, *Entry.Type, *Entry.Id));
				}
				Types.Add(Entry.Type);
				++Named;
			}
		}
		TestTrue(TEXT("The schedules name luminaires"), Named > 0);
		AddInfo(FString::Printf(TEXT("%d luminaires of %d types read across %d stages."), Named, Types.Num(), Stages.Num()));
		return true;
	}
}

#endif
