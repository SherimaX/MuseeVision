#include "Chenghuai/ChenghuaiHall.h"

#include "Chenghuai/ChenghuaiPaint.h"

namespace ChenghuaiHall
{
	/** The rafters and their boarding: the red timber's darkened ceiling red (RafterRed), else the timber's own finish. */
	inline int32 RafterPartOf(const FHallSpec& H) { return H.TimberPart == ChenghuaiBuild::RedLacquer ? int32(ChenghuaiBuild::RafterRed) : H.TimberPart; }

	namespace Kit = ChenghuaiKit;
	namespace CB = ChenghuaiBuild;
	using Kit::FMeshData;
	using Kit::FPart;
	using Kit::FProfile;
	using Kit::FFrame;

	// ------------------------------------------------------------------------------------------------ proportions
	namespace HallDims
	{
		constexpr double SeatOver = 0.12;        // the eave purlin's seat above the column's top, plus its diameter
		constexpr double RafterD = 0.095;        // round rafters (檐椽)
		constexpr double RafterPitch = 0.22;     // on centres (一椽一当: a rafter and a rafter's width between)
		constexpr double FlyD = 0.085;           // square flying rafters (飞椽)
		constexpr double Board = 0.02;           // the boarding (望板)
		constexpr double Build = 0.14;           // the lime and mud bed on the boarding, under the tiles
		constexpr double EaveRatio = 0.66;       // the round rafters reach two thirds of the eave, the flying ones the rest
		constexpr double FlyFlatten = 0.55;      // the flying rafters lie flatter than the round ones (the eave's lift)
		constexpr double KerbW = 0.34, KerbT = 0.13, FootOut = 0.05, FootT = 0.12;
		constexpr double BaseCourseH = 1.0;      // the walls' rubbed-brick base (下碱)
		constexpr double Plaster = 0.015;
		constexpr double SillH = 0.85;           // the windows' sill walls (槛墙)
		constexpr double DoorHead = 2.55;        // the door frames' head rail (中槛), above the floor
	}
	using namespace HallDims;

	// ------------------------------------------------------------------------------------------------ the section

	namespace HallCurve
	{
		/** A monotone cubic (Fritsch-Carlson) through knots (x ascending). */
		struct FCurve
		{
			TArray<double> X, Y, M;
			void Build()
			{
				const int32 N = X.Num();
				M.SetNumZeroed(N);
				if (N < 2) { return; }
				TArray<double> D;
				for (int32 i = 0; i + 1 < N; ++i) { D.Add((Y[i + 1] - Y[i]) / FMath::Max(X[i + 1] - X[i], 1e-9)); }
				M[0] = D[0];
				M[N - 1] = D[N - 2];
				for (int32 i = 1; i + 1 < N; ++i) { M[i] = (D[i - 1] * D[i] <= 0.0) ? 0.0 : 0.5 * (D[i - 1] + D[i]); }
				for (int32 i = 0; i + 1 < N; ++i)
				{
					if (FMath::Abs(D[i]) < 1e-12) { M[i] = M[i + 1] = 0.0; continue; }
					const double A = M[i] / D[i], B = M[i + 1] / D[i];
					const double S = A * A + B * B;
					if (S > 9.0)
					{
						const double T = 3.0 / FMath::Sqrt(S);
						M[i] = T * A * D[i];
						M[i + 1] = T * B * D[i];
					}
				}
			}
			double At(double V) const
			{
				const int32 N = X.Num();
				if (V <= X[0]) { return Y[0] + M[0] * (V - X[0]); }
				if (V >= X[N - 1]) { return Y[N - 1] + M[N - 1] * (V - X[N - 1]); }
				int32 i = 0;
				while (i < N - 2 && V > X[i + 1]) { ++i; }
				const double H = X[i + 1] - X[i], T = (V - X[i]) / H;
				const double T2 = T * T, T3 = T2 * T;
				return (2 * T3 - 3 * T2 + 1) * Y[i] + (T3 - 2 * T2 + T) * H * M[i] + (-2 * T3 + 3 * T2) * Y[i + 1] + (T3 - T2) * H * M[i + 1];
			}
		};
	}

	/** Everything derived from a spec: heights, the curves, the eave's stations. */
	struct FHallGeo
	{
		const FHallSpec* H = nullptr;
		double Zc = 0.0;
		TArray<double> Seat;              // the purlins' tops (the rafters' undersides)
		double RidgeV = 0.0;              // the ridge line (the rolled roof's middle)
		int32 RidgeI = 0, RidgeJ = 0;     // the ridge purlin(s)
		double EaveR = 0.0;               // v where the round rafters end (negative)
		double EaveF = 0.0;               // the eave's edge (negative)
		double BackEdge = 0.0;            // the roof's back edge (v)
		double BackR = 0.0;               // the back round rafters' end (an open back)
		HallCurve::FCurve Bed;
		double PurlinR = 0.14;

		/** The rafters' top (the boarding's underside), straight purlin to purlin, on past the eave purlins. */
		double RafterTop(double V) const
		{
			const TArray<double>& P = H->PurlinV;
			const int32 N = P.Num();
			if (V <= P[0]) { return Seat[0] + RafterD + (Seat[1] - Seat[0]) / (P[1] - P[0]) * (V - P[0]); }
			if (V >= P[N - 1]) { return Seat[N - 1] + RafterD + (Seat[N - 1] - Seat[N - 2]) / (P[N - 1] - P[N - 2]) * (V - P[N - 1]); }
			int32 i = 0;
			while (i < N - 2 && V > P[i + 1]) { ++i; }
			return FMath::Lerp(Seat[i], Seat[i + 1], (V - P[i]) / (P[i + 1] - P[i])) + RafterD;
		}
		/** The flying rafters' underside, from the round rafters' end out, flatter. */
		double FlyUnder(double V, bool bBack) const
		{
			if (!bBack)
			{
				const double S0 = (Seat[1] - Seat[0]) / (H->PurlinV[1] - H->PurlinV[0]);
				return RafterTop(EaveR) + Board + S0 * FlyFlatten * (V - EaveR);
			}
			const int32 N = H->PurlinV.Num();
			const double S1 = (Seat[N - 1] - Seat[N - 2]) / (H->PurlinV[N - 1] - H->PurlinV[N - 2]);
			return RafterTop(BackR) + Board + S1 * FlyFlatten * (V - BackR);
		}
		/** The slab's underside: the boarding over the round rafters, the flying boarding past them. */
		double Under(double V) const
		{
			if (V < EaveR) { return FlyUnder(V, false) + FlyD; }
			if (!H->bSealedBack && V > BackR) { return FlyUnder(V, true) + FlyD; }
			return RafterTop(V);
		}
		FVector P(double U, double V, double Z) const { return H->L.P(U, V, Z); }
	};

	FHallGeo MakeGeo(const FHallSpec& H)
	{
		FHallGeo G;
		G.H = &H;
		G.Zc = H.Floor + H.ColH;
		G.PurlinR = FMath::Max(0.09, 0.45 * H.ColD);
		const int32 N = H.PurlinV.Num();
		G.Seat.SetNum(N);
		G.Seat[0] = G.Zc + SeatOver + 2.0 * G.PurlinR;
		for (int32 i = 0; i + 1 < N; ++i) { G.Seat[i + 1] = G.Seat[i] + H.Rise[i] * (H.PurlinV[i + 1] - H.PurlinV[i]) * (H.Rise[i] >= 0 ? 1.0 : 1.0); }
		// The ridge: the highest purlin (the rolled roof: the pair).
		int32 Top = 0;
		for (int32 i = 1; i < N; ++i) { if (G.Seat[i] > G.Seat[Top] + 1e-6) { Top = i; } }
		G.RidgeI = G.RidgeJ = Top;
		if (H.bRolled && Top + 1 < N && FMath::Abs(G.Seat[Top + 1] - G.Seat[Top]) < 1e-6) { G.RidgeJ = Top + 1; }
		G.RidgeV = 0.5 * (H.PurlinV[G.RidgeI] + H.PurlinV[G.RidgeJ]);
		G.EaveF = H.PurlinV[0] - H.EaveFront;
		G.EaveR = H.PurlinV[0] - H.EaveFront * EaveRatio;
		if (H.bSealedBack && H.bBackWall) { G.BackEdge = H.BackWallV1 + H.EaveBack; G.BackR = G.BackEdge; }
		else
		{
			G.BackEdge = H.PurlinV[N - 1] + H.EaveBack;
			G.BackR = H.PurlinV[N - 1] + H.EaveBack * EaveRatio;
		}
		// The bed: knots at the eave's edge, the round rafters' end, each purlin (its rafters' top plus the build-up).
		auto Add = [&G](double X, double Y) { G.Bed.X.Add(X); G.Bed.Y.Add(Y); };
		const double Up = RafterD + Board + Build;
		Add(G.EaveF, G.FlyUnder(G.EaveF, false) + FlyD + Board + 0.06);
		Add(G.EaveR, G.FlyUnder(G.EaveR, false) + FlyD + Board + 0.08);
		for (int32 i = 0; i < N; ++i)
		{
			Add(H.PurlinV[i], G.Seat[i] + Up);
			if (H.bRolled && i == G.RidgeI && G.RidgeJ != G.RidgeI) { Add(G.RidgeV, G.Seat[i] + Up + 0.06); }
		}
		if (H.bSealedBack && H.bBackWall)
		{
			const double S1 = (G.Seat[N - 1] - G.Seat[N - 2]) / (H.PurlinV[N - 1] - H.PurlinV[N - 2]);
			Add(G.BackEdge, G.Seat[N - 1] + Up + S1 * 0.8 * (G.BackEdge - H.PurlinV[N - 1]));
		}
		else
		{
			Add(G.BackR, G.FlyUnder(G.BackR, true) + FlyD + Board + 0.08);
			Add(G.BackEdge, G.FlyUnder(G.BackEdge, true) + FlyD + Board + 0.06);
		}
		// Knots must ascend.
		for (int32 i = 1; i < G.Bed.X.Num(); ++i) { G.Bed.X[i] = FMath::Max(G.Bed.X[i], G.Bed.X[i - 1] + 0.01); }
		G.Bed.Build();
		return G;
	}

