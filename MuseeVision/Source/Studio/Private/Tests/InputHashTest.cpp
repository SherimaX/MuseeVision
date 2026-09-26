#include "Make/InputHash.h"
#include "Misc/AutomationTest.h"
#include "StaticMeshAttributes.h"
#include "Tests/ProofPieces.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioInputHashTest, "Musee.Studio.InputHash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStudioInputHashTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Studio;

	const auto HashOfColumn = [](double HeightMeters, const TCHAR* Material)
	{
		FMeshDescription Column = TurnColumn(HeightMeters);
		return FInputHasher().Add(TEXT("column")).Add(Material).Add(Column).Finish();
	};
	const FString First = HashOfColumn(2.8, TEXT("/Engine/EngineMaterials/WorldGridMaterial"));
	TestEqual(TEXT("a hash is 64 hex digits"), First.Len(), 64);
	TestEqual(TEXT("the same drawing hashes the same"), HashOfColumn(2.8, TEXT("/Engine/EngineMaterials/WorldGridMaterial")), First);
	TestNotEqual(TEXT("a taller column hashes apart"), HashOfColumn(2.81, TEXT("/Engine/EngineMaterials/WorldGridMaterial")), First);
	TestNotEqual(TEXT("another material hashes apart"), HashOfColumn(2.8, TEXT("/Engine/EngineMaterials/DefaultMaterial")), First);

	FMeshDescription Nudged = TurnColumn(2.8);
	FStaticMeshAttributes Attributes(Nudged);
	Attributes.GetVertexPositions()[FVertexID(0)].Z += 0.01f;
	const FString NudgedHash = FInputHasher().Add(TEXT("column")).Add(TEXT("/Engine/EngineMaterials/WorldGridMaterial")).Add(Nudged).Finish();
	TestNotEqual(TEXT("a vertex moved by a tenth of a millimetre hashes apart"), NudgedHash, First);

	TestNotEqual(TEXT("text boundaries count"), FInputHasher().Add(TEXT("ab")).Add(TEXT("c")).Finish(),
		FInputHasher().Add(TEXT("a")).Add(TEXT("bc")).Finish());
	return true;
}

#endif
