#include "ArtPieces.h"

#include "ArtCare.h"
#include "ArtHangReading.h"
#include "ArtWork.h"
#include "Canvas/CanvasMesh.h"
#include "Canvas/WallPath.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "MuseeCollision.h"
#include "MuseeRecords.h"

namespace Musee::ArtMaking
{
	namespace
	{
		const FName ArtSystem(TEXT("Art"));
		const FName WallKind(TEXT("Wall"));
		const FName CanvasSupport(TEXT("canvas"));
		const FName NoFrame(TEXT("none"));
		const FName CanvasSurface(TEXT("canvas"));
		const FName LinenSurface(TEXT("canvas.linen"));

		/** The linen a canvas is woven of, bare where it turns round the stretcher: the palette's linen. */
		const FName LinenMaterial(TEXT("accent.linen"));

		/** What is wrong with a hung work's entry on its own, one sentence each. */
		TArray<FString> EntryFaults(const FMuseeStageContext& Stage, const FArtHangEntry& Entry, const TArray<FName>& CareClasses)
		{
			TArray<FString> Out;
			const FMuseeStageAnchor* Anchor = Stage.FindAnchor(Entry.Anchor);
			if (!Anchor || Anchor->Kind != WallKind)
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
			if (!Entry.Frame.IsNone() && Entry.Frame != NoFrame)
			{
				Out.Add(FString::Printf(TEXT("its frame '%s' isn't made yet; an unframed canvas is"), *Entry.Frame.ToString()));
			}
			if (!CareClasses.Contains(Entry.Care))
			{
				Out.Add(FString::Printf(TEXT("its care class '%s' is not one of Data/Art/Care.json's"), *Entry.Care.ToString()));
			}
			if (Musee::Art::PaintMaterial(Entry.Work).IsNone())
			{
				Out.Add(TEXT("its id can't name a paint layer (it must be one key segment)"));
			}
			return Out;
		}

		/** The wall a work hangs on: its anchor's straight face, or the curved path the plan gives it. */
		FWallPath WallOf(const FMuseeStageContext& Stage, const FArtHang& Hang, const FMuseeKey& Anchor)
		{
			const FMuseeFrame& Frame = Stage.FindAnchor(Anchor)->Frame;
			const FArtWall* Curved = Hang.Walls.FindByPredicate([&Anchor](const FArtWall& W) { return W.Anchor == Anchor; });
			return Curved ? FWallPath(Frame, Curved->Path) : FWallPath(Frame);
		}

		/** The work's own frame: at its start edge on the floor line of the wall's face, x to the viewer's right, y into the room. */
		FMuseeFrame WorkFrame(const FWallPath::FStation& Start)
		{
			const FVector Right = FVector::CrossProduct(Start.Out, FVector::UpVector).GetSafeNormal();
			return FMuseeFrame(Start.Point, FQuat(FMatrix(Right, Start.Out, FVector::UpVector, FVector::ZeroVector)));
		}

		void MakeWork(const FMuseeStageContext& Stage, const FArtHang& Hang, const FArtHangEntry& Entry, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
		{
			const TValueOrError<FArtWork, FString> Work = Musee::Art::ReadWork(Entry);
			if (Work.HasError() || Work.GetValue().ImagePath.IsEmpty())
			{
				OutFindings.Add(ArtFinding(Stage, FString::Printf(TEXT("Work %s: %s."), *Entry.Work,
					Work.HasError() ? *Work.GetError() : TEXT("no image of it is in the repository's assets"))));
				return;
			}
			const FWallPath Wall = WallOf(Stage, Hang, Entry.Anchor);
			const double Width = Entry.Size[0];
			if (!Wall.Faults().IsEmpty() || Entry.Along + Width > Wall.Length())
			{
				OutFindings.Add(ArtFinding(Stage, FString::Printf(TEXT("Work %s: its wall %s: %s."), *Entry.Work, *Entry.Anchor.ToString(),
					Wall.Faults().IsEmpty() ? TEXT("the work runs past the wall's end") : *Wall.Faults())));
				return;
			}
			const FWallPath::FStation Start = Wall.At(Entry.Along);
			FCanvasSpec Canvas{ Entry.Along, Width, Entry.Height, Entry.Size[1], true };
			// The image reads left to right as seen from the room, whichever way the wall's path runs.
			Canvas.bReadsWithPath = FVector::DotProduct(Start.Along, FVector::CrossProduct(Start.Out, FVector::UpVector)) > 0.0;
			const FMuseeFrame Frame = WorkFrame(Canvas.bReadsWithPath ? Start : Wall.At(Entry.Along + Width));
			FFormMadePiece Face, Linen;
			BuildCanvas(Wall, Canvas, Frame, Face.Mesh, Linen.Mesh);
			Face.Name = FacePieceName(Entry.Work);
			Face.Surface = CanvasSurface;
			Face.Material = Musee::Art::PaintMaterial(Entry.Work);
			Linen.Name = LinenPieceName(Entry.Work);
			Linen.Surface = LinenSurface;
			Linen.Material = LinenMaterial;
			for (FFormMadePiece* Piece : { &Face, &Linen })
			{
				Piece->Collision = Collision::Solid.Profile();
				Piece->bNanite = true;
				Piece->Frame = Frame;
				OutPieces.Add(MoveTemp(*Piece));
			}
		}
	}

	FName FacePieceName(const FString& WorkId)
	{
		return FName(FString::Printf(TEXT("canvas.%s"), *WorkId));
	}

	FName LinenPieceName(const FString& WorkId)
	{
		return FName(FString::Printf(TEXT("canvas.%s.linen"), *WorkId));
	}

	FMuseeFinding ArtFinding(const FMuseeStageContext& Stage, const FString& Summary)
	{
		FMuseeFinding Found = FMuseeFinding::Make(Stage.Stage.ToName(), Summary, FMuseeMeasurement());
		Found.Owner = ArtSystem;
		return Found;
	}

	bool ReadStageHang(const FMuseeStageContext& Stage, FArtHang& OutHang, TArray<FMuseeFinding>& OutFindings)
	{
		const FString Folder = FPaths::IsRelative(Stage.ScheduleDir) ? Records::DataRoot() / Stage.ScheduleDir : Stage.ScheduleDir;
		const FString Path = Folder / FArtHang::FileName;
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

	void GatherArt(const FMuseeStageContext& Stage, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
	{
		FArtHang Hang;
		if (!ReadStageHang(Stage, Hang, OutFindings))
		{
			return;
		}
		const TValueOrError<TArray<FName>, FString> Care = Musee::Art::CareClasses();
		if (Care.HasError())
		{
			OutFindings.Add(ArtFinding(Stage, Care.GetError()));
			return;
		}
		TSet<FString> Hung;
		for (const FArtHangEntry& Entry : Hang.Works)
		{
			TArray<FString> Faults = EntryFaults(Stage, Entry, Care.GetValue());
			bool bTwice = false;
			Hung.Add(Entry.Work, &bTwice);
			if (bTwice)
			{
				Faults.Add(TEXT("it is hung twice; a work is one object"));
			}
			if (!Faults.IsEmpty())
			{
				OutFindings.Add(ArtFinding(Stage, FString::Printf(TEXT("Work %s: %s."), *Entry.Work, *FString::Join(Faults, TEXT("; ")))));
				continue;
			}
			MakeWork(Stage, Hang, Entry, OutPieces, OutFindings);
		}
	}
}