	double FSection::BedAt(double V) const { return MakeGeo(*Spec).Bed.At(V); }
	double FSection::BoardAt(double V) const { return MakeGeo(*Spec).Under(V); }

	FSection MakeSection(const FHallSpec& H)
	{
		const FHallGeo G = MakeGeo(H);
		FSection S;
		S.Spec = &H;
		S.Zc = G.Zc;
		S.Seat = G.Seat;
		S.RidgeV = G.RidgeV;
		S.RollHalf = 0.5 * (H.PurlinV[G.RidgeJ] - H.PurlinV[G.RidgeI]);
		return S;
	}

	// ------------------------------------------------------------------------------------------------ helpers

	namespace HallParts
	{
		/** The roof's u-extent at v (the mitred ends of a gallery's corner). */
		void RoofSpan(const FHallSpec& H, double V, double& A, double& B)
		{
			A = H.RoofU0 != 0.0 || H.RoofU1 != 0.0 ? H.RoofU0 : H.U0;
			B = H.RoofU0 != 0.0 || H.RoofU1 != 0.0 ? H.RoofU1 : H.U1;
			if (H.MitreAtU0 != 0) { A = A + H.MitreAtU0 * (V - H.MitreRefV); }
			if (H.MitreAtU1 != 0) { B = B + H.MitreAtU1 * (V - H.MitreRefV); }
		}

		/** The columns' axes along u that stand free (the ends stand in the gable walls, unless an end is open). */
		TArray<double> FreeColumns(const FHallSpec& H)
		{
			TArray<double> Out;
			for (int32 i = 0; i < H.ColU.Num(); ++i)
			{
				const bool bEnd = i == 0 || i == H.ColU.Num() - 1;
				const bool bOpen = (i == 0 && (!H.bGableWalls || H.bStartOpen)) || (i == H.ColU.Num() - 1 && (!H.bGableWalls || H.bEndOpen));
				if (!bEnd || bOpen) { Out.Add(H.ColU[i]); }
			}
			return Out;
		}

		/** A column with its base: a stone (柱顶石) under a raised drum (鼓镜), the shaft tapering 1 %. */
		void Column(FChMeshes& Out, const FHallSpec& H, double U, double V, double Z0, double Z1, double D)
		{
			const FVector At = H.L.P(U, V, Z0);
			const double Side = 1.9 * D;
			Kit::Box(Out[CB::StoneKerb], TEXT("Column base"), At + FVector(-0.5 * Side, -0.5 * Side, -0.08), At + FVector(0.5 * Side, 0.5 * Side, 0.012));
			FProfile Drum;
			Drum.Add(0.0, 0.0).Add(0.68 * D, 0.0).Add(0.68 * D, 0.012).Add(0.62 * D, 0.045, true).Add(0.55 * D, 0.055).Add(0.0, 0.055);
			Kit::Lathe(Out[CB::StoneKerb], TEXT("Base drum"), At + FVector(0, 0, 0.01), FVector::UpVector, Drum, 24, 0.6 * D);
			FProfile Shaft;
			const double R0 = 0.5 * D, R1 = 0.5 * D * (1.0 - 0.01 * (Z1 - Z0) / D * 0.1);
			Shaft.Add(0.0, 0.0).Add(R0 - 0.008, 0.0).Add(R0, 0.01).Add(R1, Z1 - Z0 - 0.06).Add(0.0, Z1 - Z0 - 0.06);
			Kit::Lathe(Out[H.TimberPart], TEXT("Column"), At + FVector(0, 0, 0.06), FVector::UpVector, Shaft, 20, R0, true);
		}

		/** A beam across (along v) at u, from Va to Vb, its top at Top: a W × Hb section, the arrises eased (抹角). */
		void CrossBeam(FChMeshes& Out, const FHallSpec& H, double U, double Va, double Vb, double Top, double W, double Hb)
		{
			if (H.bPainted)
			{
				ChenghuaiPaint::Box(Out[CB::PaintedBeam], TEXT("Beam"), FLocal{H.L.Plan(U, 0.0), H.L.V, H.L.U}, Va, Vb, -0.5 * W, 0.5 * W, Top - Hb, Top, 4, 0.1, 0.9);
			}
			else
			{
				Kit::Member(Out[H.TimberPart], TEXT("Beam"), H.L.P(U, Va, Top - 0.5 * Hb), H.L.P(U, Vb, Top - 0.5 * Hb), H.L.U3(), W, Hb, 0.03);
			}
		}

		/** A strut (瓜柱) at (u, v) from Z0 to Z1, square with eased arrises. */
		void Strut(FChMeshes& Out, const FHallSpec& H, double U, double V, double Z0, double Z1, double W)
		{
			if (Z1 - Z0 < 0.02) { return; }
			const FVector A = H.L.P(U, V, Z0), B = H.L.P(U, V, Z1);
			Kit::Box(Out[H.TimberPart], TEXT("Strut"), FVector(FMath::Min(A.X, B.X) - 0.5 * W, FMath::Min(A.Y, B.Y) - 0.5 * W, Z0),
					 FVector(FMath::Max(A.X, B.X) + 0.5 * W, FMath::Max(A.Y, B.Y) + 0.5 * W, Z1));
		}

		/** A wall slab in the frame: u0 … u1 (its thickness or its length), v0 … v1, from Z0 up to Top(v) (sampled). */
		void WallPrism(FPart& Part, const FString& Name, const FLocal& L, double U0, double U1, double V0, double V1, double Z0,
					   TFunctionRef<double(double)> Top, double Step = 0.25)
		{
			// The outline in the (v, z) plane, extruded along u.
			TArray<FVector2D> Outline;
			Outline.Add(FVector2D(V0, Z0));
			Outline.Add(FVector2D(V1, Z0));
			const int32 NS = FMath::Max(1, FMath::CeilToInt32((V1 - V0) / Step));
			for (int32 k = NS; k >= 0; --k)
			{
				const double V = V0 + (V1 - V0) * k / NS;
				Outline.Add(FVector2D(V, Top(V)));
			}
			Kit::Extrude(Part, Name, Outline, L.P(0.0, 0.0, 0.0), L.V3(), FVector::UpVector, L.U3(), U0, U1);
		}
	}
	using namespace HallParts;

	// ------------------------------------------------------------------------------------------------ the lattice

