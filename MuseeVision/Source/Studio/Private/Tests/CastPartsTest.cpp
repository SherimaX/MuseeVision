#include "Cast/CastParts.h"
#include "Misc/AutomationTest.h"
#include "StaticMeshAttributes.h"
#include "Tests/ProofPieces.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioCastPartsTest, "Musee.Studio.CastParts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FStudioCastPartsTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Studio;

	const FMeshDescription Wall = BuildGlazedWall();
	const TArray<FCastSlot> Slots = {{TEXT("Stone"), FSoftObjectPath()}, {TEXT("Glass"), FSoftObjectPath()}};
	const TArray<ECastKind> Kinds = {ECastKind::Opaque, ECastKind::Translucent};
	const int32 StoneTriangles = Wall.GetPolygonGroupTriangles(FPolygonGroupID(0)).Num();
	const int32 GlassTriangles = Wall.GetPolygonGroupTriangles(FPolygonGroupID(1)).Num();

	TArray<FCastPart> Parts = SplitByKind(Wall, Slots, Kinds);
	if (!TestEqual(TEXT("a stone-and-glass cast splits in two"), Parts.Num(), 2))
	{
		return false;
	}
	const FCastPart& Stone = Parts[0];
	const FCastPart& Glass = Parts[1];
	TestTrue(TEXT("the opaque part comes first"), Stone.Kind == ECastKind::Opaque && Glass.Kind == ECastKind::Translucent);
	TestEqual(TEXT("the stone keeps its triangles"), Stone.Description.Triangles().Num(), StoneTriangles);
	TestEqual(TEXT("the pane keeps its 12 triangles"), Glass.Description.Triangles().Num(), GlassTriangles);
	TestEqual(TEXT("the pane is a closed box of 8 corners"), Glass.Description.Vertices().Num(), 8);
	TestEqual(TEXT("each part has one group"), Stone.Description.PolygonGroups().Num() + Glass.Description.PolygonGroups().Num(), 2);
	TestEqual(TEXT("no vertex instance is lost or made"),
		Stone.Description.VertexInstances().Num() + Glass.Description.VertexInstances().Num(), Wall.VertexInstances().Num());

	const FStaticMeshConstAttributes GlassAttributes(Glass.Description);
	TestEqual(TEXT("the pane's group is named for its slot"), GlassAttributes.GetPolygonGroupMaterialSlotNames()[FPolygonGroupID(0)], FName(TEXT("Glass")));
	TestEqual(TEXT("the pane's slot travels with it"), Glass.Slots[0].Name, FName(TEXT("Glass")));
	bool bAxisNormals = true;
	for (const FVertexInstanceID Instance : Glass.Description.VertexInstances().GetElementIDs())
	{
		const FVector3f Normal = GlassAttributes.GetVertexInstanceNormals()[Instance];
		bAxisNormals &= FMath::IsNearlyEqual(FMath::Abs(Normal.X) + FMath::Abs(Normal.Y) + FMath::Abs(Normal.Z), 1.0f, 1.0e-6f);
	}
	TestTrue(TEXT("the pane's drawn face normals are carried unchanged"), bAxisNormals);

	const FMeshDescription Column = TurnColumn(2.8);
	TArray<FCastPart> Whole = SplitByKind(Column, {{TEXT("Stone"), FSoftObjectPath()}}, {ECastKind::Opaque});
	TestEqual(TEXT("a cast of one kind stays whole"), Whole.Num(), 1);
	TestEqual(TEXT("a whole cast keeps every triangle"), Whole[0].Description.Triangles().Num(), Column.Triangles().Num());
	return true;
}

#endif
