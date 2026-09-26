#pragma once

#include "CoreMinimal.h"
#include "Salon/SalonKit.h"

/**
 * Geometry kit for the museum's furniture and thresholds (AMuseeFurniture): solids with eased edges, swept along a
 * path. Positions are plan metres (x east, y south, z up), written in centimetres by SalonKit::FMeshData; UVs are
 * metres, U along the path (the grain of a timber, the brushing of a bronze strip), V round the section.
 *
 * A bar is a section (a rounded rectangle in its frame's A, B plane) carried along a path; each end can be rounded
 * over (the section shrinks as it reaches the end, a quarter circle of radius E) and capped. Vertex normals are the
 * area-weighted average of the faces round them, so a flat face stays flat right up to where its rounding starts
 * and the rounding shades as a round: nothing is a razor edge, and nothing is a smoothed blob.
 */
namespace FurnitureKit
{
	using SalonKit::FMeshData;

	constexpr double kPi = UE_DOUBLE_PI;

	/**
	 * A rounded rectangle about (CA, CB), half sizes HA, HB, corner radii RBottom (the two corners at −B) and RTop,
	 * Seg segments per corner and Sub segments per straight side (so every ring has the same count). Crown lifts
	 * the top side by up to Crown at A = CA (a cushion's dome), fading to nothing at its corners. Counter-clockwise.
	 */
	inline TArray<FVector2D> RoundRect(double CA, double CB, double HA, double HB, double RBottom, double RTop, int32 Seg, int32 Sub,
									   double Crown = 0.0)
	{
		const double MinR = 0.0004;
		RBottom = FMath::Clamp(RBottom, MinR, FMath::Min(HA, HB) - 1e-5);
		RTop = FMath::Clamp(RTop, MinR, FMath::Min(HA, HB) - 1e-5);
		struct FCorner { double SA, SB, R, T0; };
		const FCorner Corners[4] = {{1, -1, RBottom, -90.0}, {1, 1, RTop, 0.0}, {-1, 1, RTop, 90.0}, {-1, -1, RBottom, 180.0}};
		TArray<FVector2D> Out;
		for (int32 c = 0; c < 4; ++c)
		{
			const FCorner& K = Corners[c];
			const FVector2D Centre(CA + K.SA * (HA - K.R), CB + K.SB * (HB - K.R));
			for (int32 i = 0; i <= Seg; ++i)
			{
				const double T = FMath::DegreesToRadians(K.T0 + 90.0 * i / Seg);
				Out.Add(Centre + FVector2D(FMath::Cos(T), FMath::Sin(T)) * K.R);
			}
			// The straight side to the next corner's first point.
			const FCorner& N = Corners[(c + 1) % 4];
			const FVector2D From = Out.Last();
			const double TN = FMath::DegreesToRadians(N.T0);
			const FVector2D To = FVector2D(CA + N.SA * (HA - N.R), CB + N.SB * (HB - N.R)) + FVector2D(FMath::Cos(TN), FMath::Sin(TN)) * N.R;
			for (int32 k = 1; k < Sub; ++k) { Out.Add(FMath::Lerp(From, To, double(k) / Sub)); }
		}
		if (Crown != 0.0)
		{
			for (FVector2D& P : Out)
			{
				const double U = (P.X - CA) / HA;
				const double W = FMath::Clamp((P.Y - CB) / HB, 0.0, 1.0);
				P.Y += Crown * FMath::Max(0.0, 1.0 - U * U) * W * W * W * W;
			}
		}
		return Out;
	}

	/** A circle of radius R about (CA, CB), N points, counter-clockwise. */
	inline TArray<FVector2D> Circle(double CA, double CB, double R, int32 N)
	{
		TArray<FVector2D> Out;
		for (int32 i = 0; i < N; ++i)
		{
			const double T = 2.0 * kPi * i / N;
			Out.Add(FVector2D(CA + R * FMath::Cos(T), CB + R * FMath::Sin(T)));
		}
		return Out;
	}

	/** A frame on a path: the section's (A, B) plane at Origin; T along the path; S its distance along it (m). */
	struct FFrame
	{
		FVector Origin = FVector::ZeroVector;
		FVector T = FVector::ForwardVector;
		FVector A = FVector::RightVector;
		FVector B = FVector::UpVector;
		double S = 0.0;
	};

	/**
	 * A polyline path with smooth frames: B is UpHint made square to the path, A = B × T (for a level path with
	 * UpHint up, A is the path's left normal, as SalonKit's sweeps).
	 */
	struct FPath
	{
		TArray<FVector> P;
		TArray<double> S;
		TArray<FVector> Tan;
		FVector UpHint = FVector::UpVector;
		bool bClosed = false;

