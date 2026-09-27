#include "Plan/EntryContext.h"

#include "Algo/Find.h"
#include "ArtWork.h"
#include "Cast/ScanMesh.h"
#include "HAL/FileManager.h"
#include "MatterRecords.h"
#include "Misc/Paths.h"
#include "MatterWorking.h"
#include "Support/ArtSupport.h"

namespace Musee::ArtMaking
{
	namespace
	{

		/** A millimetre is a thousandth of a metre (the glass records give a pane's thickness in mm). */
		constexpr double MetersPerMillimeter = 0.001;

		/** A window is [left, right, top, bottom]; a centre is [x, y, z]. */
		constexpr int32 WindowNumbers = 4;
		constexpr int32 PointNumbers = 3;

		/** Why a surface the entry names (What: "frame", "glass", "mount", "glazing") has no finish in the stage's schedule. */
		TArray<FString> SurfaceFaults(const FEntryContext& Context, FName Surface, const TCHAR* What)
		{
			if (Surface.IsNone())
			{
				return { FString::Printf(TEXT("it names no %s (a surface of the finish schedule)"), What) };
			}
			if (!Context.Finishes.IsSet())
			{
				return { FString::Printf(TEXT("its %s's surface %s has no finish: %s"), What, *Surface.ToString(), *Context.FinishesWhy) };
			}
			if (!Context.Finishes->Find(Surface))
			{
				return { FString::Printf(TEXT("its %s '%s' is none of the surfaces in the stage's Finishes.json"), What, *Surface.ToString()) };
			}
			return {};
		}

		/** A pane the object is made of or lies under: a surface of the schedule whose working gives its thickness. */
		TArray<FString> PaneFaults(FName Surface, const FEntryContext& Context, const TCHAR* What)
		{
			TArray<FString> Out = SurfaceFaults(Context, Surface, What);
			if (Out.IsEmpty() && PaneMeters(Surface, Context) <= 0.0)
			{
				Out.Add(FString::Printf(TEXT("its %s '%s' is finished in a working that gives no pane thickness"), What, *Surface.ToString()));
			}
			return Out;
		}

		/** The glass a glazed object lies under. */
		TArray<FString> GlazingFaults(const FArtHangEntry& Entry, const FEntryContext& Context)
		{
			return Entry.Glazed ? PaneFaults(Entry.Glazing, Context, TEXT("glazing")) : TArray<FString>();
		}

		bool HasAnchor(const FMuseeStageContext& Stage, const FArtHangEntry& Entry, FName Kind)
		{
			return Stage.FindAnchor(Entry.Anchor, Kind) != nullptr;
		}

		/** A canvas hangs on a Wall anchor, framed in a surface of the finish schedule or not at all. */
		TArray<FString> CanvasEntryFaults(const FMuseeStageContext& Stage, const FArtHangEntry& Entry, const FEntryContext& Context)
		{
			TArray<FString> Out;
			if (!HasAnchor(Stage, Entry, WallAnchorKind))
			{
				Out.Add(FString::Printf(TEXT("%s is not a Wall anchor of the stage"), *Entry.Anchor.ToString()));
			}
			if (IsFramed(Entry))
			{
				Out.Append(SurfaceFaults(Context, Entry.Frame, TEXT("frame")));
			}
			return Out;
		}

		/** A board an object lies on or is matted in: a surface of the schedule, at least the object's size, of some depth. */
		TArray<FString> MatFaults(const FArtHangEntry& Entry, const FEntryContext& Context)
		{
			TArray<FString> Out = SurfaceFaults(Context, Entry.Mat, TEXT("mount"));
			const FVector2D Size = Entry.SizeMeters();
			if (Entry.MatSize.Num() != 2 || Entry.MatSize[0] < Size.X || Entry.MatSize[1] < Size.Y || Entry.MatDepth <= 0.0f)
			{
				Out.Add(TEXT("its mount needs a size [w, h] that holds the work, and a depth of more than zero"));
			}
			return Out;
		}

