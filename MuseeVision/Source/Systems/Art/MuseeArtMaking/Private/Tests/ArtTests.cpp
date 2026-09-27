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
#include "Frame/Hooks.h"
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

	/** The lectern's desk: x up its 18 degree slope, y across it, z its normal; its reader stands on its low side, facing +x. */
	FQuat Desk()
	{
		const double Slope = FMath::DegreesToRadians(18.0);
		const FVector Up(FMath::Cos(Slope), 0.0, FMath::Sin(Slope));
		const FVector Across(0.0, 1.0, 0.0);
		return FQuat(FMatrix(Up, Across, FVector::CrossProduct(Up, Across), FVector::ZeroVector));
	}

	/**
	 * The test hangings: a flat wall facing south where a panel and a tapestry hang, a scroll case's back wall facing south
	 * 0.5 m up, a case's bed facing up and a lectern's sloping desk.
	 */
	FMuseeStageContext Hangings()
	{
		FMuseeStageContext Context;
		Context.Stage = FMuseeKey(TEXT("hangings"));
		Context.ScheduleDir = Fixture(TEXT("Stages/hangings"));
		Context.Anchors = {
			Anchor(TEXT("hangings.wall"), FVector(0.0, -5.0, 0.0)),
			Anchor(TEXT("hangings.case"), FVector(8.0, -5.0, 0.5)),
			Anchor(TEXT("hangings.bed"), TEXT("Mount"), FVector(6.0, 0.0, 0.864)),
			Anchor(TEXT("hangings.lectern"), TEXT("Mount"), FVector(10.0, 0.0, 1.0), Desk()),
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

	/** A stage actor with a root and a mesh component per named piece, tagged and static as Studio makes them. */
	AActor* StageActor(UWorld& World, const TArray<FString>& Pieces)
	{
		AActor* Actor = World.SpawnActor<AActor>();
		USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"));
		Root->SetMobility(EComponentMobility::Static);
		Actor->SetRootComponent(Root);
		Root->RegisterComponent();
		for (const FString& Piece : Pieces)
		{
			UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Actor, FName(Piece.Replace(TEXT("."), TEXT("_"))));
			Mesh->SetMobility(EComponentMobility::Static);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtBodyTest, "Musee.Art.Body", Musee::ArtMaking::Tests::Flags)

/**
 * A work whose wall rides a moving body (a store's rack) rides it: its pieces are those the work makes standing still,
 * each carried on the body in the body's frame, and its label hangs where they hang, on the body.
 */
bool FArtBodyTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	const FMuseeFrame Rack(FVector(1.5, -4.0, 0.0), FQuat(FVector::UpVector, FMath::DegreesToRadians(30.0)));
	FMuseeStageContext Carried = Tests::Oval(TEXT("carried"));
	Carried.Anchors.Add(Tests::Anchor(TEXT("carried.rack"), TEXT("Body"), Rack.OriginMeters, Rack.Rotation));

	TArray<FFormMadePiece> Still;
	TArray<FMuseeFinding> StillFindings;
	GatherArt(Tests::Oval(TEXT("framed")), Still, StillFindings);
	TArray<FFormMadePiece> Pieces;
	TArray<FMuseeFinding> Findings;
	GatherArt(Carried, Pieces, Findings);
	TestEqual(TEXT("A work on a body the stage doesn't declare is a finding"), Findings.Num(), 1);
	TestFalse(TEXT("and nothing is made of it"), Pieces.ContainsByPredicate([](const FFormMadePiece& P) { return P.Name.ToString().Contains(TEXT("willows")); }));
	const TArray<FName> Parts = { TEXT("canvas.monet-impression-sunrise"), TEXT("canvas.monet-impression-sunrise.linen"),
		TEXT("canvas.monet-impression-sunrise.frame") };
	for (const FName Part : Parts)
	{
		const FFormMadePiece* Moving = Tests::FindPiece(Pieces, *Part.ToString());
		const FFormMadePiece* Standing = Tests::FindPiece(Still, *Part.ToString());
		if (!TestNotNull(*FString::Printf(TEXT("%s is made on the rack"), *Part.ToString()), Moving) || !TestNotNull(TEXT("and standing still"), Standing))
		{
			continue;
		}
		TestEqual(*FString::Printf(TEXT("%s rides the rack"), *Part.ToString()), Moving->Body, FMuseeKey(TEXT("carried.rack")));
		TestTrue(TEXT("A still work rides nothing"), Standing->Body.IsNone());
		const FMuseeFrame AtRest = Rack.FrameToParent(Moving->Frame);
		TestTrue(*FString::Printf(TEXT("%s, seen from the stage with the rack at rest, is where the still work is"), *Part.ToString()),
			AtRest.OriginMeters.Equals(Standing->Frame.OriginMeters, 1e-9) && AtRest.Rotation.Equals(Standing->Frame.Rotation, 1e-9));
		TestTrue(TEXT("in the rack's frame, not the stage's"), !Moving->Frame.OriginMeters.Equals(Standing->Frame.OriginMeters, 1e-3));
	}

	// The dresser hangs the label where the pieces hang: on the rack's component, so the label moves with them.
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	AActor* Actor = Tests::StageActor(*World, {});
	USceneComponent* Body = NewObject<USceneComponent>(Actor, TEXT("Rack"));
	Body->SetMobility(EComponentMobility::Movable);
	Body->SetupAttachment(Actor->GetRootComponent());
	Body->ComponentTags.Add(TEXT("carried.rack"));
	Body->RegisterComponent();
	for (const FName Part : Parts)
	{
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Actor, FName(Part.ToString().Replace(TEXT("."), TEXT("_"))));
		Mesh->SetMobility(EComponentMobility::Movable);
		Mesh->SetupAttachment(Body);
		Mesh->ComponentTags.Add(FName(FString::Printf(TEXT("musee.piece:%s"), *Part.ToString())));
		Mesh->RegisterComponent();
	}
	Carried.StageActor = Actor;
	TArray<FMuseeFinding> Dressed;
	DressWorks(Carried, Dressed);
	TestEqual(TEXT("The carried work dresses without findings"), Dressed.Num(), 0);
	TArray<UArtWorkComponent*> Works;
	Actor->GetComponents(Works);
	if (TestEqual(TEXT("It carries its label"), Works.Num(), 1))
	{
		TestTrue(TEXT("which hangs from the rack and moves with it"), Works[0]->GetAttachParent() == Body && Works[0]->Mobility == EComponentMobility::Movable);
		const UStaticMeshComponent* Face = Tests::FindMesh(*Actor, TEXT("canvas.monet-impression-sunrise"));
		TestTrue(TEXT("and the face hangs from the label, under the rack"), Face && Face->GetAttachParent() == Works[0]);
	}
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtHooksTest, "Musee.Art.Hooks", Musee::ArtMaking::Tests::Flags)

