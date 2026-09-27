#include "ChenghuaiHall.h"
#include "ChenghuaiPlan.h"

/**
 * The section rule (§3.1, ChenghuaiHall.cpp:129-177): the columns' top, the purlins' seats rising by the 举架 rule, the
 * eave's round and flying rafters, and the lime bed's curve, a monotone cubic through its knots so the roof never
 * overshoots between them (F. N. Fritsch and R. E. Carlson, "Monotone Piecewise Cubic Interpolation", SIAM J. Numer. Anal.
 * 17(2), 1980).
 */
namespace Musee::Chenghuai
{
	namespace
	{
		/** Knots nearer than this along v are spread apart, so the curve's abscissae keep ascending (as built). */
		constexpr double KnotSpacing = 0.010;

		/** The Fritsch-Carlson tangents of a curve through the knots. */
		TArray<double> MonotoneSlopes(TConstArrayView<double> X, TConstArrayView<double> Y)
		{
			const int32 Num = X.Num();
			TArray<double> M;
			M.SetNumZeroed(Num);
			if (Num < 2)
			{
				return M;
			}
			TArray<double> D;
			for (int32 I = 0; I + 1 < Num; ++I)
			{
				D.Add((Y[I + 1] - Y[I]) / (X[I + 1] - X[I]));
			}
			M[0] = D[0];
			M[Num - 1] = D[Num - 2];
			for (int32 I = 1; I + 1 < Num; ++I)
			{
				M[I] = D[I - 1] * D[I] <= 0.0 ? 0.0 : 0.5 * (D[I - 1] + D[I]);
			}
			for (int32 I = 0; I + 1 < Num; ++I)
			{
				if (FMath::IsNearlyZero(D[I], 1.0e-12))
				{
					M[I] = 0.0;
					M[I + 1] = 0.0;
					continue;
				}
				const double A = M[I] / D[I];
				const double B = M[I + 1] / D[I];
				const double S = A * A + B * B;
				// Tangents inside the circle of radius 3 keep the cubic monotone between the knots.
				if (S > 9.0)
				{
					const double T = 3.0 / FMath::Sqrt(S);
					M[I] = T * A * D[I];
					M[I + 1] = T * B * D[I];
				}
			}
			return M;
		}

		double Slope(const FHallSpec& Spec, const FHallSection& Section, int32 Step)
		{
			return (Section.Seat[Step + 1] - Section.Seat[Step]) / (Spec.PurlinV[Step + 1] - Spec.PurlinV[Step]);
		}
	}

	FHallSection FHallSection::Of(const FHallSpec& Spec)
	{
		FHallSection S;
		S.Spec = &Spec;
		const TArray<double>& P = Spec.PurlinV;
		const int32 N = P.Num();
		S.Zc = Spec.Floor + Spec.ColH;
		S.PurlinR = FMath::Max(Plan::PurlinRadiusLeast, Plan::PurlinPerDiameter * Spec.ColD);
		S.Seat.SetNum(N);
		S.Seat[0] = S.Zc + Plan::SeatOver + 2.0 * S.PurlinR;
		for (int32 I = 0; I + 1 < N; ++I)
		{
			S.Seat[I + 1] = S.Seat[I] + Spec.Rise[I] * (P[I + 1] - P[I]);
		}
		for (int32 I = 1; I < N; ++I)
		{
			S.RidgeI = S.Seat[I] > S.Seat[S.RidgeI] + UE_KINDA_SMALL_NUMBER ? I : S.RidgeI;
		}
		S.RidgeJ = S.RidgeI;
		if (Spec.bRolled && S.RidgeI + 1 < N && FMath::IsNearlyEqual(S.Seat[S.RidgeI + 1], S.Seat[S.RidgeI], UE_KINDA_SMALL_NUMBER))
		{
			S.RidgeJ = S.RidgeI + 1;
		}
		S.RidgeV = 0.5 * (P[S.RidgeI] + P[S.RidgeJ]);
		S.EaveF = P[0] - Spec.EaveFront;
		S.EaveR = P[0] - Spec.EaveFront * Plan::RoundRafterReach;
		S.bOpenBack = !(Spec.bSealedBack && Spec.bBackWall);
		if (!S.bOpenBack)
		{
			S.BackEdge = Spec.BackWallV1 + Spec.EaveBack;
			S.BackR = S.BackEdge;
		}
		else
		{
			S.BackEdge = P[N - 1] + Spec.EaveBack;
			S.BackR = P[N - 1] + Spec.EaveBack * Plan::RoundRafterReach;
		}

		// The bed's knots: the eave's edge, the round rafters' end, each purlin (the rolled ridge's middle), the back.
		const double Build = Plan::RafterDiameter + Plan::Boarding + Plan::LimeBed;
		const double Flying = Plan::FlyingRafter + Plan::Boarding;
		auto Knot = [&S](double V, double Z)
		{
			S.KnotV.Add(S.KnotV.IsEmpty() ? V : FMath::Max(V, S.KnotV.Last() + KnotSpacing));
			S.KnotZ.Add(Z);
		};
		Knot(S.EaveF, S.FlyUnder(S.EaveF, false) + Flying + Plan::EaveEdgeKnot);
		Knot(S.EaveR, S.FlyUnder(S.EaveR, false) + Flying + Plan::RoundEndKnot);
		for (int32 I = 0; I < N; ++I)
		{
			Knot(P[I], S.Seat[I] + Build);
			if (I == S.RidgeI && S.RidgeJ != S.RidgeI)
			{
				Knot(S.RidgeV, S.Seat[I] + Build + Plan::RolledRidgeLift);
			}
		}
		if (!S.bOpenBack)
		{
			Knot(S.BackEdge, S.Seat[N - 1] + Build + Slope(Spec, S, N - 2) * Plan::SealedBackSlope * (S.BackEdge - P[N - 1]));
		}
		else
		{
			Knot(S.BackR, S.FlyUnder(S.BackR, true) + Flying + Plan::RoundEndKnot);
			Knot(S.BackEdge, S.FlyUnder(S.BackEdge, true) + Flying + Plan::EaveEdgeKnot);
		}
		S.KnotSlope = MonotoneSlopes(S.KnotV, S.KnotZ);
		return S;
	}

