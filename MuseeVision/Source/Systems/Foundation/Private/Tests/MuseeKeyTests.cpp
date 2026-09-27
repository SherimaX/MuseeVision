#include "MuseeFoundationTests.h"
#include "MuseeKey.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationKeyTest, "Musee.Foundation.Key", Musee::Tests::Flags)

bool FMuseeFoundationKeyTest::RunTest(const FString& Parameters)
{
	const TCHAR* const Valid[] = {
		TEXT("travertine"), TEXT("rotunda.west"), TEXT("track-spot.3000k.25deg"), TEXT("doorway.3000x4500.arched"),
		TEXT("salon.bay1.north"), TEXT("monet-impression-sunrise"), TEXT("3000k"), TEXT("a"),
	};
	for (const TCHAR* Text : Valid)
	{
		TestTrue(FString::Printf(TEXT("'%s' is a key"), Text), FMuseeKey::IsValid(Text));
		TestEqual(FString::Printf(TEXT("'%s' is written as itself"), Text), FMuseeKey::Parse(Text).Get(FMuseeKey()).ToString(), FString(Text));
	}
	const TCHAR* const Invalid[] = {
		TEXT(""), TEXT("Travertine"), TEXT("rotunda..west"), TEXT(".rotunda"), TEXT("rotunda."), TEXT("track_spot"),
		TEXT("track--spot"), TEXT("-spot"), TEXT("spot-"), TEXT("rotunda west"), TEXT("rotunda.w\u00E9st"), TEXT("rotunda/west"),
	};
	for (const TCHAR* Text : Invalid)
	{
		FString Why;
		TestFalse(FString::Printf(TEXT("'%s' is not a key"), Text), FMuseeKey::IsValid(Text, &Why));
		TestFalse(FString::Printf(TEXT("'%s' says why"), Text), Why.IsEmpty());
		TestFalse(FString::Printf(TEXT("'%s' doesn't parse"), Text), FMuseeKey::Parse(Text).IsSet());
	}

	const FMuseeKey West(TEXTVIEW("rotunda.west"));
	TestTrue(TEXT("A key's segments"), West.Segments() == TArray<FString>{TEXT("rotunda"), TEXT("west")});
	TestEqual(TEXT("One scope out"), West.Parent().ToString(), FString(TEXT("rotunda")));
	TestTrue(TEXT("A single segment has no parent"), West.Parent().Parent().IsNone());
	TestEqual(TEXT("One scope in"), West.Child(TEXTVIEW("jamb")).ToString(), FString(TEXT("rotunda.west.jamb")));
	TestTrue(TEXT("A child must be one valid segment"), West.Child(TEXTVIEW("left.jamb")).IsNone() && West.Child(TEXTVIEW("Jamb")).IsNone());
	TestEqual(TEXT("The none key's child is the segment"), FMuseeKey().Child(TEXTVIEW("rotunda")).ToString(), FString(TEXT("rotunda")));
	TestTrue(TEXT("The none key names nothing"), FMuseeKey().IsNone() && FMuseeKey().ToString().IsEmpty() && FMuseeKey().Segments().IsEmpty());

	TSet<FMuseeKey> Keys;
	Keys.Add(West);
	TestTrue(TEXT("Keys hash and compare by their text"), Keys.Contains(FMuseeKey::Parse(TEXT("rotunda.west")).GetValue()));

	FMuseeKey Imported;
	const TCHAR* Buffer = TEXT("salon.bay1.north, rest");
	TestTrue(TEXT("A key is read from text"), Imported.ImportTextItem(Buffer, PPF_None, nullptr, nullptr));
	TestEqual(TEXT("The key read"), Imported.ToString(), FString(TEXT("salon.bay1.north")));
	TestEqual(TEXT("Reading stops at the key's end"), FString(Buffer), FString(TEXT(", rest")));
	const TCHAR* Quoted = TEXT("\"cube.space\"");
	TestTrue(TEXT("A quoted key is read"), Imported.ImportTextItem(Quoted, PPF_None, nullptr, nullptr) && Imported.ToString() == TEXT("cube.space"));
	const TCHAR* Wrong = TEXT("Cube.Space");
	TestFalse(TEXT("Text that breaks the rule is refused"), Imported.ImportTextItem(Wrong, PPF_None, nullptr, nullptr));
	TestEqual(TEXT("And leaves the key as it was"), Imported.ToString(), FString(TEXT("cube.space")));

	FString Exported;
	West.ExportTextItem(Exported, FMuseeKey(), nullptr, PPF_None, nullptr);
	TestEqual(TEXT("A key is written as its text"), Exported, FString(TEXT("rotunda.west")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMuseeFoundationSeedTest, "Musee.Foundation.Seed", Musee::Tests::Flags)

bool FMuseeFoundationSeedTest::RunTest(const FString& Parameters)
{
	// The seeds are fixed forever: these values were computed from the published algorithm, outside the engine.
	TestEqual(TEXT("travertine"), Musee::Seed(FMuseeKey(TEXTVIEW("travertine"))), -1819960022);
	TestEqual(TEXT("rotunda.west"), Musee::Seed(FMuseeKey(TEXTVIEW("rotunda.west"))), 1777066024);
	TestEqual(TEXT("track-spot.3000k.25deg"), Musee::Seed(FMuseeKey(TEXTVIEW("track-spot.3000k.25deg"))), -544557213);

	const FMuseeKey Piece(TEXTVIEW("piece"));
	TSet<int32> Seeds;
	for (int32 Index = 0; Index < 1000; ++Index)
	{
		Seeds.Add(Musee::Seed(Piece.Child(FString::FromInt(Index))));
	}
	TestEqual(TEXT("A thousand parts draw a thousand seeds"), Seeds.Num(), 1000);
	TestEqual(TEXT("A key draws the same seed every time"), Musee::Seed(Piece.Child(TEXTVIEW("7"))), Musee::Seed(FMuseeKey(TEXTVIEW("piece.7"))));
	return true;
}

#endif