	void LatticePanel(FChMeshes& Out, const FLocal& F, double U, double V, double Z, double W, double H, ELattice Pattern, int32 FramePart, bool bPaper)
	{
		constexpr double Bar = 0.018, Deep = 0.024, Rim = 0.03;
		FPart& P = Out[FramePart];
		auto Horiz = [&](double U0, double U1, double Zc) { Kit::LBox(P, TEXT("Lattice bar"), F, U0, U1, V, V + Deep, Zc - 0.5 * Bar, Zc + 0.5 * Bar); };
		auto Vert = [&](double Uc, double Z0, double Z1) { Kit::LBox(P, TEXT("Lattice bar"), F, Uc - 0.5 * Bar, Uc + 0.5 * Bar, V, V + Deep, Z0, Z1); };
		// The rim (仔边).
		Kit::LBox(P, TEXT("Lattice rim"), F, U, U + W, V - 0.004, V + Deep + 0.004, Z, Z + Rim);
		Kit::LBox(P, TEXT("Lattice rim"), F, U, U + W, V - 0.004, V + Deep + 0.004, Z + H - Rim, Z + H);
		Kit::LBox(P, TEXT("Lattice rim"), F, U, U + Rim, V - 0.004, V + Deep + 0.004, Z + Rim, Z + H - Rim);
		Kit::LBox(P, TEXT("Lattice rim"), F, U + W - Rim, U + W, V - 0.004, V + Deep + 0.004, Z + Rim, Z + H - Rim);
		const double IU0 = U + Rim, IU1 = U + W - Rim, IZ0 = Z + Rim, IZ1 = Z + H - Rim;
		const double IW = IU1 - IU0, IH = IZ1 - IZ0;
		if (Pattern == ELattice::IceCrack)
		{
			// 冰裂纹: the panel split again and again by straight cuts at random angles, each cut ending on an earlier one,
			// into convex cells 8-14 cm across; the bars are the cuts (true members 18 mm wide and 24 mm deep).
			int32 Seed = int32(FMath::Abs(FMath::Fmod(F.Origin.X * 131.7 + F.Origin.Y * 71.3 + U * 1013.0 + Z * 977.0 + V * 37.0, 1.0e6))) + 7;
			auto Rnd = [&Seed]() { Seed = (Seed * 1103515245 + 12345) & 0x7fffffff; return double(Seed % 100000) / 100000.0; };
			auto Area = [](const TArray<FVector2D>& Q) { return 0.5 * FMath::Abs(Kit::Area2(Q)); };
			TArray<TArray<FVector2D>> Cells = {{FVector2D(IU0, IZ0), FVector2D(IU1, IZ0), FVector2D(IU1, IZ1), FVector2D(IU0, IZ1)}};
			const int32 Splits = FMath::Clamp(FMath::RoundToInt32(IW * IH / 0.011), 1, 160);
			for (int32 n = 0, Tries = 0; n < Splits && Tries < Splits * 4; ++Tries)
			{
				int32 Best = 0;
				for (int32 c = 1; c < Cells.Num(); ++c) { if (Area(Cells[c]) > Area(Cells[Best])) { Best = c; } }
				const TArray<FVector2D> Q = Cells[Best];
				FVector2D C = FVector2D::ZeroVector;
				for (const FVector2D& X : Q) { C += X; }
				C /= Q.Num();
				const double Size = FMath::Sqrt(Area(Q));
				const FVector2D At = C + FVector2D(Rnd() - 0.5, Rnd() - 0.5) * 0.35 * Size;
				const double T = Rnd() * UE_DOUBLE_PI;
				const FVector2D Dir(FMath::Cos(T), FMath::Sin(T));
				TArray<FVector2D> L0, L1, Cut;
				for (int32 k = 0; k < Q.Num(); ++k)
				{
					const FVector2D X = Q[k], Y = Q[(k + 1) % Q.Num()];
					const double SX = Kit::Cross2(Dir, X - At), SY = Kit::Cross2(Dir, Y - At);
					(SX >= 0.0 ? L0 : L1).Add(X);
					if ((SX >= 0.0) != (SY >= 0.0))
					{
						const FVector2D I = X + (Y - X) * (SX / (SX - SY));
						L0.Add(I);
						L1.Add(I);
						Cut.Add(I);
					}
				}
				if (Cut.Num() != 2 || L0.Num() < 3 || L1.Num() < 3 || Area(L0) < 0.0035 || Area(L1) < 0.0035 || (Cut[1] - Cut[0]).Size() < 0.05) { continue; }
				Cells[Best] = L0;
				Cells.Add(L1);
				Kit::Member(P, TEXT("Lattice bar"), F.P(Cut[0].X, V + 0.5 * Deep, Cut[0].Y), F.P(Cut[1].X, V + 0.5 * Deep, Cut[1].Y), F.V3(), Deep, Bar);
				++n;
			}
		}
		else if (Pattern == ELattice::Grid)
		{
			const int32 NU = FMath::Max(1, FMath::RoundToInt32(IW / 0.13)), NZ = FMath::Max(1, FMath::RoundToInt32(IH / 0.15));
			for (int32 i = 1; i < NU; ++i) { Vert(IU0 + IW * i / NU, IZ0, IZ1); }
			for (int32 j = 1; j < NZ; ++j)
			{
				for (int32 i = 0; i < NU; ++i)
				{
					const double A = IU0 + IW * i / NU + (i > 0 ? 0.5 * Bar : 0.0), B = IU0 + IW * (i + 1) / NU - (i + 1 < NU ? 0.5 * Bar : 0.0);
					Horiz(A, B, IZ0 + IH * j / NZ);
				}
			}
		}
		else
		{
			// 灯笼框 (lantern): an open centre framed by a ring of small cells; 步步锦 (step brocade): two rings, the bars
			// of each ring's cells alternately upright and level.
			const int32 Rings = Pattern == ELattice::Lantern ? 1 : 2;
			double A0 = IU0, A1 = IU1, B0 = IZ0, B1 = IZ1;
			for (int32 r = 0; r < Rings; ++r)
			{
				const double M = FMath::Min(0.11, 0.18 * FMath::Min(A1 - A0, B1 - B0));
				const double N0 = A0 + M, N1 = A1 - M, O0 = B0 + M, O1 = B1 - M;
				if (N1 - N0 < 0.06 || O1 - O0 < 0.06) { break; }
				// The inner ring.
				Horiz(N0 - 0.5 * Bar, N1 + 0.5 * Bar, O0);
				Horiz(N0 - 0.5 * Bar, N1 + 0.5 * Bar, O1);
				Vert(N0, O0 + 0.5 * Bar, O1 - 0.5 * Bar);
				Vert(N1, O0 + 0.5 * Bar, O1 - 0.5 * Bar);
				// Ties across the margin: along the level sides upright, along the upright sides level.
				const int32 KU = FMath::Max(2, FMath::RoundToInt32((N1 - N0) / 0.16));
				for (int32 k = (r % 2); k <= KU; k += 1 + (Rings > 1 ? 1 : 0))
				{
					const double Uk = N0 + (N1 - N0) * k / KU;
					Vert(Uk, B0, O0 - 0.5 * Bar);
					Vert(Uk, O1 + 0.5 * Bar, B1);
				}
				const int32 KZ = FMath::Max(2, FMath::RoundToInt32((O1 - O0) / 0.16));
				for (int32 k = (r % 2); k <= KZ; k += 1 + (Rings > 1 ? 1 : 0))
				{
					const double Zk = O0 + (O1 - O0) * k / KZ;
					Horiz(A0, N0 - 0.5 * Bar, Zk);
					Horiz(N1 + 0.5 * Bar, A1, Zk);
				}
				A0 = N0 + 0.5 * Bar; A1 = N1 - 0.5 * Bar; B0 = O0 + 0.5 * Bar; B1 = O1 - 0.5 * Bar;
			}
			if (Pattern == ELattice::Lantern)
			{
				// The lantern's centre: one light cross.
				const double Cu = 0.5 * (A0 + A1), Cz = 0.5 * (B0 + B1);
				if (A1 - A0 > 0.25) { Vert(Cu, B0, B1); }
				if (B1 - B0 > 0.25) { Horiz(A0, Cu - 0.5 * Bar, Cz); Horiz(Cu + 0.5 * Bar, A1, Cz); }
			}
		}
		if (bPaper)
		{
			// Mulberry paper laminated in glass, just behind the bars (the room's side).
			Kit::LBox(Out[CB::Paper], TEXT("Paper"), F, U + 0.01, U + W - 0.01, V + Deep, V + Deep + 0.006, Z + 0.01, Z + H - 0.01);
		}
	}

	void DoorLeaf(FChMeshes& Out, const FLocal& F, double U, double V, double Z, double W, double H, int32 FramePart, ELattice Pattern)
	{
		constexpr double Stile = 0.065, Rail = 0.06, T = 0.065;
		FPart& P = Out[FramePart];
		Kit::LBox(P, TEXT("Door stile"), F, U, U + Stile, V, V + T, Z, Z + H);
		Kit::LBox(P, TEXT("Door stile"), F, U + W - Stile, U + W, V, V + T, Z, Z + H);
		// Five rails (五抹): the foot, under and over the waist panel, under the lattice head's top panel, the head.
		const double ZApron = Z + 0.30 * H, ZWaist = ZApron + 0.09 * H, ZTop = Z + H - 0.08 * H;
		const double Rails[5] = {Z, ZApron, ZWaist, ZTop, Z + H - Rail};
		for (const double R : Rails) { Kit::LBox(P, TEXT("Door rail"), F, U + Stile, U + W - Stile, V, V + T, R, R + Rail); }
		// Panels: the apron (裙板) with a raised carved field, the waist (绦环板) and the top panel, set 1 cm back.
		auto Panel = [&](double Z0, double Z1, bool bCarved)
		{
			Kit::LBox(P, TEXT("Door panel"), F, U + Stile - 0.005, U + W - Stile + 0.005, V + 0.02, V + 0.045, Z0, Z1);
			if (bCarved && Z1 - Z0 > 0.2)
			{
				const double M = 0.07;
				Kit::LBox(P, TEXT("Carved field"), F, U + Stile + M, U + W - Stile - M, V + 0.012, V + 0.02, Z0 + M, Z1 - M);
			}
		};
		Panel(Z + Rail, ZApron, true);
		Panel(ZApron + Rail, ZWaist, false);
		Panel(ZTop + Rail, Z + H - Rail, false);
		LatticePanel(Out, F, U + Stile, V + 0.018, ZWaist + Rail, W - 2 * Stile, ZTop - ZWaist - Rail, Pattern, FramePart, true);
	}

	// ------------------------------------------------------------------------------------------------ the building

	namespace HallBuild
	{
		void Platform(FChMeshes& Out, const FHallSpec& H)
		{
			const FLocal& L = H.L;
			const double U0 = H.PlatformU0, U1 = H.PlatformU1, V0 = H.PlatformFront, V1 = H.PlatformBack, F = H.Floor;
			// The brick body, the footing course and the kerb.
			Kit::LBox(Out[H.PlatformPart], TEXT("Platform body"), L, U0 + 0.01, U1 - 0.01, V0 + 0.01, V1 - 0.01, -0.10, F - KerbT + 0.002);
			Kit::LBox(Out[CB::StoneKerb], TEXT("Platform footing"), L, U0 - FootOut, U1 + FootOut, V0 - FootOut, V1 + FootOut, -0.10, 0.03);
			if (F - KerbT > 0.08)
			{
				Kit::LBox(Out[CB::StoneKerb], TEXT("Kerb front"), L, U0, U1, V0, V0 + KerbW, F - KerbT, F);
				Kit::LBox(Out[CB::StoneKerb], TEXT("Kerb back"), L, U0, U1, V1 - KerbW, V1, F - KerbT, F);
				Kit::LBox(Out[CB::StoneKerb], TEXT("Kerb side"), L, U0, U0 + KerbW, V0 + KerbW, V1 - KerbW, F - KerbT, F);
				Kit::LBox(Out[CB::StoneKerb], TEXT("Kerb side"), L, U1 - KerbW, U1, V0 + KerbW, V1 - KerbW, F - KerbT, F);
				// Corner stones (埋头) at the front corners.
				Kit::LBox(Out[CB::StoneKerb], TEXT("Corner stone"), L, U0 - 0.005, U0 + 0.3, V0 - 0.005, V0 + 0.3, 0.03, F - KerbT);
				Kit::LBox(Out[CB::StoneKerb], TEXT("Corner stone"), L, U1 - 0.3, U1 + 0.005, V0 - 0.005, V0 + 0.3, 0.03, F - KerbT);
			}
			else
			{
				// A low platform (the garden's walk): kerb stones round all four edges, solid to the footing (a strip at the
				// back once showed the body and the footing, coplanar with the garden's earth: it flickered).
				Kit::LBox(Out[CB::StoneKerb], TEXT("Kerb"), L, U0, U1, V0, V0 + KerbW, 0.03, F);
				Kit::LBox(Out[CB::StoneKerb], TEXT("Kerb back"), L, U0, U1, V1 - KerbW, V1, 0.03, F);
				Kit::LBox(Out[CB::StoneKerb], TEXT("Kerb side"), L, U0, U0 + KerbW, V0 + KerbW, V1 - KerbW, 0.03, F);
				Kit::LBox(Out[CB::StoneKerb], TEXT("Kerb side"), L, U1 - KerbW, U1, V0 + KerbW, V1 - KerbW, 0.03, F);
			}
			// The floor of square bricks inside the kerb.
			const int32 FloorPart = H.FloorPart >= 0 ? H.FloorPart : (H.Floor >= 0.5 ? CB::FloorLarge : CB::FloorSmall);
			Kit::LBox(Out[FloorPart], TEXT("Floor"), L, U0 + KerbW, U1 - KerbW, V0 + KerbW, V1 - KerbW, F - KerbT + 0.001, F - 0.002);
		}

