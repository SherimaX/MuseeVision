#include "MotionControl.h"
#include "MotionDresser.h"
#include "MotionMechanism.h"
#include "MotionProfile.h"
#include "MotionSchedule.h"
#include "MotionTestStage.h"
#include "MuseeRecords.h"
#include "MuseeSignals.h"
#include "MuseeUsable.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::Motion::Tests
{
	/**
	 * The proof stage: the lift's basin at the origin, its iris on the block's top (a Bed first, where the iris grows,
	 * and the Use under the same key, as the oval's golden iris is), the door's leaf 5 m east.
	 */
	TArray<FMuseeStageAnchor> ProofAnchors()
	{
		return {
			Anchor(TEXT("proof.basin"), TEXT("Body"), FVector::ZeroVector),
			Anchor(TEXT("proof.iris"), TEXT("Bed"), FVector(0.3, 0.0, 0.9)),
			Anchor(TEXT("proof.iris"), TEXT("Use"), FVector(0.3, 0.0, 1.0)),
			Anchor(TEXT("proof.leaf"), TEXT("Body"), FVector(5.0, 0.0, 0.0)),
			Anchor(TEXT("proof.handle"), TEXT("Use"), FVector(8.0, 0.0, 1.0)),
			Anchor(TEXT("proof.rails"), TEXT("Body"), FVector(0.0, 3.0, -1.0)),
		};
	}

	/**
	 * Dresses the proof stage: the lift's basin carries a 1 m block, the door's leaf a leaf 1 m wide, 2 m tall and
	 * 50 mm thick, hung from its hinge at the body's origin.
	 */
	void Dress(FTestStage& Stage, TArray<FMuseeFinding>& OutFindings)
	{
		Stage.AddBody(FMuseeKey(TEXTVIEW("proof.basin")), FVector::ZeroVector);
		Stage.AddBody(FMuseeKey(TEXTVIEW("proof.leaf")), FVector(5.0, 0.0, 0.0), FVector(0.5, 0.0, 1.0), FVector(0.5, 0.025, 1.0));
		Stage.AddBody(FMuseeKey(TEXTVIEW("proof.rails")), FVector(0.0, 3.0, -1.0), FVector(0.0, 0.0, 0.5), FVector(1.0, 0.025, 0.5));
		DressMechanisms(Stage.Context(FMuseeKey(TEXTVIEW("proof")), TEXT("Stages/proof"), ProofAnchors()), OutFindings);
	}

	/** The stage's mechanism with the key. */
	UMotionMechanism* Find(const FTestStage& Stage, const TCHAR* Key)
	{
		TArray<UMotionMechanism*> Mechanisms;
		Stage.Actor->GetComponents(Mechanisms);
		UMotionMechanism* const* Found = Mechanisms.FindByPredicate([Key](const UMotionMechanism* M) { return M->Spec().Key == FMuseeKey(Key); });
		return Found ? *Found : nullptr;
	}

	/** Runs a mechanism in steps of a sixtieth of a second, as a frame does. */
	void Run(UMotionMechanism& Mechanism, double Seconds)
	{
		constexpr float Frame = 1.0f / 60.0f;
		for (double Done = 0.0; Done < Seconds; Done += Frame)
		{
			Mechanism.Advance(Frame);
		}
	}

	double HeightMeters(const UMotionMechanism& Mechanism)
	{
		return FromEngine<Meters>(Mechanism.Body()->GetRelativeLocation().Z).Get();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionProfileTest, "Musee.Motion.Profile", Musee::Motion::Tests::Flags)

bool FMotionProfileTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Motion;
	TestTrue(TEXT("Profiles are read by name"), ParseProfile(TEXT("ease-in-out")) == EProfile::EaseInOut && ParseProfile(TEXT("linear")) == EProfile::Linear);
	TestFalse(TEXT("An unknown profile is not read"), ParseProfile(TEXT("bounce")).IsSet());
	TestEqual(TEXT("A move starts where it starts"), Covered(EProfile::EaseInOut, 0.0), 0.0);
	TestEqual(TEXT("And ends where it ends"), Covered(EProfile::EaseInOut, 1.0), 1.0);
	TestEqual(TEXT("It is halfway at half time"), Covered(EProfile::EaseInOut, 0.5), 0.5, 1e-12);
	TestEqual(TEXT("It starts at rest"), RelativeSpeed(EProfile::EaseInOut, 0.0), 0.0);
	TestEqual(TEXT("It ends at rest"), RelativeSpeed(EProfile::EaseInOut, 1.0), 0.0);
	TestEqual(TEXT("Its peak is 1.875 times the average speed"), RelativeSpeed(EProfile::EaseInOut, 0.5), 1.875, 1e-12);
	const double Step = 1e-6;
	const double Slope = (Covered(EProfile::EaseInOut, 0.3 + Step) - Covered(EProfile::EaseInOut, 0.3 - Step)) / (2.0 * Step);
	TestEqual(TEXT("Its speed is its position's rate"), RelativeSpeed(EProfile::EaseInOut, 0.3), Slope, 1e-6);
	TestEqual(TEXT("A linear move keeps its speed"), RelativeSpeed(EProfile::Linear, 0.2), 1.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionScheduleTest, "Musee.Motion.Schedule", Musee::Motion::Tests::Flags)

bool FMotionScheduleTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Motion;
	const TValueOrError<FMotionSchedule, FString> Read = Musee::ReadRecord<FMotionSchedule>(Tests::Fixture(TEXT("Stages/proof")) / FMotionSchedule::FileName);
	if (!TestTrue(TEXT("The contract's Mechanisms.json reads"), Read.HasValue()))
	{
		AddError(Read.GetError());
		return false;
	}
	const FMotionSchedule& Schedule = Read.GetValue();
	TestEqual(TEXT("Every mechanism"), Schedule.Mechanisms.Num(), 3);
	TestTrue(TEXT("The rails follow the lift's height"), Schedule.Mechanisms[2].Follows.IsSet() && Schedule.Mechanisms[2].Follows.TravelAt(0.4231) == 1.0);
	const FMotionMechanismSpec& Lift = Schedule.Mechanisms[0];
	TestTrue(TEXT("Keys, joint and body"), Lift.Key == FMuseeKey(TEXTVIEW("proof.lift")) && Lift.Joint == TEXT("prismatic") && Lift.Body == FMuseeKey(TEXTVIEW("proof.basin")));
	TestEqual(TEXT("Travel in metres"), Lift.Travel, 2.6f);
	TestTrue(TEXT("Without stops, drawn and home are the travel's start"), Lift.DrawnPosition() == 0.0 && Lift.HomePosition() == 0.0 && !Lift.Drive.IsSet());
	TestEqual(TEXT("The open sequence's steps"), Lift.Open.Num(), 2);
	TestTrue(TEXT("A channel step, then a move"), !Lift.Open[0].IsMove() && Lift.Open[0].Channel == FMuseeKey(TEXTVIEW("proof.rim")) && Lift.Open[1].Move == UMotionMechanism::Open);
	TestEqual(TEXT("The optional words for opening"), Lift.OpenUse.Verb.Source, FString(TEXT("Raise")));
	TestEqual(TEXT("and for closing"), Lift.CloseUse.Caption.Source, FString(TEXT("Settle the block")));
	TestTrue(TEXT("Without them, a mechanism has none"), Schedule.Mechanisms[1].OpenUse.Verb.Key.IsNone() && Schedule.Mechanisms[1].CloseUse.Verb.Key.IsNone());
	TestTrue(TEXT("A sound spec has no faults"), Faults(Lift).IsEmpty() && Faults(Schedule.Mechanisms[1]).IsEmpty() && Faults(Schedule.Mechanisms[2]).IsEmpty());

	const TValueOrError<FMotionSchedule, FString> Faulty = Musee::ReadRecord<FMotionSchedule>(Tests::Fixture(TEXT("Stages/faulty")) / FMotionSchedule::FileName);
	if (TestTrue(TEXT("A faulty schedule still reads"), Faulty.HasValue()))
	{
		const TArray<FString> Found = Faults(Faulty.GetValue().Mechanisms[0]);
		TestEqual(TEXT("Joint, axis, travel, profile, guard and the step's two faults"), Found.Num(), 7);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionDresserTest, "Musee.Motion.Dresser", Musee::Motion::Tests::Flags)

bool FMotionDresserTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Motion;
	Tests::FTestStage Stage;
	TArray<FMuseeFinding> Findings;
	Tests::Dress(Stage, Findings);
	UMotionMechanism* Lift = Tests::Find(Stage, TEXT("proof.lift"));
	TestEqual(TEXT("The proof stage dresses without findings"), Findings.Num(), 0);
	if (!TestNotNull(TEXT("The lift is placed"), Lift))
	{
		return false;
	}
	TestTrue(TEXT("It drives its body"), Lift->Body() && Lift->Body()->ComponentHasTag(TEXT("proof.basin")));
	TestEqual(TEXT("The body is made movable"), Lift->Body()->Mobility.GetValue(), EComponentMobility::Movable);
	TArray<UMotionControl*> Controls;
	Stage.Actor->GetComponents(Controls);
	TestEqual(TEXT("Each mechanism worked by hand has its control"), Controls.Num(), 2);
	for (const UMotionControl* Control : Controls)
	{
		const bool bIris = Control->ComponentHasTag(TEXT("proof.iris"));
		TestTrue(TEXT("The iris rides on the lift; the door's handle, off the leaf, stays on the stage"),
			Control->GetAttachParent() == (bIris ? Lift->Body() : Stage.Actor->GetRootComponent()));
		TestTrue(TEXT("At its anchor"), Control->GetComponentLocation().Equals(Musee::ToEngine(bIris ? FVector(0.3, 0.0, 1.0) : FVector(8.0, 0.0, 1.0)), 0.01));
	}

	Tests::FTestStage FaultyStage;
	FaultyStage.AddBody(FMuseeKey(TEXTVIEW("faulty.basin")), FVector::ZeroVector);
	TArray<FMuseeFinding> Faulty;
	DressMechanisms(FaultyStage.Context(FMuseeKey(TEXTVIEW("faulty")), TEXT("Stages/faulty"), {}), Faulty);
	TestEqual(TEXT("A faulty spec and a missing body are each a finding"), Faulty.Num(), 2);
	TArray<UMotionMechanism*> Placed;
	FaultyStage.Actor->GetComponents(Placed);
	TestEqual(TEXT("And neither is placed"), Placed.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionControlSightTest, "Musee.Motion.ControlSight", Musee::Motion::Tests::Flags)

bool FMotionControlSightTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Motion;
	Tests::FTestStage Stage;
	TArray<FMuseeFinding> Findings;
	Tests::Dress(Stage, Findings);
	// A gaze across the iris, 50 mm above the block's top and clear of it, as the visitor's Look traces it.
	const FVector From = Musee::ToEngine(FVector(0.3, -2.0, 1.05));
	const FVector To = Musee::ToEngine(FVector(0.3, 2.0, 1.05));
	const auto Trace = [&](ECollisionChannel Channel)
	{
		FHitResult Hit;
		Stage.World->LineTraceSingleByChannel(Hit, From, To, Channel, FCollisionQueryParams(SCENE_QUERY_STAT(MuseeMotionTest), true));
		return Hit;
	};
	const FHitResult Seen = Trace(ECC_Visibility);
	const UMotionControl* Control = Cast<UMotionControl>(Seen.GetComponent());
	if (!TestNotNull(TEXT("The Visibility trace finds the control's usable volume"), Control))
	{
		return false;
	}
	TestTrue(TEXT("the iris's"), Control->ComponentHasTag(TEXT("proof.iris")));
	TestTrue(TEXT("which the visitor can use"), Seen.GetComponent()->GetClass()->ImplementsInterface(UMuseeUsable::StaticClass()));
	TestTrue(TEXT("and which offers its mechanism's use"), Control->Usage().bAvailable && Control->Usage().Verb.ToString() == TEXT("Raise"));
	TestFalse(TEXT("It stops no camera"), Trace(ECC_Camera).bBlockingHit);
	TestFalse(TEXT("nor the visitor's body"), Trace(Musee::Collision::Body.Channel).bBlockingHit);
	TestFalse(TEXT("nor anything solid"), Trace(Musee::Collision::Solid.Channel).bBlockingHit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionSequenceTest, "Musee.Motion.Sequence", Musee::Motion::Tests::Flags)

bool FMotionSequenceTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Motion;
	Tests::FTestStage Stage;
	TArray<FMuseeFinding> Findings;
	Tests::Dress(Stage, Findings);
	UMotionMechanism* Lift = Tests::Find(Stage, TEXT("proof.lift"));
	if (!TestNotNull(TEXT("The lift is placed"), Lift))
	{
		return false;
	}
	UMuseeSignals* Signals = Stage.World->GetSubsystem<UMuseeSignals>();
	int32 Actuations = 0;
	double PeakSpeed = 0.0;
	Signals->OnActuation().AddLambda([&](const FMuseeActuation& A) { ++Actuations; PeakSpeed = FMath::Max(PeakSpeed, double(A.SpeedMetersPerSecond)); });
	TArray<FName> Reached;
	Lift->OnMoved().AddLambda([&Reached](const FMuseeKey&, FName State) { Reached.Add(State); });

	TestEqual(TEXT("It rests closed"), Lift->State(), UMotionMechanism::Closed);
	TestEqual(TEXT("Its control offers the spec's verb"), Lift->Usage().Verb.ToString(), FString(TEXT("Raise")));
	Lift->Use();
	Tests::Run(*Lift, 1.0);
	TestTrue(TEXT("The rim runs first while the body waits"), Signals->Read(FMuseeKey(TEXTVIEW("proof.rim"))).Get(0.0f) > 0.6f && Lift->Position() == 0.0);
	TestFalse(TEXT("It can't be used while it runs"), Lift->Usage().bAvailable);
	Tests::Run(*Lift, 6.0);
	TestEqual(TEXT("The rim is lit"), Signals->Read(FMuseeKey(TEXTVIEW("proof.rim"))).Get(0.0f), 1.0f);
	TestEqual(TEXT("It is open"), Lift->State(), UMotionMechanism::Open);
	TestEqual(TEXT("A full travel up"), Tests::HeightMeters(*Lift), 2.6, 1e-4);
	TestTrue(TEXT("Its actuation was published, peaking at 1.875 times 2.6 m in 5 s"), Actuations > 0 && FMath::IsNearlyEqual(PeakSpeed, 1.875 * 2.6 / 5.0, 0.01));
	TestEqual(TEXT("From open it offers the spec's closing verb"), Lift->Usage().Verb.ToString(), FString(TEXT("Settle")));
	TestEqual(TEXT("It publishes its position"), Signals->Read(FMuseeKey(TEXTVIEW("proof.height"))).Get(0.0f), 1.0f);
	UMotionMechanism* Rails = Tests::Find(Stage, TEXT("proof.rails"));
	if (TestNotNull(TEXT("The rails are placed"), Rails))
	{
		Rails->Advance(1.0f / 60.0f);
		TestEqual(TEXT("The rails follow the lift up to their full travel"), Tests::HeightMeters(*Rails), -1.0 + 1.0, 1e-4);
		TestFalse(TEXT("A following mechanism isn't used by hand"), Rails->Usage().bAvailable);
	}

	Lift->Use();
	Tests::Run(*Lift, 7.0);
	TestEqual(TEXT("It closes"), Lift->State(), UMotionMechanism::Closed);
	TestEqual(TEXT("Back where it was made"), Tests::HeightMeters(*Lift), 0.0, 1e-4);
	TestEqual(TEXT("And the rim goes out after"), Signals->Read(FMuseeKey(TEXTVIEW("proof.rim"))).Get(1.0f), 0.0f);
	TestTrue(TEXT("It reached open, then closed"), Reached == TArray<FName>({ UMotionMechanism::Open, UMotionMechanism::Closed }));
	TestFalse(TEXT("Nothing runs now"), Lift->IsRunning());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionGuardTest, "Musee.Motion.Guard", Musee::Motion::Tests::Flags)

