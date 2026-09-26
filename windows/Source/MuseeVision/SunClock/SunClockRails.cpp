#include "SunClock/SunClockBuild.h"

/**
 * The bronze: a balustrade round the well after Momo's (side-fixed to the inner string, in the well):
 * square standards on brackets, a moulded handrail, and between them two registers, slender balusters
 * below and a chain of rings above, each ring with a boss on four spokes, the rings drawn true on the
 * rake so they touch the rails that frame them. Along the stair's wall, a round handrail on brackets,
 * returned to the wall at its top. A newel with a ball at the foot.
 *
 * The lowered dial covers the stair's top: along the upper flight (to the split, tread 8) and across the
 * top landing's back edge the balustrade retracts. There it is a free panel (standards, balusters, rings,
 * handrail, and under them a carrier bar) standing in the slot of a hollow housing hung on the stair (marble
 * sides that deepen the string, a bronze top round the slot;
 * SunClockBuild.h, PanelDrop): while the dial is down it sits wholly inside, and it rises with the drum.
 * Below the split everything stands under the lowered dial and is fixed. The wall's rail starts a tread
 * above the split; the top flight's wall side has none (the well side guards it). Each part carries a
 * hidden fence for collision; the retracting panels' fences are in their plane and move with them.
 */
namespace SunClockBuild
{
	/** A path for a rail: a plan point at s (and an offset across the rail's plane), the plane's normal, a base height. */
	struct FRailPath
	{
		TFunction<FVector(double, double)> At;
		TFunction<FVector(double)> Across;
		TFunction<double(double)> Base;
		TArray<double> Kinks;

		FVector Point(double S, double Off, double Z) const { return At(S, Off) + FVector(0, 0, Z); }
		FVector Along(double S) const { return (At(S + 0.001, 0) - At(S - 0.001, 0)).GetSafeNormal(); }
	};

	// The balustrade's registers, above its base (the string's top).
	constexpr double BarHalf = 0.0125, BarThick = 0.01;
	constexpr double BottomBar = 0.0575, MidBar = 0.5325, TopBar = 0.8675, HandrailV = 0.90;
	constexpr double PostHalf = 0.015, PostFoot = -0.25;

	static TArray<double> RailSamples(const FRailPath& P, double S0, double S1, double Step)
	{
		TArray<double> Out;
		const int32 N = FMath::Max(1, FMath::CeilToInt((S1 - S0) / Step));
		for (int32 I = 0; I <= N; ++I) { Out.Add(S0 + (S1 - S0) * I / N); }
		for (const double K : P.Kinks)
		{
			if (K > S0 + 1e-6 && K < S1 - 1e-6) { Out.Add(K); }
		}
		Out.Sort();
		return Out;
	}

	/** A profile swept along the path from S0 to S1 (its (A, B) = (across, up) about the base + V), capped. */
	static void RailSweep(FMeshData& M, const FRailPath& P, double S0, double S1, double V, const FProfile& Section, bool bCap0, bool bCap1)
	{
		TArray<FFrame> Run;
		for (const double S : RailSamples(P, S0, S1, 0.05))
		{
			FFrame F;
			F.Origin = P.Point(S, 0, P.Base(S) + V);
			F.AxisA = F.NormA = P.Across(S);
			F.AxisB = F.NormB = FVector::UpVector;
			F.S = S;
			Run.Add(F);
		}
		SalonKit::Sweep(M, Run, Section);
		if (bCap0) { SalonKit::CapConvex(M, Run[0], Section, -P.Along(S0)); }
		if (bCap1) { SalonKit::CapConvex(M, Run.Last(), Section, P.Along(S1)); }
	}

	static void RailSweep(FMeshData& M, const FRailPath& P, double S0, double S1, double V, const FProfile& Section, bool bCaps)
	{
		RailSweep(M, P, S0, S1, V, Section, bCaps, bCaps);
	}

	static FProfile RectSection(double A0, double A1, double B0, double B1)
	{
		FProfile R;
		R.Add(A1, B0).Add(A1, B1).Add(A0, B1).Add(A0, B0);
		R.bClosed = true;
		return R;
	}