		void Steps(FChMeshes& Out, const FHallSpec& H)
		{
			const FLocal& L = H.L;
			for (const FSteps& S : H.Steps)
			{
				const int32 Risers = FMath::Max(1, FMath::CeilToInt32(H.Floor / 0.16 - 0.05));
				const double R = H.Floor / Risers;
				const double Edge = S.bBack ? H.PlatformBack : H.PlatformFront;
				const double Out1 = S.bBack ? 1.0 : -1.0;
				const double U0 = S.U - 0.5 * S.Width, U1 = S.U + 0.5 * S.Width;
				for (int32 k = 0; k + 1 < Risers + 1; ++k)
				{
					// Tread k (from the top): its riser's top at F − k·R.
					if (k == 0) { continue; }
					const double Top = H.Floor - k * R;
					const double A = Edge + Out1 * (k - 1) * S.Going, B = Edge + Out1 * k * S.Going;
					Kit::LBox(Out[CB::StoneKerb], TEXT("Step"), L, U0, U1, FMath::Min(A, B) - (S.bBack ? 0.0 : 0.02), FMath::Max(A, B) + (S.bBack ? 0.02 : 0.0), -0.08, Top);
				}
				// The sloping side stones (垂带), 0.3 wide.
				const double Run = (Risers - 1) * S.Going + 0.02;
				for (const double Su : {U0 - 0.3, U1})
				{
					TArray<FVector2D> Side = {FVector2D(0.0, -0.08), FVector2D(Out1 * (Run + 0.05), -0.08), FVector2D(Out1 * (Run + 0.05), 0.12),
											  FVector2D(0.0, H.Floor)};
					if (Out1 < 0.0) { Algo::Reverse(Side); }
					Kit::Extrude(Out[CB::StoneKerb], TEXT("Side stone"), Side, L.P(0.0, Edge, 0.0), L.V3(), FVector::UpVector, L.U3(), Su, Su + 0.3);
				}
			}
		}

		void Columns(FChMeshes& Out, const FHallSpec& H, const FHallGeo& G)
		{
			const TArray<double> Free = FreeColumns(H);
			for (const double U : Free)
			{
				Column(Out, H, U, 0.0, H.Floor, G.Zc, H.ColD);
				if (H.FacadeV > 0.01)
				{
					const int32 I = H.PurlinV.IndexOfByPredicate([&](double V) { return FMath::Abs(V - H.FacadeV) < 0.02; });
					const double Top = I != INDEX_NONE ? G.Seat[I] - SeatOver - 2.0 * G.PurlinR - 0.05 : G.Zc + 0.3;
					Column(Out, H, U, H.FacadeV, H.Floor, Top, H.ColD + 0.03);
				}
				for (const double V : H.InnerColV)
				{
					const int32 I = H.PurlinV.IndexOfByPredicate([&](double P) { return FMath::Abs(P - V) < 0.02; });
					const double Top = I != INDEX_NONE ? G.Seat[I] - SeatOver - 2.0 * G.PurlinR - 0.05 : G.Zc + 0.3;
					Column(Out, H, U, V, H.Floor, Top, H.ColD + 0.03);
				}
				if (H.bBackColumns)
				{
					Column(Out, H, U, H.PurlinV.Last(), H.Floor, G.Seat.Last() - SeatOver - 2.0 * G.PurlinR, H.ColD);
				}
			}
		}

		/** Purlins along u, the eave lintels and boards, the beams and struts at every column. */
		void Frame(FChMeshes& Out, const FHallSpec& H, const FHallGeo& G)
		{
			const FLocal& L = H.L;
			const int32 N = H.PurlinV.Num();
			const double Pr = G.PurlinR;
			const double RA = H.RoofU0 != 0.0 || H.RoofU1 != 0.0 ? H.RoofU0 : H.U0;
			const double RB = H.RoofU0 != 0.0 || H.RoofU1 != 0.0 ? H.RoofU1 : H.U1;
			// Purlins: into the gable walls (or out to the open ends' eave), the painted ones banded.
			for (int32 i = 0; i < N; ++i)
			{
				if (H.SkipPurlins.Contains(i)) { continue; }
				double A = RA + (H.bGableWalls && !H.bStartOpen ? 0.12 : 0.0), B = RB - (H.bGableWalls && !H.bEndOpen ? 0.12 : 0.0);
				double MA, MB;
				RoofSpan(H, H.PurlinV[i], MA, MB);
				A = FMath::Max(A, MA + 0.02);
				B = FMath::Min(B, MB - 0.02);
				const FVector PA = G.P(A, H.PurlinV[i], G.Seat[i] - Pr), PB = G.P(B, H.PurlinV[i], G.Seat[i] - Pr);
				if (H.bPainted) { ChenghuaiPaint::Rod(Out[CB::PaintedBeam], TEXT("Purlin"), PA, PB, Pr, 16, 5, 3.1); }
				else { Kit::Rod(Out[H.TimberPart], TEXT("Purlin"), PA, PB, Pr, 16); }
			}
			// The eave line: a lintel (檐枋) and a board (垫板) in each bay between the columns, painted by rank.
			const double LintelH = 0.9 * H.ColD + 0.04, LintelT = 0.75 * H.ColD;
			const double BoardTop = G.Seat[0] - 2.0 * Pr + 0.02;
			for (int32 i = 0; i + 1 < H.ColU.Num(); ++i)
			{
				const double A = H.ColU[i] + (i == 0 && H.bGableWalls && !H.bStartOpen ? 0.0 : 0.0), B = H.ColU[i + 1];
				const int32 Row = FMath::Clamp(H.PaintRow + (i % 4), 0, 3);
				if (H.bPainted)
				{
					ChenghuaiPaint::Box(Out[CB::PaintedBeam], TEXT("Eave board"), L, A, B, -0.03, 0.03, G.Zc, BoardTop, Row, 0.0, 0.42);
					ChenghuaiPaint::Box(Out[CB::PaintedBeam], TEXT("Eave lintel"), L, A, B, -0.5 * LintelT, 0.5 * LintelT, G.Zc - LintelH, G.Zc, Row, 0.42, 1.0);
				}
				else
				{
					Kit::LBox(Out[H.TimberPart], TEXT("Eave board"), L, A, B, -0.03, 0.03, G.Zc, BoardTop);
					Kit::LBox(Out[H.TimberPart], TEXT("Eave lintel"), L, A, B, -0.5 * LintelT, 0.5 * LintelT, G.Zc - LintelH, G.Zc);
				}
				// The facade line's lintel (金枋) over the lattice, in a veranda.
				if (H.FacadeV > 0.01)
				{
					const int32 I = H.PurlinV.IndexOfByPredicate([&](double V) { return FMath::Abs(V - H.FacadeV) < 0.02; });
					const double Zt = I != INDEX_NONE ? G.Seat[I] - SeatOver - 2.0 * Pr - 0.05 : G.Zc + 0.3;
					if (H.bPainted)
					{
						ChenghuaiPaint::Box(Out[CB::PaintedBeam], TEXT("Inner lintel"), L, A, B, H.FacadeV - 0.5 * LintelT, H.FacadeV + 0.5 * LintelT, Zt - LintelH, Zt, 4, 0.1, 0.9);
						ChenghuaiPaint::Box(Out[CB::PaintedBeam], TEXT("Inner board"), L, A, B, H.FacadeV - 0.03, H.FacadeV + 0.03, Zt, G.Seat[I != INDEX_NONE ? I : 1] - 2.0 * Pr + 0.02, 4, 0.1, 0.9);
					}
					else
					{
						Kit::LBox(Out[H.TimberPart], TEXT("Inner lintel"), L, A, B, H.FacadeV - 0.5 * LintelT, H.FacadeV + 0.5 * LintelT, Zt - LintelH, Zt);
					}
				}
			}
			// The frames across, at every free column (the gable walls carry the ends).
			// A gallery's back line: lintels and boards between its back columns too.
			if (H.bBackColumns)
			{
				const double VBk = H.PurlinV.Last();
				for (int32 i = 0; i + 1 < H.ColU.Num(); ++i)
				{
					const double A = H.ColU[i], B = H.ColU[i + 1];
					if (H.bPainted)
					{
						ChenghuaiPaint::Box(Out[CB::PaintedBeam], TEXT("Back lintel"), L, A, B, VBk - 0.5 * LintelT, VBk + 0.5 * LintelT, G.Zc - LintelH, G.Zc, 4, 0.1, 0.9);
					}
					else
					{
						Kit::LBox(Out[H.TimberPart], TEXT("Back lintel"), L, A, B, VBk - 0.5 * LintelT, VBk + 0.5 * LintelT, G.Zc - LintelH, G.Zc);
					}
				}
			}
			if (!H.bFrames) { return; }
			const TArray<double> Free = H.FrameU.Num() > 0 ? H.FrameU : FreeColumns(H);
			const double BW = 0.8 * H.ColD + 0.04, BH = 1.2 * H.ColD + 0.06;
			for (const double U : Free)
			{
				// The main span: from the front support (the facade's columns in a veranda, else the eave's) to the back
				// (the rear inner columns, else the back eave purlin on the wall or the back columns).
				int32 A = 0;
				if (H.FacadeV > 0.01) { A = FMath::Max(0, H.PurlinV.IndexOfByPredicate([&](double V) { return FMath::Abs(V - H.FacadeV) < 0.02; })); }
				int32 B = N - 1;
				if (H.InnerColV.Num() > 0) { B = FMath::Max(A + 1, H.PurlinV.IndexOfByPredicate([&](double V) { return FMath::Abs(V - H.InnerColV.Last()) < 0.02; })); }
				// A veranda's beam (抱头梁) from the eave column to the facade column, and a tie (穿插枋) under it.
				if (A > 0)
				{
					CrossBeam(Out, H, U, -0.22, H.PurlinV[A] + 0.05, G.Seat[0] - Pr * 1.2, BW, BH);
					Kit::Member(Out[H.TimberPart], TEXT("Tie"), G.P(U, -0.05, G.Zc - 0.75), G.P(U, H.PurlinV[A], G.Zc - 0.75), L.U3(), 0.6 * H.ColD, 0.8 * H.ColD, 0.02);
				}
				if (B < N - 1)
				{
					CrossBeam(Out, H, U, H.PurlinV[B] - 0.05, H.PurlinV[N - 1] + 0.2, G.Seat[N - 1] - Pr * 1.2, BW, BH);
				}
				// Beams stepping up: each carries the purlins at its ends; struts carry the next.
				double LastTop = -1.0;
				int32 a = A, b = B;
				double Width = BW, Height = BH;
				while (b - a >= 1)
				{
					const double Top = FMath::Min(G.Seat[a], G.Seat[b]) - 2.0 * Pr * 0.8;
					const double Va = H.PurlinV[a] - (a == A ? 0.22 : 0.18), Vb = H.PurlinV[b] + (b == B && B == N - 1 ? 0.2 : 0.18);
					if (LastTop > 0.0)
					{
						Strut(Out, H, U, H.PurlinV[a], LastTop, Top - Height, 0.7 * Width);
						Strut(Out, H, U, H.PurlinV[b], LastTop, Top - Height, 0.7 * Width);
					}
					CrossBeam(Out, H, U, Va, Vb, Top, Width, Height);
					// A block under the higher end's purlin, where the seats differ.
					if (FMath::Abs(G.Seat[a] - G.Seat[b]) > 0.03)
					{
						const int32 Hi = G.Seat[a] > G.Seat[b] ? a : b;
						Strut(Out, H, U, H.PurlinV[Hi], Top, G.Seat[Hi] - 2.0 * Pr * 0.8, 0.6 * Width);
					}
					LastTop = Top;
					Width *= 0.88;
					Height *= 0.85;
					++a;
					--b;
				}
				if (a == b)
				{
					// The ridge's strut (脊瓜柱) from the last beam to the ridge purlin.
					Strut(Out, H, U, H.PurlinV[a], LastTop, G.Seat[a] - 2.0 * Pr * 0.9, 0.6 * Width);
				}
			}
		}

