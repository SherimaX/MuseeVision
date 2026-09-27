#include "ArtHangReading.h"

#include "Algo/StableSort.h"
#include "ArtCare.h"
#include "ArtPaintLayer.h"
#include "Plan/EntryContext.h"
#include "Support/ArtSupport.h"
#include "HAL/FileManager.h"
#include "MatterRecords.h"
#include "Misc/Paths.h"
#include "MuseeRecords.h"

namespace Musee::ArtMaking
{
	namespace
	{
		const FName ArtSystem(TEXT("Art"));

		FString ScheduleFolder(const FMuseeStageContext& Stage)
		{
			return FPaths::IsRelative(Stage.ScheduleDir) ? Records::DataRoot() / Stage.ScheduleDir : Stage.ScheduleDir;
		}

		/**
		 * What is wrong with a work's objects together: each index once, one support; a book is its opening whole or its
		 * verso and recto; and either they share the work's one image, and so are one height, or each names its own.
		 */
		TArray<FString> CanvasFaults(const TArray<const FArtHangEntry*>& Canvases)
		{
			TArray<FString> Out;
			for (int32 Index = 1; Index < Canvases.Num(); ++Index)
			{
				if (Canvases[Index]->Canvas == Canvases[Index - 1]->Canvas)
				{
					Out.Add(FString::Printf(TEXT("its objects must be numbered once each (%d is hung twice)"), Canvases[Index]->Canvas));
					break;
				}
			}
			const FArtHangEntry& First = *Canvases[0];
			if (Canvases.ContainsByPredicate([&First](const FArtHangEntry* Entry) { return Entry->Support != First.Support; }))
			{
				Out.Add(TEXT("its objects are of one support"));
			}
			if (Canvases.FilterByPredicate([](const FArtHangEntry* Entry) { return Entry->Kind == Kinds::Book; }).Num() > BookSides)
			{
				Out.Add(TEXT("a book's objects are its opening whole, or its verso and recto"));
			}
			const int32 OwnImages = Canvases.FilterByPredicate([](const FArtHangEntry* Entry) { return !Entry->Image.IsEmpty(); }).Num();
			if (OwnImages != 0 && OwnImages != Canvases.Num())
			{
				Out.Add(TEXT("its objects either all name their own image or all share the work's one"));
			}
			const bool bOneHeight = !Canvases.ContainsByPredicate([&First](const FArtHangEntry* Entry)
			{
				return !FMath::IsNearlyEqual(Entry->SizeMeters().Y, First.SizeMeters().Y);
			});
			if (OwnImages == 0 && !bOneHeight)
			{
				Out.Add(TEXT("its canvases carry one image, so they are one height"));
			}
			return Out;
		}

		/** The plan's objects that Art makes, grouped by work, in the order the works first appear, each work's by index. */
		TArray<TArray<const FArtHangEntry*>> WorksOf(const FArtHang& Hang)
		{
			TArray<TArray<const FArtHangEntry*>> Out;
			TMap<FString, int32> IndexOf;
			for (const FArtHangEntry& Entry : Hang.Works)
			{
				if (Entry.IsBuilt())
				{
					continue;
				}
				const int32* Found = IndexOf.Find(Entry.Work);
				const int32 Index = Found ? *Found : IndexOf.Add(Entry.Work, Out.AddDefaulted());
				Out[Index].Add(&Entry);
			}
			for (TArray<const FArtHangEntry*>& Canvases : Out)
			{
				Algo::StableSortBy(Canvases, [](const FArtHangEntry* Entry) { return Entry->Canvas; });
			}
			return Out;
		}

