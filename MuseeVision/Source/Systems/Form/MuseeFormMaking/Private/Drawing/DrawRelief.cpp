#include "Drawing/Draw.h"

#include "Drawing/DrawnProfile.h"
#include "Kernel/MeshBuilder.h"
#include "Kernel/Patch.h"
#include "Kernel/PlanarFace.h"
#include "Kernel/PlanRuns.h"

/**
 * A relief is a block: its carved face, a grid of points each displaced from the ground by the carving's height, and
 * behind it a back of the same U stations with only its two edge heights, joined to the face by its edges. On a closed
 * path the block runs round and meets itself, so it has no ends.
 */
namespace Musee::Form::Drawing
{
	namespace
	{
		/** Where a point of the ground and its outward direction stand, and so where the carving and the back lie. */
		struct FGround
		{
			TFunction<FVector(double U, double V, double Proud)> At;
			TFunction<FVector(double U)> Out;
			TFunction<FVector(double U)> Along;
			double Length = 0.0;
			double From = 0.0;
			double To = 0.0;
			bool bLoop = false;
		};

		TArray<double> EvenStations(double From, double To, double Grid)
		{
			const int32 Steps = FMath::Max(1, FMath::CeilToInt32((To - From) / Grid));
			TArray<double> Stations;
			for (int32 Step = 0; Step <= Steps; ++Step)
			{
				Stations.Add(FMath::Lerp(From, To, double(Step) / Steps));
			}
			return Stations;
		}

		bool GroundOf(const FFormRelief& Shape, TArray<FSweepFrame>& OutRun, FGround& OutGround, FString& OutWhy)
		{
			if (!Shape.IsOnPath())
			{
				OutGround.At = [](double U, double V, double Proud) { return FVector(U, V, Proud); };
				OutGround.Out = [](double) { return FVector::UpVector; };
				OutGround.Along = [](double) { return FVector::ForwardVector; };
				OutGround.Length = Shape.SizeMeters.X;
				OutGround.From = 0.0;
				OutGround.To = Shape.SizeMeters.Y;
				return OutGround.Length > 0.0 && OutGround.To > 0.0;
			}
			if (!CheckProfile(Shape.Path, TEXT("the path"), false, OutWhy))
			{
				return false;
			}
			TArray<TArray<FSweepFrame>> Runs = PlanRuns(Resolve(Shape.Path), 0.0);
			if (Runs.Num() != 1)
			{
				OutWhy = TEXT("a relief's path is one smooth stretch: split a path with corners into one relief per stretch");
				return false;
			}
			OutRun = MoveTemp(Runs[0]);
			const TArray<FSweepFrame>& Run = OutRun;
			const double Offset = Shape.OffsetMeters;
			OutGround.At = [&Run, Offset](double U, double V, double Proud)
			{
				const FSweepFrame F = FrameAt(Run, U);
				return F.Origin + F.AxisA * (Offset + Proud) + FVector::UpVector * V;
			};
			OutGround.Out = [&Run](double U) { return FrameAt(Run, U).NormalA; };
			OutGround.Along = [&Run](double U) { return FrameAt(Run, U).Along; };
			OutGround.Length = Run.Last().U;
			OutGround.From = Shape.BottomMeters;
			OutGround.To = Shape.TopMeters;
			OutGround.bLoop = Shape.Path.Closed;
			return OutGround.To > OutGround.From;
		}

		/** A strip of quads between two lines of points, each facing Normal(point). */
		void Strip(FMeshBuilder& Mesh, TConstArrayView<FVector> A, TConstArrayView<FVector> B, TConstArrayView<double> UVAlong,
			TFunctionRef<FVector(int32)> Normal)
		{
			TArray<int32> VA;
			TArray<int32> VB;
			for (int32 Index = 0; Index < A.Num(); ++Index)
			{
				const double Across = FVector::Distance(A[Index], B[Index]);
				VA.Add(Mesh.AddVertex(A[Index], Normal(Index), FVector2D(UVAlong[Index], 0.0)));
				VB.Add(Mesh.AddVertex(B[Index], Normal(Index), FVector2D(UVAlong[Index], Across)));
			}
			for (int32 Index = 0; Index + 1 < A.Num(); ++Index)
			{
				Mesh.AddQuad(VA[Index], VA[Index + 1], VB[Index + 1], VB[Index]);
			}
		}
	}