		/** Rafters, flying rafters, the eave boards and the rafters' painted ends. */
		void Rafters(FChMeshes& Out, const FHallSpec& H, const FHallGeo& G)
		{
			const FLocal& L = H.L;
			const int32 N = H.PurlinV.Num();
			const double R = 0.5 * RafterD;
			const double RA = H.RoofU0 != 0.0 || H.RoofU1 != 0.0 ? H.RoofU0 : H.U0;
			const double RB = H.RoofU0 != 0.0 || H.RoofU1 != 0.0 ? H.RoofU1 : H.U1;
			const double Inset = H.bGableWalls ? H.GableT + 0.12 : 0.08;
			const double UA = RA + (H.bGableWalls && !H.bStartOpen ? Inset : 0.08), UB = RB - (H.bGableWalls && !H.bEndOpen ? Inset : 0.08);
			const int32 Count = FMath::Max(1, FMath::FloorToInt32((UB - UA) / RafterPitch));
			const double Pitch = (UB - UA) / Count;
			int32 Seq = 0;
			for (int32 k = 0; k <= Count; ++k)
			{
				const double U = UA + Pitch * k;
				// Rafter by rafter, step by step; a mitred end cuts the rafters that would cross it.
				auto Inside = [&](double V)
				{
					double A, B;
					RoofSpan(H, V, A, B);
					return U > A + 0.1 && U < B - 0.1;
				};
				for (int32 i = 0; i + 1 < N; ++i)
				{
					const double Va = H.PurlinV[i], Vb = H.PurlinV[i + 1];
					if (!Inside(Va) || !Inside(Vb)) { continue; }
					const FVector A = G.P(U, Va, G.Seat[i] + R), B = G.P(U, Vb, G.Seat[i + 1] + R);
					Kit::Rod(Out[RafterPartOf(H)], TEXT("Rafter"), A - (B - A).GetSafeNormal() * 0.04, B + (B - A).GetSafeNormal() * 0.04, R, 8);
				}
				// The eave: round rafters out to EaveR, flying rafters on them to the edge.
				auto Eave = [&](bool bBack)
				{
					const double V0 = bBack ? H.PurlinV[N - 1] : H.PurlinV[0];
					const double VR = bBack ? G.BackR : G.EaveR;
					const double VF = bBack ? G.BackEdge : G.EaveF;
					if (!Inside(VR) || !Inside(VF) || FMath::Abs(VF - V0) < 0.1) { return; }
					const FVector A = G.P(U, V0, G.RafterTop(V0) - R), B = G.P(U, VR, G.RafterTop(VR) - R);
					const FVector Dir = (B - A).GetSafeNormal();
					Kit::Rod(Out[RafterPartOf(H)], TEXT("Eave rafter"), A - Dir * 0.2, B, R, 8);
					if (H.bRafterEnds) { ChenghuaiPaint::EndCap(Out[CB::PaintedBeam], B + Dir * 0.002, Dir, FVector::UpVector, R, true, (Seq++) % 8); }
					// The flying rafter: its tail on the boarding, its head past the round rafter's end.
					const double S = bBack ? 1.0 : -1.0;
					const FVector T0 = G.P(U, VR - S * 0.9, G.RafterTop(VR - S * 0.9) + Board + 0.5 * FlyD);
					const FVector T1 = G.P(U, VR, G.FlyUnder(VR, bBack) + 0.5 * FlyD);
					const FVector T2 = G.P(U, VF, G.FlyUnder(VF, bBack) + 0.5 * FlyD);
					Kit::Member(Out[RafterPartOf(H)], TEXT("Flying rafter tail"), T0, T1 + (T1 - T0).GetSafeNormal() * 0.03, L.U3(), FlyD, FlyD);
					Kit::Member(Out[RafterPartOf(H)], TEXT("Flying rafter"), T1, T2, L.U3(), FlyD, FlyD);
					if (H.bRafterEnds) { ChenghuaiPaint::EndCap(Out[CB::PaintedBeam], T2 + (T2 - T1).GetSafeNormal() * 0.002, (T2 - T1).GetSafeNormal(), FVector::UpVector, 0.5 * FlyD, false, 8 + (Seq % 4)); }
				};
				Eave(false);
				if (!H.bSealedBack || !H.bBackWall) { Eave(true); }
			}
		}

