#include "Albion/AlbionBuild.h"

/**
 * Albion's masonry, floors, porch and outside (AlbionBuild.h). Plan metres (x east, plan y south, up); every face is
 * written facing the side it bounds; solids that meet overlap by a few millimetres or share their edge.
 */
namespace AlbionMasonryImpl
{
	namespace AP = AlbionPlan;
	using namespace AlbionKit;
	using AlbionBuild::FParts;
	using namespace AlbionBuild;

	const FVector kUp(0, 0, 1);
	constexpr double kChamfer = 0.06;       // the arrises of openings
	constexpr double kGlassDepth = 0.30;    // the lancets' glass, from the inner face (the wall's middle)
	constexpr double kSkirtProud = 0.015;
	constexpr double kPilasterHalf = 0.15, kPilasterProud = 0.10;
	constexpr double kCorniceFoot = 9.72, kCorniceProud = 0.12;
	constexpr double kParapetInner = 0.30;  // the parapet's inner face, from the wall's inner face (the gutter's width)
	constexpr double kCoping = 0.15, kCopingOver = 0.05;
	constexpr double kPlinthTop = 0.60, kPlinthProud = 0.08;
	constexpr double kSink = 1.2;           // the plinth runs this far down (its footing: the lawn falls away to the south)
	constexpr double kRail = 4.2;           // the picture rail (albion_hang.py RAIL)

	/** A band-cut wall face's pick: the red bands, the buff between; nothing under the skirting (its solid covers it). */
	auto InnerPick(FParts& P)
	{
		return [&P](double Z) -> FMeshData*
		{
			if (Z < AP::Skirting) { return nullptr; }
			return InBand(Z) ? &P[SlotRed] : &P[SlotBuff];
		};
	}

	auto OuterPick(FParts& P)
	{
		return [&P](double Z) -> FMeshData*
		{
			if (Z < kPlinthTop) { return nullptr; }
			return InBand(Z) ? &P[SlotExtRed] : &P[SlotExtBuff];
		};
	}

	/** An outer face's cuts: the bands', and the plinth's top (the face starts there: under it the plinth covers). */
	TArray<double> OuterCuts(double Top)
	{
		TArray<double> Out = BandCuts(Top);
		Out.Add(kPlinthTop);
		Out.Sort();
		return Out;
	}

