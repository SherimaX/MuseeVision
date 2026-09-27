#include "Drawing/LaidRules.h"

#include "Drawing/Draw.h"
#include "Drawing/DrawnProfile.h"
#include "Kernel/Regions.h"
#include "Stations/Stations.h"

namespace Musee::Form::Drawing::Laying
{
	namespace
	{
		/** The most cells a laid surface may set out: a floor of a hundred thousand units is a drawing's mistake, not a floor. */
		constexpr int32 MostCells = 200000;

		/** The module's outlines laid at every lattice point whose copy reaches the bounds. */
		bool ModuleCells(const FFormLaid& Shape, const FBox2D& Bounds, TArray<FCell>& OutCells, FString& OutWhy)
		{
			const double Determinant = FVector2D::CrossProduct(Shape.LatticeU, Shape.LatticeV);
			if (FMath::Abs(Determinant) <= SliverSquareMeters)
			{
				OutWhy = TEXT("a laid module's lattice has two directions that aren't parallel");
				return false;
			}
			TArray<TArray<FVector2D>> Units;
			FBox2D Module(ForceInit);
			for (const FFormProfile& Unit : Shape.Module)
			{
				if (!CheckProfile(Unit, TEXT("a module's unit"), true, OutWhy))
				{
					return false;
				}
				Units.Add(Resolve(Unit).Points());
				Module += BoundsOf(Units.Last());
			}
			// The lattice points whose module can reach the bounds: the corners of the bounds less the module, in lattice terms.
			const FVector2D Low = Bounds.Min - Module.Max;
			const FVector2D High = Bounds.Max - Module.Min;
			auto Lattice = [&](const FVector2D& P)
			{
				return FVector2D(FVector2D::CrossProduct(P, Shape.LatticeV), FVector2D::CrossProduct(Shape.LatticeU, P)) / Determinant;
			};
			const FVector2D Corners[4] = {Lattice(Low), Lattice(High), Lattice(FVector2D(Low.X, High.Y)), Lattice(FVector2D(High.X, Low.Y))};
			const FBox2D Range = BoundsOf(Corners);
			if ((Range.Max.X - Range.Min.X + 2.0) * (Range.Max.Y - Range.Min.Y + 2.0) * Units.Num() > MostCells)
			{
				OutWhy = TEXT("a laid module sets out more cells than any floor has units");
				return false;
			}
			for (int32 J = FMath::FloorToInt32(Range.Min.Y); J <= FMath::CeilToInt32(Range.Max.Y); ++J)
			{
				for (int32 I = FMath::FloorToInt32(Range.Min.X); I <= FMath::CeilToInt32(Range.Max.X); ++I)
				{
					const FVector2D At = Shape.LatticeU * I + Shape.LatticeV * J;
					for (const TArray<FVector2D>& Unit : Units)
					{
						FCell& Cell = OutCells.AddDefaulted_GetRef();
						for (const FVector2D& Point : Unit)
						{
							Cell.Outer.Add(Point + At);
						}
					}
				}
			}
			return true;
		}

		/** The joints along one course that reach the span, from its start both ways, its lengths taken in turn. */
		TArray<double> CourseJoints(const FFormCourse& Course, double From, double To)
		{
			TArray<double> Joints = {Course.StartMeters};
			const int32 Count = Course.LengthsMeters.Num();
			for (int32 Index = 0; Joints.Last() < To; ++Index)
			{
				Joints.Add(Joints.Last() + Course.LengthsMeters[Index % Count]);
			}
			for (int32 Index = Count - 1; Joints[0] > From; Index = (Index + Count - 1) % Count)
			{
				Joints.Insert(Joints[0] - Course.LengthsMeters[Index], 0);
			}
			return Joints;
		}

