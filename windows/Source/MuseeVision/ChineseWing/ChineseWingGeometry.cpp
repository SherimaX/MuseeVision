#include "ChineseWing/ChineseWingGeometry.h"

#include "Plan/MuseePlan.h"

/**
 * The Chinese Wing's architecture (Shared/Wings/ChineseWing.swift, ChinesePlan; plan/README.md), rebuilt as real
 * construction. A named namespace: the module builds in unity files.
 *
 * - The court's walls, whitewashed, closed on every face, 10 cm into the ground; the moon gate cut through the north
 *   wall (a grey stone ring lines it and stands 2.5 cm proud of both faces; a stone sill).
 * - The vestibule from the Rotunda's south door (the drum's outer face) to the north wall: plaster walls and ceiling,
 *   and a panel that closes the door's arch above the ceiling; its floor meets the sun clock's 128-gon vertex for vertex.
 * - The cloister: timber columns on stone drum bases along the garden's granite kerb; eave purlins and beams with
 *   brackets (雀替) and hanging fretwork (挂落) between the columns; tie beams curving up to the wall, a middle purlin, a
 *   wall plate, valley beams at the south corners; rafters at 0.32 m under dark timber boarding; seat rails (美人靠)
 *   in the bays along the garden. The roofs are single slopes from the walls (ChinesePlan.roofUnder) meeting in
 *   valleys at the south corners; where the west and east walks end at the north wall their eaves turn up.
 * - Real tiles on a mortar bed: concave pans and convex covers in rows 0.32 m apart, stepped in courses of 0.32 m,
 *   drip tiles and round end tiles at the eaves, cut on the valleys; a mortar fillet where they meet the walls.
 * - The walls' copings: a corbel course, a small gabled tile roof with a ridge, hips at the outer corners.
 * - Grey granite paving, the garden's gravel, the pond's bank and basin, irregular edging stones.
 * - The ceramics' drum plinths, the table cases (handscroll and Orchid Pavilion), the picture rail, the terrace benches.
 *
 * Every piece is a closed solid; where two meet one runs into the other (or they share vertices), and parallel faces
 * are at least a centimetre apart. The roofs run 5 cm into the walls, so no light comes in under them.
 */
namespace ChineseWingBuild
{
	namespace CW = MuseePlan::ChineseWing;
	namespace Kit = ChineseWingKit;
	using Kit::FFrame;
	using Kit::FMeshData;
	using Kit::FPart;
	using Kit::FProfile;

	const FVector Zenith(0, 0, 1);

	// ---- The court.
	constexpr double OuterX0 = CW::X0 - CW::Wall, OuterX1 = CW::X1 + CW::Wall;
	constexpr double OuterY0 = CW::Y0 - CW::Wall, OuterY1 = CW::Y1 + CW::Wall;
	constexpr double WallFoot = -0.1;               // the walls stand 10 cm into the ground
	constexpr double IntoWall = 0.05;               // floors, roofs and beams run this far into the walls
	constexpr double Diagonal = CW::X1 + CW::Y0;    // 23.5: the corners' mitre lines are x ± y = ±23.5

	// ---- The moon gate: the wall's cut (inside the stone), the ring and its feet under the floor.
	constexpr double GateHole = 1.86;
	constexpr double GateRingOut = 1.97, GateRingProud = 0.025, GateRingFoot = -0.05;
	constexpr int32 GateStations = 96;
	constexpr int32 GateRingStations = 120;

	// ---- The garden's kerb under the column lines, and where the paving and the gravel meet under it.
	constexpr double KerbHalf = 0.15, KerbTop = 0.06, KerbNorth0 = 17.9, KerbNorth1 = 18.15;
	constexpr double GardenNorthY = 18.025;
	constexpr double EdgingFoot = -0.07;            // the edging stones' undersides (under the gravel; over the water, seen)

	// ---- The cloister's section: d from the wall's inner face towards the garden.
	constexpr double SlabEdgeD = 5.1, ColumnD = 4.5, MidPurlinD = 2.25;
	constexpr double SlabThickness = 0.08;           // boarding 3 cm, the tiles' bed 5 cm
	constexpr double RafterWidth = 0.065, RafterDepth = 0.075, RafterSink = 0.01, RafterEndD = 5.07;
	constexpr double PurlinRadius = 0.09, PurlinDrop = 0.145;           // its centre under the boarding
	constexpr double MidPurlinRadius = 0.06, MidPurlinDrop = 0.115;
	constexpr double BeamWidth = 0.11, BeamDepth = 0.22, BeamDrop = 0.305;  // centre; top 5 cm into the purlin
	constexpr double BeamUnderDrop = 0.415;                              // the beam's underside
	constexpr double TieWidth = 0.10, TieDepth = 0.16, TieTopDrop = 0.06;
	constexpr double PlateDrop = 0.115, PlateD = 0.03;
	constexpr double ColumnRadius = 0.14, ColumnFoot = 0.20;
	constexpr double EaveOvershoot = 0.12;                               // purlins and beams run past the corner columns

	// ---- The north ends of the west and east walks turn up: most at the eave, at the wall.
	constexpr double LiftMax = 0.55, LiftFromS = 15.8, LiftLength = 2.3, LiftFromD = 1.2;

	// ---- Tiles.
	constexpr double TilePitch = 0.32, CourseLength = 0.32, LipRise = 0.012, LipRun = 0.004;
	constexpr double PanHalf = 0.13, CoverHalf = 0.08, CoverSlack = 0.012;
	constexpr double CloisterTilesFrom = -0.04, CloisterTilesTo = 5.14;

	// ---- The walls' copings (a across the wall from its centre line).
	constexpr double CopingHalf = 0.5, CopingUnder = 6.02, CopingEave = 6.06, CopingRise = 0.20;
	constexpr double CopingTilesFrom = -0.02, CopingTilesTo = 0.53;
	constexpr double CorbelBottom = 5.90, CorbelTop = 6.04, CorbelOut = 0.06;

	// ------------------------------------------------------------------------------------------------ profiles

	/** The boarding's underside at d from the wall (ChinesePlan.roofUnder, a smooth cubic through the plan's table). */
	double Under(double D)
	{
		constexpr int32 Count = 17;
		const double* X = CW::RoofProfileD;
		const double* Y = CW::RoofUnderside;
		auto Tangent = [X, Y](int32 Index)
		{
			if (Index == 0) { return (Y[1] - Y[0]) / (X[1] - X[0]); }
			if (Index == Count - 1) { return (Y[Count - 1] - Y[Count - 2]) / (X[Count - 1] - X[Count - 2]); }
			return (Y[Index + 1] - Y[Index - 1]) / (X[Index + 1] - X[Index - 1]);
		};
		if (D <= X[0]) { return Y[0] + Tangent(0) * (D - X[0]); }
		if (D >= X[Count - 1]) { return Y[Count - 1] + Tangent(Count - 1) * (D - X[Count - 1]); }
		int32 Seg = 0;
		while (Seg < Count - 2 && D > X[Seg + 1]) { ++Seg; }
		const double H = X[Seg + 1] - X[Seg], T = (D - X[Seg]) / H;
		const double T2 = T * T, T3 = T2 * T;
		return (2 * T3 - 3 * T2 + 1) * Y[Seg] + (T3 - 2 * T2 + T) * H * Tangent(Seg) + (-2 * T3 + 3 * T2) * Y[Seg + 1] + (T3 - T2) * H * Tangent(Seg + 1);
	}

	/** How far the roof turns up at the north ends of the west and east walks (s along the walk = plan y). */
	double Lift(double D, double S)
	{
		const double G = FMath::Clamp((LiftFromS - S) / LiftLength, 0.0, 1.0);
		const double F = FMath::Clamp((D - LiftFromD) / (SlabEdgeD - LiftFromD), 0.0, 1.0);
		return LiftMax * G * G * F * F;
	}

	/** The copings' tile bed across the wall: a small gable, 6.26 m at the ridge down to 6.06 m at the eaves. */
	double CopingTop(double A)
	{
		const double X = FMath::Clamp(FMath::Abs(A) / CopingHalf, 0.0, 1.0);
		return CopingEave + CopingRise * FMath::Pow(1.0 - X, 1.3);
	}

	/** A repeatable number in [0, 1) for stone I, facet K. */
	double Hash01(int32 I, int32 K)
	{
		const double V = FMath::Sin(I * 12.9898 + K * 78.233 + 0.5) * 43758.5453;
		return V - FMath::FloorToDouble(V);
	}

	/** Clips the line P0 + Dir t, t in [Lo, Hi], to the half-planes c·p ≤ k (X, Y: c; Z: k). */
	void ClipLine(const TArray<FVector>& Limits, const FVector2D& P0, const FVector2D& Dir, double& Lo, double& Hi)
	{
		for (const FVector& Lim : Limits)
		{
			const FVector2D C(Lim.X, Lim.Y);
			const double CD = FVector2D::DotProduct(C, Dir), Rhs = Lim.Z - FVector2D::DotProduct(C, P0);
			if (CD > 1e-12) { Hi = FMath::Min(Hi, Rhs / CD); }
			else if (CD < -1e-12) { Lo = FMath::Max(Lo, Rhs / CD); }
			else if (Rhs < 0.0) { Hi = Lo - 1.0; }
		}
	}

	// ------------------------------------------------------------------------------------------------ the walks

	/** One walk of the cloister: d from its wall towards the garden, s along it. */
	struct FWalk
	{
		FString Name;
		FVector2D Origin = FVector2D::ZeroVector;   // on the wall's inner face at s = 0
		FVector2D DDir = FVector2D(1, 0);
		FVector2D SDir = FVector2D(0, 1);
		double S0 = 0.0, S1 = 0.0;                  // the roof's extent along s (into the walls)
		double EaveS0 = 0.0, EaveS1 = 0.0;          // the eave purlin's and beam's
		TArray<FVector> Limits;                     // the valleys: c·p ≤ k
		bool bLifted = false;
		double Deeper = 0.0;                        // deeper eave beam and wall plate: where two cross at a corner their undersides differ
		TArray<double> Columns;
		TArray<double> Ties;
		TArray<TPair<double, double>> FretBays;
		TArray<TPair<double, double>> SeatBays;