		FPath() = default;
		FPath(const TArray<FVector>& Points, const FVector& InUp = FVector::UpVector, bool bInClosed = false)
			: P(Points), UpHint(InUp), bClosed(bInClosed)
		{
			const int32 N = P.Num();
			S.Add(0.0);
			for (int32 i = 1; i < N; ++i) { S.Add(S.Last() + FVector::Distance(P[i - 1], P[i])); }
			for (int32 i = 0; i < N; ++i)
			{
				FVector In = FVector::ZeroVector, Out = FVector::ZeroVector;
				if (i > 0) { In = (P[i] - P[i - 1]).GetSafeNormal(); }
				else if (bClosed) { In = (P[0] - P[N - 1]).GetSafeNormal(); }
				if (i + 1 < N) { Out = (P[i + 1] - P[i]).GetSafeNormal(); }
				else if (bClosed) { Out = (P[0] - P[N - 1]).GetSafeNormal(); }
				Tan.Add((In + Out).GetSafeNormal());
			}
		}

		static FPath Line(const FVector& A, const FVector& B, const FVector& Up = FVector::UpVector) { return FPath({A, B}, Up); }

		double Length() const { return S.Num() ? S.Last() : 0.0; }

		FFrame Make(const FVector& Origin, const FVector& Tangent, double Dist) const
		{
			FFrame F;
			F.Origin = Origin;
			F.T = Tangent;
			F.B = (UpHint - Tangent * FVector::DotProduct(UpHint, Tangent)).GetSafeNormal();
			F.A = FVector::CrossProduct(F.B, F.T).GetSafeNormal();
			F.S = Dist;
			return F;
		}

		/** The frame at distance At along the path (clamped; beyond the ends, straight on). */
		FFrame At(double Dist) const
		{
			const int32 N = P.Num();
			if (Dist <= 0.0) { return Make(P[0] + Tan[0] * Dist, Tan[0], Dist); }
			if (Dist >= Length()) { return Make(P[N - 1] + Tan[N - 1] * (Dist - Length()), Tan[N - 1], Dist); }
			int32 i = 0;
			while (i + 2 < N && S[i + 1] < Dist) { ++i; }
			const double L = FMath::Max(1e-12, S[i + 1] - S[i]);
			const double K = (Dist - S[i]) / L;
			return Make(FMath::Lerp(P[i], P[i + 1], K), FMath::Lerp(Tan[i], Tan[i + 1], K).GetSafeNormal(), Dist);
		}
	};

	/** The section at a station: its distance along the path and how far it is shrunk (an end's rounding). */
	using FSectionFn = TFunction<TArray<FVector2D>(double S, double Inset)>;

	/** Options for a bar. */
	struct FBarEnds
	{
		double E0 = 0.0, E1 = 0.0;        // the start's and end's rounding radii (0: square)
		int32 RoundSteps = 4;             // stations over each rounding
		bool bCap0 = true, bCap1 = true;  // close the ends (a closed path has none)
	};

