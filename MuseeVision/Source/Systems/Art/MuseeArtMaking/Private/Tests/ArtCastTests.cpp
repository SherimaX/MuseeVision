#include "ArtCastScan.h"
#include "ArtHang.h"
#include "ArtHangReading.h"
#include "ArtPaintLayer.h"
#include "ArtPieces.h"
#include "ArtWork.h"
#include "Cast/CastMesh.h"
#include "Cast/ScanMesh.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "FormMesh.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "MuseeUnits.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::ArtMaking::CastTests
{
	/** Art's making tests run in the editor, where its making twin lives, and belong to the product's own filter. */
	inline constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	FString Fixture(const FString& Name)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir() / TEXT("Systems/Art/MuseeArtMaking/Private/Tests/Data") / Name);
	}

	/** Adds a box to a scan, its corners' normals pointing out from its centre. */
	void AddBox(FArtScanMesh& Scan, const FVector3f& Min, const FVector3f& Max)
	{
		const int32 First = Scan.Points.Num();
		const FVector3f Centre = (Min + Max) * 0.5f;
		for (int32 Corner = 0; Corner < 8; ++Corner)
		{
			const FVector3f At((Corner & 1) ? Max.X : Min.X, (Corner & 2) ? Max.Y : Min.Y, (Corner & 4) ? Max.Z : Min.Z);
			Scan.Points.Add(At);
			Scan.Normals.Add((At - Centre).GetSafeNormal());
		}
		const int32 Faces[6][4] = { { 0, 2, 3, 1 }, { 4, 5, 7, 6 }, { 0, 1, 5, 4 }, { 2, 6, 7, 3 }, { 0, 4, 6, 2 }, { 1, 3, 7, 5 } };
		for (const int32* Face : Faces)
		{
			Scan.Triangles.Add(FIntVector(First + Face[0], First + Face[1], First + Face[2]));
			Scan.Triangles.Add(FIntVector(First + Face[0], First + Face[2], First + Face[3]));
		}
	}

	/**
	 * A scan in centimetres, as the glTF import writes one: a block 20 x 20 x 10 whose foot is centred at (50, 20), and on
	 * it a column that leans to the right, from x 40 to 100, up to 200.
	 */
	FArtScanMesh LeaningScan()
	{
		FArtScanMesh Scan;
		AddBox(Scan, FVector3f(40.0f, 10.0f, 0.0f), FVector3f(60.0f, 30.0f, 10.0f));
		AddBox(Scan, FVector3f(40.0f, 10.0f, 10.0f), FVector3f(100.0f, 30.0f, 200.0f));
		return Scan;
	}

	FBox StageBox(const FFormMadePiece& Piece)
	{
		FBox Box(ForceInit);
		for (const int32 V : Piece.Mesh->VertexIndicesItr())
		{
			Box += Piece.Frame.PointToParent(Musee::FromEngine(Piece.Mesh->GetVertex(V)));
		}
		return Box;
	}

	/** The box of a piece's points in the stage that stand below a height, metres. */
	FBox StageBoxBelow(const FFormMadePiece& Piece, double Height)
	{
		FBox Box(ForceInit);
		for (const int32 V : Piece.Mesh->VertexIndicesItr())
		{
			const FVector At = Piece.Frame.PointToParent(Musee::FromEngine(Piece.Mesh->GetVertex(V)));
			if (At.Z < Height)
			{
				Box += At;
			}
		}
		return Box;
	}

	FMuseeStageContext Round()
	{
		FMuseeStageContext Context;
		Context.Stage = FMuseeKey(TEXT("round"));
		Context.ScheduleDir = Fixture(TEXT("Stages/round"));
		FMuseeStageAnchor& Plinth = Context.Anchors.AddDefaulted_GetRef();
		Plinth.Key = FMuseeKey(TEXT("round.plinth"));
		Plinth.Kind = TEXT("Mount");
		Plinth.Frame = FMuseeFrame(FVector(2.0, 3.0, 1.0));
		return Context;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtCastFitTest, "Musee.Art.CastFit", Musee::ArtMaking::CastTests::Flags)