		FVector2D Plan(double D, double S) const { return Origin + DDir * D + SDir * S; }
		FVector D3() const { return FVector(DDir.X, DDir.Y, 0.0); }
		FVector S3() const { return FVector(SDir.X, SDir.Y, 0.0); }
		/** The boarding's underside. */
		double Board(double D, double S) const { return Under(D) + (bLifted ? Lift(D, S) : 0.0); }
		FVector At(double D, double S, double Up) const
		{
			const FVector2D P = Plan(D, S);
			return FVector(P.X, P.Y, Board(D, S) + Up);
		}
		FVector BoardNormal(double D, double S) const
		{
			constexpr double E = 0.005;
			const double GD = (Board(D + E, S) - Board(D - E, S)) / (2 * E);
			const double GS = (Board(D, S + E) - Board(D, S - E)) / (2 * E);
			const FVector2D G = DDir * GD + SDir * GS;
			return FVector(-G.X, -G.Y, 1.0).GetSafeNormal();
		}
		void SRange(double D, double& Lo, double& Hi) const
		{
			Lo = S0;
			Hi = S1;
			ClipLine(Limits, Plan(D, 0.0), SDir, Lo, Hi);
		}
		void DRange(double S, double& Lo, double& Hi) const { ClipLine(Limits, Plan(0.0, S), DDir, Lo, Hi); }
	};

	TArray<FWalk> MakeWalks()
	{
		TArray<FWalk> Out;
		auto Pairs = [](const double* Values, int32 From, int32 To)
		{
			TArray<TPair<double, double>> P;
			for (int32 k = From; k < To; ++k) { P.Add(TPair<double, double>(Values[k], Values[k + 1])); }
			return P;
		};
		for (const double Side : {-1.0, 1.0})
		{
			FWalk W;
			W.Name = Side < 0 ? TEXT("West walk") : TEXT("East walk");
			W.Origin = FVector2D(Side < 0 ? CW::X0 : CW::X1, 0.0);
			W.DDir = FVector2D(-Side, 0.0);
			W.SDir = FVector2D(0.0, 1.0);
			W.S0 = CW::Y0 - IntoWall;
			W.S1 = CW::Y1 + IntoWall;
			W.EaveS0 = CW::Y0 - IntoWall;
			W.EaveS1 = CW::ColumnLineY + EaveOvershoot;
			W.Limits.Add(FVector(-Side, 1.0, Diagonal));  // west: x + y ≤ 23.5; east: y ∁Ex ≤ 23.5
			W.bLifted = true;
			for (const double Y : CW::ColumnsAlongY) { W.Columns.Add(Y); }
			for (int32 k = 0; k + 1 < 7; ++k) { W.Ties.Add(CW::ColumnsAlongY[k]); }
			W.FretBays = Pairs(CW::ColumnsAlongY, 0, 6);
			W.SeatBays = Pairs(CW::ColumnsAlongY, 1, 6);
			Out.Add(W);
		}
		FWalk S;
		S.Name = TEXT("South walk");
		S.Origin = FVector2D(0.0, CW::Y1);
		S.DDir = FVector2D(0.0, -1.0);
		S.SDir = FVector2D(1.0, 0.0);
		S.S0 = CW::X0 - IntoWall;
		S.S1 = CW::X1 + IntoWall;
		S.EaveS0 = -CW::ColumnLineX - EaveOvershoot;
		S.EaveS1 = CW::ColumnLineX + EaveOvershoot;
		S.Limits.Add(FVector(-1.0, -1.0, -Diagonal));  // x + y ≥ 23.5
		S.Limits.Add(FVector(1.0, -1.0, -Diagonal));   // y ∁Ex ≥ 23.5
		S.bLifted = false;
		S.Deeper = 0.015;
		for (const double X : CW::ColumnsAlongX) { S.Columns.Add(X); }
		for (int32 k = 1; k + 1 < 6; ++k) { S.Ties.Add(CW::ColumnsAlongX[k]); }
		S.FretBays = Pairs(CW::ColumnsAlongX, 0, 5);
		S.SeatBays = Pairs(CW::ColumnsAlongX, 0, 5);
		Out.Add(S);
		return Out;
	}

	/** Stations along a walk from SA to SB: close together where the roof turns up, far apart where it is straight. */
	TArray<double> LongStations(const FWalk& W, double SA, double SB)
	{
		TArray<double> Out;
		Out.Add(SA);
		const double Fine = W.bLifted ? LiftFromS + 0.1 : SA;
		double S = SA;
		while (true)
		{
			S += S < Fine ? 0.1 : (W.bLifted ? 1.0 : 2.0);
			if (S >= SB - 1e-6) { break; }
			Out.Add(S);
		}
		Out.Add(SB);
		return Out;
	}

	/** A box in a walk's frame (the walks are square to the plan). */
	void WalkBox(FPart& Part, const TCHAR* Name, const FWalk& W, double DA, double DB, double SA, double SB, double ZA, double ZB)
	{
		const FVector2D P = W.Plan(DA, SA), Q = W.Plan(DB, SB);
		Kit::Box(Part, Name, FVector(FMath::Min(P.X, Q.X), FMath::Min(P.Y, Q.Y), ZA), FVector(FMath::Max(P.X, Q.X), FMath::Max(P.Y, Q.Y), ZB));
	}

	/** A (NU + 1) ÁE(NV + 1) grid of shared vertices and its quads. */
	void GridPatch(FMeshData& M, int32 NU, int32 NV, TFunctionRef<void(int32, int32, FVector&, FVector&, FVector2D&)> Vert)
	{
		const int32 Base = M.Positions.Num();
		for (int32 i = 0; i <= NU; ++i)
		{
			for (int32 j = 0; j <= NV; ++j)
			{
				FVector P, N;
				FVector2D UV;
				Vert(i, j, P, N, UV);
				M.Vertex(P, N, UV);
			}
		}
		const int32 Row = NV + 1;
		for (int32 i = 0; i < NU; ++i)
		{
			for (int32 j = 0; j < NV; ++j) { M.Quad(Base + i * Row + j, Base + (i + 1) * Row + j, Base + (i + 1) * Row + j + 1, Base + i * Row + j + 1); }
		}
	}

	/** A strip between two polylines (same count), facing N everywhere; U along it, V down it. */
	void Strip(FMeshData& M, const TArray<FVector>& Upper, const TArray<FVector>& Lower, const FVector& N)
	{
		const int32 Base = M.Positions.Num();
		double U = 0.0;
		for (int32 k = 0; k < Upper.Num(); ++k)
		{
			if (k > 0) { U += FVector::Distance(Upper[k], Upper[k - 1]); }
			M.Vertex(Upper[k], N, FVector2D(U, -Upper[k].Z));
			M.Vertex(Lower[k], N, FVector2D(U, -Lower[k].Z));
		}
		for (int32 k = 0; k + 1 < Upper.Num(); ++k) { M.Quad(Base + 2 * k, Base + 2 * k + 2, Base + 2 * k + 3, Base + 2 * k + 1); }
	}

	/** Frames along the straight line A ↁEB with an upright section whose A axis is Left (level). */
	TArray<FFrame> LevelRun(const FVector& A, const FVector& B, const FVector& Left)
	{
		const FVector T = (B - A).GetSafeNormal();
		const bool bFlip = FVector::DotProduct(FVector(-T.Y, T.X, 0.0), Left) < 0.0;
		return Kit::UprightFrames(bFlip ? TArray<FVector>({B, A}) : TArray<FVector>({A, B}));
	}

	// ------------------------------------------------------------------------------------------------ tiles

	/** A slope that carries rows of tiles: d down it, s along it, the bed's top Bed(d, s); rows cut by Limits. */
	struct FSlope
	{
		FVector2D Origin = FVector2D::ZeroVector;
		FVector2D DDir = FVector2D(1, 0);
		FVector2D SDir = FVector2D(0, 1);
		TFunction<double(double, double)> Bed;
		TArray<FVector> Limits;
		double From = 0.0, To = 1.0;
		bool bOrnaments = true;

		FVector D3() const { return FVector(DDir.X, DDir.Y, 0.0); }
		FVector S3() const { return FVector(SDir.X, SDir.Y, 0.0); }
		FVector At(double D, double S, double Up) const
		{
			const FVector2D P = Origin + DDir * D + SDir * S;
			return FVector(P.X, P.Y, Bed(D, S) + Up);
		}
		FVector Normal(double D, double S) const
		{
			constexpr double E = 0.005;
			const double GD = (Bed(D + E, S) - Bed(D - E, S)) / (2 * E);
			const double GS = (Bed(D, S + E) - Bed(D, S - E)) / (2 * E);
			const FVector2D G = DDir * GD + SDir * GS;
			return FVector(-G.X, -G.Y, 1.0).GetSafeNormal();
		}
		double CrossSlope(double D, double S) const
		{
			constexpr double E = 0.005;
			return (Bed(D, S + E) - Bed(D, S - E)) / (2 * E);
		}
	};

	/** A tile's section: (across, height over the bed), counter-clockwise. */
	struct FTileShape
	{
		TArray<FVector2D> P;
		TArray<bool> bTop;      // on its upper face: it steps with the courses
		TArray<bool> bSmooth;
		bool bPan = true;
	};

	/** A pan (concave, laid in the bed): 26 cm across, its edges 3 cm up. */
	FTileShape PanShape()
	{
		FTileShape T;
		T.bPan = true;
		T.P = {FVector2D(-PanHalf, -0.01), FVector2D(PanHalf, -0.01)};
		T.bTop = {false, false};
		T.bSmooth = {false, false};
		constexpr int32 N = 4;
		for (int32 k = 0; k <= N; ++k)
		{
			const double O = PanHalf * (1.0 - 2.0 * k / N);
			T.P.Add(FVector2D(O, 0.02 + 0.03 * FMath::Square(O / PanHalf)));
			T.bTop.Add(true);
			T.bSmooth.Add(k > 0 && k < N);
		}
		return T;
	}

	/** A cover (convex) over the joint between two pans: 16 cm across, its crown 9.5 cm over the bed. */
	FTileShape CoverShape()
	{
		FTileShape T;
		T.bPan = false;
		T.P = {FVector2D(-CoverHalf, 0.03), FVector2D(CoverHalf, 0.03)};
		T.bTop = {false, false};
		T.bSmooth = {false, false};
		constexpr int32 N = 4;
		for (int32 k = 0; k <= N; ++k)
		{
			const double O = CoverHalf * (1.0 - 2.0 * k / N);
			T.P.Add(FVector2D(O, 0.055 + 0.04 * FMath::Cos(0.5 * UE_DOUBLE_PI * O / CoverHalf)));
			T.bTop.Add(true);
			T.bSmooth.Add(k > 0 && k < N);
		}
		return T;
	}