		/** The roof's slab (boarding under, lime over), its eave boards, the tiles, the ridge and the gables' edges. */
		void Roof(FChMeshes& Out, const FHallSpec& H, const FHallGeo& G)
		{
			const FLocal& L = H.L;
			const double VF = G.EaveF, VB = G.BackEdge;
			// Stations along v: fine enough for the curve, with the eave's step at EaveR (twice).
			TArray<double> Vs;
			const int32 NS = FMath::Max(8, FMath::CeilToInt32((VB - VF) / 0.12));
			for (int32 k = 0; k <= NS; ++k) { Vs.Add(VF + (VB - VF) * k / NS); }
			Vs.Add(G.EaveR);
			if (!H.bSealedBack || !H.bBackWall) { Vs.Add(G.BackR); }
			for (const double P : H.PurlinV) { Vs.Add(P); }
			Vs.Add(G.RidgeV);
			Vs.Sort();
			TArray<double> Clean;
			for (const double V : Vs) { if (Clean.Num() == 0 || V > Clean.Last() + 0.005) { Clean.Add(V); } }
			Vs = Clean;
			const int32 NU = 6;
			auto Span = [&](double V, int32 J)
			{
				double A, B;
				RoofSpan(H, V, A, B);
				return FMath::Lerp(A, B, double(J) / NU);
			};
			const FString Slab = H.Name + TEXT(" roof slab");
			// The top (the lime bed), under the tiles.
			Out[CB::RoofMortar].Begin(H.Name + TEXT(" roof bed"), false, Slab);
			Kit::GridPatch(Out[CB::RoofMortar].M, Vs.Num() - 1, NU, [&](int32 I, int32 J, FVector& P, FVector& Nrm, FVector2D& UV)
			{
				const double V = Vs[I], U = Span(V, J);
				P = G.P(U, V, G.Bed.At(V));
				const double Slope = (G.Bed.At(V + 0.01) - G.Bed.At(V - 0.01)) / 0.02;
				Nrm = (FVector(0, 0, 1) - L.V3() * Slope).GetSafeNormal();
				UV = FVector2D(P.X, P.Y);
			});
			// The underside: the boarding (red), in two pieces either side of the eave's step.
			auto Underside = [&](double VA, double VB2, int32 Mode)
			{
				TArray<double> Sub;
				for (const double V : Vs) { if (V >= VA - 1e-6 && V <= VB2 + 1e-6) { Sub.Add(V); } }
				if (Sub.Num() < 2) { return; }
				auto ZOf = [&](double V)
				{
					if (Mode == 1) { return G.FlyUnder(V, false) + FlyD; }
					if (Mode == 2) { return G.FlyUnder(V, true) + FlyD; }
					return G.RafterTop(V);
				};
				Out[RafterPartOf(H)].Begin(H.Name + TEXT(" boarding"), false, Slab);
				Kit::GridPatch(Out[RafterPartOf(H)].M, Sub.Num() - 1, NU, [&](int32 I, int32 J, FVector& P, FVector& Nrm, FVector2D& UV)
				{
					const double V = Sub[I], U = Span(V, J);
					P = G.P(U, V, ZOf(V));
					const double Slope = (ZOf(V + 0.01) - ZOf(V - 0.01)) / 0.02;
					Nrm = -(FVector(0, 0, 1) - L.V3() * Slope).GetSafeNormal();
					UV = FVector2D(P.X, P.Y);
				});
			};
			const bool bOpenBack = !H.bSealedBack || !H.bBackWall;
			Underside(VF, G.EaveR, 1);
			Underside(G.EaveR, bOpenBack ? G.BackR : VB, 0);
			if (bOpenBack) { Underside(G.BackR, VB, 2); }
			// The step at the round rafters' end (小连檐), facing out.
			auto Riser = [&](double V, bool bBack, double ZLo, double ZHi)
			{
				TArray<FVector> Upper, Lower;
				for (int32 j = 0; j <= NU; ++j)
				{
					const double U = Span(V, j);
					Upper.Add(G.P(U, V, ZHi));
					Lower.Add(G.P(U, V, ZLo));
				}
				Out[RafterPartOf(H)].Begin(TEXT("Eave board"), false, Slab);
				Kit::Strip(Out[RafterPartOf(H)].M, Upper, Lower, L.V3() * (bBack ? 1.0 : -1.0));
			};
			Riser(G.EaveR, false, G.RafterTop(G.EaveR), G.FlyUnder(G.EaveR, false) + FlyD);
			if (bOpenBack) { Riser(G.BackR, true, G.RafterTop(G.BackR), G.FlyUnder(G.BackR, true) + FlyD); }
			// The fascia (大连檐 and 瓦口) at the eave's edge, and the back edge.
			auto Edge = [&](double V, bool bBack, int32 PartId, double ZLo)
			{
				TArray<FVector> Upper, Lower;
				for (int32 j = 0; j <= NU; ++j)
				{
					const double U = Span(V, j);
					Upper.Add(G.P(U, V, G.Bed.At(V)));
					Lower.Add(G.P(U, V, ZLo));
				}
				Out[PartId].Begin(TEXT("Eave edge"), false, Slab);
				Kit::Strip(Out[PartId].M, Upper, Lower, L.V3() * (bBack ? 1.0 : -1.0));
			};
			Edge(VF, false, H.TimberPart, G.Under(VF));
			Edge(VB, true, bOpenBack ? H.TimberPart : CB::BrickFine, G.Under(VB));
			// The ends (the gables' faces show them as the brick band under the edge tiles, 博缝).
			for (const int32 End : {0, NU})
			{
				TArray<FVector> Upper, Lower;
				for (const double V : Vs)
				{
					const double U = Span(V, End);
					Upper.Add(G.P(U, V, G.Bed.At(V)));
					Lower.Add(G.P(U, V, G.Under(V)));
				}
				FVector N = L.U3() * (End == 0 ? -1.0 : 1.0);
				const bool bMitre = (End == 0 && H.MitreAtU0 != 0) || (End == NU && H.MitreAtU1 != 0);
				if (bMitre)
				{
					const double S = End == 0 ? H.MitreAtU0 : H.MitreAtU1;
					N = (L.U3() * (End == 0 ? -1.0 : 1.0) + L.V3() * (End == 0 ? S : -S)).GetSafeNormal();
				}
				// A gable's brick band (博缝), an overhanging end's board (悬山), a mitre's lime.
				const int32 EndPart = bMitre ? CB::RoofMortar : (H.bGableWalls ? CB::BrickFine : H.TimberPart);
				Out[EndPart].Begin(TEXT("Roof end"), false, Slab);
				Kit::Strip(Out[EndPart].M, Upper, Lower, N);
			}

			// The tiles: two slopes from the ridge, whole rows between the gables' edge tiles.
			const ChenghuaiRoof::FTileStyle Style = H.Tiles == ChenghuaiRoof::ETiles::Tong ? ChenghuaiRoof::FTileStyle::Tong() : ChenghuaiRoof::FTileStyle::He();
			const double EdgeW = H.bGableWalls ? 0.26 : 0.0;
			const double RidgeHalf = H.Ridge == ERidge::Qingshui ? 0.13 : 0.0;
			double UA, UB;
			RoofSpan(H, G.RidgeV, UA, UB);
			for (const int32 Side : {0, 1})
			{
				ChenghuaiRoof::FSlope S;
				const double Sign = Side == 0 ? -1.0 : 1.0;
				const double VStart = G.RidgeV + Sign * (H.bRolled ? 0.0 : 0.0);
				S.Origin = L.Plan(0.0, VStart);
				S.DDir = L.V * Sign;
				S.SDir = L.U;
				S.Bed = [&G, VStart, Sign](double D, double) { return G.Bed.At(VStart + Sign * D); };
				S.From = RidgeHalf;
				S.To = Side == 0 ? (VStart - VF) : (VB - VStart);
				S.bOrnaments = Side == 0 || bOpenBack;
				// Mitred ends: the half-planes of the slanted end lines.
				auto Mitre = [&](int32 Sgn, double UEnd, bool bAtU0)
				{
					// The end line: u = UEnd + Sgn (v − RefV) → in plan: point on the line and its normal pointing out of the roof.
					const FVector2D P0 = L.Plan(UEnd, H.MitreRefV);
					const FVector2D Dir = (L.U * Sgn + L.V).GetSafeNormal();
					FVector2D Nrm(-Dir.Y, Dir.X);
					const FVector2D Inside = L.Plan(UEnd + (bAtU0 ? 1.0 : -1.0), H.MitreRefV);
					if (FVector2D::DotProduct(Nrm, Inside - P0) > 0.0) { Nrm = -Nrm; }
					S.Limits.Add(FVector(Nrm.X, Nrm.Y, FVector2D::DotProduct(Nrm, P0)));
				};
				const double BaseA = H.RoofU0 != 0.0 || H.RoofU1 != 0.0 ? H.RoofU0 : H.U0;
				const double BaseB = H.RoofU0 != 0.0 || H.RoofU1 != 0.0 ? H.RoofU1 : H.U1;
				if (H.MitreAtU0 != 0) { Mitre(H.MitreAtU0, BaseA, true); }
				if (H.MitreAtU1 != 0) { Mitre(H.MitreAtU1, BaseB, false); }
				const double SA = H.MitreAtU0 != 0 ? BaseA - 6.0 : UA + EdgeW;
				const double SB = H.MitreAtU1 != 0 ? BaseB + 6.0 : UB - EdgeW;
				ChenghuaiRoof::TileRows(Out[CB::Tiles], S, SA, SB, Style, UA + EdgeW + Style.PanHalf);
			}

			// The ridge.
			const double ZR = G.Bed.At(G.RidgeV);
			if (H.Ridge == ERidge::Qingshui)
			{
				const double A = UA + 0.02, B = UB - 0.02;
				FProfile P;
				P.bClosed = true;
				// 当沟 and 瓦条 at the foot, the body (脊身), the cap of half-round tiles (筒瓦 on top).
				P.Add(-0.17, -0.08).Add(0.17, -0.08).Add(0.17, 0.06).Add(0.13, 0.08).Add(0.13, 0.12).Add(0.105, 0.14).Add(0.105, 0.34).Add(0.12, 0.36)
				 .Add(0.12, 0.40).Add(0.075, 0.44, true).Add(0.0, 0.46, true).Add(-0.075, 0.44, true).Add(-0.12, 0.40).Add(-0.12, 0.36).Add(-0.105, 0.34)
				 .Add(-0.105, 0.14).Add(-0.13, 0.12).Add(-0.13, 0.08).Add(-0.17, 0.06);
				Kit::SweepSolid(Out[CB::Ridges], TEXT("Ridge"), Kit::RunFrames(G.P(A, G.RidgeV, ZR), G.P(B, G.RidgeV, ZR), L.V3()), P);
				// The scorpion tails (蝎子尾): at each end a tail rising outwards at 30°, on a plate of carved brick (平草).
				for (const int32 End : {0, 1})
				{
					const double UE = End == 0 ? A + 0.35 : B - 0.35;
					const double Out1 = End == 0 ? -1.0 : 1.0;
					Kit::LBox(Out[CB::Ridges], TEXT("Tail plate"), L, UE - 0.28, UE + 0.28, G.RidgeV - 0.16, G.RidgeV + 0.16, ZR + 0.40, ZR + 0.47);
					TArray<FVector> Path;
					for (int32 k = 0; k <= 8; ++k)
					{
						const double T = double(k) / 8;
						Path.Add(G.P(UE + Out1 * (0.1 + 0.55 * T), G.RidgeV, ZR + 0.44 + 0.34 * T + 0.08 * T * T));
					}
					FProfile Tail;
					Tail.bClosed = true;
					Tail.Add(-0.06, -0.06).Add(0.06, -0.06).Add(0.06, 0.04).Add(0.0, 0.07, true).Add(-0.06, 0.04);
					Kit::SweepSolid(Out[CB::Ridges], TEXT("Scorpion tail"), Kit::UprightFrames(Path), Tail);
				}
			}
			else if (H.Ridge == ERidge::Rolled)
			{
				// The rolled ridge: a row of bent tiles (罗锅瓦) over the top, low.
				FProfile P;
				P.bClosed = true;
				P.Add(-0.16, -0.04).Add(0.16, -0.04).Add(0.16, 0.02).Add(0.1, 0.06, true).Add(0.0, 0.075, true).Add(-0.1, 0.06, true).Add(-0.16, 0.02);
				Kit::SweepSolid(Out[CB::Ridges], TEXT("Rolled ridge"), Kit::RunFrames(G.P(UA + EdgeW, G.RidgeV, ZR), G.P(UB - EdgeW, G.RidgeV, ZR), L.V3()), P);
			}
			// The gables' edges (排山): a small ridge down each gable over its brick band, from the ridge to the eave.
			if (H.bGableWalls)
			{
				for (const int32 End : {0, 1})
				{
					if ((End == 0 && H.bStartOpen) || (End == 1 && H.bEndOpen)) { continue; }
					double A0, B0;
					RoofSpan(H, 0.0, A0, B0);
					const double U = End == 0 ? A0 + 0.5 * EdgeW : B0 - 0.5 * EdgeW;
					for (const int32 Side : {0, 1})
					{
						TArray<FVector> Path;
						const double V1 = Side == 0 ? VF - 0.02 : VB + 0.02;
						const int32 K = 16;
						for (int32 k = 0; k <= K; ++k)
						{
							const double V = FMath::Lerp(G.RidgeV, V1, double(k) / K);
							Path.Add(G.P(U, V, G.Bed.At(V) + 0.02));
						}
						FProfile E;
						E.bClosed = true;
						E.Add(-0.5 * EdgeW - 0.015, -0.06).Add(0.5 * EdgeW + 0.015, -0.06).Add(0.5 * EdgeW + 0.015, 0.05).Add(0.08, 0.11, true).Add(0.0, 0.13, true)
						 .Add(-0.08, 0.11, true).Add(-0.5 * EdgeW - 0.015, 0.05);
						Kit::SweepSolid(Out[CB::Ridges], TEXT("Gable edge"), Kit::UprightFrames(Path), E);
					}
				}
			}
		}

