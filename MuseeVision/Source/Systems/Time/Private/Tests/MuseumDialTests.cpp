#include "Tests/TimeTestSupport.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "TimeMuseumDial.h"

// The museum's sun clock as a public dial: what the Rotunda lays its inlay from (Docs/Stages/rotunda.md, section 5).
namespace Musee::Time::Tests
{
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimeMuseumDialTest, "Musee.Time.MuseumDial", Flags)
	bool FTimeMuseumDialTest::RunTest(const FString& Parameters)
	{
		const TValueOrError<FMuseumDial, FString> Read = FMuseumDial::Read();
		if (!TestTrue(TEXT("The museum's dial reads from its time record"), Read.HasValue()))
		{
			AddError(Read.GetError());
			return false;
		}
		const FMuseumDial& Dial = Read.GetValue();
		Near(*this, TEXT("its node is the bronze node's centre, 20.25 m up"), Dial.NodusHeightMeters(), 20.25, 1e-12);
		Near(*this, TEXT("its shadow walks the hour ring, r 6.78"), Dial.ShadowRadiusMeters(), 6.78, 1e-12);
		Near(*this, TEXT("VI in the west"), Dial.AngleDegrees(6.0), 180.0, 1e-9);
		Near(*this, TEXT("XII in the north, the meridian"), Dial.AngleDegrees(12.0), 270.0, 1e-9);
		Near(*this, TEXT("VI in the east"), Dial.AngleDegrees(18.0), 360.0, 1e-9);
		Near(*this, TEXT("15 degrees an hour"), Dial.AngleDegrees(9.5) - Dial.AngleDegrees(8.5), 15.0, 1e-9);

		const TArray<FDialMark> Minutes = Dial.Marks(1);
		TestEqual(TEXT("A mark for every minute from 06:00 to 18:00"), Minutes.Num(), 12 * 60 + 1);
		TestEqual(TEXT("thirteen of them whole hours"), Minutes.FilterByPredicate([](const FDialMark& M) { return M.IsHour(); }).Num(), 13);
		TestEqual(TEXT("Marks every five minutes"), Dial.Marks(5).Num(), 12 * 12 + 1);

		// Time.DialTrue on the lowered dial: every mark lies where the sun Time shows at that minute casts the node's
		// shadow, found from the node along the sun's direction onto the face, and reads back its own minute.
		double LargestMiss = 0.0;
		for (const FDialMark& Mark : Minutes)
		{
			const FVector Sun = Dial.SunDirection(Mark.Hour());
			const FVector Node(0.0, 0.0, Dial.NodusHeightMeters());
			const FVector Shadow = Node - Sun * (Dial.NodusHeightMeters() / Sun.Z);
			const FVector Laid(Mark.Direction * Dial.ShadowRadiusMeters(), 0.0);
			LargestMiss = FMath::Max(LargestMiss, (Shadow - Laid).Size());
			LargestMiss = FMath::Max(LargestMiss, FMath::Abs(Dial.ReadHour(Laid) - Mark.Hour()) * Dial.ShadowRadiusMeters());
		}
		Near(*this, TEXT("largest miss of any mark, 06:00-18:00 (m)"), LargestMiss, 0.0, 1e-9);

		TestEqual(TEXT("The morning's first numeral"), FMuseumDial::Numeral(6), FString(TEXT("VI")));
		TestEqual(TEXT("noon's"), FMuseumDial::Numeral(12), FString(TEXT("XII")));
		TestEqual(TEXT("one o'clock's, on a twelve-hour face"), FMuseumDial::Numeral(13), FString(TEXT("I")));
		TestEqual(TEXT("four o'clock's, as the face spells it"), FMuseumDial::Numeral(16), FString(TEXT("IV")));
		TestEqual(TEXT("the evening's last"), FMuseumDial::Numeral(18), FString(TEXT("VI")));
		return true;
	}
}

#endif
