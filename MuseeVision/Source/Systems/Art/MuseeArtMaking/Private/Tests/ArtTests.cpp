#include "ArtCare.h"
#include "ArtHang.h"
#include "ArtHangEntry.h"
#include "ArtHangReading.h"
#include "ArtPaintLayer.h"
#include "ArtPieces.h"
#include "ArtWallPath.h"
#include "ArtWork.h"
#include "ArtWorkComponent.h"
#include "ArtWorkImage.h"
#include "Canvas/WallPath.h"
#include "Components/StaticMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "FormMake.h"
#include "Glass/GlassPane.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "MuseeRecords.h"
#include "MuseeUnits.h"
#include "UObject/Package.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::ArtMaking::Tests
{
	/** Art's making tests run in the editor, where its making twin lives, and belong to the product's own filter. */
	inline constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	/** The fixtures' folder, as the repository's root sees it (a hanging plan's `source` is relative to the root). */
	const TCHAR* const Fixtures = TEXT("MuseeVision/Source/Systems/Art/MuseeArtMaking/Private/Tests/Data");

	FString Fixture(const FString& Name)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir() / TEXT("Systems/Art/MuseeArtMaking/Private/Tests/Data") / Name);
	}

	FMuseeStageAnchor Anchor(const TCHAR* Key, const FVector& At, const FQuat& Turn = FQuat::Identity)
	{
		FMuseeStageAnchor Out;
		Out.Key = FMuseeKey(Key);
		Out.Kind = TEXT("Wall");
		Out.Frame = FMuseeFrame(At, Turn);
		return Out;
	}

	/** The test oval: its curved wall starting at the east door's north jamb, its room inside; a flat wall facing south. */
	FMuseeStageContext Oval(const TCHAR* Stage = TEXT("oval"))
	{
		FMuseeStageContext Context;
		Context.Stage = FMuseeKey(Stage);
		Context.ScheduleDir = Fixture(FString::Printf(TEXT("Stages/%s"), Stage));
		Context.Anchors = {
			Anchor(TEXT("oval.wall.willows"), FVector(10.778, -1.5, 0.0), FQuat(FVector::UpVector, UE_DOUBLE_HALF_PI)),
			Anchor(TEXT("oval.wall.flat"), FVector(0.0, -5.0, 0.0)),
			Anchor(TEXT("faulty.wall"), FVector(0.0, -5.0, 0.0)),
		};
		return Context;
	}

	FMuseeStageAnchor Anchor(const TCHAR* Key, const TCHAR* Kind, const FVector& At, const FQuat& Turn = FQuat::Identity)
	{
		FMuseeStageAnchor Out = Anchor(Key, At, Turn);
		Out.Kind = Kind;
		return Out;
	}

	/**
	 * The test band: two Glazing lights in a screen facing north into the court (as Albion's band: x to the viewer's
	 * right is -x, y towards the viewer is -y), and the wall a built work stands in.
	 */
	FMuseeStageContext Glazed()
	{
		FMuseeStageContext Context;
		Context.Stage = FMuseeKey(TEXT("glazed"));
		Context.ScheduleDir = Fixture(TEXT("Stages/glazed"));
		const FQuat FacingNorth(FVector::UpVector, UE_DOUBLE_PI);
		Context.Anchors = {
			Anchor(TEXT("glazed.band.t1"), TEXT("Glazing"), FVector(4.8, 18.3, 10.9), FacingNorth),
			Anchor(TEXT("glazed.band.t2"), TEXT("Glazing"), FVector(3.2, 18.3, 10.9), FacingNorth),
			Anchor(TEXT("glazed.wall"), FVector(0.0, -5.0, 0.0)),
		};
		return Context;
	}

	/**
	 * The test cabinet: a flat wall facing south where a print hangs on its mount, a reader facing south (its face's
	 * centre at 1.3 m, x to the viewer's right, y out of the face) where a card and a plate lie, and a case's bed.
	 */
	FMuseeStageContext Cabinet()
	{
		FMuseeStageContext Context;
		Context.Stage = FMuseeKey(TEXT("cabinet"));
		Context.ScheduleDir = Fixture(TEXT("Stages/cabinet"));
		Context.Anchors = {
			Anchor(TEXT("cabinet.wall"), FVector(0.0, -5.0, 0.0)),
			Anchor(TEXT("cabinet.reader"), TEXT("Mount"), FVector(2.0, 0.0, 1.3)),
			Anchor(TEXT("cabinet.bed"), TEXT("Mount"), FVector(4.0, 0.0, 0.55)),
		};
		return Context;
	}

	/** A piece's bounds in the stage, metres. */
	FBox StageBox(const FFormMadePiece& Piece)
	{
		FBox Box(ForceInit);
		for (const int32 V : Piece.Mesh->VertexIndicesItr())
		{
			Box += Piece.Frame.PointToParent(Musee::FromEngine(Piece.Mesh->GetVertex(V)));
		}
		return Box;
	}

	const FFormMadePiece* FindPiece(const TArray<FFormMadePiece>& Pieces, const TCHAR* Name)
	{
		return Pieces.FindByPredicate([Name](const FFormMadePiece& P) { return P.Name == Name; });
	}

	/** A stage actor with a root and a mesh component per named piece, tagged as Studio tags them. */
	AActor* StageActor(UWorld& World, const TArray<FString>& Pieces)
	{
		AActor* Actor = World.SpawnActor<AActor>();
		USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"));
		Actor->SetRootComponent(Root);
		Root->RegisterComponent();
		for (const FString& Piece : Pieces)
		{
			UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Actor, FName(Piece.Replace(TEXT("."), TEXT("_"))));
			Mesh->SetupAttachment(Root);
			Mesh->ComponentTags.Add(FName(FString::Printf(TEXT("musee.piece:%s"), *Piece)));
			Mesh->RegisterComponent();
		}
		return Actor;
	}

	UStaticMeshComponent* FindMesh(const AActor& Actor, const FString& Piece)
	{
		TArray<UStaticMeshComponent*> Meshes;
		Actor.GetComponents(Meshes);
		const FName Tag(FString::Printf(TEXT("musee.piece:%s"), *Piece));
		UStaticMeshComponent* const* Found = Meshes.FindByPredicate([Tag](const UStaticMeshComponent* M) { return M->ComponentHasTag(Tag); });
		return Found ? *Found : nullptr;
	}

	FArtHangEntry Entry(const TCHAR* Work, const TCHAR* Catalogue)
	{
		FArtHangEntry Out;
		Out.Work = Work;
		Out.Source = FString(Fixtures) / Catalogue;
		Out.Size = { 0.63, 0.48 };
		Out.Support = TEXT("canvas");
		Out.Care = TEXT("oil-and-tempera");
		return Out;
	}

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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtCatalogueTest, "Musee.Art.Catalogue", Musee::ArtMaking::Tests::Flags)

