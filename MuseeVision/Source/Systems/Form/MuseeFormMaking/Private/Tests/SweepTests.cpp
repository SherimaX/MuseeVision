#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Kernel/MeshBuilder.h"
#include "Kernel/PlanRuns.h"
#include "Kernel/Profile.h"
#include "Kernel/Sweep.h"
#include "Kernel/Welding.h"
#include "Stations/Stations.h"
#include "Tests/FormTestShapes.h"

namespace Musee::Form::Tests
{
	namespace
	{
		/**
		 * A plan path: a 3 m radius curve through 60 degrees, stationed at the chord tolerance, then a 2 m leg after a
		 * hard right-angle turn towards the plan normal.
		 */
		TArray<FVector2D> CurveThenCorner()
		{
			TArray<FVector2D> Path;
			for (const double Angle : ArcStations(3.0, 0.0, UE_DOUBLE_PI / 3.0, 0.001))
			{
				Path.Add(3.0 * FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)));
			}
			const FVector2D Direction = (Path.Last() - Path[Path.Num() - 2]).GetSafeNormal();
			Path.Add(Path.Last() + 2.0 * FVector2D(-Direction.Y, Direction.X));
			return Path;
		}

		/** For each welded point, the distinct normals of the vertices standing on it. */
		TArray<TArray<FVector>> NormalsByPoint(const FMeshBuilder& Mesh, const FWeld& Weld)
		{
			TArray<TArray<FVector>> Normals;
			Normals.SetNum(Weld.Points.Num());
			for (int32 Vertex = 0; Vertex < Mesh.NumVertices(); ++Vertex)
			{
				TArray<FVector>& Here = Normals[Weld.PointOfVertex[Vertex]];
				if (!Here.ContainsByPredicate([&](const FVector& N) { return N.Equals(Mesh.Normal(Vertex), 1e-9); }))
				{
					Here.Add(Mesh.Normal(Vertex));
				}
			}
			return Normals;
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormSweepNormalsContinuousTest, "Musee.Form.SweepNormalsContinuous",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

	bool FFormSweepNormalsContinuousTest::RunTest(const FString& Parameters)
	{
		const FProfileStations Cornice = OvoloCornice().Resolve(0.001);
		const TArray<FVector2D> Path = CurveThenCorner();
		const TArray<TArray<FSweepFrame>> Runs = PlanRuns(Path, false, 3.0);
		if (!TestEqual(TEXT("The hard corner splits the path into two runs"), Runs.Num(), 2))
		{
			return false;
		}

		// Along the curve: one normal per point everywhere but along the drawn arris, where there are two.
		FMeshBuilder Curve;
		Sweep(Curve, Runs[0], Cornice);
		const TArray<TArray<FVector>> CurveNormals = NormalsByPoint(Curve, WeldVertices(Curve));
		int32 Creased = 0;
		int32 Torn = 0;
		for (const TArray<FVector>& Normals : CurveNormals)
		{
			Creased += Normals.Num() == 2 ? 1 : 0;
			Torn += Normals.Num() > 2 ? 1 : 0;
		}
		TestEqual(TEXT("Points on the curve with two normals: the arris at each frame"), Creased, Runs[0].Num());
		TestEqual(TEXT("Points on the curve with more than two normals"), Torn, 0);

		// Every vertex normal leans from its triangles' faces by less than one station's turn: the ovolo turns
		// 15 degrees a station at 1 mm (0.1 m radius), the plan 3 degrees (3 m radius).
		FMeshBuilder Whole;
		for (const TArray<FSweepFrame>& Run : Runs)
		{
			Sweep(Whole, Run, Cornice);
		}
		const double StationTurn = FMath::DegreesToRadians(16.0);
		int32 Leaning = 0;
		for (int32 Triangle = 0; Triangle < Whole.NumTriangles(); ++Triangle)
		{
			const FVector Face = FaceNormal(Whole, Triangle);
			const FIntVector& T = Whole.Triangle(Triangle);
			for (const int32 Vertex : {T.X, T.Y, T.Z})
			{
				Leaning += AngleBetween(Whole.Normal(Vertex), Face) < StationTurn ? 0 : 1;
			}
		}
		TestEqual(TEXT("Vertex normals leaning from their faces by more than a station's turn"), Leaning, 0);

		// At the hard corner the two runs keep their own normals: the vertical face turns a right angle there.
		const FWeld Weld = WeldVertices(Whole);
		const TArray<TArray<FVector>> WholeNormals = NormalsByPoint(Whole, Weld);
		const FVector Corner(Path[Path.Num() - 2].X, Path[Path.Num() - 2].Y, 3.0);
		int32 Mitred = 0;
		int32 Astray = 0;
		for (int32 Point = 0; Point < WholeNormals.Num(); ++Point)
		{
			const TArray<FVector>& Normals = WholeNormals[Point];
			bool bSquare = false;
			for (int32 I = 0; I < Normals.Num(); ++I)
			{
				for (int32 J = I + 1; J < Normals.Num(); ++J)
				{
					bSquare |= AngleBetween(Normals[I], Normals[J]) > FMath::DegreesToRadians(60.0);
				}
			}
			const bool bAtCorner = FVector::Distance(Weld.Points[Point], Corner) < 0.4;
			Mitred += (bSquare && bAtCorner) ? 1 : 0;
			// Away from the corner only the arris can split normals, and it turns 90 degrees too: allow it.
			Astray += (bSquare && !bAtCorner && Normals.Num() > 2) ? 1 : 0;
		}
		TestTrue(TEXT("The mitre keeps each run's own normals"), Mitred > 0);
		TestEqual(TEXT("Points away from the corner with torn normals"), Astray, 0);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFormPlanRunsMetersTest, "Musee.Form.PlanRunsMeters",
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

	bool FFormPlanRunsMetersTest::RunTest(const FString& Parameters)
	{
		// A 4 m by 3 m room (hard corners, closed) and the curve-then-corner path (smooth and hard, open).
		const TArray<FVector2D> Room = {FVector2D(0, 0), FVector2D(4, 0), FVector2D(4, 3), FVector2D(0, 3)};
		const TArray<TArray<TArray<FSweepFrame>>> Paths = {PlanRuns(Room, true, 0.0),
			PlanRuns(CurveThenCorner(), false, 0.0)};
		TestEqual(TEXT("The room splits into four runs"), Paths[0].Num(), 4);

		// U is metres along the path itself, and runs on unbroken from one run into the next across a mitre.
		int32 Wrong = 0;
		int32 Broken = 0;
		for (const TArray<TArray<FSweepFrame>>& Runs : Paths)
		{
			for (int32 RunIndex = 0; RunIndex < Runs.Num(); ++RunIndex)
			{
				const TArray<FSweepFrame>& Run = Runs[RunIndex];
				for (int32 Frame = 0; Frame + 1 < Run.Num(); ++Frame)
				{
					const double Step = FVector::Distance(Run[Frame].Origin, Run[Frame + 1].Origin);
					Wrong += FMath::IsNearlyEqual(Run[Frame + 1].U - Run[Frame].U, Step, 1e-12) ? 0 : 1;
				}
				if (RunIndex > 0)
				{
					Broken += Runs[RunIndex - 1].Last().U == Run[0].U ? 0 : 1;
				}
			}
		}
		TestEqual(TEXT("Steps whose U is not their length along the path"), Wrong, 0);
		TestEqual(TEXT("Runs whose U does not follow on from the run before"), Broken, 0);

		// Round the 3 m curve an offset a towards its centre is shorter than its U by the factor 1 - a / 3, as
		// FSweepFrame documents: a small stretch, never a shear.
		const TArray<FSweepFrame>& Curve = Paths[1][0];
		const FVector2D Inside(0.3, 0.0);
		const double Travelled = Curve[Curve.Num() - 2].U - Curve[1].U;
		double Offset = 0.0;
		for (int32 Frame = 1; Frame + 2 < Curve.Num(); ++Frame)
		{
			Offset += FVector::Distance(Curve[Frame].At(Inside), Curve[Frame + 1].At(Inside));
		}
		TestEqual(TEXT("The inside offset's length is its U times 1 - a / R"), Offset / Travelled, 1.0 - 0.3 / 3.0, 1e-3);

		// The mitre keeps the section: at a corner the offset point lies at the offset from both walls.
		const FSweepFrame& Arriving = Paths[0][0].Last();
		const FVector Landed = Arriving.At(FVector2D(0.25, 0.0));
		TestTrue(TEXT("A mitred corner keeps the offset from both walls"), Landed.Equals(FVector(3.75, 0.25, 0.0), 1e-12));
		return true;
	}
}

#endif
