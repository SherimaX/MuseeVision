#include "ArtHang.h"
#include "FormMesh.h"
#include "Misc/AutomationTest.h"
#include "MuseeRecords.h"
#include "Stone/StoneMesh.h"
#include "Stone/StoneRecord.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::ArtMaking::StoneTests
{
	/** Art's making tests run in the editor, where its making twin lives, and belong to the product's own filter. */
	inline constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	/** The rock of a work as the record forms it, at a height. */
	FStoneSpec SpecOf(const FArtStones& Stones, const TCHAR* Work, double HeightMeters)
	{
		const FArtStone* Stone = Stones.Find(Work);
		return { Stone ? Stones.FindKind(Stone->Kind) : nullptr, HeightMeters, Stone ? Stone->FootRadiusMeters : 0.0, 7 };
	}
}

/**
 * Chenghuai's scholar's rocks, formed from the stones' record: each kind well recorded; each rock a surface up to its
 * plan's height, its foot within the radius its ground or stand leaves it, the same every time it is formed.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtStoneTest, "Musee.Art.Stone", Musee::ArtMaking::StoneTests::Flags)

bool FArtStoneTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	const TValueOrError<FArtStones, FString> Stones = Musee::ReadRecord<FArtStones>(FArtStones::Record);
	if (!TestTrue(TEXT("The stones' record reads"), Stones.HasValue()))
	{
		AddError(Stones.GetError());
		return false;
	}
	for (const FArtStoneKind& Kind : Stones.GetValue().Kinds)
	{
		for (const FString& Fault : StoneKindFaults(Kind))
		{
			AddError(FString::Printf(TEXT("The %s kind: %s"), *Kind.Kind.ToString(), *Fault));
		}
	}
	struct FCase
	{
		const TCHAR* Work;
		double HeightMeters;
	};
	for (const FCase& Case : { FCase{ TEXT("chenghuai-shixiong"), 3.0 }, FCase{ TEXT("chenghuai-lingbi-rock"), 0.32 } })
	{
		const FStoneSpec Spec = StoneTests::SpecOf(Stones.GetValue(), Case.Work, Case.HeightMeters);
		if (!TestNotNull(*FString::Printf(TEXT("%s has its kind of rock"), Case.Work), Spec.Kind))
		{
			continue;
		}
		FFormMesh Rock;
		BuildStone(Spec, Rock);
		if (!TestFalse(*FString::Printf(TEXT("%s is formed"), Case.Work), Rock.IsEmpty()))
		{
			continue;
		}
		double Top = 0.0;
		double Lowest = TNumericLimits<double>::Max();
		double FootReach = 0.0;
		for (const FVector& Point : Rock.AllPositions())
		{
			Top = FMath::Max(Top, Point.Z);
			Lowest = FMath::Min(Lowest, Point.Z);
		}
		// The foot: every point of the surface within a cell of its plane.
		const double Cell = Case.HeightMeters / Spec.Kind->Cells;
		for (const FVector& Point : Rock.AllPositions())
		{
			if (Point.Z < Lowest + Cell)
			{
				FootReach = FMath::Max(FootReach, FVector2D(Point.X, Point.Y).Size());
			}
		}
		TestTrue(*FString::Printf(TEXT("%s stands on its foot's plane, nothing of it below (%.4f)"), Case.Work, Lowest), FMath::Abs(Lowest) < Cell);
		TestTrue(*FString::Printf(TEXT("%s rises to its plan's height (%.3f of %.3f)"), Case.Work, Top, Case.HeightMeters),
			Top > 0.93 * Case.HeightMeters && Top < 1.05 * Case.HeightMeters);
		TestTrue(*FString::Printf(TEXT("%s keeps its foot within %.3f (%.3f)"), Case.Work, Spec.FootRadiusMeters, FootReach),
			FootReach <= Spec.FootRadiusMeters + Cell);
		FFormMesh Again;
		BuildStone(Spec, Again);
		TestTrue(*FString::Printf(TEXT("%s is the same every time it is formed"), Case.Work), Again.NumVertices() == Rock.NumVertices()
			&& Again.NumTriangles() == Rock.NumTriangles() && Again.Position(0) == Rock.Position(0));
	}
	return true;
}

/**
 * The Lingbi rock's stand (座), carved to the rock as a scholar's-rock stand is: the rock sits down into a recess cut to
 * its own foot, which no part of it passes through; the stand's plan is the rock's foot grown by an even lip, not a
 * turned disc; and it stands on short feet, its body clear of the table.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtStoneStandTest, "Musee.Art.StoneStand", Musee::ArtMaking::StoneTests::Flags)

bool FArtStoneStandTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	const TValueOrError<FArtStones, FString> Stones = Musee::ReadRecord<FArtStones>(FArtStones::Record);
	if (!TestTrue(TEXT("The stones' record reads"), Stones.HasValue()))
	{
		AddError(Stones.GetError());
		return false;
	}
	const FArtStoneStand& Record = Stones.GetValue().Stand;
	for (const FString& Fault : StoneStandFaults(Record))
	{
		AddError(FString::Printf(TEXT("The stand: %s"), *Fault));
	}
	const TValueOrError<FArtHang, FString> Hang = Musee::ReadRecord<FArtHang>(TEXT("Stages/chenghuai/Hang.json"));
	const FArtHangEntry* Entry = Hang.HasValue()
		? Hang.GetValue().Works.FindByPredicate([](const FArtHangEntry& Each) { return Each.Work == TEXT("chenghuai-lingbi-rock"); }) : nullptr;
	if (!TestNotNull(TEXT("Chenghuai's plan stands the Lingbi rock"), Entry))
	{
		return false;
	}
	TestTrue(TEXT("on its stand, which Art carves"), Entry->Stand);
	const FStoneSpec Spec = StoneTests::SpecOf(Stones.GetValue(), TEXT("chenghuai-lingbi-rock"), Entry->SizeMeters().Y);
	FFormMesh Rock;
	BuildStone(Spec, Rock);
	const FStoneStandForm Stand = CarveStoneStand(Rock, StoneCellMeters(Spec), Record);
	if (!TestFalse(TEXT("Its stand is carved"), Stand.Mesh.IsEmpty()) || !TestEqual(TEXT("all round"), Stand.RecessRadiiMeters.Num(), StandAngles))
	{
		return false;
	}
	TestEqual(TEXT("The rock sits down into the recess: its foot on the recess's floor"), Stand.RecessFloorMeters, Record.HeightMeters - Record.RecessMeters, 1e-9);

	// No part of the rock within the recess's depth reaches the recess's wall at its angle.
	int32 Through = 0;
	for (const FVector& Point : Rock.AllPositions())
	{
		if (Point.Z <= Record.RecessMeters)
		{
			const double Angle = FMath::Atan2(Point.Y, Point.X);
			const int32 Index = (FMath::RoundToInt(Angle / (2.0 * UE_DOUBLE_PI) * StandAngles) % StandAngles + StandAngles) % StandAngles;
			Through += FVector2D(Point.X, Point.Y).Size() >= Stand.RecessRadiiMeters[Index] ? 1 : 0;
		}
	}
	TestEqual(TEXT("and no part of it passes through the recess's wall"), Through, 0);

	// Carved to the rock: its plan as long and narrow as the rock's foot, with an even lip; a turned disc is as wide every way.
	double Longest = 0.0;
	double Shortest = TNumericLimits<double>::Max();
	bool bEvenLip = true;
	for (int32 Index = 0; Index < StandAngles; ++Index)
	{
		Longest = FMath::Max(Longest, Stand.OutlineRadiiMeters[Index]);
		Shortest = FMath::Min(Shortest, Stand.OutlineRadiiMeters[Index]);
		bEvenLip &= FMath::IsNearlyEqual(Stand.OutlineRadiiMeters[Index] - Stand.RecessRadiiMeters[Index], Record.LipMeters, 1e-9);
	}
	TestTrue(*FString::Printf(TEXT("Its plan follows the rock's foot, not a disc (%.3f by %.3f)"), 2.0 * Longest, 2.0 * Shortest),
		Longest > 1.3 * Shortest);
	TestTrue(TEXT("with an even lip round the recess"), bEvenLip);

	// It stands on its feet: they reach the table, and its side, all round, stands their height clear of it.
	int32 OnGround = 0;
	int32 BodyLow = 0;
	int32 Below = 0;
	for (const FVector& Point : Stand.Mesh.AllPositions())
	{
		if (Point.Z < Record.FootHeightMeters - UE_KINDA_SMALL_NUMBER)
		{
			OnGround += Point.Z < UE_KINDA_SMALL_NUMBER ? 1 : 0;
			const double Angle = FMath::Atan2(Point.Y, Point.X);
			const int32 Index = (FMath::RoundToInt(Angle / (2.0 * UE_DOUBLE_PI) * StandAngles) % StandAngles + StandAngles) % StandAngles;
			BodyLow += FVector2D(Point.X, Point.Y).Size() >= Stand.OutlineRadiiMeters[Index] - 1e-4 ? 1 : 0;
		}
		Below += Point.Z < -UE_KINDA_SMALL_NUMBER ? 1 : 0;
	}
	TestEqual(TEXT("Nothing of it is below the table"), Below, 0);
	TestTrue(TEXT("It stands on the table on its feet"), OnGround > 0);
	TestEqual(TEXT("which stand in from its side, its side clear of the table by their height"), BodyLow, 0);
	return true;
}

#endif