bool FArtCastFitTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	const FArtScanMesh Scan = CastTests::LeaningScan();
	const FCastFit Fit = FitCast(Scan, 2.0, 0.04);
	TestTrue(TEXT("A scan 200 units high stands 2 m at 0.01 m a unit"), FMath::IsNearlyEqual(Fit.Scale, 0.01, 1e-12));
	TestTrue(TEXT("Its foot is its base slice's centre at its lowest point, not the centre of its lean"), Fit.Foot.Equals(FVector(50.0, 20.0, 0.0), 1e-9));
	TestTrue(TEXT("Its width, depth and base are measured at its true size"), FMath::IsNearlyEqual(Fit.WidthMeters, 0.6, 1e-9)
		&& FMath::IsNearlyEqual(Fit.DepthMeters, 0.2, 1e-9) && Fit.BaseMeters.Equals(FVector2D(0.2, 0.2), 1e-9));
	TestFalse(TEXT("A scan without height can't be fitted"), FitCast(FArtScanMesh(), 2.0, 0.04).IsValid());

	TArray<FFormMesh> Parts;
	const FCastBand Band{ 1.0, 1.5 };
	BuildCast(Scan, Fit, MakeArrayView(&Band, 1), Parts);
	if (!TestEqual(TEXT("A body and one part"), Parts.Num(), 2))
	{
		return false;
	}
	TestEqual(TEXT("Every triangle is cast, in the body or its part"), Parts[0].NumTriangles() + Parts[1].NumTriangles(), 24);
	TestTrue(TEXT("The part takes the triangles whose centre stands in its band"), Parts[1].NumTriangles() > 0 && Parts[0].NumTriangles() > Parts[1].NumTriangles());
	FBox Box(ForceInit);
	for (const FFormMesh& Part : Parts)
	{
		for (const FVector& At : Part.AllPositions())
		{
			Box += At;
		}
	}
	TestTrue(TEXT("It stands on its foot, 2 m high, leaning 0.5 m to the right"), Box.Min.Equals(FVector(-0.1, -0.1, 0.0), 1e-9) && Box.Max.Equals(FVector(0.5, 0.1, 2.0), 1e-9));
	UE::Geometry::FDynamicMesh3 Written;
	Parts[0].Write(Written);
	const UE::Geometry::FDynamicMeshUVOverlay* UVs = Written.Attributes()->GetUVLayer(0);
	FVector2f Least(TNumericLimits<float>::Max()), Most(TNumericLimits<float>::Lowest());
	for (const int32 E : UVs->ElementIndicesItr())
	{
		Least = FVector2f::Min(Least, UVs->GetElement(E));
		Most = FVector2f::Max(Most, UVs->GetElement(E));
	}
	TestTrue(TEXT("Its UV0 is its faces projected in metres, up reading as -v"), Least.Equals(FVector2f(-0.1f, -2.0f), 1e-5f) && Most.Equals(FVector2f(0.5f, 0.1f), 1e-5f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtMvmTest, "Musee.Art.Mvm", Musee::ArtMaking::CastTests::Flags)

bool FArtMvmTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	FArtScanMesh Scan;
	FString Why;
	if (!TestTrue(TEXT("An .mvm reads"), ReadMvm(CastTests::Fixture(TEXT("scans/wedge.mvm")), Scan, Why)))
	{
		AddError(Why);
		return false;
	}
	TestTrue(TEXT("Its four points and four triangles"), Scan.Points.Num() == 4 && Scan.Normals.Num() == 4 && Scan.Triangles.Num() == 4);
	TestTrue(TEXT("Its y-up apex stands up z"), Scan.Points[3].Equals(FVector3f(0.0f, 0.0f, 0.3f), 1e-6f));
	TestTrue(TEXT("Its front (+z) faces y"), Scan.Points[2].Equals(FVector3f(0.0f, 0.1f, 0.0f), 1e-6f));
	TestTrue(TEXT("Its right stays x"), Scan.Points[1].Equals(FVector3f(0.1f, -0.05f, 0.0f), 1e-6f));
	TestTrue(TEXT("Its normals are unit"), FMath::IsNearlyEqual(Scan.Normals[3].Size(), 1.0f, 1e-5f));
	TestFalse(TEXT("A glTF file is not an .mvm"), ReadMvm(CastTests::Fixture(TEXT("scans/wedge.glb")), Scan, Why));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtCastTest, "Musee.Art.Cast", Musee::ArtMaking::CastTests::Flags)

