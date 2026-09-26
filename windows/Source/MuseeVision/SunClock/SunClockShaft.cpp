#include "SunClock/SunClockBuild.h"

/**
 * The shaft (fixed): the stair's wall (r 6.48) from the landing (−6) up to the drum's pocket; the
 * pocket (r 6.54–7.0, floor −3.58) that the drum's wall slides into; the masonry out to r 7.6, the
 * joint with the classical hall beyond; the port north (3.6 × 4.4 m at r 7.6, round-arched), whose
 * upper part the pocket passes over, so that inside r 7.05 it is closed by a tympanum over a lintel
 * (the portal onto the stair is 3.6 × 2.32 m); and the landing's floor, with a marble rosette under
 * the well: a sixteen-point star of nero and rosso halves on a nero core, in bands of nero and rosso.
 *
 * The port reads as the same round arch from both sides (PortDressings): on the stair's side an archivolt
 * of two orders springs from plinth blocks, a plain arch ring whose soffit carries the jambs' reveal round
 * the half circle (the hall's arch, crown −1.6) and a moulded architrave round it, with a keystone; the
 * lunette inside it (the stair's wall, the drum's pocket behind) sits over a moulded transom on the
 * lintel. On the hall's side the reveal's arch runs through to the tympanum, which stands on the same
 * transom. The lunette is solid because the drum's wall passes behind it when lowered.
 */
namespace SunClockBuild
{
	/** The port's columns on the circle R, in ascending angle (west jamb first): its samples J = N … 0. */
	static void PortColumns(TArray<FColumn>& Out, double R, const TArray<double>& Outside, TFunctionRef<TArray<double>(int32)> HoleSpan)
	{
		const double Lo = PortPhi(R, ArchSegments), Hi = PortPhi(R, 0), Margin = 0.15 * Deg;
		bool bDone = false;
		for (int32 I = 0; I < Around; ++I)
		{
			const double Phi = GridAngle(I);
			if (Phi >= Lo - Margin && Phi <= Hi + Margin)
			{
				if (!bDone)
				{
					bDone = true;
					// West jamb: the wall, then the port.
					Out.Add({Lo, {Outside}});
					for (int32 J = ArchSegments; J >= 0; --J) { Out.Add({PortPhi(R, J), {HoleSpan(J)}}); }
					Out.Add({Hi, {Outside}});
				}
				continue;
			}
			Out.Add({Phi, {Outside}});
		}
	}