bool FArtCatalogueTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	const TValueOrError<FArtWork, FString> Willows = Musee::Art::ReadWork(Tests::Entry(TEXT("monet-willows-two-willows"), TEXT("collection.json")));
	if (!TestTrue(TEXT("A work reads from a collection.json-style catalogue"), Willows.HasValue()))
	{
		AddError(Willows.GetError());
		return false;
	}
	TestEqual(TEXT("Its title"), Willows.GetValue().Title, FString(TEXT("The Two Willows")));
	TestEqual(TEXT("Its date as the catalogue writes it"), Willows.GetValue().Date, FString(TEXT("1914–1926")));
	TestTrue(TEXT("Its image, found beside its catalogue"), Willows.GetValue().ImagePath.EndsWith(TEXT("salon/monet-willows-two-willows.png")));
	TestTrue(TEXT("Its label names the artist and where it is held"), Willows.GetValue().Caption().Body.ToString().StartsWith(TEXT("Claude Monet, 1914"))
		&& Willows.GetValue().Caption().Credit.ToString() == TEXT("Musée de l'Orangerie, Paris"));

	const TValueOrError<FArtWork, FString> Sunrise = Musee::Art::ReadWork(Tests::Entry(TEXT("monet-impression-sunrise"), TEXT("artworks.json")));
	TestTrue(TEXT("A work reads from a data/*.json-style catalogue, galleries, sculptures and nulls and all"), Sunrise.HasValue());
	if (Sunrise.HasValue())
	{
		TestTrue(TEXT("The high-resolution image is preferred"), Sunrise.GetValue().ImagePath.Contains(TEXT("paintings_hires")));
		TestEqual(TEXT("The hanging plan's size"), Sunrise.GetValue().SizeMeters, FVector2D(0.63, 0.48));
	}
	TestFalse(TEXT("An id the catalogue doesn't hold is refused"), Musee::Art::ReadWork(Tests::Entry(TEXT("monet-no-such-work"), TEXT("artworks.json"))).HasValue());
	TestEqual(TEXT("A work's paint layer is its own palette key"), Musee::Art::PaintMaterial(TEXT("monet-willows-two-willows")), FName(TEXT("paint.monet-willows-two-willows")));
	TestTrue(TEXT("An id that can't be one key segment names no paint"), Musee::Art::PaintMaterial(TEXT("Not A Key")).IsNone());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtWallPathTest, "Musee.Art.WallPath", Musee::ArtMaking::Tests::Flags)

