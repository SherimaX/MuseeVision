#include "Algo/Find.h"
#include "ArtHang.h"
#include "ArtHangEntry.h"
#include "ArtPaintLayer.h"
#include "ArtPieces.h"
#include "ArtWork.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "MuseeRecords.h"
#include "MuseeUnits.h"
#include "Plan/EntryContext.h"
#include "Scroll/ScrollLayout.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace Musee::ArtMaking::HandscrollTests
{
	/** Art's making tests run in the editor, where its making twin lives, and belong to the product's own filter. */
	inline constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	/** A round part is drawn as a polygon within a chord's sag of its circle. */
	constexpr double Sag = 1e-3;

	/** Lengths in metres, and proportions, compared to within a micrometre's rounding. */
	constexpr double Near = 1e-6;

	/** The fixtures' folder, as the repository's root sees it (a hanging plan's `source` is relative to the root). */
	const TCHAR* const Fixtures = TEXT("MuseeVision/Source/Systems/Art/MuseeArtMaking/Private/Tests/Data");

	FString Fixture(const FString& Name)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir() / TEXT("Systems/Art/MuseeArtMaking/Private/Tests/Data") / Name);
	}

	/**
	 * The fixtures' stand-ins for the collection's photographs, plain images at a tenth of their size: the Autobiography's
	 * -1 and -2, the Qingming's -1 to -4.
	 */
	const FIntPoint AutobiographyPixels(641, 48);
	const FIntPoint QingmingPixels(731, 140);

	/**
	 * The test scroll case: a level bed 1 m up (a Wall anchor, as Chenghuai's beds are), x along it, its face up, and up
	 * its slope (its z) towards -y, away from the reader who stands at +y. Its works are shown from the stand-ins beside the
	 * fixtures' catalogue, so what the collection's photographs hold doesn't move the test.
	 */
	FMuseeStageContext Scrolls()
	{
		FMuseeStageContext Context;
		Context.Stage = FMuseeKey(TEXT("scrolls"));
		Context.ScheduleDir = Fixture(TEXT("Stages/scrolls"));
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

	/** The window a record gives a work's photograph, or the whole photograph. */
	FArtHandscrollWindow WindowOf(const FArtHandscroll* Record, int32 Number)
	{
		const FArtHandscrollWindow* Found = Record ? Algo::FindBy(Record->Windows, Number, &FArtHandscrollWindow::Image) : nullptr;
		FArtHandscrollWindow Whole;
		Whole.Image = Number;
		return Found ? *Found : Whole;
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
		const FScrollPanel& Left = Laid.Panels[0];
		const FScrollPanel& Right = Laid.Panels[1];
		TestTrue(TEXT("at the sheet's height, cut evenly at the two ends"), FMath::IsNearlyEqual(Left.ImageTopMeters, 0.0)
			&& FMath::IsNearlyEqual(Right.ImageTopMeters, 0.0) && FMath::IsNearlyEqual(Laid.CutMeters, 0.2));
		TestTrue(TEXT("the first from the sheet's left edge, 0.2 into its photograph"), FMath::IsNearlyEqual(Left.AlongMeters, 0.0)
			&& FMath::IsNearlyEqual(Left.LengthMeters, 9.8) && FMath::IsNearlyEqual(Left.ImageLeftMeters, 0.2));
		TestTrue(TEXT("the second joined to it without a gap, to the sheet's right edge"), FMath::IsNearlyEqual(Right.AlongMeters, 9.8)
			&& FMath::IsNearlyEqual(Right.AlongMeters + Right.LengthMeters, 19.6) && FMath::IsNearlyEqual(Right.ImageLeftMeters, 0.0));
		TestTrue(TEXT("each at its own proportion, never stretched"), Left.ImageMeters.Equals(FVector2D(10.0, 1.0)) && Right.ImageMeters.Equals(FVector2D(10.0, 1.0)));
	}
	// The same photographs, the second's last fifth its mount, on a sheet of 18 : 1: 0 % stray, nothing cut.
	const FScrollImage Windowed[] = { { FIntPoint(100, 10) }, { FIntPoint(100, 10), 0.0, 0.8 } };
	const FScrollLayout Framed = LayScroll(Windowed, FVector2D(18.0, 1.0));
	TestTrue(TEXT("A window keeps the mount at an end off the sheet"), Framed.Panels.Num() == 2 && FMath::IsNearlyZero(Framed.Stray, 1e-9)
		&& FMath::IsNearlyEqual(Framed.Panels[1].LengthMeters, 8.0) && FMath::IsNearlyZero(Framed.CutMeters, 1e-9));
	// A photograph of 100 x 12 whose top and bottom twelfths are the mount's silk: its window is 100 x 10, the sheet's own
	// 10 : 1, so nothing strays; the photograph stands 1.2 high, and the sheet's top edge 0.1 below the photograph's.
	const FScrollImage Bordered[] = { { FIntPoint(100, 12), 0.0, 1.0, 1.0 / 12.0, 11.0 / 12.0 } };
	const FScrollLayout Headed = LayScroll(Bordered, FVector2D(10.0, 1.0));
	TestTrue(TEXT("A window keeps the mount at the head and foot off the sheet, and out of the proportion"), Headed.Panels.Num() == 1
		&& FMath::IsNearlyZero(Headed.Stray, 1e-9) && FMath::IsNearlyZero(Headed.CutMeters, 1e-9)
		&& Headed.Panels[0].ImageMeters.Equals(FVector2D(10.0, 1.2), 1e-9) && FMath::IsNearlyEqual(Headed.Panels[0].ImageTopMeters, 0.1)
		&& FMath::IsNearlyEqual(Headed.Panels[0].LengthMeters, 10.0));
	// Two photographs whose windows differ in height (one whole, 50 x 10; one 50 x 20 whose bottom half is the mount) join
	// at the one height of their windows, each at its own proportion.
	const FScrollImage Unequal[] = { { FIntPoint(50, 10) }, { FIntPoint(50, 20), 0.0, 1.0, 0.0, 0.5 } };
	const FScrollLayout Joined = LayScroll(Unequal, FVector2D(10.0, 1.0));
	TestTrue(TEXT("Windows of different heights join at one height"), Joined.Panels.Num() == 2 && FMath::IsNearlyZero(Joined.Stray, 1e-9)
		&& Joined.Panels[0].ImageMeters.Equals(FVector2D(5.0, 1.0), 1e-9) && Joined.Panels[1].ImageMeters.Equals(FVector2D(5.0, 2.0), 1e-9)
		&& FMath::IsNearlyEqual(Joined.Panels[1].AlongMeters, 5.0) && FMath::IsNearlyZero(Joined.Panels[1].ImageTopMeters, 1e-9));
	// One photograph of 10 : 1 on a sheet of 10.5 : 1: short, so laid at the sheet's length and cut at its head and foot.
	const FScrollImage Short[] = { { FIntPoint(100, 10) } };
	const FScrollLayout Covered = LayScroll(Short, FVector2D(10.5, 1.0));
	TestTrue(TEXT("A short photograph covers the sheet at its length, cut evenly at its head and foot"), Covered.Panels.Num() == 1
		&& Covered.Stray < 0.0 && FMath::IsNearlyEqual(Covered.Panels[0].ImageTopMeters, 0.025) && FMath::IsNearlyEqual(Covered.Panels[0].ImageMeters.X, 10.5)
		&& FMath::IsNearlyEqual(Covered.Panels[0].LengthMeters, 10.5));
	TestTrue(TEXT("Nothing to lay lays nothing"), LayScroll(TConstArrayView<FScrollImage>(), FVector2D(1.0, 1.0)).Panels.IsEmpty());
	const FScrollImage Empty[] = { { FIntPoint(100, 10), 0.0, 1.0, 0.5, 0.5 } };
	TestTrue(TEXT("An empty window lays nothing"), LayScroll(Empty, FVector2D(1.0, 1.0)).Panels.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtSectionImagesTest, "Musee.Art.SectionImages", Musee::ArtMaking::HandscrollTests::Flags)

bool FArtSectionImagesTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking::HandscrollTests;
	// The fixtures hold test-sections-1 to -3 twice: three distinct cuts in paintings/, and in the preferred paintings_hires/
	// three copies of one file under the cuts' names, which are no sections.
	FArtHangEntry Entry;
	Entry.Work = TEXT("test-sections");
	Entry.Source = FString::Printf(TEXT("%s/collection.json"), Fixtures);
	const TArray<FString> Sections = Musee::Art::FindSectionImages(Entry);
	if (TestEqual(TEXT("The three cuts are found"), Sections.Num(), 3))
	{
		for (int32 Index = 0; Index < Sections.Num(); ++Index)
		{
			TestTrue(*FString::Printf(TEXT("-%d is its own cut, numbered in order, from the folder of distinct files"), Index + 1),
				FPaths::GetCleanFilename(Sections[Index]) == FString::Printf(TEXT("test-sections-%d.png"), Index + 1)
				&& FPaths::GetCleanFilename(FPaths::GetPath(Sections[Index])) == TEXT("paintings"));
		}
	}
	Entry.Work = TEXT("monet-impression-sunrise");
	TestTrue(TEXT("A work in one photograph has no sections"), Musee::Art::FindSectionImages(Entry).IsEmpty());
	// Whatever the collection holds of a work in several photographs, what is found is distinct files numbered from 1.
	Entry.Work = TEXT("chinese-thousand-li");
	Entry.Source = TEXT("assets/collection.json");
	const TArray<FString> Collected = Musee::Art::FindSectionImages(Entry);
	TSet<FString> Hashes;
	bool bNumbered = true;
	for (int32 Index = 0; Index < Collected.Num(); ++Index)
	{
		Hashes.Add(LexToString(FMD5Hash::HashFile(*Collected[Index])));
		bNumbered &= FPaths::GetBaseFilename(Collected[Index]) == FString::Printf(TEXT("chinese-thousand-li-%d"), Index + 1);
	}
	TestTrue(TEXT("The collection's Thousand Li: two or more distinct files, numbered from 1"), Collected.Num() >= 2 && Hashes.Num() == Collected.Num()
		&& bNumbered);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArtHandscrollTest, "Musee.Art.Handscroll", Musee::ArtMaking::HandscrollTests::Flags)