		/** A wall in the (v, z) plane at u, thickness T (u from U to U + T), with openings cut through it. */
		void GableWall(FChMeshes& Out, const FHallSpec& H, const FHallGeo& G, bool bEnd)
		{
			const FLocal& L = H.L;
			const double UOut = bEnd ? H.U1 - H.GableT : H.U0, UIn = UOut + H.GableT;
			const double U0 = bEnd ? H.U1 - H.GableT : H.U0, U1 = bEnd ? H.U1 : H.U0 + H.GableT;
			const double InnerFace = bEnd ? U0 : U1;             // the room's side
			const double VFront = -0.36;
			const double VBack = H.GableBackV > 0.0 ? H.GableBackV : (H.bBackWall ? H.BackWallV1 : H.PurlinV.Last() + 0.3);
			const double ZBase = H.Floor + BaseCourseH;
			// Up into the boarding by 1.5 cm (hidden in the lime bed over it): stopping 2 mm under it left a slit the sun came
			// through, a bright line along every gable's top inside.
			auto Top = [&G](double V) { return G.Under(V) + 0.015; };
			// Openings through it.
			TArray<FGableOpening> Ops;
			for (const FGableOpening& O : H.GableOpenings) { if (O.bEnd == bEnd) { Ops.Add(O); } }
			Ops.Sort([](const FGableOpening& A, const FGableOpening& B) { return A.V0 < B.V0; });
			// Pieces between the openings (full height), and over each opening (from its head up).
			double V = VFront;
			const double Skin = H.bInteriorPlaster ? Plaster : 0.0;
			const double BrickU0 = bEnd ? U0 + Skin : U0, BrickU1 = bEnd ? U1 : U1 - Skin;
			auto Piece = [&](double VA, double VB, double Z0)
			{
				if (VB - VA < 0.01) { return; }
				// The base course (下碱), 1.5 cm proud outside, then the wall.
				if (Z0 < ZBase)
				{
					const double BU0 = bEnd ? BrickU0 : BrickU0 - 0.015, BU1 = bEnd ? BrickU1 + 0.015 : BrickU1;
					Kit::LBox(Out[H.BaseCourse], TEXT("Wall base"), L, BU0, BU1, VA - (VA <= VFront + 1e-6 ? 0.015 : 0.0), VB + (VB >= VBack - 1e-6 ? 0.015 : 0.0), Z0, ZBase);
				}
				WallPrism(Out[H.WallPart], TEXT("Gable wall"), L, BrickU0, BrickU1, VA, VB, FMath::Max(Z0, ZBase), Top);
				if (Skin > 0.0)
				{
					const double PU0 = bEnd ? U0 : U1 - Skin, PU1 = bEnd ? U0 + Skin : U1;
					// Only where it faces a room (behind the lattice line).
					const double RA = FMath::Max(VA, H.FacadeV + 0.05), RB = FMath::Min(VB, H.bBackWall ? H.BackWallV0 : VB);
					if (RB - RA > 0.01) { WallPrism(Out[CB::PlasterRoom], TEXT("Wall plaster"), L, PU0, PU1, RA, RB, FMath::Max(Z0, H.Floor), Top); }
					// Elsewhere (a veranda's end) the brick shows: close the gap with brick.
					if (RA > VA + 0.01) { WallPrism(Out[H.WallPart], TEXT("Gable wall"), L, PU0, PU1, VA, FMath::Min(RA, VB), FMath::Max(Z0, ZBase), Top); }
					if (RA > VA + 0.01 && Z0 < ZBase) { Kit::LBox(Out[H.BaseCourse], TEXT("Wall base"), L, PU0, PU1, VA, FMath::Min(RA, VB), Z0, ZBase); }
				}
			};
			for (const FGableOpening& O : Ops)
			{
				Piece(V, O.V0, H.Floor);
				// Over the opening: brick from its head to the roof.
				const double Head = H.Floor + O.Top;
				WallPrism(Out[H.WallPart], TEXT("Wall over opening"), L, U0, U1, O.V0, O.V1, Head, Top);
				if (O.bFrame)
				{
					// A red frame: head, jambs (in the opening's faces), threshold stone.
					Kit::LBox(Out[H.TimberPart], TEXT("Opening head"), L, U0 - 0.01, U1 + 0.01, O.V0, O.V1, Head - 0.1, Head);
					Kit::LBox(Out[H.TimberPart], TEXT("Opening jamb"), L, U0 - 0.01, U1 + 0.01, O.V0, O.V0 + 0.08, H.Floor, Head - 0.1);
					Kit::LBox(Out[H.TimberPart], TEXT("Opening jamb"), L, U0 - 0.01, U1 + 0.01, O.V1 - 0.08, O.V1, H.Floor, Head - 0.1);
				}
				V = O.V1;
			}
			Piece(V, VBack, H.Floor);
			// The corbelled front end (墀头): a stone at the base's top, then the courses stepping out under the eave.
			const double Zt = G.Zc - 0.35;
			const double UO = bEnd ? U1 : U0, SideOut = bEnd ? 1.0 : -1.0;
			Kit::LBox(Out[CB::StoneKerb], TEXT("Corner stone"), L, FMath::Min(U0, U1) - 0.02, FMath::Max(U0, U1) + 0.02, VFront - 0.02, VFront + 0.55, ZBase - 0.12, ZBase);
			for (int32 k = 0; k < 5; ++k)
			{
				const double Z0 = Zt + 0.085 * k, Z1 = Z0 + 0.085;
				const double Reach = 0.055 * (k + 1);
				const double Side = 0.02 * (k + 1);
				Kit::LBox(Out[CB::BrickFine], TEXT("Corbel"), L, FMath::Min(U0, U1) - (bEnd ? 0.0 : Side), FMath::Max(U0, U1) + (bEnd ? Side : 0.0),
						  VFront - Reach, VFront + 0.4, Z0, Z1);
			}
			// The corbels' top board (戗檐), slanted up to the eave's underside.
			const double ZTop = Zt + 0.425;
			const double VReach = VFront - 0.3;
			TArray<FVector2D> CorbelBoard = {FVector2D(VReach, ZTop), FVector2D(VFront + 0.4, ZTop), FVector2D(VFront + 0.4, G.Under(VFront + 0.4) - 0.003),
										FVector2D(VReach - 0.05, G.Under(VReach - 0.05) - 0.003)};
			Kit::Extrude(Out[CB::BrickFine], TEXT("Corbel board"), CorbelBoard, L.P(0.0, 0.0, 0.0), L.V3(), FVector::UpVector, L.U3(),
						 FMath::Min(U0, U1) - (bEnd ? 0.0 : 0.12), FMath::Max(U0, U1) + (bEnd ? 0.12 : 0.0));
			(void)UOut; (void)UIn; (void)InnerFace; (void)UO; (void)SideOut;
		}

		void BackWall(FChMeshes& Out, const FHallSpec& H, const FHallGeo& G)
		{
			if (!H.bBackWall) { return; }
			const FLocal& L = H.L;
			const double A = H.bGableWalls && !H.bStartOpen ? H.U0 + H.GableT : H.U0;
			const double B = H.bGableWalls && !H.bEndOpen ? H.U1 - H.GableT : H.U1;
			const double V0 = H.BackWallV0, V1 = H.BackWallV1;
			const double Skin = H.bInteriorPlaster ? Plaster : 0.0;
			const double ZTop = G.Under(V1) - 0.002;
			const double ZBase = H.Floor + BaseCourseH;
			Kit::LBox(Out[H.BaseCourse], TEXT("Back wall base"), L, A, B, V0 + Skin, V1 + 0.015, H.Floor - 0.05, ZBase);
			// The wall rises under the roof's slope (its top follows the boarding across its thickness).
			TArray<FVector2D> Sec = {FVector2D(V0 + Skin, ZBase), FVector2D(V1, ZBase), FVector2D(V1, ZTop), FVector2D(V0 + Skin, G.Under(V0 + Skin) - 0.002)};
			Kit::Extrude(Out[H.WallPart], TEXT("Back wall"), Sec, L.P(0.0, 0.0, 0.0), L.V3(), FVector::UpVector, L.U3(), A, B);
			if (Skin > 0.0)
			{
				TArray<FVector2D> PSec = {FVector2D(V0, H.Floor), FVector2D(V0 + Skin, H.Floor), FVector2D(V0 + Skin, G.Under(V0 + Skin) - 0.002), FVector2D(V0, G.Under(V0) - 0.002)};
				Kit::Extrude(Out[CB::PlasterRoom], TEXT("Back wall plaster"), PSec, L.P(0.0, 0.0, 0.0), L.V3(), FVector::UpVector, L.U3(), A, B);
			}
			// The sealed eave's corbelled cornice (冰盘檐): four courses stepping out under the roof's back edge.
			if (H.bSealedBack)
			{
				const double Z1 = G.Under(V1);
				for (int32 k = 0; k < 4; ++k)
				{
					const double Za = Z1 - 0.07 * (4 - k), Zb = Za + 0.07;
					const double Reach = FMath::Min(H.EaveBack - 0.01, 0.06 * (k + 1));
					Kit::LBox(Out[CB::BrickFine], TEXT("Cornice course"), L, H.U0, H.U1, V1 - 0.1, V1 + Reach, Za, FMath::Min(Zb, G.Under(V1 + Reach) - 0.001));
				}
			}
		}