		/**
		 * The layer an object shows is finished as its support's record says for the kind of object it is (the oil paint
		 * on a canvas or a panel, the paper of a print, the silver of a daguerreotype, the lustre on ware, the ink on a
		 * scroll's silk or paper, a tapestry's weave), and as its glass on stained glass; its faces without the image, as
		 * its support's ground.
		 */
		void FinishOf(const FArtHangEntry& Entry, const FEntryContext& Context, FCanvasPlan& OutCanvas)
		{
			if (Entry.Support == Supports::Glass)
			{
				const FMatterFinish& Glass = *Context.Finishes->Find(Entry.Wall);
				OutCanvas.Finish = { Glass.Material.ToName(), Glass.Working.ToName(), Glass.History.ToName() };
				return;
			}
			const FArtSupport& Support = *FindSupport(Entry.Support, Entry.Kind);
			OutCanvas.Finish = { Support.Palette, Support.Working, Support.History };
			OutCanvas.Ground = { Support.Palette, Support.GroundWorking, Support.GroundHistory };
		}

		/**
		 * A work in the round (cast from its scan, or turned) shows no image: each object is finished in the work's medium
		 * (Data/Art/Media.json), a fault when the record has no line for the catalogue's medium.
		 */
		void PlanInMedium(const FArtWork& Work, const TArray<const FArtHangEntry*>& Canvases, const FEntryContext& Context, FWorkPlan& OutPlan,
			TArray<FString>& OutFaults)
		{
			const FArtMedium* Medium = Context.Media.IsSet() ? Context.Media->Find(Work.Medium) : nullptr;
			if (!Medium)
			{
				OutFaults.Add(FString::Printf(TEXT("its medium '%s' has no line in Data/%s"), *Work.Medium, FArtMedia::Record));
				return;
			}
			for (const FArtHangEntry* Entry : Canvases)
			{
				FCanvasPlan& Canvas = OutPlan.Canvases.AddDefaulted_GetRef();
				Canvas.Entry = Entry;
				Canvas.Finish = { Medium->Palette, Medium->Working, Medium->History };
			}
		}

		/**
		 * A work's plan from its checked objects: each one's place in the one image or its own image, its layer and
		 * finish; and the one image's size. Faults, one clause each, where an image the plan needs can't be found.
		 */
		FWorkPlan PlanOf(const FMuseeStageContext& Stage, FArtWork&& Work, const TArray<const FArtHangEntry*>& Canvases, const FEntryContext& Context,
			TArray<FString>& OutFaults)
		{
			FWorkPlan Plan;
			Plan.bOwnImages = !Canvases[0]->Image.IsEmpty();
			if (IsInMedium(*Canvases[0]))
			{
				PlanInMedium(Work, Canvases, Context, Plan, OutFaults);
				Plan.Work = MoveTemp(Work);
				return Plan;
			}
			for (const FArtHangEntry* Entry : Canvases)
			{
				FCanvasPlan& Canvas = Plan.Canvases.AddDefaulted_GetRef();
				Canvas.Entry = Entry;
				FinishOf(*Entry, Context, Canvas);
				Canvas.GlazingMeters = GlazingMeters(*Entry, Context);
				Canvas.PlateMeters = IsTransparency(Stage, *Entry) ? PaneMeters(Entry->Wall, Context) : 0.0;
				Canvas.BackingMeters = PaneMeters(Entry->Backing, Context);
				Canvas.Layer = Plan.bOwnImages ? Musee::Art::PaintMaterial(Entry->Work, Entry->Canvas) : Musee::Art::PaintMaterial(Entry->Work);
				const bool bWare = Entry->Support == Supports::Earthenware;
				if (bWare)
				{
					Canvas.Ware = WareOf(*Entry, Context);
				}
				if (Plan.bOwnImages)
				{
					const TValueOrError<FArtWork, FString> Own = Musee::Art::ReadWork(*Entry);
					Canvas.OwnImagePath = Own.HasValue() ? Own.GetValue().ImagePath : FString();
					if (Canvas.OwnImagePath.IsEmpty())
					{
						OutFaults.Add(FString::Printf(TEXT("object %d: no image '%s' is in the collection"), Entry->Canvas, *Entry->Image));
					}
					continue;
				}
				if (bWare)
				{
					// Ware shows its photograph whole: the image spans the photograph, the piece its window in it.
					Plan.ImageMeters = WareImageMeters(Canvas.Ware);
					continue;
				}
				Canvas.ImageOffsetMeters = Plan.ImageMeters.X;
				Plan.ImageMeters = FVector2D(Plan.ImageMeters.X + Entry->Size[0], Entry->Size[1]);
			}
			if (!Plan.bOwnImages && Work.ImagePath.IsEmpty())
			{
				OutFaults.Add(TEXT("no image of it is in the collection"));
			}
			Plan.Work = MoveTemp(Work);
			return Plan;
		}

