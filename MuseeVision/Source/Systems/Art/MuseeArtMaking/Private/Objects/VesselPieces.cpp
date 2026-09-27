#include "ArtPieces.h"
#include "FormExtrusion.h"
#include "FormMake.h"
#include "MuseeCollision.h"
#include "MuseeRecords.h"
#include "Objects/ObjectMaking.h"
#include "Vessel/VesselMesh.h"

namespace Musee::ArtMaking
{
	namespace
	{
		/**
		 * The section as Form's profile through its traced points: smooth through a point where it turns less than the
		 * crease angle, an arris where it turns more.
		 */
		FFormProfile Traced(TConstArrayView<FVector2D> Points, double CreaseDegrees)
		{
			FFormProfile Profile = FFormProfile::From(Points[0]);
			const double Crease = FMath::DegreesToRadians(CreaseDegrees);
			for (int32 Index = 1; Index < Points.Num(); ++Index)
			{
				const FVector2D In = (Points[Index - 1] - (Index > 1 ? Points[Index - 2] : Points[Index - 1])).GetSafeNormal();
				const FVector2D Out = (Points[Index] - Points[Index - 1]).GetSafeNormal();
				const double Turn = FMath::Atan2(FMath::Abs(FVector2D::CrossProduct(In, Out)), FVector2D::DotProduct(In, Out));
				Profile.LineTo(Points[Index], Index > 1 && Turn < Crease ? EFormJoint::Smooth : EFormJoint::Crisp);
			}
			return Profile;
		}

		/** A turned drawing of Art's at a frame, in a finish, colliding as a solid. */
		FFormDrawing Turning(FName Name, FName Surface, FFormProfile&& Section, const FLayerFinish& Finish, const FMuseeFrame& Frame)
		{
			FFormLathe Lathe;
			Lathe.Section = MoveTemp(Section);
			FFormDrawing Drawing = FFormDrawing::Of(Name, Surface, MoveTemp(Lathe));
			Drawing.Material = Finish.Palette;
			Drawing.Working = Finish.Working;
			Drawing.History = Finish.History;
			Drawing.Collision = Collision::Solid.Profile();
			Drawing.Frame = Frame;
			Drawing.Drawer = TEXT("Art");
			return Drawing;
		}
	}

	namespace
	{
		/**
		 * A vessel's feet (FArtVesselFeet), each its traced outline stood up to their height in its medium's finish
		 * (`<name>.foot.<n>`), sized as the section is (SectionScale): across by its scale across, up by its scale up.
		 */
		void MakeFeet(const FArtVessel& Vessel, const FArtHangEntry& Entry, bool bSplit, const FLayerFinish& Finish, const FMuseeFrame& OnStand,
			TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
		{
			const FArtVesselFeet& Feet = Vessel.Feet;
			if (Feet.Each.IsEmpty() || !(Feet.HeightMeters > 0.0))
			{
				return;
			}
			const FVector2D Scale = SectionScale(Vessel, Entry.SizeMeters());
			const double Across = Scale.X;
			const double Rise = Scale.Y;
			for (int32 Index = 0; Index < Feet.Each.Num(); ++Index)
			{
				TArray<FVector2D> Outline;
				for (const FVector2D& Point : Feet.Each[Index].Outline)
				{
					Outline.Add(Point * Across);
				}
				FFormExtrusion Foot;
				Foot.Outline = FFormProfile::Polygon(Outline);
				Foot.DepthMeters = Feet.HeightMeters * Rise;
				FFormDrawing Drawing = FFormDrawing::Of(PartName(Entry, bSplit, *FString::Printf(TEXT("foot.%d"), Index + 1)), TEXT("foot"), MoveTemp(Foot));
				Drawing.Material = Finish.Palette;
				Drawing.Working = Finish.Working;
				Drawing.History = Finish.History;
				Drawing.Collision = Collision::Solid.Profile();
				Drawing.Frame = OnStand;
				Drawing.Drawer = TEXT("Art");
				Musee::FormMaking::Make(MoveTemp(Drawing), OutPieces, OutFindings);
			}
		}
	}