	/**
	 * The chamfer between an opening's outline grown on the face (at depth 0) and its plain outline (at depth D), both
	 * from the same maker so their stations pair: the jambs and the arch, facing into the room and the opening.
	 */
	void Chamfer(FMeshData& M, const FVector& Origin, const FVector& U, const FVector& N, const FOpening& Grown, const FOpening& Plain,
				 double D)
	{
		const int32 NS = FMath::Min(Grown.Stations.Num(), Plain.Stations.Num());
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			for (int32 k = 0; k + 1 < NS; ++k)
			{
				auto Pt = [&](const FOpening& O, int32 i, double Depth)
				{
					const double Z = O.Stations[i];
					const double H = i == NS - 1 ? 0.0 : (i == 0 ? O.HalfAt(O.Sill + 1e-6) : O.HalfAt(Z));
					return Origin + U * (O.U + Side * H) + kUp * Z - N * Depth;
				};
				const FVector A = Pt(Grown, k, 0.0), B = Pt(Grown, k + 1, 0.0), C = Pt(Plain, k + 1, D), E = Pt(Plain, k, D);
				FVector Nrm = FVector::CrossProduct(B - A, E - A).GetSafeNormal();
				if (Nrm.IsNearlyZero()) { Nrm = FVector::CrossProduct(C - B, A - B).GetSafeNormal(); }
				const FVector Toward = N + U * (-Side) * 0.5 - kUp * 0.1;
				if (FVector::DotProduct(Nrm, Toward) < 0.0) { Nrm = -Nrm; }
				M.Poly({A, B, C, E}, Nrm);
			}
		}
	}

	/** A flat sill across an opening from depth D0 (half-width H0) to D1 (H1), at height Z, facing up. */
	void Sill(FMeshData& M, const FVector& Origin, const FVector& U, const FVector& N, double UC, double H0, double H1, double D0, double D1, double Z0,
			  double Z1)
	{
		const FVector A = Origin + U * (UC - H0) - N * D0 + kUp * Z0, B = Origin + U * (UC + H0) - N * D0 + kUp * Z0;
		const FVector C = Origin + U * (UC + H1) - N * D1 + kUp * Z1, E = Origin + U * (UC - H1) - N * D1 + kUp * Z1;
		FVector Nrm = FVector::CrossProduct(B - A, E - A).GetSafeNormal();
		if (Nrm.Z < 0.0) { Nrm = -Nrm; }
		M.Poly({A, B, C, E}, Nrm);
	}

	/** A straight run of skirting along a face from U0 to U1 (the face at Origin, U along it, N its facing). */
	void Skirting(FMeshData& M, const FVector& Origin, const FVector& U, const FVector& N, double U0, double U1)
	{
		if (U1 - U0 < 1e-4) { return; }
		const FVector A0 = Origin + U * U0, A1 = Origin + U * U1;
		const double H = AP::Skirting, P = kSkirtProud;
		// Front (a 4 mm bevel on the top arris), top, ends; the back is against the wall.
		M.Rect(A0 + N * P, A1 + N * P, A1 + N * P + kUp * (H - 0.004), A0 + N * P + kUp * (H - 0.004), N);
		const FVector Bev = (N + kUp).GetSafeNormal();
		M.Rect(A0 + N * P + kUp * (H - 0.004), A1 + N * P + kUp * (H - 0.004), A1 + N * (P - 0.004) + kUp * H, A0 + N * (P - 0.004) + kUp * H, Bev);
		M.Rect(A0 + N * (P - 0.004) + kUp * H, A1 + N * (P - 0.004) + kUp * H, A1 + kUp * H, A0 + kUp * H, kUp);
		M.Rect(A0, A0 + N * P, A0 + N * P + kUp * H, A0 + kUp * H, -U);
		M.Rect(A1, A1 + N * P, A1 + N * P + kUp * H, A1 + kUp * H, U);
	}

	/** A box in a face's frame: U0 … U1 along it, out from the face P0 … P1 (along N), Z0 … Z1; the back face left off. */
	void FaceBox(FMeshData& M, const FVector& Origin, const FVector& U, const FVector& N, double U0, double U1, double P0, double P1, double Z0,
				 double Z1, bool bBack = false)
	{
		auto Q = [&](double Ua, double Pa, double Za) { return Origin + U * Ua + N * Pa + kUp * Za; };
		M.Rect(Q(U0, P1, Z0), Q(U1, P1, Z0), Q(U1, P1, Z1), Q(U0, P1, Z1), N);
		if (bBack) { M.Rect(Q(U0, P0, Z0), Q(U1, P0, Z0), Q(U1, P0, Z1), Q(U0, P0, Z1), -N); }
		M.Rect(Q(U0, P0, Z0), Q(U0, P1, Z0), Q(U0, P1, Z1), Q(U0, P0, Z1), -U);
		M.Rect(Q(U1, P0, Z0), Q(U1, P1, Z0), Q(U1, P1, Z1), Q(U1, P0, Z1), U);
		M.Rect(Q(U0, P0, Z1), Q(U1, P0, Z1), Q(U1, P1, Z1), Q(U0, P1, Z1), kUp);
		M.Rect(Q(U0, P0, Z0), Q(U1, P0, Z0), Q(U1, P1, Z0), Q(U0, P1, Z0), -kUp);
	}

	/** A wall moulding swept along a plan path (the profile's A out of the wall, B up; the solid on the path's left). */
	void Moulding(FMeshData& M, const TArray<FVector2D>& Path, const SalonKit::FProfile& Profile)
	{
		for (const TArray<SalonKit::FFrame>& Run : SalonKit::PlanRuns(Path, false))
		{
			SalonKit::Sweep(M, Run, Profile);
			const FVector T0 = (Run[1].Origin - Run[0].Origin).GetSafeNormal();
			const FVector T1 = (Run.Last().Origin - Run[Run.Num() - 2].Origin).GetSafeNormal();
			SalonKit::CapConvex(M, Run[0], Profile, -T0);
			SalonKit::CapConvex(M, Run.Last(), Profile, T1);
		}
	}

	/** The cornice that runs round the aisles at the vaults' springing: a chamfered string under a plain fillet. */
	SalonKit::FProfile CorniceProfile()
	{
		SalonKit::FProfile P;
		P.Add(-0.01, kCorniceFoot).Add(0.03, kCorniceFoot).Add(0.075, kCorniceFoot + 0.05).Add(0.075, kCorniceFoot + 0.12)
		 .Add(kCorniceProud, kCorniceFoot + 0.16).Add(kCorniceProud, AP::Spring).Add(-0.01, AP::Spring);
		return P;
	}

	// ============================================================================================ inside

	/** A side wall's inner face (bSouthUp: the west wall, facing east), its skirting, pilasters, lancets' inner halves. */
	void SideWall(FParts& P, bool bWest)
	{
		const double X = bWest ? AP::X0 : AP::X1;
		const FVector N(bWest ? 1 : -1, 0, 0);
		const FVector U(0, 1, 0);
		const FVector O(X, 0, 0);
		TArray<FOpening> Faces;
		for (int32 b = 0; b < 6; ++b) { Faces.Add(LancetOpening(AP::BayCentreY(b), kChamfer)); }
		WallFace(InnerPick(P), O, U, N, AP::Y0, AP::Y1, 0.0, kCorniceFoot + 0.01, Faces, BandCuts(kCorniceFoot));
		for (int32 b = 0; b < 6; ++b)
		{
			const double YC = AP::BayCentreY(b);
			const FOpening Plain = LancetOpening(YC), Grown = LancetOpening(YC, kChamfer);
			Chamfer(P[SlotDressing], O, U, N, Grown, Plain, kChamfer);
			Reveal(P[SlotDressing], O, U, N, Plain, kChamfer, kGlassDepth + 0.01);
			// The inner sill: level to the glass, a little proud over the chamfer's foot.
			Sill(P[SlotDressing], O, U, N, YC, AP::LancetHalf + kChamfer, AP::LancetHalf, 0.0, kChamfer, AP::LancetSill, AP::LancetSill);
			Sill(P[SlotDressing], O, U, N, YC, AP::LancetHalf, AP::LancetHalf, kChamfer, kGlassDepth + 0.01, AP::LancetSill, AP::LancetSill);
		}
		// Skirting between the pilasters, round each pilaster.
		double From = AP::Y0;
		for (int32 Line = 2; Line <= 6; ++Line)
		{
			const double Y = AP::LineY(Line);
			Skirting(P[SlotSlate], O, U, N, From, Y - kPilasterHalf);
			From = Y + kPilasterHalf;
			// The pilaster (dressed stone), its skirting, and the corbel the main rib's shoe sits on.
			const FVector PO = O + N * kPilasterProud;
			FaceBox(P[SlotDressing], O, U, N, Y - kPilasterHalf, Y + kPilasterHalf, -0.01, kPilasterProud, AP::Skirting - 0.01, 9.30);
			Skirting(P[SlotSlate], PO, U, N, Y - kPilasterHalf - kSkirtProud, Y + kPilasterHalf + kSkirtProud);
			Skirting(P[SlotSlate], O + U * (Y - kPilasterHalf), N, -U, 0.0, kPilasterProud);
			Skirting(P[SlotSlate], O + U * (Y + kPilasterHalf), N, U, 0.0, kPilasterProud);
			FaceBox(P[SlotDressing], O, U, N, Y - 0.17, Y + 0.17, -0.01, 0.15, 9.30, 9.45);
			FaceBox(P[SlotDressing], O, U, N, Y - 0.19, Y + 0.19, -0.01, 0.20, 9.45, 9.60);
			FaceBox(P[SlotDressing], O, U, N, Y - 0.21, Y + 0.21, -0.01, 0.26, 9.60, kCorniceFoot + 0.02);
		}
		Skirting(P[SlotSlate], O, U, N, From, AP::Y1);
		// The picture rail: a bronze rod on brackets, bay by bay between the pilasters (the works hang from it by rods).
		for (int32 b = 0; b < 6; ++b)
		{
			const double A = AP::Y0 + 6.0 * b + (b == 0 ? 0.05 : kPilasterHalf + 0.03), B = AP::Y0 + 6.0 * (b + 1) - (b == 5 ? 0.05 : kPilasterHalf + 0.03);
			const FVector R0 = O + N * 0.035 + U * A + kUp * kRail, R1 = O + N * 0.035 + U * B + kUp * kRail;
			TArray<FVector2D> Round;
			for (int32 k = 0; k < 12; ++k) { const double T = 2.0 * kPi * k / 12.0; Round.Add(FVector2D(0.011 * FMath::Cos(T), 0.011 * FMath::Sin(T))); }
			SweepStations(P[SlotBronze], {{R0, N, kUp}, {R1, N, kUp}}, Round);
			for (const FVector& End : {R0, R1}) { Ball(P[SlotBronze], End, 0.016, 12); }
			const int32 NB = FMath::Max(2, FMath::RoundToInt32((B - A) / 1.4) + 1);
			for (int32 k = 0; k < NB; ++k)
			{
				const double Along = FMath::Lerp(A + 0.15, B - 0.15, double(k) / (NB - 1));
				FaceBox(P[SlotBronze], O, U, N, Along - 0.012, Along + 0.012, -0.005, 0.03, kRail - 0.035, kRail - 0.012);
				FaceBox(P[SlotBronze], O, U, N, Along - 0.03, Along + 0.03, -0.005, 0.004, kRail - 0.06, kRail + 0.0);
			}
		}
	}

	/** The court door's hood mould (the label over the arch) on the court side, with its returns. */
	void HoodMould(FMeshData& M, const FVector& Origin, const FVector& U, const FVector& N)
	{
		const FOpening H = CourtDoorOpening(0.16);
		TArray<FVector> Path;
		const double Spring = AP::CourtDoorSpring;
		Path.Add(Origin + U * (H.U - H.HalfAt(Spring) - 0.28) + kUp * Spring);
		for (int32 i = 1; i < H.Stations.Num(); ++i)
		{
			const double Z = H.Stations[i];
			const double Half = i == H.Stations.Num() - 1 ? 0.0 : H.HalfAt(Z);
			Path.Add(Origin + U * (H.U - Half) + kUp * Z);
		}
		for (int32 i = H.Stations.Num() - 2; i >= 1; --i)
		{
			const double Z = H.Stations[i];
			Path.Add(Origin + U * (H.U + H.HalfAt(Z)) + kUp * Z);
		}
		Path.Add(Origin + U * (H.U + H.HalfAt(Spring) + 0.28) + kUp * Spring);
		const FurnitureKit::FPath FP(Path, N);
		// In the frame: B out of the wall (N), A across the path in the wall's plane. A drip-mould: a square fillet over a
		// sloped weathering, 9 cm high, 7 cm proud.
		FurnitureKit::FBarEnds Ends;
		FurnitureKit::Bar(M, FP, [](double, double Inset)
		{
			return TArray<FVector2D>({FVector2D(-0.045 + Inset, -0.01), FVector2D(0.045 - Inset, -0.01), FVector2D(0.045 - Inset, 0.035 - Inset),
									  FVector2D(0.02, 0.07 - Inset), FVector2D(-0.03, 0.07 - Inset), FVector2D(-0.045 + Inset, 0.05 - Inset)});
		}, Ends);
		// Label stops: carved blocks at the returns' ends.
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const double UC = H.U + Side * (H.HalfAt(Spring) + 0.28);
			FaceBox(M, Origin, U, N, UC - 0.07, UC + 0.07, -0.01, 0.09, Spring - 0.16, Spring + 0.06);
		}
	}

	/** An inscribed tablet: its face (UVs 0 … 1 across, bottom to top) in Face, its edges in Edge. */
	void Tablet(FMeshData& Face, FMeshData& Edge, const FVector& Origin, const FVector& U, const FVector& N, double U0, double U1, double Z0, double Z1,
				double Proud)
	{
		const double B = 0.02;   // the chamfered edge
		auto Q = [&](double Ua, double Pa, double Za) { return Origin + U * Ua + N * Pa + kUp * Za; };
		const TArray<FVector2D> Outline = {FVector2D(U0 + B, Z0 + B), FVector2D(U1 - B, Z0 + B), FVector2D(U1 - B, Z1 - B), FVector2D(U0 + B, Z1 - B)};
		Planar(Face, Outline, Origin + N * Proud, U, kUp, N, [&](const FVector& Pt)
		{
			const double Ua = FVector::DotProduct(Pt - Origin, U), Za = Pt.Z;
			return FVector2D((Ua - U0) / (U1 - U0), (Za - Z0) / (Z1 - Z0));   // (V up: the photo layer samples (U, −V))
		});
		// The chamfers round it and its sides.
		const FVector Corners[4][2] = {{Q(U0, Proud - B, Z0), Q(U0 + B, Proud, Z0 + B)}, {Q(U1, Proud - B, Z0), Q(U1 - B, Proud, Z0 + B)},
									   {Q(U1, Proud - B, Z1), Q(U1 - B, Proud, Z1 - B)}, {Q(U0, Proud - B, Z1), Q(U0 + B, Proud, Z1 - B)}};
		for (int32 i = 0; i < 4; ++i)
		{
			const int32 j = (i + 1) % 4;
			const FVector Mid = (Corners[i][0] + Corners[j][0]) * 0.5;
			const FVector Centre = Q(0.5 * (U0 + U1), 0.0, 0.5 * (Z0 + Z1));
			FVector Out = Mid - Centre;
			Out -= N * FVector::DotProduct(Out, N);
			Out = Out.GetSafeNormal();
			Edge.Poly({Corners[i][0], Corners[j][0], Corners[j][1], Corners[i][1]}, (Out + N).GetSafeNormal());
			const FVector Back0 = Corners[i][0] - N * (Proud - B + 0.01), Back1 = Corners[j][0] - N * (Proud - B + 0.01);
			Edge.Poly({Back0, Back1, Corners[j][0], Corners[i][0]}, Out);
		}
	}

	/**
	 * An end wall's inner face (bNorth: facing south, into the court), in three parts: the aisles' to the springing, the
	 * nave's to the screen's cill at 8.5 m (the court door in the north one), their returns, the cornice round the aisles.
	 */
	void EndWall(FParts& P, bool bNorth)
	{
		const double Y = bNorth ? AP::Y0 : AP::Y1;
		const double YO = bNorth ? AP::Y0 - AP::Wall : AP::Y1 + AP::Wall;
		const FVector N(0, bNorth ? 1 : -1, 0);
		const FVector U(1, 0, 0);
		const FVector O(0, Y, 0);
		TArray<FOpening> Door;
		if (bNorth) { Door.Add(CourtDoorOpening(0.10)); }
		auto Pick = InnerPick(P);
		WallFace(Pick, O, U, N, AP::X0, -AP::NaveHalf, 0.0, kCorniceFoot + 0.01, {}, BandCuts(kCorniceFoot));
		WallFace(Pick, O, U, N, AP::NaveHalf, AP::X1, 0.0, kCorniceFoot + 0.01, {}, BandCuts(kCorniceFoot));
		WallFace(Pick, O, U, N, -AP::NaveHalf, AP::NaveHalf, 0.0, AP::NaveStone, Door, BandCuts(AP::NaveStone));
		// The aisles' walls rise past the nave's: their returns at x ±6 from the cill to the springing.
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const double X = Side * AP::NaveHalf;
			const FVector RN(-Side, 0, 0);
			P[SlotBuff].Rect(FVector(X, Y, AP::NaveStone), FVector(X, YO, AP::NaveStone), FVector(X, YO, AP::Spring), FVector(X, Y, AP::Spring), RN);
			// The aisles' wall tops at the springing (the gables' glass stands on their middle).
			const double XA = Side * AP::X1 * 1.0;
			P[SlotDressing].Rect(FVector(FMath::Min(X, XA), Y, AP::Spring), FVector(FMath::Max(X, XA), Y, AP::Spring),
								 FVector(FMath::Max(X, XA), 0.5 * (Y + YO), AP::Spring), FVector(FMath::Min(X, XA), 0.5 * (Y + YO), AP::Spring), kUp);
		}
		// The nave's cill under the screen: across the wall, weathered outwards.
		{
			const double X0 = -AP::NaveHalf, X1 = AP::NaveHalf;
			const double ZIn = AP::NaveStone + 0.02, ZOut = AP::NaveStone - 0.03;
			const FVector A(X0, Y + N.Y * 0.02, ZIn), B(X1, Y + N.Y * 0.02, ZIn), C(X1, YO - N.Y * 0.05, ZOut), D(X0, YO - N.Y * 0.05, ZOut);
			P[SlotDressing].Poly({A, B, C, D}, (kUp + FVector(0, -N.Y * 0.05, 0)).GetSafeNormal());
			P[SlotDressing].Rect(FVector(X0, Y + N.Y * 0.02, AP::NaveStone - 0.12), FVector(X1, Y + N.Y * 0.02, AP::NaveStone - 0.12),
								 FVector(X1, Y + N.Y * 0.02, ZIn), FVector(X0, Y + N.Y * 0.02, ZIn), N);
			P[SlotDressing].Rect(FVector(X0, Y, AP::NaveStone - 0.12), FVector(X1, Y, AP::NaveStone - 0.12), FVector(X1, Y + N.Y * 0.02, AP::NaveStone - 0.12),
								 FVector(X0, Y + N.Y * 0.02, AP::NaveStone - 0.12), -kUp);
		}
		// Skirting (round the door's jambs in the north wall).
		if (bNorth)
		{
			Skirting(P[SlotSlate], O, U, N, AP::X0, -AP::CourtDoorHalf - 0.10 - 0.001);
			Skirting(P[SlotSlate], O, U, N, AP::CourtDoorHalf + 0.10 + 0.001, AP::X1);
		}
		else
		{
			Skirting(P[SlotSlate], O, U, N, AP::X0, AP::X1);
		}
		if (bNorth)
		{
			const FOpening Grown = CourtDoorOpening(0.10), Plain = CourtDoorOpening(0.0);
			Chamfer(P[SlotDressing], O, U, N, Grown, Plain, 0.10);
			Reveal(P[SlotDressing], O, U, N, Plain, 0.10, AP::Wall - 0.06);
			// The porch side's chamfer (the porch face is the wall's outer face).
			const FVector OP(0, AP::Y0 - AP::Wall, 0);
			Chamfer(P[SlotDressing], OP, U, -N, CourtDoorOpening(0.06), Plain, 0.06);
			HoodMould(P[SlotDressing], O, U, N);
			// Ruskin's words over the door (Modern Painters, 1843), on a tablet: "rejecting nothing, selecting nothing,
			// scorning nothing".
			Tablet(P[SlotRuskin], P[SlotDressing], O, U, N, -AP::RuskinHalf, AP::RuskinHalf, AP::RuskinBottom, AP::RuskinTop, 0.03);
			// ALBION, 1848 · 1898, carved on the west aisle's north wall.
			Tablet(P[SlotName], P[SlotDressing], O, U, N, -12.6, -7.4, 1.85, 3.45, 0.03);
		}
		// The half-columns' bases and the cornice round the aisles are the columns' and the cornice's.
	}

	/** The cornice round each aisle: along its end walls and side wall, over the pilasters. */
	void Cornices(FParts& P)
	{
		const SalonKit::FProfile Prof = CorniceProfile();
		// West aisle: from the south wall at x −6, west, north up the west wall, east along the north wall to x −6.
		Moulding(P[SlotDressing], {FVector2D(-AP::NaveHalf, AP::Y1), FVector2D(AP::X0, AP::Y1), FVector2D(AP::X0, AP::Y0), FVector2D(-AP::NaveHalf, AP::Y0)}, Prof);
		// East aisle: from the north wall at x 6, east, south down the east wall, west along the south wall to x 6.
		Moulding(P[SlotDressing], {FVector2D(AP::NaveHalf, AP::Y0), FVector2D(AP::X1, AP::Y0), FVector2D(AP::X1, AP::Y1), FVector2D(AP::NaveHalf, AP::Y1)}, Prof);
		// Corner corbels for the aisles' gable ribs.
		for (const double X : {AP::X0, AP::X1})
		{
			for (const double Y : {AP::Y0, AP::Y1})
			{
				const double SX = X < 0 ? 1.0 : -1.0, SY = Y < 30 ? 1.0 : -1.0;
				const FVector Lo(FMath::Min(X, X + SX * 0.22), FMath::Min(Y, Y + SY * 0.22), 9.35);
				const FVector Hi(FMath::Max(X, X + SX * 0.22), FMath::Max(Y, Y + SY * 0.22), kCorniceFoot + 0.02);
				P[SlotDressing].Box(Lo, Hi, FMeshData::AllFaces);
			}
		}
	}

	// ============================================================================================ outside

	void OuterSideWall(FParts& P, bool bWest)
	{
		const double X = bWest ? AP::X0 - AP::Wall : AP::X1 + AP::Wall;
		const FVector N(bWest ? -1 : 1, 0, 0);
		const FVector U(0, 1, 0);
		const FVector O(X, 0, 0);
		TArray<FOpening> Faces;
		for (int32 b = 0; b < 6; ++b) { Faces.Add(LancetOpening(AP::BayCentreY(b), kChamfer)); }
		WallFace(OuterPick(P), O, U, N, AP::Y0 - AP::Wall, AP::Y1 + AP::Wall, 0.0, AP::Parapet + 0.01, Faces, OuterCuts(AP::Parapet));
		for (int32 b = 0; b < 6; ++b)
		{
			const double YC = AP::BayCentreY(b);
			const FOpening Plain = LancetOpening(YC), Grown = LancetOpening(YC, kChamfer);
			Chamfer(P[SlotExtDressing], O, U, N, Grown, Plain, kChamfer);
			Reveal(P[SlotExtDressing], O, U, N, Plain, kChamfer, AP::Wall - kGlassDepth + 0.01);
			// The outer sill: weathered, proud of the face with a drip.
			const double H0 = AP::LancetHalf + kChamfer + 0.08;
			Sill(P[SlotExtDressing], O, U, N, YC, AP::LancetHalf, AP::LancetHalf, AP::Wall - kGlassDepth + 0.01, 0.0, AP::LancetSill + 0.04, AP::LancetSill);
			FaceBox(P[SlotExtDressing], O, U, N, YC - H0, YC + H0, -0.01, 0.07, AP::LancetSill - 0.12, AP::LancetSill);
		}
	}

	void OuterEndWall(FParts& P, bool bNorth)
	{
		const double Y = bNorth ? AP::Y0 - AP::Wall : AP::Y1 + AP::Wall;
		const FVector N(0, bNorth ? -1 : 1, 0);
		const FVector U(1, 0, 0);
		const FVector O(0, Y, 0);
		auto Pick = OuterPick(P);
		const double XO = AP::X1 + AP::Wall;
		WallFace(Pick, O, U, N, -XO, -AP::NaveHalf, 0.0, AP::Parapet + 0.01, {}, OuterCuts(AP::Parapet));
		WallFace(Pick, O, U, N, AP::NaveHalf, XO, 0.0, AP::Parapet + 0.01, {}, OuterCuts(AP::Parapet));
		if (bNorth)
		{
			// The porch covers the middle up to its roof; the court door opens into it.
			WallFace(Pick, O, U, N, -AP::NaveHalf, -AP::PorchOuterHalf, 0.0, AP::NaveStone, {}, OuterCuts(AP::NaveStone));
			WallFace(Pick, O, U, N, AP::PorchOuterHalf, AP::NaveHalf, 0.0, AP::NaveStone, {}, OuterCuts(AP::NaveStone));
			WallFace(Pick, O, U, N, -AP::PorchOuterHalf, AP::PorchOuterHalf, AP::PorchRoof - 0.1, AP::NaveStone, {}, OuterCuts(AP::NaveStone));
		}
		else
		{
			WallFace(Pick, O, U, N, -AP::NaveHalf, AP::NaveHalf, 0.0, AP::NaveStone, {}, OuterCuts(AP::NaveStone));
		}
	}

	/** The plinth course round the outside: a proud base with a chamfered weathering. */
	void Plinth(FParts& P)
	{
		const double XO = AP::X1 + AP::Wall, YN = AP::Y0 - AP::Wall, YS = AP::Y1 + AP::Wall;
		SalonKit::FProfile Prof;
		Prof.Add(-0.02, -kSink).Add(kPlinthProud, -kSink).Add(kPlinthProud, kPlinthTop - 0.08).Add(0.0, kPlinthTop).Add(-0.02, kPlinthTop);
		// Round the court and the porch with the outside on the path's left (the profile's A points out): from the drum up
		// the porch's west face, round the court anticlockwise as seen on the plan, and back up the porch's east face.
		const double YD = FMath::Sqrt(AP::DrumOuter * AP::DrumOuter - AP::PorchOuterHalf * AP::PorchOuterHalf) - 0.05;
		const TArray<FVector2D> Path = {FVector2D(-AP::PorchOuterHalf, YD), FVector2D(-AP::PorchOuterHalf, YN), FVector2D(-XO, YN), FVector2D(-XO, YS),
										FVector2D(XO, YS), FVector2D(XO, YN), FVector2D(AP::PorchOuterHalf, YN), FVector2D(AP::PorchOuterHalf, YD)};
		Moulding(P[SlotExtDressing], Path, Prof);
	}

	/** A buttress at plan (X, Y) against a face whose outward normal is (NX, NY): full depth to the set-off, then less. */
	void Buttress(FParts& P, const FVector2D& Foot, const FVector2D& Out, double Half, double Depth, double Top)
	{
		const FVector2D Along(-Out.Y, Out.X);
		auto Q = [&](double A, double D, double Z) { const FVector2D Q2 = Foot + Along * A + Out * D; return FVector(Q2.X, Q2.Y, Z); };
		const double Set = AP::ButtressSetOff, D1 = Depth, D2 = Depth * 0.62;
		FMeshData& M = P[SlotExtBuff];
		FMeshData& Dr = P[SlotExtDressing];
		const FVector NO(Out.X, Out.Y, 0), NA(Along.X, Along.Y, 0);
		// Lower stage: plinth-high base course proud, then the face.
		M.Rect(Q(-Half, D1, kPlinthTop), Q(Half, D1, kPlinthTop), Q(Half, D1, Set), Q(-Half, D1, Set), NO);
		M.Rect(Q(-Half, -0.01, kPlinthTop), Q(-Half, D1, kPlinthTop), Q(-Half, D1, Set), Q(-Half, -0.01, Set), -NA);
		M.Rect(Q(Half, -0.01, kPlinthTop), Q(Half, D1, kPlinthTop), Q(Half, D1, Set), Q(Half, -0.01, Set), NA);
		// Its base course.
		Dr.Rect(Q(-Half - 0.06, D1 + 0.06, -kSink), Q(Half + 0.06, D1 + 0.06, -kSink), Q(Half + 0.06, D1 + 0.06, kPlinthTop - 0.08), Q(-Half - 0.06, D1 + 0.06, kPlinthTop - 0.08), NO);
		Dr.Rect(Q(-Half - 0.06, -0.01, -kSink), Q(-Half - 0.06, D1 + 0.06, -kSink), Q(-Half - 0.06, D1 + 0.06, kPlinthTop - 0.08), Q(-Half - 0.06, -0.01, kPlinthTop - 0.08), -NA);
		Dr.Rect(Q(Half + 0.06, -0.01, -kSink), Q(Half + 0.06, D1 + 0.06, -kSink), Q(Half + 0.06, D1 + 0.06, kPlinthTop - 0.08), Q(Half + 0.06, -0.01, kPlinthTop - 0.08), NA);
		Dr.Poly({Q(-Half - 0.06, D1 + 0.06, kPlinthTop - 0.08), Q(Half + 0.06, D1 + 0.06, kPlinthTop - 0.08), Q(Half, D1, kPlinthTop), Q(-Half, D1, kPlinthTop)}, (NO + FVector(0, 0, 1)).GetSafeNormal());
		Dr.Poly({Q(-Half - 0.06, -0.01, kPlinthTop - 0.08), Q(-Half - 0.06, D1 + 0.06, kPlinthTop - 0.08), Q(-Half, D1, kPlinthTop), Q(-Half, -0.01, kPlinthTop)}, (-NA + FVector(0, 0, 1)).GetSafeNormal());
		Dr.Poly({Q(Half + 0.06, -0.01, kPlinthTop - 0.08), Q(Half + 0.06, D1 + 0.06, kPlinthTop - 0.08), Q(Half, D1, kPlinthTop), Q(Half, -0.01, kPlinthTop)}, (NA + FVector(0, 0, 1)).GetSafeNormal());
		// The set-off: a sloped weathering from D1 back to D2 over 0.35 m.
		const double S1 = Set + 0.35;
		Dr.Poly({Q(-Half, D1, Set), Q(Half, D1, Set), Q(Half, D2, S1), Q(-Half, D2, S1)}, (NO + FVector(0, 0, 1)).GetSafeNormal());
		Dr.Poly({Q(-Half, -0.01, Set), Q(-Half, D1, Set), Q(-Half, D2, S1), Q(-Half, -0.01, S1)}, -NA);
		Dr.Poly({Q(Half, -0.01, Set), Q(Half, D1, Set), Q(Half, D2, S1), Q(Half, -0.01, S1)}, NA);
		// Upper stage, to the top, where a steep weathering takes it back into the wall.
		const double T0 = Top - 0.6;
		M.Rect(Q(-Half, D2, S1), Q(Half, D2, S1), Q(Half, D2, T0), Q(-Half, D2, T0), NO);
		M.Rect(Q(-Half, -0.01, S1), Q(-Half, D2, S1), Q(-Half, D2, T0), Q(-Half, -0.01, T0), -NA);
		M.Rect(Q(Half, -0.01, S1), Q(Half, D2, S1), Q(Half, D2, T0), Q(Half, -0.01, T0), NA);
		Dr.Poly({Q(-Half, D2, T0), Q(Half, D2, T0), Q(Half, -0.01, Top), Q(-Half, -0.01, Top)}, (NO * 0.9 + FVector(0, 0, 0.6)).GetSafeNormal());
		Dr.Poly({Q(-Half, -0.01, T0), Q(-Half, D2, T0), Q(-Half, -0.01, Top)}, -NA);
		Dr.Poly({Q(Half, -0.01, T0), Q(Half, D2, T0), Q(Half, -0.01, Top)}, NA);
	}

	/** Parapets, their copings and the lead-lined gutters inside them. */
	void Parapets(FParts& P)
	{
		const double XO = AP::X1 + AP::Wall, XP = AP::X1 + kParapetInner;
		const double YN = AP::Y0 - AP::Wall, YS = AP::Y1 + AP::Wall;
		const double YNP = AP::Y0 - kParapetInner, YSP = AP::Y1 + kParapetInner;
		const double Z0 = AP::Spring, Z1 = AP::Parapet;
		FMeshData& B = P[SlotExtBuff];
		FMeshData& L = P[SlotLead];
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const double S = Side;
			// The side walls' parapets: inner face at x ±14.3 from the gutter to the coping.
			B.Rect(FVector(S * XP, YN, Z0), FVector(S * XP, YS, Z0), FVector(S * XP, YS, Z1), FVector(S * XP, YN, Z1), FVector(-S, 0, 0));
			// The gutter: lead from the glass's foot to the parapet, turned up both sides.
			L.Rect(FVector(S * AP::X1, YN + 0.3, Z0 + 0.012), FVector(S * XP, YN + 0.3, Z0 + 0.012), FVector(S * XP, YS - 0.3, Z0 + 0.012),
				   FVector(S * AP::X1, YS - 0.3, Z0 + 0.012), kUp);
			L.Rect(FVector(S * (XP - 0.004), YN + 0.3, Z0), FVector(S * (XP - 0.004), YS - 0.3, Z0), FVector(S * (XP - 0.004), YS - 0.3, Z0 + 0.15),
				   FVector(S * (XP - 0.004), YN + 0.3, Z0 + 0.15), FVector(-S, 0, 0));
			// The end walls' parapets over the aisles, their inner faces 0.3 m in from the outer face.
			for (const double Y : {YNP, YSP})
			{
				const double NY = Y < 30 ? 1.0 : -1.0;
				const double XA = S * AP::NaveHalf, XB = S * XP;
				B.Rect(FVector(FMath::Min(XA, XB), Y, Z0), FVector(FMath::Max(XA, XB), Y, Z0), FVector(FMath::Max(XA, XB), Y, Z1), FVector(FMath::Min(XA, XB), Y, Z1),
					   FVector(0, NY, 0));
				// The parapet's end over the nave's cill.
				const double YO = Y < 30 ? YN : YS;
				B.Rect(FVector(XA, FMath::Min(Y, YO), Z0), FVector(XA, FMath::Max(Y, YO), Z0), FVector(XA, FMath::Max(Y, YO), Z1),
					   FVector(XA, FMath::Min(Y, YO), Z1), FVector(-S, 0, 0));
			}
		}
		// Copings: a weathered cap over every parapet, overhanging both faces with a drip.
		SalonKit::FProfile Cap;
		Cap.bClosed = true;
		const double W = kParapetInner - 0.0;   // the parapet's thickness (0.3)
		// In a plan sweep's frame: A across the wall (from its outer face inwards … the path runs with the outside on its left,
		// so A points out), B up.
		Cap.Add(-W - kCopingOver, Z1 - 0.02).Add(kCopingOver, Z1 - 0.02).Add(kCopingOver, Z1 + 0.06).Add(-W * 0.5, Z1 + kCoping)
		   .Add(-W - kCopingOver, Z1 + 0.06);
		// The outside on the path's left: run anticlockwise round the building seen from above with y south… a path east
		// along the north face has the outside (north, −y) on its left.
		const TArray<FVector2D> Ring = {FVector2D(AP::NaveHalf, YS), FVector2D(XO, YS), FVector2D(XO, YN), FVector2D(AP::NaveHalf, YN)};
		const TArray<FVector2D> RingW = {FVector2D(-AP::NaveHalf, YN), FVector2D(-XO, YN), FVector2D(-XO, YS), FVector2D(-AP::NaveHalf, YS)};
		for (const TArray<FVector2D>* R : {&Ring, &RingW})
		{
			for (const TArray<SalonKit::FFrame>& Run : SalonKit::PlanRuns(*R, false))
			{
				TArray<SalonKit::FFrame> Flipped = Run;
				SalonKit::Sweep(P[SlotExtDressing], Flipped, Cap);
				const FVector T0 = (Run[1].Origin - Run[0].Origin).GetSafeNormal();
				const FVector T1 = (Run.Last().Origin - Run[Run.Num() - 2].Origin).GetSafeNormal();
				SalonKit::CapConvex(P[SlotExtDressing], Run[0], Cap, -T0);
				SalonKit::CapConvex(P[SlotExtDressing], Run.Last(), Cap, T1);
			}
		}
	}

	void Exterior(FParts& P)
	{
		OuterSideWall(P, true);
		OuterSideWall(P, false);
		OuterEndWall(P, true);
		OuterEndWall(P, false);
		Plinth(P);
		Parapets(P);
		for (int32 Line = 1; Line <= AP::Lines; ++Line)
		{
			const double Y = AP::LineY(Line);
			Buttress(P, FVector2D(AP::X1 + AP::Wall, Y), FVector2D(1, 0), AP::ButtressHalf, AP::ButtressDepth, AP::ButtressTop);
			Buttress(P, FVector2D(AP::X0 - AP::Wall, Y), FVector2D(-1, 0), AP::ButtressHalf, AP::ButtressDepth, AP::ButtressTop);
		}
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			Buttress(P, FVector2D(Side * AP::NaveHalf, AP::Y0 - AP::Wall), FVector2D(0, -1), AP::ButtressHalf, AP::ButtressDepth, 10.4);
			Buttress(P, FVector2D(Side * AP::NaveHalf, AP::Y1 + AP::Wall), FVector2D(0, 1), AP::ButtressHalf, AP::ButtressDepth, 10.4);
		}
		// The nave's end walls' tops (the screens' cills) outside are the inside's cill; the side walls' tops under the
		// parapets are the gutters'.
	}

	// ============================================================================================ floors

	void Floors(FParts& P)
	{
		const FVector Up(0, 0, 1);
		// The nave: encaustic tiles on the diagonal (the material draws them, in plan metres), inside a border of plain
		// tiles 0.3 m wide along the arcades and the ends.
		const double B = 0.30;
		const double XN = AP::NaveHalf, YA = AP::Y0, YB = AP::Y1;
		P[SlotTile].Rect(FVector(-XN + B, YA + B, 0), FVector(XN - B, YA + B, 0), FVector(XN - B, YB - B, 0), FVector(-XN + B, YB - B, 0), Up);
		// The border: U along it, V across it (0 … 0.3 m from its outer edge), four strips mitred at the corners.
		auto Strip = [&](const FVector2D& A, const FVector2D& Bv, const FVector2D& Inward)
		{
			const FVector2D Along = (Bv - A).GetSafeNormal();
			const double L = FVector2D::Distance(A, Bv);
			const FVector2D A1 = A + Inward * B + Along * B, B1 = Bv + Inward * B - Along * B;
			const int32 Base = P[SlotTileBorder].Positions.Num();
			P[SlotTileBorder].Vertex(FVector(A.X, A.Y, 0), Up, FVector2D(0, 0));
			P[SlotTileBorder].Vertex(FVector(Bv.X, Bv.Y, 0), Up, FVector2D(L, 0));
			P[SlotTileBorder].Vertex(FVector(B1.X, B1.Y, 0), Up, FVector2D(L - B, B));
			P[SlotTileBorder].Vertex(FVector(A1.X, A1.Y, 0), Up, FVector2D(B, B));
			P[SlotTileBorder].Quad(Base, Base + 1, Base + 2, Base + 3);
		};
		Strip(FVector2D(-XN, YA), FVector2D(XN, YA), FVector2D(0, 1));
		Strip(FVector2D(XN, YA), FVector2D(XN, YB), FVector2D(-1, 0));
		Strip(FVector2D(XN, YB), FVector2D(-XN, YB), FVector2D(0, -1));
		Strip(FVector2D(-XN, YB), FVector2D(-XN, YA), FVector2D(1, 0));
		// The aisles: York stone flags (the material's courses), from the arcade line to the walls.
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const double X0 = Side < 0 ? AP::X0 - 0.01 : XN, X1 = Side < 0 ? -XN : AP::X1 + 0.01;
			P[SlotYork].Rect(FVector(X0, YA - 0.01, 0), FVector(X1, YA - 0.01, 0), FVector(X1, YB + 0.01, 0), FVector(X0, YB + 0.01, 0), Up);
		}
		// Under the lancets' and walls' feet the floors run 1 cm into the walls (hidden).
		// The court door's reveal: York stone, and the bronze threshold strip on the wall's middle line.
		const double YD0 = AP::Y0 - AP::Wall - 0.005, YD1 = AP::Y0 + 0.001, HD = AP::CourtDoorHalf + 0.001;
		P[SlotYork].Rect(FVector(-HD, YD0, 0), FVector(HD, YD0, 0), FVector(HD, YD1, 0), FVector(-HD, YD1, 0), Up);
		const double YM = AP::Y0 - AP::Wall * 0.5, SW = 0.03;
		const double HS = AP::CourtDoorHalf + 0.015;
		P[SlotBronze].Rect(FVector(-HS, YM - SW, 0.0005), FVector(HS, YM - SW, 0.0005), FVector(HS, YM + SW, 0.0005), FVector(-HS, YM + SW, 0.0005), Up);
	}

	// ============================================================================================ the porch

	/** The drum's outer face (r 11.2) at plan x. */
	double DrumY(double X) { return FMath::Sqrt(FMath::Max(0.0, AP::DrumOuter * AP::DrumOuter - X * X)); }

	void Porch(FParts& P)
	{
		const double H = AP::PorchHalf, HO = AP::PorchOuterHalf;
		const double YL = 11.30;                          // the lining's face against the drum
		const double YC = AP::Y0 - AP::Wall;              // the court's north wall (outer face) = the porch's south end
		const double Top = AP::PorchCeiling;
		auto Pick = InnerPick(P);
		// Side walls (inner faces at x ±3), from the lining to the court wall.
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			const FVector N(-Side, 0, 0);
			WallFace(Pick, FVector(Side * H, 0, 0), FVector(0, 1, 0), N, YL - 0.01, YC + 0.01, 0.0, Top + 0.01, {}, BandCuts(Top));
			Skirting(P[SlotSlate], FVector(Side * H, 0, 0), FVector(0, 1, 0), N, YL, YC);
			// Outer faces (x ±3.6), from the drum to the court's north face, to under the coping.
			WallFace(OuterPick(P), FVector(Side * HO, 0, 0), FVector(0, 1, 0), -N, DrumY(HO) - 0.05, YC + 0.01, 0.0, AP::PorchRoof - 0.1,
					 {}, OuterCuts(AP::PorchRoof));
		}
		// The porch's south end: the court wall's outer face inside the porch, with the court door.
		{
			const FVector N(0, -1, 0);
			WallFace(Pick, FVector(0, YC, 0), FVector(1, 0, 0), N, -H, H, 0.0, Top + 0.01, {CourtDoorOpening(0.06)}, BandCuts(Top));
			Skirting(P[SlotSlate], FVector(0, YC, 0), FVector(1, 0, 0), N, -H, -AP::CourtDoorHalf - 0.061);
			Skirting(P[SlotSlate], FVector(0, YC, 0), FVector(1, 0, 0), N, AP::CourtDoorHalf + 0.061, H);
		}
		// The lining against the drum: a banded face at y 11.3 round the Rotunda's door (round-arched, 4 m, springing
		// 4.5 m), whose reveal it carries on from the drum's outer face.
		{
			const FVector N(0, 1, 0);
			const FOpening Door = ArchedOpening(0.0, AP::RotundaDoorHalf, 0.0, AP::RotundaDoorSpring, AP::RotundaDoorSpring + AP::RotundaDoorHalf, 24);
			WallFace(Pick, FVector(0, YL, 0), FVector(1, 0, 0), N, -H - 0.01, H + 0.01, 0.0, Top + 0.01, {Door}, BandCuts(Top));
			Skirting(P[SlotSlate], FVector(0, YL, 0), FVector(1, 0, 0), N, -H, -AP::RotundaDoorHalf);
			Skirting(P[SlotSlate], FVector(0, YL, 0), FVector(1, 0, 0), N, AP::RotundaDoorHalf, H);
			// The reveal, station by station from the drum's face to the lining.
			for (int32 Side = -1; Side <= 1; Side += 2)
			{
				for (int32 k = 0; k + 1 < Door.Stations.Num(); ++k)
				{
					const double Za = Door.Stations[k], Zb = Door.Stations[k + 1];
					const double Ha = k == 0 ? Door.HalfAt(Door.Sill + 1e-6) : Door.HalfAt(Za);
					const double Hb = k + 1 == Door.Stations.Num() - 1 ? 0.0 : Door.HalfAt(Zb);
					const FVector A(Side * Ha, DrumY(Ha), Za), B(Side * Hb, DrumY(Hb), Zb);
					const FVector C(Side * Hb, YL, Zb), D(Side * Ha, YL, Za);
					FVector Nrm = FVector::CrossProduct(B - A, D - A).GetSafeNormal();
					if (FVector::DotProduct(Nrm, FVector(-Side, 0, -0.01)) < 0.0) { Nrm = -Nrm; }
					P[SlotDressing].Poly({A, B, C, D}, Nrm);
				}
			}
			// A roll moulding round the door on the lining.
			const FOpening Roll = ArchedOpening(0.0, AP::RotundaDoorHalf + 0.09, 0.0, AP::RotundaDoorSpring, AP::RotundaDoorSpring + AP::RotundaDoorHalf + 0.09, 24);
			TArray<FVector> Path;
			for (int32 i = 0; i < Roll.Stations.Num(); ++i)
			{
				const double Z = Roll.Stations[i];
				Path.Add(FVector(-(i == Roll.Stations.Num() - 1 ? 0.0 : (i == 0 ? Roll.HalfAt(1e-6) : Roll.HalfAt(Z))), YL, Z));
			}
			for (int32 i = Roll.Stations.Num() - 2; i >= 0; --i)
			{
				const double Z = Roll.Stations[i];
				Path.Add(FVector(i == 0 ? Roll.HalfAt(1e-6) : Roll.HalfAt(Z), YL, Z));
			}
			FurnitureKit::FBarEnds Ends;
			Ends.bCap0 = Ends.bCap1 = true;
			FurnitureKit::Bar(P[SlotDressing], FurnitureKit::FPath(Path, N), [](double, double)
			{
				TArray<FVector2D> Ring;
				for (int32 i = 0; i <= 10; ++i)
				{
					const double T = kPi * i / 10.0;
					Ring.Add(FVector2D(0.07 * FMath::Cos(T), -0.01 + 0.06 * FMath::Sin(T)));
				}
				return Ring;
			}, Ends);
		}
		// The ceiling: oak boards between three moulded beams and wall plates (SlotCeiling).
		{
			FMeshData& C = P[SlotCeiling];
			const FVector Down(0, 0, -1);
			C.Rect(FVector(-H, YL, Top), FVector(H, YL, Top), FVector(H, YC, Top), FVector(-H, YC, Top), Down);
			for (const double Y : {12.60, 13.95, 15.30})
			{
				FurnitureKit::Bar(C, FurnitureKit::FPath::Line(FVector(-H - 0.02, Y, Top - 0.14), FVector(H + 0.02, Y, Top - 0.14), FVector(0, 0, 1)),
								  FurnitureKit::RectSection(0.0, 0.0, 0.10, 0.15, 0.012));
			}
			for (int32 Side = -1; Side <= 1; Side += 2)
			{
				FurnitureKit::Bar(C, FurnitureKit::FPath::Line(FVector(Side * (H - 0.07), YL, Top - 0.08), FVector(Side * (H - 0.07), YC, Top - 0.08), FVector(0, 0, 1)),
								  FurnitureKit::RectSection(0.0, 0.0, 0.07, 0.09, 0.01));
			}
		}
		// The porch's roof: a lead flat at 7.2 m inside a coped parapet on the side walls.
		{
			const double ZR = AP::PorchRoof - 0.25;
			P[SlotLead].Rect(FVector(-HO + 0.25, DrumY(HO) - 0.2, ZR), FVector(HO - 0.25, DrumY(HO) - 0.2, ZR), FVector(HO - 0.25, YC + 0.01, ZR),
							 FVector(-HO + 0.25, YC + 0.01, ZR), kUp);
			for (int32 Side = -1; Side <= 1; Side += 2)
			{
				const double XI = Side * (HO - 0.25);
				P[SlotExtBuff].Rect(FVector(XI, DrumY(HO) - 0.2, ZR), FVector(XI, YC, ZR), FVector(XI, YC, AP::PorchRoof - 0.1), FVector(XI, DrumY(HO) - 0.2, AP::PorchRoof - 0.1),
									FVector(-Side, 0, 0));
				// The coping.
				const double XA = Side * (HO + 0.05), XB = Side * (HO - 0.30);
				P[SlotExtDressing].Box(FVector(FMath::Min(XA, XB), DrumY(HO) - 0.25, AP::PorchRoof - 0.1), FVector(FMath::Max(XA, XB), YC + 0.02, AP::PorchRoof + 0.02),
									   FMeshData::AllFaces);
			}
			// Between the porch's flat and the ceiling: closed (the ceiling's back).
			P[SlotExtBuff].Rect(FVector(-H, YL, Top + 0.01), FVector(-H, YL, ZR), FVector(H, YL, ZR), FVector(H, YL, Top + 0.01), FVector(0, 1, 0));
		}
		// Floors: the Rotunda door's reveal in honed stone (from the sun clock's edge, vertex for vertex), the porch's field
		// of encaustic tiles inside a black border.
		{
			const double Step = 2.0 * kPi / AP::SunClockSides;
			const int32 South = AP::SunClockSides / 4;
			const int32 Half = FMath::CeilToInt32(FMath::Asin((AP::RotundaDoorHalf + 0.05) / AP::SunClockRadius) / Step);
			TArray<FVector2D> Reveal;
			for (int32 i = South + Half; i >= South - Half; --i)
			{
				Reveal.Add(FVector2D(AP::SunClockRadius * FMath::Cos(Step * i), AP::SunClockRadius * FMath::Sin(Step * i)));
			}
			Reveal.Add(FVector2D(Reveal.Last().X, YL + 0.001));
			Reveal.Add(FVector2D(Reveal[0].X, YL + 0.001));
			Planar(P[SlotThreshold], Reveal, FVector::ZeroVector, FVector(1, 0, 0), FVector(0, 1, 0), kUp);
			const double B = 0.30;
			P[SlotTile].Rect(FVector(-H + B, YL + B, 0), FVector(H - B, YL + B, 0), FVector(H - B, YC - B, 0), FVector(-H + B, YC - B, 0), kUp);
			auto Strip = [&](const FVector2D& A, const FVector2D& Bv, const FVector2D& Inward)
			{
				const FVector2D Along = (Bv - A).GetSafeNormal();
				const double L = FVector2D::Distance(A, Bv);
				const FVector2D A1 = A + Inward * B + Along * B, B1 = Bv + Inward * B - Along * B;
				FMeshData& M = P[SlotTileBorder];
				const int32 Base = M.Positions.Num();
				M.Vertex(FVector(A.X, A.Y, 0), kUp, FVector2D(0, 0));
				M.Vertex(FVector(Bv.X, Bv.Y, 0), kUp, FVector2D(L, 0));
				M.Vertex(FVector(B1.X, B1.Y, 0), kUp, FVector2D(L - B, B));
				M.Vertex(FVector(A1.X, A1.Y, 0), kUp, FVector2D(B, B));
				M.Quad(Base, Base + 1, Base + 2, Base + 3);
			};
			Strip(FVector2D(-H, YL + 0.001), FVector2D(H, YL + 0.001), FVector2D(0, 1));
			Strip(FVector2D(H, YL + 0.001), FVector2D(H, YC), FVector2D(-1, 0));
			Strip(FVector2D(H, YC), FVector2D(-H, YC), FVector2D(0, -1));
			Strip(FVector2D(-H, YC), FVector2D(-H, YL + 0.001), FVector2D(1, 0));
		}
	}
}

