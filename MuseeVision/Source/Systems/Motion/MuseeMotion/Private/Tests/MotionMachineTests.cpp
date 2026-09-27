#include "Components/InstancedStaticMeshComponent.h"
#include "HAL/FileManager.h"
#include "MotionControl.h"
#include "MotionFaults.h"
#include "MotionLiftStage.h"
#include "MotionSchedule.h"
#include "MuseeRecords.h"

#if WITH_DEV_AUTOMATION_TESTS

// The machines of the stages beyond the lift: a group of screens, the Cube's timed piece and its derived light, the
// Rotunda's telescopic rams, and every stage's schedule read whole.

namespace Musee::Motion::Tests
{
	/** Three screens sharing an aisle, 3 m apart, each carrying a panel 2 m tall. */
	void DressScreens(FTestStage& Stage, TArray<FMuseeFinding>& OutFindings)
	{
		TArray<FMuseeStageAnchor> Anchors;
		const TCHAR* const Screens[] = { TEXT("a"), TEXT("b"), TEXT("c") };
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const FString Body = FString::Printf(TEXT("screens.%s"), Screens[Index]);
			const FVector At(3.0 * Index, 0.0, 0.0);
			Anchors.Add(Anchor(*Body, TEXT("Body"), At));
			Anchors.Add(Anchor(*(Body + TEXT(".pull")), TEXT("Use"), At + FVector(-1.0, 0.0, 1.0)));
			Stage.AddBody(LiftKey(*Body), At, FVector(0.0, 0.0, 1.0), FVector(0.02, 0.5, 1.0));
		}
		DressMechanisms(Stage.Context(LiftKey(TEXT("screens")), TEXT("Stages/screens"), MoveTemp(Anchors)), OutFindings);
	}

	/** The Cube's ceiling iris as bodies at its pins' bearings, and the performance control on the Cube's floor. */
	void DressCube(FTestStage& Stage, TArray<FMuseeFinding>& OutFindings)
	{
		TArray<FMuseeStageAnchor> Anchors = { Anchor(TEXT("cube.performance"), TEXT("Use"), FVector(0.0, 5.0, -22.0)) };
		for (int32 Blade = 0; Blade < 7; ++Blade)
		{
			const FString Name = FString::Printf(TEXT("cube.up.iris.blade.%d"), Blade + 1);
			const FVector Pin = FRotator(0.0, 360.0 * Blade / 7.0, 0.0).RotateVector(FVector(3.745, 0.0, -7.94));
			Anchors.Add(Anchor(*Name, TEXT("Body"), Pin));
			Stage.AddBody(LiftKey(*Name), Pin, FVector(-0.65, 0.0, -0.001), FVector(0.6, 0.3, 0.001));
		}
		DressMechanisms(Stage.ContextAt(LiftKey(TEXT("cube")), TEXT("Stages/cube"), MoveTemp(Anchors)), OutFindings);
	}

	/** The Rotunda's lifting drum and its balustrade's panels, with the three stages of each of its eight rams as made. */
	void DressRotunda(FTestStage& Stage, TArray<FMuseeFinding>& OutFindings)
	{
		TArray<FMuseeStageAnchor> Anchors = {
			Anchor(TEXT("rotunda.sun-clock.drum"), TEXT("Body"), FVector::ZeroVector),
			Anchor(TEXT("rotunda.stair.balustrade.panels"), TEXT("Body"), FVector(0.0, -3.0, -1.0)),
			Anchor(TEXT("rotunda.sun-clock.control"), TEXT("Use"), FVector(8.5, 0.0, 1.0)),
		};
		Stage.AddBody(LiftKey(TEXT("rotunda.sun-clock.drum")), FVector::ZeroVector, FVector(0.0, 0.0, -1.8), FVector(6.9, 6.9, 1.75));
		Stage.AddBody(LiftKey(TEXT("rotunda.stair.balustrade.panels")), FVector(0.0, -3.0, -1.0), FVector(0.0, 0.0, 0.5), FVector(1.0, 0.02, 0.5));
		for (int32 Ram = 1; Ram <= 8; ++Ram)
		{
			for (int32 Stage3 = 1; Stage3 <= 3; ++Stage3)
			{
				Stage.AddPiece(FString::Printf(TEXT("rotunda.sun-clock.ram.%d.stage-%d"), Ram, Stage3));
			}
		}
		DressMechanisms(Stage.ContextAt(LiftKey(TEXT("rotunda")), TEXT("Stages/rotunda"), MoveTemp(Anchors)), OutFindings);
	}

	UMotionControl* ControlAt(const FTestStage& Stage, const TCHAR* UseAnchor)
	{
		TArray<UMotionControl*> Controls;
		Stage.Actor->GetComponents(Controls);
		UMotionControl* const* Found = Controls.FindByPredicate([UseAnchor](const UMotionControl* C) { return C->UseAnchor() == LiftKey(UseAnchor); });
		return Found ? *Found : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionGroupTest, "Musee.Motion.Group", Musee::Motion::Tests::Flags)

