#include "Drawing/Draw.h"

#include "Drawing/DrawnProfile.h"
#include "Drawing/LaidRules.h"
#include "Kernel/Extrude.h"
#include "Kernel/MeshBuilder.h"
#include "Kernel/PlanarFace.h"
#include "Kernel/Prism.h"
#include "Kernel/Regions.h"
#include "Stations/Stations.h"

/**
 * Lay: a surface set out in units as the mason and the joiner set it out. The rule gives every unit's cell, joint centre
 * to joint centre, in the setting-out frame (LaidRules.h); each cell is cut to what is laid and loses half a joint to
 * each neighbour, butting the outline and the holes, and what remains is one unit, a solid of its own. Each unit is drawn
 * in its own frame, its grain along its longest spread (a board's length, a flag's long side), and takes its own window
 * on the stock's figure, since no two units are cut from the same place: the unit's texture origin steps through the
 * stock by the R2 sequence, so no two neighbours repeat.
 */
namespace Musee::Form::Drawing
{
	namespace
	{
		using UE::Geometry::FGeneralPolygon2d;
		using Laying::BoundsOf;
		using Laying::FCell;
		using Laying::FSetting;

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

		/** How far the joints' filling stands off the units: two weld tolerances, so the two never weld. */
		constexpr double FillClearance = 2.0 * WeldToleranceMeters;

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

		/** Each cell cut to the laid region, with its bounds grown by a joint: the cells that lay anything. */
		void CutCells(const FFormLaid& Shape, const FSetting& Setting, TConstArrayView<FCell> Cells, const FBox2D& Bounds, const FRegion& Laid,
			TArray<FRegion>& OutCut, TArray<FBox2D>& OutBoxes)
		{
			auto Drawn = [&Setting](TConstArrayView<FVector2D> Loop)
			{
				TArray<FVector2D> Out;
				for (const FVector2D& P : Loop)
				{
					Out.Add(Setting.ToDrawing(P));
				}
				return Out;
			};
			for (const FCell& Cell : Cells)
			{
				if (!BoundsOf(Cell.Outer).Intersect(Bounds))
				{
					continue;
				}
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
					OutBoxes.Add(Box.ExpandBy(Shape.JointMeters));
					OutCut.Add(MoveTemp(Within));
				}
			}
		}

		/** For each box, the others it meets, found through a grid of buckets as large as the largest box. */
		TArray<TArray<int32>> Neighbours(TConstArrayView<FBox2D> Boxes)
		{
			double Bucket = WeldToleranceMeters;
			for (const FBox2D& Box : Boxes)
			{
				Bucket = FMath::Max(Bucket, Box.GetSize().GetMax());
			}
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
			TMultiMap<FIntPoint, int32> Grid;
			for (int32 Index = 0; Index < Boxes.Num(); ++Index)
			{
				Buckets(Boxes[Index], [&Grid, Index](const FIntPoint& Key) { Grid.AddUnique(Key, Index); });
			}
			TArray<TArray<int32>> Out;
			for (int32 Index = 0; Index < Boxes.Num(); ++Index)
			{
				TSet<int32> Near;
				Buckets(Boxes[Index], [&Grid, &Near](const FIntPoint& Key)
				{
					TArray<int32> Found;
					Grid.MultiFind(Key, Found);
					Near.Append(Found);
				});
				TArray<int32>& Met = Out.AddDefaulted_GetRef();
				for (const int32 Other : Near)
				{
					if (Other != Index && Boxes[Other].Intersect(Boxes[Index]))
					{
						Met.Add(Other);
					}
				}
				Met.Sort();
			}
			return Out;
		}

		/**
		 * The units the cells leave, each cut to what is laid: a unit gives up half a joint to each neighbour (its cut cell
		 * less its neighbours' grown by half a joint) and butts whatever it is laid against, a wall, a plinth, a stone's ring.
		 */
		TArray<FRegion> CutUnits(const FFormLaid& Shape, const FSetting& Setting, TConstArrayView<FCell> Cells, const FBox2D& Bounds, const FRegion& Laid)
		{
			TArray<FRegion> Cut;
			TArray<FBox2D> Boxes;
			CutCells(Shape, Setting, Cells, Bounds, Laid, Cut, Boxes);
			TArray<FRegion> Grown;
			for (const FRegion& Region : Cut)
			{
				Grown.Add(Offset(Region, 0.5 * Shape.JointMeters));
			}
			const TArray<TArray<int32>> Near = Neighbours(Boxes);
			TArray<FRegion> Units;
			for (int32 Index = 0; Index < Cut.Num(); ++Index)
			{
				FRegion Taken;
				for (const int32 Other : Near[Index])
				{
					Taken.Append(Grown[Other]);
				}
				Units.Add(Difference(Cut[Index], Taken));
			}
			return Units;
		}

		/**
		 * The filling of the joints: what is laid less every unit, up to the fill's level. It stands a hair off the units
		 * (FillClearance), so where a joint narrows to a point between two units' corners the filling parts there.
		 */
		bool Filling(const FFormLaid& Shape, const FRegion& Laid, const FRegion& Units, FMeshBuilder& Joints, FString& OutWhy)
		{
			Joints.UseSlot(FFormLaid::JointPart);
			const FPlaneFrame Plane;
			for (const FGeneralPolygon2d& Polygon : Offset(Difference(Laid, Units), -FillClearance))
			{
				TArray<FProfileStations> Loops;
				for (const TArray<FVector2D>& Loop : LoopsOf(Polygon))
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
		if (!Laying::SetOut(Shape, Bounds, Cells, OutWhy))
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
