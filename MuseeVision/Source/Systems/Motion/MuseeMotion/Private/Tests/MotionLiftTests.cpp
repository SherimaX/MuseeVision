#include "MotionControl.h"
#include "MotionDrive.h"
#include "MotionLiftStage.h"
#include "MotionSCurve.h"
#include "MotionSchedule.h"
#include "MuseeRecords.h"

#if WITH_DEV_AUTOMATION_TESTS

// The many-stop lift, tested on the Atrium's Élan car as its sheet specifies it (Docs/Stages/atrium.md, section 6):
// 44.0 m of travel, five stops, 1.5 m/s and 0.75 m/s2 on the rails, 1.0 m/s and 0.5 m/s2 on the Cube mast, and a jerk
// of 1.0 m/s3 everywhere.

namespace Musee::Motion::Tests
{
	/** The rails' limits and the jerk limit, and the Cube mast's zone, as the Atrium's drive gives them. */
	FMotionDrive AtriumDrive()
	{
		FMotionDrive Drive;
		Drive.Speed = 1.5;
		Drive.Accel = 0.75;
		Drive.Jerk = 1.0;
		FMotionDriveZone& Mast = Drive.Zones.AddDefaulted_GetRef();
		Mast.From = 0.0;
		Mast.To = 0.290341;
		Mast.Speed = 1.0;
		Mast.Accel = 0.5;
		return Drive;
	}

	/** Whether a run keeps its limits and runs from rest to rest, sampled every millisecond. */
	bool KeepsLimits(const FSCurve& Curve, double Speed, double Accel, double Jerk)
	{
		constexpr double Step = 1.0e-3;
		constexpr double Slack = 1.0e-9;
		double Travelled = 0.0;
		double Before = 0.0;
		for (double T = Step; T <= Curve.Duration(); T += Step)
		{
			const FSCurve::FSample Now = Curve.At(T);
			const bool bWithin = Now.Speed <= Speed + Slack && FMath::Abs(Now.Accel) <= Accel + Slack
				&& FMath::Abs(Now.Accel - Before) <= Jerk * Step + Slack && Now.Covered >= Travelled - Slack;
			if (!bWithin)
			{
				return false;
			}
			Travelled = Now.Covered;
			Before = Now.Accel;
		}
		const FSCurve::FSample End = Curve.At(Curve.Duration());
		return Curve.At(0.0).Speed == 0.0 && End.Speed == 0.0 && End.Covered == Curve.Distance();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionSCurveTest, "Musee.Motion.SCurve", Musee::Motion::Tests::Flags)

bool FMotionSCurveTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Motion;
	// The car from the neck to the Atrium, 8.4 m: 2.75 s up to 1.5 m/s (0.75 s of jerk, 1.25 s held, 0.75 s of jerk)
	// over 2.0625 m, 2.85 s at speed, and the mirror: 8.35 s, where the as-built drive without a jerk limit took 7.6 s.
	const FSCurve Leg = FSCurve::RestToRest(8.4, 1.5, 0.75, 1.0);
	TestEqual(TEXT("A long run takes its jerk, its hold and its cruise"), Leg.Duration(), 8.35, 1e-9);
	TestEqual(TEXT("and cruises at the speed limit"), Leg.PeakSpeed(), 1.5, 1e-12);
	TestTrue(TEXT("never passing its speed, acceleration or jerk"), Tests::KeepsLimits(Leg, 1.5, 0.75, 1.0));
	TestEqual(TEXT("It is halfway at half time"), Leg.At(0.5 * Leg.Duration()).Covered, 4.2, 1e-9);

	const FSCurve Short = FSCurve::RestToRest(3.0, 1.5, 0.75, 1.0);
	TestTrue(TEXT("A run too short to cruise peaks below the speed limit, at half time"),
		Short.PeakSpeed() < 1.5 && FMath::IsNearlyEqual(Short.At(0.5 * Short.Duration()).Speed, Short.PeakSpeed(), 1e-9));
	TestTrue(TEXT("and keeps its limits"), Tests::KeepsLimits(Short, 1.5, 0.75, 1.0));

	// Half a metre: the acceleration never reaches its limit, peaking at sqrt(v j) with v = (D sqrt(j) / 2)^(2/3).
	const FSCurve Nudge = FSCurve::RestToRest(0.5, 1.5, 0.75, 1.0);
	TestEqual(TEXT("A run too short to hold its acceleration peaks lower still"), Nudge.PeakSpeed(), FMath::Pow(0.25, 2.0 / 3.0), 1e-9);
	TestTrue(TEXT("and keeps its limits"), Tests::KeepsLimits(Nudge, 1.5, 0.75, 1.0));

