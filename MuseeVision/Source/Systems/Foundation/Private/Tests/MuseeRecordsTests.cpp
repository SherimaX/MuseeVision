#include "Misc/FileHelper.h"
#include "Misc/SecureHash.h"
#include "MuseeConfiguration.h"
#include "MuseeContact.h"
#include "MuseeDependencySet.h"
#include "MuseeFoundationTests.h"
#include "MuseeRecords.h"

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

#endif
