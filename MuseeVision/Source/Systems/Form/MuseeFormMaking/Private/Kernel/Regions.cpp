#include "Kernel/Regions.h"

#include "Curve/PolygonIntersectionUtils.h"
#include "Curve/PolygonOffsetUtils.h"
#include "Kernel/Profile.h"

namespace Musee::Form
{
	namespace
	{
		using UE::Geometry::FGeneralPolygon2d;
		using UE::Geometry::FPolygon2d;

		/**
		 * The mitre a shrunk or grown corner may reach, in multiples of the offset: Clipper's default of 2 would square
		 * off every corner sharper than 60 degrees. Ten keeps corners down to about 11.5 degrees sharp, finer than any
		 * the museum's settings-out draw.
		 */
		constexpr double MitreLimit = 10.0;

		FPolygon2d LoopPolygon(TConstArrayView<FVector2D> Loop, bool bAnticlockwise)
		{
			TArray<UE::Math::TVector2<double>> Points;
			for (const FVector2D& Point : Loop)
			{
				Points.Add(UE::Math::TVector2<double>(Point.X, Point.Y));
			}
			FPolygon2d Polygon(Points);
			if ((Polygon.SignedArea() > 0.0) != bAnticlockwise)
			{
				Polygon.Reverse();
			}
			return Polygon;
		}

		/** A region without its slivers. */
		FRegion Clean(FRegion Region)
		{
			Region.RemoveAll([](const FGeneralPolygon2d& Polygon) { return FMath::Abs(Polygon.SignedArea()) < SliverSquareMeters; });
			return Region;
		}
	}

	FRegion RegionOf(TConstArrayView<FVector2D> Outer, TConstArrayView<TArray<FVector2D>> Holes)
	{
		FRegion Region;
		Region.Add(FGeneralPolygon2d(LoopPolygon(Outer, true)));
		FRegion Taken;
		for (const TArray<FVector2D>& Hole : Holes)
		{
			Taken.Add(FGeneralPolygon2d(LoopPolygon(Hole, true)));
		}
		// The holes may run over the outline or each other: a difference settles the region they leave.
		FRegion Settled;
		UE::Geometry::PolygonsUnion(Region, Settled, false);
		return Taken.IsEmpty() ? Clean(MoveTemp(Settled)) : Difference(Settled, Taken);
	}

	FRegion Union(const FRegion& A, const FRegion& B)
	{
		FRegion Both = A;
		Both.Append(B);
		FRegion Out;
		UE::Geometry::PolygonsUnion(Both, Out, false);
		return Clean(MoveTemp(Out));
	}

	FRegion Difference(const FRegion& A, const FRegion& B)
	{
		if (B.IsEmpty() || A.IsEmpty())
		{
			return A;
		}
		FRegion Out;
		UE::Geometry::PolygonsDifference(A, B, Out);
		return Clean(MoveTemp(Out));
	}

	FRegion Intersection(const FRegion& A, const FRegion& B)
	{
		if (A.IsEmpty() || B.IsEmpty())
		{
			return {};
		}
		FRegion Out;
		UE::Geometry::PolygonsIntersection(A, B, Out);
		return Clean(MoveTemp(Out));
	}

	FRegion Offset(const FRegion& Region, double Meters)
	{
		if (Region.IsEmpty() || Meters == 0.0)
		{
			return Region;
		}
		FRegion Out;
		UE::Geometry::PolygonsOffset(Meters, Region, Out, false, MitreLimit, UE::Geometry::EPolygonOffsetJoinType::Miter);
		return Clean(MoveTemp(Out));
	}

	double AreaSquareMeters(const FRegion& Region)
	{
		double Area = 0.0;
		for (const FGeneralPolygon2d& Polygon : Region)
		{
			Area += FMath::Abs(Polygon.SignedArea());
		}
		return Area;
	}

	TArray<TArray<FVector2D>> LoopsOf(const FGeneralPolygon2d& Polygon)
	{
		TArray<TArray<FVector2D>> Loops;
		auto Add = [&Loops](const FPolygon2d& Loop)
		{
			TArray<FVector2D> Points;
			for (const UE::Math::TVector2<double>& Point : Loop.GetVertices())
			{
				Points.Add(FVector2D(Point.X, Point.Y));
			}
			Points = CleanLoop(Points, WeldToleranceMeters);
			if (Points.Num() >= 3)
			{
				Loops.Add(MoveTemp(Points));
			}
		};
		Add(Polygon.GetOuter());
		if (Loops.IsEmpty())
		{
			return {};
		}
		for (const FPolygon2d& Hole : Polygon.GetHoles())
		{
			Add(Hole);
		}
		return Loops;
	}

	TArray<TArray<FVector2D>> AllLoops(const FRegion& Region)
	{
		TArray<TArray<FVector2D>> Loops;
		for (const FGeneralPolygon2d& Polygon : Region)
		{
			Loops.Append(LoopsOf(Polygon));
		}
		return Loops;
	}