bool FArtHandscrollTest::RunTest(const FString& Parameters)
{
	using namespace Musee::ArtMaking;
	using namespace Musee::ArtMaking::HandscrollTests;
	const TValueOrError<FArtHang, FString> Hang = Musee::ReadRecord<FArtHang>(Fixture(TEXT("Stages/scrolls/Hang.json")));
	if (!TestTrue(TEXT("The test plan reads"), Hang.HasValue()))
	{
		return false;
	}
	FEntryContext Context;
	ReadHandscrollsInto(Hang.GetValue(), Context);
	if (!TestTrue(TEXT("The handscrolls' fittings and records read"), Context.Handscroll.IsSet()))
	{
		return false;
	}
	// Each work planned from the stand-ins with Data/Art/Handscrolls.json's order and windows: only its windows lie on its
	// sheet, each photograph at its own proportion; and a note of its stray just when its windows, joined, stray past the
	// tolerance, reckoned here from the windows on the stand-ins' pixels.
	int32 Strays = 0;
	for (const FArtHangEntry& Entry : Hang.GetValue().Works)
	{
		const TValueOrError<FArtWork, FString> Work = Musee::Art::ReadWork(Entry);
		if (!TestTrue(*FString::Printf(TEXT("%s reads"), *Entry.Work), Work.HasValue()))
		{
			continue;
		}
		FCanvasPlan Canvas;
		TArray<FString> Faults;
		TArray<FString> Notes;
		PlanHandscroll(Work.GetValue(), Entry, Context, Canvas, Faults, Notes);
		TestTrue(*FString::Printf(TEXT("%s plans without a fault"), *Entry.Work), Faults.IsEmpty() && !Canvas.Sections.IsEmpty());
		const FArtHandscroll* Record = Algo::FindBy(Context.Handscrolls, Entry.Work, &FArtHandscroll::Work);
		const FIntPoint Pixels = Entry.Work == TEXT("chinese-qingming") ? QingmingPixels : AutobiographyPixels;
		double Joined = 0.0;
		for (const FScrollSection& Section : Canvas.Sections)
		{
			const FArtHandscrollWindow Window = WindowOf(Record, Section.Number);
			const FScrollPanel& Panel = Section.Panel;
			const FVector2D Image = Panel.ImageMeters;
			TestTrue(*FString::Printf(TEXT("%s -%d is the test's stand-in"), *Entry.Work, Section.Number), Section.ImagePath.Contains(TEXT("/Private/Tests/Data/assets/paintings/")));
			TestTrue(*FString::Printf(TEXT("%s -%d: only its window lies on the sheet, none of the mount"), *Entry.Work, Section.Number),
				Panel.ImageLeftMeters >= Window.From * Image.X - Near && Panel.ImageLeftMeters + Panel.LengthMeters <= Window.To * Image.X + Near
				&& Panel.ImageTopMeters >= Window.Top * Image.Y - Near && Panel.ImageTopMeters + Entry.SizeMeters().Y <= Window.Bottom * Image.Y + Near);
			TestTrue(*FString::Printf(TEXT("%s -%d at its own proportion, never stretched"), *Entry.Work, Section.Number),
				FMath::IsNearlyEqual(Image.X / Image.Y, double(Pixels.X) / Pixels.Y, Near));
			Joined += (Window.To - Window.From) * Pixels.X / ((Window.Bottom - Window.Top) * Pixels.Y);
		}
		const double Stray = Joined / (Entry.SizeMeters().X / Entry.SizeMeters().Y) - 1.0;
		const bool bStrays = FMath::Abs(Stray) > Context.Handscroll->ProportionTolerance;
		Strays += bStrays ? 1 : 0;
		TestTrue(*FString::Printf(TEXT("%s: a note just when its windows stray from its proportion"), *Entry.Work), Notes.Num() == (bStrays ? 1 : 0)
			&& (!bStrays || Notes[0].Contains(Stray > 0.0 ? TEXT("longer") : TEXT("shorter"))));
	}
	TArray<FFormMadePiece> Pieces;
	TArray<FMuseeFinding> Findings;
	GatherArt(Scrolls(), Pieces, Findings);
	TestTrue(TEXT("Its findings are the strays' notes, and nothing unmade"), Findings.Num() == Strays
		&& !Findings.ContainsByPredicate([](const FMuseeFinding& F) { return !F.Summary.Contains(TEXT("than its")); }));
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
		StartBox.Min.X > EndBox.Min.X && FMath::IsNearlyEqual(StartBox.Min.X, EndBox.Max.X, Near));
	TestTrue(TEXT("At its true length, 7.55 m from its along"), FMath::IsNearlyEqual(EndBox.Min.X, 0.2, Near) && FMath::IsNearlyEqual(StartBox.Max.X, 7.75, Near));
	TestTrue(TEXT("Its height up the bed, lying on it, 0.3 mm thick"), FMath::IsNearlyEqual(StartBox.Max.Y, -0.1, Near)
		&& FMath::IsNearlyEqual(StartBox.Min.Y, -0.383, Near) && FMath::IsNearlyEqual(StartBox.Max.Z, 1.0003, Near));
	TestTrue(TEXT("Each panel in its photograph's own layer over the ink on its paper"), Start->Material == TEXT("paint.chinese-huaisu-autobiography.2")
		&& End->Material == TEXT("paint.chinese-huaisu-autobiography.1") && Start->Working == TEXT("ink-painted"));
	// The mount: borders 12 mm, a band 25 mm at each end, the lead 60 mm beyond the beginning.
	const FFormMadePiece* Mount = FindPiece(Pieces, TEXT("paper.chinese-huaisu-autobiography.mount"));
	if (TestNotNull(TEXT("The mount"), Mount))
	{
		const FBox Box = StageBox(*Mount);
		TestTrue(TEXT("in the mount's silk, from the left roll to the lead's end, its borders above and below"), Mount->Surface == TEXT("scrolls.mount.silk")
			&& FMath::IsNearlyEqual(Box.Min.X, 0.175, Near) && FMath::IsNearlyEqual(Box.Max.X, 7.835, Near)
			&& FMath::IsNearlyEqual(Box.Max.Y, -0.088, Near) && FMath::IsNearlyEqual(Box.Min.Y, -0.395, Near));
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
			&& Foot->Surface == TEXT("scrolls.mount.knob") && FMath::IsNearlyEqual(StageBox(*Foot).Max.Y, -0.068, Near)
			&& FMath::IsNearlyEqual(StageBox(*Head).Min.Y, -0.415, Near));
		const FBox WrapperBox = StageBox(*Wrapper);
		TestTrue(TEXT("The wrapper wound on the stave at the right, where the lead ends"), WrapperBox.Min.X < 7.835 && WrapperBox.Max.X > 7.835
			&& WrapperBox.Max.X - WrapperBox.Min.X > 2.0 * 0.006 && Stave->Material == TEXT("accent.chinese-fir"));
		// The mount's front at the right, from the museum's record: the separator 10.2 cm and the frontispiece 136.3 cm, of which
		// only the band and lead lie open; the rest is wound on the stave over the wrapper, each turn the silk's thickness.
		const FArtHandscroll* Record = Algo::FindBy(Context.Handscrolls, FString(TEXT("chinese-huaisu-autobiography")), &FArtHandscroll::Work);
		const TValueOrError<FArtScrollFittings, FString> Silk = Musee::ReadRecord<FArtScrollFittings>(FArtScrollFittings::Record);
		if (TestTrue(TEXT("The Autobiography's record gives its separator and frontispiece, as the museum measures them"), Record && Silk.HasValue()
			&& FMath::IsNearlyEqual(Record->SeparatorMeters, 0.102, Near) && FMath::IsNearlyEqual(Record->FrontispieceMeters, 1.363, Near)))
		{
			const double T = Silk.GetValue().MountMeters;
			const double FrontRolled = 0.102 + 1.363 - 0.025 - 0.06;
			const double Rolled = FMath::Sqrt(FMath::Square(0.025) + T * (7.835 - 0.175 + FrontRolled) / UE_DOUBLE_PI);
			const double Wound = Context.Handscroll->WrapperTurns * UE_DOUBLE_TWO_PI * Rolled;
			const double Radius = FMath::Sqrt(FMath::Square(Context.Handscroll->StaveRadiusMeters) + T * (Wound + FrontRolled) / UE_DOUBLE_PI);
			TestTrue(TEXT("The roll at the stave holds the wrapper and, over it, the frontispiece's unshown 1.38 m"),
				FMath::IsNearlyEqual(WrapperBox.Max.Z, 1.0 + T + 2.0 * Radius, Sag) && FMath::IsNearlyEqual(WrapperBox.Max.X - WrapperBox.Min.X, 2.0 * Radius, Sag));
		}
	}
	// The Qingming's four photographs, -4 its beginning at the right, -1 its end at the left.
	const FFormMadePiece* First = FindPiece(Pieces, TEXT("silk.chinese-qingming.section.4"));
	const FFormMadePiece* Last = FindPiece(Pieces, TEXT("silk.chinese-qingming.section.1"));
	if (TestTrue(TEXT("The Qingming is its four photographs"), First && Last && FindPiece(Pieces, TEXT("silk.chinese-qingming.section.2"))
		&& FindPiece(Pieces, TEXT("silk.chinese-qingming.section.3"))))
	{
		TestTrue(TEXT("-4 opens it at the right, -1 ends it at the left, at its true length"), FMath::IsNearlyEqual(StageBox(*First).Max.X, 14.287, Near)
			&& FMath::IsNearlyEqual(StageBox(*Last).Min.X, 9.0, Near));
	}
	const TArray<FArtPaintLayer> Layers = PaintLayers(Scrolls());
	const FArtPaintLayer* Fourth = Layers.FindByPredicate([](const FArtPaintLayer& L) { return L.Work == TEXT("chinese-qingming.4"); });
	TestEqual(TEXT("A layer for each photograph"), Layers.Num(), 6);
	TestTrue(TEXT("The fourth photograph's layer is its own image, at its own proportion, over the ink on silk"), Fourth
		&& Fourth->ImagePath.EndsWith(TEXT("chinese-qingming-4.png")) && Fourth->Parent == TEXT("accent.ink-on-silk")
		&& FMath::IsNearlyEqual(Fourth->SizeMeters.X / Fourth->SizeMeters.Y, double(QingmingPixels.X) / QingmingPixels.Y, Near));
	return true;
}

#endif