		bool Offers(const FMatterPaletteEntry& Entry, FName Working, FName History)
		{
			return Entry.Finishes.ContainsByPredicate([Working, History](const FMatterPaletteFinish& Finish)
			{
				return Finish.Working.ToName() == Working && Finish.History.ToName() == History;
			});
		}

		/** Whether every support's record reads and the palette offers the finishes it names; why not, if not. */
		bool ReadSupportFinishes(FString& OutWhy)
		{
			const TValueOrError<FArtSupports, FString>& Supports = ReadSupports();
			const TValueOrError<FMatterPalette, FString> Palette = Musee::Matter::ReadPalette();
			if (Supports.HasError() || Palette.HasError())
			{
				OutWhy = Supports.HasError() ? Supports.GetError() : Palette.GetError();
				return false;
			}
			for (const FArtSupport& Support : Supports.GetValue().Supports)
			{
				const FMatterPaletteEntry* Entry = Palette.GetValue().Find(FMuseeKey(Support.Palette.ToString()));
				const bool bGround = !Support.GroundWorking.IsNone();
				if (!Entry || !Offers(*Entry, Support.Working, Support.History) || (bGround && !Offers(*Entry, Support.GroundWorking, Support.GroundHistory)))
				{
					OutWhy = FString::Printf(TEXT("the palette offers no finish %s/%s/%s%s that %s names for %s"), *Support.Palette.ToString(),
						*Support.Working.ToString(), *Support.History.ToString(), bGround ? TEXT(" (or its ground's)") : TEXT(""), FArtSupports::Record,
						*Support.Support.ToString());
					return false;
				}
			}
			return true;
		}

		/** The turned forms of the plan's ware, read once when the plan holds ware. */
		void ReadTurnedInto(const FArtHang& Hang, FEntryContext& Context)
		{
			if (!Hang.Works.ContainsByPredicate([](const FArtHangEntry& Entry) { return !Entry.IsBuilt() && Entry.Support == Supports::Earthenware; }))
			{
				return;
			}
			TValueOrError<TArray<FArtTurned>, FString> Turned = ReadRecords<FArtTurned>(FArtTurned::Record);
			if (Turned.HasValue())
			{
				Context.Turned = Turned.StealValue();
			}
			else
			{
				Context.TurnedWhy = Turned.GetError();
			}
		}

		/** A finish a record of works in the round names, and whose it is, for the palette to offer. */
		struct FNamedFinish
		{
			FName Palette;
			FName Working;
			FName History;
			FString For;
		};

		/** Why the palette doesn't offer a finish a record names, or empty when it offers every one. */
		FString UnofferedFinish(const FMatterPalette& Palette, TConstArrayView<FNamedFinish> Finishes, const TCHAR* Record)
		{
			for (const FNamedFinish& Finish : Finishes)
			{
				const FMatterPaletteEntry* Entry = Palette.Find(FMuseeKey(Finish.Palette.ToString()));
				if (!Entry || !Offers(*Entry, Finish.Working, Finish.History))
				{
					return FString::Printf(TEXT("the palette offers no finish %s/%s/%s that Data/%s names for %s"), *Finish.Palette.ToString(),
						*Finish.Working.ToString(), *Finish.History.ToString(), Record, *Finish.For);
				}
			}
			return FString();
		}