		/** The front: frames, doors, windows, sill walls, transoms, in each bay on the facade line. */
		void Front(FChMeshes& Out, const FHallSpec& H, const FHallGeo& G)
		{
			const FLocal& L = H.L;
			const double V = H.FacadeV;
			const int32 I = H.PurlinV.IndexOfByPredicate([&](double P) { return FMath::Abs(P - V) < 0.02; });
			const double ColTop = V > 0.01 ? (I != INDEX_NONE ? G.Seat[I] - SeatOver - 2.0 * G.PurlinR - 0.05 : G.Zc + 0.3) : G.Zc;
			const double Head = ColTop - (0.9 * H.ColD + 0.04);        // the lintel's underside
			const double ColR = 0.5 * (H.ColD + (V > 0.01 ? 0.03 : 0.0));
			const int32 FramePart = H.TimberPart;
			const ELattice BasePattern = H.Lattice >= 0 ? ELattice(H.Lattice) : (H.bPainted ? ELattice::StepBrocade : ELattice::Lantern);
			for (int32 i = 0; i + 1 < H.ColU.Num() && i < H.Bays.Num(); ++i)
			{
				const EBay Bay = H.Bays[i];
				const ELattice Pattern = Bay == EBay::DoorsOpen && H.LatticeOpenBay >= 0 ? ELattice(H.LatticeOpenBay) : BasePattern;
				if (Bay == EBay::Open) { continue; }
				const bool bFirstInWall = i == 0 && H.bGableWalls && !H.bStartOpen;
				const bool bLastInWall = i + 2 == H.ColU.Num() && H.bGableWalls && !H.bEndOpen;
				const double A = bFirstInWall ? H.U0 + H.GableT : H.ColU[i] + ColR;
				const double B = bLastInWall ? H.U1 - H.GableT : H.ColU[i + 1] - ColR;
				const double F = H.Floor;
				if (Bay == EBay::Wall)
				{
					Kit::LBox(Out[H.BaseCourse], TEXT("Bay wall base"), L, A, B, V - 0.2, V + 0.2, F - 0.05, F + BaseCourseH);
					Kit::LBox(Out[H.WallPart], TEXT("Bay wall"), L, A, B, V - 0.185, V + 0.185 - Plaster, F + BaseCourseH, Head);
					Kit::LBox(Out[CB::PlasterRoom], TEXT("Bay wall plaster"), L, A, B, V + 0.185 - Plaster, V + 0.185, F, Head);
					continue;
				}
				// The frame (槛框): posts against the columns, the head rail under the lintel.
				constexpr double Post = 0.1, FrameT = 0.1;
				Kit::LBox(Out[FramePart], TEXT("Frame post"), L, A, A + Post, V - 0.5 * FrameT, V + 0.5 * FrameT, F, Head);
				Kit::LBox(Out[FramePart], TEXT("Frame post"), L, B - Post, B, V - 0.5 * FrameT, V + 0.5 * FrameT, F, Head);
				Kit::LBox(Out[FramePart], TEXT("Head rail"), L, A + Post, B - Post, V - 0.5 * FrameT, V + 0.5 * FrameT, Head - 0.1, Head);
				const double IA = A + Post, IB = B - Post;
				// A door clears 1.95 m over its sill (the visitor stands 1.8 m): in the low rows (columns of 2.5 m, no
				// veranda) there's no room left for a transom, and the leaves run up to the head rail.
				double DoorTop = FMath::Max(FMath::Min(F + DoorHead, Head - 0.45), F + 0.1 + 1.95);
				const bool bTransom = Head - 0.1 - (DoorTop + 0.1) > 0.18;
				if (!bTransom) { DoorTop = Head - 0.1; }
				if (Bay == EBay::Doors || Bay == EBay::DoorsOpen)
				{
					// The sill (下槛), the door head (中槛), the transom (横披) of lattice panels between it and the head rail.
					Kit::LBox(Out[FramePart], TEXT("Sill"), L, IA, IB, V - 0.06, V + 0.06, F, F + 0.1);
					if (bTransom) { Kit::LBox(Out[FramePart], TEXT("Door head"), L, IA, IB, V - 0.06, V + 0.06, DoorTop, DoorTop + 0.1); }
					const int32 NT = FMath::Max(1, FMath::RoundToInt32((IB - IA) / 0.95));
					for (int32 t = 0; t < NT && bTransom; ++t)
					{
						const double TA = IA + (IB - IA) * t / NT, TB = IA + (IB - IA) * (t + 1) / NT;
						LatticePanel(Out, L, TA + 0.01, V - 0.02, DoorTop + 0.1, TB - TA - 0.02, Head - 0.1 - DoorTop - 0.1, Pattern, FramePart, true);
						if (t > 0) { Kit::LBox(Out[FramePart], TEXT("Transom mullion"), L, TA - 0.03, TA + 0.03, V - 0.05, V + 0.05, DoorTop + 0.1, Head - 0.1); }
					}
					// Leaves: four in a wide bay, two in a narrow one; the middle pair swung in when the bay is open.
					const int32 NL = (IB - IA) > 2.2 ? 4 : 2;
					const double LW = (IB - IA) / NL;
					for (int32 k = 0; k < NL; ++k)
					{
						const double LA = IA + LW * k;
						const bool bOpen = Bay == EBay::DoorsOpen && (k == NL / 2 - 1 || k == NL / 2);
						if (!bOpen)
						{
							DoorLeaf(Out, L, LA + 0.003, V - 0.035, F + 0.1, LW - 0.006, DoorTop - F - 0.1, FramePart, Pattern);
						}
						else
						{
							// Swung 90° into the room about its outer stile: the leaf now runs along v.
							const bool bLeft = k == NL / 2 - 1;
							const double Hinge = bLeft ? LA + 0.003 : LA + LW - 0.003;
							FLocal Leaf;
							Leaf.Origin = L.Plan(Hinge, V + 0.04);
							Leaf.U = L.V;
							Leaf.V = L.U * (bLeft ? 1.0 : -1.0);
							DoorLeaf(Out, Leaf, 0.0, 0.0, F + 0.1, LW - 0.006, DoorTop - F - 0.1, FramePart, Pattern);
						}
					}
					// Brass pulls on the closed middle leaves (and knockers' plates on the gate are the gate's own).
				}
				else if (Bay == EBay::Windows)
				{
					// The sill wall (槛墙) of rubbed brick, its board (榻板), the window frame, 支摘窗 in two columns.
					const double ZS = F + SillH;
					Kit::LBox(Out[CB::BrickFine], TEXT("Sill wall"), L, IA, IB, V - 0.17, V + 0.17, F - 0.05, ZS - 0.06);
					Kit::LBox(Out[FramePart], TEXT("Sill board"), L, IA - 0.01, IB + 0.01, V - 0.2, V + 0.2, ZS - 0.06, ZS);
					Kit::LBox(Out[FramePart], TEXT("Window sill"), L, IA, IB, V - 0.05, V + 0.05, ZS, ZS + 0.08);
					const double Mid = 0.5 * (IA + IB);
					Kit::LBox(Out[FramePart], TEXT("Window post"), L, Mid - 0.04, Mid + 0.04, V - 0.05, V + 0.05, ZS + 0.08, Head - 0.1);
					const double ZW0 = ZS + 0.08, ZW1 = Head - 0.1;
					const double Split = ZW0 + 0.42 * (ZW1 - ZW0);
					for (const int32 C : {0, 1})
					{
						const double CA = C == 0 ? IA : Mid + 0.04, CB2 = C == 0 ? Mid - 0.04 : IB;
						Kit::LBox(Out[FramePart], TEXT("Window rail"), L, CA, CB2, V - 0.045, V + 0.045, Split - 0.03, Split + 0.03);
						LatticePanel(Out, L, CA + 0.005, V - 0.025, ZW0 + 0.005, CB2 - CA - 0.01, Split - 0.03 - ZW0 - 0.01, ELattice::Grid, FramePart, true);
						LatticePanel(Out, L, CA + 0.005, V - 0.025, Split + 0.035, CB2 - CA - 0.01, ZW1 - Split - 0.04, Pattern, FramePart, true);
					}
				}
			}
		}

		/** The veranda's ceiling line and the gable wall's inside face over a veranda are open; nothing else. */
	}
	using namespace HallBuild;

	void BuildHall(FChMeshes& Out, const FHallSpec& H)
	{
		const FHallGeo G = MakeGeo(H);
		if (H.bPlatform) { Platform(Out, H); }
		Steps(Out, H);
		if (H.bColumns) { Columns(Out, H, G); }
		Frame(Out, H, G);
		Rafters(Out, H, G);
		Roof(Out, H, G);
		if (H.bGableWalls)
		{
			if (!H.bStartOpen) { GableWall(Out, H, G, false); }
			if (!H.bEndOpen) { GableWall(Out, H, G, true); }
		}
		BackWall(Out, H, G);
		Front(Out, H, G);
	}
}