	static void ShaftWalls(FSunClockMeshes& M)
	{
		FMeshData& S = M.Masonry;
		const double R0 = SC::StairWall, R1 = SC::PocketInner, R2 = SC::DialRadius, R3 = SC::ShaftOuter, RT = SC::PortTympanum;
		const double Top = SleeveTop;
		const TArray<double> Grid = GridAngles();

		// The stair's wall, cut by the portal (3.6 wide, up to the lintel).
		TArray<FColumn> Inner;
		PortColumns(Inner, R0, {SC::Floor, SC::PortLintel, Top}, [Top](int32) { return TArray<double>({SC::PortLintel, Top}); });
		CylinderColumns(S, R0, Inner, false, true);
		// Its top (the sleeve's), the sleeve's back, the pocket's floor and outer face.
		Annulus(S, R0, PortCircle(R0), R1, Grid, Top, true);
		TArray<FColumn> Sleeve, Pocket;
		for (const double Phi : Grid)
		{
			Sleeve.Add({Phi, {{SC::PocketFloor, Top}}});
			Pocket.Add({Phi, {{SC::PocketFloor, 0.0}}});
		}
		CylinderColumns(S, R1, Sleeve, true, true);
		Annulus(S, R1, Grid, R2, Grid, SC::PocketFloor, true);
		CylinderColumns(S, R2, Pocket, false, true);
		// The outer face (the joint), cut by the port's arch.
		TArray<FColumn> Outer;
		PortColumns(Outer, R3, {SC::Floor, SC::PortSpring, 0.0}, [](int32 J) { return TArray<double>({PortArchZ(J), 0.0}); });
		CylinderColumns(S, R3, Outer, true, true);

		// The port: jambs (the planes x = ±1.8), the lintel's underside, the tympanum, the arch's intrados.
		const double X = SC::PortWidth / 2;
		auto Y = [X](double R) { return -FMath::Sqrt(R * R - X * X); };
		const TArray<FVector2D> Jamb = {
			FVector2D(Y(R0), SC::Floor), FVector2D(Y(R3), SC::Floor), FVector2D(Y(R3), SC::PortSpring),
			FVector2D(Y(RT), SC::PortSpring), FVector2D(Y(RT), SC::PortLintel), FVector2D(Y(R0), SC::PortLintel)};
		for (const double Sign : {1.0, -1.0})
		{
			FlatPolygon(S, Jamb, [X, Sign](const FVector2D& Q) { return FVector(X * Sign, Q.X, Q.Y); }, FVector(-Sign, 0, 0));
		}
		auto OnCircle = [](int32 J, double R, double Z) { const double PX = PortX(J); return FVector(PX, -FMath::Sqrt(R * R - PX * PX), Z); };
		S.Patch(ArchSegments, 1, [&](int32 J, int32 K) { return OnCircle(J, K == 0 ? R0 : RT, SC::PortLintel); },
				[](const FVector& P) { return FVector2D(P.X, P.Y); }, [](const FVector&) { return -FVector::UpVector; });
		TArray<FColumn> Tympanum;
		for (int32 J = ArchSegments; J >= 0; --J) { Tympanum.Add({PortPhi(RT, J), {{SC::PortLintel, PortArchZ(J)}}}); }
		CylinderColumns(S, RT, Tympanum, true, false);
		S.Patch(ArchSegments, 1, [&](int32 J, int32 K) { return OnCircle(J, K == 0 ? RT : R3, PortArchZ(J)); },
				[](const FVector& P) { return FVector2D(P.X, P.Z); },
				[](const FVector& P) { return FVector(0, P.Y, SC::PortSpring) - P; });
		// The portal's floor, out to the joint.
		M.FloorA.Patch(ArchSegments, 1, [&](int32 J, int32 K) { return OnCircle(J, K == 0 ? R0 : R3, SC::Floor); },
					   [](const FVector& P) { return FVector2D(P.X, P.Y); }, [](const FVector&) { return FVector::UpVector; });
	}

	/**
	 * A face of the port on the circle R, as the port's dressings are cut on it: a point U along it (plan x), Z up and
	 * B proud of the face, towards the shaft's axis (Side +1, the stair's side) or away from it (Side −1, the hall's).
	 */
	struct FPortFace
	{
		double R = 0;
		double Side = 1;

		FVector At(double U, double Z, double B) const { return FVector(U, -FMath::Sqrt(R * R - U * U) + Side * B, Z); }
		/** dy/dU of the face. */
		double Slope(double U) const { return U / FMath::Sqrt(R * R - U * U); }
	};

	/** A station along a dressing's path on the face: its point, and the direction (in U, Z) away from the opening. */
	struct FPortStation
	{
		double U = 0, Z = 0, NU = 0, NZ = 0, S = 0;
	};

