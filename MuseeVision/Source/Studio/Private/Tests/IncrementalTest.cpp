#include "Make/Make.h"
#include "Misc/AutomationTest.h"
#include "Tests/ProofPlan.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::Studio
{
	namespace
	{
		FMakeReport MakeProof(double ColumnHeightMeters)
		{
			FMakePlan Plan = ProofPlan(FProofShape{ColumnHeightMeters});
			return Make(Plan);
		}

		TArray<FString> Written(const FMakeReport& Report)
		{
			TArray<FString> Names;
			for (const FMadePackage& Package : Report.Packages)
			{
				if (Package.Outcome == EMadeOutcome::Written)
				{
					Names.Add(Package.PackageName);
				}
			}
			return Names;
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioIncrementalTest, "Musee.Studio.Incremental",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStudioIncrementalTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Studio;

	const double Height = FProofShape().ColumnHeightMeters;
	const FString Column = ProofPaths().StageMesh(TEXT("Proof"), TEXT("Column"));

	TestTrue(TEXT("the proof is made"), MakeProof(Height).Succeeded());
	const FMakeReport Again = MakeProof(Height);
	TestEqual(TEXT("a second make writes nothing"), Again.Count(EMadeOutcome::Written), 0);
	TestEqual(TEXT("a second make finds all five packages unchanged"), Again.Count(EMadeOutcome::Unchanged), 5);

	// A taller column changes one drawing: exactly its mesh is recast. The levels only name the mesh, so they stay.
	const FMakeReport Taller = MakeProof(Height + 0.2);
	TestEqual(TEXT("a changed column recasts only the column"), Written(Taller), TArray<FString>{Column});

	const FMakeReport Restored = MakeProof(Height);
	TestEqual(TEXT("restoring the column recasts it again"), Written(Restored), TArray<FString>{Column});
	TestTrue(TEXT("every make succeeds"), Taller.Succeeded() && Restored.Succeeded());
	AddInfo(FString::Printf(TEXT("Second make: %d written, %d unchanged. Taller column: %d written (%s)."),
		Again.Count(EMadeOutcome::Written), Again.Count(EMadeOutcome::Unchanged), Taller.Count(EMadeOutcome::Written),
		*FString::Join(Written(Taller), TEXT(", "))));
	return true;
}

#endif