bool FMotionGroupTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Motion;
	Tests::FTestStage Stage;
	TArray<FMuseeFinding> Findings;
	Tests::DressScreens(Stage, Findings);
	UMotionMechanism* A = Tests::Machine(Stage, TEXT("screens.a"));
	UMotionMechanism* B = Tests::Machine(Stage, TEXT("screens.b"));
	UMotionMechanism* C = Tests::Machine(Stage, TEXT("screens.c"));
	if (!TestTrue(TEXT("The three screens dress without findings"), Findings.IsEmpty() && A && B && C))
	{
		return false;
	}
	Tests::StandAll(Stage);
	TestTrue(TEXT("When the museum opens, screen B stands out where it starts, the others home"),
		B->State() == UMotionMechanism::Open && A->State() == UMotionMechanism::Closed && C->State() == UMotionMechanism::Closed);
	TestEqual(TEXT("A plain-string verb is the control's word"), A->Usage().Verb.ToString(), FString(TEXT("Draw out screen A")));

	double Clock = 0.0;
	A->Use();
	Tests::RunAll(Stage, 1.0, Clock);
	TestTrue(TEXT("Drawing A first sends B home, and A waits for it"), B->IsRunning() && A->WaitsFor() == UMotionMechanism::EWait::Group && A->Position() == 0.0);
	TestFalse(TEXT("A's control says why it waits"), A->Usage().WhyNot.IsEmpty());
	Tests::RunAll(Stage, 1.2, Clock);
	TestTrue(TEXT("Once B is home, A comes out"), B->State() == UMotionMechanism::Closed && A->Position() > 0.0);
	Tests::RunAll(Stage, 2.5, Clock);
	TestEqual(TEXT("A is out"), A->State(), UMotionMechanism::Open);
	C->Use();
	Tests::RunAll(Stage, 5.0, Clock);
	TestTrue(TEXT("Only ever one out: C out, A home"), C->State() == UMotionMechanism::Open && A->State() == UMotionMechanism::Closed
		&& B->State() == UMotionMechanism::Closed);

	FMotionSchedule Two;
	FMotionMechanismSpec Out = C->Spec();
	Out.Start = UMotionMechanism::Open;
	Two.Mechanisms = { Out, Out };
	Two.Mechanisms[1].Key = Tests::LiftKey(TEXT("screens.d"));
	TestEqual(TEXT("A group that starts with two out is a fault"), ScheduleFaults(Two).Num(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionPerformanceTest, "Musee.Motion.Performance", Musee::Motion::Tests::Flags)

bool FMotionPerformanceTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Motion;
	Tests::FTestStage Stage;
	TArray<FMuseeFinding> Findings;
	Tests::DressCube(Stage, Findings);
	for (const FMuseeFinding& Finding : Findings)
	{
		AddError(Finding.Summary);
	}
	UMotionControl* Control = Tests::ControlAt(Stage, TEXT("cube.performance"));
	UMotionPerformance* Piece = Control ? Control->Performance() : nullptr;
	if (!TestTrue(TEXT("The Cube dresses without findings, its piece at its control"), Findings.IsEmpty() && Piece))
	{
		return false;
	}
	Tests::StandAll(Stage);
	double Clock = 0.0;
	Tests::RunAll(Stage, Tests::LiftFrame, Clock);
	TestEqual(TEXT("At rest no work performs"), Tests::ChannelValue(Stage, TEXT("cube.performance.active")), 0.0f);
	TestEqual(TEXT("so the mast's pinspots, derived from it, are on"), Tests::ChannelValue(Stage, TEXT("cube.mast.light")), 1.0f);
	TestTrue(TEXT("With the Cube's port unmated the control offers nothing"), !Control->Usage().bAvailable && Control->Usage().Verb.IsEmpty());

	Piece->SetMatedPorts({ Tests::LiftKey(TEXT("cube.up")) });
	const FMuseeUse Unsealed = Control->Usage();
	TestTrue(TEXT("Mated but unsealed, it names the piece and waits"), !Unsealed.bAvailable && Unsealed.Verb.ToString() == TEXT("II · Sea Light, 8 min")
		&& !Unsealed.WhyNot.IsEmpty());
	Tests::PublishValue(Stage, TEXT("cube.up.closure"), 0.3206f);
	Tests::PublishValue(Stage, TEXT("atrium.car.light"), 0.03f);
	TestTrue(TEXT("Sealed (the car in the Cube, leaves shut, its light a glimmer, the iris clamped), it is offered"), Control->Usage().bAvailable);
	TestFalse(TEXT("its caption keyed under the piece"), Piece->Spec().Caption.Key.IsNone());
	Control->Use();
	Tests::RunAll(Stage, 1.0, Clock);
	TestEqual(TEXT("While it runs it publishes 1"), Tests::ChannelValue(Stage, TEXT("cube.performance.active")), 1.0f);
	TestEqual(TEXT("and the pinspots go out"), Tests::ChannelValue(Stage, TEXT("cube.mast.light")), 0.0f);
	TestEqual(TEXT("It offers its end"), Control->Usage().Verb.ToString(), FString(TEXT("End the piece")));
	Tests::RunAll(Stage, 478.0, Clock);
	TestTrue(TEXT("Near its end it still runs"), Piece->IsRunning());
	Tests::RunAll(Stage, 1.5, Clock);
	TestTrue(TEXT("After its 480 s it ends by itself"), !Piece->IsRunning() && Tests::ChannelValue(Stage, TEXT("cube.performance.active")) == 0.0f
		&& Tests::ChannelValue(Stage, TEXT("cube.mast.light")) == 1.0f);

	Control->Use();
	Tests::RunAll(Stage, 10.0, Clock);
	Control->Use();
	TestTrue(TEXT("Ended early it plays out its fade-out"), Piece->IsRunning() && !Control->Usage().bAvailable);
	Tests::RunAll(Stage, 3.1, Clock);
	TestTrue(TEXT("and then stops"), !Piece->IsRunning() && Tests::ChannelValue(Stage, TEXT("cube.performance.active")) == 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionTelescopeTest, "Musee.Motion.Telescope", Musee::Motion::Tests::Flags)

bool FMotionTelescopeTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Motion;
	Tests::FTestStage Stage;
	TArray<FMuseeFinding> Findings;
	Tests::DressRotunda(Stage, Findings);
	for (const FMuseeFinding& Finding : Findings)
	{
		AddError(Finding.Summary);
	}
	UMotionMechanism* Lift = Tests::Machine(Stage, TEXT("rotunda.sun-clock.lift"));
	TArray<UMotionExtent*> Rams;
	Stage.Actor->GetComponents(Rams);
	if (!TestTrue(TEXT("The Rotunda's lift and its eight rams dress without findings"), Findings.IsEmpty() && Lift && Rams.Num() == 8))
	{
		return false;
	}
	const FMotionExtentSpec& Ram = Rams[0]->Spec();
	TestEqual(TEXT("Closed, a ram is its barrel's 2.34 m"), Ram.ClosedLength(), 2.34, 1e-9);
	TestEqual(TEXT("Rising, the largest stage slides out first"), Ram.StageExtension(0, 3.0), 0.66, 1e-9);
	TestEqual(TEXT("while the others stay home in it"), Ram.StageExtension(1, 3.0) + Ram.StageExtension(2, 3.0), 0.0, 1e-9);
	TestTrue(TEXT("then the next, each fully out before the one after"), FMath::IsNearlyEqual(Ram.StageExtension(0, 4.0), 1.1667, 1e-9)
		&& FMath::IsNearlyEqual(Ram.StageExtension(1, 4.0), 4.0 - 2.34 - 1.1667, 1e-9) && Ram.StageExtension(2, 4.0) == 0.0);
	TestEqual(TEXT("Fully out, the plunger's head stands the drum's whole 3.5 m higher"), Ram.StageOffset(2, 5.84), 3.5, 1e-9);

	USceneComponent* Plunger = Stage.Actor->GetRootComponent();
	TArray<USceneComponent*> Parts;
	Stage.Actor->GetRootComponent()->GetChildrenComponents(true, Parts);
	for (USceneComponent* Part : Parts)
	{
		if (Part->ComponentHasTag(TEXT("musee.piece:rotunda.sun-clock.ram.1.stage-3")))
		{
			Plunger = Part;
		}
	}
	TestTrue(TEXT("Each stage rides a carriage of its own"), Plunger != Stage.Actor->GetRootComponent() && Plunger->GetAttachParent() != Stage.Actor->GetRootComponent());
	const double Home = Plunger->GetComponentLocation().Z;
	Tests::StandAll(Stage);
	double Clock = 0.0;
	Lift->Use();
	Tests::RunAll(Stage, 9.0, Clock);
	TestEqual(TEXT("The drum rises 3.5 m"), Lift->State(), UMotionMechanism::Open);
	TestEqual(TEXT("and every ram grows with it to 5.84 m"), Rams[4]->LengthMeters(), 5.84, 1e-4);
	TestEqual(TEXT("its plunger's head bearing on the drum's foot, 3.5 m up"), Musee::FromEngine<Musee::Meters>(Plunger->GetComponentLocation().Z - Home).Get(), 3.5, 1e-3);
	Lift->Use();
	Tests::RunAll(Stage, 9.0, Clock);
	TestEqual(TEXT("Lowered, the rams close again"), Musee::FromEngine<Musee::Meters>(Plunger->GetComponentLocation().Z - Home).Get(), 0.0, 1e-4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionColumnTest, "Musee.Motion.Column", Musee::Motion::Tests::Flags)

bool FMotionColumnTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Motion;
	Tests::FTestStage Stage;
	TArray<FMuseeFinding> Findings;
	Tests::DressAtrium(Stage, Findings);
	TArray<UInstancedStaticMeshComponent*> Columns;
	Stage.Actor->GetComponents(Columns);
	UMotionMechanism* Car = Tests::Machine(Stage, TEXT("atrium.car.lift"));
	if (!TestTrue(TEXT("The mast's turn becomes a column of instances on the mast's head"), Car && Columns.Num() == 1
		&& Columns[0]->GetAttachParent()->ComponentHasTag(TEXT("atrium.car.mast.head"))))
	{
		return false;
	}
	TestEqual(TEXT("At the Atrium the column has no turns"), Columns[0]->GetInstanceCount(), 0);
	TArray<UStaticMeshComponent*> Meshes;
	Stage.Actor->GetComponents(Meshes);
	const int32 Tagged = Meshes.FilterByPredicate([](const UStaticMeshComponent* M) { return M->ComponentHasTag(TEXT("musee.piece:atrium.cube-mast.column.turn")); }).Num();
	TestTrue(TEXT("and the drawn turn is gone, the column keeping its piece's tag"), Tagged == 1 && Columns[0]->ComponentHasTag(TEXT("musee.piece:atrium.cube-mast.column.turn")));
	Tests::StandAll(Stage);
	Car->SetMatedPorts({ Tests::LiftKey(TEXT("atrium.down")) });
	double Clock = 0.0;
	Car->UseAt(Tests::LiftKey(TEXT("atrium.call")));
	Tests::RunAll(Stage, 16.0, Clock);
	Car->UseAt(Tests::LiftKey(TEXT("atrium.car.panel")));
	Tests::RunAll(Stage, 38.0, Clock);
	TestEqual(TEXT("At the Cube, 12.775 m of 50 mm turns: 256, the last still in the crown"), Columns[0]->GetInstanceCount(), 256);
	FTransform Lowest;
	Columns[0]->GetInstanceTransform(255, Lowest, false);
	TestEqual(TEXT("each a pitch lower than the one above it"), Musee::FromEngine<Musee::Meters>(Lowest.GetLocation().Z).Get(), -255 * 0.05, 1e-6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionEveryScheduleTest, "Musee.Motion.EveryScheduleReads", Musee::Motion::Tests::Flags)

bool FMotionEveryScheduleTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Motion;
	TArray<FString> Stages;
	IFileManager::Get().FindFiles(Stages, *(Musee::Records::DataRoot() / TEXT("Stages") / TEXT("*")), false, true);
	int32 Read = 0;
	for (const FString& Stage : Stages)
	{
		const FString Path = FString(TEXT("Stages")) / Stage / FMotionSchedule::FileName;
		if (!IFileManager::Get().FileExists(*(Musee::Records::DataRoot() / Path)))
		{
			continue;
		}
		const TValueOrError<FMotionSchedule, FString> Schedule = FMotionSchedule::Read(Path);
		if (!TestTrue(FString::Printf(TEXT("%s's mechanisms read"), *Stage), Schedule.HasValue()))
		{
			AddError(Schedule.GetError());
			continue;
		}
		++Read;
		TArray<FString> Found = ScheduleFaults(Schedule.GetValue());
		for (const FMotionMechanismSpec& Spec : Schedule.GetValue().Mechanisms)
		{
			for (const FString& Fault : Faults(Spec))
			{
				Found.Add(FString::Printf(TEXT("Mechanism %s: %s"), *Spec.Key.ToString(), *Fault));
			}
		}
		for (const FString& Fault : Found)
		{
			AddError(FString::Printf(TEXT("%s: %s"), *Stage, *Fault));
		}
	}
	TestTrue(TEXT("Every stage that has machines has its schedule read"), Read >= 6);
	return true;
}

#endif