	const FSCurve Still = FSCurve::RestToRest(0.0, 1.5, 0.75, 1.0);
	TestTrue(TEXT("A run of no distance takes no time"), Still.Duration() == 0.0 && Still.At(1.0).Covered == 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionDriveTest, "Musee.Motion.Drive", Musee::Motion::Tests::Flags)

bool FMotionDriveTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Motion;
	const FMotionDrive Drive = Tests::AtriumDrive();
	constexpr double Travel = 44.0;
	// Clamp to Cube on the mast, 12.775 m: 2.5 s up to 1.0 m/s over 1.25 m, 10.275 s at speed, and the mirror.
	const FSCurve Mast = Drive.Run(0.290341, 0.0, Travel);
	TestEqual(TEXT("On the mast the zone's limits hold"), Mast.Duration(), 15.275, 1e-3);
	TestEqual(TEXT("its speed"), Mast.PeakSpeed(), 1.0, 1e-9);
	TestEqual(TEXT("On the rails the drive's own do"), Drive.Run(0.727273, 0.536364, Travel).Duration(), 8.35, 1e-4);
	TestEqual(TEXT("A move that only touches the zone's edge runs on the rails"), Drive.Run(0.536364, 0.290341, Travel).PeakSpeed(), 1.5, 1e-9);
	const FSCurve Through = Drive.Run(0.536364, 0.0, Travel);
	TestTrue(TEXT("A move through the zone runs under its limits all the way"), FMath::IsNearlyEqual(Through.PeakSpeed(), 1.0, 1e-9)
		&& Tests::KeepsLimits(Through, 1.0, 0.5, 1.0));
	TestEqual(TEXT("up or down alike"), Drive.Run(0.0, 0.536364, Travel).Duration(), Through.Duration(), 1e-12);
	TestEqual(TEXT("A move to where it is takes no time"), Drive.Run(0.5, 0.5, Travel).Duration(), 0.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionLiftScheduleTest, "Musee.Motion.LiftSchedule", Musee::Motion::Tests::Flags)

bool FMotionLiftScheduleTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Motion;
	const TValueOrError<FMotionSchedule, FString> Read = Musee::ReadRecord<FMotionSchedule>(Tests::Fixture(TEXT("Stages/atrium")) / FMotionSchedule::FileName);
	if (!TestTrue(TEXT("The Atrium's schedule in the contract's many-stop shape reads"), Read.HasValue()))
	{
		AddError(Read.GetError());
		return false;
	}
	const TArray<FMotionMechanismSpec>& Mechanisms = Read.GetValue().Mechanisms;
	TestEqual(TEXT("Its thirteen mechanisms"), Mechanisms.Num(), 13);
	for (const FMotionMechanismSpec& Spec : Mechanisms)
	{
		const TArray<FString> Found = Faults(Spec);
		TestTrue(FString::Printf(TEXT("%s is sound: %s"), *Spec.Key.ToString(), *FString::Join(Found, TEXT("; "))), Found.IsEmpty());
	}
	const FMotionMechanismSpec& Car = Mechanisms[0];
	TestEqual(TEXT("The car's five stops"), Car.Stops.Num(), 5);
	TestEqual(TEXT("A stop is where its share of the travel says"), Car.PositionOf(TEXT("clamp")).Get(-1.0), 0.290341);
	TestEqual(TEXT("It is drawn at the Atrium"), Car.DrawnPosition(), 0.536364);
	TestEqual(TEXT("and starts in the neck"), Car.HomePosition(), 0.727273);
	TestTrue(TEXT("It gives a drive with the mast's zone"), Car.Drive.IsSet() && Car.Drive.Zones.Num() == 1 && Car.Drive.Jerk == 1.0);
	TestEqual(TEXT("Its six rides"), Car.Rides.Num(), 6);
	TestTrue(TEXT("Down to the Cube needs the well below mated"), Car.Rides[0].Requires.Mated == TArray<FMuseeKey>({ Tests::LiftKey(TEXT("atrium.down")) }));
	const FMotionChannelCondition& NoWork = Car.Rides[2].Requires.Channels[0];
	TestTrue(TEXT("Up from the Cube waits for no work performing"), NoWork.Holds(0.0f) && !NoWork.Holds(1.0f));
	TestTrue(TEXT("Down from the Sphere waits for the leaves"), !Car.Rides[4].Requires.Channels[0].Holds(0.5f) && Car.Rides[4].Requires.Channels[0].Holds(1.0f));

