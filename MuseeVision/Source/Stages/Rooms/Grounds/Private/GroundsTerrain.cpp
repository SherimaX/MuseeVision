#include "FormExtrusion.h"
#include "FormSweep.h"
#include "GroundsParts.h"

/**
 * The terrain (§3.2): a flat solid of topsoil over the whole footprint, z -0.330 to -0.030 (0.30 of it, as the Reserve's
 * sheet expects under it), made as one Nanite mesh and no Landscape; its top takes Nature's lawn. The sheet cuts it by
 * rule: the footprint less every received building's volume and less every piece of the Grounds set into it. The soil
 * gives way to every building the site plan stands on the grounds whose volume passes through it (a prism reaching
 * between its bottom and its top: the Rotunda's room and its well, a hall's gardens, a pond), so it never fills their
 * floors or their wells; to the canal and the round basin down to their bluestone floors; to the parvis's paving as one
 * rectangle, its joints with it; and to every laid piece (the walks' gravel, the strip) and swept one (the kerbs, the
 * strip's edge, the copings) by what it covers in plan, so each sits in its own pocket and the lawn laid on the soil's top
 * stops at its faces. Form joins the holes where they meet.
 */
namespace Musee::Grounds
{
	namespace
	{
		/** Form divides every curve so no chord strays more than a millimetre from it (Docs/Architecture.md, Form). */
		constexpr double ChordToleranceMeters = 0.001;

		/** The point of an arc at an angle, degrees. */
		FVector2D ArcPoint(const FFormArc& Arc, double Degrees)
		{
			const double T = FMath::DegreesToRadians(Degrees);
			return FVector2D(Arc.CenterMeters[0] + Arc.RadiiMeters[0] * FMath::Cos(T), Arc.CenterMeters[1] + Arc.RadiiMeters.Last() * FMath::Sin(T));
		}

		/** A point of a piece's own plane in the stage's plan. */
		FVector2D InStage(const FMuseeFrame& Frame, const FVector2D& Point)
		{
			const FVector Stage = Frame.PointToParent(FVector(Point.X, Point.Y, 0.0));
			return FVector2D(Stage.X, Stage.Y);
		}

		/** The walker's right in plan (x east, y south, seen from above with north up), for a direction of travel. */
		FVector2D RightOf(const FVector2D& Direction)
		{
			return FVector2D(-Direction.Y, Direction.X);
		}

		/**
		 * How far along the path from its end a band's edge at a reach across it meets the end's cut plane (its normal in
		 * plan; zero for a square end).
		 */
		double AlongCut(const FVector2D& Direction, const FVector& Cut, double Across)
		{
			const FVector2D Normal(Cut.X, Cut.Y);
			const double Facing = Direction | Normal;
			if (Normal.IsNearlyZero() || FMath::IsNearlyZero(Facing))
			{
				return 0.0;
			}
			return -Across * (RightOf(Direction) | Normal) / Facing;
		}

		/**
		 * A swept piece's band in plan: along its path's chords, between its section's least and greatest reach across it.
		 * Where two chords meet, both quadrilaterals end on the corner's bisector, so they meet edge to edge; an open path's
		 * ends are square, or cut on the sweep's end planes; a closed path's last chord meets its first as any two do.
		 */
		TArray<TArray<FVector2D>> BandOf(const FFormSweep& Sweep, const FMuseeFrame& Frame)
		{
			TArray<TArray<FVector2D>> Out;
			TArray<FVector2D> Path;
			for (const FVector2D& Point : PlanPoints(Sweep.Path))
			{
				if (Path.IsEmpty() || !Path.Last().Equals(Point, UE_KINDA_SMALL_NUMBER))
				{
					Path.Add(Point);
				}
			}
			const bool bClosed = Sweep.Path.Closed && Path.Num() > 2;
			if (bClosed && Path.Last().Equals(Path[0], UE_KINDA_SMALL_NUMBER))
			{
				Path.Pop();
			}
			const TArray<FVector2D> Section = PlanPoints(Sweep.Section);
			if (Path.Num() < 2 || Section.IsEmpty())
			{
				return Out;
			}
			double Least = TNumericLimits<double>::Max();
			double Most = -TNumericLimits<double>::Max();
			for (const FVector2D& Point : Section)
			{
				Least = FMath::Min(Least, Point.X);
				Most = FMath::Max(Most, Point.X);
			}
			const int32 Count = Path.Num();
			const int32 Chords = bClosed ? Count : Count - 1;
			const auto Direction = [&Path, Count](int32 Chord) { return (Path[(Chord + 1) % Count] - Path[Chord % Count]).GetSafeNormal(); };
			// The band's point at a vertex of a chord: on the corner's bisector where two chords meet, on the end's cut (or
			// square) at an open path's ends.
			const auto At = [&](int32 Vertex, int32 Chord, double Across) -> FVector2D
			{
				const FVector2D Along = Direction(Chord);
				const bool bStart = Vertex == Chord;
				if (!bClosed && (bStart ? Vertex == 0 : Vertex == Count - 1))
				{
					const FVector& Cut = bStart ? Sweep.StartCut : Sweep.EndCut;
					return Path[Vertex] + Across * RightOf(Along) + AlongCut(Along, Cut, Across) * Along;
				}
				const FVector2D Before = Direction(bStart ? (Chord + Chords - 1) % Chords : Chord);
				const FVector2D After = Direction(bStart ? Chord : (Chord + 1) % Chords);
				const FVector2D Mitre = (RightOf(Before) + RightOf(After)).GetSafeNormal();
				const double Cosine = Mitre | RightOf(After);
				return Path[Vertex % Count] + Across / FMath::Max(Cosine, UE_KINDA_SMALL_NUMBER) * Mitre;
			};
			for (int32 Chord = 0; Chord < Chords; ++Chord)
			{
				TArray<FVector2D>& Quad = Out.AddDefaulted_GetRef();
				for (const FVector2D& Point : {At(Chord, Chord, Least), At(Chord + 1, Chord, Least), At(Chord + 1, Chord, Most), At(Chord, Chord, Most)})
				{
					Quad.Add(InStage(Frame, Point));
				}
			}
			return Out;
		}
	}