namespace AlbionBuild
{
	using namespace AlbionMasonryImpl;

	const TCHAR* DefaultMaterial(int32 Slot)
	{
		static const TCHAR* Names[SlotCount] = {
			TEXT("Albion/MI_Albion_Buff"), TEXT("Albion/MI_Albion_Red"), TEXT("Albion/MI_Albion_Slate"), TEXT("Albion/MI_Albion_Dressing"),
			TEXT("Albion/MI_Albion_Carved"), TEXT("Albion/MI_Albion_Ruskin"), TEXT("Albion/MI_Albion_Name"),
			TEXT("Albion/MI_Albion_Buff_Ext"), TEXT("Albion/MI_Albion_Red_Ext"), TEXT("Albion/MI_Albion_Dressing_Ext"),
			TEXT("Albion/MI_Albion_Encaustic"), TEXT("Albion/MI_Albion_YorkStone"), TEXT("Albion/MI_Albion_EncausticBorder"), TEXT("M_Travertine_Honed"),
			TEXT("Albion/MI_Albion_Iron"), TEXT("Albion/MI_Albion_Lead"), TEXT("M_Bronze_Brushed"),
			TEXT("Albion/MI_Albion_Glass"), TEXT("Albion/MI_Albion_Tristram"), TEXT("Albion/MI_Albion_LancetWest"), TEXT("Albion/MI_Albion_LancetEast"),
			TEXT("Albion/MI_Albion_Oak"), TEXT("Albion/MI_Albion_CaseGlass"), TEXT("Albion/MI_Albion_Oak"), TEXT("M_Felt"), TEXT("M_Felt"),
			TEXT("Albion/MI_Albion_Stone_Serpentine"), TEXT("Albion/MI_Albion_Stone_Peterhead"), TEXT("Albion/MI_Albion_Stone_Rubislaw"),
			TEXT("Albion/MI_Albion_Stone_Purbeck"), TEXT("Albion/MI_Albion_Stone_Frosterley"), TEXT("Albion/MI_Albion_Stone_HoptonWood"),
			TEXT("Albion/MI_Albion_Stone_Ashburton"), TEXT("Albion/MI_Albion_Stone_Iona"), TEXT("Albion/MI_Albion_Stone_Tiree"),
			TEXT("Albion/MI_Albion_Stone_Mona"),
		};
		return (Slot >= 0 && Slot < SlotCount) ? Names[Slot] : TEXT("");
	}