	/**
	 * One row of tiles down a slope, centred at s = Centre: stepped in courses (a 1.2 cm lip every 0.32 m along the
	 * slope, where the texture's joint falls), each point of the section cut by the limits on its own line (so a row
	 * ends on a valley or hip exactly), capped at both ends, with a drip tile or an end tile where it reaches the eave.
	 */
	void TileRow(FPart& Part, const FSlope& Slope, double Centre, const FTileShape& Shape)
	{
		const int32 NP = Shape.P.Num();
		// A cover runs 1.2 cm past the pans at the eave and over a valley or hip, so their cut ends never share a plane.
		const double Slack = Shape.bPan ? 0.0 : CoverSlack;
		const double From = Slope.From - Slack, To = Slope.To + Slack;
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
		if (Longest < 0.08) { return; }

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
		auto Step = [&](double D)
		{
			const double Phi = FMath::Fmod(FMath::Max(ArcAt(D), 0.0), CourseLength);
			return Phi < CourseLength - LipRun ? LipRise * Phi / (CourseLength - LipRun) : LipRise * (CourseLength - Phi) / LipRun;
		};

		// Stations down the row: each course's lip twice over (the top's normals turn crisply at it).
		struct FStation
		{
			double D;
			bool bLip;
		};
		TArray<FStation> Stations;
		Stations.Add({From, false});
		const double Total = AL.Last();
		for (int32 m = 1; m * CourseLength < Total - 0.02; ++m)
		{
			const double B0 = DAt(m * CourseLength - LipRun), B1 = DAt(m * CourseLength);
			Stations.Add({B0, false});
			Stations.Add({B0, true});
			Stations.Add({B1, true});
			Stations.Add({B1, false});
			// A cut never ends inside a lip: it moves to the lip's top (or, for a start, its foot).
			for (int32 j = 0; j < NP; ++j)
			{
				if (Hi[j] > B0 && Hi[j] < B1) { Hi[j] = B0; }
				if (Lo[j] > B0 && Lo[j] < B1) { Lo[j] = B1; }
			}
		}
		Stations.Add({To, false});
		// The sides and underside need each distinct station once.
		TArray<int32> Plain;
		for (int32 k = 0; k < Stations.Num(); ++k)
		{
			if (!Stations[k].bLip && (k == 0 || Stations[k].D > Stations[Plain.Last()].D + 1e-9)) { Plain.Add(k); }
			else if (Stations[k].bLip && Stations[k - 1].bLip) { Plain.Add(k); }   // the lip's foot
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
		Part.Begin(Shape.bPan ? TEXT("Pan row") : TEXT("Cover row"));
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
				M.Vertex(PointAt(J0, K), NormalAt(J0, K, N0), FVector2D(Centre + Shape.P[J0].X, ArcAt(DOf(J0, K))));
				M.Vertex(PointAt(J1, K), NormalAt(J1, K, N1), FVector2D(Centre + Shape.P[J1].X, ArcAt(DOf(J1, K))));
			}
			for (int32 k = 0; k + 1 < Ks.Num(); ++k)
			{
				const int32 V = Base + 2 * k;
				M.Quad(V, V + 1, V + 3, V + 2);
			}
		}
		// The ends: square to the row, or on the valley or hip that cut it.
		auto Cap = [&](int32 K, bool bEnd)
		{
			FVector N = bEnd ? D3 : -D3;
			for (int32 j = 0; j < NP; ++j)
			{
				const FVector2D& Cut = bEnd ? HiCut[j] : LoCut[j];
				if (!Cut.IsNearlyZero()) { N = FVector(Cut.X, Cut.Y, 0.0).GetSafeNormal(); break; }
			}
			// Triangulated in the cap's own plane (along a slanted cut the courses' lips shear the section).
			const FVector Across = FVector::CrossProduct(N, Zenith).GetSafeNormal();
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

		// At the eave: a drip tile under a pan, a round end tile on a cover.
		bool bReachesEave = Slope.bOrnaments;
		for (int32 j = 0; j < NP && bReachesEave; ++j) { bReachesEave = HiCut[j].IsNearlyZero() && Hi[j] >= To - 1e-9; }
		if (!bReachesEave) { return; }
		const double Tilt = Slope.CrossSlope(Slope.To, Centre);
		if (Shape.bPan)
		{
			// 5 mm wider than the pan each side, so its sides never lie in the pan's.
			constexpr double Wider = 1.0 + 0.005 / PanHalf;
			TArray<FVector2D> Drip;
			for (int32 j = 0; j < NP; ++j) { if (Shape.bTop[j]) { Drip.Add(FVector2D(Shape.P[j].X * Wider, Shape.P[j].Y + 0.004)); } }
			const double HL = Drip.Last().Y, HR = Drip[0].Y;
			Drip.Append({FVector2D(-PanHalf * Wider, HL - 0.038), FVector2D(-0.055, -0.035), FVector2D(0.0, -0.078), FVector2D(0.055, -0.035),
						 FVector2D(PanHalf * Wider, HR - 0.038)});
			Kit::Extrude(Part, TEXT("Drip tile"), Drip, Slope.At(Slope.To, Centre, 0.0), (S3 + Zenith * Tilt).GetSafeNormal(), Zenith, D3, -0.004, 0.010);
		}
		else
		{
			// In front of the pans' drips (which reach 1 cm past the eave line), the cover's end inside it.
			FProfile Disc;
			Disc.Add(0.0, 0.0).Add(0.085, 0.0).Add(0.085, 0.016).Add(0.0, 0.016);
			Kit::Lathe(Part, TEXT("End tile"), Slope.At(Slope.To + 0.006, Centre, 0.05), D3, Disc, 16, 0.085);
		}
	}

	/** Rows of pans and covers along a slope from SA to SB (a cover every 0.32 m of plan from s = 0, a pan between). */
	void TileRows(FPart& Part, const FSlope& Slope, double SA, double SB)
	{
		const FTileShape Pan = PanShape(), Cover = CoverShape();
		for (int32 k = FMath::FloorToInt32(SA / TilePitch) - 1; k <= FMath::CeilToInt32(SB / TilePitch) + 1; ++k)
		{
			const double Base = k * TilePitch;
			if (Base + 0.08 + CoverHalf > SA && Base + 0.08 - CoverHalf < SB) { TileRow(Part, Slope, Base + 0.08, Cover); }
			if (Base + 0.24 + PanHalf > SA && Base + 0.24 - PanHalf < SB) { TileRow(Part, Slope, Base + 0.24, Pan); }
		}
	}

	// ------------------------------------------------------------------------------------------------ walls and gate

	/** The gate's circle of radius R in the (x, z) plane cut at z = Cut: from the left foot over the top to the right foot. */
	TArray<FVector2D> GateArc(double R, double Cut, TArray<double>* OutPhi = nullptr)
	{
		const double Cz = CW::MoonGateCentreHeight;
		const double PhiRight = FMath::Asin(FMath::Clamp((Cut - Cz) / R, -1.0, 1.0));
		const double PhiLeft = UE_DOUBLE_PI - PhiRight;
		TArray<double> Phis;
		Phis.Add(PhiLeft);
		const double StepAngle = 2.0 * UE_DOUBLE_PI / GateStations;
		for (int32 k = GateStations; k >= -GateStations; --k)
		{
			const double Phi = k * StepAngle;
			if (Phi < PhiLeft - 1e-6 && Phi > PhiRight + 1e-6) { Phis.Add(Phi); }
		}
		Phis.Add(PhiRight);
		TArray<FVector2D> Out;
		for (const double Phi : Phis) { Out.Add(FVector2D(R * FMath::Cos(Phi), Cz + R * FMath::Sin(Phi))); }
		if (OutPhi) { *OutPhi = Phis; }
		return Out;
	}

	/** The court's four walls as one closed solid, the moon gate cut through the north one. */
	void BuildWalls(FPart& Part)
	{
		Part.Begin(TEXT("Court walls"));
		FMeshData& M = Part.M;
		const double Top = CW::WallHeight;
		auto Vertical = [&M](const TArray<FVector2D>& Outline, const FVector& Origin, const FVector& Along, const FVector& N)
		{
			Kit::Planar(M, Outline, {}, Origin, Along, Zenith, N, [N](const FVector& P) { return SalonKit::FaceUV(P, N); });
		};
		auto Rect2 = [](double A0, double A1, double Z0, double Z1)
		{
			return TArray<FVector2D>({FVector2D(A0, Z0), FVector2D(A1, Z0), FVector2D(A1, Z1), FVector2D(A0, Z1)});
		};
		TArray<double> Phis;
		const TArray<FVector2D> Arc = GateArc(GateHole, WallFoot, &Phis);
		// The north wall's two faces, round the gate.
		TArray<FVector2D> NorthIn = {FVector2D(CW::X0, WallFoot)};
		NorthIn.Append(Arc);
		NorthIn.Append({FVector2D(CW::X1, WallFoot), FVector2D(CW::X1, Top), FVector2D(CW::X0, Top)});
		Vertical(NorthIn, FVector(0, CW::Y0, 0), FVector(1, 0, 0), FVector(0, 1, 0));
		TArray<FVector2D> NorthOut = {FVector2D(OuterX0, WallFoot)};
		NorthOut.Append(Arc);
		NorthOut.Append({FVector2D(OuterX1, WallFoot), FVector2D(OuterX1, Top), FVector2D(OuterX0, Top)});
		Vertical(NorthOut, FVector(0, OuterY0, 0), FVector(1, 0, 0), FVector(0, -1, 0));
		// The other faces.
		Vertical(Rect2(CW::Y0, CW::Y1, WallFoot, Top), FVector(CW::X0, 0, 0), FVector(0, 1, 0), FVector(1, 0, 0));
		Vertical(Rect2(CW::Y0, CW::Y1, WallFoot, Top), FVector(CW::X1, 0, 0), FVector(0, 1, 0), FVector(-1, 0, 0));
		Vertical(Rect2(CW::X0, CW::X1, WallFoot, Top), FVector(0, CW::Y1, 0), FVector(1, 0, 0), FVector(0, -1, 0));
		Vertical(Rect2(OuterY0, OuterY1, WallFoot, Top), FVector(OuterX0, 0, 0), FVector(0, 1, 0), FVector(-1, 0, 0));
		Vertical(Rect2(OuterY0, OuterY1, WallFoot, Top), FVector(OuterX1, 0, 0), FVector(0, 1, 0), FVector(1, 0, 0));
		Vertical(Rect2(OuterX0, OuterX1, WallFoot, Top), FVector(0, OuterY1, 0), FVector(1, 0, 0), FVector(0, 1, 0));
		// The top and the foot, mitred at the corners; the foot of the north wall is cut by the gate.
		const FVector2D I00(CW::X0, CW::Y0), I10(CW::X1, CW::Y0), I11(CW::X1, CW::Y1), I01(CW::X0, CW::Y1);
		const FVector2D O00(OuterX0, OuterY0), O10(OuterX1, OuterY0), O11(OuterX1, OuterY1), O01(OuterX0, OuterY1);
		for (const bool bUp : {true, false})
		{
			const double Z = bUp ? Top : WallFoot;
			Kit::Level(M, {O00, I00, I01, O01}, {}, Z, bUp);
			Kit::Level(M, {I01, I11, O11, O01}, {}, Z, bUp);
			Kit::Level(M, {I10, O10, O11, I11}, {}, Z, bUp);
		}
		Kit::Level(M, {O00, O10, I10, I00}, {}, Top, true);
		const double Foot = Arc[0].X;   // the left foot's x (negative)
		Kit::Level(M, {O00, FVector2D(Foot, OuterY0), FVector2D(Foot, CW::Y0), I00}, {}, WallFoot, false);
		Kit::Level(M, {FVector2D(-Foot, OuterY0), O10, I10, FVector2D(-Foot, CW::Y0)}, {}, WallFoot, false);
		// The gate's reveal (inside the stone ring).
		for (int32 k = 0; k + 1 < Arc.Num(); ++k)
		{
			auto V = [&](int32 Index, double Y)
			{
				const FVector N(-FMath::Cos(Phis[Index]), 0.0, -FMath::Sin(Phis[Index]));
				return M.Vertex(FVector(Arc[Index].X, Y, Arc[Index].Y), N, FVector2D(GateHole * Phis[Index], Y));
			};
			M.Quad(V(k, OuterY0), V(k, CW::Y0), V(k + 1, CW::Y0), V(k + 1, OuterY0));
		}
	}

