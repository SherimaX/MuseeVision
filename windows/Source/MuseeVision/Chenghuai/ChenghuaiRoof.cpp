#include "Chenghuai/ChenghuaiRoof.h"

namespace ChenghuaiRoof
{
	namespace Kit = ChenghuaiKit;
	using Kit::FMeshData;

	namespace ChenghuaiRoofImpl
	{
		constexpr double LipRun = 0.004;
		constexpr double UpperSlack = 0.012;   // an upper tile runs this far past the lower ones at a cut, so their ends never share a plane

		/** A tile's section: (across, height over the bed), counter-clockwise; which points step with the courses. */
		struct FTileShape
		{
			TArray<FVector2D> P;
			TArray<bool> bTop;
			TArray<bool> bSmooth;
		};

		/** A lower pan (concave, laid in the bed), its edges 3 cm up. */
		FTileShape PanShape(double Half)
		{
			FTileShape T;
			T.P = {FVector2D(-Half, -0.01), FVector2D(Half, -0.01)};
			T.bTop = {false, false};
			T.bSmooth = {false, false};
			constexpr int32 N = 4;
			for (int32 k = 0; k <= N; ++k)
			{
				const double O = Half * (1.0 - 2.0 * k / N);
				T.P.Add(FVector2D(O, 0.02 + 0.03 * FMath::Square(O / Half)));
				T.bTop.Add(true);
				T.bSmooth.Add(k > 0 && k < N);
			}
			return T;
		}

		/** A half-round cover (筒瓦) over the joint between two pans. */
		FTileShape CoverShape(double Half)
		{
			FTileShape T;
			T.P = {FVector2D(-Half, 0.03), FVector2D(Half, 0.03)};
			T.bTop = {false, false};
			T.bSmooth = {false, false};
			constexpr int32 N = 6;
			for (int32 k = 0; k <= N; ++k)
			{
				const double O = Half * (1.0 - 2.0 * k / N);
				T.P.Add(FVector2D(O, 0.045 + 0.05 * FMath::Sqrt(FMath::Max(0.0, 1.0 - FMath::Square(O / Half)))));
				T.bTop.Add(true);
				T.bSmooth.Add(k > 0 && k < N);
			}
			return T;
		}

		/** An upper pan of a butterfly roof (合瓦), face down over the joint: a flat arch resting on the pans' edges. */
		FTileShape UpperPanShape(double Half)
		{
			FTileShape T;
			T.P = {FVector2D(-Half, 0.035), FVector2D(Half, 0.035)};
			T.bTop = {false, false};
			T.bSmooth = {false, false};
			constexpr int32 N = 4;
			for (int32 k = 0; k <= N; ++k)
			{
				const double O = Half * (1.0 - 2.0 * k / N);
				T.P.Add(FVector2D(O, 0.05 + 0.035 * (1.0 - FMath::Square(O / Half))));
				T.bTop.Add(true);
				T.bSmooth.Add(k > 0 && k < N);
			}
			return T;
		}
	}

	using namespace ChenghuaiRoofImpl;

