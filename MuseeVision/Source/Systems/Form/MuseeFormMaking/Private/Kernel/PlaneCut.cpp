#include "Kernel/PlaneCut.h"

#include "Kernel/MeshBuilder.h"
#include "Kernel/PlanarFace.h"
#include "Kernel/Regions.h"
#include "Kernel/ShellEdges.h"
#include "Kernel/Welding.h"

namespace Musee::Form
{
	namespace
	{
		/**
		 * A vertex within a nanometre of the plane lies on it: that is rounding in the cut's own arithmetic, a thousandth
		 * of the weld tolerance, and a face drawn on the plane (a block cut exactly at its bed) is kept whole.
		 */
		constexpr double OnPlaneMeters = 1.0e-9;

		/** The clipped mesh being written: kept vertices copied once, each cut edge split once. */
		struct FCutter
		{
			const FMeshBuilder& In;
			FMeshBuilder& Out;
			TArray<double> Side;
			TArray<int32> Kept;
			TMap<TPair<int32, int32>, int32> Split;

			FCutter(const FMeshBuilder& InMesh, FMeshBuilder& OutMesh, const FVector& Point, const FVector& Normal)
				: In(InMesh), Out(OutMesh)
			{
				for (int32 Vertex = 0; Vertex < In.NumVertices(); ++Vertex)
				{
					const double Distance = FVector::DotProduct(In.Position(Vertex) - Point, Normal);
					Side.Add(FMath::Abs(Distance) < OnPlaneMeters ? 0.0 : Distance);
				}
				Kept.Init(INDEX_NONE, In.NumVertices());
			}

			/** A vertex of the kept mesh between two of the given mesh's, T of the way from A to B. */
			int32 Blend(int32 A, int32 B, double T)
			{
				const FVector Normal = FMath::Lerp(In.Normal(A), In.Normal(B), T);
				const int32 Vertex = Out.AddVertex(FMath::Lerp(In.Position(A), In.Position(B), T),
					Normal.IsNearlyZero() ? In.Normal(A) : Normal, FMath::Lerp(In.UV(A), In.UV(B), T));
				for (int32 Channel = 1; Channel < In.NumUVChannels(); ++Channel)
				{
					Out.SetUV(Vertex, Channel, FMath::Lerp(In.UV(A, Channel), In.UV(B, Channel), T));
				}
				if (In.HasColors())
				{
					Out.SetColor(Vertex, FMath::Lerp(In.Color(A), In.Color(B), T));
				}
				return Vertex;
			}

			int32 Keep(int32 Vertex)
			{
				if (Kept[Vertex] == INDEX_NONE)
				{
					Kept[Vertex] = Blend(Vertex, Vertex, 0.0);
				}
				return Kept[Vertex];
			}

			int32 Cut(int32 A, int32 B)
			{
				const TPair<int32, int32> Key(FMath::Min(A, B), FMath::Max(A, B));
				if (const int32* Found = Split.Find(Key))
				{
					return *Found;
				}
				return Split.Add(Key, Blend(Key.Key, Key.Value, Side[Key.Key] / (Side[Key.Key] - Side[Key.Value])));
			}

			/** Keeps the part of one triangle on the kept side, as a fan over its clipped outline. Returns whether any is kept. */
			bool Clip(int32 Triangle)
			{
				const FIntVector& T = In.Triangle(Triangle);
				const int32 Corners[3] = {T.X, T.Y, T.Z};
				TArray<int32, TInlineAllocator<4>> Outline;
				for (int32 Index = 0; Index < 3; ++Index)
				{
					const int32 Here = Corners[Index];
					const int32 Next = Corners[(Index + 1) % 3];
					if (Side[Here] <= 0.0)
					{
						Outline.Add(Keep(Here));
					}
					if ((Side[Here] < 0.0 && Side[Next] > 0.0) || (Side[Here] > 0.0 && Side[Next] < 0.0))
					{
						Outline.Add(Cut(Here, Next));
					}
				}
				Out.UseSlot(In.Slots()[In.TriangleSlot(Triangle)]);
				for (int32 Index = 1; Index + 1 < Outline.Num(); ++Index)
				{
					Out.AddTriangle(Outline[0], Outline[Index], Outline[Index + 1]);
				}
				return Outline.Num() >= 3;
			}
		};