	/** Points of a circle of radius R round the gate's centre, evenly from its left foot at z = Cut over the top to its right foot. */
	TArray<FVector2D> GateArcEven(double R, double Cut, int32 N, TArray<double>& OutPhi)
	{
		const double Cz = CW::MoonGateCentreHeight;
		const double PhiRight = FMath::Asin(FMath::Clamp((Cut - Cz) / R, -1.0, 1.0));
		const double PhiLeft = UE_DOUBLE_PI - PhiRight;
		TArray<FVector2D> Out;
		OutPhi.Reset();
		for (int32 k = 0; k <= N; ++k)
		{
			const double Phi = FMath::Lerp(PhiLeft, PhiRight, double(k) / N);
			OutPhi.Add(Phi);
			Out.Add(FVector2D(R * FMath::Cos(Phi), Cz + R * FMath::Sin(Phi)));
		}
		return Out;
	}

	/** The moon gate's grey stone ring: it lines the opening and stands proud of both faces of the wall. */
	void BuildGateRing(FPart& Part)
	{
		Part.Begin(TEXT("Moon gate ring"));
		FMeshData& M = Part.M;
		TArray<double> PhiIn, PhiOut;
		const TArray<FVector2D> In = GateArcEven(CW::MoonGateRadius, GateRingFoot, GateRingStations, PhiIn);
		const TArray<FVector2D> Out = GateArcEven(GateRingOut, GateRingFoot, GateRingStations, PhiOut);
		const double YF = OuterY0 - GateRingProud, YB = CW::Y0 + GateRingProud;
		auto P3 = [](const FVector2D& Q, double Y) { return FVector(Q.X, Y, Q.Y); };
		for (const bool bFront : {true, false})
		{
			const double Y = bFront ? YF : YB;
			const FVector N(0, bFront ? -1 : 1, 0);
			for (int32 k = 0; k < GateRingStations; ++k)
			{
				M.Rect(P3(In[k], Y), P3(Out[k], Y), P3(Out[k + 1], Y), P3(In[k + 1], Y), N);
			}
		}
		for (const bool bInner : {true, false})
		{
			const TArray<FVector2D>& C = bInner ? In : Out;
			const TArray<double>& Phi = bInner ? PhiIn : PhiOut;
			const double R = bInner ? CW::MoonGateRadius : GateRingOut;
			const int32 Base = M.Positions.Num();
			for (int32 k = 0; k <= GateRingStations; ++k)
			{
				const FVector N = FVector(FMath::Cos(Phi[k]), 0.0, FMath::Sin(Phi[k])) * (bInner ? -1.0 : 1.0);
				M.Vertex(P3(C[k], YF), N, FVector2D(R * Phi[k], YF));
				M.Vertex(P3(C[k], YB), N, FVector2D(R * Phi[k], YB));
			}
			for (int32 k = 0; k < GateRingStations; ++k) { M.Quad(Base + 2 * k, Base + 2 * k + 1, Base + 2 * k + 3, Base + 2 * k + 2); }
		}
		for (const int32 K : {0, GateRingStations})
		{
			M.Rect(P3(In[K], YF), P3(Out[K], YF), P3(Out[K], YB), P3(In[K], YB), FVector(0, 0, -1));
		}
	}

	/**
	 * The vestibule: plaster walls either side (0.3 m), the ceiling slab at 4.4 m, and over it the panel that closes the
	 * Rotunda door's arch (its face 0.3 m inside the drum's outer face, which the door's reveal reaches). It starts inside
	 * the drum's wall and runs 5 cm into the court's north wall.
	 */
	void BuildVestibule(FPart& Part)
	{
		const double H = CW::VestibuleHalfWidth, T = 0.3, Arch = 2.1;
		const TArray<double> Xs = {-H - T, -H, -Arch, Arch, H, H + T};
		const TArray<double> Ys = {10.85, 10.9, 11.3, OuterY0 + IntoWall};
		const TArray<double> Zs = {WallFoot + 0.01, CW::VestibuleCeiling, CW::VestibuleCeiling + 0.3, 6.7};
		Kit::GridSolid(Part, TEXT("Vestibule"), Xs, Ys, Zs, [](int32 I, int32 J, int32 K)
		{
			if (I == 0 || I == 4) { return K <= 1; }    // the side walls, up to the slab's top
			if (J == 0) { return false; }               // (inside the drum)
			if (K == 1) { return true; }                // the ceiling slab
			return K == 2 && J == 1 && I == 2;          // the panel in the door's arch
		});
	}

	// ------------------------------------------------------------------------------------------------ ground

	TArray<FVector2D> PondEdge()
	{
		TArray<FVector2D> Out;
		for (int32 i = 0; i < 28; ++i) { Out.Add(FVector2D(CW::Pond[i][0], CW::Pond[i][1])); }
		return Out;
	}

	FVector2D PondCentroid()
	{
		FVector2D C = FVector2D::ZeroVector;
		for (const FVector2D& P : PondEdge()) { C += P; }
		return C / 28.0;
	}

	/** The vestibule's floor, the walks' and terrace's paving, the garden's gravel round the pond: one sheet at z 0. */
	void BuildFloors(FPart& Paving, FPart& Gravel)
	{
		// From the sun clock's edge (its 128-gon, vertex for vertex) to under the court's north wall.
		Paving.Begin(TEXT("Vestibule floor"), false, TEXT("Ground"));
		const double StepAngle = 2.0 * UE_DOUBLE_PI / CW::SunClockSides;
		const int32 Half = FMath::CeilToInt32(FMath::Asin((CW::VestibuleHalfWidth + 0.05) / CW::SunClockRadius) / StepAngle);
		const int32 South = CW::SunClockSides / 4;
		TArray<FVector2D> Vest;
		for (int32 i = South + Half; i >= South - Half; --i)
		{
			Vest.Add(FVector2D(CW::SunClockRadius * FMath::Cos(StepAngle * i), CW::SunClockRadius * FMath::Sin(StepAngle * i)));
		}
		const double XW = Vest[0].X, XE = Vest.Last().X;
		Vest.Add(FVector2D(XE, OuterY0 + IntoWall));
		Vest.Add(FVector2D(XW, OuterY0 + IntoWall));
		Kit::Level(Paving.M, Vest, {}, 0.0, true);

		const double XG = CW::ColumnLineX, YG = CW::ColumnLineY;
		const TArray<FVector2D> Garden = {FVector2D(-XG, GardenNorthY), FVector2D(XG, GardenNorthY), FVector2D(XG, YG), FVector2D(-XG, YG)};
		Paving.Begin(TEXT("Court paving"), false, TEXT("Ground"));
		const TArray<FVector2D> Court = {FVector2D(CW::X0 - IntoWall, CW::Y0 - IntoWall), FVector2D(CW::X1 + IntoWall, CW::Y0 - IntoWall),
										 FVector2D(CW::X1 + IntoWall, CW::Y1 + IntoWall), FVector2D(CW::X0 - IntoWall, CW::Y1 + IntoWall)};
		Kit::Level(Paving.M, Court, {Garden}, 0.0, true);
		Gravel.Begin(TEXT("Garden gravel"), false, TEXT("Ground"));
		Kit::Level(Gravel.M, Garden, {PondEdge()}, 0.0, true);
	}