bool FArtCastTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	TArray<FFormMadePiece> Pieces;
	TArray<FMuseeFinding> Findings;
	GatherArt(CastTests::Round(), Pieces, Findings);
	const FFormMadePiece* Figure = Pieces.FindByPredicate([](const FFormMadePiece& P) { return P.Name == TEXT("cast.test-cast-figure"); });
	if (!TestNotNull(TEXT("The figure is cast from its scan"), Figure))
	{
		for (const FMuseeFinding& Finding : Findings)
		{
			AddInfo(Finding.ToString());
		}
		return false;
	}
	TestTrue(TEXT("in its medium's finish: plaster, sized, its seams kept"), Figure->Material == TEXT("white.plaster-of-paris")
		&& Figure->Working == TEXT("cast.seams-kept.sized") && Figure->History == TEXT("new"));
	const FBox Box = CastTests::StageBox(*Figure);
	TestTrue(TEXT("It stands on its carrier's top at its true height"), FMath::IsNearlyEqual(Box.Min.Z, 1.0, 1e-4) && FMath::IsNearlyEqual(Box.Max.Z, 1.994, 1e-4));
	TestTrue(TEXT("Facing +y, its width runs along x"), FMath::IsNearlyEqual(Box.GetSize().X, 0.37, 0.005));
	const FBox Base = CastTests::StageBoxBelow(*Figure, 1.04);
	TestTrue(TEXT("Its base slice is centred on the plan's centre"), FMath::IsNearlyEqual(Base.GetCenter().X, 2.0, 1e-3) && FMath::IsNearlyEqual(Base.GetCenter().Y, 3.0, 1e-3));
	TestTrue(TEXT("A plan whose width isn't its scan's is a finding"), Findings.ContainsByPredicate([](const FMuseeFinding& F)
	{
		return F.ToString().Contains(TEXT("test-cast-misread")) && F.ToString().Contains(TEXT("wide"));
	}));
	TestFalse(TEXT("and that work isn't made"), Pieces.ContainsByPredicate([](const FFormMadePiece& P) { return P.Name == TEXT("cast.test-cast-misread"); }));
	TestTrue(TEXT("A work in the round shows no image: no paint layer"), PaintLayers(CastTests::Round()).IsEmpty());

	const FFormMadePiece* Vase = Pieces.FindByPredicate([](const FFormMadePiece& P) { return P.Name == TEXT("ware.chinese-longquan-vase"); });
	const FFormMadePiece* Stand = Pieces.FindByPredicate([](const FFormMadePiece& P) { return P.Name == TEXT("ware.chinese-longquan-vase.stand"); });
	if (TestTrue(TEXT("A vessel without a scan is turned from its section, on its stand"), Vase && Stand))
	{
		TestTrue(TEXT("in its medium's glaze: celadon over stoneware"), Vase->Material == TEXT("accent.stoneware-glazed") && Vase->Working == TEXT("glazed")
			&& Vase->History == TEXT("clean"));
		TestTrue(TEXT("its stand in phoebe"), Stand->Material == TEXT("accent.phoebe"));
		const FBox VaseBox = CastTests::StageBox(*Vase);
		const FBox StandBox = CastTests::StageBox(*Stand);
		TestTrue(TEXT("The stand is low under a vessel under 0.20 m, on the deck"), FMath::IsNearlyEqual(StandBox.Min.Z, 1.0, 1e-6)
			&& FMath::IsNearlyEqual(StandBox.Max.Z, 1.028, 1e-6));
		TestTrue(TEXT("The vessel stands on it, its plan's size"), FMath::IsNearlyEqual(VaseBox.Min.Z, 1.028, 1e-6) && FMath::IsNearlyEqual(VaseBox.Max.Z, 1.174, 1e-6)
			&& FMath::IsNearlyEqual(VaseBox.GetSize().X, 0.083, 1e-3) && FMath::IsNearlyEqual(VaseBox.GetCenter().X, 3.0, 1e-3));
		TestTrue(TEXT("The stand is wider than the foot, narrower than the vessel"), StandBox.GetSize().X > 0.02 && StandBox.GetSize().X < VaseBox.GetSize().X);
	}

	FArtHang Hang;
	TArray<FMuseeFinding> Read;
	if (TestTrue(TEXT("The test plan reads"), ReadStageHang(CastTests::Round(), Hang, Read)))
	{
		const TValueOrError<FArtWork, FString> Work = Musee::Art::ReadWork(Hang.Works[0]);
		TestTrue(TEXT("Its label credits the scan as its licence asks"), Work.HasValue() && Work.GetValue().Caption().Credit.ToString().Contains(TEXT("passivactivist")));
		FArtHangEntry Uncredited = Hang.Works[0];
		Uncredited.Scan.Mesh = TEXT("assets/sculptures/no-such-scan.mvm");
		TestFalse(TEXT("A scan without a credit is refused"), Musee::Art::ReadWork(Uncredited).HasValue());
		FArtHangEntry Glb = Hang.Works[0];
		Glb.Scan.Mesh = TEXT("assets/classical/augustus-prima-porta.glb");
		TestEqual(TEXT("A glTF scan is imported under Art/Casts"), ScanPackage(TEXT("/Game/Generated"), ScanName(Glb)),
			FString(TEXT("/Game/Generated/Art/Casts/SM_Scan_AugustusPrimaPorta")));
	}
	return true;
}

#endif