	bool DrawRelief(const FFormRelief& Shape, FMeshBuilder& Mesh, FString& OutWhy)
	{
		if (!Shape.HeightMeters || Shape.GridMeters <= 0.0 || Shape.BackMeters <= 0.0)
		{
			OutWhy = TEXT("a relief has a height function, a grid and a block behind its ground");
			return false;
		}
		TArray<FSweepFrame> Run;
		FGround Ground;
		if (!GroundOf(Shape, Run, Ground, OutWhy))
		{
			if (OutWhy.IsEmpty())
			{
				OutWhy = TEXT("the relief's ground has no extent");
			}
			return false;
		}
		const TArray<double> Us = EvenStations(0.0, Ground.Length, Shape.GridMeters);
		const TArray<double> Vs = EvenStations(Ground.From, Ground.To, Shape.GridMeters);
		const double BackVs[2] = {Ground.From, Ground.To};
		const auto& Height = Shape.HeightMeters;

		// The carved face, and the back behind it on the same U stations. A grid's own normal is the cross product of
		// its first and second directions: along a path, U then V faces into the block (along travel, then up), on a
		// plane out of it (x, then y). Each grid takes the order that faces it away from the block.
		const bool bFaceVFirst = Shape.IsOnPath();
		auto Sample = [&Us](TConstArrayView<double> V, bool bVFirst, TFunctionRef<FVector(double, double)> Point)
		{
			return bVFirst ? FPatchGrid::Sample(V, Us, [&Point](double A, double B) { return Point(B, A); })
				: FPatchGrid::Sample(Us, V, Point);
		};
		auto At = [](const FPatchGrid& Grid, bool bVFirst, int32 I, int32 J) { return bVFirst ? Grid.At(J, I) : Grid.At(I, J); };
		auto NoHint = [](const FVector&) { return FVector::ZeroVector; };
		Mesh.UseSlot(FFormRelief::FacePart);
		const FPatchGrid Face = Sample(Vs, bFaceVFirst, [&](double U, double V) { return Ground.At(U, V, FMath::Max(0.0, Height(U, V))); });
		Patch(Mesh, Face, NoHint);
		Mesh.UseSlot(FFormRelief::BlockPart);
		const FPatchGrid BackGrid = Sample(BackVs, !bFaceVFirst, [&](double U, double V) { return Ground.At(U, V, -Shape.BackMeters); });
		Patch(Mesh, BackGrid, NoHint);

		// The bottom and top edges, face to back along every U station.
		for (const bool bTop : {false, true})
		{
			TArray<FVector> Front;
			TArray<FVector> Rear;
			for (int32 I = 0; I < Us.Num(); ++I)
			{
				Front.Add(At(Face, bFaceVFirst, I, bTop ? Vs.Num() - 1 : 0));
				Rear.Add(At(BackGrid, !bFaceVFirst, I, bTop ? 1 : 0));
			}
			const FVector Facing = Shape.IsOnPath() ? FVector::UpVector : FVector::RightVector;
			Strip(Mesh, Front, Rear, Us, [&](int32) { return bTop ? Facing : -Facing; });
		}
		if (Ground.bLoop)
		{
			return true;
		}
		// The two ends: each a flat section through the face's column of points and the back's two.
		for (const bool bEnd : {false, true})
		{
			const int32 I = bEnd ? Us.Num() - 1 : 0;
			TArray<FVector> Section;
			for (int32 J = 0; J < Vs.Num(); ++J)
			{
				Section.Add(At(Face, bFaceVFirst, I, J));
			}
			Section.Add(At(BackGrid, !bFaceVFirst, I, 1));
			Section.Add(At(BackGrid, !bFaceVFirst, I, 0));
			const FVector Along = bEnd ? Ground.Along(Us[I]) : -Ground.Along(Us[I]);
			const FVector Up = Shape.IsOnPath() ? FVector::UpVector : FVector::RightVector;
			const FVector Out = FVector::CrossProduct(Up, Along).GetSafeNormal();
			const FVector Origin = Section[0];
			TArray<FVector2D> Drawn;
			for (const FVector& P : Section)
			{
				Drawn.Add(FVector2D(FVector::DotProduct(P - Origin, Out), FVector::DotProduct(P - Origin, Up)));
			}
			auto Place = [&Origin, &Out, &Up](const FVector2D& Point) { return Origin + Out * Point.X + Up * Point.Y; };
			if (!AddFlatPolygon(Mesh, Drawn, {}, Place, Along, [](const FVector2D& Point) { return Point; }))
			{
				OutWhy = TEXT("the carving undercuts its own end, so the end's section crosses itself");
				return false;
			}
		}
		return true;
	}
}