	/** The pond: its bank from the gravel down through the water's edge to a rounded foot and a level basin floor. */
	void BuildPond(FPart& Bank, FPart& Edging, FPart& Guard)
	{
		const TArray<FVector2D> Edge = PondEdge();
		const FVector2D C = PondCentroid();
		const int32 N = Edge.Num();
		constexpr int32 Rings = 4;
		const double Insets[Rings] = {0.0, CW::WaterInset, 0.26, 0.36};
		const double Depths[Rings] = {0.0, -CW::WaterDepth, -0.58, -0.66};
		auto P = [&](int32 R, int32 I)
		{
			const FVector2D E = Edge[(I % N + N) % N];
			const FVector2D Q = E + (C - E).GetSafeNormal() * Insets[R];
			return FVector(Q.X, Q.Y, Depths[R]);
		};
		TArray<double> Around = {0.0};
		for (int32 i = 1; i <= N; ++i) { Around.Add(Around.Last() + FVector2D::Distance(Edge[i - 1], Edge[i % N])); }
		TArray<double> Down = {0.0};
		for (int32 r = 1; r < Rings; ++r) { Down.Add(Down.Last() + FVector::Distance(P(r - 1, 0), P(r, 0))); }
		Bank.Begin(TEXT("Pond bank"), false, TEXT("Ground"));
		GridPatch(Bank.M, Rings - 1, N, [&](int32 R, int32 I, FVector& Pos, FVector& Nrm, FVector2D& UV)
		{
			Pos = P(R, I);
			const FVector Ta = P(R, I + 1) - P(R, I - 1);
			const FVector Td = P(FMath::Min(R + 1, Rings - 1), I) - P(FMath::Max(R - 1, 0), I);
			Nrm = FVector::CrossProduct(Ta, Td).GetSafeNormal();
			const FVector2D Q(Pos.X, Pos.Y);
			if (FVector::DotProduct(Nrm, FVector(C.X - Q.X, C.Y - Q.Y, 0.0).GetSafeNormal() + Zenith) < 0.0) { Nrm = -Nrm; }
			UV = FVector2D(Around[I], Down[R]);
		});
		TArray<FVector2D> Floor;
		for (int32 i = 0; i < N; ++i) { Floor.Add(FVector2D(P(Rings - 1, i).X, P(Rings - 1, i).Y)); }
		Bank.Begin(TEXT("Pond floor"), false, TEXT("Ground"));
		Kit::Level(Bank.M, Floor, {}, Depths[Rings - 1], true);

		// The edging stones: irregular slabs of rock, each laid along the edge and reaching over the water.
		for (int32 i = 0; i < 14; ++i)
		{
			const FVector2D A(CW::EdgingStones[i][0], CW::EdgingStones[i][1]);
			const FVector2D B(CW::EdgingStones[(i + 1) % 14][0], CW::EdgingStones[(i + 1) % 14][1]);
			const FVector2D U = (B - A).GetSafeNormal();
			FVector2D V(-U.Y, U.X);
			if (FVector2D::DotProduct(V, C - A) < 0.0) { V = -V; }
			const FVector2D Mid = A + V * 0.06;
			const double RA = 0.24 + 0.16 * Hash01(i, 101), RB = 0.16 + 0.09 * Hash01(i, 102);
			const double TopZ = 0.045 + 0.04 * Hash01(i, 103);
			const double TU = 0.04 * (Hash01(i, 104) - 0.5), TV = 0.03 * (Hash01(i, 105) - 0.5);
			constexpr int32 Facets = 13;
			TArray<FVector2D> Outline, Inner;
			for (int32 k = 0; k < Facets; ++k)
			{
				const double T = 2.0 * UE_DOUBLE_PI * (k + 0.45 * (Hash01(i, k) - 0.5)) / Facets;
				const double S = 1.0 + 0.34 * (Hash01(i, k + 50) - 0.5) + 0.12 * FMath::Cos(2.0 * T + 6.0 * Hash01(i, 106));
				const FVector2D Q = Mid + U * (RA * S * FMath::Cos(T)) + V * (RB * S * FMath::Sin(T));
				Outline.Add(Q);
				Inner.Add(Mid + (Q - Mid) * (1.0 - 0.028 / FMath::Max(0.05, FVector2D::Distance(Q, Mid))));
			}
			auto TopAt = [&](const FVector2D& Q) { return TopZ + TU * FVector2D::DotProduct(Q - Mid, U) / RA + TV * FVector2D::DotProduct(Q - Mid, V) / RB; };
			const FVector2D Grad = U * (TU / RA) + V * (TV / RB);
			const FVector TopN = FVector(-Grad.X, -Grad.Y, 1.0).GetSafeNormal();
			Edging.Begin(TEXT("Edging stone"));
			FMeshData& M = Edging.M;
			// The top (level enough to step on), its chamfer, the sides and the foot under the gravel.
			const TArray<int32> Tris = Kit::EarClip(Inner);
			int32 Base = M.Positions.Num();
			for (const FVector2D& Q : Inner) { M.Vertex(FVector(Q.X, Q.Y, TopAt(Q)), TopN, Q); }
			for (int32 t = 0; t + 2 < Tris.Num(); t += 3) { M.Tri(Base + Tris[t], Base + Tris[t + 1], Base + Tris[t + 2]); }
			Base = M.Positions.Num();
			for (const FVector2D& Q : Outline) { M.Vertex(FVector(Q.X, Q.Y, EdgingFoot), FVector(0, 0, -1), Q); }
			for (int32 t = 0; t + 2 < Tris.Num(); t += 3) { M.Tri(Base + Tris[t], Base + Tris[t + 1], Base + Tris[t + 2]); }
			for (int32 k = 0; k < Facets; ++k)
			{
				const int32 K1 = (k + 1) % Facets;
				auto Radial = [&](int32 Index) { const FVector2D R = (Outline[Index] - Mid).GetSafeNormal(); return FVector(R.X, R.Y, 0.0); };
				const FVector R0 = Radial(k), R1 = Radial(K1);
				const FVector C0 = (R0 + TopN).GetSafeNormal(), C1 = (R1 + TopN).GetSafeNormal();
				const FVector O0(Outline[k].X, Outline[k].Y, TopAt(Outline[k]) - 0.02), O1(Outline[K1].X, Outline[K1].Y, TopAt(Outline[K1]) - 0.02);
				const FVector I0(Inner[k].X, Inner[k].Y, TopAt(Inner[k])), I1(Inner[K1].X, Inner[K1].Y, TopAt(Inner[K1]));
				const double L = FVector2D::Distance(Outline[k], Outline[K1]);
				M.Quad(M.Vertex(O0, C0, FVector2D(0, 0)), M.Vertex(O1, C1, FVector2D(L, 0)), M.Vertex(I1, C1, FVector2D(L, 0.03)), M.Vertex(I0, C0, FVector2D(0, 0.03)));
				const FVector F0(Outline[k].X, Outline[k].Y, EdgingFoot), F1(Outline[K1].X, Outline[K1].Y, EdgingFoot);
				M.Quad(M.Vertex(F0, R0, FVector2D(0, -EdgingFoot)), M.Vertex(F1, R1, FVector2D(L, -EdgingFoot)), M.Vertex(O1, R1, FVector2D(L, -O1.Z)), M.Vertex(O0, R0, FVector2D(0, -O0.Z)));
			}
		}

		// The visitor's fence round the edge (not drawn).
		for (int32 i = 0; i < N; ++i)
		{
			const FVector2D A = Edge[i], B = Edge[(i + 1) % N];
			const FVector2D U = (B - A).GetSafeNormal(), V(-U.Y, U.X);
			const FVector2D A2 = A - U * 0.03, B2 = B + U * 0.03;
			Kit::Prism(Guard, TEXT("Pond guard"), {A2 - V * 0.01, B2 - V * 0.01, B2 + V * 0.01, A2 + V * 0.01}, -0.1, 0.7);
		}
	}

	// ------------------------------------------------------------------------------------------------ stone

	/** The drum base under a column (鼓磴): grey stone, a little swelling, 10 cm wider than the column. */
	FProfile ColumnBaseProfile()
	{
		FProfile P;
		P.Add(0.0, -0.05).Add(0.205, -0.05).Add(0.205, 0.0).Add(0.215, 0.03).Add(0.228, 0.09, true).Add(0.225, 0.14, true).Add(0.21, 0.19, true)
		 .Add(0.19, 0.213).Add(0.17, 0.225).Add(0.0, 0.225);
		return P;
	}

	TArray<FVector2D> ColumnList()
	{
		TArray<FVector2D> Out;
		for (const double Side : {-1.0, 1.0})
		{
			for (const double Y : CW::ColumnsAlongY) { Out.Add(FVector2D(Side * CW::ColumnLineX, Y)); }
		}
		for (int32 k = 1; k + 1 < 6; ++k) { Out.Add(FVector2D(CW::ColumnsAlongX[k], CW::ColumnLineY)); }
		return Out;
	}

	void BuildStone(FPart& Grey, FPart& Plinths, FPart& Gravel)
	{
		BuildGateRing(Grey);
		Kit::Box(Grey, TEXT("Moon gate sill"), FVector(-1.0, OuterY0 - 0.04, -0.08), FVector(1.0, CW::Y0 + 0.04, 0.02));
		// The garden's kerb, under the colonnade and across the terrace's edge.
		const double XK = CW::ColumnLineX, YK = CW::ColumnLineY;
		Kit::GridSolid(Grey, TEXT("Garden kerb"), {-XK - KerbHalf, -XK + KerbHalf, XK - KerbHalf, XK + KerbHalf},
					   {KerbNorth0, KerbNorth1, YK - KerbHalf, YK + KerbHalf}, {-0.1, KerbTop}, [](int32 I, int32 J, int32) { return !(I == 1 && J == 1); });
		for (const FVector2D& C : ColumnList()) { Kit::Lathe(Grey, TEXT("Column base"), FVector(C.X, C.Y, 0.0), Zenith, ColumnBaseProfile(), 40, 0.22); }
		// The bamboo beds against the north wall: a stone kerb round a bed of pebbles.
		for (const double Side : {-1.0, 1.0})
		{
			const double XA = Side * CW::BambooBedX0, XB = Side * CW::BambooBedX1;
			const double X0 = FMath::Min(XA, XB), X1 = FMath::Max(XA, XB);
			Kit::GridSolid(Grey, TEXT("Bamboo bed kerb"), {X0, X0 + 0.08, X1 - 0.08, X1}, {CW::Y0 - IntoWall, CW::BambooBedY1 - 0.08, CW::BambooBedY1},
						   {-0.05, 0.22}, [](int32 I, int32 J, int32) { return !(I == 1 && J == 0); });
			Kit::Box(Gravel, TEXT("Bamboo bed"), FVector(X0 + 0.07, CW::Y0 - IntoWall + 0.01, -0.035), FVector(X1 - 0.07, CW::BambooBedY1 - 0.07, 0.16));
		}
		// The terrace's two benches are AMuseeFurniture's now (Furniture/MuseeFurniture.cpp: granite on waisted legs).
		// The ceramics' drum plinths (a shadow gap at the foot, a soft arris at the top).
		FProfile Drum;
		const double R = CW::PlinthRadius, H = CW::PlinthTop;
		Drum.Add(0.0, 0.0).Add(R - 0.02, 0.0).Add(R - 0.02, 0.06).Add(R, 0.06).Add(R, H - 0.015).Add(R - 0.015, H).Add(0.0, H);
		for (const double Y : CW::CeramicsY) { Kit::Lathe(Plinths, TEXT("Ceramic plinth"), FVector(CW::CeramicsX, Y, 0.0), Zenith, Drum, 64, R); }
	}