		/** Every finish the records of works in the round name: each medium's, each cast part's and the vessels' stand. */
		FString RoundFinishesWhy(const FMatterPalette& Palette, const FArtMedia& Media, const FArtCasts& Casts, const FArtVessels& Vessels)
		{
			TArray<FNamedFinish> OfMedia;
			for (const FArtMedium& Medium : Media.Media)
			{
				OfMedia.Add({ Medium.Palette, Medium.Working, Medium.History, Medium.Medium });
			}
			TArray<FNamedFinish> OfParts;
			for (const FArtCastPart& Part : Casts.Parts)
			{
				OfParts.Add({ Part.Palette, Part.Working, Part.History, Part.Work + TEXT("'s ") + Part.Part.ToString() });
			}
			const FNamedFinish Stand{ Vessels.Stand.Palette, Vessels.Stand.Working, Vessels.Stand.History, TEXT("the vessels' stand") };
			FString Why = UnofferedFinish(Palette, OfMedia, FArtMedia::Record);
			Why = Why.IsEmpty() ? UnofferedFinish(Palette, OfParts, FArtCasts::Record) : Why;
			return Why.IsEmpty() ? UnofferedFinish(Palette, MakeArrayView(&Stand, 1), FArtVessels::Record) : Why;
		}

		/**
		 * How works in the round are made (their media, how they are cast and turned), read once when the plan holds any,
		 * with every finish the records name offered by the palette.
		 */
		void ReadRoundInto(const FArtHang& Hang, FEntryContext& Context)
		{
			if (!Hang.Works.ContainsByPredicate([](const FArtHangEntry& Entry) { return !Entry.IsBuilt() && IsInMedium(Entry); }))
			{
				return;
			}
			TValueOrError<FArtMedia, FString> Media = ReadRecord<FArtMedia>(FArtMedia::Record);
			TValueOrError<FArtCasts, FString> Casts = ReadRecord<FArtCasts>(FArtCasts::Record);
			TValueOrError<FArtVessels, FString> Vessels = ReadRecord<FArtVessels>(FArtVessels::Record);
			const TValueOrError<FMatterPalette, FString> Palette = Musee::Matter::ReadPalette();
			Context.RoundWhy = Media.HasError() ? Media.GetError() : Casts.HasError() ? Casts.GetError() : Vessels.HasError() ? Vessels.GetError()
				: Palette.HasError() ? Palette.GetError() : RoundFinishesWhy(Palette.GetValue(), Media.GetValue(), Casts.GetValue(), Vessels.GetValue());
			if (Context.RoundWhy.IsEmpty())
			{
				Context.Media.Emplace(Media.StealValue());
				Context.Casts.Emplace(Casts.StealValue());
				Context.Vessels.Emplace(Vessels.StealValue());
			}
		}

		/** The bindings of the plan's books, read once when the plan holds a book. */
		void ReadBooksInto(const FArtHang& Hang, FEntryContext& Context)
		{
			if (!Hang.Works.ContainsByPredicate([](const FArtHangEntry& Entry) { return !Entry.IsBuilt() && Entry.Kind == Kinds::Book; }))
			{
				return;
			}
			TValueOrError<TArray<FArtBook>, FString> Books = ReadRecords<FArtBook>(FArtBook::Record);
			if (Books.HasValue())
			{
				Context.Books = Books.StealValue();
			}
			else
			{
				Context.BooksWhy = Books.GetError();
			}
		}

		void ReadFinishesInto(const FMuseeStageContext& Stage, const FArtHang& Hang, FEntryContext& Context)
		{
			const bool bNamesSurfaces = Hang.Works.ContainsByPredicate([](const FArtHangEntry& Entry)
			{
				return !Entry.IsBuilt() && (IsFramed(Entry) || Entry.Support == Supports::Glass || !Entry.Mat.IsNone() || Entry.Glazed || !Entry.Wall.IsNone());
			});
			if (!bNamesSurfaces)
			{
				return;
			}
			TValueOrError<FMatterFinishSchedule, FString> Finishes = Musee::Matter::ReadFinishSchedule(ScheduleFolder(Stage));
			if (Finishes.HasValue())
			{
				Context.Finishes.Emplace(Finishes.StealValue());
			}
			else
			{
				Context.FinishesWhy = Finishes.GetError();
			}
		}