bool FArtWallPathTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	const FMuseeStageContext Stage = Tests::Oval();
	FArtWallPath Path;
	Path.Ellipse = { 11.0, 7.5 };
	Path.From_Deg = -11.537f;
	Path.Direction = TEXT("decreasing");
	const FWallPath Wall(Stage.Anchors[0].Frame, Path);
	TestTrue(TEXT("The anchor stands on its path"), Wall.Faults().IsEmpty());
	// Ramanujan's second approximation to an ellipse's perimeter is good to parts in a billion at this eccentricity.
	const double H = FMath::Square((11.0 - 7.5) / (11.0 + 7.5));
	const double Perimeter = UE_DOUBLE_PI * (11.0 + 7.5) * (1.0 + 3.0 * H / (10.0 + FMath::Sqrt(4.0 - 3.0 * H)));
	TestEqual(TEXT("The wall's length is the ellipse's perimeter"), Wall.Length(), Perimeter, 0.001);
	const FWallPath::FStation Start = Wall.At(0.0);
	TestTrue(TEXT("It starts at the anchor"), Start.Point.Equals(FVector(10.778, -1.5, 0.0), 0.002));
	TestTrue(TEXT("It runs north first"), Start.Along.Y < -0.9);
	TestTrue(TEXT("The room is inside the oval"), FVector::DotProduct(Start.Out, -Start.Point) > 0.0);
	TestEqual(TEXT("Distance is arc length"), FVector::Dist(Wall.At(10.0).Point, Wall.At(10.05).Point), 0.05, 1e-4);
	TestTrue(TEXT("Stations keep within the chord tolerance"), Wall.StationSpacing(17.0) < 0.21);

	const FWallPath Flat(Stage.Anchors[1].Frame);
	TestTrue(TEXT("A straight wall runs along its anchor's x"), Flat.At(2.0).Point.Equals(FVector(2.0, -5.0, 0.0), 1e-9) && Flat.At(0.0).Out.Equals(FVector(0.0, 1.0, 0.0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtCanvasTest, "Musee.Art.Canvas", Musee::ArtMaking::Tests::Flags)

bool FArtCanvasTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	TArray<FFormMadePiece> Pieces;
	TArray<FMuseeFinding> Findings;
	GatherArt(Tests::Oval(), Pieces, Findings);
	for (const FMuseeFinding& Found : Findings)
	{
		AddError(Found.ToString());
	}
	if (!TestEqual(TEXT("Two works, each a face and its linen"), Pieces.Num(), 4))
	{
		return false;
	}
	const FFormMadePiece& Willows = Pieces[0];
	const FFormMadePiece& Sunrise = Pieces[2];
	TestEqual(TEXT("The face is named for the work"), Willows.Name, FName(TEXT("canvas.monet-willows-two-willows")));
	TestEqual(TEXT("It is painted in the work's own paint"), Willows.Material, FName(TEXT("paint.monet-willows-two-willows")));
	TestTrue(TEXT("with the paint's working and history, from its palette entry"), Willows.Working == TEXT("varnished") && Willows.History == TEXT("clean"));
	TestTrue(TEXT("Its tacking edges and back are linen, stretched and new"), Pieces[1].Material == TEXT("accent.linen") && Pieces[1].Working == TEXT("stretched")
		&& Pieces[1].History == TEXT("new"));
	TestEqual(TEXT("A flat canvas's face is its real size"), Tests::AreaSquareMeters(Sunrise.Mesh), 0.63 * 0.48, 1e-6);
	// The curved face stands 40 mm inside the wall, round the oval's west end, which turns it nearly 2 radians: 80 mm shorter.
	TestEqual(TEXT("A curved canvas's face is its size, less its offset inside the curve"), Tests::AreaSquareMeters(Willows.Mesh), 2.0 * 17.0, 0.2);
	const UE::Geometry::FDynamicMeshAttributeSet* Attributes = Willows.Mesh->Attributes();
	TestTrue(TEXT("It carries the kernel's layers"), Attributes && Attributes->NumNormalLayers() == 3 && Attributes->NumUVLayers() == 2);
	// UV0 is metres from the image's top left as the room sees it; its left edge is the far end of the run, which goes north then west.
	FVector2f Least(TNumericLimits<float>::Max()), Most(TNumericLimits<float>::Lowest());
	FVector LeftBottom = FVector::ZeroVector;
	for (const int32 E : Attributes->GetUVLayer(0)->ElementIndicesItr())
	{
		const FVector2f UV = Attributes->GetUVLayer(0)->GetElement(E);
		Least = FVector2f::Min(Least, UV);
		Most = FVector2f::Max(Most, UV);
	}
	TestTrue(TEXT("The image spans the face in metres"), Least.Equals(FVector2f(0.0f, 0.0f), 1e-4f) && Most.Equals(FVector2f(17.0f, 2.0f), 1e-3f));
	for (const int32 T : Willows.Mesh->TriangleIndicesItr())
	{
		const UE::Geometry::FIndex3i Corners = Willows.Mesh->GetTriangle(T);
		const UE::Geometry::FIndex3i UVs = Attributes->GetUVLayer(0)->GetTriangle(T);
		for (int32 K = 0; K < 3; ++K)
		{
			if (Attributes->GetUVLayer(0)->GetElement(UVs[K]).Equals(FVector2f(0.0f, 2.0f), 1e-4f))
			{
				LeftBottom = Willows.Frame.PointToParent(Musee::FromEngine(Willows.Mesh->GetVertex(Corners[K])));
			}
		}
	}
	TestTrue(TEXT("The image's left edge is at the run's far end, past the oval's west end"), LeftBottom.X < -9.0 && LeftBottom.Y > 0.0);
	TestEqual(TEXT("Its lower edge hangs at the plan's height"), LeftBottom.Z, 0.7, 1e-6);
	TestEqual(TEXT("It stands the stretcher's depth off the wall"), FMath::Square(LeftBottom.X / 11.0) + FMath::Square(LeftBottom.Y / 7.5) < 1.0, true);

	TArray<FFormMadePiece> None;
	TArray<FMuseeFinding> Faults;
	GatherArt(Tests::Oval(TEXT("faulty")), None, Faults);
	TestEqual(TEXT("A work on no wall, in a frame not yet made and an unknown care class, and a work the catalogue lacks, are findings"), Faults.Num(), 2);
	TestEqual(TEXT("And nothing is made of them"), None.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtFrameTest, "Musee.Art.Frame", Musee::ArtMaking::Tests::Flags)

bool FArtFrameTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	TArray<FFormMadePiece> Pieces;
	TArray<FMuseeFinding> Findings;
	GatherArt(Tests::Oval(TEXT("framed")), Pieces, Findings);
	for (const FMuseeFinding& Found : Findings)
	{
		AddError(Found.ToString());
	}
	const FFormMadePiece* Frame = Pieces.FindByPredicate([](const FFormMadePiece& P) { return P.Name == TEXT("canvas.monet-impression-sunrise.frame"); });
	if (!TestNotNull(TEXT("The framed canvas has its frame"), Frame))
	{
		return false;
	}
	TestTrue(TEXT("It takes the finish of the plan's frame surface"), Frame->Surface == TEXT("framed.frame.gilt") && Frame->Material.IsNone());
	TestTrue(TEXT("It is one closed solid"), Musee::FormMaking::MeasureShell(*Frame).IsClosed());
	FBox Box(ForceInit);
	for (const int32 V : Frame->Mesh->VertexIndicesItr())
	{
		Box += Frame->Frame.PointToParent(Musee::FromEngine(Frame->Mesh->GetVertex(V)));
	}
	// The moulding is 0.10 m wide round a 0.63 m canvas (0.09 + 0.015 x 0.63, held at the least), reaching 0.957 of it out.
	const double Out = 0.957 * 0.10;
	TestEqual(TEXT("It runs outside the canvas, across"), Box.GetSize().X, 0.63 + 2.0 * Out, 1e-3);
	TestEqual(TEXT("and up"), Box.GetSize().Z, 0.48 + 2.0 * Out, 1e-3);
	TestEqual(TEXT("It frames the canvas where it hangs"), Box.Min.Z, 1.31 - Out, 1e-3);
	TestEqual(TEXT("Its back stands on the hanging gap"), Box.Min.Y, -5.0 + 0.002, 1e-6);
	TestTrue(TEXT("Its moulding stands proud of the canvas"), Box.Max.Y > -5.0 + 0.04 + 0.03);

	const FFormMadePiece* Right = Pieces.FindByPredicate([](const FFormMadePiece& P) { return P.Name == TEXT("canvas.monet-willows-two-willows.1"); });
	const bool bLeft = Pieces.ContainsByPredicate([](const FFormMadePiece& P) { return P.Name == TEXT("canvas.monet-willows-two-willows.0"); });
	if (TestTrue(TEXT("A work on two canvases is two canvases, named by index"), Right && bLeft))
	{
		const UE::Geometry::FDynamicMeshUVOverlay* UVs = Right->Mesh->Attributes()->GetUVLayer(0);
		float Least = TNumericLimits<float>::Max(), Most = TNumericLimits<float>::Lowest();
		for (const int32 E : UVs->ElementIndicesItr())
		{
			Least = FMath::Min(Least, UVs->GetElement(E).X);
			Most = FMath::Max(Most, UVs->GetElement(E).X);
		}
		TestTrue(TEXT("The second canvas shows the right half of the image"), FMath::IsNearlyEqual(Least, 8.5f, 1e-4f) && FMath::IsNearlyEqual(Most, 17.0f, 1e-4f));
	}
	const TArray<FArtPaintLayer> Layers = PaintLayers(Tests::Oval(TEXT("framed")));
	if (TestEqual(TEXT("One paint layer a work"), Layers.Num(), 2))
	{
		const FArtPaintLayer& Willows = Layers[1];
		TestTrue(TEXT("Its image spans its canvases"), Willows.SizeMeters.Equals(FVector2D(17.0, 2.0)));
		TestTrue(TEXT("It is the work's own paint over the paint's palette entry"), Willows.Palette == TEXT("paint.monet-willows-two-willows")
			&& Willows.Parent == PaintPalette && Willows.Working == TEXT("varnished") && Willows.History == TEXT("clean") && !Willows.ImagePath.IsEmpty());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtImageTest, "Musee.Art.Image", Musee::ArtMaking::Tests::Flags)

bool FArtImageTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	UTexture2D* Texture = NewObject<UTexture2D>(GetTransientPackage());
	FString Why;
	if (!TestTrue(TEXT("A work's image fills a texture"), ComposeWorkImage(Tests::Fixture(TEXT("assets/salon/monet-willows-two-willows.png")), *Texture, Why)))
	{
		AddError(Why);
		return false;
	}
	TestTrue(TEXT("At its own resolution"), Texture->Source.GetSizeX() == 4 && Texture->Source.GetSizeY() == 2);
	TestTrue(TEXT("In its own colour space, sRGB"), Texture->SRGB);
	TestEqual(TEXT("Shown at most at the museum's size for works"), Texture->MaxTextureSize, WorkImageMaxPixels);
	TestFalse(TEXT("A file that isn't an image is refused"), ComposeWorkImage(Tests::Fixture(TEXT("collection.json")), *Texture, Why));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtDresserTest, "Musee.Art.Dresser", Musee::ArtMaking::Tests::Flags)

bool FArtDresserTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	AActor* Actor = Tests::StageActor(*World, { TEXT("canvas.monet-willows-two-willows"), TEXT("canvas.monet-willows-two-willows.linen") });
	FMuseeStageContext Stage = Tests::Oval();
	Stage.StageActor = Actor;
	TArray<FMuseeFinding> Findings;
	DressWorks(Stage, Findings);
	TestEqual(TEXT("The works dress without findings"), Findings.Num(), 0);
	TArray<UArtWorkComponent*> Works;
	Actor->GetComponents(Works);
	if (TestEqual(TEXT("The made work carries its label (the unmade one is skipped)"), Works.Num(), 1))
	{
		TestEqual(TEXT("Its label's title"), Works[0]->Caption().Title.ToString(), FString(TEXT("The Two Willows")));
		TestEqual(TEXT("Its care class"), Works[0]->CareClass(), FName(TEXT("oil-and-tempera")));
		const UStaticMeshComponent* Face = Tests::FindMesh(*Actor, TEXT("canvas.monet-willows-two-willows"));
		TestTrue(TEXT("Its pieces hang from it, so a look at the face finds the label"), Face && Face->GetAttachParent() == Works[0]
			&& Tests::FindMesh(*Actor, TEXT("canvas.monet-willows-two-willows.linen"))->GetAttachParent() == Works[0]);
		TestTrue(TEXT("It hangs where the face hung, and the face kept its place"), Works[0]->GetAttachParent() == Actor->GetRootComponent()
			&& Face && Face->GetComponentLocation().Equals(FVector::ZeroVector));
	}
	TArray<UStaticMeshComponent*> Meshes;
	Actor->GetComponents(Meshes);
	for (const UStaticMeshComponent* Mesh : Meshes)
	{
		TestTrue(TEXT("Both pieces carry the work and its care class"), Mesh->ComponentHasTag(TEXT("musee.care:oil-and-tempera"))
			&& Mesh->ComponentHasTag(TEXT("musee.work:monet-willows-two-willows")));
	}
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtGlassTest, "Musee.Art.Glass", Musee::ArtMaking::Tests::Flags)