	void InsertPointsOnEdges(TArray<FVector2D>& Loop, TConstArrayView<FVector2D> Points)
	{
		TArray<FVector2D> Out;
		for (int32 Index = 0; Index < Loop.Num(); ++Index)
		{
			const FVector2D A = Loop[Index];
			const FVector2D B = Loop[(Index + 1) % Loop.Num()];
			Out.Add(A);
			const FVector2D Edge = B - A;
			const double Length = Edge.Size();
			if (Length <= WeldToleranceMeters)
			{
				continue;
			}
			const FVector2D Along = Edge / Length;
			TArray<TPair<double, FVector2D>> OnEdge;
			for (const FVector2D& Point : Points)
			{
				const double At = FVector2D::DotProduct(Point - A, Along);
				const double Off = FMath::Abs(FVector2D::CrossProduct(Along, Point - A));
				if (At > WeldToleranceMeters && At < Length - WeldToleranceMeters && Off <= WeldToleranceMeters)
				{
					OnEdge.Emplace(At, Point);
				}
			}
			OnEdge.Sort([](const TPair<double, FVector2D>& L, const TPair<double, FVector2D>& R) { return L.Key < R.Key; });
			for (const TPair<double, FVector2D>& Point : OnEdge)
			{
				if (FVector2D::Distance(Out.Last(), Point.Value) > WeldToleranceMeters)
				{
					Out.Add(Point.Value);
				}
			}
		}
		Loop = MoveTemp(Out);
	}

	FProfileStations LoopStations(TConstArrayView<FVector2D> Loop, double CreaseRadians)
	{
		TArray<FVector2D> Points;
		for (const FVector2D& Point : Loop)
		{
			if (Points.IsEmpty() || FVector2D::Distance(Points.Last(), Point) > WeldToleranceMeters)
			{
				Points.Add(Point);
			}
		}
		while (Points.Num() > 1 && FVector2D::Distance(Points.Last(), Points[0]) <= WeldToleranceMeters)
		{
			Points.Pop();
		}
		FProfileStations Stations;
		Stations.bClosed = true;
		const int32 Count = Points.Num();
		// The normal on the right of each edge: the side a loop drawn with its solid on the left faces.
		auto EdgeNormal = [&Points, Count](int32 Edge)
		{
			const FVector2D Step = Points[(Edge + 1) % Count] - Points[Edge];
			return FVector2D(Step.Y, -Step.X).GetSafeNormal();
		};
		double Length = 0.0;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FProfileVertex& Vertex = Stations.Vertices.AddDefaulted_GetRef();
			Vertex.Point = Points[Index];
			Vertex.NormalIn = EdgeNormal((Index + Count - 1) % Count);
			Vertex.NormalOut = EdgeNormal(Index);
			const double Turn = FMath::Atan2(FMath::Abs(FVector2D::CrossProduct(Vertex.NormalIn, Vertex.NormalOut)),
				FVector2D::DotProduct(Vertex.NormalIn, Vertex.NormalOut));
			Vertex.bCrisp = Turn > CreaseRadians;
			if (!Vertex.bCrisp)
			{
				Vertex.NormalIn = Vertex.NormalOut = (Vertex.NormalIn + Vertex.NormalOut).GetSafeNormal();
			}
			Vertex.LengthMeters = Length;
			Length += FVector2D::Distance(Points[Index], Points[(Index + 1) % Count]);
		}
		Stations.ClosedLengthMeters = Length;
		return Stations;
	}

	TArray<FVector2D> CleanLoop(TConstArrayView<FVector2D> Loop, double LeastMeters)
	{
		// A turn within a microradian of straight back is a spike, not a corner.
		constexpr double StraightBack = 1.0e-6;
		TArray<FVector2D> Points(Loop.GetData(), Loop.Num());
		bool bDropped = true;
		while (bDropped && Points.Num() >= 3)
		{
			bDropped = false;
			for (int32 Index = 0; Index < Points.Num() && Points.Num() >= 3; ++Index)
			{
				const FVector2D In = Points[Index] - Points[(Index + Points.Num() - 1) % Points.Num()];
				const FVector2D Out = Points[(Index + 1) % Points.Num()] - Points[Index];
				const bool bShort = In.Size() < LeastMeters;
				const bool bSpike = FVector2D::DotProduct(In, Out) < 0.0
					&& FMath::Abs(FVector2D::CrossProduct(In, Out)) <= StraightBack * In.Size() * Out.Size();
				if (bShort || bSpike)
				{
					Points.RemoveAt(Index);
					bDropped = true;
					--Index;
				}
			}
		}
		return Points;
	}

	bool IsInside(TConstArrayView<FVector2D> Loop, const FVector2D& Point)
	{
		bool bInside = false;
		for (int32 Index = 0, Previous = Loop.Num() - 1; Index < Loop.Num(); Previous = Index++)
		{
			const FVector2D& A = Loop[Index];
			const FVector2D& B = Loop[Previous];
			if ((A.Y > Point.Y) != (B.Y > Point.Y) && Point.X < (B.X - A.X) * (Point.Y - A.Y) / (B.Y - A.Y) + A.X)
			{
				bInside = !bInside;
			}
		}
		return bInside;
	}
}