	const TValueOrError<FMotionSchedule, FString> Faulty = Musee::ReadRecord<FMotionSchedule>(Tests::Fixture(TEXT("Stages/faulty-lift")) / FMotionSchedule::FileName);
	if (TestTrue(TEXT("A faulty lift still reads"), Faulty.HasValue()))
	{
		const TArray<FString> Found = Faults(Faulty.GetValue().Mechanisms[0]);
		// A drive and a time; a zone run backwards; a stop named open; a stop named twice; a stop past the travel; an
		// unknown home; a move to an unknown state; a ride at an unknown stop; a ride's bounds the wrong way round.
		TestEqual(TEXT("Each broken rule is one fault"), Found.Num(), 9);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionLiftDresserTest, "Musee.Motion.LiftDresser", Musee::Motion::Tests::Flags)

bool FMotionLiftDresserTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Motion;
	Tests::FTestStage Stage;
	TArray<FMuseeFinding> Findings;
	Tests::DressAtrium(Stage, Findings);
	TestEqual(TEXT("The Atrium's lift dresses without findings, the Sphere's call across the seam included"), Findings.Num(), 0);
	TestEqual(TEXT("Every mechanism is placed"), Tests::Machines(Stage).Num(), 13);
	UMotionMechanism* Car = Tests::Machine(Stage, TEXT("atrium.car.lift"));
	if (!TestNotNull(TEXT("The car is placed"), Car))
	{
		return false;
	}
	TestTrue(TEXT("Made, it stands where it is drawn: at the Atrium"), Car->State() == TEXT("atrium") && Tests::LevelMeters(*Car) == 0.0);
	TArray<UMotionControl*> Controls;
	Stage.Actor->GetComponents(Controls);
	TestEqual(TEXT("A control at the call and one at the panel; the Sphere's call is the Sphere's"), Controls.Num(), 2);
	for (const UMotionControl* Control : Controls)
	{
		const bool bPanel = Control->UseAnchor() == Tests::LiftKey(TEXT("atrium.car.panel"));
		TestTrue(TEXT("The panel rides in the car; the call stays on the floor"),
			Control->GetAttachParent() == (bPanel ? Car->Body() : Stage.Actor->GetRootComponent()));
	}
	Tests::StandAll(Stage);
	TestEqual(TEXT("When the museum opens the car is at home, in the neck"), Car->State(), FName(TEXT("neck")));
	TestEqual(TEXT("8.4 m above the Atrium"), Tests::LevelMeters(*Car), 8.4, 1e-4);
	TestEqual(TEXT("It publishes its height"), Tests::ChannelValue(Stage, TEXT("atrium.car.height")), 0.727273f, 1e-6f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionLiftTest, "Musee.Motion.Lift", Musee::Motion::Tests::Flags)

bool FMotionLiftTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Motion;
	Tests::FTestStage Stage;
	TArray<FMuseeFinding> Findings;
	Tests::DressAtrium(Stage, Findings);
	UMotionMechanism* Car = Tests::Machine(Stage, TEXT("atrium.car.lift"));
	UMotionMechanism* Mast = Tests::Machine(Stage, TEXT("atrium.cube-mast"));
	UMotionMechanism* CarLeaf = Tests::Machine(Stage, TEXT("atrium.car.leaf.north"));
	UMotionMechanism* CarLeafSouth = Tests::Machine(Stage, TEXT("atrium.car.leaf.south"));
	UMotionMechanism* LandingLeaf = Tests::Machine(Stage, TEXT("atrium.landing.leaf.north"));
	UMotionMechanism* Blade = Tests::Machine(Stage, TEXT("atrium.up.iris.blade.4"));
	if (!TestTrue(TEXT("The car, its mast, its leaves and the collar iris are placed"), Car && Mast && CarLeaf && CarLeafSouth && LandingLeaf && Blade))
	{
		return false;
	}
	Tests::StandAll(Stage);
	const FMuseeKey Call = Tests::LiftKey(TEXT("atrium.call"));
	const FMuseeKey Panel = Tests::LiftKey(TEXT("atrium.car.panel"));
	TestTrue(TEXT("The call offers the car"), Car->UsageAt(Call).bAvailable && Car->UsageAt(Call).Verb.ToString() == TEXT("Call the car"));