	TArray<FVector2D> PlanPoints(const FFormProfile& Profile)
	{
		TArray<FVector2D> Out;
		if (Profile.Start.Num() < 2)
		{
			return Out;
		}
		Out.Add(Profile.StartPoint());
		for (const FFormProfileStep& Step : Profile.Steps)
		{
			if (!Step.IsArc())
			{
				Out.Add(FVector2D(Step.Line[0], Step.Line[1]));
				continue;
			}
			// An arc that starts elsewhere is joined to the current point by a line first, as Form draws it.
			const FFormArc& Arc = Step.Arc;
			const double Largest = FMath::Max(Arc.RadiiMeters[0], Arc.RadiiMeters.Last());
			const double Station = FMath::RadiansToDegrees(2.0 * FMath::Acos(1.0 - ChordToleranceMeters / FMath::Max(Largest, ChordToleranceMeters)));
			const int32 Stations = FMath::Max(1, FMath::CeilToInt32(FMath::Abs(Arc.ToDegrees - Arc.FromDegrees) / Station));
			for (int32 Index = 0; Index <= Stations; ++Index)
			{
				Out.Add(ArcPoint(Arc, FMath::Lerp(Arc.FromDegrees, Arc.ToDegrees, double(Index) / Stations)));
			}
		}
		return Out;
	}

	TArray<TArray<FVector2D>> SetInto(const FFormDrawing& Piece)
	{
		if (const FFormExtrusion* Laid = Piece.Shape.TryGet<FFormExtrusion>())
		{
			TArray<FVector2D> Outline;
			for (const FVector2D& Point : PlanPoints(Laid->Outline))
			{
				Outline.Add(InStage(Piece.Frame, Point));
			}
			return {Outline};
		}
		if (const FFormSweep* Swept = Piece.Shape.TryGet<FFormSweep>(); Swept && Swept->Face.Start.IsEmpty())
		{
			return BandOf(*Swept, Piece.Frame);
		}
		return {};
	}

	void DrawTerrain(TArray<FFormDrawing>& Out, TConstArrayView<FSitePrism> Received)
	{
		const double Half = Plan::FootprintHalf;
		const FBox2D Paving = ParvisPaving();
		TArray<FFormProfile> Holes = {Rectangle({Plan::CanalWestX, Plan::CanalNorthY}, {Plan::CanalEastX, Plan::CanalSouthY}),
			Circle(At(Plan::RondPoint), Plan::BasinOuterRadius), Rectangle(Paving.Min, Paving.Max)};
		for (const FSitePrism& Building : Received)
		{
			if (Building.BottomMeters < Plan::LawnTop && Building.TopMeters > Plan::SoilBottom && Building.OutlineMeters.Num() >= 3)
			{
				Holes.Add(Polygon(Building.OutlineMeters));
			}
		}
		for (const FFormDrawing& Piece : Out)
		{
			for (const TArray<FVector2D>& Covered : SetInto(Piece))
			{
				if (Covered.Num() >= 3)
				{
					Holes.Add(Polygon(Covered));
				}
			}
		}
		Out.Add(Laid(Pieces::Terrain, TEXT("terrain"), Rectangle({-Half, -Half}, {Half, Half}), MoveTemp(Holes), Plan::SoilBottom, Plan::LawnTop,
			&Musee::Collision::Walkable));
	}
}