/**
 * A framed canvas on a wire mesh (a store's rack) hangs by a pair of S-hooks through the screw eyes in its frame's back:
 * each eye's ring lies level against the back a third of the frame down, halfway between two uprights; each hook rises
 * from its eye, passes over a level wire and turns down behind it, all between the frame's back and the mesh. Where the
 * mesh stands too close, or no wire is high enough, or the work has no frame to hang by, it is a finding.
 */
bool FArtHooksTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	TArray<FFormMadePiece> Pieces;
	TArray<FMuseeFinding> Findings;
	GatherArt(Tests::Oval(TEXT("meshed")), Pieces, Findings);
	if (TestEqual(TEXT("One finding: the unframed canvas on a mesh"), Findings.Num(), 1))
	{
		TestTrue(TEXT("it has no eyes to hang by"), Findings[0].ToString().Contains(TEXT("hooks in its frame's back")));
	}
	const double Wall = -5.0;
	const double Back = Wall + 0.002;
	const double Mesh = Wall - 0.0105;
	for (const TCHAR* Index : { TEXT("1"), TEXT("2") })
	{
		const FFormMadePiece* Hook = Tests::FindPiece(Pieces, *FString::Printf(TEXT("canvas.monet-impression-sunrise.hook.%s"), Index));
		const FFormMadePiece* Eye = Tests::FindPiece(Pieces, *FString::Printf(TEXT("canvas.monet-impression-sunrise.eye.%s"), Index));
		if (!TestNotNull(TEXT("Each side of the frame has its hook"), Hook) || !TestNotNull(TEXT("and its eye"), Eye))
		{
			continue;
		}
		TestTrue(TEXT("both in the mesh's hooks surface"), Hook->Surface == TEXT("meshed.hook") && Eye->Surface == TEXT("meshed.hook"));
		TestTrue(TEXT("each a closed solid"), Musee::FormMaking::MeasureShell(*Hook).IsClosed() && Musee::FormMaking::MeasureShell(*Eye).IsClosed());
		const FBox H = Tests::StageBox(*Hook);
		const FBox E = Tests::StageBox(*Eye);
		// The eye: a 4 mm ring of 2 mm wire lying level against the frame's back, a third of the 0.671 m frame down its 1.886 top.
		// Form draws a round of wire as a polygon, whose corners stand past the round by under a millimetre.
		const double Polygon = 0.001;
		TestEqual(TEXT("The eye's ring touches the frame's back"), E.Max.Y, Back, Polygon);
		TestEqual(TEXT("and stands 8 mm off it"), E.GetSize().Y, 0.008, Polygon);
		TestEqual(TEXT("a third of the frame down"), E.GetCenter().Z, 1.3100 + 0.48 + 0.0957 - (0.48 + 2.0 * 0.0957) / 3.0, 2e-4);
		const double Gap = (E.GetCenter().X - 0.1) / 0.025 - 0.5;
		TestEqual(TEXT("halfway between two uprights"), Gap, FMath::RoundToDouble(Gap), 1e-6);
		TestEqual(TEXT("and its hook with it"), H.GetCenter().X, E.GetCenter().X, 1e-6);
		// The hook: its upper bend's inside rests over a level wire (its top 1.5 + 2 + 0.2 mm over the wire's centre).
		const double Wire = (H.Max.Z - 0.0037 - 0.23) / 0.025;
		TestEqual(TEXT("The hook hangs over a level wire"), Wire, FMath::RoundToDouble(Wire), Polygon / 0.025);
		TestTrue(TEXT("with at least its least shank above the eye"), H.Max.Z - E.GetCenter().Z >= 0.02 && H.Max.Z - E.GetCenter().Z < 0.02 + 0.025 + 0.01);
		TestTrue(TEXT("between the frame's back and the mesh's face, turning down behind the wire"), H.Max.Y < Back && H.Min.Y < Mesh - 0.0015);
		TestTrue(TEXT("its lower bend round the eye's far wire"), H.Min.Z < E.Min.Z && H.Min.Y < E.Min.Y);
	}
	const FFormMadePiece* Frame = Tests::FindPiece(Pieces, TEXT("canvas.monet-impression-sunrise.frame"));
	TestTrue(TEXT("The frame hangs where it would on a wall"), Frame && FMath::IsNearlyEqual(Tests::StageBox(*Frame).Min.Y, Back, 1e-6));

	// Hung where it can't be: the mesh too close behind the frame's back for a hook's bends, no wire high enough.
	const TValueOrError<FArtHooks, FString> Hooks = Musee::ReadRecord<FArtHooks>(FArtHooks::Record);
	const TValueOrError<FArtMoulding, FString> Moulding = Musee::ReadRecord<FArtMoulding>(FArtMoulding::Record);
	if (TestTrue(TEXT("The hooks' and the moulding's records read"), Hooks.HasValue() && Moulding.HasValue()))
	{
		FArtMesh Close;
		Close.Anchor = FMuseeKey(TEXT("oval.wall.flat"));
		Close.Levels = { 0.23, 3.28, 0.025 };
		Close.Uprights = { 0.1, 4.9, 0.025 };
		Close.Behind = 0.006;
		Close.Wire = 0.003;
		FArtHangEntry Work = Tests::Entry(TEXT("monet-impression-sunrise"), TEXT("artworks.json"));
		Work.Along = 1.0;
		Work.Height = 1.31;
		TestTrue(TEXT("A mesh too close behind the frame is refused"), PlanHooks(Hooks.GetValue(), Close, Moulding.GetValue(), Work).HasError());
		FArtMesh Low = Close;
		Low.Behind = 0.0105;
		Low.Levels.To = 1.5;
		TestTrue(TEXT("and one with no wire over the eyes"), PlanHooks(Hooks.GetValue(), Low, Moulding.GetValue(), Work).HasError());
		Low.Levels.To = 3.28;
		TestTrue(TEXT("while the rack's own mesh hangs it"), PlanHooks(Hooks.GetValue(), Low, Moulding.GetValue(), Work).HasValue());
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtPanelAndTapestryTest, "Musee.Art.PanelAndTapestry", Musee::ArtMaking::Tests::Flags)

bool FArtPanelAndTapestryTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	TArray<FFormMadePiece> Pieces;
	TArray<FMuseeFinding> Findings;
	GatherArt(Tests::Hangings(), Pieces, Findings);
	TestTrue(TEXT("A handscroll on silk, not made yet, is the one finding"), Findings.Num() == 1 && Findings[0].Summary.Contains(TEXT("handscroll")));
	const FFormMadePiece* Face = Tests::FindPiece(Pieces, TEXT("panel.monet-impression-sunrise"));
	const FFormMadePiece* Board = Tests::FindPiece(Pieces, TEXT("panel.monet-impression-sunrise.board"));
	const FFormMadePiece* Cradle = Tests::FindPiece(Pieces, TEXT("panel.monet-impression-sunrise.cradle"));
	const FFormMadePiece* Frame = Tests::FindPiece(Pieces, TEXT("panel.monet-impression-sunrise.frame"));
	if (TestTrue(TEXT("A panel is made of its painted face, its board and its cradle, in its frame"), Face && Board && Cradle && Frame))
	{
		TestTrue(TEXT("Its face is the work's own layer over the paint on its gesso"), Face->Material == TEXT("paint.monet-impression-sunrise")
			&& Face->Working == TEXT("varnished.gesso") && Face->History == TEXT("clean"));
		// From the wall out: the hanging gap 2 mm, the cradle 15 mm, the board 8 mm, the ground and paint 0.5 mm.
		const FBox FaceBox = Tests::StageBox(*Face);
		TestTrue(TEXT("at its place and size, 25.5 mm off the wall"), FaceBox.Min.Equals(FVector(1.0, -5.0 + 0.025, 1.2), 1e-6)
			&& FaceBox.Max.Equals(FVector(1.5, -5.0 + 0.0255, 1.6), 1e-6));
		const FBox BoardBox = Tests::StageBox(*Board);
		TestTrue(TEXT("The board behind it, in the wood its wall surface names"), FMath::IsNearlyEqual(BoardBox.Min.Y, -5.0 + 0.017, 1e-6)
			&& FMath::IsNearlyEqual(BoardBox.Max.Y, -5.0 + 0.025, 1e-6) && Board->Surface == TEXT("hangings.panel") && Board->Material.IsNone());
		// The grain runs along the landscape panel's width: four members across its height, the outermost 20 mm in.
		const FBox CradleBox = Tests::StageBox(*Cradle);
		TestTrue(TEXT("The cradle between the board and the hanging gap, its members and slides 20 mm in from the edges"),
			CradleBox.Min.Equals(FVector(1.0, -5.0 + 0.002, 1.22), 1e-6) && CradleBox.Max.Equals(FVector(1.5, -5.0 + 0.017, 1.58), 1e-6));
		TestEqual(TEXT("four members and four slides, each a box"), Cradle->Mesh->TriangleCount(), 8 * 12);
		TestTrue(TEXT("The frame is one closed solid, its back on the hanging gap"), Musee::FormMaking::MeasureShell(*Frame).IsClosed()
			&& FMath::IsNearlyEqual(Tests::StageBox(*Frame).Min.Y, -5.0 + 0.002, 1e-6));
	}
	const FFormMadePiece* Weave = Tests::FindPiece(Pieces, TEXT("tapestry.monet-willows-two-willows"));
	if (TestNotNull(TEXT("A tapestry is its weave"), Weave))
	{
		TestTrue(TEXT("the work's own layer, woven"), Weave->Material == TEXT("paint.monet-willows-two-willows") && Weave->Working == TEXT("woven"));
		const FBox WeaveBox = Tests::StageBox(*Weave);
		TestTrue(TEXT("3 mm thick, its back 47 mm off the wall on its lining"), WeaveBox.Min.Equals(FVector(3.0, -5.0 + 0.047, 1.0), 1e-6)
			&& WeaveBox.Max.Equals(FVector(5.0, -5.0 + 0.05, 2.5), 1e-6));
	}
	const TArray<FArtPaintLayer> Layers = PaintLayers(Tests::Hangings());
	const FArtPaintLayer* Panel = Layers.FindByPredicate([](const FArtPaintLayer& L) { return L.Work == TEXT("monet-impression-sunrise"); });
	const FArtPaintLayer* Tapestry = Layers.FindByPredicate([](const FArtPaintLayer& L) { return L.Work == TEXT("monet-willows-two-willows"); });
	TestTrue(TEXT("A panel's layer is over the paint on panel, a tapestry's over its weave"), Panel && Panel->Parent == TEXT("accent.oil-on-panel")
		&& Tapestry && Tapestry->Parent == TEXT("accent.tapestry") && Tapestry->SizeMeters.Equals(FVector2D(2.0, 1.5)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtHangingScrollTest, "Musee.Art.HangingScroll", Musee::ArtMaking::Tests::Flags)

bool FArtHangingScrollTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	TArray<FFormMadePiece> Pieces;
	TArray<FMuseeFinding> Findings;
	GatherArt(Tests::Hangings(), Pieces, Findings);
	const FFormMadePiece* Painting = Tests::FindPiece(Pieces, TEXT("silk.lumiere-peacock"));
	const FFormMadePiece* Mount = Tests::FindPiece(Pieces, TEXT("silk.lumiere-peacock.mount"));
	const FFormMadePiece* Stick = Tests::FindPiece(Pieces, TEXT("silk.lumiere-peacock.stick"));
	const FFormMadePiece* Roller = Tests::FindPiece(Pieces, TEXT("silk.lumiere-peacock.roller"));
	const FFormMadePiece* Left = Tests::FindPiece(Pieces, TEXT("silk.lumiere-peacock.knob.left"));
	const FFormMadePiece* Right = Tests::FindPiece(Pieces, TEXT("silk.lumiere-peacock.knob.right"));
	const FFormMadePiece* Cord = Tests::FindPiece(Pieces, TEXT("silk.lumiere-peacock.cord.left"));
	if (!TestTrue(TEXT("A hanging scroll is its painting in its mount, its stick, roller, knobs and cord"), Painting && Mount && Stick && Roller && Left
		&& Right && Cord && Tests::FindPiece(Pieces, TEXT("silk.lumiere-peacock.cord.right"))))
	{
		return false;
	}
	TestTrue(TEXT("The painting is the work's own layer over the ink on its silk"), Painting->Material == TEXT("paint.lumiere-peacock")
		&& Painting->Working == TEXT("ink-painted"));
	// The case's wall is 0.5 m up; the mount's back 20 mm off it and 0.3 mm thick.
	const FBox PaintingBox = Tests::StageBox(*Painting);
	TestTrue(TEXT("The painting at its place and size, flush with its mount"), PaintingBox.Min.Equals(FVector(8.2, -5.0 + 0.02, 1.0), 1e-6)
		&& PaintingBox.Max.Equals(FVector(8.6, -5.0 + 0.0203, 1.8), 1e-6));
	// Borders 0.04 either side; below, a band 0.06 and the earth 0.2; above, a band and the heaven 0.3.
	const FBox MountBox = Tests::StageBox(*Mount);
	TestTrue(TEXT("The mount round it in the frame surface's silk"), MountBox.Min.Equals(FVector(8.16, -5.0 + 0.02, 0.74), 1e-6)
		&& MountBox.Max.Equals(FVector(8.64, -5.0 + 0.0203, 2.16), 1e-6) && Mount->Surface == TEXT("hangings.mount.silk"));
	TestTrue(TEXT("one closed solid with the painting's window through it"), Musee::FormMaking::MeasureShell(*Mount).IsClosed()
		&& FMath::IsNearlyEqual(Tests::AreaSquareMeters(Mount->Mesh), 2.0 * (0.48 * 1.42 - 0.4 * 0.8) + 2.0 * (0.48 + 1.42 + 0.4 + 0.8) * 0.0003, 1e-5));
	const FBox StickBox = Tests::StageBox(*Stick);
	// Round parts are drawn as polygons within a chord's sag of their circles.
	TestTrue(TEXT("The stick behind the mount's top edge, its middle on it, its round towards the wall"), FMath::IsNearlyEqual(StickBox.Max.Z, 2.168, 1e-3)
		&& FMath::IsNearlyEqual(StickBox.Min.Z, 2.152, 1e-3) && FMath::IsNearlyEqual(StickBox.Max.Y, -5.0 + 0.02, 1e-6)
		&& FMath::IsNearlyEqual(StickBox.Min.Y, -5.0 + 0.012, 1e-3));
	const FBox RollerBox = Tests::StageBox(*Roller);
	TestTrue(TEXT("The roller along the mount's foot"), FMath::IsNearlyEqual(RollerBox.Min.Z, 0.74, 1e-3) && FMath::IsNearlyEqual(RollerBox.Min.X, 8.16, 1e-6)
		&& FMath::IsNearlyEqual(RollerBox.Max.X, 8.64, 1e-6));
	TestTrue(TEXT("its knobs past the mount's edges, in their own surface"), FMath::IsNearlyEqual(Tests::StageBox(*Left).Min.X, 8.125, 1e-6)
		&& FMath::IsNearlyEqual(Tests::StageBox(*Right).Max.X, 8.675, 1e-6) && Left->Surface == TEXT("hangings.mount.knob"));
	const FBox CordBox = Tests::StageBox(*Cord);
	TestTrue(TEXT("The cord rises from the stick to its hook 0.093 above the stick's middle"), CordBox.Max.Z > 2.16 + 0.093 - 1e-3
		&& CordBox.Max.Z < 2.16 + 0.093 + 0.0016 && Cord->Surface == TEXT("hangings.mount.cord"));
	const TArray<FArtPaintLayer> Layers = PaintLayers(Tests::Hangings());
	const FArtPaintLayer* Layer = Layers.FindByPredicate([](const FArtPaintLayer& L) { return L.Work == TEXT("lumiere-peacock"); });
	TestTrue(TEXT("Its layer is the painting's image at its size over the ink on silk"), Layer && Layer->Parent == TEXT("accent.ink-on-silk")
		&& Layer->Working == TEXT("ink-painted") && Layer->SizeMeters.Equals(FVector2D(0.4, 0.8)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtBookTest, "Musee.Art.Book", Musee::ArtMaking::Tests::Flags)

bool FArtBookTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	TArray<FFormMadePiece> Pieces;
	TArray<FMuseeFinding> Findings;
	GatherArt(Tests::Hangings(), Pieces, Findings);
	// The catalogue lies on the bed (z 0.864), its reader on the -y side, so its reader's right is -x.
	const FFormMadePiece* Verso = Tests::FindPiece(Pieces, TEXT("paper.salon-1874-case.0.verso"));
	const FFormMadePiece* Recto = Tests::FindPiece(Pieces, TEXT("paper.salon-1874-case.0.recto"));
	const FFormMadePiece* Cover = Tests::FindPiece(Pieces, TEXT("paper.salon-1874-case.0.board.verso"));
	const FFormMadePiece* Leaf = Tests::FindPiece(Pieces, TEXT("paper.salon-1874-case.1"));
	if (TestTrue(TEXT("An opening shown whole is its two pages on their leaves and covers, beside a loose leaf"), Verso && Recto && Cover && Leaf
		&& Tests::FindPiece(Pieces, TEXT("paper.salon-1874-case.0.block.recto"))))
	{
		// Of its 16 leaves (1.6 mm), the seventh opening leaves 7 on the verso's side and 9 on the recto's, on a 0.2 mm wrapper.
		const FBox VersoBox = Tests::StageBox(*Verso);
		const FBox RectoBox = Tests::StageBox(*Recto);
		TestTrue(TEXT("The verso on the reader's left, as high as its leaves"), VersoBox.Min.Equals(FVector(6.0, -0.092, 0.864 + 0.0006), 1e-6)
			&& VersoBox.Max.Equals(FVector(6.125, 0.092, 0.864 + 0.0009), 1e-6));
		TestTrue(TEXT("the recto on the right, higher"), RectoBox.Min.Equals(FVector(5.875, -0.092, 0.864 + 0.0008), 1e-6)
			&& FMath::IsNearlyEqual(RectoBox.Max.Z, 0.864 + 0.0011, 1e-6));
		// Its faces' UV0 runs from 0.125 to 0.25 across the image; its edges' only along their own length.
		const UE::Geometry::FDynamicMeshUVOverlay* UVs = Recto->Mesh->Attributes()->GetUVLayer(0);
		float Most = TNumericLimits<float>::Lowest();
		for (const int32 E : UVs->ElementIndicesItr())
		{
			Most = FMath::Max(Most, UVs->GetElement(E).X);
		}
		TestTrue(TEXT("the recto showing the image's right half"), FMath::IsNearlyEqual(Most, 0.25f, 1e-4f));
		TestTrue(TEXT("Its covers in its binding's finish"), Cover->Material == TEXT("accent.print-on-paper") && Recto->Material == TEXT("paint.salon-1874-case.0"));
		const FBox LeafBox = Tests::StageBox(*Leaf);
		TestTrue(TEXT("The leaf lies flat at its centre, its own layer"), LeafBox.Min.Equals(FVector(6.34, -0.2365, 0.864), 1e-6)
			&& LeafBox.Max.Equals(FVector(6.66, 0.2365, 0.864 + 0.0003), 1e-6) && Leaf->Material == TEXT("paint.salon-1874-case.1"));
	}
	// The Chaucer on its sloping desk, its verso and recto two objects: at its first opening one leaf lies on the verso's side.
	const FFormMadePiece* Title = Tests::FindPiece(Pieces, TEXT("paper.albion-kelmscott-chaucer.0"));
	const FFormMadePiece* First = Tests::FindPiece(Pieces, TEXT("paper.albion-kelmscott-chaucer.1"));
	if (TestTrue(TEXT("A book given as its verso and recto is made of both"), Title && First && Tests::FindPiece(Pieces,
		TEXT("paper.albion-kelmscott-chaucer.1.block.recto"))))
	{
		TestFalse(TEXT("the verso's one leaf is its page alone"), Pieces.ContainsByPredicate([](const FFormMadePiece& P)
		{
			return P.Name == TEXT("paper.albion-kelmscott-chaucer.0.block.verso");
		}));
		TestTrue(TEXT("its verso on the reader's left (-y), its recto on the right"), Tests::StageBox(*Title).Max.Y < 1e-6 && Tests::StageBox(*First).Min.Y > -1e-6);
		// The recto's page stands its board and its 276 leaves off the desk, along the desk's normal.
		const double Slope = FMath::DegreesToRadians(18.0);
		const FVector Normal(-FMath::Sin(Slope), 0.0, FMath::Cos(Slope));
		double Highest = -1.0;
		for (const int32 V : First->Mesh->VertexIndicesItr())
		{
			Highest = FMath::Max(Highest, (First->Frame.PointToParent(Musee::FromEngine(First->Mesh->GetVertex(V))) - FVector(10.0, 0.0, 1.0)) | Normal);
		}
		TestEqual(TEXT("its leaves as thick as the block holds them"), Highest, 0.003 + 276.0 * 0.066 / 277.0, 1e-6);
	}
	return true;
}

#endif
