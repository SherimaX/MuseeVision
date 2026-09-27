#include "Drawing/Draw.h"

#include "Drawing/DrawnProfile.h"
#include "Kernel/MeshBuilder.h"
#include "Kernel/Tolerances.h"
#include "Kernel/Triangulation.h"

/**
 * A heightfield is set out on one triangulation of its plan: the outline and the holes, their chords divided to the grid,
 * and a triangular lattice of the grid within, kept half a grid clear of every boundary. The top and the underside are
 * that triangulation raised to their heights, each normal from its surface's slope there; the sides stand plumb on the
 * boundary's stations, smooth along its arcs and crisp at its corners, and meet the two surfaces point for point.
 */
namespace Musee::Form::Drawing
{
	namespace
	{
		/** The step of the central differences that give a surface's slope: a tenth of a millimetre. */
		constexpr double SlopeStepMeters = 1.0e-4;

		/** Lattice points keep at least half a grid from every boundary, so no sliver lies along it. */
		constexpr double HeightfieldClearance = 0.5;

		/** A boundary loop run with the solid on its left (the outline anticlockwise, a hole clockwise), its chords divided. */
		TArray<FProfileVertex> DividedLoop(const FProfileStations& Loop, bool bOutline, double Grid)
		{
			const bool bAnticlockwise = Loop.SignedAreaSquareMeters() >= 0.0;
			const FProfileStations Run = bAnticlockwise == bOutline ? Loop : Loop.Reversed();
			TArray<FProfileVertex> Out;
			const int32 Count = Run.Vertices.Num();
			for (int32 Index = 0; Index < Count; ++Index)
			{
				const FProfileVertex& From = Run.Vertices[Index];
				const FProfileVertex& To = Run.Vertices[(Index + 1) % Count];
				Out.Add(From);
				const int32 Parts = FMath::Max(1, FMath::CeilToInt32(FVector2D::Distance(From.Point, To.Point) / Grid));
				for (int32 Part = 1; Part < Parts; ++Part)
				{
					FProfileVertex Between;
					Between.Point = FMath::Lerp(From.Point, To.Point, double(Part) / Parts);
					Between.NormalIn = From.NormalOut;
					Between.NormalOut = From.NormalOut;
					Between.bCrisp = false;
					Out.Add(Between);
				}
			}
			return Out;
		}

		TArray<FVector2D> PointsOf(TConstArrayView<FProfileVertex> Loop)
		{
			TArray<FVector2D> Out;
			for (const FProfileVertex& Vertex : Loop)
			{
				Out.Add(Vertex.Point);
			}
			return Out;
		}

		/** Whether a point lies inside a closed loop (the even-odd rule). */
		bool InsideLoop(TConstArrayView<FVector2D> Loop, const FVector2D& Point)
		{
			bool bInside = false;
			for (int32 I = 0, J = Loop.Num() - 1; I < Loop.Num(); J = I++)
			{
				const FVector2D& A = Loop[I];
				const FVector2D& B = Loop[J];
				if ((A.Y > Point.Y) != (B.Y > Point.Y) && Point.X < A.X + (Point.Y - A.Y) * (B.X - A.X) / (B.Y - A.Y))
				{
					bInside = !bInside;
				}
			}
			return bInside;
		}

		/** The distance from a point to the nearest chord of any loop. */
		double Clearance(TConstArrayView<TArray<FVector2D>> Loops, const FVector2D& Point)
		{
			double Least = TNumericLimits<double>::Max();
			for (const TArray<FVector2D>& Loop : Loops)
			{
				for (int32 I = 0, J = Loop.Num() - 1; I < Loop.Num(); J = I++)
				{
					const FVector2D Chord = Loop[I] - Loop[J];
					const double Along = FMath::Clamp(FVector2D::DotProduct(Point - Loop[J], Chord) / FMath::Max(Chord.SizeSquared(), UE_DOUBLE_SMALL_NUMBER), 0.0, 1.0);
					Least = FMath::Min(Least, FVector2D::Distance(Loop[J] + Chord * Along, Point));
				}
			}
			return Least;
		}

		/** A triangular lattice of the grid inside the outline and outside the holes, clear of every boundary. */
		TArray<FVector2D> InnerLattice(TConstArrayView<TArray<FVector2D>> Loops, double Grid)
		{
			FBox2D Bounds(ForceInit);
			for (const FVector2D& Point : Loops[0])
			{
				Bounds += Point;
			}
			const double Row = Grid * FMath::Sqrt(3.0) * 0.5;
			TArray<FVector2D> Inner;
			const int32 Rows = FMath::FloorToInt32((Bounds.Max.Y - Bounds.Min.Y) / Row);
			const int32 Columns = FMath::FloorToInt32((Bounds.Max.X - Bounds.Min.X) / Grid);
			for (int32 J = 1; J <= Rows; ++J)
			{
				for (int32 I = 0; I <= Columns; ++I)
				{
					const FVector2D Point(Bounds.Min.X + Grid * (I + ((J % 2) ? 0.5 : 0.0)), Bounds.Min.Y + Row * J);
					bool bInside = InsideLoop(Loops[0], Point);
					for (int32 Hole = 1; bInside && Hole < Loops.Num(); ++Hole)
					{
						bInside = !InsideLoop(Loops[Hole], Point);
					}
					if (bInside && Clearance(Loops, Point) >= HeightfieldClearance * Grid)
					{
						Inner.Add(Point);
					}
				}
			}
			return Inner;
		}

