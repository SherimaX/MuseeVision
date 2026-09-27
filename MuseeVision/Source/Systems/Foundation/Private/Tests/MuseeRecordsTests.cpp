#include "Misc/FileHelper.h"
#include "Misc/SecureHash.h"
#include "MuseeConfiguration.h"
#include "MuseeContact.h"
#include "MuseeDependencySet.h"
#include "MuseeFoundationTests.h"
#include "MuseeRecords.h"
#include "MuseeTestSurvey.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::Tests
{
	/** The SHA-1 of a fixture's bytes, as a dependency set holds it. */
	FString FixtureHash(const FString& Name)
	{
		TArray<uint8> Bytes;
		FFileHelper::LoadFileToArray(Bytes, *Fixture(Name));
		return FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num()).ToString();
	}

	/** Reads a fixture that must be refused, and returns the reason (empty if it was read). */
	template <typename T>
	FString RefusalOf(const FString& Name)
	{
		const TValueOrError<T, FString> Read = ReadRecord<T>(Fixture(Name));
		return Read.HasError() ? Read.GetError() : FString();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationRecordsDataRootTest, "Musee.Foundation.Records.DataRoot", Musee::Tests::Flags)

bool FMuseeFoundationRecordsDataRootTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Records;
	TestEqual(TEXT("The editor's absolute project folder holds the data"),
		DataRootOf(TEXT("C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/"), TEXT("C:/mv/wt/slot/MuseeVision/")),
		FString(TEXT("C:/mv/wt/slot/MuseeVision/Data")));
	TestEqual(TEXT("A packaged game's relative project folder holds it too, where Data/ is staged beside the game"),
		DataRootOf(TEXT("D:/Builds/MuseeVision-20260927-1200/Windows/MuseeVision/Binaries/Win64/"), TEXT("../../../MuseeVision/")),
		FString(TEXT("D:/Builds/MuseeVision-20260927-1200/Windows/MuseeVision/Data")));
	const FString Root = DataRoot();
	TestEqual(TEXT("This process resolves its data root the same way"), Root, DataRootOf(FPlatformProcess::BaseDir(), FPaths::ProjectDir()));
	TestFalse(TEXT("as a full path"), FPaths::IsRelative(Root) || Root.Contains(TEXT("..")));
	TestTrue(TEXT("that holds the records the museum reads while it runs"), FPaths::DirectoryExists(Root / TEXT("Time")) && FPaths::DirectoryExists(Root / TEXT("Light")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationRecordsReadTest, "Musee.Foundation.Records.Read", Musee::Tests::Flags)

bool FMuseeFoundationRecordsReadTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Tests;

	const TValueOrError<FMuseeConfiguration, FString> Raised = Musee::ReadRecord<FMuseeConfiguration>(Fixture(TEXT("Configurations/cylinder-raised.json")));
	if (!TestTrue(TEXT("One record is read from its file"), Raised.HasValue()))
	{
		AddError(Raised.GetError());
		return false;
	}
	const FMuseeConfiguration& Configuration = Raised.GetValue();
	TestEqual(TEXT("A key is read from its text"), Configuration.Name.ToString(), FString(TEXT("rotunda.cylinder-raised")));
	TestTrue(TEXT("Nested structs and arrays are read"), Configuration.Poses.Num() == 1 && Configuration.Poses[0].Key == FMuseeKey(TEXTVIEW("rotunda.cylinder")));
	TestEqual(TEXT("Numbers are read in their declared unit"), Configuration.Poses[0].Frame.OriginMeters.Z, 1.2, 1e-12);

	const TValueOrError<TArray<FMuseeConfiguration>, FString> Folder = Musee::ReadRecords<FMuseeConfiguration>(Fixture(TEXT("Configurations")));
	TestTrue(TEXT("A folder's records are read in name order"), Folder.HasValue() && Folder.GetValue().Num() == 2
		&& Folder.GetValue()[0].Name.ToString() == TEXT("rotunda.cylinder-lowered") && Folder.GetValue()[1].Name.ToString() == TEXT("rotunda.cylinder-raised"));

	const TValueOrError<TArray<FMuseeContact>, FString> Contacts = Musee::ReadRecords<FMuseeContact>(Fixture(TEXT("Contacts.json")));
	TestTrue(TEXT("A file's listed records are read"), Contacts.HasValue() && Contacts.GetValue().Num() == 2);
	if (Contacts.HasValue() && Contacts.GetValue().Num() == 2)
	{
		TestEqual(TEXT("The first record's speed"), Contacts.GetValue()[0].SpeedMetersPerSecond, 0.35f);
		TestEqual(TEXT("The second record's substance"), Contacts.GetValue()[1].Substance.ToString(), FString(TEXT("oak")));
	}

	const FString Two = RefusalOf<FMuseeContact>(TEXT("Contacts.json"));
	TestTrue(TEXT("One record is read only from a file of one"), Two.Contains(TEXT("holds 2 records where one is read")));
	const FString Missing = RefusalOf<FMuseeContact>(TEXT("Nowhere.json"));
	TestTrue(TEXT("A missing file is refused"), Missing.Contains(TEXT("there is no record file or folder here")));
	TestTrue(TEXT("Records live under the project's Data folder"), Musee::Records::DataRoot().EndsWith(TEXT("/Data")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationRecordsRequiredTest, "Musee.Foundation.Records.Required", Musee::Tests::Flags)

bool FMuseeFoundationRecordsRequiredTest::RunTest(const FString& Parameters)
{
	const FString Refusal = Musee::Tests::RefusalOf<FMuseeConfiguration>(TEXT("Rejects/missing-required.json"));
	TestTrue(TEXT("A missing required field is refused, by its place in the record"), Refusal.Contains(TEXT("Poses[0].Key: is required and missing")));
	TestTrue(TEXT("The refusal names the file"), Refusal.Contains(TEXT("missing-required.json")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationRecordsUnitsTest, "Musee.Foundation.Records.Units", Musee::Tests::Flags)

bool FMuseeFoundationRecordsUnitsTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Tests;
	TestTrue(TEXT("A unit's name and its symbol both agree with the field"),
		Musee::ReadRecords<FMuseeConfiguration>(Fixture(TEXT("Configurations"))).HasValue());
	TestTrue(TEXT("A declared unit that disagrees is refused"),
		RefusalOf<FMuseeContact>(TEXT("Rejects/units-disagree.json")).Contains(TEXT("$units.SpeedMetersPerSecond: is declared in km/h, but the field is in m/s")));
	TestTrue(TEXT("A unit declared for a field without one is refused"),
		RefusalOf<FMuseeContact>(TEXT("Rejects/units-on-unitless.json")).Contains(TEXT("but the field has no unit")));
	TestTrue(TEXT("A unit declared for no field is refused"),
		RefusalOf<FMuseeContact>(TEXT("Rejects/units-unknown-field.json")).Contains(TEXT("FMuseeContact has no field of this name")));
	TestTrue(TEXT("A unit nobody knows is refused"),
		RefusalOf<FMuseeContact>(TEXT("Rejects/units-unknown-unit.json")).Contains(TEXT("is not a unit the engine's unit table knows")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationRecordsRejectsTest, "Musee.Foundation.Records.Rejects", Musee::Tests::Flags)

bool FMuseeFoundationRecordsRejectsTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Tests;
	TestTrue(TEXT("A field the struct doesn't have is refused"),
		RefusalOf<FMuseeContact>(TEXT("Rejects/unknown-field.json")).Contains(TEXT("Sped: FMuseeContact has no field of this name")));
	TestTrue(TEXT("A value of the wrong kind is refused"),
		RefusalOf<FMuseeContact>(TEXT("Rejects/wrong-type.json")).Contains(TEXT("SpeedMetersPerSecond: is a string where a number is expected")));
	TestTrue(TEXT("A key that breaks the rule is refused"),
		RefusalOf<FMuseeContact>(TEXT("Rejects/invalid-key.json")).Contains(TEXT("Substance: 'Travertine' is not a key")));
	TestTrue(TEXT("A file that isn't JSON is refused"),
		RefusalOf<FMuseeContact>(TEXT("Rejects/not-json.json")).Contains(TEXT("is not a JSON object")));
	TestTrue(TEXT("An unknown declaration is refused"),
		RefusalOf<FMuseeContact>(TEXT("Rejects/unknown-declaration.json")).Contains(TEXT("$unit: is not a declaration")));
	TestTrue(TEXT("Fields beside listed records are refused"),
		RefusalOf<FMuseeContact>(TEXT("Rejects/fields-beside-records.json")).Contains(TEXT("has fields beside $records")));
	TestTrue(TEXT("A required key written empty is refused"),
		RefusalOf<FMuseeConfiguration>(TEXT("Rejects/required-key-empty.json")).Contains(TEXT("Name: '' is not a key")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationRecordsRangesTest, "Musee.Foundation.Records.Ranges", Musee::Tests::Flags)

bool FMuseeFoundationRecordsRangesTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Tests;
	const TValueOrError<FMuseeTestSurvey, FString> Read = Musee::ReadRecord<FMuseeTestSurvey>(Fixture(TEXT("Survey/survey.json")));
	if (!TestTrue(TEXT("A survey with open ranges and an empty key is read"), Read.HasValue()))
	{
		AddError(Read.GetError());
		return false;
	}
	const FMuseeTestSurvey& Survey = Read.GetValue();
	TestTrue(TEXT("[50, null] is at least 50, unbounded above"), Survey.Lux.Num() == 2 && Survey.Lux[0] == 50.0 && !FMath::IsFinite(Survey.Lux[1]) && Survey.Lux[1] > 0.0);
	TestTrue(TEXT("[null, 3] is at most 3, unbounded below"), Survey.Ratio.Num() == 2 && !FMath::IsFinite(Survey.Ratio[0]) && Survey.Ratio[0] < 0.0f && Survey.Ratio[1] == 3.0f);
	TestTrue(TEXT("An optional key written empty names nothing"), Survey.Body.IsNone());
	TestTrue(TEXT("A range open at both ends is refused"),
		RefusalOf<FMuseeTestSurvey>(TEXT("Rejects/range-both-open.json")).Contains(TEXT("Lux: is open at both ends")));
	TestTrue(TEXT("A range whose least is above its most is refused"),
		RefusalOf<FMuseeTestSurvey>(TEXT("Rejects/range-backwards.json")).Contains(TEXT("Lux: has its least end above its most")));
	TestTrue(TEXT("A range of three numbers is refused"),
		RefusalOf<FMuseeTestSurvey>(TEXT("Rejects/range-three.json")).Contains(TEXT("Lux: has 3 numbers where a range [least, most] is written")));
	TestTrue(TEXT("A null in an array that isn't a range is still refused"),
		RefusalOf<FMuseeTestSurvey>(TEXT("Rejects/range-null-array.json")).Contains(TEXT("Outline[1]: is null; an array holds values")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationRecordsDependenciesTest, "Musee.Foundation.Records.Dependencies", Musee::Tests::Flags)

bool FMuseeFoundationRecordsDependenciesTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Tests;
	FMuseeDependencySet Outer;
	FMuseeDependencySet Inner;
	{
		FMuseeDependencySet::FScope OuterScope(Outer);
		(void)Musee::ReadRecords<FMuseeContact>(Fixture(TEXT("Contacts.json")));
		{
			FMuseeDependencySet::FScope InnerScope(Inner);
			(void)Musee::ReadRecords<FMuseeConfiguration>(Fixture(TEXT("Configurations")));
			(void)Musee::ReadRecord<FMuseeContact>(Fixture(TEXT("Rejects/invalid-key.json")));
		}
	}
	(void)Musee::ReadRecords<FMuseeContact>(Fixture(TEXT("Contacts.json")));

	TestEqual(TEXT("The inner make read three files, one of them refused"), Inner.Files().Num(), 3);
	TestEqual(TEXT("The outer make read those and its own"), Outer.Files().Num(), 4);
	const FString* ContactsHash = Outer.Files().Find(Fixture(TEXT("Contacts.json")));
	TestTrue(TEXT("A file is kept with the hash of its content"), ContactsHash && *ContactsHash == FixtureHash(TEXT("Contacts.json")));
	TestFalse(TEXT("The inner make doesn't depend on the outer make's reads"), Inner.Files().Contains(Fixture(TEXT("Contacts.json"))));

	FMuseeDependencySet Forward;
	FMuseeDependencySet Backward;
	Forward.Add(TEXT("Matter/Substances/travertine.json"), TEXT("aa"));
	Forward.Add(TEXT("Site/PortClasses.json"), TEXT("bb"));
	Backward.Add(TEXT("Site/PortClasses.json"), TEXT("bb"));
	Backward.Add(TEXT("Matter/Substances/travertine.json"), TEXT("aa"));
	TestEqual(TEXT("The set's hash doesn't depend on reading order"), Forward.Hash(), Backward.Hash());
	Backward.Add(TEXT("Site/PortClasses.json"), TEXT("cc"));
	TestNotEqual(TEXT("A changed record changes the hash"), Forward.Hash(), Backward.Hash());
	FMuseeDependencySet Joined;
	Joined.Append(Forward);
	TestEqual(TEXT("A set appended holds the same files"), Joined.Hash(), Forward.Hash());
	return true;
}



IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationRecordsVectorsTest, "Musee.Foundation.Records.Vectors", Musee::Tests::Flags)

bool FMuseeFoundationRecordsVectorsTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Tests;
	const TValueOrError<FMuseeTestSurvey, FString> Read = Musee::ReadRecord<FMuseeTestSurvey>(Fixture(TEXT("Survey/survey.json")));
	if (!TestTrue(TEXT("Vectors written as their numbers are read"), Read.HasValue()))
	{
		AddError(Read.GetError());
		return false;
	}
	const FMuseeTestSurvey& Survey = Read.GetValue();
	TestEqual(TEXT("A point is read from [x, y, z]"), Survey.PointMeters, FVector(1.5, -2.25, 0.4));
	TestEqual(TEXT("A size is read from [x, y]"), Survey.SizeMeters, FVector2D(12.75, 2.0));
	TestTrue(TEXT("An outline is read from [[x, y], ...]"), Survey.Outline.Num() == 4 && Survey.Outline[2] == FVector2D(3.0, 2.0));
	TestTrue(TEXT("A path mixes both forms of a point"), Survey.Path.Num() == 2 && Survey.Path[1] == FVector(1.0, 2.0, 3.0));
	const FVector2D* East = Survey.Marks.Find(TEXT("east"));
	TestTrue(TEXT("A map's vectors are read"), East && *East == FVector2D(4.15, 0.0));
	TestTrue(TEXT("A text written as a plain string is its source, with no key yet"), Survey.Labels.Num() == 2
		&& Survey.Labels[0].Source == TEXT("The east mark") && Survey.Labels[0].Key.IsNone());
	TestTrue(TEXT("A text written as a key and a source keeps both"), Survey.Labels.Num() == 2
		&& Survey.Labels[1].Key == FMuseeKey(TEXTVIEW("survey.west")) && Survey.Labels[1].Source == TEXT("The west mark"));

	TestTrue(TEXT("A vector of the wrong count is refused"),
		RefusalOf<FMuseeTestSurvey>(TEXT("Rejects/vector-short.json")).Contains(TEXT("PointMeters: has 2 numbers where a vector of 3 is written")));
	TestTrue(TEXT("A vector's numbers are numbers"),
		RefusalOf<FMuseeTestSurvey>(TEXT("Rejects/vector-not-numbers.json")).Contains(TEXT("Outline[1][1]: is a string where a number is expected")));
	return true;
}

#endif
