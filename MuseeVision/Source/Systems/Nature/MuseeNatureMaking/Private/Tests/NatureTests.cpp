#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "Growth/Plant.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "MuseeCollision.h"
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
			Anchor(TEXT("pond.golden-iris"), TEXT("Bed"), FVector(5.85, 0.0, 0.40)),
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
	TestTrue(TEXT("The oval's five species, the contract's example, and the gardens' and the pergola's twenty-three are recorded"), Species.GetValue().Num() >= 29);
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
	Water.Working = TEXT("water.dyed-black");
	Water.History = TEXT("clean");
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
	TestTrue(TEXT("Its material is a full spec"), Piece.Working == Water.Working && Piece.History == Water.History);
	const UE::Geometry::FDynamicMeshAttributeSet* Attributes = Piece.Mesh->Attributes();
	TestTrue(TEXT("It carries the kernel's layers: normal, tangent and bitangent; UV0 and the piece layout; colours"),
		Attributes && Attributes->NumNormalLayers() == 3 && Attributes->NumUVLayers() == 2 && Attributes->HasPrimaryColors() && Attributes->HasMaterialID());

	// A rill of water 2 m by 1 m with a notch cut from one corner: an L of 1.75 m^2.
	FNatureWater Rill = Water;
	Rill.Outline.Ellipse.Empty();
	Rill.Outline.Polygon = { FVector2D(-1.0, -0.5), FVector2D(1.0, -0.5), FVector2D(1.0, 0.0), FVector2D(0.5, 0.0), FVector2D(0.5, 0.5), FVector2D(-1.0, 0.5) };
	TestEqual(TEXT("A polygon's area is its shoelace sum"), Rill.Outline.Area(), 1.75, 1e-12);
	TestTrue(TEXT("A polygon holds a point inside it"), Rill.Outline.Contains(FVector2D(0.0, 0.0), 0.2));
	TestFalse(TEXT("but not one in its notch"), Rill.Outline.Contains(FVector2D(0.75, 0.25), 0.0));
	TestFalse(TEXT("nor one nearer its edge than the margin"), Rill.Outline.Contains(FVector2D(0.0, 0.4), 0.2));
	FFormMadePiece Polygon;
	if (TestTrue(TEXT("A polygon of water is made"), MakeWaterPiece(Stage, Rill, Polygon, Why)))
	{
		TestEqual(TEXT("Its surface is the polygon's"), Tests::AreaSquareMeters(Polygon.Mesh), 1.75, 1e-6);
		TestTrue(TEXT("and it is the water's piece"), Polygon.Name == Piece.Name && Polygon.Material == Water.Material && Polygon.Frame.OriginMeters.Equals(Piece.Frame.OriginMeters));
	}
	else
	{
		AddError(Why);
	}

	Water.History = NAME_None;
	TestFalse(TEXT("Water without its history is refused"), MakeWaterPiece(Stage, Water, Piece, Why));
	Water.History = TEXT("clean");
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
	Site.WaterDepth = 0.45;
	Site.IsClear = [](const FVector& Point, double Margin) { return Point.Z > 0.4 || FMath::Square(Point.X / (3.9 - Margin)) + FMath::Square(Point.Y / (2.0 - Margin)) <= 1.0; };
	// What a garden's beds give the forms planted along a line, over a region, or trained: a metre of hedge, a square of
	// sward, a vine up a post and out along a bar.
	Site.Line = { FVector(-0.5, 0.0, 0.0), FVector(0.5, 0.0, 0.0) };
	Site.Clip.Width = 0.45f;
	Site.Clip.Height = 1.5f;
	Site.Clip.Batter = 0.02f;
	Site.Clip.Shoulder = 0.06f;
	Site.Clip.CoreInset = 0.065f;
	Site.Clip.Ends = TEXT("open");
	Site.Region = { FVector2D(-1.0, -1.0), FVector2D(1.0, -1.0), FVector2D(1.0, 1.0), FVector2D(-1.0, 1.0) };
	Site.Trained = { { FVector(0.0, 0.0, 0.0), FVector(0.0, 0.0, 1.8) }, { FVector(0.0, 0.0, 1.8), FVector(1.5, 0.0, 1.8) } };
	for (const FNatureSpecies& One : Species.GetValue())
	{
		const FForm* Form = FindForm(One.Form);
		if (!Form->Grow)
		{
			// A carpet is laid over its region by the planting (Musee.Nature.Garden), not grown plant by plant.
			continue;
		}
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
		if (Form->bFloating && Leaves)
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
	for (const FFormMadePiece& Piece : Pieces)
	{
		const bool bCast = Piece.Name.ToString().StartsWith(TEXT("pond.golden-iris."));
		const bool bWater = Piece.Name == TEXT("pond.water.surface");
		if (bCast)
		{
			TestTrue(FString::Printf(TEXT("%s is cast in gilt bronze, and solid"), *Piece.Name.ToString()), Piece.Material == TEXT("bronze.cusn10")
				&& Piece.Working == TEXT("gilt") && Piece.History == TEXT("handled") && Piece.Collision == Musee::Collision::Solid.Profile());
		}
		else if (!bWater)
		{
			TestTrue(FString::Printf(TEXT("%s is living tissue, as the bed works and ages it"), *Piece.Name.ToString()), Piece.Working == TEXT("living")
				&& Piece.History == TEXT("in-season") && Piece.Collision.IsNone() && Piece.Material.ToString().StartsWith(TEXT("living.")));
		}
	}
	TestTrue(TEXT("The golden iris grows the iris's leaves and flowers"), Names.Contains(TEXT("pond.golden-iris.iris-laevigata.leaf"))
		&& Names.Contains(TEXT("pond.golden-iris.iris-laevigata.flower")));
	const auto MaterialOf = [&Pieces](const TCHAR* Name)
	{
		const FFormMadePiece* Found = Pieces.FindByPredicate([Name](const FFormMadePiece& P) { return P.Name == Name; });
		return Found ? Found->Material : NAME_None;
	};
	TestTrue(TEXT("Each organ is its own tissue: a lily's pad a floating leaf, an iris's swords leaf, a forget-me-not's flowers petal"),
		MaterialOf(TEXT("pond.lilies.nymphaea-marliacea-carnea.leaf")) == TEXT("living.leaf-floating")
		&& MaterialOf(TEXT("pond.shelf.east.iris-laevigata.leaf")) == TEXT("living.leaf")
		&& MaterialOf(TEXT("pond.shelf.east.myosotis-scorpioides.flower")) == TEXT("living.petal"));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FNatureGardenTest, "Musee.Nature.Garden", Musee::NatureMaking::Tests::Flags)