	/**
	 * A profile (A away from the opening in the face, B proud of it; SalonKit's winding: the solid on the left) swept
	 * along the stations, bent to the curved face, so its back stays on the face and every B is measured from it.
	 */
	static void FaceSweep(FMeshData& M, const FPortFace& F, const TArray<FPortStation>& Run, const FProfile& Profile)
	{
		const int32 NP = Profile.Points.Num();
		if (Run.Num() < 2 || NP < 2) { return; }
		const int32 NSeg = Profile.bClosed ? NP : NP - 1;
		TArray<FVector2D> SegN;
		TArray<double> Len = {0.0};
		for (int32 J = 0; J < NSeg; ++J)
		{
			const FVector2D D = Profile.Points[(J + 1) % NP].P - Profile.Points[J].P;
			SegN.Add(FVector2D(D.Y, -D.X).GetSafeNormal());
			Len.Add(Len.Last() + D.Size());
		}
		auto SegmentNormal = [&](int32 J, bool bEnd) -> FVector2D
		{
			const int32 Pi = bEnd ? (J + 1) % NP : J;
			if (!Profile.Points[Pi].bSmooth) { return SegN[J]; }
			int32 Other = bEnd ? J + 1 : J - 1;
			if (Other < 0 || Other >= NSeg)
			{
				if (!Profile.bClosed) { return SegN[J]; }
				Other = (Other + NSeg) % NSeg;
			}
			return (SegN[J] + SegN[Other]).GetSafeNormal();
		};
		auto Place = [&F](const FPortStation& St, const FVector2D& Q) { return F.At(St.U + Q.X * St.NU, St.Z + Q.X * St.NZ, Q.Y); };
		// The true normal of the bent surface: across the path's tangent and the profile's, on the face there.
		auto Normal = [&F](const FPortStation& St, const FVector2D& Q, const FVector2D& N2)
		{
			const double Sl = F.Slope(St.U + Q.X * St.NU);
			const FVector DA(St.NU, Sl * St.NU, St.NZ), DB(0, F.Side, 0), Along(-St.NZ, -Sl * St.NZ, St.NU);
			const FVector Approx = DA * N2.X + DB * N2.Y;
			FVector N = FVector::CrossProduct(Along, DA * -N2.Y + DB * N2.X).GetSafeNormal();
			if (FVector::DotProduct(N, Approx) < 0) { N = -N; }
			return N;
		};
		for (int32 J = 0; J < NSeg; ++J)
		{
			const FVector2D P0 = Profile.Points[J].P, P1 = Profile.Points[(J + 1) % NP].P;
			const FVector2D N0 = SegmentNormal(J, false), N1 = SegmentNormal(J, true);
			const int32 Base = M.Positions.Num();
			for (const FPortStation& St : Run)
			{
				M.Vertex(Place(St, P0), Normal(St, P0, N0), FVector2D(St.S, Len[J]));
				M.Vertex(Place(St, P1), Normal(St, P1, N1), FVector2D(St.S, Len[J + 1]));
			}
			for (int32 K = 0; K + 1 < Run.Num(); ++K)
			{
				const int32 A = Base + 2 * K;
				M.Quad(A, A + 2, A + 3, A + 1);
			}
		}
	}

	/** A block on the face: its corners C[0..7] as (U, Z, B), bottom four then top four, each face flat, facing out. */
	static void FaceBlock(FMeshData& M, const FPortFace& F, const FVector (&C)[8])
	{
		FVector P[8];
		FVector Mid = FVector::ZeroVector;
		for (int32 I = 0; I < 8; ++I)
		{
			P[I] = F.At(C[I].X, C[I].Y, C[I].Z);
			Mid += P[I] / 8.0;
		}
		static const int32 Faces[6][4] = {{0, 1, 2, 3}, {4, 5, 6, 7}, {0, 1, 5, 4}, {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}};
		for (const auto& Q : Faces)
		{
			const TArray<FVector> Pts = {P[Q[0]], P[Q[1]], P[Q[2]], P[Q[3]]};
			FVector N = FVector::CrossProduct(Pts[1] - Pts[0], Pts[2] - Pts[0]).GetSafeNormal();
			if (FVector::DotProduct(N, (Pts[0] + Pts[1] + Pts[2] + Pts[3]) / 4.0 - Mid) < 0) { N = -N; }
			M.Poly(Pts, N);
		}
	}

	/** A block with its U from U0 to U1, Z from Z0 to Z1, B from B0 to B1 (front and back cut in Steps along U, following the face). */
	static void FaceSlab(FMeshData& M, const FPortFace& F, double U0, double U1, double Z0, double Z1, double B0, double B1, int32 Steps)
	{
		for (int32 I = 0; I < Steps; ++I)
		{
			const double A = U0 + (U1 - U0) * I / Steps, B = U0 + (U1 - U0) * (I + 1) / Steps;
			const FVector C[8] = {FVector(A, Z0, B0), FVector(B, Z0, B0), FVector(B, Z0, B1), FVector(A, Z0, B1),
								  FVector(A, Z1, B0), FVector(B, Z1, B0), FVector(B, Z1, B1), FVector(A, Z1, B1)};
			FaceBlock(M, F, C);
		}
	}