		/** The unit normal of the surface z = Height(x, y), facing up, or down for an underside. */
		FVector SurfaceNormal(const TFunction<double(double, double)>& Height, const FVector2D& At, bool bUp)
		{
			const double H = SlopeStepMeters;
			const double DX = (Height(At.X + H, At.Y) - Height(At.X - H, At.Y)) / (2.0 * H);
			const double DY = (Height(At.X, At.Y + H) - Height(At.X, At.Y - H)) / (2.0 * H);
			const FVector Up = FVector(-DX, -DY, 1.0).GetSafeNormal();
			return bUp ? Up : -Up;
		}

		/** The plumb side on one boundary loop, between the underside and the top at each station. */
		void DrawSide(const FFormHeightfield& Shape, TConstArrayView<FProfileVertex> Loop, FMeshBuilder& Mesh)
		{
			Mesh.UseSlot(FFormHeightfield::SidePart);
			const int32 Count = Loop.Num();
			double Travelled = 0.0;
			for (int32 Index = 0; Index < Count; ++Index)
			{
				const FProfileVertex& From = Loop[Index];
				const FProfileVertex& To = Loop[(Index + 1) % Count];
				const double Length = FVector2D::Distance(From.Point, To.Point);
				auto Column = [&Shape](const FProfileVertex& At, const FVector2D& Normal2, double U, int32& OutBottom, int32& OutTop, FMeshBuilder& Into)
				{
					const FVector Normal(Normal2.X, Normal2.Y, 0.0);
					const double Bottom = Shape.BottomMeters(At.Point.X, At.Point.Y);
					const double Top = Shape.TopMeters(At.Point.X, At.Point.Y);
					OutBottom = Into.AddVertex(FVector(At.Point.X, At.Point.Y, Bottom), Normal, FVector2D(U, Bottom));
					OutTop = Into.AddVertex(FVector(At.Point.X, At.Point.Y, Top), Normal, FVector2D(U, Top));
				};
				int32 B0, T0, B1, T1;
				// Along a chord the side takes its stations' normals: the leaving one at its start, the arriving one at its end.
				Column(From, From.bCrisp ? From.NormalOut : (From.NormalIn + From.NormalOut).GetSafeNormal(), Travelled, B0, T0, Mesh);
				Column(To, To.bCrisp ? To.NormalIn : (To.NormalIn + To.NormalOut).GetSafeNormal(), Travelled + Length, B1, T1, Mesh);
				Mesh.AddQuad(B0, B1, T1, T0);
				Travelled += Length;
			}
		}

		bool CheckHeightfield(const FFormHeightfield& Shape, TArray<FProfileStations>& OutHoles, FString& OutWhy)
		{
			if (!CheckProfile(Shape.Outline, TEXT("the outline"), true, OutWhy))
			{
				return false;
			}
			for (const FFormProfile& Hole : Shape.Holes)
			{
				if (!CheckProfile(Hole, TEXT("a hole"), true, OutWhy))
				{
					return false;
				}
				OutHoles.Add(Resolve(Hole));
			}
			if (!Shape.TopMeters || !Shape.BottomMeters || Shape.GridMeters <= 0.0)
			{
				OutWhy = TEXT("a heightfield has a top, an underside and a grid");
				return false;
			}
			return true;
		}
	}

	bool DrawHeightfield(const FFormHeightfield& Shape, FMeshBuilder& Mesh, FString& OutWhy)
	{
		TArray<FProfileStations> Holes;
		if (!CheckHeightfield(Shape, Holes, OutWhy))
		{
			return false;
		}
		TArray<TArray<FProfileVertex>> Boundaries = {DividedLoop(Resolve(Shape.Outline), true, Shape.GridMeters)};
		for (const FProfileStations& Hole : Holes)
		{
			Boundaries.Add(DividedLoop(Hole, false, Shape.GridMeters));
		}
		TArray<TArray<FVector2D>> Loops;
		for (const TArray<FProfileVertex>& Boundary : Boundaries)
		{
			Loops.Add(PointsOf(Boundary));
		}
		const TArray<FVector2D> Inner = InnerLattice(Loops, Shape.GridMeters);
		TArray<FIntVector> Triangles;
		if (!TriangulatePolygonWithPoints(Loops[0], MakeArrayView(Loops).RightChop(1), Inner, Triangles))
		{
			OutWhy = TEXT("the outline or a hole crosses itself or another");
			return false;
		}
		TArray<FVector2D> Plan;
		for (const TArray<FVector2D>& Loop : Loops)
		{
			Plan.Append(Loop);
		}
		Plan.Append(Inner);
		for (const FVector2D& Point : Plan)
		{
			if (Shape.TopMeters(Point.X, Point.Y) <= Shape.BottomMeters(Point.X, Point.Y) + 2.0 * WeldToleranceMeters)
			{
				OutWhy = FString::Printf(TEXT("its top doesn't stand above its underside at (%.3f, %.3f)"), Point.X, Point.Y);
				return false;
			}
		}
		for (const bool bTop : {true, false})
		{
			Mesh.UseSlot(bTop ? FFormHeightfield::TopPart : FFormHeightfield::BottomPart);
			const TFunction<double(double, double)>& Height = bTop ? Shape.TopMeters : Shape.BottomMeters;
			TArray<int32> Vertices;
			for (const FVector2D& Point : Plan)
			{
				Vertices.Add(Mesh.AddVertex(FVector(Point.X, Point.Y, Height(Point.X, Point.Y)), SurfaceNormal(Height, Point, bTop), Point));
			}
			for (const FIntVector& T : Triangles)
			{
				Mesh.AddTriangle(Vertices[T.X], Vertices[T.Y], Vertices[T.Z]);
			}
		}
		for (const TArray<FProfileVertex>& Boundary : Boundaries)
		{
			DrawSide(Shape, Boundary, Mesh);
		}
		return true;
	}
}