	/** A flat piece in the rail's developed plane: an outline in (s, z), from Off0 to Off1 across. */
	static void RailPrism(FMeshData& M, const FRailPath& P, const TArray<FVector2D>& Outline, double Off0, double Off1)
	{
		double SMid = 0;
		for (const FVector2D& Q : Outline) { SMid += Q.X; }
		SMid /= Outline.Num();
		const FVector Across = P.Across(SMid), Along = P.Along(SMid);
		FlatPolygon(M, Outline, [&P, Off0](const FVector2D& Q) { return P.Point(Q.X, Off0, Q.Y); }, -Across);
		FlatPolygon(M, Outline, [&P, Off1](const FVector2D& Q) { return P.Point(Q.X, Off1, Q.Y); }, Across);
		double Area = 0;
		for (int32 I = 0; I < Outline.Num(); ++I) { Area += SalonKit::Cross2(Outline[I], Outline[(I + 1) % Outline.Num()]); }
		for (int32 I = 0; I < Outline.Num(); ++I)
		{
			const FVector2D A = Outline[I], B = Outline[(I + 1) % Outline.Num()];
			FVector2D Out(B.Y - A.Y, A.X - B.X);
			if (Area < 0) { Out = -Out; }
			Out.Normalize();
			const FVector N = (Along * Out.X + FVector::UpVector * Out.Y).GetSafeNormal();
			M.Rect(P.Point(A.X, Off0, A.Y), P.Point(B.X, Off0, B.Y), P.Point(B.X, Off1, B.Y), P.Point(A.X, Off1, A.Y), N);
		}
	}

	/** A vertical bar at S from V0 to V1 above the base there. */
	static void RailUpright(FMeshData& M, const FRailPath& P, double S, double HalfS, double V0, double V1, double Off0, double Off1)
	{
		const double Z = P.Base(S);
		RailPrism(M, P, {FVector2D(S - HalfS, Z + V0), FVector2D(S + HalfS, Z + V0), FVector2D(S + HalfS, Z + V1), FVector2D(S - HalfS, Z + V1)}, Off0, Off1);
	}

	/** A ring in the developed plane (true circles on the rake), Off0 … Off1 thick. */
	static void RailRing(FMeshData& M, const FRailPath& P, const FVector2D& C, double ROut, double RIn, double Off0, double Off1)
	{
		constexpr int32 Segs = 36;
		const FVector Across = P.Across(C.X), Along = P.Along(C.X);
		auto Pt = [&](double R, double T, double Off) { return P.Point(C.X + R * FMath::Cos(T), Off, C.Y + R * FMath::Sin(T)); };
		for (int32 I = 0; I < Segs; ++I)
		{
			const double T0 = Turn * I / Segs, T1 = Turn * (I + 1) / Segs;
			M.Rect(Pt(RIn, T0, Off0), Pt(ROut, T0, Off0), Pt(ROut, T1, Off0), Pt(RIn, T1, Off0), -Across);
			M.Rect(Pt(RIn, T0, Off1), Pt(ROut, T0, Off1), Pt(ROut, T1, Off1), Pt(RIn, T1, Off1), Across);
			auto Radial = [&](double T) { return (Along * FMath::Cos(T) + FVector::UpVector * FMath::Sin(T)).GetSafeNormal(); };
			const FVector N0 = Radial(T0), N1 = Radial(T1);
			M.Quad(M.Vertex(Pt(ROut, T0, Off0), N0, FVector2D(T0 * ROut, Off0)), M.Vertex(Pt(ROut, T1, Off0), N1, FVector2D(T1 * ROut, Off0)),
				   M.Vertex(Pt(ROut, T1, Off1), N1, FVector2D(T1 * ROut, Off1)), M.Vertex(Pt(ROut, T0, Off1), N0, FVector2D(T0 * ROut, Off1)));
			if (RIn > 0)
			{
				M.Quad(M.Vertex(Pt(RIn, T0, Off0), -N0, FVector2D(T0 * RIn, Off0)), M.Vertex(Pt(RIn, T1, Off0), -N1, FVector2D(T1 * RIn, Off0)),
					   M.Vertex(Pt(RIn, T1, Off1), -N1, FVector2D(T1 * RIn, Off1)), M.Vertex(Pt(RIn, T0, Off1), -N0, FVector2D(T0 * RIn, Off1)));
			}
		}
	}