bool FNatureGardenTest::RunTest(const FString& Parameters)
{
	using namespace Musee::NatureMaking;
	FMuseeStageContext Stage;
	Stage.Stage = FMuseeKey(TEXTVIEW("garden"));
	Stage.ScheduleDir = Tests::Fixture(TEXT("Stages/garden"));
	Stage.Anchors = {
		Tests::Anchor(TEXT("garden.pool"), TEXT("Water"), FVector(4.0, 4.0, -0.1)),
		Tests::Anchor(TEXT("garden.ground"), TEXT("Bed"), FVector(5.0, 5.0, 0.02)),
		Tests::Anchor(TEXT("garden.birch"), TEXT("Bed"), FVector(8.0, 2.0, 0.02)),
		Tests::Anchor(TEXT("garden.bamboo"), TEXT("Bed"), FVector(1.5, 8.5, 0.02)),
		Tests::Anchor(TEXT("garden.hedge"), TEXT("Bed"), FVector(5.0, 10.2, 0.0)),
		Tests::Anchor(TEXT("garden.sward"), TEXT("Bed"), FVector(12.5, 1.0, 0.0)),
		Tests::Anchor(TEXT("garden.planter"), TEXT("Bed"), FVector(12.0, 8.0, 0.5)),
		Tests::Anchor(TEXT("garden.stray"), TEXT("Bed"), FVector(1.0, 1.0, 0.02)),
	};
	TArray<FFormMadePiece> Pieces;
	TArray<FMuseeFinding> Findings;
	GatherNature(Stage, Pieces, Findings);
	const auto Named = [&Pieces](const TCHAR* Name) { return Pieces.FindByPredicate([Name](const FFormMadePiece& P) { return P.Name == Name; }); };
	TestEqual(TEXT("Only the stray bed, which counts three plants at two points, is a finding"), Findings.Num(), 1);
	TestTrue(TEXT("and it says so"), Findings.Num() == 1 && Findings[0].Summary.Contains(TEXT("counts 3 plants at 2 points")));

	const FFormMadePiece* Ground = Named(TEXT("garden.ground.ground"));
	if (TestNotNull(TEXT("The garden's ground is made"), Ground))
	{
		// A 10 m square of ground less the 1 m radius pool that lies in it.
		TestEqual(TEXT("over its outline less the water in it"), Tests::AreaSquareMeters(Ground->Mesh), 100.0 - UE_DOUBLE_PI, 0.01);
		TestTrue(TEXT("in its soil's material spec, walkable"), Ground->Material == TEXT("accent.site-geology") && Ground->Working == TEXT("topsoil.bare")
			&& Ground->History == TEXT("weathered") && Ground->Collision == Musee::Collision::Walkable.Profile());
		TestTrue(TEXT("on no body: its body written empty is the none key"), Ground->Body.IsNone());
	}
	const FFormMadePiece* Carpet = Named(TEXT("garden.ground.haplocladium-microphyllum.leaf"));
	if (TestNotNull(TEXT("The moss is a carpet over the ground"), Carpet))
	{
		TestEqual(TEXT("over the same region"), Tests::AreaSquareMeters(Carpet->Mesh), 100.0 - UE_DOUBLE_PI, 0.01);
		TestTrue(TEXT("its shoots' length above the soil"), Carpet->Frame.OriginMeters.Z > 0.02 + 0.009 && Carpet->Frame.OriginMeters.Z < 0.02 + 0.016);
		TestTrue(TEXT("in its leaves' tissue"), Carpet->Material == TEXT("living.leaf") && Carpet->Collision.IsNone());
	}
	const FFormMadePiece* Trunk = Named(TEXT("garden.birch.betula-pendula.trunk"));
	if (TestNotNull(TEXT("The birch grows a trunk"), Trunk))
	{
		TestTrue(TEXT("which the visitor can't pass through"), Trunk->Collision == Musee::Collision::Solid.Profile());
		double Top = 0.0;
		for (const int32 V : Trunk->Mesh->VertexIndicesItr())
		{
			Top = FMath::Max(Top, Musee::FromEngine(Trunk->Mesh->GetVertex(V)).Z);
		}
		TestTrue(TEXT("up to the height its bed draws"), Top > 0.9 * 9.3 * 0.97 - 0.1 && Top < 9.3);
	}
	TestNotNull(TEXT("and its leaves"), Named(TEXT("garden.birch.betula-pendula.leaf")));
	TestNotNull(TEXT("The bamboo grows its culms at their points"), Named(TEXT("garden.bamboo.phyllostachys-aureosulcata.stem")));
	const FFormMadePiece* Core = Named(TEXT("garden.hedge.taxus-baccata.trunk"));
	TestTrue(TEXT("The hedge grows its solid core along its line"), Core && Core->Collision == Musee::Collision::Solid.Profile());
	TestNotNull(TEXT("and its skin of needles"), Named(TEXT("garden.hedge.taxus-baccata.leaf")));
	TestNotNull(TEXT("The sward grows its tufts over its region"), Named(TEXT("garden.sward.festuca-rubra.leaf")));
	const FFormMadePiece* Soil = Named(TEXT("garden.planter.ground"));
	TestTrue(TEXT("The planter's soil is made, potted"), Soil && Soil->Working == TEXT("topsoil.potted"));
	const FFormMadePiece* Vine = Named(TEXT("garden.planter.vitis-vinifera.leaf"));
	if (TestNotNull(TEXT("The vine grows its leaves"), Vine))
	{
		bool bHigh = true;
		for (const int32 V : Vine->Mesh->VertexIndicesItr())
		{
			bHigh &= Musee::FromEngine(Vine->Mesh->GetVertex(V)).Z > 1.2;
		}
		TestTrue(TEXT("along the canes it is trained on, up by the bar"), bHigh);
	}
	TestNotNull(TEXT("and its stem up the post"), Named(TEXT("garden.planter.vitis-vinifera.trunk")));
	return true;
}

#endif