	double Clock = 0.0;
	double Arrived = -1.0;
	Car->OnMoved().AddLambda([&Clock, &Arrived](const FMuseeKey&, FName State) { if (State == TEXT("atrium") && Arrived < 0.0) { Arrived = Clock; } });
	Car->UseAt(Call);
	Tests::RunAll(Stage, 4.0 + Tests::LiftFrame, Clock);
	TestEqual(TEXT("The collar iris opens first: each blade turns 52 degrees on its pin"), -Tests::TurnDegrees(*Blade), 52.0, 0.01);
	TestEqual(TEXT("while the car waits in the neck"), Tests::LevelMeters(*Car), 8.4, 1e-3);
	Tests::RunAll(Stage, 8.35 + 3.0 + 0.1, Clock);
	TestEqual(TEXT("The car lands at the Atrium"), Car->State(), FName(TEXT("atrium")));
	TestEqual(TEXT("level with the floor it is drawn at"), Tests::LevelMeters(*Car), 0.0, 1e-4);
	TestEqual(TEXT("Its run down from the neck is one S-curve of 8.35 s"), Arrived - 4.0, 8.35, 2.0 * Tests::LiftFrame);
	TestEqual(TEXT("Then the car's north leaf turns 24 degrees into its pocket"), Tests::TurnDegrees(*CarLeaf), 24.0, 0.01);
	TestEqual(TEXT("the south leaf the other way"), Tests::TurnDegrees(*CarLeafSouth), -24.0, 0.01);
	TestEqual(TEXT("and the landing's leaves follow theirs"), Tests::TurnDegrees(*LandingLeaf), 24.0, 0.01);
	TestEqual(TEXT("From here the call sends the car home"), Car->UsageAt(Call).Verb.ToString(), FString(TEXT("Send the car home")));

	Car->SetMatedPorts({ Tests::LiftKey(TEXT("atrium.down")), Tests::LiftKey(TEXT("atrium.up")) });
	TestEqual(TEXT("With both wells mated the panel's first ride at the Atrium is down"), Car->UsageAt(Panel).Verb.ToString(), FString(TEXT("Down to the Cube")));
	double RailPeak = 0.0;
	double MastPeak = 0.0;
	Stage.World->GetSubsystem<UMuseeSignals>()->OnActuation().AddLambda([&](const FMuseeActuation& Actuation)
	{
		if (Actuation.Mechanism == Car->Spec().Key)
		{
			double& Peak = Car->Position() < 0.290341 ? MastPeak : RailPeak;
			Peak = FMath::Max(Peak, double(Actuation.SpeedMetersPerSecond));
		}
	});
	Car->UseAt(Panel);
	Tests::RunAll(Stage, 38.0, Clock);
	TestEqual(TEXT("The ride ends in the Cube"), Car->State(), FName(TEXT("cube")));
	TestEqual(TEXT("23.6 m below the Atrium"), Tests::LevelMeters(*Car), -23.6, 1e-4);
	TestEqual(TEXT("On the rails the car cruises at 1.5 m/s"), RailPeak, 1.5, 1e-3);
	TestTrue(TEXT("On the Cube mast it never passes 1.0 m/s"), MastPeak > 0.99 && MastPeak <= 1.0 + 1e-6);
	TestEqual(TEXT("The Cube's iris clamps the mast's head"), Tests::ChannelValue(Stage, TEXT("cube.up.closure")), 0.378f, 1e-6f);
	TestEqual(TEXT("The car's light falls to a glimmer"), Tests::ChannelValue(Stage, TEXT("atrium.car.light")), 0.03f, 1e-6f);
	TestEqual(TEXT("The mast's head stays in the Cube's iris while the car sinks: the crown feeds out 12.775 m"),
		Musee::FromEngine<Musee::Meters>(Mast->Body()->GetComponentLocation().Z).Get(), -10.825 + 2.78, 1e-3);
	TestEqual(TEXT("The car's leaves ride down with it"), CarLeaf->Body()->GetComponentLocation().Z, Car->Body()->GetComponentLocation().Z, 0.01);

