#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Kernel/MeshBuilder.h"
#include "Kernel/PlanarFace.h"
#include "Kernel/Triangulation.h"
#include "Stations/Stations.h"
#include "Tests/FormTestShapes.h"

namespace Musee::Form::Tests
{
	namespace
	{
		double LoopArea(TConstArrayView<FVector2D> Loop)
		{
			double Twice = 0.0;
			for (int32 Index = 0; Index < Loop.Num(); ++Index)
			{
				Twice += FVector2D::CrossProduct(Loop[Index], Loop[(Index + 1) % Loop.Num()]);
			}
			return FMath::Abs(0.5 * Twice);
		}

		bool Inside(TConstArrayView<FVector2D> Loop, const FVector2D& P)
		{
			bool bInside = false;
			for (int32 I = 0, J = Loop.Num() - 1; I < Loop.Num(); J = I++)
			{
				if ((Loop[I].Y > P.Y) != (Loop[J].Y > P.Y)
					&& P.X < (Loop[J].X - Loop[I].X) * (P.Y - Loop[I].Y) / (Loop[J].Y - Loop[I].Y) + Loop[I].X)
				{
					bInside = !bInside;
				}
			}
			return bInside;
		}

		TArray<FVector2D> Square(const FVector2D& Center, double Half)
		{
			return {Center + FVector2D(-Half, -Half), Center + FVector2D(Half, -Half), Center + FVector2D(Half, Half),
				Center + FVector2D(-Half, Half)};
		}

		/** A round well stationed at the chord tolerance, drawn clockwise. */
		TArray<FVector2D> Well(const FVector2D& Center, double Radius)
		{
			TArray<FVector2D> Loop;
			const TArray<double> Angles = ArcStations(Radius, UE_DOUBLE_TWO_PI, 0.0, 0.001);
			for (int32 Index = 0; Index + 1 < Angles.Num(); ++Index)
			{
				Loop.Add(Center + Radius * FVector2D(FMath::Cos(Angles[Index]), FMath::Sin(Angles[Index])));
			}
			return Loop;
		}

		uint64 EdgeKey(int32 A, int32 B)
		{
			return (uint64(uint32(FMath::Min(A, B))) << 32) | uint64(uint32(FMath::Max(A, B)));
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormTriangulationWithHolesTest, "Musee.Form.TriangulationWithHoles",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

	bool FFormTriangulationWithHolesTest::RunTest(const FString& Parameters)
	{
		// An L-shaped room 12 m by 10 m with a 5 m by 4 m bite, drawn clockwise, round two column bases and a stair well.
		const TArray<FVector2D> Room = {FVector2D(0, 0), FVector2D(0, 10), FVector2D(7, 10), FVector2D(7, 6),
			FVector2D(12, 6), FVector2D(12, 0)};
		const TArray<TArray<FVector2D>> Holes = {Square(FVector2D(3, 3), 0.4), Square(FVector2D(9, 3), 0.4),
			Well(FVector2D(3.5, 7.5), 1.2)};
		TArray<FVector2D> All = Room;
		TArray<uint64> BoundaryEdges;
		for (int32 Index = 0; Index < Room.Num(); ++Index)
		{
			BoundaryEdges.Add(EdgeKey(Index, (Index + 1) % Room.Num()));
		}
		double ExpectedArea = LoopArea(Room);
		for (const TArray<FVector2D>& Hole : Holes)
		{
			const int32 First = All.Num();
			All.Append(Hole);
			for (int32 Index = 0; Index < Hole.Num(); ++Index)
			{
				BoundaryEdges.Add(EdgeKey(First + Index, First + (Index + 1) % Hole.Num()));
			}
			ExpectedArea -= LoopArea(Hole);
		}

		TArray<FIntVector> Triangles;
		if (!TestTrue(TEXT("The room triangulates"), TriangulatePolygon(Room, Holes, Triangles)))
		{
			return false;
		}
		// No point added: a polygon of V points and H holes has V + 2H - 2 triangles.
		TestEqual(TEXT("Triangles use exactly the given points"), Triangles.Num(), All.Num() + 2 * Holes.Num() - 2);
		double Area = 0.0;
		int32 Outside = 0;
		TMap<uint64, int32> EdgeUses;
		for (const FIntVector& T : Triangles)
		{
			const FVector2D A = All[T.X], B = All[T.Y], C = All[T.Z];
			Area += 0.5 * FMath::Abs(FVector2D::CrossProduct(B - A, C - A));
			const FVector2D Centroid = (A + B + C) / 3.0;
			bool bInHole = false;
			for (const TArray<FVector2D>& Hole : Holes)
			{
				bInHole |= Inside(Hole, Centroid);
			}
			Outside += (!Inside(Room, Centroid) || bInHole) ? 1 : 0;
			++EdgeUses.FindOrAdd(EdgeKey(T.X, T.Y));
			++EdgeUses.FindOrAdd(EdgeKey(T.Y, T.Z));
			++EdgeUses.FindOrAdd(EdgeKey(T.Z, T.X));
		}
		TestEqual(TEXT("The triangles cover the floor exactly"), Area, ExpectedArea, 1e-9 * ExpectedArea);
		TestEqual(TEXT("Triangles outside the room or inside a hole"), Outside, 0);
		int32 WrongUses = 0;
		for (const TPair<uint64, int32>& Use : EdgeUses)
		{
			WrongUses += Use.Value == (BoundaryEdges.Contains(Use.Key) ? 1 : 2) ? 0 : 1;
		}
		for (const uint64 Edge : BoundaryEdges)
		{
			WrongUses += EdgeUses.Contains(Edge) ? 0 : 1;
		}
		TestEqual(TEXT("Edges not used once on a boundary and twice inside"), WrongUses, 0);

		// A face of it in the builder faces its plane's normal, every triangle wound to match.
		FMeshBuilder Floor;
		FPlaneFrame Plane;
		TestTrue(TEXT("The floor face is added"), AddPlanarFace(Floor, Plane, Room, Holes));
		TestEqual(TEXT("The floor has one vertex per point"), Floor.NumVertices(), All.Num());
		int32 Turned = 0;
		for (int32 Triangle = 0; Triangle < Floor.NumTriangles(); ++Triangle)
		{
			Turned += FVector::DotProduct(FaceNormal(Floor, Triangle), Plane.Normal) > 0.999999 ? 0 : 1;
		}
		TestEqual(TEXT("Floor triangles not facing up"), Turned, 0);

		// A bow tie crosses itself and is refused.
		const TArray<FVector2D> BowTie = {FVector2D(0, 0), FVector2D(2, 2), FVector2D(2, 0), FVector2D(0, 2)};
		TestFalse(TEXT("A crossing outline is refused"), TriangulatePolygon(BowTie, {}, Triangles));
		return true;
	}
}

#endif
