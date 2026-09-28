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
		// Each measure of the section as turned, against the one its museum publishes.
		struct FMeasure
		{
			const TCHAR* What;
			double Section;
			double Published;
		};
		TArray<FMeasure> Measures;
		if (Vessel.Museum.IsOval())
		{
			FVesselPress Press;
			const FString Why = PressOf(Vessel, FVector2D(1.0, 1.0), Press);
			if (!Why.IsEmpty())
			{
				return Why;
			}
			const TArray<double>& Mouth = Vessel.Museum.MouthAxesMeters;
			Measures.Add({ TEXT("its mouth across"), 2.0 * Most.X, Mouth[0] });
			Measures.Add({ TEXT("its mouth front to back"), 2.0 * Most.X * Press.RatioAt(Press.MouthHeightMeters), Mouth[1] });
			if (Vessel.Museum.FootAxesMeters.Num() == 2)
			{
				const double Foot = RadiusAt(Vessel.Section, Press.FootHeightMeters);
				Measures.Add({ TEXT("its foot across"), 2.0 * Foot, Vessel.Museum.FootAxesMeters[0] });
				Measures.Add({ TEXT("its foot front to back"), 2.0 * Foot * Press.RatioAt(Press.FootHeightMeters), Vessel.Museum.FootAxesMeters[1] });
			}
		}
		else
		{
			Measures.Add({ TEXT("its section across"), 2.0 * Most.X, Vessel.Museum.DiameterMeters });
		}
		for (const FMeasure& Measure : Measures)
		{
			const double Off = Measure.Published > 0.0 ? FMath::Abs(Measure.Section - Measure.Published) / Measure.Published : 0.0;
			if (Off > DiameterTolerance && Vessel.Departure.IsEmpty())
			{
				return FString::Printf(TEXT("%s is %.1f mm, %.1f%% off its museum's %.1f, and its record gives no departure saying why"), Measure.What,
					1000.0 * Measure.Section, 100.0 * Off, 1000.0 * Measure.Published);
			}
		}
		return FString();
	}

	double FVesselPress::RatioAt(double HeightMeters) const
	{
		if (MouthHeightMeters <= FootHeightMeters)
		{
			return MouthRatio;
		}
		const double Share = FMath::Clamp((HeightMeters - FootHeightMeters) / (MouthHeightMeters - FootHeightMeters), 0.0, 1.0);
		return FMath::Lerp(FootRatio, MouthRatio, Share);
	}

	double FVesselPress::SlopeAt(double HeightMeters) const
	{
		if (MouthHeightMeters <= FootHeightMeters || HeightMeters < FootHeightMeters || HeightMeters > MouthHeightMeters)
		{
			return 0.0;
		}
		return (MouthRatio - FootRatio) / (MouthHeightMeters - FootHeightMeters);
	}

	double FVesselPress::ExponentAt(double HeightMeters) const
	{
		if (MouthHeightMeters <= FootHeightMeters)
		{
			return MouthExponent;
		}
		const double Share = FMath::Clamp((HeightMeters - FootHeightMeters) / (MouthHeightMeters - FootHeightMeters), 0.0, 1.0);
		return FMath::Lerp(FootExponent, MouthExponent, Share);
	}

	FVector FVesselPress::Pressed(double Radius, double Height, double Angle) const
	{
		const double S = FMath::Sin(Angle);
		const double C = FMath::Cos(Angle);
		const double Exponent = ExponentAt(Height);
		// a circle's point carried along its ray to the superellipse of the same half-axes: (|s|^n + |c|^n)^(-1/n)
		const double Out = FMath::IsNearlyEqual(Exponent, 2.0, 1e-12) ? 1.0
			: FMath::Pow(FMath::Pow(FMath::Abs(S), Exponent) + FMath::Pow(FMath::Abs(C), Exponent), -1.0 / Exponent);
		return FVector(Radius * Out * S, RatioAt(Height) * Radius * Out * C, Height);
	}

	FString PressOf(const FArtVessel& Vessel, const FVector2D& Scale, FVesselPress& Out)
	{
		Out = FVesselPress();
		const FArtVesselMuseum& Museum = Vessel.Museum;
		if (!Museum.IsOval() || Vessel.Section.IsEmpty())
		{
			return FString();
		}
		if (!(Museum.MouthAxesMeters[0] > 0.0 && Museum.MouthAxesMeters[1] > 0.0))
		{
			return TEXT("an oval's mouth has two axes of more than zero");
		}
		// The mouth: the section's widest point (the highest, where several are as wide).
		int32 Widest = 0;
		for (int32 Index = 1; Index < Vessel.Section.Num(); ++Index)
		{
			const FVector2D& Point = Vessel.Section[Index];
			const FVector2D& Best = Vessel.Section[Widest];
			Widest = Point.X > Best.X || (Point.X == Best.X && Point.Y > Best.Y) ? Index : Widest;
		}
		Out.MouthHeightMeters = Vessel.Section[Widest].Y * Scale.Y;
		Out.MouthRatio = Museum.MouthAxesMeters[1] / Museum.MouthAxesMeters[0];
		// the plan's exponents as its photographs show them; an ellipse where they show none
		Out.MouthExponent = Vessel.Plan.MouthExponent > 0.0 ? Vessel.Plan.MouthExponent : 2.0;
		Out.FootExponent = Vessel.Plan.FootExponent > 0.0 ? Vessel.Plan.FootExponent : Out.MouthExponent;
		if (Out.MouthExponent < 1.0 || Out.FootExponent < 1.0)
		{
			return TEXT("an oval's plan has exponents of one or more (a superellipse's, bulging outward)");
		}
		if (Museum.FootAxesMeters.Num() != 2)
		{
			Out.FootHeightMeters = Out.MouthHeightMeters;
			Out.FootRatio = Out.MouthRatio;
			Out.FootExponent = Out.MouthExponent;
			return FString();
		}
		if (!(Museum.FootAxesMeters[0] > 0.0 && Museum.FootAxesMeters[1] > 0.0))
		{
			return TEXT("an oval's foot has two axes of more than zero");
		}
		// The foot: the highest point of the outside, below the mouth, at the museum's foot's long half-axis.
		const double Half = 0.5 * Museum.FootAxesMeters[0];
		int32 Foot = INDEX_NONE;
		if (Vessel.Outside.Num() == 2)
		{
			for (int32 Index = FMath::Max(Vessel.Outside[0], 0); Index <= FMath::Min(Vessel.Outside[1], Vessel.Section.Num() - 1); ++Index)
			{
				const FVector2D& Point = Vessel.Section[Index];
				if (FMath::Abs(Point.X - Half) <= FootToleranceMeters && Point.Y < Vessel.Section[Widest].Y
					&& (Foot == INDEX_NONE || Point.Y > Vessel.Section[Foot].Y))
				{
					Foot = Index;
				}
			}
		}
		if (Foot == INDEX_NONE)
		{
			return FString::Printf(TEXT("its outside comes nowhere to its museum's foot, %.1f mm across"), 1000.0 * Museum.FootAxesMeters[0]);
		}
		Out.FootHeightMeters = Vessel.Section[Foot].Y * Scale.Y;
		Out.FootRatio = Museum.FootAxesMeters[1] / Museum.FootAxesMeters[0];
		return FString();
	}

	FString FeetFault(const FArtVessel& Vessel, const FVesselPress& Press)
	{
		if (Vessel.Feet.Each.IsEmpty())
		{
			return FString();
		}
		const double Height = Vessel.Feet.HeightMeters;
		const double Radius = RadiusAt(Vessel.Section, Height);
		if (!(Radius > 0.0))
		{
			return FString::Printf(TEXT("its feet stand %.1f mm high, where its section has no underside"), 1000.0 * Height);
		}
		const double Across = Radius;
		const double Deep = Radius * Press.RatioAt(Height);
		const double Exponent = Press.ExponentAt(Height);
		double Furthest = 0.0;
		for (const FArtVesselFoot& Foot : Vessel.Feet.Each)
		{
			for (const FVector2D& Point : Foot.Outline)
			{
				Furthest = FMath::Max(Furthest, FMath::Pow(FMath::Pow(FMath::Abs(Point.X) / Across, Exponent) + FMath::Pow(FMath::Abs(Point.Y) / Deep, Exponent),
					1.0 / Exponent));
			}
		}
		if (Furthest > 1.0 + FeetToleranceShare)
		{
			return FString::Printf(TEXT("its feet reach %.3f of its underside's plan at their height (%.1f x %.1f mm), up to %.1f mm past its edge, into the air"),
				Furthest, 2000.0 * Across, 2000.0 * Deep, 1000.0 * (Furthest - 1.0) * Radius);
		}
		return FString();
	}

	double RadiusAt(TConstArrayView<FVector2D> Section, double HeightMeters)
	{
		double Most = 0.0;
		for (int32 Index = 1; Index < Section.Num(); ++Index)
		{
			const FVector2D& A = Section[Index - 1];
			const FVector2D& B = Section[Index];
			if (FMath::Min(A.Y, B.Y) <= HeightMeters && HeightMeters <= FMath::Max(A.Y, B.Y))
			{
				Most = FMath::Max(Most, A.Y == B.Y ? FMath::Max(A.X, B.X) : FMath::Lerp(A.X, B.X, (HeightMeters - A.Y) / (B.Y - A.Y)));
			}
		}
		return Most;
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