		/** A transparency: its plate's glass, its window mat, the pane behind it if it names one, its frame and glazing. */
		TArray<FString> TransparencyEntryFaults(const FArtHangEntry& Entry, const FEntryContext& Context)
		{
			TArray<FString> Out = PaneFaults(Entry.Wall, Context, TEXT("plate's glass (its `wall`)"));
			Out.Append(MatFaults(Entry, Context));
			if (!Entry.Backing.IsNone())
			{
				Out.Append(PaneFaults(Entry.Backing, Context, TEXT("backing")));
			}
			if (IsFramed(Entry))
			{
				Out.Append(SurfaceFaults(Context, Entry.Frame, TEXT("frame")));
			}
			Out.Append(GlazingFaults(Entry, Context));
			return Out;
		}

		/**
		 * A stained-glass panel is set in a Glazing anchor, unframed (its came is the opening's), in its glass's surface.
		 * Glass on a Wall anchor in its window mat is a transparency (an autochrome, lit through its diffusing backing),
		 * which stands in its frame against a wall, never in an opening.
		 */
		TArray<FString> GlassEntryFaults(const FMuseeStageContext& Stage, const FArtHangEntry& Entry, const FEntryContext& Context)
		{
			if (IsTransparency(Stage, Entry))
			{
				return TransparencyEntryFaults(Entry, Context);
			}
			TArray<FString> Out;
			if (!HasAnchor(Stage, Entry, GlazingAnchorKind))
			{
				Out.Add(FString::Printf(TEXT("%s is not a Glazing anchor of the stage"), *Entry.Anchor.ToString()));
			}
			if (IsFramed(Entry))
			{
				Out.Add(TEXT("stained glass is leaded in its opening, never framed"));
			}
			if (Entry.Wall.IsNone())
			{
				Out.Add(TEXT("it names no glass to be fired in (its `wall`, a surface of the finish schedule)"));
			}
			else
			{
				Out.Append(SurfaceFaults(Context, Entry.Wall, TEXT("glass")));
			}
			return Out;
		}

		/** A print on a wall lies on its mount, a board at least its size and of some depth, framed and glazed or not. */
		TArray<FString> PrintEntryFaults(const FArtHangEntry& Entry, const FEntryContext& Context)
		{
			TArray<FString> Out = MatFaults(Entry, Context);
			if (IsFramed(Entry))
			{
				Out.Append(SurfaceFaults(Context, Entry.Frame, TEXT("frame")));
			}
			Out.Append(GlazingFaults(Entry, Context));
			return Out;
		}

		/** A card or a plate lies on its reader, unframed, under its glazing or not. */
		TArray<FString> CardEntryFaults(const FMuseeStageContext& Stage, const FArtHangEntry& Entry, const FEntryContext& Context)
		{
			TArray<FString> Out;
			if (!HasAnchor(Stage, Entry, MountAnchorKind))
			{
				Out.Add(FString::Printf(TEXT("%s is not a Mount anchor of the stage (a reader it lies on)"), *Entry.Anchor.ToString()));
			}
			if (IsFramed(Entry))
			{
				Out.Add(TEXT("a card or a plate on its reader is unframed"));
			}
			Out.Append(GlazingFaults(Entry, Context));
			return Out;
		}

		/**
		 * A hanging scroll hangs by its cord in front of a Wall anchor, in its mount: the mount's silk (its `frame`, a
		 * surface of the schedule), its heaven, earth, bands and borders, its knobs' and cord's surfaces, the hook its cord
		 * hangs from above its stick, and how far its back hangs off the wall.
		 */
		TArray<FString> HangingScrollFaults(const FMuseeStageContext& Stage, const FArtHangEntry& Entry, const FEntryContext& Context)
		{
			TArray<FString> Out;
			if (!HasAnchor(Stage, Entry, WallAnchorKind))
			{
				Out.Add(FString::Printf(TEXT("%s is not a Wall anchor of the stage"), *Entry.Anchor.ToString()));
			}
			Out.Append(SurfaceFaults(Context, IsFramed(Entry) ? Entry.Frame : FName(), TEXT("mount")));
			const FArtScrollMount& Mount = Entry.Mount;
			if (Mount.Heaven < 0.0f || Mount.Earth < 0.0f || Mount.Band <= 0.0f || Mount.Border <= 0.0f || Mount.Hook <= 0.0f || Entry.OffWall <= 0.0f)
			{
				Out.Add(TEXT("its mount needs a heaven and an earth of zero or more, bands and borders round the painting, a hook above its stick and an "
					"offWall, each of more than zero"));
			}
			if (Mount.Roller)
			{
				Out.Append(SurfaceFaults(Context, Mount.Knob, TEXT("knobs")));
			}
			Out.Append(SurfaceFaults(Context, Mount.Cord, TEXT("cord")));
			return Out;
		}

