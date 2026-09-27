#include "Vessel/VesselSection.h"

#include "Algo/BinarySearch.h"

namespace Musee::ArtMaking
{
	TArray<FVector2D> SizedSection(const FArtVessel& Vessel, const FVector2D& SizeMeters)
	{
		FBox2D Bounds(ForceInit);
		for (const FVector2D& Point : Vessel.Section)
		{
			Bounds += Point;
		}
		const FVector2D Scale(0.5 * SizeMeters.X / Bounds.Max.X, SizeMeters.Y / Bounds.Max.Y);
		TArray<FVector2D> Out;
		Out.Reserve(Vessel.Section.Num());
		for (const FVector2D& Point : Vessel.Section)
		{
			Out.Add(Point * Scale);
		}
		return Out;
	}

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

	FVesselLayout LayoutOf(TConstArrayView<FVector2D> Section)
	{
		FVesselLayout Layout;
		double Length = 0.0;
		for (int32 Index = 0; Index < Section.Num(); ++Index)
		{
			Length += Index > 0 ? FVector2D::Distance(Section[Index], Section[Index - 1]) : 0.0;
			Layout.Lengths.Add(Length);
			Layout.ReferenceRadius = FMath::Max(Layout.ReferenceRadius, Section[Index].X);
		}
		return Layout;
	}

	FSectionPlace PlaceAlong(const FVesselLayout& Layout, double LengthMeters)
	{
		FSectionPlace Place;
		const int32 Last = Layout.Lengths.Num() - 1;
		if (Last < 1)
		{
			return Place;
		}
		// The last point at or before the distance, and never the section's end, so a segment follows it.
		const int32 Upper = Algo::UpperBound(Layout.Lengths, LengthMeters);
		Place.Segment = FMath::Clamp(Upper - 1, 0, Last - 1);
		const double Span = Layout.Lengths[Place.Segment + 1] - Layout.Lengths[Place.Segment];
		Place.Along = Span > 0.0 ? FMath::Clamp((LengthMeters - Layout.Lengths[Place.Segment]) / Span, 0.0, 1.0) : 0.0;
		return Place;
	}

	FVector2D PointAt(TConstArrayView<FVector2D> Section, const FSectionPlace& Place)
	{
		return FMath::Lerp(Section[Place.Segment], Section[Place.Segment + 1], Place.Along);
	}

	FVector2D NormalOf(TConstArrayView<FVector2D> Section, int32 Segment)
	{
		// The solid lies on the section's left, so the surface faces its right: (dz, -dr).
		const FVector2D Run = (Section[Segment + 1] - Section[Segment]).GetSafeNormal();
		return FVector2D(Run.Y, -Run.X);
	}
}
