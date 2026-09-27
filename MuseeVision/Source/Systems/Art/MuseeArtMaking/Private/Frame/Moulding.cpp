#include "Frame/Moulding.h"

#include "Misc/Paths.h"

namespace Musee::ArtMaking
{
	namespace
	{
		TArray<double> Scaled(const TArray<double>& Numbers, double Factor)
		{
			TArray<double> Out;
			for (const double Number : Numbers)
			{
				Out.Add(Number * Factor);
			}
			return Out;
		}

		/** A profile drawn in units, at a size in metres. */
		FFormProfile ScaledProfile(const FFormProfile& Profile, double Meters)
		{
			FFormProfile Out = Profile;
			Out.Start = Scaled(Profile.Start, Meters);
			for (FFormProfileStep& Step : Out.Steps)
			{
				Step.Line = Scaled(Step.Line, Meters);
				Step.Arc.CenterMeters = Scaled(Step.Arc.CenterMeters, Meters);
				Step.Arc.RadiiMeters = Scaled(Step.Arc.RadiiMeters, Meters);
			}
			return Out;
		}

		/**
		 * The frame's closed section: the moulding's face at its width, then under the lip along the face it rests on to
		 * the rebate, down the rebate to the frame's back, and along the back to the outside edge.
		 */
		FFormProfile Section(const FArtMoulding& Moulding, double Width, const FFrameDepth& Depth)
		{
			const double Back = Depth.BackMeters - Depth.FaceMeters;
			const double Outside = Moulding.Face.StartPoint().X * Width;
			FFormProfile Out = ScaledProfile(Moulding.Face, Width);
			Out.LineTo(FVector2D(Moulding.RebateClearanceMeters, 0.0), EFormJoint::Crisp)
				.LineTo(FVector2D(Moulding.RebateClearanceMeters, Back), EFormJoint::Crisp)
				.LineTo(FVector2D(Outside, Back), EFormJoint::Crisp)
				.Close();
			return Out;
		}
	}

	FFormDrawing FrameDrawing(const FArtMoulding& Moulding, const FVector2D& FramedMeters, const FMuseeFrame& TopLeft, const FFrameDepth& Depth,
		FName Name, FName Surface)
	{
		FFormSweep Sweep;
		Sweep.Section = Section(Moulding, Moulding.WidthFor(FramedMeters), Depth);
		// Run anticlockwise as the room sees it (x along the top, y down), so the moulding lies outside what it frames.
		const TArray<FVector2D> Edge = { FVector2D(0.0, 0.0), FVector2D(0.0, FramedMeters.Y), FramedMeters, FVector2D(FramedMeters.X, 0.0) };
		Sweep.Path = FFormProfile::Polyline(Edge, true);
		Sweep.ElevationMeters = Depth.FaceMeters;
		FFormDrawing Drawing = FFormDrawing::Of(Name, Surface, MoveTemp(Sweep));
		Drawing.Frame = TopLeft;
		return Drawing;
	}
}

FString FArtMoulding::RecordFor(FName FrameStyle)
{
	for (const TCHAR* Style : DrawnStyles)
	{
		if (FrameStyle == Style)
		{
			return FPaths::Combine(TEXT("Art/Mouldings"), FString(Style) + TEXT(".json"));
		}
	}
	return Record;
}

double FArtMoulding::WidthFor(const FVector2D& CanvasMeters) const
{
	return FMath::Clamp(WidthBaseMeters + WidthPerLongerSide * CanvasMeters.GetMax(), LeastWidthMeters, MostWidthMeters);
}
