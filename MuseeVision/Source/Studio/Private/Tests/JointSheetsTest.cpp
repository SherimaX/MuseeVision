#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/FileManager.h"
#include "Internationalization/Regex.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "MuseeRecords.h"
#include "SiteDescription.h"
#include "SitePlan.h"
#include "SiteRoom.h"

/**
 * The sheets say who draws a port's surround as the ports mark it. Where a stage marks a port's surround as its own
 * (FSitePort::bOwnSurround: the Rotunda's archivolts, the Salon's P5, the oval's P6), the joint draws none on that
 * stage's face, so no sheet may say the joint builds a surround on that stage's face, and the sheet of the stage across
 * the mating may not say the joint builds both surrounds. Musee.Studio.JointSheets.
 */
namespace Musee::Studio::Tests
{
	namespace
	{
		/** A sheet read whole, its lines run together and its runs of white space made one space, so prose across a line break reads. */
		FString SheetText(const FString& Stage)
		{
			const FString Path = FPaths::ConvertRelativePathToFull(Musee::Records::DataRoot() / TEXT("../Docs/Stages") / (Stage + TEXT(".md")));
			FString Text;
			if (!FFileHelper::LoadFileToString(Text, *Path))
			{
				return FString();
			}
			FString Out;
			Out.Reserve(Text.Len());
			bool bSpace = false;
			for (const TCHAR Char : Text)
			{
				if (FChar::IsWhitespace(Char))
				{
					bSpace = true;
					continue;
				}
				if (bSpace && !Out.IsEmpty())
				{
					Out.AppendChar(TEXT(' '));
				}
				bSpace = false;
				Out.AppendChar(Char);
			}
			return Out;
		}

		/** The first passage of a text a pattern matches, or empty. */
		FString FirstMatch(const FString& Text, const FString& Pattern)
		{
			FRegexMatcher Match(FRegexPattern(Pattern, ERegexPatternFlags::CaseInsensitive), Text);
			if (!Match.FindNext())
			{
				return FString();
			}
			const int32 From = FMath::Max(0, Match.GetMatchBeginning() - 60);
			return Text.Mid(From, Match.GetMatchEnding() + 60 - From);
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioJointSheetsTest, "Musee.Studio.JointSheets", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	bool FStudioJointSheetsTest::RunTest(const FString& Parameters)
	{
		TMap<FMuseeKey, FSitePort> Ports;
		TArray<FString> Stages;
		for (const USiteRoom* Room : USiteRoom::AllRooms())
		{
			FSiteDescription Description;
			Room->Describe(Description);
			Stages.Add(Description.Stage.ToString());
			for (const FSitePort& Port : Description.Ports)
			{
				Ports.Add(Port.Key, Port);
			}
		}
		const TValueOrError<FSitePlan, FString> Plan = Musee::Site::ReadPlan();
		if (!TestTrue(TEXT("the site plan reads"), Plan.HasValue()))
		{
			return false;
		}
		int32 Judged = 0;
		for (const FSiteMating& Mating : Plan.GetValue().Matings)
		{
			for (const auto& [Own, Other] : {TPair<FMuseeKey, FMuseeKey>(Mating.A, Mating.B), TPair<FMuseeKey, FMuseeKey>(Mating.B, Mating.A)})
			{
				const FSitePort* Port = Ports.Find(Own);
				if (!Port || !Port->bOwnSurround)
				{
					continue;
				}
				++Judged;
				const FString Stage = Musee::Site::StageOfPort(Own).ToString();
				const FString Name = Stage.Replace(TEXT("-"), TEXT(" "));
				// No sheet says the joint draws a surround on a face whose stage draws its own.
				for (const FString& Sheet : Stages)
				{
					const FString Said = FirstMatch(SheetText(Sheet), FString::Printf(TEXT("surrounds? on the %s['’]s inner face"), *Name));
					TestTrue(*FString::Printf(TEXT("%s.md doesn't put the joint's surround on %s's face, whose port %s marks its own (\"%s\")"), *Sheet, *Stage,
						*Own.ToString(), *Said), Said.IsEmpty());
				}
				// Nor does the sheet across the mating give the joint both surrounds.
				const FString Across = Musee::Site::StageOfPort(Other).ToString();
				const FString Said = FirstMatch(SheetText(Across), TEXT("both surrounds"));
				TestTrue(*FString::Printf(TEXT("%s.md doesn't give the joint both surrounds at %s, whose port %s marks its own (\"%s\")"), *Across,
					*Other.ToString(), *Own.ToString(), *Said), Said.IsEmpty());
			}
		}
		TestTrue(TEXT("some port draws its own surround"), Judged > 0);
		return true;
	}
}

#endif
