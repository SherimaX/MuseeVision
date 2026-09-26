#include "MuseeCheck.h"
#include "MuseeDefect.h"
#include "MuseeFoundationTests.h"
#include "MuseeProbe.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::Tests
{
	/** A check over a configuration: every body stands on or above its floor. */
	TArray<FMuseeFinding> CheckBodiesAboveFloor(const FMuseeConfiguration& Configuration)
	{
		TArray<FMuseeFinding> Findings;
		for (const FMuseePose& Pose : Configuration.Poses)
		{
			const FVector Origin = Pose.Frame.OriginMeters;
			if (Origin.Z < 0.0)
			{
				const FMuseeMeasurement Height = FMuseeMeasurement::Of(TEXT("height of the body above its floor"), Meters(Origin.Z))
					.Expecting(Meters(0.0), Meters(std::numeric_limits<double>::infinity()))
					.At(Pose.Key, Configuration.Name.Parent(), Origin);
				Findings.Add(FMuseeFinding::Make(TEXT("Rotunda"), TEXT("A body stands below its floor."), Height));
			}
		}
		return Findings;
	}

	/** A check over nothing, as a museum-wide convention is checked. */
	TArray<FMuseeFinding> CheckNothingMissing()
	{
		return {};
	}

	/** A probe: the distance from the museum's origin to the reticle. */
	TArray<FMuseeMeasurement> MeasureDistanceFromOrigin(const IMuseeProbe::FQuery& Query)
	{
		return {FMuseeMeasurement::Of(TEXT("distance from the origin"), Meters(Query.PointMeters.Size()))
			.At(Query.Subject, FMuseeKey(TEXTVIEW("museum")), Query.PointMeters)};
	}

	FMuseeConfiguration SunkCylinder()
	{
		FMuseePose Pose;
		Pose.Key = FMuseeKey(TEXTVIEW("rotunda.cylinder"));
		Pose.Frame = FMuseeFrame(FVector(0.0, 0.0, -0.5));
		FMuseeConfiguration Configuration;
		Configuration.Name = FMuseeKey(TEXTVIEW("rotunda.cylinder-sunk"));
		Configuration.Set(Pose);
		return Configuration;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationChecksTest, "Musee.Foundation.Checks", Musee::Tests::Flags)

bool FMuseeFoundationChecksTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Tests;
	const FName Name(TEXT("Foundation.BodiesAboveFloor"));
	{
		const FMuseeCheck::FRegistrar Registrar(FMuseeCheck::Make<EMuseeCheckScope::Configuration>(TEXT("Foundation.BodiesAboveFloor"), &CheckBodiesAboveFloor, __FILE__));
		const FMuseeCheck* Check = FMuseeCheck::Find(Name);
		if (!TestNotNull(TEXT("A check is registered while its registrar lives"), Check))
		{
			return false;
		}
		TestTrue(TEXT("It keeps its scope"), Check->Scope() == EMuseeCheckScope::Configuration);
		TestTrue(TEXT("Its owner is its first name"), Check->Owner() == TEXT("Foundation"));
		TestTrue(TEXT("It declares its inputs"), Check->InputTypes() == TArray<const UScriptStruct*>{FMuseeConfiguration::StaticStruct()});
		TestTrue(TEXT("It is listed"), FMuseeCheck::All().Contains(Check));

		const TValueOrError<TArray<FMuseeFinding>, FString> Unfed = Check->Evaluate(FMuseeCheck::FInputs());
		TestTrue(TEXT("A check without its inputs doesn't run"), Unfed.HasError() && Unfed.GetError().Contains(TEXT("FMuseeConfiguration")));

		const FMuseeConfiguration Sunk = SunkCylinder();
		const TValueOrError<TArray<FMuseeFinding>, FString> Result = Check->Evaluate(FMuseeCheck::FInputs().Add(Sunk));
		if (TestTrue(TEXT("A check finds what is wrong"), Result.HasValue() && Result.GetValue().Num() == 1))
		{
			const FMuseeFinding& Finding = Result.GetValue()[0];
			TestTrue(TEXT("The finding names its check and owner"), Finding.Check == Name && Finding.Owner == TEXT("Foundation"));
			TestTrue(TEXT("And the drawer of the piece at fault"), Finding.Drawer == TEXT("Rotunda"));
			TestTrue(TEXT("And the reading, outside its range"), Finding.Measurement.Unit == EUnit::Meters && !Finding.Measurement.IsExpected());
			TestTrue(TEXT("A finding reads as one line"), Finding.ToString().Contains(TEXT("Foundation.BodiesAboveFloor")));
		}

		AddExpectedErrorPlain(TEXT("is already registered by"), EAutomationExpectedErrorFlags::Contains, 1);
		TestFalse(TEXT("A truth has one check"),
			FMuseeCheck::Register(FMuseeCheck::Make<EMuseeCheckScope::Configuration>(TEXT("Foundation.BodiesAboveFloor"), &CheckBodiesAboveFloor, "Elsewhere.cpp")));
	}
	TestNull(TEXT("A check leaves with its registrar"), FMuseeCheck::Find(Name));

	const FMuseeCheck Convention = FMuseeCheck::Make<EMuseeCheckScope::Museum>(TEXT("Foundation.NothingMissing"), &CheckNothingMissing, __FILE__);
	TestTrue(TEXT("A check may take no inputs"), Convention.InputTypes().IsEmpty() && Convention.Evaluate(FMuseeCheck::FInputs()).HasValue());

	TestTrue(TEXT("Form.Sealed is a check's name"), FMuseeCheck::IsValidName(TEXT("Form.Sealed")));
	TestTrue(TEXT("Light.MeetsTarget is a check's name"), FMuseeCheck::IsValidName(TEXT("Light.MeetsTarget")));
	for (const TCHAR* Wrong : {TEXT("Form.sealed"), TEXT("form.Sealed"), TEXT("Form.Sealed.Twice"), TEXT("FormSealed"), TEXT("Form."), TEXT("Form.Seal-ed")})
	{
		TestFalse(FString::Printf(TEXT("%s is not a check's name"), Wrong), FMuseeCheck::IsValidName(Wrong));
	}
	AddExpectedErrorPlain(TEXT("can't be a check"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("A name that breaks the rule is refused"),
		FMuseeCheck::Register(FMuseeCheck::Make<EMuseeCheckScope::Museum>(TEXT("form.sealed"), &CheckNothingMissing, __FILE__)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationProbesTest, "Musee.Foundation.Probes", Musee::Tests::Flags)

bool FMuseeFoundationProbesTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Tests;
	const FName Name(TEXT("Foundation::MeasureDistanceFromOrigin"));
	{
		const IMuseeProbe::FRegistrar Registrar(TEXT("Foundation::MeasureDistanceFromOrigin"), &MeasureDistanceFromOrigin, __FILE__);
		const TSharedPtr<IMuseeProbe> Probe = IMuseeProbe::Find(Name);
		if (!TestTrue(TEXT("A probe is registered while its registrar lives"), Probe.IsValid()))
		{
			return false;
		}
		TestTrue(TEXT("It is listed"), IMuseeProbe::Names().Contains(Name));
		IMuseeProbe::FQuery Query;
		Query.PointMeters = FVector(3.0, 4.0, 0.0);
		const TArray<FMuseeMeasurement> Readings = Probe->Read(Query);
		TestTrue(TEXT("It is read at the reticle"), Readings.Num() == 1 && FMath::IsNearlyEqual(Readings[0].Value, 5.0) && Readings[0].Unit == EUnit::Meters);

		AddExpectedErrorPlain(TEXT("is already registered by"), EAutomationExpectedErrorFlags::Contains, 1);
		const IMuseeProbe::FRegistrar Rival(TEXT("Foundation::MeasureDistanceFromOrigin"), &MeasureDistanceFromOrigin, "Elsewhere.cpp");
		TestTrue(TEXT("A measurement has one probe"), IMuseeProbe::Find(Name) == Probe);
	}
	TestFalse(TEXT("A probe leaves with its registrar"), IMuseeProbe::Find(Name).IsValid());

	for (const TCHAR* Right : {TEXT("Light::MeasureIlluminance"), TEXT("Form::FindLeakPaths"), TEXT("Light::ListSources")})
	{
		TestTrue(FString::Printf(TEXT("%s is a probe's name"), Right), IMuseeProbe::IsValidName(Right));
	}
	for (const TCHAR* Wrong : {TEXT("Light::Illuminance"), TEXT("light::MeasureIlluminance"), TEXT("Light.MeasureIlluminance"), TEXT("Light::Measure"),
		TEXT("Light::Measureilluminance"), TEXT("::MeasureIlluminance")})
	{
		TestFalse(FString::Printf(TEXT("%s is not a probe's name"), Wrong), IMuseeProbe::IsValidName(Wrong));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationMeasurementTest, "Musee.Foundation.Measurement", Musee::Tests::Flags)

bool FMuseeFoundationMeasurementTest::RunTest(const FString& Parameters)
{
	using namespace Musee;
	const FMuseeMeasurement Free = FMuseeMeasurement::Of(TEXT("illuminance on the canvas"), Lux(180.0));
	TestTrue(TEXT("A reading without a range expects nothing"), !Free.HasExpectation() && Free.IsExpected());
	const FMuseeMeasurement Within = Free.Expecting(Lux(150.0), Lux(250.0));
	TestTrue(TEXT("A reading within its range is expected"), Within.HasExpectation() && Within.IsExpected());
	TestFalse(TEXT("A reading outside it isn't"), FMuseeMeasurement::Of(TEXT("illuminance on the canvas"), Lux(90.0)).Expecting(Lux(150.0), Lux(250.0)).IsExpected());
	const FString Line = Within.At(FMuseeKey(TEXTVIEW("salon.bay1.north")), FMuseeKey(TEXTVIEW("salon")), FVector(1.0, 2.0, 1.6)).ToString();
	TestTrue(TEXT("A reading reads as one line, with its unit, subject and place"),
		Line.Contains(TEXT("180 lux")) && Line.Contains(TEXT("expected 150 to 250 lux")) && Line.Contains(TEXT("salon.bay1.north")) && Line.Contains(TEXT("in salon")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationDefectsTest, "Musee.Foundation.Defects", Musee::Tests::Flags)

bool FMuseeFoundationDefectsTest::RunTest(const FString& Parameters)
{
	FMuseeDefect Streak;
	Streak.Number = TEXT("D-0001");
	Streak.Title = TEXT("A light streak across the dome");
	Streak.Symptom = TEXT("A bright line between two coffer rings at 15:00 in June.");
	Streak.Check = TEXT("Form.Sealed");
	Streak.Drawer = TEXT("Rotunda");
	Streak.Cause = TEXT("The coffer ring stopped 2 mm short of the drum.");
	Streak.Fix = TEXT("The ring is drawn to the drum's inner face.");
	Streak.bClosed = true;
	FMuseeDefect Glare = Streak;
	Glare.Number = TEXT("D-0002");
	Glare.Title = TEXT("Glare on the Water Lilies");
	Glare.Check = TEXT("Light.MeetsTarget");
	Glare.Fix.Empty();
	Glare.bClosed = false;

	const FString Journal = TEXT("# Defects\n\nWhat the museum got wrong.\n\n") + Streak.ToMarkdown() + TEXT("\n") + Glare.ToMarkdown();
	const TValueOrError<TArray<FMuseeDefect>, FString> Parsed = FMuseeDefect::ParseJournal(Journal);
	if (!TestTrue(TEXT("A Journal written from defects reads back"), Parsed.HasValue() && Parsed.GetValue().Num() == 2))
	{
		AddError(Parsed.HasError() ? Parsed.GetError() : FString(TEXT("The wrong number of defects.")));
		return false;
	}
	const FMuseeDefect& Read = Parsed.GetValue()[0];
	TestTrue(TEXT("Every field survives"), Read.Number == Streak.Number && Read.Title == Streak.Title && Read.Symptom == Streak.Symptom
		&& Read.Check == Streak.Check && Read.Drawer == Streak.Drawer && Read.Cause == Streak.Cause && Read.Fix == Streak.Fix && Read.bClosed);
	TestTrue(TEXT("An open defect may wait for its fix"), !Parsed.GetValue()[1].bClosed && Parsed.GetValue()[1].Fix.IsEmpty());

	TestTrue(TEXT("D-0001 is a defect's number"), FMuseeDefect::IsValidNumber(TEXT("D-0001")));
	TestFalse(TEXT("D-001 is a decision's"), FMuseeDefect::IsValidNumber(TEXT("D-001")));

	const auto RefusalOf = [](const FString& Text) { const auto Result = FMuseeDefect::ParseJournal(Text); return Result.HasError() ? Result.GetError() : FString(); };
	TestTrue(TEXT("A closed defect needs its fix"), RefusalOf(Streak.ToMarkdown().Replace(TEXT("- **Fix:** The ring is drawn to the drum's inner face.\n"), TEXT(""))).Contains(TEXT("closed without its fix")));
	TestTrue(TEXT("A defect needs its check"), RefusalOf(Streak.ToMarkdown().Replace(TEXT("- **Check:** Form.Sealed\n"), TEXT(""))).Contains(TEXT("has no Check")));
	TestTrue(TEXT("A check's name follows the rule"), RefusalOf(Streak.ToMarkdown().Replace(TEXT("Form.Sealed"), TEXT("sealed"))).Contains(TEXT("names 'sealed' as its check")));
	TestTrue(TEXT("A status is open or closed"), RefusalOf(Streak.ToMarkdown().Replace(TEXT("closed"), TEXT("done"))).Contains(TEXT("has status 'done'")));
	TestTrue(TEXT("A heading is a defect's number"), RefusalOf(Streak.ToMarkdown().Replace(TEXT("D-0001"), TEXT("D-1"))).Contains(TEXT("is not a defect's heading")));
	TestTrue(TEXT("A field is given once"), RefusalOf(Streak.ToMarkdown() + TEXT("- **Cause:** Again.\n")).Contains(TEXT("gives Cause twice")));
	TestTrue(TEXT("A number is used once"), RefusalOf(Streak.ToMarkdown() + Streak.ToMarkdown()).Contains(TEXT("numbered twice")));
	TestTrue(TEXT("An entry holds only fields"), RefusalOf(Streak.ToMarkdown() + TEXT("A stray sentence.\n")).Contains(TEXT("neither a field nor a heading")));

	FMuseeDefect Example = Streak;
	Example.Check = TEXT("Foundation.ExampleHolds");
	TestFalse(TEXT("A defect naming an unregistered check says so"), Example.NamesExistingCheck());
	{
		const FMuseeCheck::FRegistrar Registrar(FMuseeCheck::Make<EMuseeCheckScope::Museum>(TEXT("Foundation.ExampleHolds"), &Musee::Tests::CheckNothingMissing, __FILE__));
		TestTrue(TEXT("And a defect naming a registered one is satisfied"), Example.NamesExistingCheck());
	}
	return true;
}

#endif
