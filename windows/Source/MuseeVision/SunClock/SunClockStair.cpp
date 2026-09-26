#include "SunClock/SunClockBuild.h"

/**
 * The stair (fixed): after Momo's and Bramante's, one wide helical flight round an open well. From the
 * top landing inside the drum's door (−0.15, 164°–194°) it winds down anticlockwise on the plan (west,
 * south, east) to the landing at −6, where the port opens north: 18 treads, a mid landing at −3.0,
 * 19 treads, 40 risers of 150 mm. The treads (polished marble, a rounded nosing over each riser) span
 * 4.1 m from the inner string (r 2.35) to the stair's wall (r 6.48): goings of 0.24 m at the string,
 * 0.40 m on the walking line (r 4.4), 0.59 m at the wall.
 *
 * The inner string is a closed marble band (r 2.2–2.35) 120 mm over the pitch line, flush with the
 * top landing; the underside is a smooth helicoid 400 mm under it, into the floor at the foot.
 * The visitor walks on a hidden ramp through the treads' middles (flat on the landings).
 */
namespace SunClockBuild
{
	static constexpr double StairOuter = SC::StairWall;          // 6.48
	static constexpr double NosingReach = 0.022;                  // the nosing's top edge, forward of the riser

	/** The descending direction at φ (decreasing φ): the risers face it. */
	static FVector Forward(double Phi) { return FVector(FMath::Sin(Phi), -FMath::Cos(Phi), 0.0); }

	/** Angles from A down to B (A > B) at most MaxStep apart, and every kink between them. */
	static TArray<double> StairSamples(double A, double B, double MaxStep, const TArray<double>& Kinks)
	{
		TArray<double> Out;
		const int32 N = FMath::Max(1, FMath::CeilToInt((A - B) / MaxStep));
		for (int32 I = 0; I <= N; ++I) { Out.Add(A + (B - A) * I / N); }
		for (const double K : Kinks)
		{
			if (K < A - 1e-9 && K > B + 1e-9) { Out.Add(K); }
		}
		Out.Sort([](double X, double Y) { return X > Y; });
		for (int32 I = Out.Num() - 1; I > 0; --I)
		{
			if (FMath::Abs(Out[I] - Out[I - 1]) < 1e-7) { Out.RemoveAt(I); }
		}
		return Out;
	}

	/** Splits descending samples into runs at the kinks (so each run is smooth, the corners crisp). */
	static TArray<TArray<double>> StairRuns(const TArray<double>& Samples, const TArray<double>& Kinks)
	{
		TArray<TArray<double>> Runs;
		Runs.AddDefaulted();
		for (int32 I = 0; I < Samples.Num(); ++I)
		{
			Runs.Last().Add(Samples[I]);
			const bool bKink = Kinks.ContainsByPredicate([&](double K) { return FMath::Abs(K - Samples[I]) < 1e-7; });
			if (bKink && I > 0 && I + 1 < Samples.Num())
			{
				Runs.AddDefaulted();
				Runs.Last().Add(Samples[I]);
			}
		}
		return Runs;
	}

	static void StairTreads(FSunClockMeshes& M)
	{
		const FStairLayout& L = FStairLayout::Get();
		FMeshData& T = M.Treads;
		for (int32 K = 1; K < SC::Risers; ++K)
		{
			const double Z = L.TreadZ(K), F = L.Front[K], B = L.Back(K);
			// Each runs a centimetre into the string and the wall.
			const double RIn = CurbOuter - 0.01, ROut = StairOuter + 0.01;
			const int32 N = FMath::Max(1, FMath::CeilToInt((B - F) / Deg));
			T.Patch(N, 1,
				[&](int32 I, int32 J)
				{
					const double U = double(I) / N;
					const double Phi = F + (B - F) * U;
					return Polar(J == 0 ? RIn : ROut, Phi, Z) + Forward(F) * (NosingReach * (1.0 - U));
				},
				[](const FVector& P) { return FVector2D(P.X, P.Y); }, [](const FVector&) { return FVector::UpVector; });
			// The riser (into the tread below) and the nosing, along the radial line at the front.
			FProfile Nosing;
			const double Drop = (K + 1 < SC::Risers ? L.TreadZ(K + 1) : SC::Floor) - Z;
			Nosing.Add(0.0, Drop - 0.01).Add(0.0, -0.050).Add(0.022, -0.040).Arc(0.022, -0.020, 0.020, 0.020, -90, 90, 10);
			TArray<FFrame> Run;
			for (const double R : {RIn, ROut})
			{
				FFrame Fr;
				Fr.Origin = Polar(R, F, Z);
				Fr.AxisA = Fr.NormA = Forward(F);
				Fr.AxisB = Fr.NormB = FVector::UpVector;
				Fr.S = R;
				Run.Add(Fr);
			}
			SalonKit::Sweep(T, Run, Nosing);
		}
	}