		/** Courses up the rule's y from its origin (and down from it), taken in turn, cut at their joints. */
		bool CourseCells(const FFormLaid& Shape, const FBox2D& Bounds, TArray<FCell>& OutCells, FString& OutWhy)
		{
			for (const FFormCourse& Course : Shape.Courses)
			{
				if (Course.HeightMeters <= WeldToleranceMeters || Course.LengthsMeters.IsEmpty()
					|| Course.LengthsMeters.ContainsByPredicate([](double Length) { return Length <= WeldToleranceMeters; }))
				{
					OutWhy = TEXT("a course has a height and lengths");
					return false;
				}
			}
			const int32 Count = Shape.Courses.Num();
			auto Add = [&](const FFormCourse& Course, double Bed)
			{
				const TArray<double> Joints = CourseJoints(Course, Bounds.Min.X, Bounds.Max.X);
				for (int32 Index = 0; Index + 1 < Joints.Num(); ++Index)
				{
					OutCells.AddDefaulted_GetRef().Outer = {FVector2D(Joints[Index], Bed), FVector2D(Joints[Index + 1], Bed),
						FVector2D(Joints[Index + 1], Bed + Course.HeightMeters), FVector2D(Joints[Index], Bed + Course.HeightMeters)};
				}
				return OutCells.Num() <= MostCells;
			};
			bool bFits = true;
			double Bed = 0.0;
			for (int32 Index = 0; bFits && Bed < Bounds.Max.Y; ++Index)
			{
				const FFormCourse& Course = Shape.Courses[Index % Count];
				bFits = Bed + Course.HeightMeters <= Bounds.Min.Y || Add(Course, Bed);
				Bed += Course.HeightMeters;
			}
			Bed = 0.0;
			for (int32 Index = Count - 1; bFits && Bed > Bounds.Min.Y; Index = (Index + Count - 1) % Count)
			{
				const FFormCourse& Course = Shape.Courses[Index];
				Bed -= Course.HeightMeters;
				bFits = Bed >= Bounds.Max.Y || Add(Course, Bed);
			}
			if (!bFits)
			{
				OutWhy = TEXT("the courses set out more cells than any floor has units");
			}
			return bFits;
		}

		/** Rings of radial slabs about the rule's origin, their arcs divided at the chord tolerance. */
		bool RingCells(const FFormLaid& Shape, TArray<FCell>& OutCells, FString& OutWhy)
		{
			for (const FFormRing& Ring : Shape.Rings)
			{
				if (Ring.Units < 1 || Ring.InnerMeters < 0.0 || Ring.OuterMeters <= Ring.InnerMeters + WeldToleranceMeters)
				{
					OutWhy = TEXT("a ring has slabs between two circles");
					return false;
				}
				auto Arc = [](double Radius, double From, double To)
				{
					TArray<FVector2D> Points;
					for (const double Angle : ArcStations(Radius, From, To, DefaultChordToleranceMeters))
					{
						Points.Add(FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
					}
					return Points;
				};
				const double Step = 2.0 * UE_DOUBLE_PI / Ring.Units;
				const double Start = FMath::DegreesToRadians(Ring.StartDegrees);
				for (int32 Unit = 0; Unit < Ring.Units; ++Unit)
				{
					FCell& Cell = OutCells.AddDefaulted_GetRef();
					const double From = Start + Step * Unit;
					if (Ring.Units == 1)
					{
						Cell.Outer = Arc(Ring.OuterMeters, From, From + Step);
						Cell.Outer.Pop();
						if (Ring.InnerMeters > WeldToleranceMeters)
						{
							Cell.Holes.Add(Arc(Ring.InnerMeters, From, From + Step));
							Cell.Holes.Last().Pop();
						}
						continue;
					}
					Cell.Outer = Arc(Ring.OuterMeters, From, From + Step);
					if (Ring.InnerMeters > WeldToleranceMeters)
					{
						Cell.Outer.Append(Arc(Ring.InnerMeters, From + Step, From));
					}
					else
					{
						Cell.Outer.Add(FVector2D::ZeroVector);
					}
				}
			}
			return true;
		}
	}

	FBox2D BoundsOf(TConstArrayView<FVector2D> Points)
	{
		FBox2D Box(ForceInit);
		for (const FVector2D& Point : Points)
		{
			Box += Point;
		}
		return Box;
	}

	bool SetOut(const FFormLaid& Shape, const FBox2D& Bounds, TArray<FCell>& OutCells, FString& OutWhy)
	{
		return !Shape.Module.IsEmpty() ? ModuleCells(Shape, Bounds, OutCells, OutWhy)
			: (!Shape.Courses.IsEmpty() ? CourseCells(Shape, Bounds, OutCells, OutWhy) : RingCells(Shape, OutCells, OutWhy));
	}
}
