#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "Growth/Plant.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "MuseeUnits.h"
#include "NaturePieces.h"
#include "NaturePlanting.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::NatureMaking::Tests
{
	/** Nature's tests run in the editor, where its making twin lives, and belong to the product's own filter. */
	inline constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	FString Fixture(const FString& Name)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir() / TEXT("Systems/Nature/MuseeNatureMaking/Private/Tests/Data") / Name);
	}

	FMuseeStageAnchor Anchor(const TCHAR* Key, const TCHAR* Kind, const FVector& At)
	{
		FMuseeStageAnchor Out;
		Out.Key = FMuseeKey(Key);
		Out.Kind = Kind;
		Out.Frame = FMuseeFrame(At);
		return Out;
	}

	/** The test pond: its basin 10 m east of the stage's origin, the water 0.38 m up, the east shelf 0.12 m under it. */
	FMuseeStageContext Pond()
	{
		FMuseeStageContext Stage;
		Stage.Stage = FMuseeKey(TEXTVIEW("pond"));
		Stage.ScheduleDir = Fixture(TEXT("Stages/pond"));
		Stage.Anchors = {
			Anchor(TEXT("pond.basin"), TEXT("Body"), FVector(10.0, 0.0, 0.0)),
			Anchor(TEXT("pond.datum"), TEXT("Water"), FVector(10.0, 0.0, 0.38)),
			Anchor(TEXT("pond.lilies"), TEXT("Bed"), FVector(10.0, 0.0, 0.06)),
			Anchor(TEXT("pond.shelf.east"), TEXT("Bed"), FVector(13.45, 0.0, 0.26)),
		};
		return Stage;
	}

	/** The area of a mesh's triangles, in square metres. */
	double AreaSquareMeters(const UE::Geometry::FDynamicMesh3& Mesh)
	{
		double Area = 0.0;
		for (const int32 T : Mesh.TriangleIndicesItr())
		{
			FVector A, B, C;
			Mesh.GetTriVertices(T, A, B, C);
			Area += 0.5 * FVector::CrossProduct(B - A, C - A).Size();
		}
		return Area / FMath::Square(ToEngine(Meters(1.0)));
	}

	const FNatureSpecies* FindSpecies(const TArray<FNatureSpecies>& All, const TCHAR* Key)
	{
		return All.FindByPredicate([Key](const FNatureSpecies& S) { return S.Key == FMuseeKey(Key); });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNatureSpeciesTest, "Musee.Nature.Species", Musee::NatureMaking::Tests::Flags)

