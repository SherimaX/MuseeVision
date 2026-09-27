#include "Frame/Moulding.h"

#include "Canvas/CanvasMesh.h"

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
		 * The frame's closed section: the moulding's face at its width, then under the lip along the canvas's face to the
		 * rebate, down the rebate to the frame's back (standing on the hanging gap, as the stretcher's does), and along
		 * the back to the outside edge.
		 */
		FFormProfile Section(const FArtMoulding& Moulding, double Width)
		{
			const double Back = -(StretcherDepthMeters - HangingGapMeters);
			const double Outside = Moulding.Face.StartPoint().X * Width;
			FFormProfile Out = ScaledProfile(Moulding.Face, Width);
			Out.LineTo(FVector2D(Moulding.RebateClearanceMeters, 0.0), EFormJoint::Crisp)
				.LineTo(FVector2D(Moulding.RebateClearanceMeters, Back), EFormJoint::Crisp)
				.LineTo(FVector2D(Outside, Back), EFormJoint::Crisp)
				.Close();
			return Out;
		}
	}

	FFormDrawing FrameDrawing(const FArtMoulding& Moulding, const FVector2D& CanvasMeters, const FMuseeFrame& CanvasTopLeft, FName Name, FName Surface)
	{
		FFormSweep Sweep;
		Sweep.Section = Section(Moulding, Moulding.WidthFor(CanvasMeters));
		// Run anticlockwise as the room sees it (x along the top, y down), so the moulding lies outside the canvas.
		const TArray<FVector2D> Edge = { FVector2D(0.0, 0.0), FVector2D(0.0, CanvasMeters.Y), CanvasMeters, FVector2D(CanvasMeters.X, 0.0) };
		Sweep.Path = FFormProfile::Polyline(Edge, true);
		Sweep.ElevationMeters = StretcherDepthMeters;
		FFormDrawing Drawing = FFormDrawing::Of(Name, Surface, MoveTemp(Sweep));
		Drawing.Frame = CanvasTopLeft;
		return Drawing;
	}
}

double FArtMoulding::WidthFor(const FVector2D& CanvasMeters) const
{
	return FMath::Clamp(WidthBaseMeters + WidthPerLongerSide * CanvasMeters.GetMax(), LeastWidthMeters, MostWidthMeters);
}
