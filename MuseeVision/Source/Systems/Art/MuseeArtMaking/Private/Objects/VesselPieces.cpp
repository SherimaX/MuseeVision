#include "ArtPieces.h"
#include "FormMake.h"
#include "MuseeCollision.h"
#include "MuseeRecords.h"
#include "Objects/ObjectMaking.h"

namespace Musee::ArtMaking
{
	namespace
	{
		/** The section scaled to the plan's size: its widest radius to half the width, its height to the height, its foot on 0. */
		TArray<FVector2D> SizedSection(const FArtVessel& Vessel, const FVector2D& SizeMeters)
		{
			FBox2D Bounds(ForceInit);
			for (const FVector2D& Point : Vessel.Section)
			{
				Bounds += Point;
			}
			const FVector2D Scale(0.5 * SizeMeters.X / Bounds.Max.X, SizeMeters.Y / (Bounds.Max.Y - Bounds.Min.Y));
			TArray<FVector2D> Out;
			for (const FVector2D& Point : Vessel.Section)
			{
				Out.Add(FVector2D(Point.X * Scale.X, (Point.Y - Bounds.Min.Y) * Scale.Y));
			}
			return Out;
		}

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

		/** The radius of a section's foot: its widest point at its lowest height. */
		double FootRadius(TConstArrayView<FVector2D> Points)
		{
			double Lowest = TNumericLimits<double>::Max();
			double Radius = 0.0;
			for (const FVector2D& Point : Points)
			{
				Radius = Point.Y < Lowest ? Point.X : (Point.Y == Lowest ? FMath::Max(Radius, Point.X) : Radius);
				Lowest = FMath::Min(Lowest, Point.Y);
			}
			return Radius;
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
		if (Vessels.HasError())
		{
			OutFindings.Add(ArtFinding(Making.Stage, FString::Printf(TEXT("Work %s: it can't be turned: %s."), *Entry.Work, *Vessels.GetError())));
			return;
		}
		const FArtVesselStand& Stand = Vessels.GetValue().Stand;
		const TArray<FVector2D> Section = SizedSection(*Vessels.GetValue().Find(Entry.Work), Entry.SizeMeters());
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
		Musee::FormMaking::Make(Turning(FName(ObjectName(Entry, bSplit)), FName(ObjectWord(Entry.Support)), Traced(Section, Vessels.GetValue().CreaseDegrees),
			Canvas.Finish, OnStand), OutPieces, OutFindings);
	}
}