		/**
		 * A book or a leaf lies at its centre on a Mount anchor (a case's bed, a lectern's desk), unframed and unglazed (a
		 * case's glass is the case's), a book bound as Data/Art/Books.json gives it.
		 */
		TArray<FString> LyingEntryFaults(const FMuseeStageContext& Stage, const FArtHangEntry& Entry, const FEntryContext& Context)
		{
			TArray<FString> Out;
			if (!HasAnchor(Stage, Entry, MountAnchorKind))
			{
				Out.Add(FString::Printf(TEXT("%s is not a Mount anchor of the stage (a bed or a desk it lies on)"), *Entry.Anchor.ToString()));
			}
			if (Entry.Centre.Num() != PointNumbers)
			{
				Out.Add(TEXT("it needs its centre [x, y, z], where it lies"));
			}
			if (IsFramed(Entry) || Entry.Glazed)
			{
				Out.Add(TEXT("a book or a leaf lies in its case, unframed and unglazed"));
			}
			if (Entry.Kind == Kinds::Book && !FindBook(Entry.Work, Context.Books))
			{
				Out.Add(Context.BooksWhy.IsEmpty() ? FString::Printf(TEXT("%s gives no binding for it"), FArtBook::Record) : Context.BooksWhy);
			}
			return Out;
		}

		/**
		 * Paper is made as a print on its mount on a Wall anchor, or a card on its reader at a Mount anchor; painted and
		 * mounted as a hanging scroll; bound as a book, or a loose leaf, lying on a Mount anchor.
		 */
		TArray<FString> PaperEntryFaults(const FMuseeStageContext& Stage, const FArtHangEntry& Entry, const FEntryContext& Context)
		{
			if (Entry.Kind == Kinds::HangingScroll)
			{
				return HangingScrollFaults(Stage, Entry, Context);
			}
			if (Entry.Kind == Kinds::Book || Entry.Kind == Kinds::Leaf)
			{
				return LyingEntryFaults(Stage, Entry, Context);
			}
			if (!Entry.Kind.IsNone())
			{
				return { FString::Printf(TEXT("paper made as a %s isn't made yet; a print on its mount, a card on its reader, a hanging scroll, a book and a "
					"leaf are"), *Entry.Kind.ToString()) };
			}
			if (HasAnchor(Stage, Entry, WallAnchorKind))
			{
				return PrintEntryFaults(Entry, Context);
			}
			return CardEntryFaults(Stage, Entry, Context);
		}

		/** Silk is made as a painting mounted as a hanging scroll. */
		TArray<FString> SilkEntryFaults(const FMuseeStageContext& Stage, const FArtHangEntry& Entry, const FEntryContext& Context)
		{
			if (Entry.Kind != Kinds::HangingScroll)
			{
				return { FString::Printf(TEXT("silk made as a %s isn't made yet; a hanging scroll is"), *Entry.Kind.ToString()) };
			}
			return HangingScrollFaults(Stage, Entry, Context);
		}

		/** A panel hangs on a Wall anchor as a canvas does, framed or not, made of the wood its `wall` surface names. */
		TArray<FString> PanelEntryFaults(const FMuseeStageContext& Stage, const FArtHangEntry& Entry, const FEntryContext& Context)
		{
			TArray<FString> Out = CanvasEntryFaults(Stage, Entry, Context);
			if (Entry.Wall.IsNone())
			{
				Out.Add(TEXT("it names no wood to be made of (its `wall`, a surface of the finish schedule)"));
			}
			else
			{
				Out.Append(SurfaceFaults(Context, Entry.Wall, TEXT("wood")));
			}
			return Out;
		}