	const FStone& Stone(int32 Index)
	{
		static const FStone Stones[10] = {
			{TEXT("A"), TEXT("Lizard serpentine"), TEXT("Serpentine"), -AlbionPlan::ColumnX, 23.2, 0},
			{TEXT("B"), TEXT("Peterhead granite"), TEXT("Peterhead"), -AlbionPlan::ColumnX, 29.2, 1},
			{TEXT("C"), TEXT("Rubislaw granite"), TEXT("Rubislaw"), -AlbionPlan::ColumnX, 35.2, 2},
			{TEXT("D"), TEXT("Purbeck marble"), TEXT("Purbeck"), -AlbionPlan::ColumnX, 41.2, 3},
			{TEXT("E"), TEXT("Frosterley marble"), TEXT("Frosterley"), -AlbionPlan::ColumnX, 47.2, 4},
			{TEXT("F"), TEXT("Hopton Wood stone"), TEXT("HoptonWood"), AlbionPlan::ColumnX, 23.2, 5},
			{TEXT("G"), TEXT("Ashburton marble"), TEXT("Ashburton"), AlbionPlan::ColumnX, 29.2, 6},
			{TEXT("H"), TEXT("Iona marble"), TEXT("Iona"), AlbionPlan::ColumnX, 35.2, 7},
			{TEXT("I"), TEXT("Tiree marble"), TEXT("Tiree"), AlbionPlan::ColumnX, 41.2, 8},
			{TEXT("J"), TEXT("Mona marble"), TEXT("Mona"), AlbionPlan::ColumnX, 47.2, 9},
		};
		return Stones[FMath::Clamp(Index, 0, 9)];
	}