		/** The faults of one work's objects together and each on its own, one clause each. */
		TArray<FString> WorkFaults(const FMuseeStageContext& Stage, const TArray<const FArtHangEntry*>& Canvases, const FEntryContext& Context)
		{
			TArray<FString> Faults = CanvasFaults(Canvases);
			for (const FArtHangEntry* Entry : Canvases)
			{
				for (const FString& Fault : EntryFaults(Stage, *Entry, Context))
				{
					Faults.Add(Canvases.Num() > 1 ? FString::Printf(TEXT("object %d: %s"), Entry->Canvas, *Fault) : Fault);
				}
			}
			return Faults;
		}
	}

	FMuseeFinding ArtFinding(const FMuseeStageContext& Stage, const FString& Summary)
	{
		FMuseeFinding Found = FMuseeFinding::Make(Stage.Stage.ToName(), Summary, FMuseeMeasurement());
		Found.Owner = ArtSystem;
		return Found;
	}

	bool ReadStageHang(const FMuseeStageContext& Stage, FArtHang& OutHang, TArray<FMuseeFinding>& OutFindings)
	{
		const FString Path = ScheduleFolder(Stage) / FArtHang::FileName;
		if (!IFileManager::Get().FileExists(*Path))
		{
			return false;
		}
		TValueOrError<FArtHang, FString> Hang = ReadRecord<FArtHang>(Path);
		if (Hang.HasError())
		{
			OutFindings.Add(ArtFinding(Stage, Hang.GetError()));
			return false;
		}
		if (!(Hang.GetValue().Stage == Stage.Stage))
		{
			OutFindings.Add(ArtFinding(Stage, FString::Printf(TEXT("%s names the stage %s."), *Path, *Hang.GetValue().Stage.ToString())));
			return false;
		}
		OutHang = Hang.StealValue();
		return true;
	}

	TValueOrError<FCareKeys, FString> ReadCareKeys()
	{
		TValueOrError<TArray<FName>, FString> Classes = Musee::Art::CareClasses();
		TValueOrError<TArray<FName>, FString> Reproductions = Musee::Art::Reproductions();
		if (Classes.HasError() || Reproductions.HasError())
		{
			return MakeError(Classes.HasError() ? Classes.StealError() : Reproductions.StealError());
		}
		return MakeValue(FCareKeys{ Classes.StealValue(), Reproductions.StealValue() });
	}

	TValueOrError<FArtWork, FString> ResolveEntry(const FArtHangEntry& Entry, const FCareKeys& Care)
	{
		TArray<FString> Faults;
		if (!Entry.Built.IsNone() && !Entry.IsBuilt())
		{
			Faults.Add(FString::Printf(TEXT("it is built '%s'; a work the stage builds is built 'form'"), *Entry.Built.ToString()));
		}
		if (!Care.Classes.Contains(Entry.Care))
		{
			Faults.Add(FString::Printf(TEXT("its care class '%s' is not one of Data/Art/Care.json's"), *Entry.Care.ToString()));
		}
		if (Entry.IsReproduction() && !Care.Reproductions.Contains(Entry.Reproduction))
		{
			Faults.Add(FString::Printf(TEXT("its reproduction '%s' is not one of Data/Art/Care.json's"), *Entry.Reproduction.ToString()));
		}
		const FVector2D Original = Entry.After.OriginalMeters();
		if (!Entry.After.Size.IsEmpty() && (!Entry.After.IsSet() || Original.X <= 0.0 || Original.Y <= 0.0))
		{
			Faults.Add(TEXT("the original it is made after needs a size [w, h] of more than zero"));
		}
		if (!Faults.IsEmpty())
		{
			return MakeError(FString::Join(Faults, TEXT("; ")));
		}
		return Musee::Art::ReadWork(Entry);
	}