	TestTrue(TEXT("At the Cube the panel offers the way up"), Car->UsageAt(Panel).bAvailable && Car->UsageAt(Panel).Verb.ToString() == TEXT("Up to the Atrium"));
	Tests::PublishValue(Stage, TEXT("cube.performance.active"), 1.0f);
	const FMuseeUse Waiting = Car->UsageAt(Panel);
	TestTrue(TEXT("While a work performs it waits, and says why"), !Waiting.bAvailable && Waiting.Verb.ToString() == TEXT("Up to the Atrium") && !Waiting.WhyNot.IsEmpty());
	Tests::PublishValue(Stage, TEXT("cube.performance.active"), 0.0f);
	Car->UseAt(Panel);
	Tests::RunAll(Stage, 38.0, Clock);
	TestEqual(TEXT("Back at the Atrium"), Car->State(), FName(TEXT("atrium")));
	TestEqual(TEXT("the Cube's iris shut behind it"), Tests::ChannelValue(Stage, TEXT("cube.up.closure")), 0.0f);
	TestEqual(TEXT("the mast's head back on the crown"), Tests::LevelMeters(*Mast), 2.78, 1e-4);

	Car->UseAt(Call);
	Tests::RunAll(Stage, 1.5 + 1.5 + 8.35 + 4.0 + 0.5, Clock);
	TestEqual(TEXT("Sent home, the car is in the neck"), Car->State(), FName(TEXT("neck")));
	TestEqual(TEXT("the collar iris shut under it"), Tests::TurnDegrees(*Blade), 0.0, 0.01);
	TestEqual(TEXT("and its leaves shut"), Tests::TurnDegrees(*CarLeaf), 0.0, 0.01);
	TestFalse(TEXT("Nothing runs now"), Car->IsRunning());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMotionRidesTest, "Musee.Motion.Rides", Musee::Motion::Tests::Flags)

bool FMotionRidesTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Motion;
	Tests::FTestStage Stage;
	TArray<FMuseeFinding> Findings;
	Tests::DressAtrium(Stage, Findings);
	UMotionMechanism* Car = Tests::Machine(Stage, TEXT("atrium.car.lift"));
	if (!TestNotNull(TEXT("The car is placed"), Car))
	{
		return false;
	}
	Tests::StandAll(Stage);
	const FMuseeKey Panel = Tests::LiftKey(TEXT("atrium.car.panel"));
	const FMuseeUse Unmated = Car->UsageAt(Panel);
	TestTrue(TEXT("With no well mated the panel offers nothing"), !Unmated.bAvailable && Unmated.Verb.IsEmpty());

	Car->SetMatedPorts({ Tests::LiftKey(TEXT("atrium.up")) });
	const FMuseeUse Elsewhere = Car->UsageAt(Panel);
	TestTrue(TEXT("With the well above mated, the car parked in the neck still offers no ride"), !Elsewhere.bAvailable && Elsewhere.Verb.IsEmpty());
	TestFalse(TEXT("A requirement naming an unmated port doesn't hold"), Car->Holds(Car->Spec().Rides[0].Requires));

	double Clock = 0.0;
	Car->UseAt(Tests::LiftKey(TEXT("atrium.call")));
	Tests::RunAll(Stage, 16.0, Clock);
	const FMuseeUse Up = Car->UsageAt(Panel);
	TestTrue(TEXT("At the Atrium the panel offers the Sphere"), Up.bAvailable && Up.Verb.ToString() == TEXT("Up to the Sphere"));
	Car->UseAt(Panel);
	Tests::RunAll(Stage, 10.0, Clock);
	const FMuseeUse Moving = Car->UsageAt(Panel);
	TestTrue(TEXT("While it runs nothing is offered, and the panel says why"), Car->IsRunning() && !Moving.bAvailable && !Moving.WhyNot.IsEmpty());
	Tests::RunAll(Stage, 26.5, Clock);
	TestEqual(TEXT("The ride ends at the Sphere"), Car->State(), FName(TEXT("sphere")));
	TestEqual(TEXT("20.4 m above the Atrium"), Tests::LevelMeters(*Car), 20.4, 1e-4);
	TestTrue(TEXT("through the airlock: the collar iris shut, the Sphere's two open"), Tests::ChannelValue(Stage, TEXT("atrium.up.closure")) == 0.0f
		&& Tests::ChannelValue(Stage, TEXT("sphere.neck.closure")) == 1.0f && Tests::ChannelValue(Stage, TEXT("sphere.platform.closure")) == 1.0f);
	const FMuseeUse Down = Car->UsageAt(Panel);
	TestTrue(TEXT("With its leaves open the panel offers the way down"), Down.bAvailable && Down.Verb.ToString() == TEXT("Down to the Atrium"));
	return true;
}

#endif