bool FArtGlassTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	TArray<FFormMadePiece> Pieces;
	TArray<FMuseeFinding> Findings;
	GatherArt(Tests::Glazed(), Pieces, Findings);
	for (const FMuseeFinding& Found : Findings)
	{
		AddError(Found.ToString());
	}
	if (!TestEqual(TEXT("Two panels of glass; nothing of the work the stage builds"), Pieces.Num(), 2))
	{
		return false;
	}
	const FFormMadePiece& First = Pieces[0];
	TestEqual(TEXT("A panel is named for its work and index"), First.Name, FName(TEXT("glass.monet-willows-two-willows.1")));
	TestTrue(TEXT("It is the glass its plan names, carrying its own image's layer"), First.Surface == TEXT("glazed.band.glass")
		&& First.Material == TEXT("paint.monet-willows-two-willows.1"));
	TestTrue(TEXT("with that glass's working and history"), First.Working == TEXT("leaded") && First.History == TEXT("aged.patina"));
	TestFalse(TEXT("Glass is translucent, never Nanite"), First.bNanite);
	TestTrue(TEXT("It is one closed solid"), Musee::FormMaking::MeasureShell(First).IsClosed());
	FBox Box(ForceInit);
	for (const int32 V : First.Mesh->VertexIndicesItr())
	{
		Box += First.Frame.PointToParent(Musee::FromEngine(First.Mesh->GetVertex(V)));
	}
	// T1's light runs from x 4.80 towards -x (the viewer's right, facing north); the image starts 0.07 in and up.
	TestTrue(TEXT("It fills its image's place in the light"), Box.Min.X > 4.8 - 0.07 - 1.46 - 1e-6 && Box.Max.X < 4.8 - 0.07 + 1e-6
		&& FMath::IsNearlyEqual(Box.Min.Z, 10.97, 1e-6) && FMath::IsNearlyEqual(Box.Max.Z, 12.53, 1e-6));
	TestEqual(TEXT("It stands on the glass line, the glass's thickness through"), Box.GetSize().Y, PaneThicknessMeters, 1e-6);
	const UE::Geometry::FDynamicMeshUVOverlay* UVs = First.Mesh->Attributes()->GetUVLayer(0);
	FVector2f Least(TNumericLimits<float>::Max()), Most(TNumericLimits<float>::Lowest());
	for (const int32 E : UVs->ElementIndicesItr())
	{
		Least = FVector2f::Min(Least, UVs->GetElement(E));
		Most = FVector2f::Max(Most, UVs->GetElement(E));
	}
	TestTrue(TEXT("Its image spans it in metres"), Least.Equals(FVector2f(0.0f, 0.0f), 1e-5f) && Most.Equals(FVector2f(1.46f, 1.56f), 1e-5f));

	const TArray<FArtPaintLayer> Layers = PaintLayers(Tests::Glazed());
	if (TestEqual(TEXT("One layer a panel, each its own image"), Layers.Num(), 2))
	{
		TestTrue(TEXT("The first panel's layer is its own"), Layers[0].Work == TEXT("monet-willows-two-willows.1")
			&& Layers[0].Palette == TEXT("paint.monet-willows-two-willows.1") && Layers[0].SizeMeters.Equals(FVector2D(1.46, 1.56)));
		TestTrue(TEXT("over the glass it is fired in"), Layers[0].Parent == TEXT("glass.mouth-blown") && Layers[0].Working == TEXT("leaded")
			&& Layers[0].History == TEXT("aged.patina"));
		TestTrue(TEXT("and each panel shows the image its entry names"), Layers[0].ImagePath.EndsWith(TEXT("monet-willows-two-willows.png"))
			&& Layers[1].ImagePath.EndsWith(TEXT("monet-impression-sunrise.png")));
	}
	FArtHang Hang;
	TArray<FMuseeFinding> Reported;
	if (TestTrue(TEXT("The band's plan reads"), ReadStageHang(Tests::Glazed(), Hang, Reported)))
	{
		const FVector2D Times = Hang.Works[0].TimesOriginal();
		TestTrue(TEXT("A panel made after its original knows how many times its size it is"), FMath::IsNearlyEqual(Times.X, 1.46 / 0.605, 1e-9)
			&& FMath::IsNearlyEqual(Times.Y, 1.56 / 0.68, 1e-9));
		TestEqual(TEXT("A work at its own size is at 1"), Hang.Works[2].TimesOriginal(), FVector2D::UnitVector);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtBuiltTest, "Musee.Art.Built", Musee::ArtMaking::Tests::Flags)