bool FMotionGuardTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Motion;
	Tests::FTestStage Stage;
	TArray<FMuseeFinding> Findings;
	Tests::Dress(Stage, Findings);
	UMotionMechanism* Lift = Tests::Find(Stage, TEXT("proof.lift"));
	UMotionMechanism* Door = Tests::Find(Stage, TEXT("proof.door"));
	if (!TestNotNull(TEXT("The lift is placed"), Lift) || !TestNotNull(TEXT("The door is placed"), Door))
	{
		return false;
	}
	Lift->MoveTo(UMotionMechanism::Open);
	Tests::Run(*Lift, 6.0);
	TestEqual(TEXT("The lift is up"), Lift->State(), UMotionMechanism::Open);

	// A visitor 1.8 m tall stands under the lifted block: the lift lowers onto the top of the head, less the clearance.
	UCapsuleComponent* Visitor = Stage.AddVisitor(FVector::ZeroVector);
	Lift->MoveTo(UMotionMechanism::Closed);
	Tests::Run(*Lift, 8.0);
	TestTrue(TEXT("The guard holds the lift"), Lift->IsHeld() && Lift->IsRunning());
	TestTrue(TEXT("Clear of the head by the clearance"), Tests::HeightMeters(*Lift) >= 1.8 + 0.05 - 0.02 && Tests::HeightMeters(*Lift) < 1.9);
	TestFalse(TEXT("Its control says why it can't be used"), Lift->Usage().bAvailable || Lift->Usage().WhyNot.IsEmpty());

	Visitor->SetWorldLocation(Musee::ToEngine(FVector(20.0, 0.0, 0.9)));
	Tests::Run(*Lift, 6.0);
	TestEqual(TEXT("Once clear, it resumes and closes"), Lift->State(), UMotionMechanism::Closed);

	// The door's leaf swings through a visitor standing in its arc, 45 degrees round: a door turns back.
	Door->MoveTo(UMotionMechanism::Open);
	Tests::Run(*Door, 4.0);
	TestEqual(TEXT("The door opens a quarter turn"), Door->State(), UMotionMechanism::Open);
	Visitor->SetWorldLocation(Musee::ToEngine(FVector(5.5, 0.5, 0.9)));
	Door->Use();
	Tests::Run(*Door, 5.0);
	TestEqual(TEXT("Closing into the visitor, it goes back to open"), Door->State(), UMotionMechanism::Open);
	return true;
}

#endif