	AlbionKit::FOpening LancetOpening(double U, double Grow)
	{
		return AlbionKit::ArchedOpening(U, AlbionPlan::LancetHalf, AlbionPlan::LancetSill, AlbionPlan::LancetSpring(), AlbionPlan::LancetHead, 16, Grow);
	}

	AlbionKit::FOpening CourtDoorOpening(double Grow)
	{
		return AlbionKit::ArchedOpening(0.0, AlbionPlan::CourtDoorHalf, 0.0, AlbionPlan::CourtDoorSpring, AlbionPlan::CourtDoorCrown, 24, Grow);
	}

	AlbionKit::FArcHalf VaultHalf(int32 Index)
	{
		namespace AP = AlbionPlan;
		switch (Index)
		{
		case 0: return AlbionKit::PointedHalf(0.0, AP::NaveHalf, AP::Spring, AP::NaveCrown(), false);
		case 1: return AlbionKit::PointedHalf(0.0, AP::NaveHalf, AP::Spring, AP::NaveCrown(), true);
		case 2: return AlbionKit::PointedHalf(-AP::AisleCentreX, AP::AisleHalfSpan, AP::Spring, AP::AisleCrown(), false);
		case 3: return AlbionKit::PointedHalf(-AP::AisleCentreX, AP::AisleHalfSpan, AP::Spring, AP::AisleCrown(), true);
		case 4: return AlbionKit::PointedHalf(AP::AisleCentreX, AP::AisleHalfSpan, AP::Spring, AP::AisleCrown(), false);
		default: return AlbionKit::PointedHalf(AP::AisleCentreX, AP::AisleHalfSpan, AP::Spring, AP::AisleCrown(), true);
		}
	}

	void BuildMasonry(FParts& P)
	{
		SideWall(P, true);
		SideWall(P, false);
		EndWall(P, true);
		EndWall(P, false);
		Cornices(P);
	}

	void BuildFloors(FParts& P) { Floors(P); }
	void BuildPorch(FParts& P) { Porch(P); }
	void BuildExterior(FParts& P) { Exterior(P); }

	FParts BuildAll()
	{
		FParts P;
		BuildMasonry(P);
		BuildFloors(P);
		BuildPorch(P);
		BuildExterior(P);
		BuildColumns(P);
		BuildIron(P);
		BuildGlass(P);
		BuildFurniture(P);
		return P;
	}
}
