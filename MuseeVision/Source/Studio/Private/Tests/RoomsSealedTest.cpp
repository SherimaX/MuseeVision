#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "FormMake.h"
#include "FormShells.h"
#include "MuseeCheck.h"
#include "SiteDescription.h"
#include "SiteRoom.h"

/**
 * Every loaded room's own drawings make the closed solids its enclosures claim (Form.Sealed), before any blind or
 * system adds to them: the room's shells proved from its description alone, as the stage contract's first steps make
 * them. A room module can't make its drawings itself (rooms never see FormMaking), so Studio proves them.
 */
namespace Musee::Studio::Tests
{
	namespace
	{
		constexpr EAutomationTestFlags SealedFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioRoomsSealedTest, "Musee.Studio.RoomsSealed", SealedFlags)
	bool FStudioRoomsSealedTest::RunTest(const FString& Parameters)
	{
		const FMuseeCheck* Sealed = FMuseeCheck::Find(TEXT("Form.Sealed"));
		if (!TestNotNull(TEXT("Form.Sealed is registered"), Sealed))
		{
			return false;
		}
		for (const USiteRoom* Room : USiteRoom::AllRooms())
		{
			FSiteDescription Description;
			Room->Describe(Description);
			const FString Stage = Description.Stage.ToString();
			for (FFormDrawing& Drawing : Description.Pieces)
			{
				Drawing.Drawer = FName(*Stage);
			}
			TArray<FFormMadePiece> Pieces;
			TArray<FMuseeFinding> Made;
			Musee::FormMaking::Make(Description.Pieces, Pieces, Made);
			for (const FMuseeFinding& Finding : Made)
			{
				AddInfo(FString::Printf(TEXT("%s: %s (%s)"), *Stage, *Finding.Summary, *Finding.Check.ToString()));
			}
			FFormShells Shells;
			Shells.Stage = Description.Stage;
			Shells.Enclosures = Description.Enclosures;
			for (const FFormMadePiece& Piece : Pieces)
			{
				Shells.Pieces.Add(Musee::FormMaking::MeasureShell(Piece));
			}
			FMuseeCheck::FInputs Inputs;
			Inputs.Add(Shells);
			const TValueOrError<TArray<FMuseeFinding>, FString> Found = Sealed->Evaluate(Inputs);
			if (!TestTrue(*FString::Printf(TEXT("%s's shells can be judged"), *Stage), Found.HasValue()))
			{
				AddError(Found.GetError());
				continue;
			}
			for (const FMuseeFinding& Finding : Found.GetValue())
			{
				AddError(FString::Printf(TEXT("%s: %s"), *Stage, *Finding.Summary));
			}
			TestEqual(*FString::Printf(TEXT("%s's enclosures are sealed"), *Stage), Found.GetValue().Num(), 0);
		}
		return true;
	}
}

#endif
