#include "Drawing/Draw.h"

#include "Kernel/MeshBuilder.h"
#include "Kernel/Patch.h"
#include "Kernel/Tolerances.h"

/**
 * A cloth's sag is the membrane's: under an even tension T and its weight q per square metre, a rectangle a by b fixed on
 * its edges hangs w(x, y) = (4 q a^2 / (pi^3 T)) sum over odd n of n^-3 (1 - cosh(n pi (y - b/2) / a) / cosh(n pi b / (2a)))
 * sin(n pi x / a) (Timoshenko and Woinowsky-Krieger, 1959, section 22, the Navier-Levy solution of the membrane
 * equation). The drawing gives the depth at the middle, which fixes q/T, so the shape is the sum scaled to that depth.
 * The top and the underside are patches over the grid, and four thin edges stand on the frame.
 */
namespace Musee::Form::Drawing
{
	namespace
	{
		/**
		 * The odd terms of the membrane's series taken: they fall as n^-3, so the last taken is below a millionth of the
		 * first and the shape is true within a millionth of its depth (0.03 um for a 30 mm sag).
		 */
		constexpr int32 MembraneTerms = 100;

		/** cosh(P) / cosh(Q) for |P| <= Q, without overflow for a long, narrow cloth. */
		double CoshRatio(double P, double Q)
		{
			const double A = FMath::Abs(P);
			return FMath::Exp(A - Q) * (1.0 + FMath::Exp(-2.0 * A)) / (1.0 + FMath::Exp(-2.0 * Q));
		}

		/** The membrane's shape at (x, y) over a by b, unscaled: zero on the edges, greatest at the middle. */
		double MembraneShape(double X, double Y, double A, double B)
		{
			double Sum = 0.0;
			for (int32 Term = 0; Term < MembraneTerms; ++Term)
			{
				const double N = 2.0 * Term + 1.0;
				const double K = N * UE_DOUBLE_PI / A;
				Sum += (1.0 - CoshRatio(K * (Y - 0.5 * B), 0.5 * K * B)) * FMath::Sin(K * X) / (N * N * N);
			}
			return Sum;
		}

		/** Stations from zero to a length, no further apart than the grid. */
		TArray<double> ClothStations(double Length, double Grid)
		{
			const int32 Steps = FMath::Max(1, FMath::CeilToInt32(Length / Grid));
			TArray<double> Stations;
			for (int32 Step = 0; Step <= Steps; ++Step)
			{
				Stations.Add(Length * Step / Steps);
			}
			return Stations;
		}
	}

	bool DrawCloth(const FFormCloth& Shape, FMeshBuilder& Mesh, FString& OutWhy)
	{
		const double A = Shape.SizeMeters.X;
		const double B = Shape.SizeMeters.Y;
		if (A <= 0.0 || B <= 0.0 || Shape.SagMeters < 0.0 || Shape.ThicknessMeters < 2.0 * WeldToleranceMeters || Shape.GridMeters <= 0.0)
		{
			OutWhy = TEXT("a cloth has a span both ways, a sag of zero or more, a thickness and a grid");
			return false;
		}
		const double Middle = MembraneShape(0.5 * A, 0.5 * B, A, B);
		const double Scale = Shape.SagMeters / Middle;
		const double Thickness = Shape.ThicknessMeters;
		auto Hang = [A, B, Scale](double X, double Y) { return -Scale * MembraneShape(X, Y, A, B); };
		const TArray<double> Xs = ClothStations(A, Shape.GridMeters);
		const TArray<double> Ys = ClothStations(B, Shape.GridMeters);

		const FPatchGrid Top = FPatchGrid::Sample(Xs, Ys, [&Hang](double X, double Y) { return FVector(X, Y, Hang(X, Y)); });
		const FPatchGrid Under = FPatchGrid::Sample(Xs, Ys, [&Hang, Thickness](double X, double Y) { return FVector(X, Y, Hang(X, Y) - Thickness); });
		Patch(Mesh, Top, [](const FVector&) { return FVector::UpVector; });
		Patch(Mesh, Under, [](const FVector&) { return -FVector::UpVector; });

		// The edges on the frame, where the cloth doesn't hang: a strip of its thickness round the four sides.
		auto Edge = [&Mesh, Thickness](const FVector2D& From, const FVector2D& To, const FVector& Out, double U0)
		{
			const double Length = FVector2D::Distance(From, To);
			const int32 P0 = Mesh.AddVertex(FVector(From.X, From.Y, 0.0), Out, FVector2D(U0, 0.0));
			const int32 P1 = Mesh.AddVertex(FVector(To.X, To.Y, 0.0), Out, FVector2D(U0 + Length, 0.0));
			const int32 Q1 = Mesh.AddVertex(FVector(To.X, To.Y, -Thickness), Out, FVector2D(U0 + Length, Thickness));
			const int32 Q0 = Mesh.AddVertex(FVector(From.X, From.Y, -Thickness), Out, FVector2D(U0, Thickness));
			Mesh.AddQuad(P0, P1, Q1, Q0);
		};
		for (int32 I = 0; I + 1 < Xs.Num(); ++I)
		{
			Edge(FVector2D(Xs[I], 0.0), FVector2D(Xs[I + 1], 0.0), -FVector::RightVector, Xs[I]);
			Edge(FVector2D(Xs[I], B), FVector2D(Xs[I + 1], B), FVector::RightVector, Xs[I]);
		}
		for (int32 J = 0; J + 1 < Ys.Num(); ++J)
		{
			Edge(FVector2D(0.0, Ys[J]), FVector2D(0.0, Ys[J + 1]), -FVector::ForwardVector, Ys[J]);
			Edge(FVector2D(A, Ys[J]), FVector2D(A, Ys[J + 1]), FVector::ForwardVector, Ys[J]);
		}
		return true;
	}
}
