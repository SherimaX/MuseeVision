#include "FormMake.h"
#include "Misc/AutomationTest.h"
#include "MotionPieces.h"
#include "MotionSchedule.h"
#include "MuseeUnits.h"

#if WITH_DEV_AUTOMATION_TESTS

// Motion's parts drawn with Form's drawing: the Rotunda's rams and the Élan masts' columns, from their stages' schedules.

namespace Musee::MotionMaking::Tests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** The schedule of a stage, as the museum keeps it. */
	TValueOrError<FMotionSchedule, FString> Schedule(const TCHAR* Stage)
	{
		return FMotionSchedule::Read(FString(TEXT("Stages")) / Stage / FMotionSchedule::FileName);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionPartsTest, "Musee.Motion.Parts", Musee::MotionMaking::Tests::Flags)

bool FMotionPartsTest::RunTest(const FString& Parameters)
{
	using namespace Musee::MotionMaking;
	const TValueOrError<FMotionSchedule, FString> Rotunda = Tests::Schedule(TEXT("rotunda"));
	const TValueOrError<FMotionSchedule, FString> Atrium = Tests::Schedule(TEXT("atrium"));
	if (!TestTrue(TEXT("The Rotunda's and the Atrium's schedules read"), Rotunda.HasValue() && Atrium.HasValue()))
	{
		return false;
	}
	const FMotionExtentSpec& Ram = Rotunda.GetValue().Extents[0];
	const TArray<FFormDrawing> Stages = DrawExtent(Ram);
	TestEqual(TEXT("A ram is drawn as its three stages"), Stages.Num(), 3);
	TArray<FFormMadePiece> Made;
	TArray<FMuseeFinding> Findings;
	Musee::FormMaking::Make(Stages, Made, Findings);
	TestTrue(TEXT("each made as a closed solid, with nothing found"), Made.Num() == 3 && Findings.IsEmpty());
	for (int32 Index = 0; Index < Made.Num(); ++Index)
	{
		const FFormMadePiece& Stage = Made[Index];
		TestEqual(TEXT("named for its ram and stage"), Stage.Name, Ram.StagePiece(Index));
		TestTrue(TEXT("finished as the ram, colliding, standing on the stage"), Stage.Surface == Ram.Surface && !Stage.Collision.IsNone() && Stage.Body.IsNone());
		const UE::Geometry::FAxisAlignedBox3d Box = Stage.Mesh->GetBounds();
		const double Top = Musee::FromEngine<Musee::Meters>(Box.Max.Z).Get() + Stage.Frame.OriginMeters.Z;
		TestEqual(TEXT("home, every stage's head at the lowered drum's foot"), Top, -3.56, 1e-3);
		TestEqual(TEXT("each within the one before"), Musee::FromEngine<Musee::Meters>(Box.Max.X).Get(), Ram.Telescope[Index].Radius, 2e-3);
	}

	const FMotionExtentSpec& Column = Atrium.GetValue().Extents[0];
	const TArray<FFormDrawing> Turn = DrawExtent(Column);
	TArray<FFormMadePiece> Turns;
	Musee::FormMaking::Make(Turn, Turns, Findings);
	TestTrue(TEXT("A column is drawn as one turn, on the body its end rides"), Turns.Num() == 1 && Findings.IsEmpty() && Turns[0].Body == Column.Body
		&& Turns[0].Name == Column.TurnPiece());
	if (Turns.Num() == 1)
	{
		const UE::Geometry::FAxisAlignedBox3d Box = Turns[0].Mesh->GetBounds();
		TestEqual(TEXT("one pitch deep"), Musee::FromEngine<Musee::Meters>(Box.Max.Z - Box.Min.Z).Get(), Column.Turns.Pitch, 1e-6);
		TestEqual(TEXT("the column's radius across"), Musee::FromEngine<Musee::Meters>(Box.Max.X).Get(), Column.Turns.Radius, 2e-3);
	}
	return true;
}

#endif
