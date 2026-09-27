#include "ArtHangReading.h"

#include "Algo/StableSort.h"
#include "ArtCare.h"
#include "ArtPaintLayer.h"
#include "HAL/FileManager.h"
#include "MatterRecords.h"
#include "Misc/Paths.h"
#include "MuseeRecords.h"

namespace Musee::ArtMaking
{
	namespace
	{
		const FName ArtSystem(TEXT("Art"));
		const FName CanvasSupport(TEXT("canvas"));
		const FName NoFrame(TEXT("none"));

		FString ScheduleFolder(const FMuseeStageContext& Stage)
		{
			return FPaths::IsRelative(Stage.ScheduleDir) ? Records::DataRoot() / Stage.ScheduleDir : Stage.ScheduleDir;
		}

		/** What the checks of a plan's entries read once: the care classes and the stage's framed surfaces. */
		struct FEntryContext
		{
			TArray<FName> CareClasses;
			TOptional<FMatterFinishSchedule> Finishes;
			FString FinishesWhy;
		};

		bool IsFramed(const FArtHangEntry& Entry)
		{
			return !Entry.Frame.IsNone() && Entry.Frame != NoFrame;
		}

		/** What is wrong with one canvas's entry on its own, one clause each. */
		TArray<FString> EntryFaults(const FMuseeStageContext& Stage, const FArtHangEntry& Entry, const FEntryContext& Context)
		{
			TArray<FString> Out;
			if (!Stage.FindAnchor(Entry.Anchor, WallAnchorKind))
			{
				Out.Add(FString::Printf(TEXT("%s is not a Wall anchor of the stage"), *Entry.Anchor.ToString()));
			}
			if (Entry.Size.Num() != 2 || Entry.Size[0] <= 0.0 || Entry.Size[1] <= 0.0 || Entry.Along < 0.0f || Entry.Height < 0.0f)
			{
				Out.Add(TEXT("it needs a size [w, h] of more than zero, and an along and a height of zero or more"));
			}
			if (Entry.Support != CanvasSupport)
			{
				Out.Add(FString::Printf(TEXT("its support '%s' isn't made yet; a canvas is"), *Entry.Support.ToString()));
			}
			if (IsFramed(Entry) && !Context.Finishes.IsSet())
			{
				Out.Add(FString::Printf(TEXT("its frame's surface %s has no finish: %s"), *Entry.Frame.ToString(), *Context.FinishesWhy));
			}
			else if (IsFramed(Entry) && !Context.Finishes->Find(Entry.Frame))
			{
				Out.Add(FString::Printf(TEXT("its frame '%s' is none of the surfaces in the stage's Finishes.json"), *Entry.Frame.ToString()));
			}
			if (!Context.CareClasses.Contains(Entry.Care))
			{
				Out.Add(FString::Printf(TEXT("its care class '%s' is not one of Data/Art/Care.json's"), *Entry.Care.ToString()));
			}
			if (Musee::Art::PaintMaterial(Entry.Work).IsNone())
			{
				Out.Add(TEXT("its id can't name a paint layer (it must be one key segment)"));
			}
			return Out;
		}

		/** What is wrong with a work's canvases together: each index from 0 once, and one height, as one image is. */
		TArray<FString> CanvasFaults(const TArray<const FArtHangEntry*>& Canvases)
		{
			TArray<FString> Out;
			for (int32 Index = 0; Index < Canvases.Num(); ++Index)
			{
				if (Canvases[Index]->Canvas != Index)
				{
					Out.Add(FString::Printf(TEXT("its canvases must be numbered once each from 0 (canvas %d is hung where canvas %d belongs)"),
						Canvases[Index]->Canvas, Index));
					break;
				}
			}
			const bool bOneHeight = !Canvases.ContainsByPredicate([&Canvases](const FArtHangEntry* Entry)
			{
				return Entry->Size.Num() != 2 || Canvases[0]->Size.Num() != 2 || !FMath::IsNearlyEqual(Entry->Size[1], Canvases[0]->Size[1]);
			});
			if (!bOneHeight)
			{
				Out.Add(TEXT("its canvases carry one image, so they are one height"));
			}
			return Out;
		}

