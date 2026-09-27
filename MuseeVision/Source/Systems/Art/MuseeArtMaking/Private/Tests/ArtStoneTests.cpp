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

#endif