		/** A tapestry hangs unframed on a Wall anchor, its back the plan's offWall off the wall (on its batten and lining). */
		TArray<FString> TapestryEntryFaults(const FMuseeStageContext& Stage, const FArtHangEntry& Entry)
		{
			TArray<FString> Out;
			if (!HasAnchor(Stage, Entry, WallAnchorKind))
			{
				Out.Add(FString::Printf(TEXT("%s is not a Wall anchor of the stage"), *Entry.Anchor.ToString()));
			}
			if (IsFramed(Entry))
			{
				Out.Add(TEXT("a tapestry hangs on its batten, unframed"));
			}
			if (Entry.OffWall <= 0.0f)
			{
				Out.Add(TEXT("it needs its offWall, how far its back hangs off the wall, of more than zero"));
			}
			return Out;
		}

		const FArtTurned* FindTurned(const FArtHangEntry& Entry, const FEntryContext& Context)
		{
			return Context.Turned.FindByPredicate([&Entry](const FArtTurned& Turned) { return Turned.Work == Entry.Work; });
		}

		/** Ware stands at its centre on a Mount anchor, unframed, a dish or a vessel whose form Data/Art/Turned.json gives. */
		TArray<FString> WareEntryFaults(const FMuseeStageContext& Stage, const FArtHangEntry& Entry, const FEntryContext& Context)
		{
			TArray<FString> Out;
			if (!HasAnchor(Stage, Entry, MountAnchorKind))
			{
				Out.Add(FString::Printf(TEXT("%s is not a Mount anchor of the stage (a bed it stands on)"), *Entry.Anchor.ToString()));
			}
			if (Entry.Kind != DishKind && Entry.Kind != VesselKind)
			{
				Out.Add(FString::Printf(TEXT("its kind '%s' is neither a dish nor a vase"), *Entry.Kind.ToString()));
			}
			if (Entry.Centre.Num() != PointNumbers)
			{
				Out.Add(TEXT("it needs its centre [x, y, z], where its foot stands"));
			}
			if (IsFramed(Entry))
			{
				Out.Add(TEXT("ware stands in the round, unframed"));
			}
			const FArtTurned* Turned = FindTurned(Entry, Context);
			if (!Turned)
			{
				Out.Add(Context.TurnedWhy.IsEmpty() ? FString::Printf(TEXT("%s gives no form for it"), FArtTurned::Record) : Context.TurnedWhy);
			}
			else if (Turned->Window.Num() != WindowNumbers || Turned->Window[1] <= Turned->Window[0] || Turned->Window[3] <= Turned->Window[2]
				|| (Entry.Kind == VesselKind && Turned->Profile.Num() < 2))
			{
				Out.Add(FString::Printf(TEXT("%s: its window must be [left, right, top, bottom], and a vessel's profile two points or more"), FArtTurned::Record));
			}
			return Out;
		}

		/** A work in the round stands at its centre on a Mount anchor (its carrier's top, a case's deck), unframed. */
		TArray<FString> RoundEntryFaults(const FMuseeStageContext& Stage, const FArtHangEntry& Entry)
		{
			TArray<FString> Out;
			if (!HasAnchor(Stage, Entry, MountAnchorKind))
			{
				Out.Add(FString::Printf(TEXT("%s is not a Mount anchor of the stage (the carrier it stands on)"), *Entry.Anchor.ToString()));
			}
			if (Entry.Centre.Num() != PointNumbers)
			{
				Out.Add(TEXT("it needs its centre [x, y, z], where its base stands"));
			}
			if (IsFramed(Entry))
			{
				Out.Add(TEXT("a work in the round is unframed"));
			}
			return Out;
		}