	// ------------------------------------------------------------------------------------------------ timber

	/** The sparrow brace (雀替) under a beam where it meets a column: (along the beam from the column's axis, down). */
	TArray<FVector2D> BracketOutline()
	{
		TArray<FVector2D> P = {FVector2D(0.10, -0.02), FVector2D(0.55, -0.02)};
		constexpr int32 N = 10;
		for (int32 k = 0; k <= N; ++k)
		{
			const double T = 0.5 * UE_DOUBLE_PI * k / N;
			P.Add(FVector2D(0.10 + 0.45 * FMath::Cos(T), 0.20 * FMath::Sin(T)));
		}
		return P;
	}

	/** The hanging fretwork (挂落) under the eave beam between two columns. */
	void Fretwork(FPart& Part, const FWalk& W, double SA, double SB)
	{
		const double T0 = SA + ColumnRadius - 0.01, L = (SB - SA) - 2.0 * (ColumnRadius - 0.01);
		const double ZB = W.Board(ColumnD, 0.5 * (SA + SB)) - BeamUnderDrop - W.Deeper;
		auto Bar = [&](double TA, double TB, double VA, double VB, double Half)
		{
			WalkBox(Part, TEXT("Fretwork"), W, ColumnD - Half, ColumnD + Half, T0 + TA, T0 + TB, ZB - VB, ZB - VA);
		};
		Bar(0.0, L, -0.01, 0.045, 0.02);             // the head, into the beam
		Bar(0.0, 0.05, -0.01, 0.34, 0.018);          // the two feet against the columns
		Bar(L - 0.05, L, -0.01, 0.34, 0.018);
		Bar(0.04, L - 0.04, 0.255, 0.29, 0.02);      // the lower rail
		const int32 Panels = FMath::Max(3, FMath::RoundToInt32((L - 0.1) / 0.22));
		const double Pitch = (L - 0.1) / Panels;
		for (int32 k = 1; k < Panels; ++k) { Bar(0.05 + k * Pitch - 0.011, 0.05 + k * Pitch + 0.011, 0.04, 0.26, 0.015); }
		for (int32 k = 0; k < Panels; k += 2) { Bar(0.05 + k * Pitch, 0.05 + (k + 1) * Pitch, 0.139, 0.161, 0.012); }
	}

	/**
	 * A seat rail (美人靠, 吴王靠) between two columns: a plank seat on a low rail, its goose-neck back leaning out over the
	 * garden. Guard: an unseen wall over the back, from the seat's garden edge out past the top rail and up into the
	 * fretwork, so a visitor who hops onto the seat can't climb on to the back's top rail (7 cm wide, 0.45 m above the
	 * seat: a step) and be caught there between the fretwork overhead and the drop into the garden.
	 */
	void SeatRail(FPart& Part, FPart& Guard, const FWalk& W, double SA, double SB)
	{
		const double T0 = SA + ColumnRadius - 0.01, T1 = SB - ColumnRadius + 0.01;
		WalkBox(Guard, TEXT("Seat back guard"), W, ColumnD + 0.14, ColumnD + 0.44, SA + 0.02, SB - 0.02, -0.1, 2.9);
		auto Block = [&](const TCHAR* Name, double A0, double A1, double TA, double TB, double Z0, double Z1)
		{
			WalkBox(Part, Name, W, ColumnD + A0, ColumnD + A1, TA, TB, Z0, Z1);
		};
		auto Map = [&W](double A, double T, double Z) { const FVector2D P = W.Plan(ColumnD + A, T); return FVector(P.X, P.Y, Z); };
		Block(TEXT("Seat rail"), -0.03, 0.03, T0, T1, 0.05, 0.12);
		Block(TEXT("Seat"), -0.20, 0.13, T0, T1, 0.40, 0.45);
		const int32 Posts = FMath::Max(2, FMath::RoundToInt32((T1 - T0) / 0.32));
		for (int32 k = 0; k < Posts; ++k)
		{
			const double T = T0 + (k + 0.5) * (T1 - T0) / Posts;
			Block(TEXT("Seat post"), -0.02, 0.02, T - 0.02, T + 0.02, 0.11, 0.41);
		}
		// The back: goose-neck balusters from the seat out and up to the top rail, a stile and a tie to each column.
		auto Neck = [&](double T, double Size)
		{
			TArray<FVector> Path;
			constexpr int32 Steps = 8;
			for (int32 m = 0; m <= Steps; ++m)
			{
				const double Tau = double(m) / Steps;
				Path.Add(Map(0.09 + 0.21 * (1.0 - FMath::Square(1.0 - Tau)), T, 0.44 + 0.42 * Tau));
			}
			Kit::SweepSolid(Part, TEXT("Baluster"), Kit::PlaneFrames(Path, W.S3()), Kit::RectProfile(Size, Size));
		};
		const int32 Balusters = FMath::Max(2, FMath::RoundToInt32((T1 - T0 - 0.3) / 0.115));
		for (int32 k = 0; k < Balusters; ++k) { Neck(T0 + 0.15 + k * (T1 - T0 - 0.3) / (Balusters - 1), 0.028); }
		// At the south corners the backs of two walks meet over a corner stile; elsewhere each end has a stile and a tie to its column.
		auto IsCorner = [&W](double S)
		{
			const FVector2D P = W.Plan(ColumnD, S);
			return FMath::Abs(FMath::Abs(P.X) - CW::ColumnLineX) < 1e-6 && FMath::Abs(P.Y - CW::ColumnLineY) < 1e-6;
		};
		const bool bCornerA = IsCorner(SA), bCornerB = IsCorner(SB);
		for (const double T : {SA + 0.10, SB - 0.10})
		{
			if ((T < 0.5 * (SA + SB)) ? bCornerA : bCornerB) { continue; }
			Neck(T, 0.04);
			Block(TEXT("Back tie"), -0.02, 0.33, T - 0.015, T + 0.015, 0.855, 0.895);
		}
		const double RailA = bCornerA ? SA + 0.30 : SA + 0.10, RailB = bCornerB ? SB - 0.30 : SB - 0.10;   // ending inside the ties or the corner caps
		Kit::SweepSolid(Part, TEXT("Top rail"), Kit::UprightFrames({Map(0.30, RailA, 0.875), Map(0.30, RailB, 0.875)}),
						Kit::RoundedRectProfile(0.07, 0.045, 0.015));
		if (W.SDir.X == 0.0) { return; }   // the corners once, with the south walk's bays
		for (const double S : {SA, SB})
		{
			if (!IsCorner(S)) { continue; }
			const FVector2D Column = W.Plan(ColumnD, S);
			const FVector2D Out = FVector2D(-FMath::Sign(Column.X), -1.0);   // towards the garden, on the diagonal
			TArray<FVector> Path;
			for (int32 m = 0; m <= 8; ++m)
			{
				const double Tau = m / 8.0, A = 0.09 + 0.21 * (1.0 - FMath::Square(1.0 - Tau));
				Path.Add(FVector(Column.X + Out.X * A, Column.Y + Out.Y * A, 0.44 + 0.42 * Tau));
			}
			const FVector Side = FVector(Out.Y, -Out.X, 0.0).GetSafeNormal();
			Kit::SweepSolid(Part, TEXT("Corner stile"), Kit::PlaneFrames(Path, Side), Kit::RectProfile(0.05, 0.05));
			const FVector2D Top = Column + Out * 0.30;
			Kit::Box(Part, TEXT("Corner cap"), FVector(Top.X - 0.04, Top.Y - 0.04, 0.84), FVector(Top.X + 0.04, Top.Y + 0.04, 0.91));
		}
	}

	/** A table case: a recessed plinth, the body, the bed the work lies on, and a lip round it. */
	void TableCase(FPart& Timber, FPart& Beds, double X0, double X1, double Y0, double Y1, double BedTop)
	{
		// Every level face at least a centimetre from the next that faces the same way; the lip stands 5 mm proud of the body.
		Kit::Box(Timber, TEXT("Case plinth"), FVector(X0 + 0.04, Y0 + 0.04, -0.02), FVector(X1 - 0.04, Y1 - 0.04, 0.095));
		Kit::Box(Timber, TEXT("Case body"), FVector(X0, Y0, 0.08), FVector(X1, Y1, BedTop - 0.025));
		Kit::Box(Beds, TEXT("Case bed"), FVector(X0 + 0.04, Y0 + 0.04, BedTop - 0.055), FVector(X1 - 0.04, Y1 - 0.04, BedTop));
		Kit::GridSolid(Timber, TEXT("Case lip"), {X0 - 0.005, X0 + 0.05, X1 - 0.05, X1 + 0.005}, {Y0 - 0.005, Y0 + 0.05, Y1 - 0.05, Y1 + 0.005},
					   {BedTop - 0.04, BedTop + 0.035}, [](int32 I, int32 J, int32) { return !(I == 1 && J == 1); });
	}