bool FArtBuiltTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	AActor* Actor = Tests::StageActor(*World, { TEXT("work.monet-impression-sunrise"), TEXT("work.monet-impression-sunrise.coping"),
		TEXT("work.monet-impression-sunrise-lantern") });
	FMuseeStageContext Stage = Tests::Glazed();
	Stage.StageActor = Actor;
	TArray<FMuseeFinding> Findings;
	DressWorks(Stage, Findings);
	TestEqual(TEXT("The built work dresses without findings (the band's glass isn't made here, and isn't Art's to report)"), Findings.Num(), 0);
	TArray<UArtWorkComponent*> Works;
	Actor->GetComponents(Works);
	if (TestEqual(TEXT("The built work carries its label"), Works.Num(), 1))
	{
		TestEqual(TEXT("Its label's title, from its catalogue"), Works[0]->Caption().Title.ToString(), FString(TEXT("Impression, Sunrise")));
		TestEqual(TEXT("Its care class"), Works[0]->CareClass(), FName(TEXT("ceramic-and-glass")));
		const UStaticMeshComponent* Whole = Tests::FindMesh(*Actor, TEXT("work.monet-impression-sunrise"));
		const UStaticMeshComponent* Part = Tests::FindMesh(*Actor, TEXT("work.monet-impression-sunrise.coping"));
		TestTrue(TEXT("The pieces the stage built for it hang from its label"), Whole && Part && Whole->GetAttachParent() == Works[0]
			&& Part->GetAttachParent() == Works[0]);
		TestTrue(TEXT("and carry its care class"), Whole && Whole->ComponentHasTag(TEXT("musee.care:ceramic-and-glass"))
			&& Whole->ComponentHasTag(TEXT("musee.work:monet-impression-sunrise")));
	}
	const UStaticMeshComponent* Other = Tests::FindMesh(*Actor, TEXT("work.monet-impression-sunrise-lantern"));
	TestTrue(TEXT("A piece whose name only begins the same is not the work's"), Other && Other->GetAttachParent() == Actor->GetRootComponent());

	AActor* Bare = Tests::StageActor(*World, {});
	Stage.StageActor = Bare;
	TArray<FMuseeFinding> Missing;
	DressWorks(Stage, Missing);
	TestEqual(TEXT("A built work whose pieces the stage didn't make is a finding"), Missing.Num(), 1);
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtReproductionTest, "Musee.Art.Reproduction", Musee::ArtMaking::Tests::Flags)