		/** Chains the open edges that lie in the plane into closed loops of points; false where one doesn't close. */
		bool CutLoops(const FMeshBuilder& Mesh, const FWeld& Weld, const FVector& Point, const FVector& Normal, TArray<TArray<int32>>& OutLoops)
		{
			const FShellEdges Edges = MeasureShellEdges(Mesh, Weld);
			TMultiMap<int32, int32> Next;
			auto InPlane = [&](int32 P) { return FMath::Abs(FVector::DotProduct(Weld.Points[P] - Point, Normal)) <= WeldToleranceMeters; };
			for (const FIntPoint& Edge : Edges.BoundaryEdges)
			{
				if (InPlane(Edge.X) && InPlane(Edge.Y))
				{
					Next.Add(Edge.X, Edge.Y);
					Next.Add(Edge.Y, Edge.X);
				}
			}
			TSet<int32> Used;
			TArray<int32> Starts;
			Next.GetKeys(Starts);
			for (const int32 Start : Starts)
			{
				TArray<int32> Ends;
				Next.MultiFind(Start, Ends);
				if (Ends.Num() != 2)
				{
					return false;
				}
				if (Used.Contains(Start))
				{
					continue;
				}
				TArray<int32>& Loop = OutLoops.AddDefaulted_GetRef();
				int32 Previous = INDEX_NONE;
				for (int32 Here = Start; !Used.Contains(Here);)
				{
					Used.Add(Here);
					Loop.Add(Here);
					TArray<int32> Ways;
					Next.MultiFind(Here, Ways);
					const int32 Way = Ways[0] != Previous ? Ways[0] : Ways[1];
					Previous = Here;
					Here = Way;
				}
				if (Loop.Num() < 3)
				{
					return false;
				}
			}
			return true;
		}

		/** Closes the cut: each loop that lies inside an even number of others is an outline, holed by the loops just inside it. */
		bool CloseCut(FMeshBuilder& Mesh, const FVector& Point, const FVector& Normal, FName CutSlot)
		{
			const FWeld Weld = WeldVertices(Mesh);
			TArray<TArray<int32>> Loops;
			if (!CutLoops(Mesh, Weld, Point, Normal, Loops))
			{
				return false;
			}
			const FVector AxisU = Normal.GetAbs().X < 0.9 ? FVector::CrossProduct(Normal, FVector::ForwardVector).GetSafeNormal()
				: FVector::CrossProduct(Normal, FVector::RightVector).GetSafeNormal();
			const FVector AxisV = FVector::CrossProduct(Normal, AxisU);
			TArray<TArray<FVector2D>> Flat;
			for (const TArray<int32>& Loop : Loops)
			{
				TArray<FVector2D>& Points = Flat.AddDefaulted_GetRef();
				for (const int32 P : Loop)
				{
					Points.Add(FVector2D(FVector::DotProduct(Weld.Points[P] - Point, AxisU), FVector::DotProduct(Weld.Points[P] - Point, AxisV)));
				}
			}
			TArray<int32> Depth;
			for (int32 Loop = 0; Loop < Flat.Num(); ++Loop)
			{
				int32 Around = 0;
				for (int32 Other = 0; Other < Flat.Num(); ++Other)
				{
					Around += Other != Loop && IsInside(Flat[Other], Flat[Loop][0]) ? 1 : 0;
				}
				Depth.Add(Around);
			}
			Mesh.UseSlot(CutSlot);
			auto Place = [&](const FVector2D& P) { return Point + AxisU * P.X + AxisV * P.Y; };
			auto UVOf = [](const FVector2D& P) { return P; };
			for (int32 Outer = 0; Outer < Flat.Num(); ++Outer)
			{
				if (Depth[Outer] % 2 != 0)
				{
					continue;
				}
				TArray<TArray<FVector2D>> Holes;
				for (int32 Hole = 0; Hole < Flat.Num(); ++Hole)
				{
					if (Depth[Hole] == Depth[Outer] + 1 && IsInside(Flat[Outer], Flat[Hole][0]))
					{
						Holes.Add(Flat[Hole]);
					}
				}
				if (!AddFlatPolygon(Mesh, Flat[Outer], Holes, Place, Normal, UVOf))
				{
					return false;
				}
			}
			return true;
		}
	}

	bool CutMesh(FMeshBuilder& Mesh, const FVector& PointMeters, const FVector& Normal, FName CutSlot, FString& OutWhy)
	{
		const FVector Unit = Normal.GetSafeNormal();
		if (Unit.IsZero())
		{
			OutWhy = TEXT("a cut's plane has no normal");
			return false;
		}
		FMeshBuilder Out(Mesh.NumUVChannels());
		FCutter Cutter(Mesh, Out, PointMeters, Unit);
		if (!Cutter.Side.ContainsByPredicate([](double Side) { return Side > 0.0; }))
		{
			return true;
		}
		bool bAnyKept = false;
		for (int32 Triangle = 0; Triangle < Mesh.NumTriangles(); ++Triangle)
		{
			bAnyKept |= Cutter.Clip(Triangle);
		}
		if (!bAnyKept)
		{
			OutWhy = TEXT("a cut takes the whole solid away");
			return false;
		}
		if (!CloseCut(Out, PointMeters, Unit, CutSlot))
		{
			OutWhy = TEXT("a cut's edge doesn't close: the solid is open where the plane cuts it, or the cut crosses itself");
			return false;
		}
		Mesh = MoveTemp(Out);
		return true;
	}
}
