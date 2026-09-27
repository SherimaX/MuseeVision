#include "ArtHangEntry.h"
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
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
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
	AActor* Actor = World->SpawnActor<AActor>();
	USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"));
	Actor->SetRootComponent(Root);
	Root->RegisterComponent();
	for (const TCHAR* Piece : { TEXT("canvas.monet-willows-two-willows"), TEXT("canvas.monet-willows-two-willows.linen") })
	{
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Actor, FName(FString(Piece).Replace(TEXT("."), TEXT("_"))));
		Mesh->SetupAttachment(Root);
		Mesh->ComponentTags.Add(FName(FString::Printf(TEXT("musee.piece:%s"), Piece)));
		Mesh->RegisterComponent();
	}
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
		TestTrue(TEXT("On its face"), Works[0]->GetAttachParent() && Works[0]->GetAttachParent()->ComponentHasTag(TEXT("musee.piece:canvas.monet-willows-two-willows")));
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

#endif