		/** The plan's canvases grouped by work, in the order the works first appear, each work's by canvas index. */
		TArray<TArray<const FArtHangEntry*>> WorksOf(const FArtHang& Hang)
		{
			TArray<TArray<const FArtHangEntry*>> Out;
			TMap<FString, int32> IndexOf;
			for (const FArtHangEntry& Entry : Hang.Works)
			{
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

		/** A work's plan from its checked canvases: each canvas's place in the image, and the image's size. */
		FWorkPlan PlanOf(FArtWork&& Work, const TArray<const FArtHangEntry*>& Canvases)
		{
			FWorkPlan Plan;
			Plan.Work = MoveTemp(Work);
			for (const FArtHangEntry* Entry : Canvases)
			{
				Plan.Canvases.Add({ Entry, Plan.ImageMeters.X });
				Plan.ImageMeters = FVector2D(Plan.ImageMeters.X + Entry->Size[0], Entry->Size[1]);
			}
			return Plan;
		}

		/** The paint palette entry's one finish, or why it has none. */
		bool ReadPaintFinish(FPaintFinish& OutPaint, FString& OutWhy)
		{
			const TValueOrError<FMatterPalette, FString> Palette = Musee::Matter::ReadPalette();
			const FMatterPaletteEntry* Entry = Palette.HasValue() ? Palette.GetValue().Find(FMuseeKey(PaintPalette)) : nullptr;
			if (!Entry || Entry->Finishes.Num() != 1)
			{
				OutWhy = Palette.HasError() ? Palette.GetError()
					: FString::Printf(TEXT("the paint's palette entry %s must offer one finish, its working and history"), PaintPalette);
				return false;
			}
			OutPaint = { Entry->Finishes[0].Working.ToName(), Entry->Finishes[0].History.ToName() };
			return true;
		}

		FEntryContext ReadEntryContext(const FMuseeStageContext& Stage, const FArtHang& Hang, TArray<FMuseeFinding>& OutFindings)
		{
			FEntryContext Context;
			const TValueOrError<TArray<FName>, FString> Care = Musee::Art::CareClasses();
			if (Care.HasError())
			{
				OutFindings.Add(ArtFinding(Stage, Care.GetError()));
			}
			else
			{
				Context.CareClasses = Care.GetValue();
			}
			if (Hang.Works.ContainsByPredicate(&IsFramed))
			{
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
			return Context;
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

	bool PlanWorks(const FMuseeStageContext& Stage, const FArtHang& Hang, TArray<FWorkPlan>& OutWorks, FPaintFinish& OutPaint,
		TArray<FMuseeFinding>& OutFindings)
	{
		FString Why;
		if (!ReadPaintFinish(OutPaint, Why))
		{
			OutFindings.Add(ArtFinding(Stage, FString::Printf(TEXT("No work can be painted: %s."), *Why)));
			return false;
		}
		const FEntryContext Context = ReadEntryContext(Stage, Hang, OutFindings);
		for (const TArray<const FArtHangEntry*>& Canvases : WorksOf(Hang))
		{
			const FString& Id = Canvases[0]->Work;
			TArray<FString> Faults = CanvasFaults(Canvases);
			for (const FArtHangEntry* Entry : Canvases)
			{
				for (const FString& Fault : EntryFaults(Stage, *Entry, Context))
				{
					Faults.Add(Canvases.Num() > 1 ? FString::Printf(TEXT("canvas %d: %s"), Entry->Canvas, *Fault) : Fault);
				}
			}
			TValueOrError<FArtWork, FString> Work = Faults.IsEmpty() ? Musee::Art::ReadWork(*Canvases[0]) : MakeError(FString());
			if (Faults.IsEmpty() && (Work.HasError() || Work.GetValue().ImagePath.IsEmpty()))
			{
				Faults.Add(Work.HasError() ? Work.GetError() : TEXT("no image of it is in the repository's assets"));
			}
			if (!Faults.IsEmpty())
			{
				OutFindings.Add(ArtFinding(Stage, FString::Printf(TEXT("Work %s: %s."), *Id, *FString::Join(Faults, TEXT("; ")))));
				continue;
			}
			OutWorks.Add(PlanOf(Work.StealValue(), Canvases));
		}
		return true;
	}

	TArray<FArtPaintLayer> PaintLayers(const FMuseeStageContext& Stage)
	{
		TArray<FArtPaintLayer> Out;
		FArtHang Hang;
		TArray<FMuseeFinding> Reported;
		TArray<FWorkPlan> Works;
		FPaintFinish Paint;
		if (!ReadStageHang(Stage, Hang, Reported) || !PlanWorks(Stage, Hang, Works, Paint, Reported))
		{
			return Out;
		}
		for (const FWorkPlan& Plan : Works)
		{
			FArtPaintLayer& Layer = Out.AddDefaulted_GetRef();
			Layer.Work = Plan.Work.Id;
			Layer.ImagePath = Plan.Work.ImagePath;
			Layer.SizeMeters = Plan.ImageMeters;
			Layer.Palette = Musee::Art::PaintMaterial(Plan.Work.Id);
			Layer.Parent = PaintPalette;
			Layer.Working = Paint.Working;
			Layer.History = Paint.History;
		}
		return Out;
	}
}