	static FProfile HandrailSection()
	{
		// A soft rounded bar, 60 × 45 mm.
		FProfile H;
		for (int32 I = 0; I < 24; ++I)
		{
			const double T = Turn * I / 24;
			const double C = FMath::Cos(T), S = FMath::Sin(T);
			H.Add(0.030 * FMath::Sign(C) * FMath::Pow(FMath::Abs(C), 0.7), 0.0225 * FMath::Sign(S) * FMath::Pow(FMath::Abs(S), 0.7), true);
		}
		H.bClosed = true;
		return H;
	}

	/**
	 * The balustrade from S0 to S1: standards at both ends (if asked), at every kink and at most 1.6 m
	 * apart, panels of balusters and rings between them, the bars and handrail over the whole length.
	 * Brackets from each standard's foot to the string (BracketOff0 … BracketOff1 across), or none (a
	 * retracting panel: its standards run down to Foot, into its housing).
	 */
	static void Balustrade(FMeshData& M, const FRailPath& P, double S0, double S1, bool bPost0, bool bPost1, double BracketOff0, double BracketOff1,
						   double Foot = PostFoot, bool bBrackets = true)
	{
		TArray<double> Posts;
		TArray<double> Breaks = {S0};
		for (const double K : P.Kinks)
		{
			if (K > S0 + 0.05 && K < S1 - 0.05) { Breaks.Add(K); }
		}
		Breaks.Add(S1);
		for (int32 I = 0; I + 1 < Breaks.Num(); ++I)
		{
			const int32 N = FMath::Max(1, FMath::CeilToInt((Breaks[I + 1] - Breaks[I]) / 1.6));
			for (int32 J = 0; J < N; ++J) { Posts.Add(Breaks[I] + (Breaks[I + 1] - Breaks[I]) * J / N); }
		}
		Posts.Add(S1);
		for (int32 I = 0; I < Posts.Num(); ++I)
		{
			const bool bEnd = I == 0 || I == Posts.Num() - 1;
			if ((I == 0 && !bPost0) || (I == Posts.Num() - 1 && !bPost1)) { continue; }
			const double S = FMath::Clamp(Posts[I], S0 + (bEnd ? PostHalf : 0), S1 - (bEnd ? PostHalf : 0));
			RailUpright(M, P, S, PostHalf, Foot, HandrailV - 0.01, -PostHalf, PostHalf);
			if (bBrackets) { RailUpright(M, P, S, PostHalf * 0.8, Foot + 0.02, Foot + 0.08, BracketOff0, BracketOff1); }
		}
		// Panels.
		for (int32 I = 0; I + 1 < Posts.Num(); ++I)
		{
			const double A = Posts[I] + PostHalf, B = Posts[I + 1] - PostHalf, Len = B - A;
			if (Len < 0.1) { continue; }
			const double Slope = (P.Base(B) - P.Base(A)) / Len;
			const double Cos = 1.0 / FMath::Sqrt(1.0 + Slope * Slope);
			// Balusters, about 110 mm apart.
			const int32 NB = FMath::Max(1, FMath::FloorToInt((Len - 0.08) / 0.11));
			for (int32 J = 0; J <= NB; ++J)
			{
				const double S = A + 0.04 + (Len - 0.08) * J / NB;
				RailUpright(M, P, S, 0.007, BottomBar, MidBar, -0.007, 0.007);
			}
			// Rings between the mid bar and the top bar, touching both.
			const double Gap = (TopBar - BarHalf) - (MidBar + BarHalf);
			const double Rho = Gap * Cos / 2, Step = 2 * Rho * Cos;
			const int32 NR = FMath::FloorToInt((Len - 0.02) / Step);
			const double First = A + (Len - NR * Step) / 2 + Step / 2;
			const double VMid = (MidBar + TopBar) / 2;
			for (int32 J = 0; J < NR; ++J)
			{
				const double S = First + J * Step;
				const FVector2D C(S, P.Base(S) + VMid);
				RailRing(M, P, C, Rho + 0.002, Rho - 0.010, -0.006, 0.006);
				RailRing(M, P, C, 0.022, 0.0, -0.011, 0.011);
				for (int32 K = 0; K < 4; ++K)
				{
					const double T = (45.0 + 90.0 * K) * Deg;
					const FVector2D D(FMath::Cos(T), FMath::Sin(T)), Nn(-D.Y, D.X);
					const FVector2D P0 = C + D * 0.018, P1 = C + D * (Rho - 0.008);
					RailPrism(M, P, {P0 - Nn * 0.0035, P1 - Nn * 0.0035, P1 + Nn * 0.0035, P0 + Nn * 0.0035}, -0.004, 0.004);
				}
			}
		}
		// The bars and the handrail.
		for (const double V : {BottomBar, MidBar, TopBar})
		{
			RailSweep(M, P, S0, S1, V, RectSection(-BarThick, BarThick, -BarHalf, BarHalf), true);
		}
		FProfile Hand = HandrailSection();
		for (SalonKit::FProfilePoint& Pt : Hand.Points) { Pt.P.X += 0.004; }
		RailSweep(M, P, S0, S1, HandrailV, Hand, true);
	}