	void TileRow(FPart& Part, const FSlope& Slope, double Centre, bool bUpper, const FTileStyle& Style, double DFrom, double DTo)
	{
		const FTileShape Shape = !bUpper ? PanShape(Style.PanHalf) : (Style.Kind == ETiles::Tong ? CoverShape(Style.TopHalf) : UpperPanShape(Style.TopHalf));
		const double CourseLength = bUpper ? Style.TopCourse : Style.PanCourse;
		const double LipRise = Style.Lip;
		const int32 NP = Shape.P.Num();
		const double Slack = bUpper ? UpperSlack : 0.0;
		const double From = DFrom - Slack, To = DTo + Slack;
		TArray<double> Lo, Hi;
		TArray<FVector2D> LoCut, HiCut;
		double Longest = 0.0;
		for (int32 j = 0; j < NP; ++j)
		{
			double L = From, H = To;
			FVector2D CL = FVector2D::ZeroVector, CH = FVector2D::ZeroVector;
			const FVector2D P0 = Slope.Origin + Slope.SDir * (Centre + Shape.P[j].X);
			for (const FVector& Lim : Slope.Limits)
			{
				const FVector2D C(Lim.X, Lim.Y);
				const double CD = FVector2D::DotProduct(C, Slope.DDir), Rhs = Lim.Z + Slack * C.Size() - FVector2D::DotProduct(C, P0);
				if (CD > 1e-12 && Rhs / CD < H) { H = Rhs / CD; CH = C; }
				else if (CD < -1e-12 && Rhs / CD > L) { L = Rhs / CD; CL = C; }
				else if (FMath::Abs(CD) <= 1e-12 && Rhs < 0.0) { H = L - 1.0; }
			}
			if (H < L) { L = H = FMath::Clamp(H, From, To); }
			Lo.Add(L);
			Hi.Add(H);
			LoCut.Add(CL);
			HiCut.Add(CH);
			Longest = FMath::Max(Longest, H - L);
		}
		if (Longest < 0.06) { return; }

		// Length along the row's centre line, for the courses and the V of the UVs.
		const int32 NA = FMath::Max(8, FMath::CeilToInt32((To - From) / 0.02));
		TArray<double> AD, AL;
		for (int32 a = 0; a <= NA; ++a)
		{
			const double D = From + (To - From) * a / NA;
			AD.Add(D);
			AL.Add(a == 0 ? 0.0 : AL.Last() + FVector2D(D - AD[a - 1], Slope.Bed(D, Centre) - Slope.Bed(AD[a - 1], Centre)).Size());
		}
		auto ArcAt = [&](double D)
		{
			const double F = FMath::Clamp((D - From) / (To - From) * NA, 0.0, double(NA));
			const int32 I = FMath::Min(FMath::FloorToInt32(F), NA - 1);
			return FMath::Lerp(AL[I], AL[I + 1], F - I);
		};
		auto DAt = [&](double Len)
		{
			int32 I = 0;
			while (I < NA - 1 && AL[I + 1] < Len) { ++I; }
			const double Span = AL[I + 1] - AL[I];
			return FMath::Lerp(AD[I], AD[I + 1], Span > 1e-12 ? FMath::Clamp((Len - AL[I]) / Span, 0.0, 1.0) : 0.0);
		};
		// Courses are counted from the eave up (the last course is whole at the eave, as a roofer lays them).
		const double Total = AL.Last();
		const double Phase = FMath::Fmod(Total, CourseLength);
		auto Step = [&](double D)
		{
			const double Phi = FMath::Fmod(FMath::Max(ArcAt(D) - Phase + CourseLength, 0.0), CourseLength);
			return Phi < CourseLength - LipRun ? LipRise * Phi / (CourseLength - LipRun) : LipRise * (CourseLength - Phi) / LipRun;
		};

		struct FStation
		{
			double D;
			bool bLip;
		};
		TArray<FStation> Stations;
		Stations.Add({From, false});
		for (int32 m = 1; Phase + m * CourseLength < Total - 0.02; ++m)
		{
			const double At = Phase + m * CourseLength;
			if (At < 0.02) { continue; }
			const double B0 = DAt(At - LipRun), B1 = DAt(At);
			Stations.Add({B0, false});
			Stations.Add({B0, true});
			Stations.Add({B1, true});
			Stations.Add({B1, false});
			for (int32 j = 0; j < NP; ++j)
			{
				if (Hi[j] > B0 && Hi[j] < B1) { Hi[j] = B0; }
				if (Lo[j] > B0 && Lo[j] < B1) { Lo[j] = B1; }
			}
		}
		Stations.Add({To, false});
		TArray<int32> Plain;
		for (int32 k = 0; k < Stations.Num(); ++k)
		{
			if (!Stations[k].bLip && (k == 0 || Stations[k].D > Stations[Plain.Last()].D + 1e-9)) { Plain.Add(k); }
			else if (Stations[k].bLip && k > 0 && Stations[k - 1].bLip) { Plain.Add(k); }
		}

		const FVector D3 = Slope.D3(), S3 = Slope.S3();
		const FVector LipN = (D3 + FVector(0, 0, 0.3)).GetSafeNormal();
		auto DOf = [&](int32 J, int32 K) { return FMath::Clamp(Stations[K].D, Lo[J], Hi[J]); };
		auto PointAt = [&](int32 J, int32 K)
		{
			const double D = DOf(J, K);
			return Slope.At(D, Centre + Shape.P[J].X, Shape.P[J].Y + (Shape.bTop[J] ? Step(D) : 0.0));
		};
		TArray<FVector2D> SegN;
		for (int32 e = 0; e < NP; ++e)
		{
			const FVector2D Dir = Shape.P[(e + 1) % NP] - Shape.P[e];
			SegN.Add(FVector2D(Dir.Y, -Dir.X).GetSafeNormal());
		}
		FMeshData& M = Part.M;
		// Each row its own slice of the tile texture (U across, V down), so no two rows repeat.
		const double RowU = 17.0 * Kit::Hash01(FMath::RoundToInt32(Centre * 100.0), bUpper ? 3 : 7);
		Part.Begin(bUpper ? TEXT("Upper tile row") : TEXT("Pan row"));
		for (int32 e = 0; e < NP; ++e)
		{
			const int32 J0 = e, J1 = (e + 1) % NP;
			const FVector2D N0 = Shape.bSmooth[J0] ? (SegN[(e + NP - 1) % NP] + SegN[e]).GetSafeNormal() : SegN[e];
			const FVector2D N1 = Shape.bSmooth[J1] ? (SegN[e] + SegN[(e + 1) % NP]).GetSafeNormal() : SegN[e];
			const bool bTopSeg = Shape.bTop[J0] && Shape.bTop[J1];
			auto NormalAt = [&](int32 J, int32 K, const FVector2D& N2)
			{
				if (bTopSeg && Stations[K].bLip) { return LipN; }
				const FVector SN = Slope.Normal(DOf(J, K), Centre + Shape.P[J].X);
				return (S3 * N2.X + SN * N2.Y).GetSafeNormal();
			};
			TArray<int32> Ks;
			if (bTopSeg) { for (int32 k = 0; k < Stations.Num(); ++k) { Ks.Add(k); } }
			else { Ks = Plain; }
			const int32 Base = M.Positions.Num();
			for (const int32 K : Ks)
			{
				M.Vertex(PointAt(J0, K), NormalAt(J0, K, N0), FVector2D(RowU + Shape.P[J0].X, ArcAt(DOf(J0, K))));
				M.Vertex(PointAt(J1, K), NormalAt(J1, K, N1), FVector2D(RowU + Shape.P[J1].X, ArcAt(DOf(J1, K))));
			}
			for (int32 k = 0; k + 1 < Ks.Num(); ++k)
			{
				const int32 V = Base + 2 * k;
				M.Quad(V, V + 1, V + 3, V + 2);
			}
		}
		auto Cap = [&](int32 K, bool bEnd)
		{
			FVector N = bEnd ? D3 : -D3;
			for (int32 j = 0; j < NP; ++j)
			{
				const FVector2D& Cut = bEnd ? HiCut[j] : LoCut[j];
				if (!Cut.IsNearlyZero()) { N = FVector(Cut.X, Cut.Y, 0.0).GetSafeNormal(); break; }
			}
			const FVector Across = FVector::CrossProduct(N, Kit::Zenith).GetSafeNormal();
			const FVector P0 = PointAt(0, K);
			TArray<FVector2D> Flat;
			const int32 Base = M.Positions.Num();
			for (int32 j = 0; j < NP; ++j)
			{
				const FVector P = PointAt(j, K);
				Flat.Add(FVector2D(FVector::DotProduct(P - P0, Across), P.Z));
				M.Vertex(P, N, SalonKit::FaceUV(P, N));
			}
			const TArray<int32> Tris = Kit::EarClip(Flat);
			for (int32 t = 0; t + 2 < Tris.Num(); t += 3) { M.Tri(Base + Tris[t], Base + Tris[t + 1], Base + Tris[t + 2]); }
		};
		Cap(0, false);
		Cap(Stations.Num() - 1, true);

		// At the eave: a drip tile under a lower pan; a round end tile on a cover; an edge tile on an upper pan.
		bool bReachesEave = Slope.bOrnaments && DTo >= Slope.To - 1e-6;
		for (int32 j = 0; j < NP && bReachesEave; ++j) { bReachesEave = HiCut[j].IsNearlyZero() && Hi[j] >= To - 1e-9; }
		if (!bReachesEave) { return; }
		const double Tilt = Slope.CrossSlope(Slope.To, Centre);
		const FVector AcrossDir = (S3 + Kit::Zenith * Tilt).GetSafeNormal();
		if (!bUpper)
		{
			const double Wider = 1.0 + 0.005 / Style.PanHalf;
			TArray<FVector2D> Drip;
			for (int32 j = 0; j < NP; ++j) { if (Shape.bTop[j]) { Drip.Add(FVector2D(Shape.P[j].X * Wider, Shape.P[j].Y + 0.004)); } }
			const double HL = Drip.Last().Y, HR = Drip[0].Y, W = Style.PanHalf * Wider;
			Drip.Append({FVector2D(-W, HL - 0.038), FVector2D(-0.45 * W, -0.035), FVector2D(0.0, -0.08), FVector2D(0.45 * W, -0.035), FVector2D(W, HR - 0.038)});
			Kit::Extrude(Part, TEXT("Drip tile"), Drip, Slope.At(Slope.To, Centre, 0.0), AcrossDir, Kit::Zenith, D3, -0.004, 0.010);
		}
		else if (Style.Kind == ETiles::Tong)
		{
			FProfile Disc;
			const double R = Style.TopHalf + 0.004;
			Disc.Add(0.0, 0.0).Add(R, 0.0).Add(R, 0.016).Add(0.0, 0.016);
			Kit::Lathe(Part, TEXT("End tile"), Slope.At(Slope.To + 0.006, Centre, 0.045), D3, Disc, 16, R);
		}
		else
		{
			// 花边瓦: the upper pan's end turned down into a scalloped lip in front of the drips.
			const double W = Style.TopHalf + 0.004;
			TArray<FVector2D> Edge;
			constexpr int32 N = 4;
			for (int32 k = 0; k <= N; ++k)
			{
				const double O = W * (1.0 - 2.0 * k / N);
				Edge.Add(FVector2D(O, 0.055 + 0.035 * (1.0 - FMath::Square(O / W))));
			}
			Edge.Append({FVector2D(-W, 0.005), FVector2D(-0.5 * W, 0.0), FVector2D(0.0, 0.012), FVector2D(0.5 * W, 0.0), FVector2D(W, 0.005)});
			Kit::Extrude(Part, TEXT("Edge tile"), Edge, Slope.At(Slope.To, Centre, 0.0), AcrossDir, Kit::Zenith, D3, 0.0, 0.014);
		}
	}

	void TileRows(FPart& Part, const FSlope& Slope, double SA, double SB, const FTileStyle& Style, double Offset)
	{
		const double P = Style.Pitch;
		for (int32 k = FMath::FloorToInt32((SA - Offset) / P) - 1; k <= FMath::CeilToInt32((SB - Offset) / P) + 1; ++k)
		{
			const double Base = Offset + k * P;
			// A lower pan centred on k·P, an upper tile over the joint half a pitch on.
			// Only whole rows: the roof's ends (a gable's edge tiles, a hip) cover the margin.
			if (Base - Style.PanHalf >= SA - 0.01 && Base + Style.PanHalf <= SB + 0.01) { TileRow(Part, Slope, Base, false, Style, Slope.From, Slope.To); }
			const double Up = Base + 0.5 * P;
			if (Up - Style.TopHalf >= SA - 0.01 && Up + Style.TopHalf <= SB + 0.01) { TileRow(Part, Slope, Up, true, Style, Slope.From, Slope.To); }
		}
	}
}
