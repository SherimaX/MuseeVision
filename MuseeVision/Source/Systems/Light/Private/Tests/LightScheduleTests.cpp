#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/FileManager.h"
#include "Internationalization/Regex.h"
#include "LightLuminaire.h"
#include "Misc/FileHelper.h"
#include "MuseeRecords.h"

// The stages' lighting schedules against Light's catalogue: every luminaire type a room names is a real product Light
// holds a record of, so no room waits on a type that doesn't exist.
namespace Musee::Light::ScheduleTests
{
	inline constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** The stages' schedules, under Data. */
	constexpr const TCHAR* StagesFolder = TEXT("Stages");
	constexpr const TCHAR* LightingSchedule = TEXT("Lighting.json");

	/**
	 * A luminaire's type, found in the schedule's text: only a luminaire entry has a "type". The schedule is read as text
	 * so that its fields other systems or later rounds add never hide a type from this check.
	 */
	const TCHAR* const TypePattern = TEXT("\"type\"\\s*:\\s*\"([^\"]*)\"");

	TArray<FString> StageKeys()
	{
		TArray<FString> Stages;
		IFileManager::Get().FindFiles(Stages, *(Records::DataRoot() / StagesFolder / TEXT("*")), false, true);
		Stages.Sort();
		return Stages;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLightEveryScheduleResolves, "Musee.Light.EveryScheduleResolves", Flags)
	bool FLightEveryScheduleResolves::RunTest(const FString& Parameters)
	{
		const TArray<FString> Stages = StageKeys();
		TestTrue(TEXT("There are stages to check"), !Stages.IsEmpty());
		int32 Named = 0;
		TSet<FString> Types;
		for (const FString& Stage : Stages)
		{
			const FString Path = Records::DataRoot() / StagesFolder / Stage / LightingSchedule;
			FString Text;
			if (!FFileHelper::LoadFileToString(Text, *Path))
			{
				AddError(FString::Printf(TEXT("%s has no lighting schedule."), *Stage));
				continue;
			}
			FRegexMatcher Matcher(FRegexPattern(TypePattern), Text);
			while (Matcher.FindNext())
			{
				const FString Type = Matcher.GetCaptureGroup(1);
				const TOptional<FMuseeKey> Key = FMuseeKey::Parse(Type);
				if (!Key.IsSet() || !FindLuminaire(Key.GetValue()))
				{
					AddError(FString::Printf(TEXT("%s/%s: the luminaire type '%s' isn't in the catalogue."), *Stage, LightingSchedule, *Type));
				}
				Types.Add(Type);
				++Named;
			}
		}
		TestTrue(TEXT("The schedules name luminaires"), Named > 0);
		AddInfo(FString::Printf(TEXT("%d luminaires of %d types checked across %d stages."), Named, Types.Num(), Stages.Num()));
		return true;
	}
}

#endif