	/** A hidden wall for collision along the path (both faces and the top), from V0 to V1 over its base. */
	static void RailFence(FMeshData& M, const FRailPath& P, double S0, double S1, double V0, double V1)
	{
		RailSweep(M, P, S0, S1, 0.0, RectSection(-0.01, 0.01, V0, V1), true);
	}

	/** A closed outline in (across, up), anticlockwise, each segment (to the next point) in one material. */
	struct FHousingOutline
	{
		enum EMat : int32 { Stone = 0, Metal = 1 };
		TArray<FVector2D> Points;
		TArray<int32> Mats;

		/** A segment from the last point to (A, B) (nothing if it has no length). */
		void To(double A, double B, int32 Mat)
		{
			const FVector2D Q(A, B);
			if (Points.Num() && FVector2D::Distance(Points.Last(), Q) < 1e-9) { return; }
			if (Points.Num()) { Mats.Add(Mat); }
			Points.Add(Q);
		}
	};

	/**
	 * The housing's section: marble sides between a bronze cap (the top, and on an open side its first 6 cm,
	 * standing 8 mm proud) and a bronze shoe (the bottom, and its last 5 cm); on an open side a sunk field
	 * (12 mm) between them. With the slot (the channel between the end blocks) or without (the end blocks).
	 */
	static FHousingOutline HousingSection(double A0, double A1, bool bOpen0, bool bOpen1, bool bSlot, bool bField)
	{
		constexpr double Over = 0.008, Cap = 0.06, Shoe = 0.05, Sink = 0.012, FieldTop = -0.10, FieldBottom = -1.09;
		constexpr int32 Stone = FHousingOutline::Stone, Metal = FHousingOutline::Metal;
		const double D = HousingDepth;
		const double X0 = A0 - (bOpen0 ? Over : 0.0), X1 = A1 + (bOpen1 ? Over : 0.0);
		FHousingOutline O;
		// Up the A1 side.
		O.To(X1, -D, Metal);
		O.To(X1, -D + Shoe, Metal);
		O.To(A1, -D + Shoe, Metal);
		if (bOpen1 && bField)
		{
			O.To(A1, FieldBottom, Stone);
			O.To(A1 - Sink, FieldBottom, Stone);
			O.To(A1 - Sink, FieldTop, Stone);
			O.To(A1, FieldTop, Stone);
		}
		O.To(A1, -Cap, Stone);
		O.To(X1, -Cap, Metal);
		O.To(X1, 0.0, Metal);
		// The top, and the slot.
		if (bSlot)
		{
			O.To(SlotHalf, 0.0, Metal);
			O.To(SlotHalf, -D + HousingFloor, Metal);
			O.To(-SlotHalf, -D + HousingFloor, Metal);
			O.To(-SlotHalf, 0.0, Metal);
		}
		O.To(X0, 0.0, Metal);
		// Down the A0 side.
		O.To(X0, -Cap, Metal);
		O.To(A0, -Cap, Metal);
		if (bOpen0 && bField)
		{
			O.To(A0, FieldTop, Stone);
			O.To(A0 + Sink, FieldTop, Stone);
			O.To(A0 + Sink, FieldBottom, Stone);
			O.To(A0, FieldBottom, Stone);
		}
		O.To(A0, -D + Shoe, Stone);
		O.To(X0, -D + Shoe, Metal);
		O.To(X0, -D, Metal);
		O.Mats.Add(Metal);   // the bottom, back to the start
		return O;
	}