	bool PlanWorks(const FMuseeStageContext& Stage, const FArtHang& Hang, TArray<FWorkPlan>& OutWorks, TArray<FMuseeFinding>& OutFindings)
	{
		FEntryContext Context;
		FString Why;
		if (!ReadSupportFinishes(Why))
		{
			OutFindings.Add(ArtFinding(Stage, FString::Printf(TEXT("No work can be made: %s."), *Why)));
			return false;
		}
		TValueOrError<FCareKeys, FString> Care = ReadCareKeys();
		if (Care.HasError())
		{
			OutFindings.Add(ArtFinding(Stage, Care.GetError()));
		}
		else
		{
			Context.Care = Care.StealValue();
		}
		ReadFinishesInto(Stage, Hang, Context);
		Context.Meshes = Hang.Meshes;
		ReadTurnedInto(Hang, Context);
		ReadRoundInto(Hang, Context);
		ReadBooksInto(Hang, Context);
		for (const TArray<const FArtHangEntry*>& Canvases : WorksOf(Hang))
		{
			const FString& Id = Canvases[0]->Work;
			TArray<FString> Faults = WorkFaults(Stage, Canvases, Context);
			TValueOrError<FArtWork, FString> Work = Faults.IsEmpty() ? ResolveEntry(*Canvases[0], Context.Care) : MakeError(FString());
			if (Faults.IsEmpty() && Work.HasError())
			{
				Faults.Add(Work.GetError());
			}
			if (Faults.IsEmpty())
			{
				FWorkPlan Plan = PlanOf(Stage, Work.StealValue(), Canvases, Context, Faults);
				if (Faults.IsEmpty())
				{
					OutWorks.Add(MoveTemp(Plan));
					continue;
				}
			}
			OutFindings.Add(ArtFinding(Stage, FString::Printf(TEXT("Work %s: %s."), *Id, *FString::Join(Faults, TEXT("; ")))));
		}
		return true;
	}

	TArray<FArtPaintLayer> PaintLayers(const FMuseeStageContext& Stage)
	{
		TArray<FArtPaintLayer> Out;
		FArtHang Hang;
		TArray<FMuseeFinding> Reported;
		TArray<FWorkPlan> Works;
		if (!ReadStageHang(Stage, Hang, Reported) || !PlanWorks(Stage, Hang, Works, Reported))
		{
			return Out;
		}
		for (const FWorkPlan& Plan : Works)
		{
			for (const FCanvasPlan& Canvas : Plan.Canvases)
			{
				if (IsInMedium(*Canvas.Entry))
				{
					// A work in the round shows no image: it is its medium's finish throughout.
					break;
				}
				FArtPaintLayer& Layer = Out.AddDefaulted_GetRef();
				Layer.Work = Plan.bOwnImages ? FString::Printf(TEXT("%s.%d"), *Plan.Work.Id, Canvas.Entry->Canvas) : Plan.Work.Id;
				Layer.ImagePath = Plan.bOwnImages ? Canvas.OwnImagePath : Plan.Work.ImagePath;
				const bool bWare = Canvas.Entry->Support == Supports::Earthenware;
				Layer.SizeMeters = !Plan.bOwnImages ? Plan.ImageMeters : bWare ? WareImageMeters(Canvas.Ware) : Canvas.Entry->SizeMeters();
				Layer.Palette = Canvas.Layer;
				Layer.Parent = Canvas.Finish.Palette;
				Layer.Working = Canvas.Finish.Working;
				Layer.History = Canvas.Finish.History;
				if (!Plan.bOwnImages)
				{
					// The work's canvases share one image: one layer.
					break;
				}
			}
		}
		return Out;
	}
}