	/**
	 * Rings (each the same number of points, closed round) joined into a smooth surface; normals area-weighted from the
	 * faces, each face turned away from its ring's centroid (sections are convex, or nearly). U: the rings' distances.
	 */
	inline void Rings(FMeshData& M, const TArray<TArray<FVector>>& R, const TArray<double>& U, bool bClosedPath, bool bCap0, bool bCap1)
	{
		const int32 NR = R.Num();
		if (NR < 2) { return; }
		const int32 NP = R[0].Num();
		TArray<FVector> Centroid;
		for (const TArray<FVector>& Ring : R)
		{
			FVector C = FVector::ZeroVector;
			for (const FVector& P : Ring) { C += P; }
			Centroid.Add(C / FMath::Max(1, Ring.Num()));
		}
		const int32 NI = bClosedPath ? NR : NR - 1;
		TArray<FVector> Nrm;
		Nrm.Init(FVector::ZeroVector, NR * NP);
		auto Idx = [NP](int32 i, int32 j) { return i * NP + j; };
		for (int32 i = 0; i < NI; ++i)
		{
			const int32 i1 = (i + 1) % NR;
			for (int32 j = 0; j < NP; ++j)
			{
				const int32 j1 = (j + 1) % NP;
				const FVector& A = R[i][j];
				const FVector& B = R[i1][j];
				const FVector& C = R[i1][j1];
				const FVector& D = R[i][j1];
				FVector N = FVector::CrossProduct(C - A, D - B);   // twice the quad's area along its normal
				const FVector Mid = (A + B + C + D) * 0.25;
				if (FVector::DotProduct(N, Mid - (Centroid[i] + Centroid[i1]) * 0.5) < 0.0) { N = -N; }
				Nrm[Idx(i, j)] += N;
				Nrm[Idx(i1, j)] += N;
				Nrm[Idx(i1, j1)] += N;
				Nrm[Idx(i, j1)] += N;
			}
		}
		const int32 Base = M.Positions.Num();
		for (int32 i = 0; i < NR; ++i)
		{
			double V = 0.0;
			for (int32 j = 0; j < NP; ++j)
			{
				if (j > 0) { V += FVector::Distance(R[i][j - 1], R[i][j]); }
				FVector N = Nrm[Idx(i, j)];
				if (!N.Normalize(1e-20)) { N = (R[i][j] - Centroid[i]).GetSafeNormal(); }
				M.Vertex(R[i][j], N, FVector2D(U[i], V));
			}
			// The seam: the ring's first point again, at the ring's full length (so V runs on without wrapping back).
			V += FVector::Distance(R[i][NP - 1], R[i][0]);
			FVector N0 = Nrm[Idx(i, 0)];
			if (!N0.Normalize(1e-20)) { N0 = (R[i][0] - Centroid[i]).GetSafeNormal(); }
			M.Vertex(R[i][0], N0, FVector2D(U[i], V));
		}
		const int32 W = NP + 1;
		for (int32 i = 0; i < NI; ++i)
		{
			const int32 i1 = (i + 1) % NR;
			for (int32 j = 0; j < NP; ++j)
			{
				M.Quad(Base + i * W + j, Base + i1 * W + j, Base + i1 * W + j + 1, Base + i * W + j + 1);
			}
		}
		if (bClosedPath) { return; }
		auto Cap = [&](int32 i, const FVector& Facing)
		{
			// Planar UVs in the cap's own plane (metres).
			const FVector Ax = FVector::CrossProduct(Facing, FMath::Abs(Facing.Z) < 0.9 ? FVector::UpVector : FVector::ForwardVector).GetSafeNormal();
			const FVector Bx = FVector::CrossProduct(Facing, Ax);
			auto UvOf = [&](const FVector& P) { return FVector2D(FVector::DotProduct(P, Ax), FVector::DotProduct(P, Bx)); };
			const int32 Hub = M.Vertex(Centroid[i], Facing, UvOf(Centroid[i]));
			const int32 First = M.Positions.Num();
			for (int32 j = 0; j < NP; ++j) { M.Vertex(R[i][j], Facing, UvOf(R[i][j])); }
			for (int32 j = 0; j < NP; ++j) { M.Tri(Hub, First + j, First + (j + 1) % NP); }
		};
		if (bCap0) { Cap(0, (Centroid[0] - Centroid[1]).GetSafeNormal()); }
		if (bCap1) { Cap(NR - 1, (Centroid[NR - 1] - Centroid[NR - 2]).GetSafeNormal()); }
	}

	/**
	 * A bar: Section carried along Path at Stations (distances along it; the path's own vertices if empty), each end
	 * rounded over by Ends.E0 / E1 (the section shrinks by E(1 − cos θ) as the end comes on by E sin θ).
	 */
	inline void Bar(FMeshData& M, const FPath& Path, const FSectionFn& Section, const FBarEnds& Ends = FBarEnds(),
					TArray<double> Stations = TArray<double>())
	{
		const double L = Path.Length();
		if (L <= 1e-6) { return; }
		if (Stations.Num() == 0) { Stations = Path.S; }
		struct FStation { double S, Inset; };
		TArray<FStation> St;
		const int32 K = FMath::Max(1, Ends.RoundSteps);
		if (!Path.bClosed && Ends.E0 > 0.0)
		{
			for (int32 k = K; k >= 1; --k)
			{
				const double Th = 0.5 * kPi * k / K;
				St.Add({Ends.E0 * (1.0 - FMath::Sin(Th)), Ends.E0 * (1.0 - FMath::Cos(Th))});
			}
		}
		const double Lo = Path.bClosed ? 0.0 : Ends.E0, Hi = Path.bClosed ? L : L - Ends.E1;
		St.Add({Lo, 0.0});
		for (const double S : Stations)
		{
			if (S > Lo + 1e-9 && S < Hi - 1e-9) { St.Add({S, 0.0}); }
		}
		St.Add({Hi, 0.0});
		if (!Path.bClosed && Ends.E1 > 0.0)
		{
			for (int32 k = 1; k <= K; ++k)
			{
				const double Th = 0.5 * kPi * k / K;
				St.Add({L - Ends.E1 * (1.0 - FMath::Sin(Th)), Ends.E1 * (1.0 - FMath::Cos(Th))});
			}
		}
		// Drop repeats (a station on a rounding's start).
		TArray<FStation> Clean;
		for (const FStation& X : St)
		{
			if (Clean.Num() && FMath::Abs(Clean.Last().S - X.S) < 1e-7 && FMath::Abs(Clean.Last().Inset - X.Inset) < 1e-7) { continue; }
			Clean.Add(X);
		}
		if (Path.bClosed && Clean.Num() > 1 && FMath::Abs(Clean.Last().S - L) < 1e-7) { Clean.RemoveAt(Clean.Num() - 1); }
		TArray<TArray<FVector>> R;
		TArray<double> U;
		// Each member starts its U at its own multiple of 40 m (from where it stands): the oak's material takes a board's
		// slice and tone per 40 m of U, so every leg, rail and slab is its own piece of timber (M_Wood BoardLength).
		const FVector P0 = Path.At(0.0).Origin;
		const double MemberU = 40.0 * (FMath::Abs(FMath::FloorToInt(P0.X * 97.0 + P0.Y * 57.0 + P0.Z * 31.0)) % 29);
		for (const FStation& X : Clean)
		{
			const FFrame F = Path.At(X.S);
			TArray<FVector> Ring;
			for (const FVector2D& Q : Section(X.S, X.Inset)) { Ring.Add(F.Origin + F.A * Q.X + F.B * Q.Y); }
			R.Add(MoveTemp(Ring));
			U.Add(X.S + MemberU);
		}
		Rings(M, R, U, Path.bClosed, Ends.bCap0, Ends.bCap1);
	}

