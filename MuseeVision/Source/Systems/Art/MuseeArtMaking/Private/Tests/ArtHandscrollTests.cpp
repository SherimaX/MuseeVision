#include "ArtHangEntry.h"
#include "ArtPaintLayer.h"
#include "ArtPieces.h"
#include "ArtWork.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "MuseeUnits.h"
#include "Scroll/ScrollLayout.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::ArtMaking::HandscrollTests
{
	/** Art's making tests run in the editor, where its making twin lives, and belong to the product's own filter. */
	inline constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	/** A round part is drawn as a polygon within a chord's sag of its circle. */
	constexpr double Sag = 1e-3;

	/**
	 * The test scroll case: a level bed 1 m up (a Wall anchor, as Chenghuai's beds are), x along it, its face up, and up
	 * its slope (its z) towards -y, away from the reader who stands at +y.
	 */
	FMuseeStageContext Scrolls()
	{
		FMuseeStageContext Context;
		Context.Stage = FMuseeKey(TEXT("scrolls"));
		Context.ScheduleDir = FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir() / TEXT("Systems/Art/MuseeArtMaking/Private/Tests/Data/Stages/scrolls"));
		FMuseeStageAnchor Bed;
		Bed.Key = FMuseeKey(TEXT("scrolls.bed"));
		Bed.Kind = TEXT("Wall");
		Bed.Frame = FMuseeFrame(FVector(0.0, 0.0, 1.0), FQuat(FMatrix(FVector(1.0, 0.0, 0.0), FVector(0.0, 0.0, 1.0), FVector(0.0, -1.0, 0.0), FVector::ZeroVector)));
		Context.Anchors = { Bed };
		return Context;
	}

	const FFormMadePiece* FindPiece(const TArray<FFormMadePiece>& Pieces, const TCHAR* Name)
	{
		return Pieces.FindByPredicate([Name](const FFormMadePiece& P) { return P.Name == Name; });
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtScrollLayoutTest, "Musee.Art.ScrollLayout", Musee::ArtMaking::HandscrollTests::Flags)

bool FArtScrollLayoutTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	// Two photographs of 10 : 1 on a sheet of 19.6 : 1: 2 % long, so laid at the sheet's height, 0.2 cut from each end.
	const FScrollImage Long[] = { { FIntPoint(100, 10) }, { FIntPoint(100, 10) } };
	const FScrollLayout Laid = LayScroll(Long, FVector2D(19.6, 1.0));
	if (TestEqual(TEXT("A panel for each photograph"), Laid.Panels.Num(), 2))
	{
		TestEqual(TEXT("they stray 2 % long"), Laid.Stray, 20.0 / 19.6 - 1.0, 1e-9);
		TestTrue(TEXT("at the sheet's height, cut evenly at the two ends"), FMath::IsNearlyEqual(Laid.ImageTopMeters, 0.0) && FMath::IsNearlyEqual(Laid.CutMeters, 0.2));
		const FScrollPanel& Left = Laid.Panels[0];
		const FScrollPanel& Right = Laid.Panels[1];
		TestTrue(TEXT("the first from the sheet's left edge, 0.2 into its photograph"), FMath::IsNearlyEqual(Left.AlongMeters, 0.0)
			&& FMath::IsNearlyEqual(Left.LengthMeters, 9.8) && FMath::IsNearlyEqual(Left.ImageLeftMeters, 0.2));
		TestTrue(TEXT("the second joined to it without a gap, to the sheet's right edge"), FMath::IsNearlyEqual(Right.AlongMeters, 9.8)
			&& FMath::IsNearlyEqual(Right.AlongMeters + Right.LengthMeters, 19.6) && FMath::IsNearlyEqual(Right.ImageLeftMeters, 0.0));
		TestTrue(TEXT("each at its own proportion, never stretched"), Left.ImageMeters.Equals(FVector2D(10.0, 1.0)) && Right.ImageMeters.Equals(FVector2D(10.0, 1.0)));
	}
	// The same photographs, the second's last fifth its mount, on a sheet of 18 : 1: 0 % stray, nothing cut.
	const FScrollImage Windowed[] = { { FIntPoint(100, 10) }, { FIntPoint(100, 10), 0.0, 0.8 } };
	const FScrollLayout Framed = LayScroll(Windowed, FVector2D(18.0, 1.0));
	TestTrue(TEXT("A window keeps the mount off the sheet"), Framed.Panels.Num() == 2 && FMath::IsNearlyZero(Framed.Stray, 1e-9)
		&& FMath::IsNearlyEqual(Framed.Panels[1].LengthMeters, 8.0) && FMath::IsNearlyZero(Framed.CutMeters, 1e-9));
	// One photograph of 10 : 1 on a sheet of 10.5 : 1: short, so laid at the sheet's length and cut at its head and foot.
	const FScrollImage Short[] = { { FIntPoint(100, 10) } };
	const FScrollLayout Covered = LayScroll(Short, FVector2D(10.5, 1.0));
	TestTrue(TEXT("A short photograph covers the sheet at its length, cut evenly at its head and foot"), Covered.Panels.Num() == 1
		&& Covered.Stray < 0.0 && FMath::IsNearlyEqual(Covered.ImageTopMeters, 0.025) && FMath::IsNearlyEqual(Covered.Panels[0].ImageMeters.X, 10.5)
		&& FMath::IsNearlyEqual(Covered.Panels[0].LengthMeters, 10.5));
	TestTrue(TEXT("Nothing to lay lays nothing"), LayScroll(TConstArrayView<FScrollImage>(), FVector2D(1.0, 1.0)).Panels.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtSectionImagesTest, "Musee.Art.SectionImages", Musee::ArtMaking::HandscrollTests::Flags)

