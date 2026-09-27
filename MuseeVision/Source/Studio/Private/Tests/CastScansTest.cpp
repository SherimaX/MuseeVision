#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/StaticMesh.h"
#include "MatterNames.h"
#include "MeshDescription.h"
#include "Misc/Paths.h"
#include "Stages/CastScans.h"
#include "StaticMeshAttributes.h"

/**
 * A scan comes in through the engine's own glTF import as a static mesh at its package, turned into the engine's axes as
 * Art reads it (glTF's up is z, its front +y, its right x; centimetres), and a second make rewrites nothing.
 */
namespace Musee::Studio::Tests
{
	namespace
	{
		/** The specimens are made under their own root, so they never touch the museum's assets. */
		const TCHAR* const ScanRoot = TEXT("/Game/Generated/Specimens");

		/** Art's test scan: a tetrahedron on y = 0 in glTF's axes, its front point at z 0.1, its apex 0.3 up. */
		FArtCastScan Wedge()
		{
			return { TEXT("wedge"), FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir() / TEXT("Systems/Art/MuseeArtMaking/Private/Tests/Data/scans/wedge.glb")) };
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioCastScansTest, "Musee.Studio.CastScans", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	bool FStudioCastScansTest::RunTest(const FString& Parameters)
	{
		const FString Package = Musee::ArtMaking::ScanPackage(ScanRoot, Wedge().Scan);
		// A scan is geometry alone, as the collection's are: an import (not a make that finds it unchanged) says so, and
		// imports no material.
		AddExpectedMessage(TEXT("Mesh has primitives with no materials assigned"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, -1, false);
		FMakeReport First;
		ImportScan(Wedge(), Package, First);
		if (!TestEqual(TEXT("one package"), First.Packages.Num(), 1) || !TestTrue(FString::Printf(TEXT("the scan is imported (%s)"), *First.Packages[0].Why),
			First.Packages[0].Outcome != EMadeOutcome::Failed))
		{
			return false;
		}
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Musee::Matter::ObjectPathOf(Package));
		const FMeshDescription* Description = Mesh ? Mesh->GetMeshDescription(0) : nullptr;
		if (!TestNotNull(TEXT("as a static mesh keeping its description"), Description))
		{
			return false;
		}
		FBox Box(ForceInit);
		FVector3f Front = FVector3f::ZeroVector;
		const TVertexAttributesConstRef<FVector3f> Positions = FStaticMeshConstAttributes(*Description).GetVertexPositions();
		for (const FVertexID Vertex : Description->Vertices().GetElementIDs())
		{
			Box += FVector(Positions[Vertex]);
			Front = Positions[Vertex].Y > Front.Y ? Positions[Vertex] : Front;
		}
		TestEqual(TEXT("its four points and four triangles"), Description->Vertices().Num() * 10 + Description->Triangles().Num(), 44);
		TestTrue(TEXT("its apex stands 30 cm up z"), FMath::IsNearlyEqual(Box.Max.Z, 30.0, 1e-3) && FMath::IsNearlyEqual(Box.Min.Z, 0.0, 1e-3));
		TestTrue(TEXT("its front point lies 10 cm out along +y, on its base"), Front.Equals(FVector3f(0.0f, 10.0f, 0.0f), 1e-3f));
		TestTrue(TEXT("its right stays x"), FMath::IsNearlyEqual(Box.Max.X, 10.0, 1e-3));

		FMakeReport Second;
		ImportScan(Wedge(), Package, Second);
		TestTrue(TEXT("a second make rewrites nothing"), Second.Packages.Num() == 1 && Second.Packages[0].Outcome == EMadeOutcome::Unchanged);
		return true;
	}
}

#endif