	/** A plain rounded-rectangle section (half sizes HA, HB about (CA, CB), radius Rad), shrunk by an end's rounding. */
	inline FSectionFn RectSection(double CA, double CB, double HA, double HB, double Rad, int32 Seg = 4, int32 Sub = 1)
	{
		return [=](double, double Inset)
		{
			return RoundRect(CA, CB, HA - Inset, HB - Inset, Rad - Inset, Rad - Inset, Seg, Sub);
		};
	}

	/** A closed prism (hit box): the rectangle A0…A1 × B0…B1 carried along a path, capped. */
	inline void Prism(FMeshData& M, const FPath& Path, double A0, double A1, double B0, double B1)
	{
		Bar(M, Path, [=](double, double)
		{
			return TArray<FVector2D>({FVector2D(A1, B0), FVector2D(A1, B1), FVector2D(A0, B1), FVector2D(A0, B0)});
		});
	}

	/** Points on a level arc about C (plan), radius R, angles A0…A1 (radians, from east towards south), at height Z. */
	inline TArray<FVector> Arc(const FVector2D& C, double R, double A0, double A1, double Z, int32 N)
	{
		TArray<FVector> Out;
		for (int32 i = 0; i <= N; ++i)
		{
			const double A = FMath::Lerp(A0, A1, double(i) / N);
			Out.Add(FVector(C.X + R * FMath::Cos(A), C.Y + R * FMath::Sin(A), Z));
		}
		return Out;
	}

	/** A plan polyline (metres) at height Z. */
	inline TArray<FVector> AtHeight(const TArray<FVector2D>& Pts, double Z)
	{
		TArray<FVector> Out;
		for (const FVector2D& P : Pts) { Out.Add(FVector(P.X, P.Y, Z)); }
		return Out;
	}

	/** A Catmull-Rom curve through Pts, Sub points per span (as ASalonStructure's). */
	inline TArray<FVector2D> CatmullRom(const TArray<FVector2D>& Pts, int32 Sub)
	{
		TArray<FVector2D> Result;
		const int32 N = Pts.Num();
		auto P = [&](int32 i) -> FVector2D
		{
			if (i < 0) { return Pts[0] * 2.0 - Pts[1]; }
			if (i >= N) { return Pts[N - 1] * 2.0 - Pts[N - 2]; }
			return Pts[i];
		};
		for (int32 i = 0; i + 1 < N; ++i)
		{
			const FVector2D A = P(i - 1), B = P(i), C = P(i + 1), D = P(i + 2);
			for (int32 s = 0; s < Sub; ++s)
			{
				const double T = double(s) / Sub, T2 = T * T, T3 = T2 * T;
				Result.Add((B * 2.0 + (C - A) * T + (A * 2.0 - B * 5.0 + C * 4.0 - D) * T2 + (B * 3.0 - A - C * 3.0 + D) * T3) * 0.5);
			}
		}
		Result.Add(Pts.Last());
		return Result;
	}
}
