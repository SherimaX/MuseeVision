#include "Drawing/Draw.h"

#include "Drawing/DrawnProfile.h"
#include "Kernel/Extrude.h"
#include "Kernel/MeshBuilder.h"
#include "Kernel/PlanarFace.h"
#include "Kernel/Prism.h"
#include "Kernel/Regions.h"
#include "Stations/Stations.h"

/**
 * Lay: a surface set out in units as the mason and the joiner set it out. The rule gives every unit's cell, joint centre
 * to joint centre, in the setting-out frame; each cell is cut to what is laid and loses half a joint to each neighbour,
 * butting the outline and the holes, and what remains is one unit, a solid of its own. Each unit is drawn in its own frame, its grain along its longest spread (a
 * board's length, a flag's long side), and takes its own window on the stock's figure, since no two units are cut from
 * the same place: the unit's texture origin steps through the stock by the R2 sequence, so no two neighbours repeat.
 */
namespace Musee::Form::Drawing
{
	namespace
	{
		using UE::Geometry::FGeneralPolygon2d;

		/**
		 * The R2 low-discrepancy sequence (M. Roberts, "The unreasonable effectiveness of quasirandom sequences", 2018):
		 * steps of 1/g and 1/g^2, g the plastic number, fill a square more evenly than any random choice.
		 */
		constexpr double PlasticNumber = 1.32471795724474602596;

		/** How far apart two units' windows on the stock's figure may lie, metres: a metre of a block's face. */
		constexpr double FigureStepMeters = 1.0;

		/**
		 * A unit whose spread differs one way from the other by less than a part in a million is as long as it is wide:
		 * rounding in the moments, not a grain.
		 */
		constexpr double EvenSpread = 1.0e-6;

		/** A unit carries its arris when it keeps its shape shrunk by twice the arris: its eased faces stay clear of each other. */
		constexpr double ArrisClearance = 2.0;

		/** The most cells a laid surface may set out: a floor of a hundred thousand units is a drawing's mistake, not a floor. */
		constexpr int32 MostCells = 200000;

		/** The setting-out frame: where the rule's (x, y) lie in the drawing's. */
		struct FSetting
		{
			FVector2D Origin;
			double Cos = 1.0;
			double Sin = 0.0;

			FSetting(const FFormLaid& Shape)
				: Origin(Shape.OriginMeters)
				, Cos(FMath::Cos(FMath::DegreesToRadians(Shape.AngleDegrees)))
				, Sin(FMath::Sin(FMath::DegreesToRadians(Shape.AngleDegrees)))
			{
			}

			FVector2D ToDrawing(const FVector2D& P) const { return Origin + FVector2D(Cos * P.X - Sin * P.Y, Sin * P.X + Cos * P.Y); }
			FVector2D ToRule(const FVector2D& Q) const
			{
				const FVector2D D = Q - Origin;
				return FVector2D(Cos * D.X + Sin * D.Y, -Sin * D.X + Cos * D.Y);
			}
		};

		/** A cell of the rule, in the rule's frame: one loop, or a loop with a hole (a whole ring). */
		struct FCell
		{
			TArray<FVector2D> Outer;
			TArray<TArray<FVector2D>> Holes;
		};

		FBox2D BoundsOf(TConstArrayView<FVector2D> Points)
		{
			FBox2D Box(ForceInit);
			for (const FVector2D& Point : Points)
			{
				Box += Point;
			}
			return Box;
		}

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

		/**
		 * The spread of a region's area about its centroid: its centroid, and the angle of its longest spread, radians.
		 * False for a unit as long one way as the other (a square flag), which has no longest spread.
		 */
		bool Spread(const FRegion& Region, FVector2D& OutCentre, double& OutGrain)
		{
			double Area = 0.0;
			FVector2D First = FVector2D::ZeroVector;
			double XX = 0.0, YY = 0.0, XY = 0.0;
			for (const TArray<FVector2D>& Loop : AllLoops(Region))
			{
				for (int32 Index = 0; Index < Loop.Num(); ++Index)
				{
					const FVector2D& P = Loop[Index];
					const FVector2D& Q = Loop[(Index + 1) % Loop.Num()];
					const double A = FVector2D::CrossProduct(P, Q);
					Area += 0.5 * A;
					First += (P + Q) * (A / 6.0);
					XX += A * (P.X * P.X + P.X * Q.X + Q.X * Q.X) / 12.0;
					YY += A * (P.Y * P.Y + P.Y * Q.Y + Q.Y * Q.Y) / 12.0;
					XY += A * (P.X * Q.Y + 2.0 * P.X * P.Y + 2.0 * Q.X * Q.Y + Q.X * P.Y) / 24.0;
				}
			}
			OutCentre = Area != 0.0 ? First / Area : FVector2D::ZeroVector;
			const double Sxx = XX - Area * OutCentre.X * OutCentre.X;
			const double Syy = YY - Area * OutCentre.Y * OutCentre.Y;
			const double Sxy = XY - Area * OutCentre.X * OutCentre.Y;
			OutGrain = 0.5 * FMath::Atan2(2.0 * Sxy, Sxx - Syy);
			return FMath::Abs(Sxx - Syy) + FMath::Abs(2.0 * Sxy) > EvenSpread * (FMath::Abs(Sxx) + FMath::Abs(Syy));
		}
	}

