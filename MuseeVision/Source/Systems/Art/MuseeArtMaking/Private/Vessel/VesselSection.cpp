#include "Vessel/VesselSection.h"

#include "Algo/BinarySearch.h"

namespace Musee::ArtMaking
{
	namespace
	{
		/** The section's widest radius and its top, [radius, height]. */
		FVector2D Extent(const FArtVessel& Vessel)
		{
			FVector2D Most = FVector2D::ZeroVector;
			for (const FVector2D& Point : Vessel.Section)
			{
				Most = FVector2D(FMath::Max(Most.X, Point.X), FMath::Max(Most.Y, Point.Y));
			}
			return Most;
		}
	}

	FVector2D SectionScale(const FArtVessel& Vessel, const FVector2D& SizeMeters)
	{
		const FVector2D Most = Extent(Vessel);
		const double Up = SizeMeters.Y / Most.Y;
		return Vessel.Grade == TEXT("derived") ? FVector2D(Up, Up) : FVector2D(0.5 * SizeMeters.X / Most.X, Up);
	}

	FString SizeFault(const FArtVessel& Vessel, const FVector2D& PlanMeters, double DiameterTolerance)
	{
		if (Vessel.Grade != TEXT("derived"))
		{
			return FString();
		}
		const FVector2D Most = Extent(Vessel);
		const double Height = Vessel.Museum.HeightMeters;
		if (!FMath::IsNearlyEqual(Most.Y, Height, SectionHeightToleranceMeters))
		{
			return FString::Printf(TEXT("its section stands %.1f mm high, not its museum's %.1f"), 1000.0 * Most.Y, 1000.0 * Height);
		}
		if (!FMath::IsNearlyEqual(PlanMeters.Y, Height, SectionHeightToleranceMeters))
		{
			return FString::Printf(TEXT("its plan's height %.1f mm is not its museum's %.1f"), 1000.0 * PlanMeters.Y, 1000.0 * Height);
		}
		if (!FMath::IsNearlyEqual(PlanMeters.X, 2.0 * Most.X, PlanWidthToleranceMeters))
		{
			return FString::Printf(TEXT("its plan's width %.1f mm is not its section's %.1f (a derived section keeps its photographs' proportion)"),
				1000.0 * PlanMeters.X, 2000.0 * Most.X);
		}
		const double Published = Vessel.Museum.IsOval() ? Vessel.Museum.MouthAxesMeters[0] : Vessel.Museum.DiameterMeters;
		if (Published > 0.0 && FMath::Abs(2.0 * Most.X - Published) > DiameterTolerance * Published && Vessel.Departure.IsEmpty())
		{
			return FString::Printf(TEXT("its section is %.1f mm across, %.1f%% off its museum's %.1f, and its record gives no departure saying why"),
				2000.0 * Most.X, 100.0 * FMath::Abs(2.0 * Most.X - Published) / Published, 1000.0 * Published);
		}
		return FString();
	}

	TArray<FVector2D> SizedSection(const FArtVessel& Vessel, const FVector2D& SizeMeters)
	{
		const FVector2D Scale = SectionScale(Vessel, SizeMeters);
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

	namespace
	{
		/** Whether segments PQ and AB cross strictly inside both (sharing an end is not crossing). */
		bool Cross(const FVector2D& P, const FVector2D& Q, const FVector2D& A, const FVector2D& B)
		{
			const double Denominator = FVector2D::CrossProduct(Q - P, B - A);
			if (FMath::Abs(Denominator) < UE_DOUBLE_SMALL_NUMBER * UE_DOUBLE_SMALL_NUMBER)
			{
				return false;
			}
			const double S = FVector2D::CrossProduct(A - P, B - A) / Denominator;
			const double U = FVector2D::CrossProduct(A - P, Q - P) / Denominator;
			constexpr double Inside = 1e-9;
			return S > Inside && S < 1.0 - Inside && U > Inside && U < 1.0 - Inside;
		}

		/** The distance from a point to the segment AB. */
		double DistanceToSegment(const FVector2D& Point, const FVector2D& A, const FVector2D& B)
		{
			const FVector2D Run = B - A;
			const double Along = Run.SizeSquared() > 0.0 ? FMath::Clamp(FVector2D::DotProduct(Point - A, Run) / Run.SizeSquared(), 0.0, 1.0) : 0.0;
			return FVector2D::Distance(Point, A + Along * Run);
		}
	}

	FString SectionFault(TConstArrayView<FVector2D> Section, TConstArrayView<int32> Outside, double WallMeters)
	{
		const int32 Count = Section.Num();
		for (int32 I = 0; I + 1 < Count; ++I)
		{
			for (int32 J = I + 2; J + 1 < Count; ++J)
			{
				if (Cross(Section[I], Section[I + 1], Section[J], Section[J + 1]))
				{
					return FString::Printf(TEXT("its segments %d and %d cross"), I, J);
				}
			}
		}
		if (Outside.Num() != 2 || !(WallMeters > 0.0))
		{
			return FString();
		}
		// The outside runs to the lip's top (Outside[1]); the inside from the point after it down to the axis, whose last
		// point (on the axis, over the base or on the neck's floor) is left out.
		for (int32 K = Outside[1] + 1; K + 1 < Count; ++K)
		{
			double Nearest = TNumericLimits<double>::Max();
			for (int32 I = 0; I < Outside[1]; ++I)
			{
				Nearest = FMath::Min(Nearest, DistanceToSegment(Section[K], Section[I], Section[I + 1]));
			}
			if (Nearest < 0.5 * WallMeters)
			{
				return FString::Printf(TEXT("its wall at inside point %d is %.2f mm, under half the %.2f mm it was drawn with"), K, 1000.0 * Nearest,
					1000.0 * WallMeters);
			}
		}
		return FString();
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