bool FNatureSpeciesTest::RunTest(const FString& Parameters)
{
	using namespace Musee::NatureMaking;
	const TValueOrError<TArray<FNatureSpecies>, FString> Species = ReadSpecies();
	if (!TestTrue(TEXT("The species records read"), Species.HasValue()))
	{
		AddError(Species.GetError());
		return false;
	}
	TestTrue(TEXT("The oval's five species and the contract's example are recorded"), Species.GetValue().Num() >= 6);
	for (const FNatureSpecies& One : Species.GetValue())
	{
		const TArray<FString> Faults = SpeciesFaults(One);
		TestTrue(FString::Printf(TEXT("%s is complete for its form%s%s"), *One.Name, Faults.IsEmpty() ? TEXT("") : TEXT(": "), *FString::Join(Faults, TEXT("; "))), Faults.IsEmpty());
	}
	FNatureSpecies Broken = Species.GetValue()[0];
	Broken.Form = TEXT("cactus");
	TestFalse(TEXT("An unknown form is a fault"), SpeciesFaults(Broken).IsEmpty());
	Broken = Species.GetValue()[0];
	Broken.Organs.Reset();
	TestFalse(TEXT("An organ without its tissue is a fault"), SpeciesFaults(Broken).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNatureWaterTest, "Musee.Nature.Water", Musee::NatureMaking::Tests::Flags)

bool FNatureWaterTest::RunTest(const FString& Parameters)
{
	using namespace Musee::NatureMaking;
	const FMuseeStageContext Stage = Tests::Pond();
	FNatureWater Water;
	Water.Key = FMuseeKey(TEXTVIEW("pond.water"));
	Water.Anchor = FMuseeKey(TEXTVIEW("pond.datum"));
	Water.Body = FMuseeKey(TEXTVIEW("pond.basin"));
	Water.Outline.Ellipse = { 3.9, 2.0 };
	Water.Depth = 0.32f;
	Water.Material = TEXT("water.still");
	FFormMadePiece Piece;
	FString Why;
	if (!TestTrue(TEXT("The water is made"), MakeWaterPiece(Stage, Water, Piece, Why)))
	{
		AddError(Why);
		return false;
	}
	TestEqual(TEXT("Its area is the ellipse's, pi a b, within the chord tolerance"), Tests::AreaSquareMeters(Piece.Mesh), UE_DOUBLE_PI * 3.9 * 2.0, 0.01);
	TestTrue(TEXT("It rides on the basin, at the datum seen from the basin"), Piece.Body == Water.Body && Piece.Frame.OriginMeters.Equals(FVector(0.0, 0.0, 0.38), 1e-9));
	TestTrue(TEXT("It brings its own material, isn't Nanite and collides with nothing"), Piece.Material == Water.Material && !Piece.bNanite && Piece.Collision.IsNone());
	const UE::Geometry::FDynamicMeshAttributeSet* Attributes = Piece.Mesh->Attributes();
	TestTrue(TEXT("It carries the kernel's layers: normal, tangent and bitangent; UV0 and the piece layout; colours"),
		Attributes && Attributes->NumNormalLayers() == 3 && Attributes->NumUVLayers() == 2 && Attributes->HasPrimaryColors() && Attributes->HasMaterialID());

	Water.Anchor = FMuseeKey(TEXTVIEW("pond.lilies"));
	TestFalse(TEXT("A datum that isn't a Water anchor is refused"), MakeWaterPiece(Stage, Water, Piece, Why));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNatureGrowthTest, "Musee.Nature.Growth", Musee::NatureMaking::Tests::Flags)

bool FNatureGrowthTest::RunTest(const FString& Parameters)
{
	using namespace Musee::NatureMaking;
	const TValueOrError<TArray<FNatureSpecies>, FString> Species = ReadSpecies();
	if (!TestTrue(TEXT("The species records read"), Species.HasValue()))
	{
		return false;
	}
	FPlantSite Site;
	Site.WaterLevel = 0.32;
	Site.IsClear = [](const FVector& Point, double Margin) { return Point.Z > 0.4 || FMath::Square(Point.X / (3.9 - Margin)) + FMath::Square(Point.Y / (2.0 - Margin)) <= 1.0; };
	for (const FNatureSpecies& One : Species.GetValue())
	{
		const FForm* Form = FindForm(One.Form);
		FPlantParts First, Second;
		FRandomStream A(Musee::Seed(FMuseeKey(TEXTVIEW("test.plant"))));
		FRandomStream B(Musee::Seed(FMuseeKey(TEXTVIEW("test.plant"))));
		Form->Grow(One, Site, A, First);
		Form->Grow(One, Site, B, Second);
		const FNatureSurface* Leaves = First.Find(Organ::Leaf);
		TestTrue(FString::Printf(TEXT("%s grows leaves"), *One.Name), Leaves && !Leaves->IsEmpty());
		bool bSame = First.Num() == Second.Num();
		for (const TPair<FName, FNatureSurface>& Part : First)
		{
			const FNatureSurface* Other = Second.Find(Part.Key);
			bSame &= Other && Other->NumVertices() == Part.Value.NumVertices() && TArray<FVector>(Other->AllPositions()) == TArray<FVector>(Part.Value.AllPositions());
		}
		TestTrue(FString::Printf(TEXT("%s grows the same from the same stream"), *One.Name), bSame);
		if (Form->bInWater && Leaves)
		{
			const bool bFloating = !Leaves->AllPositions().ContainsByPredicate([](const FVector& P) { return FMath::Abs(P.Z - 0.32) > 0.05; });
			TestTrue(FString::Printf(TEXT("%s's pads float on the water"), *One.Name), bFloating);
			TestTrue(FString::Printf(TEXT("%s's pads keep inside the rim"), *One.Name), Site.IsClearOf(*Leaves, 0.0));
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNaturePlantingTest, "Musee.Nature.Planting", Musee::NatureMaking::Tests::Flags)

bool FNaturePlantingTest::RunTest(const FString& Parameters)
{
	using namespace Musee::NatureMaking;
	const FMuseeStageContext Stage = Tests::Pond();
	TArray<FFormMadePiece> Pieces;
	TArray<FMuseeFinding> Findings;
	GatherNature(Stage, Pieces, Findings);
	for (const FMuseeFinding& Found : Findings)
	{
		AddError(Found.ToString());
	}
	TestEqual(TEXT("The pond's plan grows without findings"), Findings.Num(), 0);
	const FFormMadePiece* Water = Pieces.FindByPredicate([](const FFormMadePiece& P) { return P.Name == TEXT("pond.water.surface"); });
	TestNotNull(TEXT("Its water is a piece"), Water);
	TSet<FName> Names;
	for (const FFormMadePiece& Piece : Pieces)
	{
		bool bAlreadyNamed = false;
		Names.Add(Piece.Name, &bAlreadyNamed);
		TestFalse(FString::Printf(TEXT("%s is named once"), *Piece.Name.ToString()), bAlreadyNamed);
		TestTrue(FString::Printf(TEXT("%s rides on the basin"), *Piece.Name.ToString()), Piece.Body == FMuseeKey(TEXTVIEW("pond.basin")));
		TestFalse(FString::Printf(TEXT("%s brings its own material"), *Piece.Name.ToString()), Piece.Material.IsNone());
		TestTrue(FString::Printf(TEXT("%s has a surface"), *Piece.Name.ToString()), Piece.Mesh->TriangleCount() > 0);
	}
	for (const TCHAR* Expected : { TEXT("pond.lilies.nymphaea-marliacea-carnea.leaf"), TEXT("pond.lilies.nymphaea-marliacea-albida.leaf"),
		TEXT("pond.shelf.east.iris-laevigata.leaf"), TEXT("pond.shelf.east.carex-elata.seed"), TEXT("pond.shelf.east.myosotis-scorpioides.flower") })
	{
		TestTrue(FString::Printf(TEXT("%s is grown"), Expected), Names.Contains(FName(Expected)));
	}
	const FFormMadePiece* Pads = Pieces.FindByPredicate([](const FFormMadePiece& P) { return P.Name == TEXT("pond.lilies.nymphaea-marliacea-carnea.leaf"); });
	if (Pads)
	{
		// The pads, from the bed 0.32 m under the water, float inside the water's outline.
		bool bInside = true;
		for (const int32 V : Pads->Mesh->VertexIndicesItr())
		{
			const FVector P = Musee::FromEngine(Pads->Mesh->GetVertex(V)) + Pads->Frame.OriginMeters - FVector(0.0, 0.0, 0.38);
			bInside &= FMath::Square(P.X / 3.9) + FMath::Square(P.Y / 2.0) <= 1.0;
		}
		TestTrue(TEXT("Every pad keeps inside the water"), bInside);
	}
	TArray<FFormMadePiece> Again;
	TArray<FMuseeFinding> AgainFindings;
	GatherNature(Stage, Again, AgainFindings);
	bool bSame = Again.Num() == Pieces.Num();
	for (int32 I = 0; bSame && I < Pieces.Num(); ++I)
	{
		bSame &= Again[I].Name == Pieces[I].Name && Again[I].Mesh->VertexCount() == Pieces[I].Mesh->VertexCount()
			&& (Pieces[I].Mesh->VertexCount() == 0 || Again[I].Mesh->GetVertex(0) == Pieces[I].Mesh->GetVertex(0));
	}
	TestTrue(TEXT("A second make grows the same planting"), bSame);
	return true;
}

#endif