bool FArtReproductionTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	const TValueOrError<FArtHang, FString> Hang = Musee::ReadRecord<FArtHang>(Tests::Fixture(TEXT("Stages/facsimile/Hang.json")));
	const TValueOrError<FCareKeys, FString> Care = ReadCareKeys();
	if (!TestTrue(TEXT("The plan and Care.json read"), Hang.HasValue() && Care.HasValue()))
	{
		AddError(Hang.HasError() ? Hang.GetError() : Care.GetError());
		return false;
	}
	const TArray<FArtHangEntry>& Works = Hang.GetValue().Works;
	const TValueOrError<FArtWork, FString> Facsimile = ResolveEntry(Works[0], Care.GetValue());
	if (TestTrue(TEXT("A facsimile resolves to its original's catalogue entry"), Facsimile.HasValue()))
	{
		TestEqual(TEXT("Its care class stays the original's"), Facsimile.GetValue().Care, FName(TEXT("works-on-paper")));
		TestTrue(TEXT("Its label says what hangs, and where the original is"), Facsimile.GetValue().Caption().Credit.ToString().StartsWith(
			TEXT("Exhibition facsimile after the original in Mus")) && Facsimile.GetValue().Caption().Credit.ToString().EndsWith(TEXT("Marmottan Monet, Paris")));
	}
	TestFalse(TEXT("A reproduction Care.json doesn't list doesn't resolve"), ResolveEntry(Works[1], Care.GetValue()).HasValue());
	TestFalse(TEXT("Nor a work built any way but by form"), ResolveEntry(Works[2], Care.GetValue()).HasValue());

	FArtLightDose Daylight;
	Daylight.Illuminance = Musee::Lux(5000.0);
	Daylight.AnnualLuxHours = 5000.0 * 3000.0;
	Daylight.UvMicrowattsPerLumen = 100.0;
	const TValueOrError<TArray<FString>, FString> Original = Musee::Art::CareFaults(TEXT("works-on-paper"), NAME_None, Daylight);
	TestTrue(TEXT("An original print in a glass hall's daylight is over its lux, its dose and its ultraviolet"), Original.HasValue()
		&& Original.GetValue().Num() == 3);
	const TValueOrError<TArray<FString>, FString> Facsimiles = Musee::Art::CareFaults(TEXT("works-on-paper"), TEXT("exhibition-facsimile"), Daylight);
	TestTrue(TEXT("Its exhibition facsimile is within care under any light"), Facsimiles.HasValue() && Facsimiles.GetValue().IsEmpty());
	FArtLightDose Gallery;
	Gallery.Illuminance = Musee::Lux(50.0);
	Gallery.AnnualLuxHours = 50.0 * 3000.0;
	Gallery.UvMicrowattsPerLumen = 10.0;
	const TValueOrError<TArray<FString>, FString> Within = Musee::Art::CareFaults(TEXT("works-on-paper"), NAME_None, Gallery);
	TestTrue(TEXT("The original at 50 lx for 3,000 hours is within"), Within.HasValue() && Within.GetValue().IsEmpty());
	const TValueOrError<TArray<FString>, FString> Stone = Musee::Art::CareFaults(TEXT("stone-and-bronze"), NAME_None, Daylight);
	TestTrue(TEXT("Stone has no lux or dose limit, only its ultraviolet"), Stone.HasValue() && Stone.GetValue().Num() == 1);
	TestFalse(TEXT("An unknown class is refused"), Musee::Art::CareFaults(TEXT("no-such-class"), NAME_None, Daylight).HasValue());
	TestFalse(TEXT("An unknown reproduction is refused"), Musee::Art::CareFaults(TEXT("works-on-paper"), TEXT("photograph-on-sale"), Daylight).HasValue());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtPaperTest, "Musee.Art.Paper", Musee::ArtMaking::Tests::Flags)