	/** The transom's cornice: an ogee from its face (B Face) up and out to B Top, and its top back into the wall. */
	static FProfile TransomCornice(double Face, double Top)
	{
		FProfile P;
		P.Add(0.075, -0.01).Add(0.075, Top).Add(0.06, Top);
		// The cyma reversa, down and in to the transom's face.
		P.Arc(0.06, Top - 0.02, 0.02, 0.02, 90, 180, 4);
		P.SmoothLast();
		P.Arc(0.0, Top - 0.02, 0.04, Top - 0.02 - Face, 0, -90, 4);
		return P;
	}

	/**
	 * The port's dressings (the marble of the treads): on the stair's side an archivolt of two orders, plinth blocks, a
	 * keystone, and a transom on the lintel under the lunette; on the hall's side the same transom under the tympanum.
	 */
	static void PortDressings(FSunClockMeshes& M)
	{
		FMeshData& D = M.PortStone;
		const double X = SC::PortWidth / 2, Spring = SC::PortSpring, Crown = SC::PortSpring + X;
		constexpr double Plinth = 0.40;                      // the plinth blocks' height
		constexpr double RingFace = 0.14;                    // the plain arch ring (the first order): its face, proud
		constexpr double RingWidth = 0.18;
		constexpr double Outer = 0.40;                       // the architrave (the second order) out to here …
		constexpr double Crest = 0.20;                       // … its crown this proud
		constexpr double TransomFace = 0.10;                 // the transom stands back of the ring
		const FPortFace Stair{SC::StairWall, 1.0};
		const FPortFace Hall{SC::PortTympanum, -1.0};

		// The archivolt's path: up the east jamb from the plinth, round the half circle, down the west jamb.
		TArray<FPortStation> Run;
		double S = 0;
		auto Add = [&Run, &S](double U, double Z, double NU, double NZ)
		{
			if (Run.Num()) { S += FVector2D(U - Run.Last().U, Z - Run.Last().Z).Size(); }
			Run.Add({U, Z, NU, NZ, S});
		};
		const double Foot = SC::Floor + Plinth - 0.01;
		constexpr int32 JambSteps = 8;
		for (int32 I = 0; I < JambSteps; ++I) { Add(X, Foot + (Spring - Foot) * I / JambSteps, 1, 0); }
		for (int32 J = 0; J <= ArchSegments; ++J)
		{
			const double T = UE_DOUBLE_PI * J / ArchSegments;
			Add(X * FMath::Cos(T), Spring + X * FMath::Sin(T), FMath::Cos(T), FMath::Sin(T));
		}
		for (int32 I = 1; I <= JambSteps; ++I) { Add(-X, Spring + (Foot - Spring) * I / JambSteps, -1, 0); }

		// The two orders in one section (A away from the opening, B proud), traced from the architrave's back edge in to
		// the ring's soffit: the architrave's outer fillet, a cyma reversa (an ovolo under a cavetto) up to its crown,
		// the crown's fillet, a fascia, then the arch ring's face and its soffit (which carries the jambs' reveal).
		FProfile Arch;
		Arch.Add(Outer, -0.01).Add(Outer, 0.035).Add(Outer - 0.015, 0.035);
		Arch.Arc(Outer - 0.06, 0.035, 0.045, 0.06, 0, 90, 6);                     // to (0.34, 0.095), convex
		Arch.SmoothLast();
		Arch.Arc(Outer - 0.06, Crest, 0.04, Crest - 0.095, -90, -180, 6);        // to (0.30, Crest), concave
		Arch.Add(Outer - 0.14, Crest).Add(Outer - 0.14, Crest - 0.035);          // the crown's fillet
		Arch.Add(RingWidth + 0.01, Crest - 0.035).Add(RingWidth + 0.01, RingFace);   // the fascia (7 cm)
		Arch.Add(0.0, RingFace).Add(0.0, -0.01);                                  // the ring's face and soffit
		FaceSweep(D, Stair, Run, Arch);

		// The plinth blocks, under both orders, and a keystone at the crown.
		for (const double Sign : {1.0, -1.0})
		{
			const double U0 = Sign * X, U1 = Sign * (X + Outer + 0.03);
			FaceSlab(D, Stair, FMath::Min(U0, U1), FMath::Max(U0, U1), SC::Floor - 0.01, SC::Floor + Plinth, -0.02, Crest + 0.025, 3);
		}
		{
			const double Z0 = Crown - 0.015, Z1 = Crown + Outer + 0.08, W0 = 0.13, W1 = 0.18, B0 = -0.02, B1 = Crest + 0.03;
			const FVector C[8] = {FVector(-W0, Z0, B0), FVector(W0, Z0, B0), FVector(W0, Z0, B1), FVector(-W0, Z0, B1),
								  FVector(-W1, Z1, B0), FVector(W1, Z1, B0), FVector(W1, Z1, B1), FVector(-W1, Z1, B1)};
			FaceBlock(D, Stair, C);
		}

		// The transoms: a stone on the lintel (its underside the portal's head) from jamb to jamb, and its cornice.
		TArray<FPortStation> Across;
		constexpr int32 AcrossSteps = 36;
		for (int32 I = 0; I <= AcrossSteps; ++I)
		{
			const double U = -X + 2 * X * I / AcrossSteps;
			Across.Add({U, Spring, 0, 1, 2 * X * I / AcrossSteps});
		}
		for (const FPortFace* F : {&Stair, &Hall})
		{
			FaceSlab(D, *F, -X, X, SC::PortLintel, Spring, -0.01, TransomFace, AcrossSteps);
			FaceSweep(D, *F, Across, TransomCornice(TransomFace, RingFace));
		}
	}