		/**
		 * A vessel turned from its section: the vessels' record gives it a section of three points or more, [radius, height]
		 * with no radius below the axis, starting and ending on the axis (so turned it is a closed solid).
		 */
		TArray<FString> VesselEntryFaults(const FMuseeStageContext& Stage, const FArtHangEntry& Entry, const FEntryContext& Context)
		{
			TArray<FString> Out = RoundEntryFaults(Stage, Entry);
			if (!Context.Vessels.IsSet())
			{
				Out.Add(Context.RoundWhy);
				return Out;
			}
			const FArtVessel* Vessel = Context.Vessels->Find(Entry.Work);
			if (!Vessel)
			{
				Out.Add(FString::Printf(TEXT("Data/%s gives no section to turn it to, and the plan gives no scan"), FArtVessels::Record));
			}
			else if (Vessel->Section.Num() < 3 || Vessel->Section[0].X != 0.0 || Vessel->Section.Last().X != 0.0
				|| Vessel->Section.ContainsByPredicate([](const FVector2D& Point) { return Point.X < 0.0; }))
			{
				Out.Add(FString::Printf(TEXT("Data/%s: its section must run from the axis round to the axis, never inside it"), FArtVessels::Record));
			}
			return Out;
		}

		/**
		 * A work cast from its scan stands in the round: its scan a glTF binary or an `.mvm` the collection holds, and any
		 * part the casts' record names a band of heights within it.
		 */
		TArray<FString> CastEntryFaults(const FMuseeStageContext& Stage, const FArtHangEntry& Entry, const FEntryContext& Context)
		{
			TArray<FString> Out = RoundEntryFaults(Stage, Entry);
			const FString Extension = FPaths::GetExtension(Entry.Scan.Mesh);
			if (!Extension.Equals(GltfExtension, ESearchCase::IgnoreCase) && !Extension.Equals(MvmExtension, ESearchCase::IgnoreCase))
			{
				Out.Add(FString::Printf(TEXT("its scan.mesh '%s' is neither a .%s nor an .%s"), *Entry.Scan.Mesh, GltfExtension, MvmExtension));
			}
			else if (!IFileManager::Get().FileExists(*Musee::Art::SourcePath(Entry.Scan.Mesh)))
			{
				Out.Add(FString::Printf(TEXT("its scan %s is not in the collection"), *Entry.Scan.Mesh));
			}
			if (!Context.Casts.IsSet())
			{
				Out.Add(Context.RoundWhy);
				return Out;
			}
			for (const FArtCastPart& Part : Context.Casts->Parts)
			{
				if (Part.Work == Entry.Work && (Part.Band.Num() != 2 || Part.Band[0] < 0.0 || Part.Band[1] <= Part.Band[0] || Part.Band[1] > Entry.SizeMeters().Y))
				{
					Out.Add(FString::Printf(TEXT("its part %s needs a band [from, to] of heights within its own"), *Part.Part.ToString()));
				}
			}
			return Out;
		}

		/** A work cast from a scan may leave its width 0, to be measured from the scan; any other object gives both. */
		bool SizeFaulty(const FArtHangEntry& Entry)
		{
			const FVector2D Size = Entry.SizeMeters();
			const bool bWidthMeasured = IsCast(Entry) && Size.X == 0.0;
			return (Size.X <= 0.0 && !bWidthMeasured) || Size.Y <= 0.0 || Entry.Along < 0.0f || Entry.Height < 0.0f;
		}
	}

	bool IsFramed(const FArtHangEntry& Entry)
	{
		return !Entry.Frame.IsNone() && Entry.Frame != NoFrame;
	}

