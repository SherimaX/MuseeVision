#include "ArtPieces.h"
#include "Objects/ObjectMaking.h"
#include "Plan/EntryContext.h"
#include "Support/ArtSupport.h"

namespace Musee::ArtMaking
{
	namespace
	{
		/**
		 * The frame a book or a leaf lies in on its Mount anchor: at its centre, x to its reader's right, y out of the bed or
		 * the desk (the anchor's z), z up the page, away from the reader, whom its facing (a bearing from the stage's +x
		 * towards +y) turns its foot to.
		 */
		FMuseeFrame LyingFrame(const FArtHangEntry& Entry, const FMuseeFrame& Bed)
		{
			const double Bearing = FMath::DegreesToRadians(static_cast<double>(Entry.Facing_Deg));
			const FVector Out = Bed.DirectionToParent(FVector::UpVector);
			const FVector Towards(FMath::Cos(Bearing), FMath::Sin(Bearing), 0.0);
			const FVector Head = -(Towards - Out * (Out | Towards)).GetSafeNormal();
			const FVector Right = FVector::CrossProduct(Out, Head);
			const FVector Middle(Entry.Centre[0], Entry.Centre[1], Entry.Centre[2]);
			return FMuseeFrame(Middle, FQuat(FMatrix(Right, Out, Head, FVector::ZeroVector)));
		}

		/** A sheet's own frame in a lying frame: its lower-left corner so far to the reader's right and up the page. */
		FMuseeFrame CornerAt(const FMuseeFrame& Lying, double Right, double Up)
		{
			return Lying.FrameToParent(FMuseeFrame(FVector(Right, 0.0, Up)));
		}

		/** The work's layer, finished as its support's record says. */
		FLayerFinish PageLayer(const FCanvasPlan& Canvas)
		{
			return { Canvas.Layer, Canvas.Finish.Working, Canvas.Finish.History };
		}

		FLayerFinish BindingFinish(const FArtFinishSpec& Spec)
		{
			return { Spec.Palette, Spec.Working, Spec.History };
		}

		/** The surfaces a book's own parts are named by; their materials are its binding's. */
		const FName CoverSurface(TEXT("book.cover"));
		const FName EdgesSurface(TEXT("book.edges"));
	}

	void MakeBook(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces,
		TArray<FMuseeFinding>& OutFindings)
	{
		const FArtHangEntry& Entry = *Canvas.Entry;
		const TValueOrError<TArray<FArtBook>, FString>& Books = Making.Books();
		const FArtBook* Book = Books.HasValue() ? FindBook(Entry.Work, Books.GetValue()) : nullptr;
		if (!Book || Book->Leaves <= 0)
		{
			OutFindings.Add(ArtFinding(Making.Stage, FString::Printf(TEXT("Work %s: it can't be bound: %s."), *Entry.Work,
				Books.HasError() ? *Books.GetError() : TEXT("Data/Art/Books.json gives no leaves for it"))));
			return;
		}
		const bool bSplit = Plan.IsSplit();
		const FMuseeFrame Lying = LyingFrame(Entry, Making.Stage.FindAnchor(Entry.Anchor, MountAnchorKind)->Frame);
		// The opening whole (its verso left of the spine, its recto right), or its one page: the work's first book object is
		// the verso, its second the recto.
		int32 BookObjects = 0;
		const FCanvasPlan* First = nullptr;
		for (const FCanvasPlan& Each : Plan.Canvases)
		{
			if (Each.Entry->Kind == Kinds::Book)
			{
				First = First ? First : &Each;
				++BookObjects;
			}
		}
		const bool bWhole = BookObjects == 1;
		const bool bFirst = First == &Canvas;
		const FVector2D Size = Entry.SizeMeters();
		const double PageWidth = bWhole ? 0.5 * Size.X : Size.X;
		const double Half = 0.5 * Size.Y;
		const double Square = Book->SquareMeters;
		const double Paper = FindSupport(Entry.Support)->ThicknessMeters;
		// The leaves each side holds at this opening: the verso side the leaves already turned (the first leaf, whose verso
		// the first opening shows, and one more for each opening after it), the recto side the rest.
		const double Leaf = Book->BlockMeters / Book->Leaves;
		const int32 Turned = FMath::Clamp(Entry.Opening + 1, 0, Book->Leaves);
		for (const bool bVerso : { true, false })
		{
			if (!bWhole && bVerso != bFirst)
			{
				continue;
			}
			const TCHAR* const Side = bVerso ? TEXT("verso") : TEXT("recto");
			const double Block = FMath::Max(Leaf * (bVerso ? Turned : Book->Leaves - Turned), Paper);
			const double Left = bVerso ? -PageWidth : 0.0;
			// The board under the side, standing its square past the leaves at the head, the foot and the fore-edge.
			const FVector2D Board(PageWidth + Square, Size.Y + 2.0 * Square);
			OutPieces.Add(SheetPiece(PartName(Entry, bSplit, *FString::Printf(TEXT("board.%s"), Side)), CoverSurface, BindingFinish(Book->Cover),
				CornerAt(Lying, bVerso ? Left - Square : 0.0, -Half - Square), Board, Book->BoardMeters, Book->BoardMeters, false));
			// The side's leaves on the board, less the page on top.
			if (Block > Paper)
			{
				OutPieces.Add(SheetPiece(PartName(Entry, bSplit, *FString::Printf(TEXT("block.%s"), Side)), EdgesSurface, BindingFinish(Book->Edges),
					CornerAt(Lying, Left, -Half), FVector2D(PageWidth, Size.Y), Block - Paper, Book->BoardMeters + Block - Paper, false));
			}
			// The page on top, showing its share of the image: the recto of an opening shown whole shows the image's right half.
			const FName Name = bWhole ? PartName(Entry, bSplit, Side) : FName(ObjectName(Entry, bSplit));
			OutPieces.Add(SheetPiece(Name, FName(ObjectWord(Entry.Support)), PageLayer(Canvas), CornerAt(Lying, Left, -Half), FVector2D(PageWidth, Size.Y),
				Paper, Book->BoardMeters + Block, false, bWhole && !bVerso ? PageWidth : 0.0));
		}
	}

	void MakeLeaf(const FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces)
	{
		const FArtHangEntry& Entry = *Canvas.Entry;
		const FMuseeFrame Lying = LyingFrame(Entry, Making.Stage.FindAnchor(Entry.Anchor, MountAnchorKind)->Frame);
		const FVector2D Size = Entry.SizeMeters();
		const double Paper = FindSupport(Entry.Support)->ThicknessMeters;
		// Centred where it lies, its underside on the bed.
		OutPieces.Add(SheetPiece(FName(ObjectName(Entry, Plan.IsSplit())), FName(ObjectWord(Entry.Support)), PageLayer(Canvas),
			CornerAt(Lying, -0.5 * Size.X, -0.5 * Size.Y), Size, Paper, Paper, false));
	}
}
