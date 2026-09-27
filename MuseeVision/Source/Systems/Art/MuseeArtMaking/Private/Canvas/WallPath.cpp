#include "Canvas/WallPath.h"

#include "Algo/BinarySearch.h"

namespace Musee::ArtMaking
{
	namespace
	{
		/** Chords stray at most a millimetre from the face: the museum's chord tolerance (Docs/Architecture.md, Form). */
		constexpr double ChordToleranceMeters = 0.001;

		/** The arc-length table's steps round a full turn: a 20 m ellipse is walked in 3 mm steps. */
		constexpr int32 ArcSteps = 20000;

		/** An anchor may stand this far off its path before it is a fault: half a centimetre. */
		constexpr double OnPathToleranceMeters = 0.005;

		FVector2D EllipsePoint(const FVector2D& Axes, double Theta)
		{
			return FVector2D(Axes.X * FMath::Cos(Theta), Axes.Y * FMath::Sin(Theta));
		}
	}

	FWallPath::FWallPath(const FMuseeFrame& InAnchor)
		: Anchor(InAnchor)
	{
	}

	FWallPath::FWallPath(const FMuseeFrame& InAnchor, const FArtWallPath& Path)
		: Anchor(InAnchor)
		, bCurved(true)
	{
		if (Path.Ellipse.Num() != 2 || Path.Ellipse[0] <= 0.0 || Path.Ellipse[1] <= 0.0)
		{
			Fault = TEXT("its path needs an ellipse of two semi-axes");
			return;
		}
		Axes = FVector2D(Path.Ellipse[0], Path.Ellipse[1]);
		Direction = Path.IsIncreasing() ? 1.0 : -1.0;
		StartTheta = FMath::DegreesToRadians(double(Path.From_Deg));
		ThetaStep = 2.0 * UE_DOUBLE_PI / ArcSteps;
		ArcAt.SetNum(ArcSteps + 1);
		ArcAt[0] = 0.0;
		for (int32 I = 1; I <= ArcSteps; ++I)
		{
			const FVector2D A = EllipsePoint(Axes, StartTheta + Direction * ThetaStep * (I - 1));
			const FVector2D B = EllipsePoint(Axes, StartTheta + Direction * ThetaStep * I);
			ArcAt[I] = ArcAt[I - 1] + FVector2D::Distance(A, B);
		}
		const FVector2D Start = EllipsePoint(Axes, StartTheta);
		if (FVector2D::Distance(Start, FVector2D(Anchor.OriginMeters.X, Anchor.OriginMeters.Y)) > OnPathToleranceMeters)
		{
			Fault = FString::Printf(TEXT("its anchor stands off its path's start (%.3f, %.3f)"), Start.X, Start.Y);
		}
		// The room is on the side of the face the anchor's y axis points to: inside the ellipse or outside it.
		const FVector2D Outward = FVector2D(Axes.Y * FMath::Cos(StartTheta), Axes.X * FMath::Sin(StartTheta)).GetSafeNormal();
		const FVector AnchorOut = Anchor.DirectionToParent(FVector(0.0, 1.0, 0.0));
		RoomSide = FVector2D::DotProduct(Outward, FVector2D(AnchorOut.X, AnchorOut.Y)) > 0.0 ? 1.0 : -1.0;
	}

	double FWallPath::ThetaAt(double DistanceMeters) const
	{
		const int32 Upper = FMath::Clamp(Algo::LowerBound(ArcAt, DistanceMeters), 1, ArcSteps);
		const double Span = ArcAt[Upper] - ArcAt[Upper - 1];
		const double Share = Span > 0.0 ? (DistanceMeters - ArcAt[Upper - 1]) / Span : 0.0;
		return StartTheta + Direction * ThetaStep * (Upper - 1 + Share);
	}

	FWallPath::FStation FWallPath::At(double DistanceMeters) const
	{
		FStation Station;
		if (!bCurved)
		{
			Station.Point = Anchor.PointToParent(FVector(DistanceMeters, 0.0, 0.0));
			Station.Along = Anchor.DirectionToParent(FVector(1.0, 0.0, 0.0));
			Station.Out = Anchor.DirectionToParent(FVector(0.0, 1.0, 0.0));
			return Station;
		}
		const double Theta = ThetaAt(DistanceMeters);
		const FVector2D P = EllipsePoint(Axes, Theta);
		const FVector2D Tangent = FVector2D(-Axes.X * FMath::Sin(Theta), Axes.Y * FMath::Cos(Theta)).GetSafeNormal() * Direction;
		const FVector2D Outward = FVector2D(Axes.Y * FMath::Cos(Theta), Axes.X * FMath::Sin(Theta)).GetSafeNormal() * RoomSide;
		Station.Point = FVector(P.X, P.Y, Anchor.OriginMeters.Z);
		Station.Along = FVector(Tangent.X, Tangent.Y, 0.0);
		Station.Out = FVector(Outward.X, Outward.Y, 0.0);
		return Station;
	}

	double FWallPath::StationSpacing(double SpanMeters) const
	{
		if (!bCurved)
		{
			return SpanMeters;
		}
		// A chord c on a curve of radius R strays c^2 / 8R from it; the tightest radius of an ellipse is b^2 / a.
		const double TightestRadius = FMath::Square(FMath::Min(Axes.X, Axes.Y)) / FMath::Max(Axes.X, Axes.Y);
		return FMath::Min(SpanMeters, 2.0 * FMath::Sqrt(2.0 * TightestRadius * ChordToleranceMeters));
	}

	double FWallPath::Length() const
	{
		return bCurved && ArcAt.Num() > 0 ? ArcAt.Last() : TNumericLimits<double>::Max();
	}
}