	void BuildFrame(const TArray<FWalk>& Walks, FPart& Timber, FPart& Seats, FPart& Beds, FPart& Guard)
	{
		const double ColumnTop = Under(ColumnD) - PurlinDrop;
		FProfile Column;
		Column.Add(0.0, ColumnFoot).Add(ColumnRadius, ColumnFoot).Add(ColumnRadius, ColumnTop).Add(0.0, ColumnTop);
		for (const FVector2D& C : ColumnList()) { Kit::Lathe(Timber, TEXT("Column"), FVector(C.X, C.Y, 0.0), Zenith, Column, 32, ColumnRadius, true); }

		auto AlongWalk = [&Timber](const TCHAR* Name, const FWalk& W, double D, double SA, double SB, double Drop, const FProfile& Profile)
		{
			TArray<FVector> Path;
			for (const double S : LongStations(W, SA, SB)) { Path.Add(W.At(D, S, -Drop)); }
			Kit::SweepSolid(Timber, Name, Kit::UprightFrames(Path), Profile);
		};
		for (const FWalk& W : Walks)
		{
			AlongWalk(TEXT("Eave purlin"), W, ColumnD, W.EaveS0, W.EaveS1, PurlinDrop, Kit::CircleProfile(PurlinRadius, 16));
			AlongWalk(TEXT("Eave beam"), W, ColumnD, W.EaveS0, W.EaveS1, BeamDrop + W.Deeper / 2, Kit::RectProfile(BeamWidth, BeamDepth + W.Deeper));
			double Lo = 0.0, Hi = 0.0;
			W.SRange(MidPurlinD, Lo, Hi);
			AlongWalk(TEXT("Middle purlin"), W, MidPurlinD, Lo, Hi, MidPurlinDrop, Kit::CircleProfile(MidPurlinRadius, 12));
			AlongWalk(TEXT("Wall plate"), W, PlateD, W.S0, W.S1, PlateDrop + W.Deeper / 2, Kit::RectProfile(0.14, 0.12 + W.Deeper));
			// Brackets both sides of each column, wherever the beam goes on.
			for (const double S : W.Columns)
			{
				const double ZB = W.Board(ColumnD, S) - BeamUnderDrop - W.Deeper;
				for (const double Sigma : {-1.0, 1.0})
				{
					if (S + Sigma * 0.2 < W.EaveS0 || S + Sigma * 0.2 > W.EaveS1) { continue; }
					const FVector2D P = W.Plan(ColumnD, S);
					Kit::Extrude(Timber, TEXT("Bracket"), BracketOutline(), FVector(P.X, P.Y, ZB), W.S3() * Sigma, -Zenith, W.D3(), -0.035, 0.035);
				}
			}
			for (const TPair<double, double>& Bay : W.FretBays) { Fretwork(Timber, W, Bay.Key, Bay.Value); }
			for (const TPair<double, double>& Bay : W.SeatBays) { SeatRail(Seats, Guard, W, Bay.Key, Bay.Value); }
			// Tie beams from each column up to the wall, under the rafters.
			for (const double S : W.Ties)
			{
				TArray<FVector> Path;
				for (double D = -0.04; D < ColumnD - 1e-6; D += 0.25) { Path.Add(W.At(D, S, -TieTopDrop - TieDepth / 2)); }
				Path.Add(W.At(ColumnD, S, -TieTopDrop - TieDepth / 2));
				Kit::SweepSolid(Timber, TEXT("Tie beam"), Kit::UprightFrames(Path), Kit::RectProfile(TieWidth, TieDepth));
			}
			// Rafters every 0.32 m, from the wall plate to the eave or the valley.
			const double First = W.bLifted ? CW::Y0 + RafterWidth : W.S0;
			for (int32 k = FMath::CeilToInt32((First - 0.16) / TilePitch); 0.16 + k * TilePitch < W.S1; ++k)
			{
				const double S = 0.16 + k * TilePitch;
				double DLo = -0.04, DHi = RafterEndD;
				W.DRange(S, DLo, DHi);
				if (DHi - DLo < 0.3) { continue; }
				TArray<FVector> Path;
				for (double D = DLo; D < DHi - 1e-6; D += 0.26) { Path.Add(W.At(D, S, RafterSink - RafterDepth / 2)); }
				Path.Add(W.At(DHi, S, RafterSink - RafterDepth / 2));
				if (Path.Num() >= 2) { Kit::SweepSolid(Timber, TEXT("Rafter"), Kit::UprightFrames(Path), Kit::RectProfile(RafterWidth, RafterDepth)); }
			}
		}
		// The valley beams at the south corners, from the wall's corner over the corner column to the eave's.
		for (const double Side : {-1.0, 1.0})
		{
			TArray<FVector> Path;
			for (double D = -0.04; D < RafterEndD - 1e-6; D += 0.25)
			{
				Path.Add(FVector(Side * (CW::X1 - D), CW::Y1 - D, Under(D) + 0.01 - 0.11));
			}
			Path.Add(FVector(Side * (CW::X1 - RafterEndD), CW::Y1 - RafterEndD, Under(RafterEndD) + 0.01 - 0.11));
			Kit::SweepSolid(Timber, TEXT("Valley beam"), Kit::UprightFrames(Path), Kit::RectProfile(0.12, 0.22));
		}
		// The picture rail along the south wall (the scrolls' cords start in it, 5 cm out from the wall).
		{
			FProfile Rail;
			Rail.bClosed = true;
			Rail.Add(-0.01, -0.03).Add(0.045, -0.03).Add(0.06, -0.015, true).Add(0.062, 0.0, true).Add(0.058, 0.02, true).Add(0.04, 0.03).Add(-0.01, 0.03);
			const double Z = CW::PictureRailHeight;
			Kit::SweepSolid(Timber, TEXT("Picture rail"), LevelRun(FVector(CW::X0 - 0.01, CW::Y1, Z), FVector(CW::X1 + 0.01, CW::Y1, Z), FVector(0, -1, 0)), Rail);
		}
		TableCase(Timber, Beds, CW::HandscrollX0, CW::HandscrollX1, CW::HandscrollY0, CW::HandscrollY1, CW::HandscrollBed);
		TableCase(Timber, Beds, CW::OrchidX0, CW::OrchidX1, CW::OrchidY0, CW::OrchidY1, CW::OrchidBed);
	}

	// ------------------------------------------------------------------------------------------------ roofs

	/** A walk's roof: the slab (boarding under a mortar bed, one closed solid with the other walks'), its tiles, the flashing at the wall. */
	void CloisterRoof(const FWalk& W, FPart& Mortar, FPart& Timber, FPart& Tiles)
	{
		TArray<double> DS = {-IntoWall};
		constexpr int32 DSteps = 30;
		for (int32 k = 0; k <= DSteps; ++k) { DS.Add(SlabEdgeD * k / DSteps); }
		TArray<double> Ref;
		for (double S = W.S0; S < W.S1 - 1e-6; S += (W.bLifted && S < LiftFromS + 0.4) ? 0.1 : 0.4) { Ref.Add(S); }
		Ref.Add(W.S1);
		auto SAt = [&](int32 I, int32 J)
		{
			double Lo = 0.0, Hi = 0.0;
			W.SRange(DS[I], Lo, Hi);
			if (J == 0) { return Lo; }
			if (J == Ref.Num() - 1) { return Hi; }
			return Lo + (Ref[J] - Ref[0]) * (Hi - Lo) / (Ref.Last() - Ref[0]);
		};
		const FString Slab = TEXT("Cloister roof slab");
		const int32 NU = DS.Num() - 1, NV = Ref.Num() - 1;
		Mortar.Begin(W.Name + TEXT(" roof bed"), false, Slab);
		GridPatch(Mortar.M, NU, NV, [&](int32 I, int32 J, FVector& P, FVector& N, FVector2D& UV)
		{
			const double S = SAt(I, J);
			P = W.At(DS[I], S, SlabThickness);
			N = W.BoardNormal(DS[I], S);
			UV = FVector2D(P.X, P.Y);
		});
		Timber.Begin(W.Name + TEXT(" roof boarding"), false, Slab);
		GridPatch(Timber.M, NU, NV, [&](int32 I, int32 J, FVector& P, FVector& N, FVector2D& UV)
		{
			const double S = SAt(I, J);
			P = W.At(DS[I], S, 0.0);
			N = -W.BoardNormal(DS[I], S);
			UV = FVector2D(P.X, P.Y);
		});
		// The fascia at the eave, the edge in the wall, and (west and east) the end in the north wall.
		auto RowStrip = [&](int32 I, const FVector& N)
		{
			TArray<FVector> Upper, Lower;
			for (int32 j = 0; j <= NV; ++j)
			{
				const double S = SAt(I, j);
				Upper.Add(W.At(DS[I], S, SlabThickness));
				Lower.Add(W.At(DS[I], S, 0.0));
			}
			Strip(Timber.M, Upper, Lower, N);
		};
		RowStrip(NU, W.D3());
		RowStrip(0, -W.D3());
		if (W.bLifted)
		{
			TArray<FVector> Upper, Lower;
			for (int32 i = 0; i <= NU; ++i)
			{
				Upper.Add(W.At(DS[i], SAt(i, 0), SlabThickness));
				Lower.Add(W.At(DS[i], SAt(i, 0), 0.0));
			}
			Strip(Timber.M, Upper, Lower, -W.S3());
		}

		// The tiles.
		FSlope Slope;
		Slope.Origin = W.Origin;
		Slope.DDir = W.DDir;
		Slope.SDir = W.SDir;
		Slope.Limits = W.Limits;
		Slope.Bed = [&W](double D, double S) { return W.Board(D, S) + SlabThickness; };
		Slope.From = CloisterTilesFrom;
		Slope.To = CloisterTilesTo;
		Slope.bOrnaments = true;
		TileRows(Tiles, Slope, W.bLifted ? CW::Y0 + 0.02 : W.S0 - 0.3, W.bLifted ? W.S1 : W.S1 + 0.3);

		// The flashing: a mortar fillet where the tiles meet the wall.
		const double Z0 = Under(0.0) + SlabThickness, Front = Under(0.15) - Under(0.0);
		FProfile Fillet;
		Fillet.bClosed = true;
		Fillet.Add(-0.03, -0.02).Add(0.15, Front - 0.02).Add(0.15, Front + 0.05).Add(0.06, 0.12).Add(-0.03, 0.15);
		const FVector2D A = W.Plan(0.0, W.S0), B = W.Plan(0.0, W.S1);
		Kit::SweepSolid(Mortar, TEXT("Flashing"), LevelRun(FVector(A.X, A.Y, Z0), FVector(B.X, B.Y, Z0), W.D3()), Fillet);
	}

	/** One of the court's walls, for its coping: its centre line, which way is out, and the mitres at its corners. */
	struct FWallLine
	{
		FVector2D Centre = FVector2D::ZeroVector;   // the centre line at s = 0
		FVector2D Along = FVector2D(1, 0);
		FVector2D Out = FVector2D(0, -1);
		TArray<FVector> Limits;
		double S0 = 0.0, S1 = 0.0;
	};