bool FArtPaperTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	TArray<FFormMadePiece> Pieces;
	TArray<FMuseeFinding> Findings;
	GatherArt(Tests::Cabinet(), Pieces, Findings);
	TestEqual(TEXT("A scroll, not made yet, is the one finding"), Findings.Num(), 1);
	const FFormMadePiece* Print = Tests::FindPiece(Pieces, TEXT("paper.monet-impression-sunrise"));
	const FFormMadePiece* Mount = Tests::FindPiece(Pieces, TEXT("paper.monet-impression-sunrise.mount"));
	const FFormMadePiece* Glazing = Tests::FindPiece(Pieces, TEXT("paper.monet-impression-sunrise.glazing"));
	const FFormMadePiece* Frame = Tests::FindPiece(Pieces, TEXT("paper.monet-impression-sunrise.frame"));
	if (!TestTrue(TEXT("A print is made on its mount, glazed and framed"), Print && Mount && Glazing && Frame))
	{
		return false;
	}
	TestTrue(TEXT("The print is the work's own layer over the paper's finish"), Print->Material == TEXT("paint.monet-impression-sunrise")
		&& Print->Working == TEXT("printed") && Print->History == TEXT("clean"));
	const FBox PrintBox = Tests::StageBox(*Print);
	TestTrue(TEXT("at its place and size on the mount's face, a sheet of paper"), PrintBox.Min.Equals(FVector(1.0, -5.0, 1.5), 1e-6)
		&& PrintBox.Max.Equals(FVector(1.2, -5.0 + 0.0003, 1.75), 1e-6));
	const FBox MountBox = Tests::StageBox(*Mount);
	TestTrue(TEXT("The mount is centred on the print, its face on the wall's plane and its depth behind"), MountBox.Min.Equals(FVector(0.6, -5.03, 1.175), 1e-6)
		&& MountBox.Max.Equals(FVector(1.6, -5.0, 2.075), 1e-6) && Mount->Surface == TEXT("cabinet.mount") && Mount->Material.IsNone());
	const FBox GlassBox = Tests::StageBox(*Glazing);
	TestTrue(TEXT("The glazing lies on the print, the mount's size, as thick as its working's pane (8 mm)"), GlassBox.Min.Equals(FVector(0.6, -5.0 + 0.0003, 1.175), 1e-6)
		&& GlassBox.Max.Equals(FVector(1.6, -5.0 + 0.0083, 2.075), 1e-6) && Glazing->Surface == TEXT("cabinet.glazing") && !Glazing->bNanite);
	const FBox FrameBox = Tests::StageBox(*Frame);
	TestTrue(TEXT("The frame is the photograph's 22 mm moulding round the mount, one closed solid"), Musee::FormMaking::MeasureShell(*Frame).IsClosed()
		&& FMath::IsNearlyEqual(FrameBox.GetSize().X, 1.044, 1e-4) && FMath::IsNearlyEqual(FrameBox.GetSize().Z, 0.944, 1e-4));
	TestTrue(TEXT("its back flush with the mount's, its face 0.8 of its width proud of the glass"), FMath::IsNearlyEqual(FrameBox.Min.Y, -5.03, 1e-6)
		&& FMath::IsNearlyEqual(FrameBox.Max.Y, -5.0 + 0.0083 + 0.8 * 0.022, 1e-4));

	const FFormMadePiece* Card = Tests::FindPiece(Pieces, TEXT("paper.monet-willows-two-willows"));
	const FFormMadePiece* Cover = Tests::FindPiece(Pieces, TEXT("paper.monet-willows-two-willows.glazing"));
	if (TestTrue(TEXT("A card lies on its reader under its glass"), Card && Cover))
	{
		const FBox CardBox = Tests::StageBox(*Card);
		TestTrue(TEXT("centred on the reader's face"), CardBox.GetCenter().Equals(FVector(2.0, 0.00015, 1.3), 1e-6)
			&& FMath::IsNearlyEqual(CardBox.GetSize().X, 0.171, 1e-6) && FMath::IsNearlyEqual(CardBox.Min.Y, 0.0, 1e-9));
		TestTrue(TEXT("the glass on it"), FMath::IsNearlyEqual(Tests::StageBox(*Cover).Min.Y, 0.0003, 1e-9));
	}
	const FFormMadePiece* Plate = Tests::FindPiece(Pieces, TEXT("plate.daguerre-boulevard"));
	if (TestNotNull(TEXT("A daguerreotype is its plate"), Plate))
	{
		TestTrue(TEXT("its layer over the silver, developed and clean"), Plate->Material == TEXT("paint.daguerre-boulevard")
			&& Plate->Working == TEXT("developed") && Plate->History == TEXT("clean"));
		TestTrue(TEXT("a sheet half a millimetre thick"), FMath::IsNearlyEqual(Tests::StageBox(*Plate).GetSize().Y, 0.0005, 1e-9));
		TestFalse(TEXT("and bare: it names no glazing"), Pieces.ContainsByPredicate([](const FFormMadePiece& P) { return P.Name == TEXT("plate.daguerre-boulevard.glazing"); }));
	}
	const TArray<FArtPaintLayer> Layers = PaintLayers(Tests::Cabinet());
	const FArtPaintLayer* PrintLayer = Layers.FindByPredicate([](const FArtPaintLayer& L) { return L.Work == TEXT("monet-impression-sunrise"); });
	const FArtPaintLayer* PlateLayer = Layers.FindByPredicate([](const FArtPaintLayer& L) { return L.Work == TEXT("daguerre-boulevard"); });
	TestTrue(TEXT("A print's layer is its image at its size over the paper"), PrintLayer && PrintLayer->Parent == TEXT("accent.print-on-paper")
		&& PrintLayer->SizeMeters.Equals(FVector2D(0.2, 0.25)));
	TestTrue(TEXT("A plate's is over the daguerreotype's silver"), PlateLayer && PlateLayer->Parent == TEXT("accent.daguerreotype")
		&& PlateLayer->Working == TEXT("developed"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtTransparencyTest, "Musee.Art.Transparency", Musee::ArtMaking::Tests::Flags)

bool FArtTransparencyTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	TArray<FFormMadePiece> Pieces;
	TArray<FMuseeFinding> Findings;
	GatherArt(Tests::Cabinet(), Pieces, Findings);
	const FFormMadePiece* Plate = Tests::FindPiece(Pieces, TEXT("glass.lumiere-peacock"));
	const FFormMadePiece* Mat = Tests::FindPiece(Pieces, TEXT("glass.lumiere-peacock.mat"));
	const FFormMadePiece* Backing = Tests::FindPiece(Pieces, TEXT("glass.lumiere-peacock.backing"));
	const FFormMadePiece* Glazing = Tests::FindPiece(Pieces, TEXT("glass.lumiere-peacock.glazing"));
	const FFormMadePiece* Frame = Tests::FindPiece(Pieces, TEXT("glass.lumiere-peacock.frame"));
	if (!TestTrue(TEXT("A transparency on a wall is made in its window mat, backed, glazed and framed"), Plate && Mat && Backing && Glazing && Frame))
	{
		return false;
	}
	TestTrue(TEXT("Its plate is the work's own layer over the plate's glass, translucent"), Plate->Material == TEXT("paint.lumiere-peacock")
		&& Plate->Surface == TEXT("cabinet.plate") && Plate->Working == TEXT("plate.photographic") && !Plate->bNanite);
	const FBox PlateBox = Tests::StageBox(*Plate);
	TestTrue(TEXT("behind the mat's window, as thick as its working's plate (3 mm)"), PlateBox.Min.Equals(FVector(5.0, -5.0 - 0.0015 - 0.003, 1.5), 1e-6)
		&& PlateBox.Max.Equals(FVector(5.177, -5.0 - 0.0015, 1.63), 1e-6));
	TestTrue(TEXT("The mat is one closed solid with the window cut through it"), Musee::FormMaking::MeasureShell(*Mat).IsClosed()
		&& FMath::IsNearlyEqual(Tests::AreaSquareMeters(Mat->Mesh), 2.0 * (1.0 * 0.9 - 0.177 * 0.13) + 2.0 * (1.0 + 0.9 + 0.177 + 0.13) * 0.0015, 1e-6));
	const FBox BackingBox = Tests::StageBox(*Backing);
	TestTrue(TEXT("The diffusing pane lies behind the plate, the mat's size"), FMath::IsNearlyEqual(BackingBox.Max.Y, -5.0045, 1e-6)
		&& FMath::IsNearlyEqual(BackingBox.Min.Y, -5.0045 - 0.006, 1e-6) && FMath::IsNearlyEqual(BackingBox.GetSize().X, 1.0, 1e-6));
	const FBox FrameBox = Tests::StageBox(*Frame);
	TestTrue(TEXT("The frame's back is flush with the backing's, its lip on the glazing"), FMath::IsNearlyEqual(FrameBox.Min.Y, -5.0105, 1e-6)
		&& FMath::IsNearlyEqual(FrameBox.Max.Y, -5.0 + 0.008 + 0.8 * 0.022, 1e-4));
	const TArray<FArtPaintLayer> Layers = PaintLayers(Tests::Cabinet());
	const FArtPaintLayer* Layer = Layers.FindByPredicate([](const FArtPaintLayer& L) { return L.Work == TEXT("lumiere-peacock"); });
	TestTrue(TEXT("Its layer is its image over the plate's glass"), Layer && Layer->Parent == TEXT("glass.clear-float") && Layer->Working == TEXT("plate.photographic")
		&& Layer->SizeMeters.Equals(FVector2D(0.177, 0.13)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtWareTest, "Musee.Art.Ware", Musee::ArtMaking::Tests::Flags)

bool FArtWareTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	TArray<FFormMadePiece> Pieces;
	TArray<FMuseeFinding> Findings;
	GatherArt(Tests::Cabinet(), Pieces, Findings);
	const FFormMadePiece* Dish = Tests::FindPiece(Pieces, TEXT("ware.albion-de-morgan-charger"));
	const FFormMadePiece* Underside = Tests::FindPiece(Pieces, TEXT("ware.albion-de-morgan-charger.ground"));
	const FFormMadePiece* Jar = Tests::FindPiece(Pieces, TEXT("ware.albion-de-morgan-jar"));
	const FFormMadePiece* Inside = Tests::FindPiece(Pieces, TEXT("ware.albion-de-morgan-jar.ground"));
	if (!TestTrue(TEXT("A dish and a jar are each turned, decorated and grounded"), Dish && Underside && Jar && Inside))
	{
		return false;
	}
	TestTrue(TEXT("Its decoration is the work's own layer over the lustre"), Dish->Material == TEXT("paint.albion-de-morgan-charger")
		&& Dish->Working == TEXT("lustre-painted") && Dish->History == TEXT("clean"));
	TestTrue(TEXT("its underside the plain tin glaze"), Underside->Material == TEXT("accent.lustreware") && Underside->Working == TEXT("tin-glazed"));
	const FBox DishBox = Tests::StageBox(*Dish) + Tests::StageBox(*Underside);
	TestTrue(TEXT("The dish stands at its centre on the bed, its size"), FMath::IsNearlyEqual(DishBox.GetSize().X, 0.514, 1e-3)
		&& FMath::IsNearlyEqual(DishBox.Min.Z, 0.55, 1e-6) && FMath::IsNearlyEqual(DishBox.Max.Z, 0.62, 1e-6)
		&& FMath::IsNearlyEqual(DishBox.GetCenter().Y, -0.3, 1e-3));
	const FBox JarBox = Tests::StageBox(*Jar);
	TestTrue(TEXT("The jar stands its height"), FMath::IsNearlyEqual(JarBox.Min.Z, 0.55, 1e-6) && FMath::IsNearlyEqual(JarBox.Max.Z, 0.93, 1e-6));
	// The dish is photographed from above: its image's top-left corner lies at its far left, the image spanning the dish.
	const UE::Geometry::FDynamicMeshUVOverlay* UVs = Dish->Mesh->Attributes()->GetUVLayer(0);
	FVector2f Least(TNumericLimits<float>::Max()), Most(TNumericLimits<float>::Lowest());
	for (const int32 E : UVs->ElementIndicesItr())
	{
		Least = FVector2f::Min(Least, UVs->GetElement(E));
		Most = FVector2f::Max(Most, UVs->GetElement(E));
	}
	TestTrue(TEXT("Its image spans its face in metres"), Least.Equals(FVector2f(0.0f, 0.0f), 2e-3f) && Most.Equals(FVector2f(0.514f, 0.514f), 2e-3f));
	const TArray<FArtPaintLayer> Layers = PaintLayers(Tests::Cabinet());
	const FArtPaintLayer* JarLayer = Layers.FindByPredicate([](const FArtPaintLayer& L) { return L.Work == TEXT("albion-de-morgan-jar"); });
	TestTrue(TEXT("The jar's layer spans its whole photograph, the jar filling its window"), JarLayer && JarLayer->Parent == TEXT("accent.lustreware")
		&& JarLayer->SizeMeters.Equals(FVector2D(0.24 / (0.926 - 0.066), 0.38 / (0.997 - 0.0278)), 1e-6));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtEveryHangResolves, "Musee.Art.EveryHangResolves", Musee::ArtMaking::Tests::Flags)

bool FArtEveryHangResolves::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	const TValueOrError<FCareKeys, FString> Care = ReadCareKeys();
	if (!TestTrue(TEXT("Care.json reads"), Care.HasValue()))
	{
		AddError(Care.GetError());
		return false;
	}
	const FString Stages = Musee::Records::DataRoot() / TEXT("Stages");
	TArray<FString> Folders;
	IFileManager::Get().FindFiles(Folders, *(Stages / TEXT("*")), false, true);
	Folders.Sort();
	int32 Plans = 0, Resolved = 0, Built = 0;
	for (const FString& Folder : Folders)
	{
		const FString Path = Stages / Folder / FArtHang::FileName;
		if (!IFileManager::Get().FileExists(*Path))
		{
			continue;
		}
		++Plans;
		const TValueOrError<FArtHang, FString> Hang = Musee::ReadRecord<FArtHang>(Path);
		if (Hang.HasError())
		{
			AddError(FString::Printf(TEXT("%s: %s"), *Folder, *Hang.GetError()));
			continue;
		}
		TestEqual(FString::Printf(TEXT("%s's plan names its stage"), *Folder), Hang.GetValue().Stage.ToString(), Folder);
		for (const FArtHangEntry& Entry : Hang.GetValue().Works)
		{
			const TValueOrError<FArtWork, FString> Work = ResolveEntry(Entry, Care.GetValue());
			if (Work.HasError())
			{
				AddError(FString::Printf(TEXT("%s: %s (object %d) does not resolve: %s"), *Folder, *Entry.Work, Entry.Canvas, *Work.GetError()));
				continue;
			}
			++Resolved;
			Built += Entry.IsBuilt() ? 1 : 0;
		}
	}
	TestTrue(TEXT("There are hanging plans to resolve"), Plans > 0 && Resolved > 0);
	AddInfo(FString::Printf(TEXT("%d entries of %d hanging plans resolve to catalogue works, %d of them built by their stage."), Resolved, Plans, Built));
	return true;
}

#endif