	namespace
	{
		/** A cut polygon's loops, cleaned (CleanLoop); none when its outline is left with no area, and no hole that is. */
		TArray<TArray<FVector2D>> CleanLoops(const FGeneralPolygon2d& Polygon)
		{
			TArray<TArray<FVector2D>> Loops;
			for (const TArray<FVector2D>& Loop : LoopsOf(Polygon))
			{
				TArray<FVector2D> Clean = CleanLoop(Loop);
				if (Clean.Num() >= 3)
				{
					Loops.Add(MoveTemp(Clean));
				}
				else if (Loops.IsEmpty())
				{
					return {};
				}
			}
			return Loops;
		}

		/**
		 * Whether a unit can carry its arris: shrunk by a little more than the arris it stays one piece of the same shape, so
		 * the eased faces don't meet across it.
		 */
		bool CarriesArris(const FGeneralPolygon2d& Cut, TConstArrayView<TArray<FVector2D>> Loops, const FFormArris& Arris)
		{
			if (Arris.IsSharp())
			{
				return false;
			}
			const FRegion Inset = Offset({Cut}, -ArrisClearance * Arris.Meters);
			if (Inset.Num() != 1 || Inset[0].GetHoles().Num() != Cut.GetHoles().Num())
			{
				return false;
			}
			// The eased face's edge is each loop moved in by the arris, mitred at every point as the sweep mitres it: no
			// edge of it may turn round, as it does past a corner too sharp for the arris.
			for (const TArray<FVector2D>& Loop : Loops)
			{
				const int32 Count = Loop.Num();
				auto Normal = [&Loop, Count](int32 Edge)
				{
					const FVector2D Step = Loop[(Edge + 1) % Count] - Loop[Edge];
					return FVector2D(Step.Y, -Step.X).GetSafeNormal();
				};
				auto Moved = [&](int32 Index)
				{
					const FVector2D In = Normal((Index + Count - 1) % Count);
					const FVector2D Out = Normal(Index);
					return Loop[Index] - (In + Out) * (Arris.Meters / (1.0 + FVector2D::DotProduct(In, Out)));
				};
				for (int32 Index = 0; Index < Count; ++Index)
				{
					const FVector2D Step = Loop[(Index + 1) % Count] - Loop[Index];
					if (FVector2D::DotProduct(Moved((Index + 1) % Count) - Moved(Index), Step) <= 0.0)
					{
						return false;
					}
				}
			}
			return true;
		}