	/** The section swept from S0 to S1, each run of one material into its mesh (Out[0] marble, Out[1] bronze). */
	static void SweepHousing(FMeshData* Out[2], const FRailPath& P, double S0, double S1, const FHousingOutline& O)
	{
		const int32 N = O.Points.Num();
		int32 I = 0;
		while (I < N)
		{
			const int32 Mat = O.Mats[I];
			FProfile Run;
			Run.Add(O.Points[I].X, O.Points[I].Y);
			int32 J = I;
			while (J < N && O.Mats[J] == Mat)
			{
				const FVector2D& Q = O.Points[(J + 1) % N];
				Run.Add(Q.X, Q.Y);
				++J;
			}
			RailSweep(*Out[Mat], P, S0, S1, 0.0, Run, false);
			I = J;
		}
	}

	/** A flat face across the path at S (an outline in (across, up)), facing Facing. */
	static void HousingFace(FMeshData& M, const FRailPath& P, double S, const TArray<FVector2D>& Outline, const FVector& Facing)
	{
		const double Z = P.Base(S);
		FlatPolygon(M, Outline, [&P, S, Z](const FVector2D& Q) { return P.Point(S, Q.X, Z + Q.Y); }, Facing);
	}

	/**
	 * A retracting balustrade's housing along the path, S0 … S1: a hollow channel A0 … A1 across, its top on
	 * the base, HousingDepth deep, a slot SlotHalf either side of the panel's plane along its top, a solid
	 * block closing each end. Bound in bronze (HousingSection): the cap round the slot, the shoe, the end plates;
	 * the open sides (bOpen0, bOpen1: those seen, not against the stair) stand out with sunk marble fields.
	 */
	static void Housing(FMeshData& Bronze, FMeshData& Stone, const FRailPath& P, double S0, double S1, double A0, double A1, bool bOpen0, bool bOpen1)
	{
		FMeshData* Out[2] = {&Stone, &Bronze};
		const double L = HousingEndBlock;
		const FHousingOutline Block = HousingSection(A0, A1, bOpen0, bOpen1, false, false);
		const FHousingOutline Field = HousingSection(A0, A1, bOpen0, bOpen1, true, true);
		SweepHousing(Out, P, S0, S0 + L, Block);
		SweepHousing(Out, P, S1 - L, S1, Block);
		SweepHousing(Out, P, S0 + L, S1 - L, Field);
		// The end plates.
		HousingFace(Bronze, P, S0, Block.Points, -P.Along(S0));
		HousingFace(Bronze, P, S1, Block.Points, P.Along(S1));
		// Where the blocks meet the channel: the slot's ends, and the fields' ends.
		constexpr double Sink = 0.012, FieldTop = -0.10, FieldBottom = -1.09;
		auto Rect = [](double Lo, double Hi, double B0, double B1) { return TArray<FVector2D>({FVector2D(Lo, B0), FVector2D(Hi, B0), FVector2D(Hi, B1), FVector2D(Lo, B1)}); };
		for (const double Face : {S0 + L, S1 - L})
		{
			const FVector Inward = Face < (S0 + S1) / 2 ? P.Along(Face) : -P.Along(Face);
			HousingFace(Bronze, P, Face, Rect(-SlotHalf, SlotHalf, -HousingDepth + HousingFloor, 0.0), Inward);
			if (bOpen0) { HousingFace(Stone, P, Face, Rect(A0, A0 + Sink, FieldBottom, FieldTop), Inward); }
			if (bOpen1) { HousingFace(Stone, P, Face, Rect(A1 - Sink, A1, FieldBottom, FieldTop), Inward); }
		}
	}

	/**
	 * A retracting panel, S0 … S1: the balustrade, its standards run down into the carrier bar in the housing
	 * (their feet are level, the bar follows the rake: they stop 2 cm inside it, so that only the bar comes near
	 * the housing's floor).
	 */
	static void RetractingPanel(FMeshData& M, const FRailPath& P, double S0, double S1)
	{
		Balustrade(M, P, S0, S1, true, true, 0.0, 0.0, StemFoot + 0.02, false);
		RailSweep(M, P, S0, S1, 0.0, RectSection(-0.012, 0.012, StemFoot, CarrierTop), true);
	}

