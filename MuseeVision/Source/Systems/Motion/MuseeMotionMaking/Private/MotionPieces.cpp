#include "MotionPieces.h"

#include "FormMake.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "MotionSchedule.h"
#include "MuseeCollision.h"
#include "MuseeRecords.h"

namespace Musee::MotionMaking
{
	namespace
	{
		const FName Drawer(TEXT("Motion"));

		/**
		 * The rolled edge of each turn of a spiral-band column: the band's formed edges meet in a shallow V seam, drawn
		 * as a chamfer this deep on the outer edge of each turn (half of a 3 mm seam, the order of the band's thickness).
		 */
		constexpr double SeamChamferMeters = 0.0015;

		FMuseeFinding Finding(const FMuseeStageContext& Stage, const FString& Summary)
		{
			FMuseeFinding Found = FMuseeFinding::Make(Stage.Stage.ToName(), Summary, FMuseeMeasurement());
			Found.Owner = Drawer;
			return Found;
		}

		/** The extent's frame: at its anchored end, its z axis along the length. */
		FMuseeFrame ExtentFrame(const FMotionExtentSpec& Extent)
		{
			return FMuseeFrame(Extent.At, FQuat::FindBetweenNormals(FVector::UpVector, Extent.Axis.GetSafeNormal()));
		}

		FFormDrawing Part(const FMotionExtentSpec& Extent, FName Name, FFormLathe&& Lathe)
		{
			FFormDrawing Drawing = FFormDrawing::Of(Name, Extent.Surface, MoveTemp(Lathe));
			Drawing.Body = Extent.Body;
			Drawing.Collision = Collision::Solid.Profile();
			Drawing.Frame = ExtentFrame(Extent);
			Drawing.Drawer = Drawer;
			return Drawing;
		}

		/** A telescope's stage, home: a tube (or the solid plunger) whose far end stands at the closed length. */
		FFormDrawing Stage(const FMotionExtentSpec& Extent, int32 Index)
		{
			const FMotionTelescopeStage& Tube = Extent.Telescope[Index];
			const double Top = Extent.ClosedLength();
			const double Bottom = Top - Tube.Length;
			FFormLathe Lathe;
			Lathe.Section = Tube.Bore > 0.0
				? FFormProfile::Rectangle(FVector2D(Tube.Bore, Bottom), FVector2D(Tube.Radius, Top))
				: FFormProfile::Polyline({ FVector2D(0.0, Bottom), FVector2D(Tube.Radius, Bottom), FVector2D(Tube.Radius, Top), FVector2D(0.0, Top) }, false);
			return Part(Extent, Extent.StagePiece(Index), MoveTemp(Lathe));
		}

		/** One turn of a column: a ring one pitch deep from the anchored end, its outer edges rolled into the seam. */
		FFormDrawing Turn(const FMotionExtentSpec& Extent)
		{
			const FMotionTurns& Turns = Extent.Turns;
			const double Pitch = Turns.Pitch;
			FFormLathe Lathe;
			Lathe.Section = FFormProfile::Polyline({ FVector2D(Turns.Bore, 0.0), FVector2D(Turns.Radius - SeamChamferMeters, 0.0),
				FVector2D(Turns.Radius, SeamChamferMeters), FVector2D(Turns.Radius, Pitch - SeamChamferMeters),
				FVector2D(Turns.Radius - SeamChamferMeters, Pitch), FVector2D(Turns.Bore, Pitch) }, true);
			return Part(Extent, Extent.TurnPiece(), MoveTemp(Lathe));
		}
	}

	TArray<FFormDrawing> DrawExtent(const FMotionExtentSpec& Extent)
	{
		TArray<FFormDrawing> Drawings;
		for (int32 Index = 0; Index < Extent.Telescope.Num(); ++Index)
		{
			Drawings.Add(Stage(Extent, Index));
		}
		if (Extent.Telescope.IsEmpty() && Extent.Turns.IsSet())
		{
			Drawings.Add(Turn(Extent));
		}
		return Drawings;
	}

	void GatherParts(const FMuseeStageContext& Stage, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
	{
		const FString Folder = FPaths::IsRelative(Stage.ScheduleDir) ? Records::DataRoot() / Stage.ScheduleDir : Stage.ScheduleDir;
		const FString Path = Folder / FMotionSchedule::FileName;
		if (!IFileManager::Get().FileExists(*Path))
		{
			return;
		}
		const TValueOrError<FMotionSchedule, FString> Schedule = FMotionSchedule::Read(Path);
		if (Schedule.HasError())
		{
			OutFindings.Add(Finding(Stage, Schedule.GetError()));
			return;
		}
		for (const FMotionExtentSpec& Extent : Schedule.GetValue().Extents)
		{
			FormMaking::Make(DrawExtent(Extent), OutPieces, OutFindings);
		}
	}
}