		/** One unit, drawn in its own frame about its centre with its grain along x and its window on the figure, into Out. */
		bool Unit(const FFormLaid& Shape, const FSetting& Setting, const FGeneralPolygon2d& Cut, int32 Index, FMeshBuilder& Out,
			FLaidTally& Tally, FString& OutWhy)
		{
			FVector2D Centre;
			double Grain = 0.0;
			const bool bLong = Spread({Cut}, Centre, Grain);
			const FVector2D Axis = bLong ? FVector2D(FMath::Cos(Grain), FMath::Sin(Grain)) : FVector2D(Setting.Cos, Setting.Sin);
			const FVector2D Figure = FigureStepMeters * FVector2D(FMath::Frac(0.5 + Index / PlasticNumber), FMath::Frac(0.5 + Index / (PlasticNumber * PlasticNumber)));
			TArray<FProfileStations> Loops;
			FBox2D Extent(ForceInit);
			const TArray<TArray<FVector2D>> Cleaned = CleanLoops(Cut);
			for (const TArray<FVector2D>& Loop : Cleaned)
			{
				TArray<FVector2D> Local;
				for (const FVector2D& P : Loop)
				{
					const FVector2D D = P - Centre;
					Local.Add(FVector2D(FVector2D::DotProduct(D, Axis), FVector2D::CrossProduct(Axis, D)) + Figure);
					Extent += Local.Last();
				}
				Loops.Add(LoopStations(Local));
			}
			if (Loops.IsEmpty())
			{
				// A sliver where the outline grazes a cell is no unit: the joint takes it.
				return true;
			}
			FMeshBuilder Drawn(Out.NumUVChannels());
			const FName Part(FFormLaid::UnitPart);
			// A unit too small or too sharp to carry its arris (a sliver cut at the outline, whose arrises would meet) is left
			// sharp, as the fixer leaves it: easing it would take it away.
			FString Unused;
			const bool bEased = CarriesArris(Cut, Cleaned, Shape.Arris)
				&& ArrisedPrism(Drawn, Loops, Shape.ThicknessMeters, FFormArris(), Shape.Arris, {}, {Part, Part, Part, Part}, Unused);
			if (!bEased)
			{
				Drawn = FMeshBuilder(Out.NumUVChannels());
				Drawn.UseSlot(Part);
				if (!Extrude(Drawn, FPlaneFrame(), Loops[0], TConstArrayView<FProfileStations>(Loops).RightChop(1), Shape.ThicknessMeters, true))
				{
					OutWhy = TEXT("a unit's outline crosses itself");
					return false;
				}
			}
			// Carried back to the drawing's frame: the grain turns the normals with the points.
			const FVector2D Size = Extent.GetSize();
			Tally.LargestMeters = FVector2D(FMath::Max(Tally.LargestMeters.X, Size.GetMax()), FMath::Max(Tally.LargestMeters.Y, Size.GetMin()));
			const FVector2D Stock(Shape.StockMeters.GetMax(), Shape.StockMeters.GetMin());
			Tally.BeyondStock += !Stock.IsZero() && (Size.GetMax() > Stock.X + WeldToleranceMeters || Size.GetMin() > Stock.Y + WeldToleranceMeters) ? 1 : 0;
			++Tally.Units;
			auto Turn = [&Axis](const FVector& V) { return FVector(Axis * V.X + FVector2D(-Axis.Y, Axis.X) * V.Y, V.Z); };
			TArray<int32> Carried;
			for (int32 Vertex = 0; Vertex < Drawn.NumVertices(); ++Vertex)
			{
				const FVector Local = Drawn.Position(Vertex) - FVector(Figure, 0.0);
				Carried.Add(Out.AddVertex(Turn(Local) + FVector(Centre, 0.0), Turn(Drawn.Normal(Vertex)), Drawn.UV(Vertex)));
			}
			Out.UseSlot(Part);
			for (int32 Triangle = 0; Triangle < Drawn.NumTriangles(); ++Triangle)
			{
				const FIntVector& T = Drawn.Triangle(Triangle);
				Out.AddTriangle(Carried[T.X], Carried[T.Y], Carried[T.Z]);
			}
			return true;
		}

		/**
		 * The units the cells leave, each cut to what is laid: a unit gives up half a joint to each neighbour (its cut cell
		 * less its neighbours' grown by half a joint) and butts whatever it is laid against, a wall, a plinth, a stone's ring.
		 */
		TArray<FRegion> CutUnits(const FFormLaid& Shape, const FSetting& Setting, TConstArrayView<FCell> Cells, const FBox2D& Bounds, const FRegion& Laid)
		{
			TArray<FRegion> Cut;
			TArray<FBox2D> Boxes;
			for (const FCell& Cell : Cells)
			{
				if (!BoundsOf(Cell.Outer).Intersect(Bounds))
				{
					continue;
				}
				auto Drawn = [&Setting](TConstArrayView<FVector2D> Loop)
				{
					TArray<FVector2D> Out;
					for (const FVector2D& P : Loop)
					{
						Out.Add(Setting.ToDrawing(P));
					}
					return Out;
				};
				TArray<TArray<FVector2D>> CellHoles;
				for (const TArray<FVector2D>& Hole : Cell.Holes)
				{
					CellHoles.Add(Drawn(Hole));
				}
				FRegion Within = Intersection(RegionOf(Drawn(Cell.Outer), CellHoles), Laid);
				if (!Within.IsEmpty())
				{
					FBox2D Box(ForceInit);
					for (const TArray<FVector2D>& Loop : AllLoops(Within))
					{
						Box += BoundsOf(Loop);
					}
					Boxes.Add(Box.ExpandBy(Shape.JointMeters));
					Cut.Add(MoveTemp(Within));
				}
			}
			// Neighbours are found through a grid of buckets as large as the largest unit.
			double Bucket = WeldToleranceMeters;
			for (const FBox2D& Box : Boxes)
			{
				Bucket = FMath::Max(Bucket, Box.GetSize().GetMax());
			}
			TMultiMap<FIntPoint, int32> Grid;
			auto Buckets = [Bucket](const FBox2D& Box, TFunctionRef<void(const FIntPoint&)> Visit)
			{
				for (int32 X = FMath::FloorToInt32(Box.Min.X / Bucket); X <= FMath::FloorToInt32(Box.Max.X / Bucket); ++X)
				{
					for (int32 Y = FMath::FloorToInt32(Box.Min.Y / Bucket); Y <= FMath::FloorToInt32(Box.Max.Y / Bucket); ++Y)
					{
						Visit(FIntPoint(X, Y));
					}
				}
			};
			for (int32 Index = 0; Index < Boxes.Num(); ++Index)
			{
				Buckets(Boxes[Index], [&Grid, Index](const FIntPoint& Key) { Grid.AddUnique(Key, Index); });
			}
			TArray<FRegion> Grown;
			for (const FRegion& Region : Cut)
			{
				Grown.Add(Offset(Region, 0.5 * Shape.JointMeters));
			}
			TArray<FRegion> Units;
			for (int32 Index = 0; Index < Cut.Num(); ++Index)
			{
				TSet<int32> Near;
				Buckets(Boxes[Index], [&Grid, &Near](const FIntPoint& Key)
				{
					TArray<int32> Found;
					Grid.MultiFind(Key, Found);
					Near.Append(Found);
				});
				FRegion Taken;
				for (const int32 Other : Near)
				{
					if (Other != Index && Boxes[Other].Intersect(Boxes[Index]))
					{
						Taken.Append(Grown[Other]);
					}
				}
				Units.Add(Difference(Cut[Index], Taken));
			}
			return Units;
		}