	/**
	 * The wall rail's top end (s = 0) returned to the wall: a level quarter bend of radius Bend from the rail
	 * towards the wall, a straight a centimetre into it, and a rose on the wall round it.
	 */
	static void WallReturn(FMeshData& M, const FRailPath& P, double V, const FProfile& Round, double Bend)
	{
		const FVector A = P.Point(0.0, 0.0, P.Base(0.0) + V);
		const FVector N = P.Across(0.0), U = -P.Along(0.0), Up = FVector::UpVector;
		const FVector C = A + N * Bend;
		TArray<FFrame> Run;
		constexpr int32 Segs = 12;
		for (int32 I = 0; I <= Segs; ++I)
		{
			const double T = 0.5 * UE_DOUBLE_PI * I / Segs;
			FFrame F;
			F.Origin = C - N * (Bend * FMath::Cos(T)) + U * (Bend * FMath::Sin(T));
			F.AxisA = F.NormA = N * FMath::Cos(T) - U * FMath::Sin(T);
			F.AxisB = F.NormB = Up;
			F.S = -Bend * T;
			Run.Add(F);
		}
		SalonKit::Sweep(M, Run, Round);
		// On along N: how far to the circle R.
		const FFrame End = Run.Last();
		auto Reach = [&End, &N](double R)
		{
			const FVector2D E(End.Origin.X, End.Origin.Y), D(N.X, N.Y);
			const double B = FVector2D::DotProduct(E, D), CC = E.SizeSquared() - R * R;
			return -B + FMath::Sqrt(FMath::Max(0.0, B * B - CC));
		};
		auto At = [&End, &N](double L)
		{
			FFrame F = End;
			F.Origin = End.Origin + N * L;
			F.S = End.S - L;
			return F;
		};
		SalonKit::Sweep(M, TArray<FFrame>({End, At(Reach(SC::StairWall + 0.01))}), Round);
		// The rose: a disc 45 mm round, 12 mm proud of the wall.
		FProfile Rose;
		for (int32 I = 0; I < 24; ++I)
		{
			const double T = Turn * I / 24;
			Rose.Add(0.045 * FMath::Cos(T), 0.045 * FMath::Sin(T), true);
		}
		Rose.bClosed = true;
		const TArray<FFrame> Disc = {At(Reach(SC::StairWall - 0.012)), At(Reach(SC::StairWall + 0.005))};
		SalonKit::Sweep(M, Disc, Rose);
		SalonKit::CapConvex(M, Disc[0], Rose, -N);
	}

	static void RoundFinial(FMeshData& M, const FVector& Ball, double R)
	{
		M.Patch(16, 12,
			[&](int32 I, int32 J)
			{
				const double Phi = Turn * I / 16, Theta = UE_DOUBLE_PI * J / 12;
				return Ball + FVector(FMath::Sin(Theta) * FMath::Cos(Phi), FMath::Sin(Theta) * FMath::Sin(Phi), -FMath::Cos(Theta)) * R;
			},
			[](const FVector& P) { return FVector2D(P.X, P.Z); }, [Ball](const FVector& P) { return P - Ball; });
	}

