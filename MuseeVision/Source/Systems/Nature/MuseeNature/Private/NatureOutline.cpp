#include "NatureOutline.h"

namespace
{
	/** Twice the signed area of a polygon (the shoelace formula): positive when its corners run anticlockwise in (x, y). */
	double TwiceSignedArea(TConstArrayView<FVector2D> Corners)
	{
		double Sum = 0.0;
		for (int32 Index = 0; Index < Corners.Num(); ++Index)
		{
			Sum += FVector2D::CrossProduct(Corners[Index], Corners[(Index + 1) % Corners.Num()]);
		}
		return Sum;
	}

	/** Whether a point lies inside a polygon: an odd count of its edges crossed by the ray from the point along +x. */
	bool InsidePolygon(TConstArrayView<FVector2D> Corners, const FVector2D& Point)
	{
		bool bInside = false;
		for (int32 Index = 0, Previous = Corners.Num() - 1; Index < Corners.Num(); Previous = Index++)
		{
			const FVector2D& A = Corners[Index];
			const FVector2D& B = Corners[Previous];
			if ((A.Y > Point.Y) != (B.Y > Point.Y) && Point.X < A.X + (B.X - A.X) * (Point.Y - A.Y) / (B.Y - A.Y))
			{
				bInside = !bInside;
			}
		}
		return bInside;
	}

	/** The distance from a point to the nearest edge of a polygon, metres. */
	double DistanceToEdges(TConstArrayView<FVector2D> Corners, const FVector2D& Point)
	{
		double Nearest = TNumericLimits<double>::Max();
		for (int32 Index = 0; Index < Corners.Num(); ++Index)
		{
			const FVector2D& A = Corners[Index];
			const FVector2D& B = Corners[(Index + 1) % Corners.Num()];
			const FVector2D Edge = B - A;
			const double Along = FMath::Clamp(FVector2D::DotProduct(Point - A, Edge) / FMath::Max(Edge.SizeSquared(), UE_DOUBLE_SMALL_NUMBER), 0.0, 1.0);
			Nearest = FMath::Min(Nearest, FVector2D::Distance(Point, A + Edge * Along));
		}
		return Nearest;
	}
}

bool FNatureOutline::IsValid() const
{
	if (IsEllipse())
	{
		return Polygon.IsEmpty() && Ellipse.Num() == 2 && Ellipse[0] > 0.0 && Ellipse[1] > 0.0;
	}
	return Polygon.Num() >= 3 && FMath::Abs(TwiceSignedArea(Polygon)) > UE_DOUBLE_SMALL_NUMBER;
}

FVector2D FNatureOutline::SemiAxes() const
{
	return IsEllipse() && IsValid() ? FVector2D(Ellipse[0], Ellipse[1]) : FVector2D::ZeroVector;
}

bool FNatureOutline::Contains(const FVector2D& Point, double MarginMeters) const
{
	if (!IsValid())
	{
		return false;
	}
	if (IsEllipse())
	{
		const FVector2D Axes = SemiAxes() - FVector2D(MarginMeters);
		return Axes.X > 0.0 && Axes.Y > 0.0 && FMath::Square(Point.X / Axes.X) + FMath::Square(Point.Y / Axes.Y) <= 1.0;
	}
	return InsidePolygon(Polygon, Point) && DistanceToEdges(Polygon, Point) >= MarginMeters;
}

double FNatureOutline::Area() const
{
	if (!IsValid())
	{
		return 0.0;
	}
	return IsEllipse() ? UE_DOUBLE_PI * Ellipse[0] * Ellipse[1] : 0.5 * FMath::Abs(TwiceSignedArea(Polygon));
}

FNatureOutline FNatureOutline::SeenFrom(const FMuseeFrame& Anchor) const
{
	if (IsEllipse())
	{
		return *this;
	}
	FNatureOutline Out;
	for (const FVector2D& Corner : Polygon)
	{
		const FVector Local = Anchor.PointFromParent(FVector(Corner.X, Corner.Y, Anchor.OriginMeters.Z));
		Out.Polygon.Add(FVector2D(Local.X, Local.Y));
	}
	return Out;
}