		/** The filling of the joints: what is laid less every unit, up to the fill's level. */
		bool Filling(const FFormLaid& Shape, const FRegion& Laid, const FRegion& Units, FMeshBuilder& Joints, FString& OutWhy)
		{
			Joints.UseSlot(FFormLaid::JointPart);
			const FPlaneFrame Plane;
			for (const FGeneralPolygon2d& Polygon : Difference(Laid, Units))
			{
				TArray<FProfileStations> Loops;
				for (const TArray<FVector2D>& Loop : CleanLoops(Polygon))
				{
					Loops.Add(LoopStations(Loop));
				}
				if (!Loops.IsEmpty() && !Extrude(Joints, Plane, Loops[0], TConstArrayView<FProfileStations>(Loops).RightChop(1), Shape.ThicknessMeters - Shape.FillMeters, true))
				{
					OutWhy = TEXT("the joints' filling crosses itself");
					return false;
				}
			}
			return true;
		}
	}

	bool DrawLaid(const FFormLaid& Shape, FMeshBuilder& Units, FMeshBuilder& Joints, FLaidTally& OutTally, FString& OutWhy)
	{
		TArray<FProfileStations> Holes;
		if (!CheckProfile(Shape.Outline, TEXT("the laid outline"), true, OutWhy))
		{
			return false;
		}
		const int32 Rules = (Shape.Module.IsEmpty() ? 0 : 1) + (Shape.Courses.IsEmpty() ? 0 : 1) + (Shape.Rings.IsEmpty() ? 0 : 1);
		if (Rules != 1 || Shape.ThicknessMeters <= 0.0 || Shape.JointMeters <= 2.0 * WeldToleranceMeters || Shape.FillMeters >= Shape.ThicknessMeters)
		{
			OutWhy = TEXT("a laid surface has one rule (a module, courses or rings), a thickness, joints of some width, and a fill below its face");
			return false;
		}
		TArray<TArray<FVector2D>> HolePoints;
		for (const FFormProfile& Hole : Shape.Holes)
		{
			if (!CheckProfile(Hole, TEXT("a hole"), true, OutWhy))
			{
				return false;
			}
			HolePoints.Add(Resolve(Hole).Points());
		}
		const FRegion Laid = RegionOf(Resolve(Shape.Outline).Points(), HolePoints);
		const FSetting Setting(Shape);
		TArray<FVector2D> Seen;
		for (const TArray<FVector2D>& Loop : AllLoops(Laid))
		{
			for (const FVector2D& Point : Loop)
			{
				Seen.Add(Setting.ToRule(Point));
			}
		}
		const FBox2D Bounds = BoundsOf(Seen);
		TArray<FCell> Cells;
		const bool bSetOut = !Shape.Module.IsEmpty() ? ModuleCells(Shape, Bounds, Cells, OutWhy)
			: (!Shape.Courses.IsEmpty() ? CourseCells(Shape, Bounds, Cells, OutWhy) : RingCells(Shape, Cells, OutWhy));
		if (!bSetOut)
		{
			return false;
		}
		FRegion AllUnits;
		for (const FRegion& Cut : CutUnits(Shape, Setting, Cells, Bounds, Laid))
		{
			for (const FGeneralPolygon2d& Polygon : Cut)
			{
				if (!Unit(Shape, Setting, Polygon, OutTally.Units, Units, OutTally, OutWhy))
				{
					return false;
				}
				AllUnits.Add(Polygon);
			}
		}
		if (OutTally.Units == 0)
		{
			OutWhy = TEXT("the rule lays no unit on the outline");
			return false;
		}
		return Shape.FillMeters < 0.0 || Filling(Shape, Laid, AllUnits, Joints, OutWhy);
	}
}