	static void ShaftFloor(FSunClockMeshes& M)
	{
		const double Z = SC::Floor;
		const FVector Up = FVector::UpVector;
		const TArray<double> Grid = GridAngles();
		constexpr int32 Points = 16;
		constexpr double Core = 0.30, CoreEye = 0.12, Long = 1.80, Short = 1.20, Field = 1.90;
		// The core: a pale eye in a nero disc, its edge a 32-gon through the rays' roots and the valleys.
		TArray<double> Core32;
		for (int32 I = 0; I < 2 * Points; ++I) { Core32.Add(GridAngle(I * Around / (2 * Points))); }
		Annulus(M.FloorA, 0.0, Core32, CoreEye, Core32, Z, true);
		Annulus(M.FloorNero, CoreEye, Core32, Core, Core32, Z, true);
		auto At = [Z](double R, double Phi) { return Polar(R, Phi, Z); };
		const double Step = Turn / Points;
		for (int32 I = 0; I < Points; ++I)
		{
			const double Phi = Step * I;
			const double Tip = I % 2 == 0 ? Long : Short;
			const FVector T = At(Tip, Phi), C = At(Core, Phi), L = At(Core, Phi - Step / 2), R = At(Core, Phi + Step / 2);
			// Each point in two halves, dark and red (the short points the other way round).
			FMeshData& First = I % 2 == 0 ? M.FloorNero : M.FloorRosso;
			FMeshData& Second = I % 2 == 0 ? M.FloorRosso : M.FloorNero;
			First.Poly(TArray<FVector>({C, T, L}), Up);
			Second.Poly(TArray<FVector>({C, R, T}), Up);
			// The pale field between this point and the next, out to the nero band.
			const double Next = Step * (I + 1);
			const double NextTip = (I + 1) % 2 == 0 ? Long : Short;
			TArray<FVector2D> Outline;
			Outline.Add(FVector2D(T.X, T.Y));
			Outline.Add(FVector2D(R.X, R.Y));
			const FVector TN = At(NextTip, Next);
			Outline.Add(FVector2D(TN.X, TN.Y));
			const int32 G0 = I * Around / Points, G1 = (I + 1) * Around / Points;
			for (int32 G = G1; G >= G0; --G)
			{
				const FVector P = At(Field, GridAngle(G));
				Outline.Add(FVector2D(P.X, P.Y));
			}
			FlatPolygon(M.FloorA, Outline, [Z](const FVector2D& Q) { return FVector(Q.X, Q.Y, Z); }, Up);
		}
		// Bands of nero and rosso, then the pale landing out to the wall (with the portal's samples).
		Annulus(M.FloorNero, Field, Grid, 2.00, Grid, Z, true);
		Annulus(M.FloorRosso, 2.00, Grid, 2.06, Grid, Z, true);
		Annulus(M.FloorA, 2.06, Grid, SC::StairWall, PortCircle(SC::StairWall), Z, true);
	}

	void BuildShaft(FSunClockMeshes& M)
	{
		ShaftWalls(M);
		PortDressings(M);
		ShaftFloor(M);
	}
}