bool FArtSectionImagesTest::RunTest(const FString& Parameters)
{
	FArtHangEntry Entry;
	Entry.Work = TEXT("chinese-thousand-li");
	Entry.Source = TEXT("assets/collection.json");
	const TArray<FString> Sections = Musee::Art::FindSectionImages(Entry);
	// The high-resolution folder holds four copies of one whole view under the sections' names; they are no sections.
	if (TestEqual(TEXT("The Thousand Li's four photographs are found"), Sections.Num(), 4))
	{
		for (int32 Index = 0; Index < Sections.Num(); ++Index)
		{
			TestTrue(*FString::Printf(TEXT("-%d is its own cut, from the working copies"), Index + 1),
				FPaths::GetCleanFilename(Sections[Index]) == FString::Printf(TEXT("chinese-thousand-li-%d.jpg"), Index + 1)
				&& FPaths::GetCleanFilename(FPaths::GetPath(Sections[Index])) == TEXT("paintings"));
		}
	}
	Entry.Work = TEXT("chinese-orchid-pavilion");
	TestTrue(TEXT("A work in one photograph has no sections"), Musee::Art::FindSectionImages(Entry).IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtHandscrollTest, "Musee.Art.Handscroll", Musee::ArtMaking::HandscrollTests::Flags)

bool FArtHandscrollTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	using namespace Musee::ArtMaking::HandscrollTests;
	TArray<FFormMadePiece> Pieces;
	TArray<FMuseeFinding> Findings;
	GatherArt(Scrolls(), Pieces, Findings);
	TestTrue(TEXT("The one finding: the Qingming's photographs are short of its catalogued length"), Findings.Num() == 1
		&& Findings[0].Summary.Contains(TEXT("chinese-qingming")) && Findings[0].Summary.Contains(TEXT("shorter")));
	const FFormMadePiece* End = FindPiece(Pieces, TEXT("paper.chinese-huaisu-autobiography.section.1"));
	const FFormMadePiece* Start = FindPiece(Pieces, TEXT("paper.chinese-huaisu-autobiography.section.2"));
	if (!TestTrue(TEXT("The Autobiography is its two photographs, each a panel of its sheet"), Start && End))
	{
		return false;
	}
	// Reading order: -2 is the beginning, which lies at the right, where the reader starts.
	const FBox StartBox = StageBox(*Start);
	const FBox EndBox = StageBox(*End);
	TestTrue(TEXT("Read from right to left: -2, the beginning, at the right; -1, the end, at the left, joined without a gap"),
		StartBox.Min.X > EndBox.Min.X && FMath::IsNearlyEqual(StartBox.Min.X, EndBox.Max.X, 1e-6));
	TestTrue(TEXT("At its true length, 7.55 m from its along"), FMath::IsNearlyEqual(EndBox.Min.X, 0.2, 1e-6) && FMath::IsNearlyEqual(StartBox.Max.X, 7.75, 1e-6));
	TestTrue(TEXT("Its height up the bed, lying on it, 0.3 mm thick"), FMath::IsNearlyEqual(StartBox.Max.Y, -0.1, 1e-6)
		&& FMath::IsNearlyEqual(StartBox.Min.Y, -0.383, 1e-6) && FMath::IsNearlyEqual(StartBox.Max.Z, 1.0003, 1e-6));
	TestTrue(TEXT("Each panel in its photograph's own layer over the ink on its paper"), Start->Material == TEXT("paint.chinese-huaisu-autobiography.2")
		&& End->Material == TEXT("paint.chinese-huaisu-autobiography.1") && Start->Working == TEXT("ink-painted"));
	// The mount: borders 12 mm, a band 25 mm at each end, the lead 60 mm beyond the beginning.
	const FFormMadePiece* Mount = FindPiece(Pieces, TEXT("paper.chinese-huaisu-autobiography.mount"));
	if (TestNotNull(TEXT("The mount"), Mount))
	{
		const FBox Box = StageBox(*Mount);
		TestTrue(TEXT("in the mount's silk, from the left roll to the lead's end, its borders above and below"), Mount->Surface == TEXT("scrolls.mount.silk")
			&& FMath::IsNearlyEqual(Box.Min.X, 0.175, 1e-6) && FMath::IsNearlyEqual(Box.Max.X, 7.835, 1e-6)
			&& FMath::IsNearlyEqual(Box.Max.Y, -0.088, 1e-6) && FMath::IsNearlyEqual(Box.Min.Y, -0.395, 1e-6));
	}
	// The rolled ends: the unshown length on the roller at the left, the wrapper on the stave at the right.
	const FFormMadePiece* Roll = FindPiece(Pieces, TEXT("paper.chinese-huaisu-autobiography.roll"));
	const FFormMadePiece* Roller = FindPiece(Pieces, TEXT("paper.chinese-huaisu-autobiography.roller"));
	const FFormMadePiece* Foot = FindPiece(Pieces, TEXT("paper.chinese-huaisu-autobiography.knob.foot"));
	const FFormMadePiece* Head = FindPiece(Pieces, TEXT("paper.chinese-huaisu-autobiography.knob.head"));
	const FFormMadePiece* Wrapper = FindPiece(Pieces, TEXT("paper.chinese-huaisu-autobiography.wrapper"));
	const FFormMadePiece* Stave = FindPiece(Pieces, TEXT("paper.chinese-huaisu-autobiography.stave"));
	if (TestTrue(TEXT("Its rolled ends: the roll, roller and knobs at the left, the wrapper and stave at the right"), Roll && Roller && Foot && Head
		&& Wrapper && Stave))
	{
		const FBox RollBox = StageBox(*Roll);
		TestTrue(TEXT("The roll at the left, 25 mm in radius, resting on the silk where the band ends"), FMath::IsNearlyEqual(RollBox.Min.X, 0.15, Sag)
			&& FMath::IsNearlyEqual(RollBox.Max.X, 0.2, Sag) && FMath::IsNearlyEqual(RollBox.Min.Z, 1.0003, Sag) && FMath::IsNearlyEqual(RollBox.Max.Z, 1.0503, Sag));
		TestTrue(TEXT("its roller in bare fir, its knobs past the foot and head in their surface"), Roller->Material == TEXT("accent.chinese-fir")
			&& Foot->Surface == TEXT("scrolls.mount.knob") && FMath::IsNearlyEqual(StageBox(*Foot).Max.Y, -0.068, 1e-6)
			&& FMath::IsNearlyEqual(StageBox(*Head).Min.Y, -0.415, 1e-6));
		const FBox WrapperBox = StageBox(*Wrapper);
		TestTrue(TEXT("The wrapper wound on the stave at the right, where the lead ends"), WrapperBox.Min.X < 7.835 && WrapperBox.Max.X > 7.835
			&& WrapperBox.Max.X - WrapperBox.Min.X > 2.0 * 0.006 && Stave->Material == TEXT("accent.chinese-fir"));
	}
	// The Qingming's four photographs, the mount cut from the fourth, laid at its length and cut at its head and foot.
	const FFormMadePiece* First = FindPiece(Pieces, TEXT("silk.chinese-qingming.section.4"));
	const FFormMadePiece* Last = FindPiece(Pieces, TEXT("silk.chinese-qingming.section.1"));
	if (TestTrue(TEXT("The Qingming is its four photographs"), First && Last && FindPiece(Pieces, TEXT("silk.chinese-qingming.section.2"))
		&& FindPiece(Pieces, TEXT("silk.chinese-qingming.section.3"))))
	{
		TestTrue(TEXT("-4 opens it at the right, -1 ends it at the left, at its true length"), FMath::IsNearlyEqual(StageBox(*First).Max.X, 14.287, 1e-6)
			&& FMath::IsNearlyEqual(StageBox(*Last).Min.X, 9.0, 1e-6));
	}
	const TArray<FArtPaintLayer> Layers = PaintLayers(Scrolls());
	const FArtPaintLayer* Fourth = Layers.FindByPredicate([](const FArtPaintLayer& L) { return L.Work == TEXT("chinese-qingming.4"); });
	TestEqual(TEXT("A layer for each photograph"), Layers.Num(), 6);
	TestTrue(TEXT("The fourth photograph's layer is its own image, at its own proportion, over the ink on silk"), Fourth
		&& Fourth->ImagePath.EndsWith(TEXT("chinese-qingming-4.jpg")) && Fourth->Parent == TEXT("accent.ink-on-silk")
		&& FMath::IsNearlyEqual(Fourth->SizeMeters.X / Fourth->SizeMeters.Y, 7313.0 / 1400.0, 1e-3));
	return true;
}

#endif