	TArray<FWallLine> WallLines()
	{
		const double H = CW::Wall / 2;
		TArray<FWallLine> Out;
		FWallLine N;
		N.Centre = FVector2D(0.0, CW::Y0 - H);
		N.Along = FVector2D(1, 0);
		N.Out = FVector2D(0, -1);
		N.Limits = {FVector(-1, 1, Diagonal), FVector(1, 1, Diagonal)};
		N.S0 = OuterX0 - 0.6;
		N.S1 = OuterX1 + 0.6;
		Out.Add(N);
		FWallLine S = N;
		S.Centre = FVector2D(0.0, CW::Y1 + H);
		S.Out = FVector2D(0, 1);
		S.Limits = {FVector(-1, -1, -Diagonal), FVector(1, -1, -Diagonal)};
		Out.Add(S);
		FWallLine Wst;
		Wst.Centre = FVector2D(CW::X0 - H, 0.0);
		Wst.Along = FVector2D(0, 1);
		Wst.Out = FVector2D(-1, 0);
		Wst.Limits = {FVector(1, -1, -Diagonal), FVector(1, 1, Diagonal)};
		Wst.S0 = OuterY0 - 0.6;
		Wst.S1 = OuterY1 + 0.6;
		Out.Add(Wst);
		FWallLine E = Wst;
		E.Centre = FVector2D(CW::X1 + H, 0.0);
		E.Out = FVector2D(1, 0);
		E.Limits = {FVector(-1, -1, -Diagonal), FVector(-1, 1, Diagonal)};
		Out.Add(E);
		return Out;
	}

	/** The walls' copings: a corbel course, a small gabled roof of tiles on a mortar bed, a ridge, hips at the outer corners. */
	void Copings(FPart& Mortar, FPart& Tiles, FPart& Ridges)
	{
		const double C = CorbelOut;
		Kit::GridSolid(Mortar, TEXT("Corbel course"), {OuterX0 - C, CW::X0 + C, CW::X1 - C, OuterX1 + C}, {OuterY0 - C, CW::Y0 + C, CW::Y1 - C, OuterY1 + C},
					   {CorbelBottom, CorbelTop}, [](int32 I, int32 J, int32) { return !(I == 1 && J == 1); });
		const FString Slab = TEXT("Wall coping");
		constexpr int32 AlongSteps = 70, AcrossSteps = 5;
		for (const FWallLine& L : WallLines())
		{
			for (const double Sigma : {1.0, -1.0})
			{
				const FVector2D Side = L.Out * Sigma;
				auto SAt = [&](int32 I, int32 J)
				{
					double Lo = L.S0, Hi = L.S1;
					ClipLine(L.Limits, L.Centre + Side * (CopingHalf * I / AcrossSteps), L.Along, Lo, Hi);
					return FMath::Lerp(Lo, Hi, double(J) / AlongSteps);
				};
				auto Pos = [&](int32 I, int32 J, double Z)
				{
					const FVector2D P = L.Centre + Side * (CopingHalf * I / AcrossSteps) + L.Along * SAt(I, J);
					return FVector(P.X, P.Y, Z);
				};
				Mortar.Begin(TEXT("Coping bed"), false, Slab);
				GridPatch(Mortar.M, AcrossSteps, AlongSteps, [&](int32 I, int32 J, FVector& P, FVector& N, FVector2D& UV)
				{
					const double A = CopingHalf * I / AcrossSteps;
					P = Pos(I, J, CopingTop(A));
					const double Grade = (CopingTop(A + 0.005) - CopingTop(FMath::Max(A - 0.005, 0.0))) / (A + 0.005 - FMath::Max(A - 0.005, 0.0));
					N = FVector(-Grade * Side.X, -Grade * Side.Y, 1.0).GetSafeNormal();
					UV = FVector2D(P.X, P.Y);
				});
				Mortar.Begin(TEXT("Coping soffit"), false, Slab);
				GridPatch(Mortar.M, AcrossSteps, AlongSteps, [&](int32 I, int32 J, FVector& P, FVector& N, FVector2D& UV)
				{
					P = Pos(I, J, CopingUnder);
					N = FVector(0, 0, -1);
					UV = FVector2D(P.X, P.Y);
				});
				TArray<FVector> Upper, Lower;
				for (int32 j = 0; j <= AlongSteps; ++j)
				{
					Upper.Add(Pos(AcrossSteps, j, CopingTop(CopingHalf)));
					Lower.Add(Pos(AcrossSteps, j, CopingUnder));
				}
				Mortar.Begin(TEXT("Coping edge"), false, Slab);
				Strip(Mortar.M, Upper, Lower, FVector(Side.X, Side.Y, 0.0));

				FSlope Slope;
				Slope.Origin = L.Centre;
				Slope.DDir = Side;
				Slope.SDir = L.Along;
				Slope.Limits = L.Limits;
				Slope.Bed = [](double D, double) { return CopingTop(D); };
				Slope.From = CopingTilesFrom;
				Slope.To = CopingTilesTo;
				Slope.bOrnaments = true;
				TileRows(Tiles, Slope, L.S0, L.S1);
			}
			// The ridge, corner to corner (they cross at the corners).
			double Lo = L.S0, Hi = L.S1;
			ClipLine(L.Limits, L.Centre, L.Along, Lo, Hi);
			FProfile Ridge;
			Ridge.bClosed = true;
			Ridge.Add(-0.08, -0.08).Add(0.08, -0.08).Add(0.08, 0.08).Add(0.055, 0.125, true).Add(0.0, 0.14, true).Add(-0.055, 0.125, true).Add(-0.08, 0.08);
			const double Z = CopingTop(0.0);
			// Past the other ridge's side by 2 cm, so an end never lies in the crossing ridge's face.
			const FVector2D A = L.Centre + L.Along * (Lo - 0.10), B = L.Centre + L.Along * (Hi + 0.10);
			Kit::SweepSolid(Ridges, TEXT("Ridge"), LevelRun(FVector(A.X, A.Y, Z), FVector(B.X, B.Y, Z), FVector(L.Out.X, L.Out.Y, 0.0)), Ridge);
		}
		// The hips over the outer corners.
		const double H = CW::Wall / 2;
		const FVector2D Corners[4] = {FVector2D(CW::X0 - H, CW::Y0 - H), FVector2D(CW::X1 + H, CW::Y0 - H), FVector2D(CW::X1 + H, CW::Y1 + H), FVector2D(CW::X0 - H, CW::Y1 + H)};
		const FVector2D Outs[4] = {FVector2D(-1, -1), FVector2D(1, -1), FVector2D(1, 1), FVector2D(-1, 1)};
		FProfile Hip;
		Hip.bClosed = true;
		Hip.Add(-0.035, -0.05).Add(0.035, -0.05).Add(0.035, 0.03).Add(0.0, 0.055, true).Add(-0.035, 0.03);
		for (int32 c = 0; c < 4; ++c)
		{
			TArray<FVector> Path;
			for (int32 k = 0; k <= 8; ++k)
			{
				const double A = CopingTilesTo * k / 8;
				const FVector2D P = Corners[c] + Outs[c] * A;
				Path.Add(FVector(P.X, P.Y, CopingTop(A) + 0.07));
			}
			Kit::SweepSolid(Ridges, TEXT("Hip"), Kit::UprightFrames(Path), Hip);
		}
	}

	// ------------------------------------------------------------------------------------------------ the whole

	FWingMeshes BuildMeshes()
	{
		FWingMeshes Out;
		const TArray<FWalk> Walks = MakeWalks();
		BuildWalls(Out.Walls);
		BuildVestibule(Out.Plaster);
		BuildFloors(Out.Paving, Out.Gravel);
		BuildPond(Out.Bank, Out.Edging, Out.Guard);
		BuildStone(Out.GreyStone, Out.Plinths, Out.Gravel);
		BuildFrame(Walks, Out.Timber, Out.Seats, Out.Beds, Out.Guard);
		for (const FWalk& W : Walks) { CloisterRoof(W, Out.Mortar, Out.Timber, Out.Tiles); }
		Copings(Out.Mortar, Out.Tiles, Out.Ridges);
		return Out;
	}

	TArray<FLightStrip> EaveStrips()
	{
		TArray<FLightStrip> Out;
		constexpr double D = 4.25;
		for (const FWalk& W : MakeWalks())
		{
			const double SA = W.bLifted ? 13.9 : -5.3, SB = W.bLifted ? 28.6 : 5.3;
			const FVector2D A = W.Plan(D, SA), B = W.Plan(D, SB);
			FLightStrip L;
			L.Centre = FVector(0.5 * (A.X + B.X), 0.5 * (A.Y + B.Y), Under(D) - 0.26);
			L.Along = W.S3();
			L.Facing = (-W.D3() * 0.2 + Zenith).GetSafeNormal();   // nearly straight up: the boarding over the walk and the eave's overhang
			L.Width = FVector2D::Distance(A, B);
			L.Height = 0.03;
			Out.Add(L);
		}
		return Out;
	}

	TArray<FLightStrip> PathStrips()
	{
		TArray<FLightStrip> Out;
		for (const FWalk& W : MakeWalks())
		{
			for (const TPair<double, double>& Bay : W.SeatBays)
			{
				const FVector2D P = W.Plan(ColumnD - 0.12, 0.5 * (Bay.Key + Bay.Value));
				FLightStrip L;
				L.Centre = FVector(P.X, P.Y, 0.37);
				L.Along = W.S3();
				L.Facing = (-W.D3() * 0.5 - Zenith * 0.866).GetSafeNormal();
				L.Width = (Bay.Value - Bay.Key) - 0.5;
				L.Height = 0.02;
				Out.Add(L);
			}
		}
		for (const double X0 : CW::BenchX0)
		{
			FLightStrip L;
			L.Centre = FVector(X0 + 0.5 * CW::BenchLength, 0.5 * (CW::BenchY0 + CW::BenchY1), CW::BenchHeight - 0.115);
			L.Along = FVector(1, 0, 0);
			L.Facing = FVector(0, -0.5, -0.866).GetSafeNormal();
			L.Width = 0.7;
			L.Height = 0.02;
			Out.Add(L);
		}
		return Out;
	}

	FLightStrip VestibuleStrip()
	{
		FLightStrip L;
		L.Centre = FVector(0.0, 11.95, CW::VestibuleCeiling - 0.015);
		L.Along = FVector(1, 0, 0);
		L.Facing = FVector(0, 0, -1);
		L.Width = 1.6;
		L.Height = 0.5;
		return L;
	}

	TArray<FVector2D> ColumnCentres() { return ColumnList(); }
}