	void BuildRails(FSunClockMeshes& M)
	{
		const FStairLayout& L = FStairLayout::Get();
		// Round the well, from the top landing's back edge.
		auto Circle = [&L](double R, double Phi0, TFunction<double(double)> Base)
		{
			FRailPath P;
			P.At = [R, Phi0](double S, double Off) { return Polar(R + Off, Phi0 - S / R, 0); };
			P.Across = [R, Phi0](double S) { return RadialDir(Phi0 - S / R); };
			P.Base = [R, Phi0, Base](double S) { return Base(Phi0 - S / R); };
			for (const double K : L.Kinks())
			{
				if (K <= Phi0) { P.Kinks.Add((Phi0 - K) * R); }
			}
			P.Kinks.Sort();
			return P;
		};
		auto Curb = [&L](double Phi) { return L.CurbTop(Phi); };
		auto Ramp = [&L](double Phi) { return L.RampZ(Phi); };
		const FRailPath Well = Circle(RailRadius, L.Top, Curb);
		const double SplitS = (L.Top - L.Split) * RailRadius, EndS = (L.Top - (L.End - 1.0 * Deg)) * RailRadius;
		constexpr double Clear = 0.005;   // between a panel's ends and its housing's end blocks

		// The upper flight's well side, retracting: the housing from the landing's back edge to a centimetre short
		// of the split's standard (r 2.07 to the string's face), the panel in it, its fence in its plane.
		{
			const double H0 = 0.0, H1 = SplitS - 0.01;
			const double P0 = H0 + HousingEndBlock + Clear, P1 = H1 - HousingEndBlock - Clear;
			Housing(M.Housing, M.HousingStone, Well, H0, H1, WellHousingInner - RailRadius, CurbInner - RailRadius, true, false);
			RetractingPanel(M.TopRailBronze, Well, P0, P1);
			RailFence(M.TopRailFence, Well, P0, P1, 0.01, 0.92);
		}
		// Below the split, fixed: brackets from the standards to the string's face (r 2.2, 1 cm in).
		const double B0 = PostHalf, B1 = CurbInner + 0.01 - RailRadius;
		Balustrade(M.RailBronze, Well, SplitS, EndS, true, false, B0, B1);
		// The newel at the foot, with its ball.
		{
			const FVector Foot = Polar(RailRadius, L.Top - EndS / RailRadius, 0);
			const double Top = Well.Base(EndS) + HandrailV + 0.03;
			FrameBox(M.RailBronze, Foot, RadialDir(L.Top - EndS / RailRadius), TangentDir(L.Top - EndS / RailRadius), FVector::UpVector,
					 FVector(-0.028, -0.028, SC::Floor - 0.01), FVector(0.028, 0.028, Top), FMeshData::AllFaces & ~FMeshData::NegZ);
			FrameBox(M.RailBronze, Foot, RadialDir(L.Top - EndS / RailRadius), TangentDir(L.Top - EndS / RailRadius), FVector::UpVector,
					 FVector(-0.036, -0.036, Top), FVector(0.036, 0.036, Top + 0.02));
			RoundFinial(M.RailBronze, Foot + FVector(0, 0, Top + 0.02 + 0.042), 0.045);
		}

		// Behind the top landing, retracting: straight across its back edge, the housing's top flush with the
		// landing (under the dial's bronze rim, −0.14), from the well housing's face to a centimetre into the wall.
		{
			FRailPath Back;
			const FVector BackDir = RadialDir(L.Top), BackAcross = TangentDir(L.Top);
			Back.At = [BackDir, BackAcross](double S, double Off) { return BackDir * (CurbInner + S) + BackAcross * (BackPlane + Off); };
			Back.Across = [BackAcross](double) { return BackAcross; };
			const double Landing = L.TreadZ(1);
			Back.Base = [Landing](double) { return Landing; };
			const double H0 = WellHousingInner - CurbInner, H1 = SC::StairWall + 0.01 - CurbInner;
			const double P0 = H0 + HousingEndBlock + Clear, P1 = H1 - 0.01 - HousingEndBlock - Clear;
			Housing(M.Housing, M.HousingStone, Back, H0, H1, -BackPlane, BackPlane, false, true);
			RetractingPanel(M.TopRailBronze, Back, P0, P1);
			RailFence(M.TopRailFence, Back, P0, P1, 0.01, 0.92);
		}

		// The wall's handrail, from a tread above the split (returned to the wall there) to the floor.
		const double WallPhi0 = L.WallRailStart;
		const FRailPath Wall = Circle(WallRailRadius, WallPhi0, Ramp);
		const double WallEnd = (WallPhi0 - (L.End - 4.0 * Deg)) * WallRailRadius;
		FProfile Round;
		for (int32 I = 0; I < 16; ++I)
		{
			const double T = Turn * I / 16;
			Round.Add(0.025 * FMath::Cos(T), 0.025 * FMath::Sin(T), true);
		}
		Round.bClosed = true;
		constexpr double WallV = 0.92;
		RailSweep(M.RailBronze, Wall, 0.0, WallEnd, WallV, Round, false, true);
		WallReturn(M.RailBronze, Wall, WallV, Round, 0.05);
		for (double S = 0.3; S < WallEnd; S += 1.25)
		{
			RailUpright(M.RailBronze, Wall, S, 0.008, WallV - 0.085, WallV - 0.015, -0.008, 0.008);
			RailUpright(M.RailBronze, Wall, S, 0.008, WallV - 0.085, WallV - 0.068, 0.0, SC::StairWall + 0.01 - WallRailRadius);
		}

		// The fixed fence (hidden, for collision): along the string's face on the ramp, below the split.
		const FRailPath Fence = Circle(CurbOuter + 0.01, L.Top, Ramp);
		const double FenceSplit = (L.Top - L.Split) * (CurbOuter + 0.01), FenceEnd = (L.Top - (L.End - 1.0 * Deg)) * (CurbOuter + 0.01);
		RailFence(M.RailFence, Fence, FenceSplit, FenceEnd, -0.30, 1.10);
	}
}