	TArray<FString> EntryFaults(const FMuseeStageContext& Stage, const FArtHangEntry& Entry, const FEntryContext& Context)
	{
		TArray<FString> Out;
		if (SizeFaulty(Entry))
		{
			Out.Add(IsCast(Entry) ? TEXT("it needs a size [w, h] with its true height, its width 0 or more, and an along and a height of zero or more")
				: TEXT("it needs a size [w, h] of more than zero, and an along and a height of zero or more"));
		}
		if (IsCast(Entry))
		{
			Out.Append(CastEntryFaults(Stage, Entry, Context));
		}
		else if (IsTurned(Entry))
		{
			Out.Append(VesselEntryFaults(Stage, Entry, Context));
		}
		else if (Entry.Support == Supports::Canvas)
		{
			Out.Append(CanvasEntryFaults(Stage, Entry, Context));
		}
		else if (Entry.Support == Supports::Glass)
		{
			Out.Append(GlassEntryFaults(Stage, Entry, Context));
		}
		else if (Entry.Support == Supports::Paper)
		{
			Out.Append(PaperEntryFaults(Stage, Entry, Context));
		}
		else if (Entry.Support == Supports::Plate)
		{
			Out.Append(CardEntryFaults(Stage, Entry, Context));
		}
		else if (Entry.Support == Supports::Earthenware)
		{
			Out.Append(WareEntryFaults(Stage, Entry, Context));
		}
		else if (Entry.Support == Supports::Silk)
		{
			Out.Append(SilkEntryFaults(Stage, Entry, Context));
		}
		else if (Entry.Support == Supports::Panel)
		{
			Out.Append(PanelEntryFaults(Stage, Entry, Context));
		}
		else if (Entry.Support == Supports::Tapestry)
		{
			Out.Append(TapestryEntryFaults(Stage, Entry));
		}
		else
		{
			Out.Add(FString::Printf(TEXT("its support '%s' isn't made yet; canvas, panel, stained glass, paper, silk, a daguerreotype's "
				"plate, earthenware, tapestry, a cast and ceramic are"), *Entry.Support.ToString()));
		}
		// A work in the round is finished in its medium (Data/Art/Media.json), not in its support's layer.
		if (Entry.Support != Supports::Glass && !IsInMedium(Entry) && Out.IsEmpty() && !FindSupport(Entry.Support, Entry.Kind))
		{
			Out.Add(FString::Printf(TEXT("%s lists no support '%s'"), FArtSupports::Record, *Entry.Support.ToString()));
		}
		if (Musee::Art::PaintMaterial(Entry.Work).IsNone())
		{
			Out.Add(TEXT("its id can't name a paint layer (it must be one key segment)"));
		}
		if (!Entry.Body.IsNone() && !Stage.FindAnchor(Entry.Body, BodyAnchorKind))
		{
			Out.Add(FString::Printf(TEXT("its body %s is not a Body anchor of the stage"), *Entry.Body.ToString()));
		}
		return Out;
	}

	double PaneMeters(FName Surface, const FEntryContext& Context)
	{
		const FMatterFinish* Pane = !Surface.IsNone() && Context.Finishes.IsSet() ? Context.Finishes->Find(Surface) : nullptr;
		if (!Pane)
		{
			return 0.0;
		}
		const TValueOrError<FMatterWorking, FString> Working = Musee::Matter::ReadWorking(Pane->Working);
		const bool bGiven = Working.HasValue() && Working.GetValue().PaneThickness.HasValue() && Working.GetValue().PaneThickness.Unit == TEXT("mm");
		return bGiven ? Working.GetValue().PaneThickness.Value * MetersPerMillimeter : 0.0;
	}

	double GlazingMeters(const FArtHangEntry& Entry, const FEntryContext& Context)
	{
		return Entry.Glazed ? PaneMeters(Entry.Glazing, Context) : 0.0;
	}

	bool IsTransparency(const FMuseeStageContext& Stage, const FArtHangEntry& Entry)
	{
		return Entry.Support == Supports::Glass && !Entry.Mat.IsNone() && Stage.FindAnchor(Entry.Anchor, WallAnchorKind) != nullptr;
	}

	const FArtBook* FindBook(const FString& Work, TConstArrayView<FArtBook> Books)
	{
		return Algo::FindByPredicate(Books, [&Work](const FArtBook& Book) { return Book.Work == Work; });
	}

	FWareSpec WareOf(const FArtHangEntry& Entry, const FEntryContext& Context)
	{
		FWareSpec Ware;
		Ware.Kind = Entry.Kind;
		Ware.WidthMeters = Entry.SizeMeters().X;
		Ware.HeightMeters = Entry.SizeMeters().Y;
		if (const FArtTurned* Turned = FindTurned(Entry, Context); Turned && Turned->Window.Num() == WindowNumbers)
		{
			Ware.Window = FVector4(Turned->Window[0], Turned->Window[1], Turned->Window[2], Turned->Window[3]);
			Ware.Profile = Turned->Profile;
		}
		return Ware;
	}
}