	static void StairBody(FSunClockMeshes& M)
	{
		const FStairLayout& L = FStairLayout::Get();
		const TArray<double> Kinks = L.Kinks();
		const double CurbEnd = L.End - L.Alpha / 2 - 0.5 * Deg;   // the string runs on a little past the last riser
		const TArray<double> Samples = StairSamples(L.Top, CurbEnd, 0.5 * Deg, Kinks);
		for (const TArray<double>& Run : StairRuns(Samples, Kinks))
		{
			const int32 N = Run.Num() - 1;
			if (N < 1) { continue; }
			auto UV = [](const FVector& P) { return FVector2D(P.X, P.Y); };
			// The string's top, its face to the treads and its face to the well.
			M.Curb.Patch(N, 1, [&](int32 I, int32 J) { return Polar(J == 0 ? CurbInner : CurbOuter, Run[I], L.CurbTop(Run[I])); }, UV,
						 [](const FVector&) { return FVector::UpVector; });
			M.Curb.Patch(N, 1, [&](int32 I, int32 J) { return Polar(CurbOuter, Run[I], J == 0 ? L.SoffitZ(Run[I]) : L.CurbTop(Run[I])); },
						 [](const FVector& P) { return FVector2D(P.X + P.Y, P.Z); }, [](const FVector& P) { return FVector(P.X, P.Y, 0); });
			M.Curb.Patch(N, 1, [&](int32 I, int32 J) { return Polar(CurbInner, Run[I], J == 0 ? L.SoffitZ(Run[I]) : L.CurbTop(Run[I])); },
						 [](const FVector& P) { return FVector2D(P.X + P.Y, P.Z); }, [](const FVector& P) { return FVector(-P.X, -P.Y, 0); });
			// The underside, from the well to a centimetre into the wall.
			const double Radii[5] = {CurbInner, 3.2, 4.3, 5.4, StairOuter + 0.01};
			M.StairSoffit.Patch(N, 4, [&](int32 I, int32 J) { return Polar(Radii[J], Run[I], L.SoffitZ(Run[I])); }, UV,
								[](const FVector&) { return -FVector::UpVector; });
			// The ramp the visitor walks on (hidden), down to where it meets the floor.
			TArray<double> Walk;
			for (const double Phi : Run)
			{
				if (Phi >= L.End - L.Alpha / 2 - 1e-9) { Walk.Add(Phi); }
			}
			if (Walk.Num() >= 2)
			{
				// Rows every half metre across, so no triangle strays far from the helicoid.
				constexpr int32 Rows = 9;
				M.Ramp.Patch(Walk.Num() - 1, Rows - 1,
							 [&](int32 I, int32 J) { return Polar(FMath::Lerp(CurbOuter - 0.05, StairOuter + 0.02, double(J) / (Rows - 1)), Walk[I], L.RampZ(Walk[I])); },
							 UV, [](const FVector&) { return FVector::UpVector; });
			}
		}
		// The top's end, under the landing's back edge (facing the void behind it).
		const double SoffitTop = L.SoffitZ(L.Top);
		const TArray<FVector2D> TopEnd = {FVector2D(CurbInner, SoffitTop), FVector2D(StairOuter + 0.01, SoffitTop), FVector2D(StairOuter + 0.01, L.TreadZ(1)),
										  FVector2D(CurbOuter, L.TreadZ(1)), FVector2D(CurbOuter, L.CurbTop(L.Top)), FVector2D(CurbInner, L.CurbTop(L.Top))};
		FlatPolygon(M.StairSoffit, TopEnd, [&L](const FVector2D& Q) { return Polar(Q.X, L.Top, Q.Y); }, TangentDir(L.Top));
		// The string's end on the floor at the foot.
		const TArray<FVector2D> FootEnd = {FVector2D(CurbInner, L.SoffitZ(CurbEnd)), FVector2D(CurbOuter, L.SoffitZ(CurbEnd)),
										   FVector2D(CurbOuter, L.CurbTop(CurbEnd)), FVector2D(CurbInner, L.CurbTop(CurbEnd))};
		FlatPolygon(M.Curb, FootEnd, [CurbEnd](const FVector2D& Q) { return Polar(Q.X, CurbEnd, Q.Y); }, -TangentDir(CurbEnd));
		// The underside's end past the foot is under the floor; close it there.
		const TArray<FVector2D> SoffitEnd = {FVector2D(CurbOuter, L.SoffitZ(CurbEnd)), FVector2D(StairOuter + 0.01, L.SoffitZ(CurbEnd)),
											 FVector2D(StairOuter + 0.01, SC::Floor - 0.001), FVector2D(CurbOuter, SC::Floor - 0.001)};
		FlatPolygon(M.StairSoffit, SoffitEnd, [CurbEnd](const FVector2D& Q) { return Polar(Q.X, CurbEnd, Q.Y); }, -TangentDir(CurbEnd));
	}

	void BuildStair(FSunClockMeshes& M)
	{
		StairTreads(M);
		StairBody(M);
	}
}