	double FHallSection::Bed(double V) const
	{
		const int32 N = KnotV.Num();
		if (V <= KnotV[0])
		{
			return KnotZ[0] + KnotSlope[0] * (V - KnotV[0]);
		}
		if (V >= KnotV[N - 1])
		{
			return KnotZ[N - 1] + KnotSlope[N - 1] * (V - KnotV[N - 1]);
		}
		int32 I = 0;
		while (I < N - 2 && V > KnotV[I + 1])
		{
			++I;
		}
		// The cubic Hermite spline on [V_i, V_i+1].
		const double H = KnotV[I + 1] - KnotV[I];
		const double T = (V - KnotV[I]) / H;
		const double T2 = T * T;
		const double T3 = T2 * T;
		return (2.0 * T3 - 3.0 * T2 + 1.0) * KnotZ[I] + (T3 - 2.0 * T2 + T) * H * KnotSlope[I] + (-2.0 * T3 + 3.0 * T2) * KnotZ[I + 1]
			+ (T3 - T2) * H * KnotSlope[I + 1];
	}

	double FHallSection::RafterTop(double V) const
	{
		const TArray<double>& P = Spec->PurlinV;
		const int32 N = P.Num();
		if (V <= P[0])
		{
			return Seat[0] + Plan::RafterDiameter + Slope(*Spec, *this, 0) * (V - P[0]);
		}
		if (V >= P[N - 1])
		{
			return Seat[N - 1] + Plan::RafterDiameter + Slope(*Spec, *this, N - 2) * (V - P[N - 1]);
		}
		int32 I = 0;
		while (I < N - 2 && V > P[I + 1])
		{
			++I;
		}
		return FMath::Lerp(Seat[I], Seat[I + 1], (V - P[I]) / (P[I + 1] - P[I])) + Plan::RafterDiameter;
	}

	double FHallSection::FlyUnder(double V, bool bBack) const
	{
		const int32 N = Spec->PurlinV.Num();
		const double From = bBack ? BackR : EaveR;
		const double Pitch = bBack ? Slope(*Spec, *this, N - 2) : Slope(*Spec, *this, 0);
		return RafterTop(From) + Plan::Boarding + Pitch * Plan::FlyingRafterSlope * (V - From);
	}

	double FHallSection::Under(double V) const
	{
		if (V < EaveR)
		{
			return FlyUnder(V, false) + Plan::FlyingRafter;
		}
		if (bOpenBack && V > BackR)
		{
			return FlyUnder(V, true) + Plan::FlyingRafter;
		}
		return RafterTop(V);
	}

	double FHallSection::InnerColumnTop(int32 I) const
	{
		return Seat[I] - Plan::SeatOver - 2.0 * PurlinR - Plan::InnerColumnGap;
	}

	int32 FHallSection::PurlinAt(double V) const
	{
		constexpr double Near = 0.020;
		return Spec->PurlinV.IndexOfByPredicate([V](double P) { return FMath::Abs(P - V) < Near; });
	}

	void RoofSpan(const FHallSpec& Spec, double V, double& OutA, double& OutB)
	{
		OutA = Spec.bRoofSpan ? Spec.RoofU0 : Spec.U0;
		OutB = Spec.bRoofSpan ? Spec.RoofU1 : Spec.U1;
		OutA += Spec.MitreAtU0 * (V - Spec.MitreRefV);
		OutB += Spec.MitreAtU1 * (V - Spec.MitreRefV);
	}

	FHallSpec MirroredAbout(const FHallSpec& Spec, double Axis, const FString& Name, const FString& Room)
	{
		FHallSpec Mirror = Spec;
		Mirror.Name = Name;
		Mirror.Room = Room;
		Mirror.Frame.Origin.X = 2.0 * Axis - Spec.Frame.Origin.X;
		Mirror.Frame.U.X = -Spec.Frame.U.X;
		Mirror.Frame.V.X = -Spec.Frame.V.X;
		return Mirror;
	}
}