	const TValueOrError<FArtVessels, FString>& FObjectMaking::Vessels()
	{
		if (!VesselRecord.IsSet())
		{
			VesselRecord.Emplace(ReadRecord<FArtVessels>(FArtVessels::Record));
		}
		return VesselRecord.GetValue();
	}

	void MakeVessel(FObjectMaking& Making, const FWorkPlan& Plan, const FCanvasPlan& Canvas, TArray<FFormMadePiece>& OutPieces, TArray<FMuseeFinding>& OutFindings)
	{
		const FArtHangEntry& Entry = *Canvas.Entry;
		const TValueOrError<FArtVessels, FString>& Vessels = Making.Vessels();
		const TValueOrError<FArtWareForms, FString>& Forms = Making.Forms();
		if (Vessels.HasError() || Forms.HasError())
		{
			OutFindings.Add(ArtFinding(Making.Stage, FString::Printf(TEXT("Work %s: it can't be turned: %s."), *Entry.Work,
				Vessels.HasError() ? *Vessels.GetError() : *Forms.GetError())));
			return;
		}
		const FArtVessel& Vessel = *Vessels.GetValue().Find(Entry.Work);
		const FArtVesselStand& Stand = Vessels.GetValue().Stand;
		const TArray<FVector2D> Section = SizedSection(Vessel, Entry.SizeMeters());
		const bool bSplit = Plan.IsSplit();
		const FMuseeFrame Foot = RoundFrame(Entry, Making.Stage.FindAnchor(Entry.Anchor, MountAnchorKind)->Frame);
		double Raised = 0.0;
		if (Entry.Stand)
		{
			// The stand is a drum under the foot: its radius so many feet, its height by the vessel's.
			Raised = Entry.Size[1] < Stand.TallMeters ? Stand.LowMeters : Stand.HighMeters;
			const double Radius = Stand.FootRadii * FootRadius(Section);
			const FVector2D Drum[] = { FVector2D(0.0, 0.0), FVector2D(Radius, 0.0), FVector2D(Radius, Raised), FVector2D(0.0, Raised) };
			Musee::FormMaking::Make(Turning(PartName(Entry, bSplit, TEXT("stand")), TEXT("stand"), FFormProfile::Polyline(Drum, false),
				{ Stand.Palette, Stand.Working, Stand.History }, Foot), OutPieces, OutFindings);
		}
		const FMuseeFrame OnStand = Foot.FrameToParent(FMuseeFrame(FVector(0.0, 0.0, Raised)));
		const FName Name(ObjectName(Entry, bSplit));
		const FName Surface(ObjectWord(Entry.Support));
		if (!Vessel.IsPainted() && !Vessel.Museum.IsOval())
		{
			Musee::FormMaking::Make(Turning(Name, Surface, Traced(Section, Vessels.GetValue().CreaseDegrees), Canvas.Finish, OnStand), OutPieces, OutFindings);
			return;
		}
		// A painted vessel wears its decoration, composed in its turned surface's layout, the layout its UV0 carries; an
		// oval one is its turned section pressed across and front to back to its mouth's axes.
		FVesselTurning How;
		How.CreaseDegrees = Vessels.GetValue().CreaseDegrees;
		How.ChordToleranceMeters = Forms.GetValue().ChordToleranceMeters;
		if (Vessel.Museum.IsOval())
		{
			How.Oval = FVector2D(1.0, Vessel.Museum.MouthAxesMeters[1] / Vessel.Museum.MouthAxesMeters[0]);
		}
		const FLayerFinish Material = Vessel.IsPainted() ? FLayerFinish{ Canvas.Layer, Canvas.Finish.Working, Canvas.Finish.History } : Canvas.Finish;
		FFormMadePiece Piece = ArtPiece(Name, Surface, Material, OnStand, true);
		BuildVessel(Section, LayoutOf(Section), How, Piece.Mesh);
		OutPieces.Add(MoveTemp(Piece));
		MakeFeet(Vessel, Entry, bSplit, Canvas.Finish, OnStand, OutPieces, OutFindings);
	}
}
