#include "ClassicalHall/ClassicalHallGeometry.h"

#include "Plan/MuseePlan.h"

/**
 * The classical hall: design and construction.
 *
 * The Braccio Nuovo of the Vatican (Stern, 1817-22: a long top-lit gallery, a coffered barrel vault with skylights,
 * statue niches between columns of coloured marble, a hemicycle), the Sala delle Muse and the Sala Rotonda of the
 * Pio-Clementino (Simonetti: a ring of columns under a coffered dome; the porphyry basin on a mosaic floor), the
 * Pantheon's floor (porphyry and granite discs in squares of giallo antico), Soane's layered, top-lit low rooms, and
 * Klenze's Glyptothek (vaulted halls like Roman baths, coloured stucco behind white marble).
 *
 * The height is fixed (floor −6, nothing over −0.62 under the buildings above), so the rooms are proportioned to it: a
 * gallery 6 m wide under a segmental barrel (springing 3.70, crown 5.15; about as tall as it is wide), an Ionic order of
 * 0.34 m columns 9 diameters high (3.06 m) and an entablature of about a quarter of that (0.72 m, fasciae, frieze, dentils, corona, sima, and a lip that hides the cove lights). The
 * tribune's dome rises over a ring of columns (Ø 6.8 m), not over the whole room: a true dome at this height, as in the
 * Sala delle Muse, with the ambulatory's annular vault round it. Light is from the crowns: a laylight in each bay of the
 * gallery and the dome's eye, with concealed coves, wall-washers and accent spots (Lights()).
 *
 * Everything here is built in plan metres with heights above the hall's floor (h); Build() lifts it to FloorZ at the end.
 * Surfaces that meet share vertices: wall faces are cut into strips at stations (FWallFace), and every surface that meets
 * a wall (the floor, the vaults, the niches, the passage) is built from the same station lines, or zipped to them.
 * Mouldings, columns and fittings are closed solids that sink a centimetre into what carries them.
 */
namespace ClassicalHallBuild
{
	// Using-declarations, not a using-directive: in a unity build a directive would put the kit's names beside the
	// engine's (FFrame, …) in the global scope and make them ambiguous.
	using ClassicalHallKit::Pi;
	using ClassicalHallKit::Rad;
	using ClassicalHallKit::Cross2;
	using ClassicalHallKit::Unit;
	using ClassicalHallKit::FaceUV;
	using ClassicalHallKit::FMesh;
	using ClassicalHallKit::Triangulate;
	using ClassicalHallKit::Poly;
	using ClassicalHallKit::PolyWithHole;
	using ClassicalHallKit::Zip;
	using ClassicalHallKit::FProfile;
	using ClassicalHallKit::FSweepFrame;
	using ClassicalHallKit::Sweep;
	using ClassicalHallKit::Cap;
	using ClassicalHallKit::SweepSolid;
	using ClassicalHallKit::PlanRuns;
	using ClassicalHallKit::SweepPlan;
	using ClassicalHallKit::PlanOutline;
	using ClassicalHallKit::Lathe;
	using ClassicalHallKit::Box;
	using ClassicalHallKit::Patch;
	using ClassicalHallKit::Unique;
	using ClassicalHallKit::Interp;
	using ClassicalHallKit::FOpening;
	using ClassicalHallKit::FWallFace;
	using ClassicalHallGeometry::EPart;
	using ClassicalHallGeometry::FHall;
	namespace CH = MuseePlan::ClassicalHall;

	// ---------------------------------------------------------------------------------------------- dimensions

	constexpr double kHW = CH::GalleryHalfWidth;                 // 3.0
	constexpr double kYS = CH::GallerySouthY;                    // −8.2, the gallery's south wall
	constexpr double kYN = CH::GalleryNorthY;                    // −32.2, the screen
	constexpr double kSink = 0.01;

	// The order (Ionic): D 0.34, shaft tapering to 5/6 D, abacus top 1 cm into the architrave.
	constexpr double kD = CH::ColumnDiameter;
	constexpr double kRBot = kD * 0.5;
	constexpr double kRTop = kD * 0.5 * 5.0 / 6.0;
	constexpr double kArchitrave = CH::ColumnHeight;             // 3.06: the architrave's soffit
	constexpr double kAbacusTop = kArchitrave + kSink;
	constexpr double kLedge = CH::LedgeHeight;                   // 3.70: the cornice's ledge, where the vaults spring
	constexpr double kWallA0 = 0.07;                             // an architrave's face off its wall (the pilasters' face)
	constexpr double kPilasterHalf = 0.17;
	constexpr double kDadoTop = 0.45;                            // the dado's top and the niches' sills
	constexpr double kSoffit = 3.075;                            // the lintel's soffit between gallery and tribune

	// The gallery's segmental barrel vault: from the walls at the ledge to its crown.
	constexpr double kCrown = CH::VaultCrown;
	constexpr double kRise = kCrown - kLedge;
	constexpr double kGR = (kHW * kHW + kRise * kRise) / (2.0 * kRise);   // 3.828
	constexpr double kGZ = kCrown - kGR;                                  // the barrel's axis height
	constexpr double kModule = 0.6;                                       // coffers 0.6 m square
	constexpr double kLaylight = CH::LaylightHeight;                      // 5.38: the diffusers

	// The gallery's plan: four bays; column pairs at the bay lines, niches at the bays' middles.
	constexpr double kBay = CH::BayLength;
	constexpr int32 kBays = CH::GalleryBays;
	constexpr double kColumnAxis = kHW - CH::ColumnFromWall;              // 2.6
	constexpr double kPairHalf = CH::PairSpacing * 0.5;
	constexpr double kRessautA0 = CH::ColumnFromWall - kRTop;             // the ressaut's architrave face off the wall
	constexpr double kRessautHalf = 0.72;
	constexpr double kGalleryNicheR = 0.75, kGalleryNicheSpring = 2.19;   // crown 2.94
	constexpr double kCarpetHalf = 2.1, kCarpetSouth = -8.6, kCarpetNorth = -31.8;
	constexpr double kNicheBand = 0.1;                                     // the niches' architrave bands

	// The tribune.
	constexpr double kRo = CH::TribuneRadius;                    // 6.6: the outer wall
	constexpr double kTY = CH::TribuneCentreY;                   // −39.3
	constexpr double kRc = CH::RingRadius;                       // 3.4: the ring's column axes
	constexpr double kDomeSpring = kRc - 0.2;                    // 3.2: the dome springs from the ring beam's ledge
	constexpr double kAnnularSpring = kRc + 0.2;                 // 3.6: so does the annular vault
	constexpr double kOculus = CH::OculusRadius;
	constexpr double kTribuneNicheR = 0.9, kTribuneNicheSpring = 2.05;    // crown 2.95: the Apollo Belvedere (2.24 m) stands in one
	constexpr double kPattern = 6.3;                             // the floor's pattern disc

	// The joint with the shaft, and the masonry box.
	constexpr double kDoorHalf = CH::OpeningHalfWidth - CH::PassageInset;   // 1.79
	constexpr double kDoorSpring = CH::OpeningSpring;                       // 2.6
	constexpr double kThreshold = CH::PassageInset;                         // 0.01
	constexpr double kLip = CH::LipRadius;                                  // 7.55
	constexpr double kSleeveHalf = 2.1;
	constexpr double kShellSouth = -7.9;
	constexpr double kShellHalf = 4.2;
	constexpr double kShellRadius = 7.9;
	constexpr double kBottom = CH::ShellBottomZ - CH::FloorZ;              // −0.4
	constexpr double kRoof = CH::RoofZ - CH::FloorZ;                       // 5.45

	constexpr int32 kArchSegments = 48;      // round arches (half circle)
	constexpr int32 kRound = 240;            // stations round the tribune (1.5°)
	constexpr double kStep = 2.0 * Pi / kRound;

	inline FVector2D TC() { return FVector2D(0.0, kTY); }
	inline FVector TP(double Phi, double R, double H) { return FVector(R * FMath::Cos(Phi), kTY + R * FMath::Sin(Phi), H); }
	inline FVector Radial(double Phi) { return FVector(FMath::Cos(Phi), FMath::Sin(Phi), 0.0); }

	/** The gallery vault at arc S (from the crown, + east), plan Y and depth Dp into the vault. */
	inline FVector GV(double S, double Y, double Dp = 0.0)
	{
		const double A = S / kGR;
		return FVector((kGR + Dp) * FMath::Sin(A), Y, kGZ + (kGR + Dp) * FMath::Cos(A));
	}
	inline FVector GVN(double S) { const double A = S / kGR; return FVector(-FMath::Sin(A), 0.0, -FMath::Cos(A)); }
	inline double SpringS() { return kGR * FMath::Asin(kHW / kGR); }
	inline double ArcX(double S) { return kGR * FMath::Sin(S / kGR); }
	inline double ArcH(double S) { return kGZ + kGR * FMath::Cos(S / kGR); }
	inline double SOfX(double X) { return kGR * FMath::Asin(FMath::Clamp(X / kGR, -1.0, 1.0)); }

	/** Where the tribune's wall meets the gallery's side walls (x = ±3): the jamb corners. */
	inline double JambY() { return kTY + FMath::Sqrt(kRo * kRo - kHW * kHW); }
	inline double PhiE() { return FMath::Atan2(JambY() - kTY, kHW); }        // 62.97°
	inline double PhiW() { return Pi - PhiE(); }                              // 117.03°
	inline double LipY(double X) { return -FMath::Sqrt(kLip * kLip - X * X); }

	// ---------------------------------------------------------------------------------------------- lists

	/** The across samples of the gallery vault: springing, the springing band's middle, then every 0.2 m of the coffer rows. */
	TArray<double> VaultAcross()
	{
		const double SS = SpringS();
		TArray<double> Out = {-SS, -(SS + 3.0) * 0.5};
		for (int32 i = 0; i <= 30; ++i) { Out.Add(-3.0 + 0.2 * i); }
		Out.Append({(SS + 3.0) * 0.5, SS});
		return Out;
	}

	/** The across samples within the coffer rows (|s| ≤ 3). */
	TArray<double> VaultAcrossRows()
	{
		TArray<double> Out;
		for (int32 i = 0; i <= 30; ++i) { Out.Add(-3.0 + 0.2 * i); }
		return Out;
	}

	/** The vault's lines along the gallery, south to north: end band, 8 coffers, pair band, ... ending at the screen. */
	TArray<double> VaultLines()
	{
		TArray<double> Out = {kYS};
		for (int32 k = 0; k < kBays; ++k)
		{
			const double Y0 = kYS - 0.6 - kBay * k;
			for (int32 c = 0; c <= 8; ++c) { Out.Add(Y0 - kModule * c); }
		}
		Out.Add(kYN);
		Out.Sort();
		return Unique(Out);
	}

	inline double BayCentre(int32 k) { return kYS - kBay * 0.5 - kBay * k; }
	inline double PairCentre(int32 k) { return kYS - kBay * k; }   // k = 1 … 3

	/** The half circle of a round arch of radius R springing at H, from angle 0 (u = +R) to π, u ascending. */
	TArray<FVector2D> ArchHead(double UC, double R, double H, int32 Segments)
	{
		TArray<FVector2D> Out;
		for (int32 k = Segments; k >= 0; --k)
		{
			const double A = Pi * k / Segments;
			double U = UC + R * FMath::Cos(A);
			if (k == Segments) { U = UC - R; }
			if (k == 0) { U = UC + R; }
			Out.Add(FVector2D(U, H + R * FMath::Sin(A)));
		}
		return Out;
	}

	TArray<double> Us(const TArray<FVector2D>& Line)
	{
		TArray<double> Out;
		for (const FVector2D& P : Line) { Out.Add(P.X); }
		return Out;
	}

	/** Stations of a list within [Lo, Hi]. */
	TArray<double> Within(const TArray<double>& Stations, double Lo, double Hi)
	{
		TArray<double> Out;
		for (const double S : Stations)
		{
			if (S >= Lo - 1e-9 && S <= Hi + 1e-9) { Out.Add(S); }
		}
		return Out;
	}

	// ---------------------------------------------------------------------------------------------- niches

	/**
	 * A statue niche in a wall: half-round in plan, a quarter-sphere head. The wall is a plane, or the tribune's
	 * cylinder (its mouth is then the tangent plane at the niche's axis, joined to the wall by a thin reveal).
	 */
	struct FNiche
	{
		FVector Centre;          // on the mouth plane, on the axis, at h 0
		FVector In;              // horizontal, into the wall
		FVector Lateral;         // horizontal, along the wall (u ascending)
		double R = 0.75, Sill = 0.6, Spring = 2.1;
		double UC = 0.0;         // the axis's station (u)
		bool bCurved = false;    // on the tribune's cylinder (u is then arc length at kRo)
		double Phi = 0.0;        // the axis's plan angle (curved walls)

		/** The wall face's point at station u, height h. */
		FVector WallPoint(double U, double H) const
		{
			if (!bCurved) { return Centre + Lateral * (U - UC) + FVector(0, 0, H); }
			return TP(U / kRo, kRo, H);
		}
		/** The mouth plane's lateral coordinate of the wall point at station u. */
		double MouthT(double U) const
		{
			if (!bCurved) { return U - UC; }
			return kRo * FMath::Sin(U / kRo - Phi);
		}
		FVector MouthPoint(double U, double H) const { return Centre + Lateral * MouthT(U) + FVector(0, 0, H); }

		/** The opening in the wall face (u, h): jambs where the mouth's edges project onto the wall. */
		FOpening Opening() const
		{
			FOpening O;
			O.Sill = Sill;
			for (const FVector2D& P : ArchHead(0.0, R, Spring, kArchSegments))
			{
				const double U = bCurved ? kRo * (Phi + FMath::Asin(P.X / kRo)) : UC + P.X;
				O.Head.Add(FVector2D(U, P.Y));
			}
			O.U0 = O.Head[0].X;
			O.U1 = O.Head.Last().X;
			return O;
		}
	};

	/** The niche's inside (Niche material), its reveal on a curved wall, from the wall face's own station lines. */
	void BuildNiche(FHall& Hall, const FNiche& N, const FOpening& O, const TArray<double>& Stations)
	{
		FMesh& M = Hall[EPart::Niche];
		const TArray<double> In = Within(Stations, O.U0, O.U1);
		const FVector Up(0, 0, 1);

		// The outline's chains, u ascending: the sill, and the head.
		TArray<FVector> SillWall, SillMouth, HeadWall, HeadMouth;
		for (const double U : In)
		{
			SillWall.Add(N.WallPoint(U, O.Sill));
			SillMouth.Add(N.MouthPoint(U, O.Sill));
			HeadWall.Add(N.WallPoint(U, O.HeadAt(U)));
			HeadMouth.Add(N.MouthPoint(U, O.HeadAt(U)));
		}
		const FVector Axis = N.Centre + FVector(0, 0, N.Spring);

		// A curved wall: the reveal from the wall's outline back to the mouth.
		if (N.bCurved)
		{
			auto Band = [&M](const TArray<FVector>& A, const TArray<FVector>& B, TFunctionRef<FVector(const FVector&)> Nrm)
			{
				for (int32 i = 0; i + 1 < A.Num(); ++i)
				{
					const int32 a0 = M.Vertex(A[i], Nrm(A[i])), a1 = M.Vertex(A[i + 1], Nrm(A[i + 1]));
					const int32 b0 = M.Vertex(B[i], Nrm(B[i])), b1 = M.Vertex(B[i + 1], Nrm(B[i + 1]));
					M.Quad(a0, a1, b1, b0);
				}
			};
			Band(SillWall, SillMouth, [](const FVector&) { return FVector(0, 0, 1); });
			Band(HeadWall, HeadMouth, [&Axis, &N](const FVector& P)
			{
				FVector D = P - Axis;
				D -= N.In * FVector::DotProduct(D, N.In);
				return -Unit(D);
			});
			// The jambs' reveals (sill to spring), each a single quad.
			for (const int32 Side : {0, 1})
			{
				const double U = Side == 0 ? O.U0 : O.U1;
				const FVector Nrm = N.Lateral * (Side == 0 ? 1.0 : -1.0);
				const int32 a0 = M.Vertex(N.WallPoint(U, O.Sill), Nrm), a1 = M.Vertex(N.WallPoint(U, N.Spring), Nrm);
				const int32 b0 = M.Vertex(N.MouthPoint(U, O.Sill), Nrm), b1 = M.Vertex(N.MouthPoint(U, N.Spring), Nrm);
				M.Quad(a0, a1, b1, b0);
			}
		}

		// Samples round the half-round: β from 0 (u = +R side) round the back to π; the dome's rings match.
		constexpr int32 NB = 24;
		constexpr int32 MQ = NB / 2;
		auto Around = [&N](double Beta, double H) { return N.Centre + (N.Lateral * FMath::Cos(Beta) + N.In * FMath::Sin(Beta)) * N.R + FVector(0, 0, H); };
		auto AroundN = [&N](double Beta) { return -(N.Lateral * FMath::Cos(Beta) + N.In * FMath::Sin(Beta)); };

		// The floor: the mouth's sill chain and the half circle behind it.
		{
			TArray<FVector> Pts;
			for (int32 i = SillMouth.Num() - 1; i >= 0; --i) { Pts.Add(SillMouth[i]); }   // from +R to −R
			// SillMouth runs u ascending (t from −R to +R); reversed it runs from +R to −R, then round the back from π to 0.
			for (int32 j = NB - 1; j >= 1; --j) { Pts.Add(Around(Pi * j / NB, O.Sill)); }
			Poly(M, Pts, Up);
		}
		// The half-cylinder.
		for (int32 j = 0; j < NB; ++j)
		{
			const double B0 = Pi * j / NB, B1 = Pi * (j + 1) / NB;
			const int32 a0 = M.Vertex(Around(B0, O.Sill), AroundN(B0)), a1 = M.Vertex(Around(B1, O.Sill), AroundN(B1));
			const int32 b0 = M.Vertex(Around(B0, N.Spring), AroundN(B0)), b1 = M.Vertex(Around(B1, N.Spring), AroundN(B1));
			M.Quad(a0, a1, b1, b0);
		}
		// The quarter-sphere head: rows along the mouth's head chain, rings back to the pole.
		{
			const int32 K = HeadMouth.Num();
			TArray<double> Psi;
			for (const FVector& P : HeadMouth)
			{
				const FVector D = P - Axis;
				Psi.Add(FMath::Atan2(D.Z, FVector::DotProduct(D, N.Lateral)));
			}
			Psi[0] = Pi;
			Psi[K - 1] = 0.0;
			const int32 W = MQ + 1;
			const int32 Base = M.Positions.Num();
			for (int32 k = 0; k < K; ++k)
			{
				for (int32 j = 0; j <= MQ; ++j)
				{
					const double Ph = (Pi * 0.5) * j / MQ;
					const FVector Dir = (N.Lateral * FMath::Cos(Psi[k]) + FVector(0, 0, FMath::Sin(Psi[k]))) * FMath::Cos(Ph) + N.In * FMath::Sin(Ph);
					FVector P = Axis + Dir * N.R;
					if (j == 0) { P = HeadMouth[k]; }
					if (j == MQ) { P = Axis + N.In * N.R; }
					if (k == 0) { P = j == 0 ? HeadMouth[0] : Around(Pi - Ph, N.Spring); }
					if (k == K - 1) { P = j == 0 ? HeadMouth[K - 1] : Around(Ph, N.Spring); }
					M.Vertex(P, -Dir, FVector2D(N.R * Psi[k], N.R * Ph));
				}
			}
			for (int32 k = 0; k + 1 < K; ++k)
			{
				for (int32 j = 0; j < MQ; ++j) { M.Quad(Base + k * W + j, Base + (k + 1) * W + j, Base + (k + 1) * W + j + 1, Base + k * W + j + 1); }
			}
		}
	}

	// ---------------------------------------------------------------------------------------------- profiles

	/** The Ionic entablature (A out from the architrave's lowest fascia, B up from its soffit), back to BackA. */
	FProfile Entablature(double BackA)
	{
		FProfile P;
		P.Add(BackA, 0.0).Add(0.0, 0.0).Add(0.0, 0.055).Add(0.012, 0.055).Add(0.012, 0.115).Add(0.024, 0.115).Add(0.024, 0.185);
		P.Cyma(0.05, 0.215, 3, true);          // the architrave's crown: cyma reversa
		P.Add(0.05, 0.228).Add(0.012, 0.228);   // its fillet, back to the frieze
		P.Add(0.012, 0.418);                    // the frieze
		P.Cyma(0.04, 0.445, 3, true);           // bed moulding
		P.Add(0.045, 0.445).Add(0.045, 0.50);   // the dentil band (the dentils stand on it)
		P.Add(0.085, 0.50).Add(0.085, 0.515);   // fillet
		P.Round(0.125, 0.555, 4, true);         // ovolo
		P.Add(0.26, 0.555).Add(0.26, 0.62);     // the corona's soffit and face
		P.Add(0.272, 0.62).Add(0.272, 0.63);    // fillet
		P.Cyma(0.33, 0.705, 4, false);          // the sima: cyma recta
		P.Add(0.33, 0.72).Add(0.25, 0.72);      // the lip's top
		P.Add(0.25, kLedge - kArchitrave);      // the lip hides the cove light
		P.Add(BackA, kLedge - kArchitrave);     // the ledge
		return P;
	}

	/** The skirting at the wall's foot (A out of the wall, B up). */
	FProfile Skirting()
	{
		FProfile P;
		P.Add(-kSink, -2.5 * kSink).Add(0.025, -2.5 * kSink).Add(0.025, 0.10);
		P.Cyma(0.006, 0.135, 3, true);
		P.Add(0.006, 0.14).Add(-kSink, 0.14);
		return P;
	}

	/** The dado rail, its top 1 cm over the niches' sills. */
	FProfile DadoRail()
	{
		FProfile P;
		P.Add(-kSink, kDadoTop - 0.1).Add(0.008, kDadoTop - 0.1);
		P.Cyma(0.04, kDadoTop - 0.045, 3, false);
		P.Add(0.045, kDadoTop - 0.045).Add(0.045, kDadoTop - 0.015);
		P.Round(0.03, kDadoTop + kSink, 3, true);
		P.Add(-kSink, kDadoTop + kSink);
		return P;
	}

	/** The Attic base of a pilaster, as offsets out from the shaft's faces (A), from the floor (B). */
	FProfile PilasterBase()
	{
		FProfile P;
		P.Add(-kSink, -kSink).Add(0.065, -kSink).Add(0.065, 0.055).Add(0.045, 0.055);
		P.Round(0.075, 0.085, 3, true).Round(0.045, 0.115, 3, true);
		P.Add(0.035, 0.115).Add(0.035, 0.12);
		P.Round(0.02, 0.1375, 3, false).Round(0.035, 0.155, 3, false);
		P.Add(0.03, 0.155).Add(0.03, 0.16);
		P.Round(0.0475, 0.1775, 3, true).Round(0.03, 0.195, 3, true);
		P.Add(0.005, 0.195).Add(0.005, 0.2);
		P.Round(0.0, 0.215, 3, false);
		P.Add(-kSink, 0.215);
		return P;
	}

	/** An architrave band round an opening (A out from the opening's edge, in the wall's plane; B out of the wall). */
	FProfile Surround(double Width, double Proud)
	{
		FProfile P;
		P.Add(-0.015, -kSink).Add(Width, -kSink).Add(Width, Proud * 0.4);
		P.Cyma(Width - 0.025, Proud, 3, true);
		P.Add(0.012, Proud);
		P.Round(-0.015, Proud * 0.55, 3, true);
		return P;
	}

	/** The door's architrave: two fasciae and a cyma. */
	FProfile DoorArchitrave()
	{
		FProfile P;
		P.Add(-0.015, -kSink).Add(0.25, -kSink).Add(0.25, 0.07).Add(0.235, 0.07);
		P.Cyma(0.19, 0.042, 3, true);
		P.Add(0.08, 0.042).Add(0.08, 0.03).Add(-0.015, 0.03);
		return P;
	}

	/** A panel's frame: an ogee over the panel's edge (A out from the panel's edge, B out of the wall). */
	FProfile PanelFrame()
	{
		FProfile P;
		P.Add(-0.012, -kSink).Add(0.045, -kSink).Add(0.045, 0.012);
		P.Cyma(0.012, 0.035, 3, false);
		P.Add(-0.012, 0.035);
		return P;
	}

	/** A (radius, height) section from a wall moulding's (A, B): r = R0 + Sign * A, h = H0 + B, traced with the solid on the left. */
	FProfile ToLathe(const FProfile& In, double R0, double Sign, double H0)
	{
		FProfile Out;
		for (int32 i = 0; i < In.Num(); ++i)
		{
			const int32 k = Sign > 0 ? i : In.Num() - 1 - i;
			Out.Add(R0 + Sign * In.P[k].X, H0 + In.P[k].Y, In.Smooth[k]);
		}
		return Out;
	}

	/** Frames along a path in a vertical (or any) plane: section A along the path's in-plane left normal, B along N. */
	TArray<TArray<FSweepFrame>> PlaneRuns(const TArray<FVector2D>& Path, bool bClosed, const FVector& O, const FVector& T, const FVector& V,
									 const FVector& N, double HardDegrees = 20.0)
	{
		TArray<TArray<FSweepFrame>> Runs = PlanRuns(Path, bClosed, 0.0, HardDegrees);
		for (TArray<FSweepFrame>& Run : Runs)
		{
			for (FSweepFrame& F : Run)
			{
				F.Origin = O + T * F.Origin.X + V * F.Origin.Y;
				F.AxisA = T * F.AxisA.X + V * F.AxisA.Y;
				F.NormA = T * F.NormA.X + V * F.NormA.Y;
				F.AxisB = N;
				F.NormB = N;
			}
		}
		return Runs;
	}

	/** Sweeps a closed section along a planar path's runs and caps an open path's ends. */
	void SweepRuns(FMesh& M, const TArray<TArray<FSweepFrame>>& Runs, const FProfile& Profile, bool bClosedPath)
	{
		for (const TArray<FSweepFrame>& Run : Runs) { Sweep(M, Run, Profile, true); }
		if (!bClosedPath && Runs.Num() > 0)
		{
			const TArray<FSweepFrame>& First = Runs[0];
			const TArray<FSweepFrame>& Last = Runs.Last();
			Cap(M, First[0], Profile, Unit((First[0].Origin - First[1].Origin)));
			Cap(M, Last.Last(), Profile, Unit((Last.Last().Origin - Last[Last.Num() - 2].Origin)));
		}
	}

	// ---------------------------------------------------------------------------------------------- the order

	/** The column's turned section: Attic base, the shaft with entasis, astragal and echinus (to the axis at the top). */
	FProfile ColumnSection()
	{
		FProfile P;
		const double Plinth = 0.055;
		P.Add(0.0, Plinth - 0.005).Add(0.2, Plinth - 0.005).Add(0.2, Plinth);
		P.Round(0.2, 0.115, 5, true);                      // lower torus (bulging to 0.23)
		P.Add(0.19, 0.115).Add(0.19, 0.12);
		P.Round(0.19, 0.155, 5, false);                    // scotia
		P.Add(0.185, 0.155).Add(0.185, 0.16);
		P.Round(0.185, 0.195, 4, true);                    // upper torus
		P.Add(0.176, 0.195).Add(0.176, 0.20);
		P.Round(kRBot, 0.215, 3, false);                   // apophyge
		// The shaft: straight for a third, then the entasis to 5/6.
		const double ShaftTop = 2.865;
		const double H0 = 0.215, H1 = H0 + (ShaftTop - H0) / 3.0;
		for (int32 i = 1; i <= 10; ++i)
		{
			const double T = double(i) / 10.0;
			const double H = H1 + (ShaftTop - H1) * T;
			const double R = kRBot - (kRBot - kRTop) * (1.0 - FMath::Cos(T * Pi * 0.5));
			if (i == 1) { P.Add(kRBot, H1, true); }
			P.Add(R, H, i < 10);
		}
		P.Round(kRTop + 0.008, 2.875, 3, false);          // apophyge at the neck
		P.Add(kRTop + 0.008, 2.878);
		P.Round(kRTop + 0.008, 2.9, 4, true);             // astragal (a bead)
		P.Add(kRTop + 0.004, 2.9);
		P.Round(0.195, 2.965, 5, true);                    // echinus (ovolo)
		P.Add(0.0, 2.965);
		return P;
	}

	/** A spiral fillet on a volute's face: centre C, in the plane of Side and Up, its face towards Out. */
	void VoluteSpiral(FMesh& M, const FVector& C, const FVector& Side, const FVector& Out, double Turn, double R0)
	{
		const FVector Up(0, 0, 1);
		TArray<FVector2D> Path;
		const double R1 = R0 * 0.28;
		const double Turns = 2.5;
		const int32 N = 60;
		const double K = FMath::Loge(R0 / R1) / (Turns * 2.0 * Pi);
		for (int32 i = 0; i <= N; ++i)
		{
			const double Tt = Turns * 2.0 * Pi * i / N;
			const double R = R0 * FMath::Exp(-K * Tt);
			const double A = Pi * 0.5 - Turn * Tt;
			Path.Add(FVector2D(R * FMath::Cos(A), R * FMath::Sin(A)));
		}
		FProfile S;
		S.Add(-0.006, -0.004).Add(0.006, -0.004).Add(0.006, 0.012).Add(-0.006, 0.012);
		SweepRuns(M, PlaneRuns(Path, false, C, Side, Up, Out, 60.0), S, false);
		// The eye.
		FProfile E;
		E.Add(0.0, -0.004).Add(0.018, -0.004).Round(0.0, 0.014, 5, true);
		Lathe(M, C, Out, Up, E, 0.0, 2.0 * Pi, 16);
	}

	constexpr double kShaftWrap = 3.0;   // m of the stone image once round a shaft (materials.py: a multiple of its repeat)

	/** An Ionic column on the floor at Base (h 0), its volutes facing Front and back. */
	void Column(FHall& Hall, const FVector& Base, const FVector& FrontIn, EPart ShaftPart)
	{
		FMesh& C = Hall[EPart::Carved];
		const FVector Front = Unit(FVector(FrontIn.X, FrontIn.Y, 0.0));
		const FVector Up(0, 0, 1);
		const FVector Side = FVector::CrossProduct(Up, Front);
		// The square plinth.
		Box(C, Base + FVector(0, 0, (0.055 - kSink) * 0.5), Front, Side, Up, FVector(0.235, 0.235, (0.055 + kSink) * 0.5));
		// One turned solid: the base and echinus in marble, the shaft's segments in coloured stone.
		const FProfile Full = ColumnSection();
		auto IsShaft = [&Full](int32 Seg)
		{
			const FVector2D A = Full.P[Seg], B = Full.P[Seg + 1];
			return A.Y >= 0.215 - 1e-9 && B.Y <= 2.865 + 1e-9 && FMath::Min(A.X, B.X) > kRTop - 1e-6;
		};
		// The shaft's stone wraps 3 m once round (giallo repeats at 1.5 m, pavonazzetto at 1.5 m: no seam).
		Lathe(C, Base, Up, Front, Full, 0.0, 2.0 * Pi, 40, false, false, &Hall[ShaftPart], IsShaft, kShaftWrap);

		// The capital: canalis, bolsters with volutes on both faces, abacus.
		const double EyeH = 2.945;
		const double VX = 0.155, VR = 0.075, VD = 0.19;
		Box(C, Base + FVector(0, 0, (EyeH + 3.04) * 0.5), Side, Front, Up, FVector(VX, VD - 0.02, (3.04 - EyeH) * 0.5));
		Box(C, Base + FVector(0, 0, (3.005 + 3.05) * 0.5), Side, Front, Up, FVector(VX + 0.08, VD - 0.03, 0.0225));
		FProfile Bolster;
		Bolster.Add(0.0, -VD).Add(VR, -VD).Add(VR, -VD + 0.02);
		Bolster.Add(0.056, -0.07, true).Add(0.056, -0.035).Add(0.064, -0.03).Add(0.064, 0.03).Add(0.056, 0.035).Add(0.056, 0.07, true);
		Bolster.Add(VR, VD - 0.02).Add(VR, VD).Add(0.0, VD);
		for (const double Sgn : {-1.0, 1.0})
		{
			const FVector VC = Base + Side * (Sgn * VX) + FVector(0, 0, EyeH);
			Lathe(C, VC, Front, Up, Bolster, 0.0, 2.0 * Pi, 24);
			for (const double Face : {-1.0, 1.0})
			{
				// Each volute winds from the top outwards and down (clockwise on the right as seen from its face).
				const FVector Out = Front * Face;
				const FVector FaceSide = FVector::CrossProduct(Up, Out);   // the viewer's right on that face
				const double Turn = FVector::DotProduct(Side * Sgn, FaceSide) > 0 ? 1.0 : -1.0;
				VoluteSpiral(C, VC + Out * VD, FaceSide, Out, Turn, VR - 0.006);
			}
		}
		// The abacus: square, a small ovolo on its edge.
		const double AH = 0.035, AHalf = 0.185;
		TArray<FVector2D> Sq;
		for (const FVector2D& Q : {FVector2D(-1, -1), FVector2D(1, -1), FVector2D(1, 1), FVector2D(-1, 1)})
		{
			const FVector P = Base + Front * (Q.X * AHalf) + Side * (Q.Y * AHalf);
			Sq.Add(FVector2D(P.X, P.Y));
		}
		FProfile Ab;
		Ab.Add(0.0, 0.0).Add(0.0, 0.012).Round(0.015, 0.027, 3, true).Add(0.015, AH);
		// The loop must run clockwise in (x, y) so that the section's A (the path's left) points outwards.
		double Area = 0.0;
		for (int32 i = 0; i < 4; ++i) { Area += Cross2(Sq[i], Sq[(i + 1) % 4]); }
		if (Area > 0) { Sq = {Sq[3], Sq[2], Sq[1], Sq[0]}; }
		const double Z0 = kAbacusTop - AH;
		SweepPlan(C, Sq, true, Ab, false, Z0, 20.0);
		Poly(C, PlanOutline(Sq, true, FVector2D(0.0, 0.0), Z0), -Up);
		Poly(C, PlanOutline(Sq, true, FVector2D(0.015, AH), Z0), Up);
	}

	/**
	 * An Ionic pilaster against a flat wall: the wall point W on the floor, Out into the room, Along the wall. Its base and
	 * capital run round its three faces, their ends in the wall.
	 */
	void Pilaster(FHall& Hall, const FVector& W, const FVector& OutIn, EPart ShaftPart)
	{
		FMesh& C = Hall[EPart::Carved];
		const FVector Up(0, 0, 1);
		const FVector Out = Unit(FVector(OutIn.X, OutIn.Y, 0));
		const FVector Along = FVector::CrossProduct(Out, Up);   // so that Out, Along, Up trace the faces anticlockwise
		const double F = kWallA0;                               // the shaft's face off the wall
		auto P2 = [&W, &Out, &Along](double A, double L) { const FVector P = W + Out * A + Along * L; return FVector2D(P.X, P.Y); };
		// The shaft.
		Box(C.Positions.Num() >= 0 ? Hall[ShaftPart] : C, W + Out * ((F - 0.02) * 0.5) + FVector(0, 0, (0.2 + 2.9) * 0.5), Out, Along, Up,
			FVector((F + 0.02) * 0.5, kPilasterHalf, (2.9 - 0.2) * 0.5));
		// The three faces' path, from inside the wall round and back in: left side, front, right side.
		auto Faces = [&P2](double Face, double Half)
		{
			TArray<FVector2D> Path = {P2(-0.03, -Half), P2(Face, -Half), P2(Face, Half), P2(-0.03, Half)};
			return Path;
		};
		// The path must keep the pilaster on its right (A outwards): check and flip.
		auto Oriented = [](TArray<FVector2D> Path, const FVector2D& Inside)
		{
			const FVector2D D = Path[1] - Path[0];
			const FVector2D Left(-D.Y, D.X);
			if (FVector2D::DotProduct(Left, Inside - Path[0]) > 0) { Path = {Path[3], Path[2], Path[1], Path[0]}; }
			return Path;
		};
		const FVector2D Inside = P2(F * 0.5, 0.0);
		SweepPlan(C, Oriented(Faces(F, kPilasterHalf), Inside), false, PilasterBase(), true, 0.0, 20.0);
		// The capital: astragal and ovolo bands round the faces, the canalis, flat volutes, the abacus.
		FProfile Neck;
		Neck.Add(-kSink, 2.865).Add(0.004, 2.865).Round(0.004, 2.89, 4, true).Round(0.03, 2.945, 4, true).Add(-kSink, 2.945);
		SweepPlan(C, Oriented(Faces(F, kPilasterHalf), Inside), false, Neck, true, 0.0, 20.0);
		const double EyeH = 2.945, VX = 0.155, VR = 0.075;
		const double Front = F + 0.03;
		Box(C, W + Out * ((Front - 0.035) * 0.5) + FVector(0, 0, (EyeH + 3.04) * 0.5), Out, Along, Up, FVector((Front + 0.035) * 0.5, VX, (3.04 - EyeH) * 0.5));
		Box(C, W + Out * ((Front - 0.01 - 0.045) * 0.5) + FVector(0, 0, (3.005 + 3.05) * 0.5), Out, Along, Up, FVector((Front - 0.01 + 0.045) * 0.5, VX + 0.08, 0.0225));
		for (const double Sgn : {-1.0, 1.0})
		{
			const FVector VC = W + Along * (Sgn * VX) + FVector(0, 0, EyeH);
			FProfile Disc;
			Disc.Add(0.0, -0.06).Add(VR, -0.06).Add(VR, Front + 0.01).Add(0.0, Front + 0.01);
			Lathe(C, VC, Out, Up, Disc, 0.0, 2.0 * Pi, 24);
			const FVector FaceSide = FVector::CrossProduct(Up, Out);
			const double Turn = FVector::DotProduct(Along * Sgn, FaceSide) > 0 ? 1.0 : -1.0;
			VoluteSpiral(C, VC + Out * (Front + 0.01), FaceSide, Out, Turn, VR - 0.006);
		}
		FProfile Ab;
		Ab.Add(-kSink, 3.035).Add(0.0, 3.035).Add(0.0, 3.047).Round(0.015, 3.062, 3, true).Add(0.015, kAbacusTop).Add(-kSink, kAbacusTop);
		SweepPlan(C, Oriented(Faces(Front + 0.01, 0.2), Inside), false, Ab, true, 0.0, 20.0);
	}

	/** Dentils along a straight run of the entablature: From → To along the wall line, Out into the room at the band. */
	void Dentils(FMesh& M, const FVector& From, const FVector& To, const FVector& Out, double BandA, double Margin)
	{
		const FVector D = To - From;
		const double L = D.Size();
		const FVector Dir = D / FMath::Max(L, 1e-9);
		const double Pitch = 0.052, Width = 0.034;
		const int32 N = FMath::FloorToInt((L - 2.0 * Margin + (Pitch - Width)) / Pitch);
		if (N <= 0) { return; }
		const double Start = (L - (N * Pitch - (Pitch - Width))) * 0.5 + Width * 0.5;
		const FVector Up(0, 0, 1);
		for (int32 i = 0; i < N; ++i)
		{
			const FVector C = From + Dir * (Start + Pitch * i) + Out * (BandA + 0.01) + Up * (kArchitrave + (0.456 + 0.504) * 0.5);
			Box(M, C, Dir, Out, Up, FVector(Width * 0.5, 0.02, (0.504 - 0.456) * 0.5));
		}
	}

	/** Dentils round a circle (the tribune's entablatures), facing outwards (Sign +1) or in (−1) from radius R. */
	void DentilRing(FMesh& M, double R, double Sign)
	{
		const double Pitch = 0.052, Width = 0.034;
		const int32 N = FMath::FloorToInt(2.0 * Pi * R / Pitch);
		const FVector Up(0, 0, 1);
		for (int32 i = 0; i < N; ++i)
		{
			const double Phi = 2.0 * Pi * (i + 0.5) / N;
			const FVector Out = Radial(Phi) * Sign;
			const FVector Along = FVector::CrossProduct(Up, Out);
			const FVector C = TP(Phi, R, 0) + Out * 0.01 + Up * (kArchitrave + (0.456 + 0.504) * 0.5);
			Box(M, C, Along, Out, Up, FVector(Width * 0.5, 0.02, (0.504 - 0.456) * 0.5));
		}
	}

	// ---------------------------------------------------------------------------------------------- coffers

	/** A curved surface for coffering: Map(s, t, depth into the vault); RoomN(s, t) faces the room. */
	struct FSurface
	{
		TFunction<FVector(double, double, double)> Map;
		TFunction<FVector(double, double)> RoomN;
	};

	/** A plain patch of a surface, S × T samples (the samples must match the neighbours'). */
	void PlainPatch(FMesh& M, const FSurface& Sf, const TArray<double>& S, const TArray<double>& T)
	{
		const int32 Base = M.Positions.Num();
		for (const double s : S)
		{
			for (const double t : T) { M.Vertex(Sf.Map(s, t, 0.0), Sf.RoomN(s, t), FVector2D(s, t)); }
		}
		const int32 W = T.Num();
		for (int32 i = 0; i + 1 < S.Num(); ++i)
		{
			for (int32 j = 0; j + 1 < T.Num(); ++j) { M.Quad(Base + i * W + j, Base + (i + 1) * W + j, Base + (i + 1) * W + j + 1, Base + i * W + j + 1); }
		}
	}

	/** A rectangle's outline in (s, t), NS × NT samples per side, anticlockwise from (S0, T0); inset in metres. */
	struct FRing
	{
		TArray<FVector2D> P;    // (s, t)
		double Depth = 0.0;
	};

	FRing RectRing(double S0, double S1, double T0, double T1, int32 NS, int32 NT, double Depth)
	{
		FRing R;
		R.Depth = Depth;
		for (int32 i = 0; i < NS; ++i) { R.P.Add(FVector2D(FMath::Lerp(S0, S1, double(i) / NS), T0)); }
		for (int32 i = 0; i < NT; ++i) { R.P.Add(FVector2D(S1, FMath::Lerp(T0, T1, double(i) / NT))); }
		for (int32 i = 0; i < NS; ++i) { R.P.Add(FVector2D(FMath::Lerp(S1, S0, double(i) / NS), T1)); }
		for (int32 i = 0; i < NT; ++i) { R.P.Add(FVector2D(S0, FMath::Lerp(T1, T0, double(i) / NT))); }
		return R;
	}

	/** The band between two rings of the same sampling (a coffer's frame, riser or tread), facing FaceN(s, t, side). */
	void RingBand(FMesh& M, const FSurface& Sf, const FRing& A, const FRing& B, int32 NS, int32 NT, bool bRiser, const FVector2D& Centre)
	{
		const int32 N = A.P.Num();
		for (int32 i = 0; i < N; ++i)
		{
			const int32 i1 = (i + 1) % N;
			// Which side of the rectangle this segment is on: 0 bottom (t0), 1 right (s1), 2 top (t1), 3 left (s0).
			const int32 SideIdx = i < NS ? 0 : (i < NS + NT ? 1 : (i < 2 * NS + NT ? 2 : 3));
			auto Nrm = [&](const FVector2D& Q) -> FVector
			{
				if (!bRiser) { return Sf.RoomN(Q.X, Q.Y); }
				const double E = 1e-4;
				const bool bAlongS = SideIdx == 1 || SideIdx == 3;
				FVector D = bAlongS ? (Sf.Map(Q.X + E, Q.Y, 0) - Sf.Map(Q.X - E, Q.Y, 0)) : (Sf.Map(Q.X, Q.Y + E, 0) - Sf.Map(Q.X, Q.Y - E, 0));
				D = Unit(D);
				const FVector ToC = Sf.Map(Centre.X, Centre.Y, 0) - Sf.Map(Q.X, Q.Y, 0);
				return FVector::DotProduct(D, ToC) >= 0 ? D : -D;
			};
			const int32 a0 = M.Vertex(Sf.Map(A.P[i].X, A.P[i].Y, A.Depth), Nrm(A.P[i]), A.P[i]);
			const int32 a1 = M.Vertex(Sf.Map(A.P[i1].X, A.P[i1].Y, A.Depth), Nrm(A.P[i1]), A.P[i1]);
			const int32 b0 = M.Vertex(Sf.Map(B.P[i].X, B.P[i].Y, B.Depth), Nrm(B.P[i]), B.P[i] + FVector2D(0.01, 0.01));
			const int32 b1 = M.Vertex(Sf.Map(B.P[i1].X, B.P[i1].Y, B.Depth), Nrm(B.P[i1]), B.P[i1] + FVector2D(0.01, 0.01));
			M.Quad(a0, a1, b1, b0);
		}
	}

	/** A rosette (a gilt patera with eight petals) at P on a coffer's back, facing N, of radius R. */
	void Rosette(FMesh& M, const FVector& P, const FVector& N, double R)
	{
		const FVector Ax = Unit(N);
		const FVector X = Unit((FMath::Abs(Ax.Z) < 0.9 ? FVector::CrossProduct(Ax, FVector::UpVector) : FVector::CrossProduct(Ax, FVector::ForwardVector)));
		const FVector Y = FVector::CrossProduct(Ax, X);
		// (radius, height) of the section, and whether the petals swell it.
		const double Sec[][3] = {{0.0, -0.004, 0}, {1.0, -0.004, 1}, {1.0, 0.004, 1}, {0.86, 0.014, 1}, {0.62, 0.02, 1},
								 {0.36, 0.023, 0}, {0.2, 0.032, 0}, {0.0, 0.036, 0}};
		const int32 NSec = 8, NA = 16;
		auto Pt = [&](int32 i, int32 j)
		{
			const double A = 2.0 * Pi * j / NA;
			const double Swell = Sec[i][2] > 0 ? 0.84 + 0.16 * FMath::Abs(FMath::Cos(4.0 * A)) : 1.0;
			const double Rr = Sec[i][0] * R * Swell;
			return P + (X * FMath::Cos(A) + Y * FMath::Sin(A)) * Rr + Ax * (Sec[i][1] * R / 0.08);
		};
		// Inside the rosette's body, so its underside faces the back and its top the room.
		const FVector Inner = P + Ax * (0.006 * R / 0.08);
		auto Toward = [&Inner](const FVector& Q) { return Q - Inner; };
		auto UV = [](int32 i, int32 j) { return FVector2D(i * 0.01, j * 0.01); };
		// The flat underside and the rest, split at the rim's crease.
		Patch(M, 1, NA, Pt, UV, Toward);
		Patch(M, NSec - 2, NA, [&Pt](int32 i, int32 j) { return Pt(i + 1, j); }, UV, Toward);
	}

	/** One stepped coffer in the cell [S0, S1] × [T0, T1] (param units; KS, KT metres per unit at the cell). */
	void Coffer(FHall& Hall, const FSurface& Sf, double S0, double S1, double T0, double T1, int32 NS, int32 NT, double KS, double KT,
				double Rib, double Scale)
	{
		FMesh& M = Hall[EPart::Vault];
		const FVector2D Centre((S0 + S1) * 0.5, (T0 + T1) * 0.5);
		// Rings: the cell's edge, the frame's inner edge, then two steps, then the back.
		const double RibS = Rib / KS, RibT = Rib / KT;
		const double Step = 0.035 * Scale;
		const double D1 = CH::CofferDepth / 3.0;
		TArray<FRing> Rings;
		Rings.Add(RectRing(S0, S1, T0, T1, NS, NT, 0.0));
		Rings.Add(RectRing(S0 + RibS, S1 - RibS, T0 + RibT, T1 - RibT, NS, NT, 0.0));
		Rings.Add(RectRing(S0 + RibS, S1 - RibS, T0 + RibT, T1 - RibT, NS, NT, D1));
		Rings.Add(RectRing(S0 + RibS + Step / KS, S1 - RibS - Step / KS, T0 + RibT + Step / KT, T1 - RibT - Step / KT, NS, NT, D1));
		Rings.Add(RectRing(S0 + RibS + Step / KS, S1 - RibS - Step / KS, T0 + RibT + Step / KT, T1 - RibT - Step / KT, NS, NT, 2 * D1));
		Rings.Add(RectRing(S0 + RibS + 2 * Step / KS, S1 - RibS - 2 * Step / KS, T0 + RibT + 2 * Step / KT, T1 - RibT - 2 * Step / KT, NS, NT, 2 * D1));
		Rings.Add(RectRing(S0 + RibS + 2 * Step / KS, S1 - RibS - 2 * Step / KS, T0 + RibT + 2 * Step / KT, T1 - RibT - 2 * Step / KT, NS, NT, 3 * D1));
		for (int32 r = 0; r + 1 < Rings.Num(); ++r)
		{
			const bool bRiser = FMath::Abs(Rings[r].Depth - Rings[r + 1].Depth) > 1e-9;
			RingBand(M, Sf, Rings[r], Rings[r + 1], NS, NT, bRiser, Centre);
		}
		// The back.
		const double BS0 = S0 + RibS + 2 * Step / KS, BS1 = S1 - RibS - 2 * Step / KS, BT0 = T0 + RibT + 2 * Step / KT, BT1 = T1 - RibT - 2 * Step / KT;
		const int32 Base = M.Positions.Num();
		for (int32 i = 0; i <= NS; ++i)
		{
			for (int32 j = 0; j <= NT; ++j)
			{
				const double s = FMath::Lerp(BS0, BS1, double(i) / NS), t = FMath::Lerp(BT0, BT1, double(j) / NT);
				M.Vertex(Sf.Map(s, t, 3 * D1), Sf.RoomN(s, t), FVector2D(s, t));
			}
		}
		for (int32 i = 0; i < NS; ++i)
		{
			for (int32 j = 0; j < NT; ++j) { M.Quad(Base + i * (NT + 1) + j, Base + (i + 1) * (NT + 1) + j, Base + (i + 1) * (NT + 1) + j + 1, Base + i * (NT + 1) + j + 1); }
		}
		const double BackSize = FMath::Min((BS1 - BS0) * KS, (BT1 - BT0) * KT);
		Rosette(Hall[EPart::Gilt], Sf.Map(Centre.X, Centre.Y, 3 * D1), Sf.RoomN(Centre.X, Centre.Y), BackSize * 0.3);
	}

	// ---------------------------------------------------------------------------------------------- the gallery

	struct FGalleryWalls
	{
		TArray<double> Long;         // stations of the long walls (u = y, ascending)
		TArray<double> South;        // stations of the south wall (u = x, ascending)
		TArray<double> North;        // stations of the lintel's face over the screen
		FWallFace SouthFace;
	};

	TArray<FNiche> GalleryNiches(double Side)
	{
		TArray<FNiche> Out;
		for (int32 k = 0; k < kBays; ++k)
		{
			FNiche N;
			N.UC = BayCentre(k);
			N.Centre = FVector(Side * kHW, N.UC, 0.0);
			N.In = FVector(Side, 0, 0);
			N.Lateral = FVector(0, 1, 0);
			N.R = kGalleryNicheR;
			N.Sill = kDadoTop;
			N.Spring = kGalleryNicheSpring;
			Out.Add(N);
		}
		return Out;
	}

	TArray<double> LongStations()
	{
		TArray<double> S = VaultLines();
		S.Append({JambY(), kYN, kCarpetSouth, kCarpetNorth, kYS});
		for (const FNiche& N : GalleryNiches(1.0)) { S.Append(Us(N.Opening().Head)); }
		return Unique(S, 1e-7);
	}

	/** The long walls: x = ±3 from the south wall to the tribune (the jambs under the lintel north of the screen). */
	void LongWall(FHall& Hall, double Side, const TArray<double>& Stations)
	{
		FWallFace W;
		W.Place = [Side](double U, double H) { return FVector(Side * kHW, U, H); };
		W.Normal = [Side](double, double) { return FVector(-Side, 0, 0); };
		W.Bottom = [](double, double) { return 0.0; };
		W.Top = [](double, double Mid) { return Mid > kYN ? kLedge : kSoffit; };
		W.Stations = Stations;
		W.SplitH = kDadoTop;
		const TArray<FNiche> Niches = GalleryNiches(Side);
		for (const FNiche& N : Niches) { W.Openings.Add(N.Opening()); }
		W.Build(Hall[EPart::Dado], Hall[EPart::Wall]);
		for (int32 k = 0; k < Niches.Num(); ++k) { BuildNiche(Hall, Niches[k], W.Openings[k], Stations); }
	}

	TArray<FVector2D> EndArc()
	{
		TArray<FVector2D> Out;
		for (const double S : VaultAcross()) { Out.Add(FVector2D(ArcX(S), ArcH(S))); }
		Out[0].X = -kHW;
		Out.Last().X = kHW;
		return Out;
	}

	TArray<FVector2D> DoorHead() { return ArchHead(0.0, kDoorHalf, kDoorSpring, kArchSegments); }
	TArray<FVector2D> SleeveHead() { return ArchHead(0.0, kSleeveHalf, kDoorSpring, kArchSegments); }

	TArray<double> SouthStations()
	{
		TArray<double> S = Us(EndArc());
		S.Append(Us(DoorHead()));
		S.Append({-kCarpetHalf, kCarpetHalf});
		return Unique(S, 1e-7);
	}

	TArray<double> LipStations()
	{
		TArray<double> S = Us(DoorHead());
		S.Append(Us(SleeveHead()));
		return Unique(S, 1e-7);
	}

	/** The gallery's south wall with the door from the shaft. */
	FWallFace SouthWallFace()
	{
		FWallFace W;
		W.Place = [](double U, double H) { return FVector(U, kYS, H); };
		W.Normal = [](double, double) { return FVector(0, -1, 0); };
		W.Bottom = [](double, double) { return 0.0; };
		const TArray<FVector2D> Arc = EndArc();
		W.Top = [Arc](double U, double) { return Interp(Arc, U); };
		W.Stations = SouthStations();
		W.SplitH = kDadoTop;
		FOpening Door;
		Door.U0 = -kDoorHalf;
		Door.U1 = kDoorHalf;
		Door.Sill = 0.0;
		Door.Head = DoorHead();
		W.Openings.Add(Door);
		W.Extra = [](double U) { return FMath::Abs(FMath::Abs(U) - kDoorHalf) < 1e-9 ? TArray<double>({kThreshold}) : TArray<double>(); };
		return W;
	}

	FWallFace LipFace()
	{
		FWallFace W;
		W.Place = [](double U, double H) { return FVector(U, LipY(U), H); };
		W.Normal = [](double U, double) { return Unit(FVector(-U, -LipY(U), 0)); };
		W.Bottom = [](double, double) { return kBottom; };
		const TArray<FVector2D> Sleeve = SleeveHead();
		W.Top = [Sleeve](double U, double) { return Interp(Sleeve, U); };
		W.Stations = LipStations();
		FOpening Door;
		Door.U0 = -kDoorHalf;
		Door.U1 = kDoorHalf;
		Door.Sill = kThreshold;
		Door.Head = DoorHead();
		W.Openings.Add(Door);
		return W;
	}

	/** The chain of a face's outline points for an opening: its sill (u ascending) and head (u ascending). */
	struct FOutline
	{
		TArray<FVector> Sill, Head;
		TArray<double> SillU, HeadU;
		TArray<FVector> JambLo, JambHi;   // U0 and U1 lines between sill and spring (levels)
		TArray<double> JambLoH, JambHiH;
	};

	FOutline Outline(const FWallFace& W, const FOpening& O, double SillH)
	{
		FOutline Out;
		for (const double U : Within(W.Stations, O.U0, O.U1))
		{
			Out.Sill.Add(W.Place(U, SillH));
			Out.SillU.Add(U);
			Out.Head.Add(W.Place(U, O.HeadAt(U)));
			Out.HeadU.Add(U);
		}
		return Out;
	}

	/** The passage from the lip (r 7.55, in the shaft's wall) to the gallery: threshold, riser, jambs and arch. */
	void Passage(FHall& Hall, const FWallFace& Lip, const FWallFace& South)
	{
		const FOpening& LO = Lip.Openings[0];
		const FOpening& SO = South.Openings[0];
		const TArray<double> LipIn = Within(Lip.Stations, LO.U0, LO.U1), SouthIn = Within(South.Stations, SO.U0, SO.U1);
		// The threshold (porphyry), from the lip's sill to the riser's top.
		{
			FMesh& M = Hall[EPart::FloorPorphyry];
			TArray<int32> A, B;
			TArray<double> KA, KB;
			for (const double U : LipIn) { A.Add(M.Vertex(Lip.Place(U, kThreshold), FVector(0, 0, 1))); KA.Add(U); }
			for (const double U : SouthIn) { B.Add(M.Vertex(South.Place(U, kThreshold), FVector(0, 0, 1))); KB.Add(U); }
			Zip(M, A, KA, B, KB);
			// The riser into the gallery (1 cm).
			for (int32 i = 0; i + 1 < SouthIn.Num(); ++i)
			{
				const FVector Nrm(0, -1, 0);
				const int32 a0 = M.Vertex(South.Place(SouthIn[i], 0.0), Nrm), a1 = M.Vertex(South.Place(SouthIn[i + 1], 0.0), Nrm);
				const int32 b0 = M.Vertex(South.Place(SouthIn[i], kThreshold), Nrm), b1 = M.Vertex(South.Place(SouthIn[i + 1], kThreshold), Nrm);
				M.Quad(a0, a1, b1, b0);
			}
		}
		// The jambs and the arch (the walls' marble).
		FMesh& M = Hall[EPart::Wall];
		for (const double Side : {-1.0, 1.0})
		{
			const double U = Side * kDoorHalf;
			TArray<int32> A, B;
			TArray<double> KA, KB;
			const FVector Nrm(-Side, 0, 0);
			const int32 LI = Lip.Stations.IndexOfByPredicate([U](double S) { return FMath::Abs(S - U) < 1e-9; });
			const int32 SI = South.Stations.IndexOfByPredicate([U](double S) { return FMath::Abs(S - U) < 1e-9; });
			for (const double H : Lip.Levels[LI])
			{
				if (H >= kThreshold - 1e-9 && H <= kDoorSpring + 1e-9) { A.Add(M.Vertex(Lip.Place(U, H), Nrm)); KA.Add(H); }
			}
			for (const double H : South.Levels[SI])
			{
				if (H >= kThreshold - 1e-9 && H <= kDoorSpring + 1e-9) { B.Add(M.Vertex(South.Place(U, H), Nrm)); KB.Add(H); }
			}
			Zip(M, A, KA, B, KB);
		}
		{
			TArray<int32> A, B;
			TArray<double> KA, KB;
			auto ArchN = [](const FVector& P) { return -Unit(FVector(P.X, 0, P.Z - kDoorSpring)); };
			for (const double U : LipIn) { const FVector P = Lip.Place(U, LO.HeadAt(U)); A.Add(M.Vertex(P, ArchN(P))); KA.Add(U); }
			for (const double U : SouthIn) { const FVector P = South.Place(U, SO.HeadAt(U)); B.Add(M.Vertex(P, ArchN(P))); KB.Add(U); }
			Zip(M, A, KA, B, KB);
		}
	}

	/** The masonry round the passage outside the gallery: the sleeve from the shell's south face to the lip. */
	void Sleeve(FHall& Hall, const FWallFace& Lip, const FWallFace& ShellSouth)
	{
		FMesh& M = Hall[EPart::Shell];
		const FOpening& LO = ShellSouth.Openings[0];
		for (const double Side : {-1.0, 1.0})
		{
			const double U = Side * kSleeveHalf;
			const FVector Nrm(Side, 0, 0);
			const int32 a0 = M.Vertex(Lip.Place(U, kBottom), Nrm), a1 = M.Vertex(Lip.Place(U, kDoorSpring), Nrm);
			const int32 b0 = M.Vertex(ShellSouth.Place(U, kBottom), Nrm), b1 = M.Vertex(ShellSouth.Place(U, kDoorSpring), Nrm);
			M.Quad(a0, a1, b1, b0);
		}
		TArray<int32> A, B;
		TArray<double> KA, KB;
		auto ArchN = [](const FVector& P) { return Unit(FVector(P.X, 0, P.Z - kDoorSpring)); };
		for (const double U : Within(Lip.Stations, -kSleeveHalf, kSleeveHalf)) { const FVector P = Lip.Place(U, Lip.Top(U, U)); A.Add(M.Vertex(P, ArchN(P))); KA.Add(U); }
		for (const double U : Within(ShellSouth.Stations, LO.U0, LO.U1)) { const FVector P = ShellSouth.Place(U, LO.HeadAt(U)); B.Add(M.Vertex(P, ArchN(P))); KB.Add(U); }
		Zip(M, A, KA, B, KB);
	}

	// ---------------------------------------------------------------------------------------------- the tribune's wall

	TArray<FNiche> TribuneNiches()
	{
		TArray<FNiche> Out;
		for (const double Deg : {150.0, 210.0, 270.0, 330.0, 390.0})
		{
			FNiche N;
			N.bCurved = true;
			N.Phi = Rad(Deg);
			N.UC = kRo * N.Phi;
			N.Centre = TP(N.Phi, kRo, 0.0);
			N.In = Radial(N.Phi);
			N.Lateral = FVector(-FMath::Sin(N.Phi), FMath::Cos(N.Phi), 0.0);
			N.R = kTribuneNicheR;
			N.Sill = kDadoTop;
			N.Spring = kTribuneNicheSpring;
			Out.Add(N);
		}
		return Out;
	}

	/** The tribune wall's stations as angles, from the east jamb round by the north to the east jamb again. */
	TArray<double> TribuneAngles()
	{
		TArray<double> A = {PhiE(), PhiW(), PhiE() + 2.0 * Pi};
		for (int32 k = 0; k <= 2 * kRound; ++k)
		{
			const double P = k * kStep;
			if (P > PhiE() + 1e-7 && P < PhiE() + 2.0 * Pi - 1e-7) { A.Add(P); }
		}
		for (const FNiche& N : TribuneNiches())
		{
			for (const double U : Us(N.Opening().Head)) { A.Add(U / kRo); }
		}
		return Unique(A, 1e-9);
	}

	FWallFace TribuneWall()
	{
		FWallFace W;
		W.Place = [](double U, double H) { return TP(U / kRo, kRo, H); };
		W.Normal = [](double U, double) { return -Radial(U / kRo); };
		const double Lo = PhiE() * kRo, Hi = PhiW() * kRo;
		W.Bottom = [Lo, Hi](double, double Mid) { return (Mid > Lo && Mid < Hi) ? kSoffit : 0.0; };
		W.Top = [](double, double) { return kLedge; };
		for (const double A : TribuneAngles()) { W.Stations.Add(A * kRo); }
		W.SplitH = kDadoTop;
		for (const FNiche& N : TribuneNiches()) { W.Openings.Add(N.Opening()); }
		const double J0 = PhiE() * kRo, J1 = PhiW() * kRo, J2 = (PhiE() + 2.0 * Pi) * kRo;
		W.Extra = [J0, J1, J2](double U)
		{
			if (FMath::Abs(U - J0) < 1e-7 || FMath::Abs(U - J1) < 1e-7 || FMath::Abs(U - J2) < 1e-7) { return TArray<double>({0.0, kDadoTop, kSoffit, kLedge}); }
			return TArray<double>();
		};
		return W;
	}

	// ---------------------------------------------------------------------------------------------- vaults

	FSurface GallerySurface()
	{
		FSurface S;
		S.Map = [](double s, double t, double d) { return GV(s, t, d); };
		S.RoomN = [](double s, double) { return GVN(s); };
		return S;
	}

	/** Zips a chain along the gallery (y ascending) at across S1 to the vault lines at S2 (both on the vault surface). */
	void VaultStrip(FMesh& M, const TArray<double>& YA, double SA, const TArray<double>& YB, double SB)
	{
		TArray<int32> A, B;
		TArray<double> KA, KB;
		for (const double Y : YA) { A.Add(M.Vertex(GV(SA, Y), GVN(SA), FVector2D(SA, Y))); KA.Add(Y); }
		for (const double Y : YB) { B.Add(M.Vertex(GV(SB, Y), GVN(SB), FVector2D(SB, Y))); KB.Add(Y); }
		Zip(M, A, KA, B, KB);
	}

	/** A band across the vault, zipped between two arcs (y fixed) with their own samples. */
	void VaultEndStrip(FMesh& M, const TArray<double>& SA, double YA, const TArray<double>& SB, double YB)
	{
		TArray<int32> A, B;
		TArray<double> KA, KB;
		for (const double S : SA) { A.Add(M.Vertex(GV(S, YA), GVN(S), FVector2D(S, YA))); KA.Add(S); }
		for (const double S : SB) { B.Add(M.Vertex(GV(S, YB), GVN(S), FVector2D(S, YB))); KB.Add(S); }
		Zip(M, A, KA, B, KB);
	}

	/** A lantern in the crown: 4 × 4 coffers' room, a stepped well up to the laylight's diffuser. */
	void GalleryLantern(FHall& Hall, double YC)
	{
		FMesh& M = Hall[EPart::Vault];
		const double S0 = -1.2, S1 = 1.2, Y0 = YC - 1.2, Y1 = YC + 1.2, W = 0.06;
		const TArray<double> Across = Within(VaultAcrossRows(), S0, S1);
		TArray<double> Along = {Y0, YC - 0.6, YC, YC + 0.6, Y1};
		TArray<double> InnerAcross = {S0 + W};
		for (const double S : Across) { if (S > S0 + W + 1e-9 && S < S1 - W - 1e-9) { InnerAcross.Add(S); } }
		InnerAcross.Add(S1 - W);
		TArray<double> InnerAlong = {Y0 + W, YC - 0.6, YC, YC + 0.6, Y1 - W};
		// The frame round the opening: four zipped strips.
		VaultEndStrip(M, Across, Y0, InnerAcross, Y0 + W);
		VaultEndStrip(M, Across, Y1, InnerAcross, Y1 - W);
		VaultStrip(M, Along, S0, InnerAlong, S0 + W);
		VaultStrip(M, Along, S1, InnerAlong, S1 - W);
		// The well: vertical sides up to the diffuser.
		auto Wall = [&M](const FVector& A, const FVector& B, const FVector& Nrm)
		{
			const FVector A1(A.X, A.Y, kLaylight), B1(B.X, B.Y, kLaylight);
			const int32 a0 = M.Vertex(A, Nrm), a1 = M.Vertex(B, Nrm), b1 = M.Vertex(B1, Nrm), b0 = M.Vertex(A1, Nrm);
			M.Quad(a0, a1, b1, b0);
		};
		TArray<FVector> Loop;   // the opening's edge, round
		for (const double S : InnerAcross) { Loop.Add(GV(S, Y0 + W)); }
		for (int32 i = 1; i < InnerAlong.Num(); ++i) { Loop.Add(GV(S1 - W, InnerAlong[i])); }
		for (int32 i = InnerAcross.Num() - 2; i >= 0; --i) { Loop.Add(GV(InnerAcross[i], Y1 - W)); }
		for (int32 i = InnerAlong.Num() - 2; i >= 1; --i) { Loop.Add(GV(S0 + W, InnerAlong[i])); }
		const FVector Mid = GV(0.0, YC);
		for (int32 i = 0; i < Loop.Num(); ++i)
		{
			const FVector& A = Loop[i];
			const FVector& B = Loop[(i + 1) % Loop.Num()];
			FVector Nrm = FVector(Mid.X - (A.X + B.X) * 0.5, Mid.Y - (A.Y + B.Y) * 0.5, 0.0);
			// Straight sides face straight across.
			if (FMath::Abs(A.X - B.X) < 1e-9) { Nrm = FVector(Nrm.X, 0, 0); }
			else { Nrm = FVector(0, Nrm.Y, 0); }
			Wall(A, B, Unit(Nrm));
		}
		// The diffuser.
		TArray<FVector> Top;
		for (const FVector& P : Loop) { Top.Add(FVector(P.X, P.Y, kLaylight)); }
		Poly(Hall[EPart::Laylight], Top, FVector(0, 0, -1));
	}

	void GalleryVault(FHall& Hall, const TArray<double>& LongSt, const TArray<double>& SouthSt, const TArray<double>& NorthSt)
	{
		FMesh& M = Hall[EPart::Vault];
		const FSurface Sf = GallerySurface();
		const double SS = SpringS(), SM = (SS + 3.0) * 0.5;
		const TArray<double> Lines = VaultLines();
		const TArray<double> Wall = Within(LongSt, kYN, kYS);
		const TArray<double> Rows = VaultAcrossRows();

		// The springing bands along both walls.
		for (const double Sgn : {-1.0, 1.0})
		{
			VaultStrip(M, Wall, Sgn * SS, Lines, Sgn * SM);
			VaultStrip(M, Lines, Sgn * SM, Lines, Sgn * 3.0);
		}
		// The end bands, zipped to the end walls' tops.
		const TArray<FVector2D> Arc = EndArc();
		auto EndBand = [&](double YWall, double YIn, const TArray<double>& WallSt)
		{
			// The wall's own top points (on the arc's chords between its samples), zipped to the first coffers' line.
			TArray<int32> A, B;
			TArray<double> KA, KB;
			const double XMax = ArcX(3.0) + 1e-9;
			for (const double X : WallSt)
			{
				if (FMath::Abs(X) > XMax) { continue; }
				const double S = SOfX(X);
				A.Add(M.Vertex(FVector(X, YWall, Interp(Arc, X)), GVN(S), FVector2D(S, YWall)));
				KA.Add(S);
			}
			for (const double S : Rows) { B.Add(M.Vertex(GV(S, YIn), GVN(S), FVector2D(S, YIn))); KB.Add(S); }
			Zip(M, A, KA, B, KB);
		};
		EndBand(kYS, kYS - 0.6, SouthSt);
		EndBand(kYN, kYN + 0.6, NorthSt);
		// Pair bands.
		for (int32 k = 1; k < kBays; ++k)
		{
			const double YP = PairCentre(k);
			PlainPatch(M, Sf, Rows, {YP - 0.6, YP + 0.6});
		}
		// The coffers and the lanterns.
		for (int32 k = 0; k < kBays; ++k)
		{
			const double YC = BayCentre(k);
			for (int32 r = 0; r < 10; ++r)
			{
				for (int32 c = 0; c < 8; ++c)
				{
					const bool bLantern = r >= 3 && r <= 6 && c >= 2 && c <= 5;
					if (bLantern) { continue; }
					const double S0 = -3.0 + kModule * r, S1 = S0 + kModule;
					const double T0 = YC - 2.4 + kModule * c, T1 = T0 + kModule;
					Coffer(Hall, Sf, S0, S1, T0, T1, 3, 1, 1.0, 1.0, 0.06, 1.0);
				}
			}
			GalleryLantern(Hall, YC);
		}
	}

	/** The annular vault over the ambulatory: section angle γ from the outer wall (0) over to the ring beam. */
	struct FAnnular
	{
		double RM = 0.0, HM = 0.0, RA = 0.0, G0 = 0.0, G1 = 0.0;
		FAnnular()
		{
			RM = (kRo + kAnnularSpring) * 0.5;
			const double Half = (kRo - kAnnularSpring) * 0.5;
			RA = (Half * Half + kRise * kRise) / (2.0 * kRise);
			HM = kCrown - RA;
			G0 = FMath::Asin((kLedge - HM) / RA);
			G1 = Pi - G0;
		}
		FVector At(double Phi, double G, double Dp) const
		{
			const double R = RM + (RA + Dp) * FMath::Cos(G);
			return TP(Phi, R, HM + (RA + Dp) * FMath::Sin(G));
		}
		FVector N(double Phi, double G) const { return -(Radial(Phi) * FMath::Cos(G) + FVector(0, 0, FMath::Sin(G))); }
	};

	void AnnularVault(FHall& Hall, const TArray<double>& WallAngles)
	{
		FMesh& M = Hall[EPart::Vault];
		const FAnnular AV;
		FSurface Sf;
		Sf.Map = [AV](double Phi, double G, double Dp) { return AV.At(Phi, G, Dp); };
		Sf.RoomN = [AV](double Phi, double G) { return AV.N(Phi, G); };
		const double Band = 0.3 / AV.RA;
		const double CG0 = AV.G0 + Band, CG1 = AV.G1 - Band;
		const int32 Rows = 7, Cells = 60;
		// The outer band: zipped from the wall's stations to the uniform ring.
		TArray<double> Uniform;
		const double Start = PhiE();
		const int32 First = FMath::CeilToInt(Start / kStep - 1e-9);
		for (int32 k = First; k <= First + kRound; ++k) { Uniform.Add(k * kStep); }
		{
			TArray<int32> A, B;
			TArray<double> KA, KB;
			const double GMid = AV.G0 + Band * 0.5;
			for (const double P : WallAngles) { A.Add(M.Vertex(AV.At(P, AV.G0, 0), AV.N(P, AV.G0), FVector2D(P * kRo, 0))); KA.Add(P); }
			for (const double P : Uniform) { B.Add(M.Vertex(AV.At(P, GMid, 0), AV.N(P, GMid), FVector2D(P * kRo, 0.15))); KB.Add(P); }
			Zip(M, A, KA, B, KB);
		}
		TArray<double> Round;
		for (int32 k = 0; k <= kRound; ++k) { Round.Add(k * kStep); }
		PlainPatch(M, Sf, Round, {AV.G0 + Band * 0.5, CG0});
		PlainPatch(M, Sf, Round, {CG1, AV.G1});
		// The coffers: 60 round (centred on the axes of the niches and the intercolumns), 7 across.
		const double DG = (CG1 - CG0) / Rows;
		for (int32 c = 0; c < Cells; ++c)
		{
			const double P0 = Rad(15.0 + 6.0 * c), P1 = P0 + Rad(6.0);
			for (int32 r = 0; r < Rows; ++r)
			{
				const double G0 = CG0 + DG * r, G1 = G0 + DG;
				const double GM = (G0 + G1) * 0.5;
				const double KS = AV.RM + AV.RA * FMath::Cos(GM);
				Coffer(Hall, Sf, P0, P1, G0, G1, 4, 3, KS, AV.RA, 0.05, 0.85);
			}
		}
	}

	/** The dome over the ring: a spherical cap from the ring beam's ledge (r 3.2) to the eye (r 1.2). */
	struct FDome
	{
		double RS = 0.0, ZC = 0.0, BS = 0.0, BO = 0.0;
		FDome()
		{
			const double A2 = kOculus * kOculus, B2 = kDomeSpring * kDomeSpring, Dh = kRise;
			const double Q = (B2 - A2 - Dh * Dh) / (2.0 * Dh);   // √(R² − b²)
			RS = FMath::Sqrt(B2 + Q * Q);
			ZC = kLedge - Q;
			BS = FMath::Asin(kDomeSpring / RS);
			BO = FMath::Asin(kOculus / RS);
		}
		FVector At(double Phi, double B, double Dp) const { return TP(Phi, (RS + Dp) * FMath::Sin(B), ZC + (RS + Dp) * FMath::Cos(B)); }
		FVector N(double Phi, double B) const { return -(Radial(Phi) * FMath::Sin(B) + FVector(0, 0, FMath::Cos(B))); }
	};

	void Dome(FHall& Hall)
	{
		FMesh& M = Hall[EPart::Vault];
		const FDome DM;
		FSurface Sf;
		Sf.Map = [DM](double Phi, double B, double Dp) { return DM.At(Phi, B, Dp); };
		Sf.RoomN = [DM](double Phi, double B) { return DM.N(Phi, B); };
		TArray<double> Round;
		for (int32 k = 0; k <= kRound; ++k) { Round.Add(k * kStep); }
		const double Bottom = 0.25 / DM.RS, TopBand = 0.2 / DM.RS;
		const double C0 = DM.BS - Bottom, C1 = DM.BO + TopBand;
		// Rings of coffers, each about as tall as it is wide.
		TArray<double> Edges = {C0};
		{
			TArray<double> Try = {C0};
			double B = C0;
			while (B > C1)
			{
				const double Width = 2.0 * Pi * DM.RS * FMath::Sin(B) / 24.0;
				B -= 0.9 * Width / DM.RS;
				Try.Add(B);
			}
			const int32 Rings = FMath::Max(1, Try.Num() - 2);
			// Rescale the rings' heights (same proportions) to end exactly at C1.
			TArray<double> H;
			double Sum = 0.0;
			for (int32 i = 0; i < Rings; ++i) { H.Add(Try[i] - Try[i + 1]); Sum += H.Last(); }
			for (int32 i = 0; i < Rings; ++i) { Edges.Add(Edges.Last() - H[i] * (C0 - C1) / Sum); }
			Edges.Last() = C1;
		}
		PlainPatch(M, Sf, Round, {DM.BS, (DM.BS + C0) * 0.5, C0});
		for (int32 c = 0; c < 24; ++c)
		{
			const double P0 = Rad(7.5 + 15.0 * c), P1 = P0 + Rad(15.0);
			for (int32 r = 0; r + 1 < Edges.Num(); ++r)
			{
				// Param t runs from the top edge down, so that t ascends with s's orientation kept.
				const double B0 = Edges[r + 1], B1 = Edges[r];
				const double KS = DM.RS * FMath::Sin((B0 + B1) * 0.5);
				Coffer(Hall, Sf, P0, P1, B0, B1, 10, 3, KS, DM.RS, 0.05, 0.8 + 0.2 * (Edges.Num() - 2 - r) / FMath::Max(1, Edges.Num() - 2));
			}
		}
		PlainPatch(M, Sf, Round, {DM.BO, C1});
		// The eye: a short well up to the laylight, a gilt bead round its edge.
		const double H0 = DM.ZC + DM.RS * FMath::Cos(DM.BO);
		for (int32 k = 0; k < kRound; ++k)
		{
			const double P0 = Round[k], P1 = Round[k + 1];
			const int32 a0 = M.Vertex(TP(P0, kOculus, H0), -Radial(P0)), a1 = M.Vertex(TP(P1, kOculus, H0), -Radial(P1));
			const int32 b0 = M.Vertex(TP(P0, kOculus, kLaylight), -Radial(P0)), b1 = M.Vertex(TP(P1, kOculus, kLaylight), -Radial(P1));
			M.Quad(a0, a1, b1, b0);
		}
		{
			FMesh& L = Hall[EPart::Laylight];
			const int32 C = L.Vertex(TP(0, 0, kLaylight), FVector(0, 0, -1));
			TArray<int32> Rim;
			for (int32 k = 0; k < kRound; ++k) { Rim.Add(L.Vertex(TP(Round[k], kOculus, kLaylight), FVector(0, 0, -1))); }
			for (int32 k = 0; k < kRound; ++k) { L.Tri(C, Rim[k], Rim[(k + 1) % kRound]); }
		}
		FProfile Bead;
		Bead.Add(kOculus - 0.01, H0 + 0.02).Round(kOculus - 0.01, H0 - 0.06, 8, true);
		Bead.Add(kOculus + 0.02, H0 - 0.06).Add(kOculus + 0.02, H0 + 0.02);
		// Traced with the solid on the left: down the bead's round face, out, back up behind.
		Lathe(Hall[EPart::Gilt], FVector(0, kTY, 0), FVector(0, 0, 1), FVector(1, 0, 0), Bead, 0.0, 2.0 * Pi, 120, true);
	}

	/** The ring beam on the columns: the entablature's two faces round its soffit, from the dome's springing to the annular vault's. */
	void RingBeam(FHall& Hall)
	{
		const FProfile E = Entablature(0.0);
		// The inner face (r decreasing with A) from the dome's springing down to the soffit, then the outer face up.
		const double RIn = kRc - kRTop, ROut = kRc + kRTop;
		FProfile P;
		// Inner: walk the entablature's section backwards (ledge → soffit), r = RIn − A.
		P.Add(kDomeSpring, kLedge);
		for (int32 i = E.Num() - 2; i >= 1; --i) { P.Add(RIn - E.P[i].X, kArchitrave + E.P[i].Y, E.Smooth[i]); }
		// Outer: forwards (soffit → ledge), r = ROut + A.
		for (int32 i = 1; i <= E.Num() - 2; ++i) { P.Add(ROut + E.P[i].X, kArchitrave + E.P[i].Y, E.Smooth[i]); }
		P.Add(kAnnularSpring, kLedge);
		Lathe(Hall[EPart::Carved], FVector(0, kTY, 0), FVector(0, 0, 1), FVector(1, 0, 0), P, 0.0, 2.0 * Pi, kRound);
		DentilRing(Hall[EPart::Carved], ROut + 0.045, 1.0);
		DentilRing(Hall[EPart::Carved], RIn - 0.045, -1.0);
	}

	// ---------------------------------------------------------------------------------------------- floors

	/** A ring between two concentric circles' samples, zipped by angle (the tribune's bands). */
	void RingZip(FMesh& M, const TArray<FVector>& Outer, const TArray<FVector>& Inner, const FVector2D& C, const FVector& Nrm)
	{
		auto Chain = [&M, &C, &Nrm](const TArray<FVector>& L, TArray<int32>& Id, TArray<double>& Key)
		{
			// Start at the point of least angle, keep angles increasing, close with the first point + 2π.
			TArray<double> Ang;
			for (const FVector& P : L) { Ang.Add(FMath::Atan2(P.Y - C.Y, P.X - C.X)); }
			int32 Start = 0;
			for (int32 i = 1; i < L.Num(); ++i) { if (Ang[i] < Ang[Start]) { Start = i; } }
			// Direction: angles must increase along the list.
			const int32 Next = (Start + 1) % L.Num();
			const int32 Dir = FMath::Fmod(Ang[Next] - Ang[Start] + 4.0 * Pi, 2.0 * Pi) < Pi ? 1 : -1;
			double Prev = Ang[Start];
			for (int32 k = 0; k <= L.Num(); ++k)
			{
				const int32 i = ((Start + Dir * k) % L.Num() + L.Num()) % L.Num();
				double A = Ang[i];
				while (A < Prev - 1e-12) { A += 2.0 * Pi; }
				if (k == L.Num()) { A = Ang[Start] + 2.0 * Pi; }
				Id.Add(M.Vertex(L[i], Nrm));
				Key.Add(A);
				Prev = A;
			}
		};
		TArray<int32> A, B;
		TArray<double> KA, KB;
		Chain(Outer, A, KA);
		Chain(Inner, B, KB);
		// Both chains begin at their least angle and end at it again, a turn on: the zip closes the ring.
		Zip(M, A, KA, B, KB);
	}

	TArray<FVector> Circle(const FVector2D& C, double R, int32 N, double Phase = 0.0)
	{
		TArray<FVector> Out;
		for (int32 i = 0; i < N; ++i)
		{
			const double A = Phase + 2.0 * Pi * i / N;
			Out.Add(FVector(C.X + R * FMath::Cos(A), C.Y + R * FMath::Sin(A), 0.0));
		}
		return Out;
	}

	TArray<FVector> Rect(double X0, double X1, double Y0, double Y1)
	{
		return {FVector(X0, Y0, 0), FVector(X1, Y0, 0), FVector(X1, Y1, 0), FVector(X0, Y1, 0)};
	}

	/** A disc (fan from its centre). */
	void Disc(FMesh& M, const TArray<FVector>& Rim, const FVector2D& C)
	{
		const FVector Up(0, 0, 1);
		const int32 Ci = M.Vertex(FVector(C.X, C.Y, 0), Up);
		TArray<int32> R;
		for (const FVector& P : Rim) { R.Add(M.Vertex(P, Up)); }
		for (int32 i = 0; i < R.Num(); ++i) { M.Tri(Ci, R[i], R[(i + 1) % R.Num()]); }
	}

	/** The gallery's carpet: per bay a square with a porphyry disc; lozenges between; verde frames. Returns its side y's. */
	TArray<double> GalleryCarpet(FHall& Hall)
	{
		const FVector Up(0, 0, 1);
		TArray<double> Ys = {kCarpetSouth, kCarpetNorth};
		const double X0 = -kCarpetHalf, X1 = kCarpetHalf, FW = 0.15;
		// The end zones: plain verde.
		Poly(Hall[EPart::FloorVerde], Rect(X0, X1, kCarpetSouth - 0.5, kCarpetSouth), Up);
		Poly(Hall[EPart::FloorVerde], Rect(X0, X1, kCarpetNorth, kCarpetNorth + 0.5), Up);
		Ys.Append({kCarpetSouth - 0.5, kCarpetNorth + 0.5});
		for (int32 k = 0; k < kBays; ++k)
		{
			const double YC = BayCentre(k);
			const double Y0 = YC - 2.1, Y1 = YC + 2.1;
			Ys.Append({Y0, Y1});
			const FVector2D C(0.0, YC);
			const TArray<FVector> Sq = Rect(X0 + FW, X1 - FW, Y0 + FW, Y1 - FW);
			PolyWithHole(Hall[EPart::FloorVerde], Rect(X0, X1, Y0, Y1), Sq, Up);
			const TArray<FVector> Ring = Circle(C, 1.65, 96), Inner = Circle(C, 1.55, 96);
			PolyWithHole(Hall[EPart::FloorGiallo], Sq, Ring, Up);
			RingZip(Hall[EPart::FloorWhite], Ring, Inner, C, Up);
			Disc(Hall[EPart::FloorPorphyry], Inner, C);
			// Between this bay and the next (at the column pairs): three rosso lozenges in white cells.
			if (k + 1 < kBays)
			{
				const double TY0 = Y0 - 1.8, TY1 = Y0;
				const double IY0 = TY0 + FW, IY1 = TY1 - FW;
				TArray<double> Xs;
				for (int32 j = 0; j <= 3; ++j) { Xs.Add(X0 + FW + (X1 - X0 - 2 * FW) * j / 3.0); }
				// The frame's inner edge carries the cells' corners.
				TArray<FVector> In;
				for (int32 j = 0; j <= 3; ++j) { In.Add(FVector(Xs[j], IY0, 0)); }
				for (int32 j = 3; j >= 0; --j) { In.Add(FVector(Xs[j], IY1, 0)); }
				PolyWithHole(Hall[EPart::FloorVerde], Rect(X0, X1, TY0, TY1), In, Up);
				for (int32 j = 0; j < 3; ++j)
				{
					const FVector2D LC((Xs[j] + Xs[j + 1]) * 0.5, (IY0 + IY1) * 0.5);
					const double LX = 0.55, LY = 0.55;
					const TArray<FVector> Loz = {FVector(LC.X + LX, LC.Y, 0), FVector(LC.X, LC.Y + LY, 0), FVector(LC.X - LX, LC.Y, 0), FVector(LC.X, LC.Y - LY, 0)};
					PolyWithHole(Hall[EPart::FloorWhite], Rect(Xs[j], Xs[j + 1], IY0, IY1), Loz, Up);
					Poly(Hall[EPart::FloorRosso], Loz, Up);
				}
			}
		}
		return Unique(Ys, 1e-7);
	}

	/** The tribune's floor pattern (r ≤ 6.3), polar: every ring shares the 240 stations round (plus the jamb angles on the rim). */
	void TribunePattern(FHall& Hall, const TArray<double>& RimAngles)
	{
		const FVector Up(0, 0, 1);
		const FVector2D C = TC();
		TArray<double> Round;
		for (int32 k = 0; k < kRound; ++k) { Round.Add(k * kStep); }
		auto RingPts = [&C](double R, const TArray<double>& Angles)
		{
			TArray<FVector> Out;
			for (const double A : Angles) { Out.Add(FVector(C.X + R * FMath::Cos(A), C.Y + R * FMath::Sin(A), 0)); }
			return Out;
		};
		auto Band = [&](EPart Part, double R0, double R1, const TArray<double>& A0, const TArray<double>& A1)
		{
			RingZip(Hall[Part], RingPts(R1, A1), RingPts(R0, A0), C, Up);
		};
		auto P2 = [&C](double R, double A) { return FVector(C.X + R * FMath::Cos(A), C.Y + R * FMath::Sin(A), 0); };
		// The centre: a porphyry disc under the pedestal, a giallo ring.
		Disc(Hall[EPart::FloorPorphyry], RingPts(1.5, Round), C);
		Band(EPart::FloorGiallo, 1.5, 1.65, Round, Round);
		// A cell between two column angles: annular sector, optionally a border band, a disc on its axis.
		auto Sector = [&](EPart FieldPart, EPart DiscPart, double R0, double R1, double A0, double A1, double DiscR, double Border)
		{
			TArray<double> In;
			for (int32 k = FMath::RoundToInt(A0 / kStep); k <= FMath::RoundToInt(A1 / kStep); ++k) { In.Add(k * kStep); }
			TArray<FVector> Cell;
			for (const double A : In) { Cell.Add(P2(R0, A)); }
			for (int32 i = In.Num() - 1; i >= 0; --i) { Cell.Add(P2(R1, In[i])); }
			const double AM = (A0 + A1) * 0.5, RM = (R0 + R1) * 0.5;
			const FVector2D DC(C.X + RM * FMath::Cos(AM), C.Y + RM * FMath::Sin(AM));
			TArray<FVector> Field = Cell;
			if (Border > 0)
			{
				// Inset by Border: arcs at R0 + b and R1 − b; sides parallel to the radial edges.
				Field.Reset();
				const int32 N = In.Num();
				const double RA = R0 + Border, RB = R1 - Border;
				const double SA0 = A0 + FMath::Asin(Border / RA), SA1 = A1 - FMath::Asin(Border / RA);
				const double SB0 = A0 + FMath::Asin(Border / RB), SB1 = A1 - FMath::Asin(Border / RB);
				for (int32 i = 0; i < N; ++i) { Field.Add(P2(RA, FMath::Lerp(SA0, SA1, double(i) / (N - 1)))); }
				for (int32 i = N - 1; i >= 0; --i) { Field.Add(P2(RB, FMath::Lerp(SB0, SB1, double(i) / (N - 1)))); }
				PolyWithHole(Hall[EPart::FloorWhite], Cell, Field, Up);
			}
			const TArray<FVector> D = Circle(DC, DiscR, 48, AM);
			PolyWithHole(Hall[FieldPart], Field, D, Up);
			Disc(Hall[DiscPart], D, DC);
		};
		for (int32 k = 0; k < 12; ++k)
		{
			const double A0 = Rad(15.0 + 30.0 * k), A1 = A0 + Rad(30.0);
			Sector(EPart::FloorWhite, EPart::FloorPorphyry, 1.65, 2.9, A0, A1, 0.22, 0.0);
		}
		Band(EPart::FloorGiallo, 2.9, 3.05, Round, Round);
		Band(EPart::FloorWhite, 3.05, 3.8, Round, Round);
		Band(EPart::FloorVerde, 3.8, 3.95, Round, Round);
		for (int32 k = 0; k < 12; ++k)
		{
			const double A0 = Rad(15.0 + 30.0 * k), A1 = A0 + Rad(30.0);
			Sector(EPart::FloorGiallo, (k % 2 == 0) ? EPart::FloorPorphyry : EPart::FloorVerde, 3.95, 6.15, A0, A1, 0.8, 0.1);
		}
		// The outer band, its rim with the jamb angles too (the threshold and the white border meet it there).
		Band(EPart::FloorVerde, 6.15, kPattern, Round, RimAngles);
	}

	void Floors(FHall& Hall, const TArray<double>& LongSt, const TArray<double>& SouthSt, const FWallFace& Tribune)
	{
		const FVector Up(0, 0, 1);
		FMesh& White = Hall[EPart::FloorWhite];
		const TArray<double> CarpetYs = GalleryCarpet(Hall);
		// The side strips (x from the carpet to the walls), y from the south strip to the north strip.
		for (const double Sgn : {-1.0, 1.0})
		{
			TArray<int32> A, B;
			TArray<double> KA, KB;
			for (const double Y : Within(LongSt, kCarpetNorth, kCarpetSouth)) { A.Add(White.Vertex(FVector(Sgn * kHW, Y, 0), Up)); KA.Add(Y); }
			for (const double Y : CarpetYs) { B.Add(White.Vertex(FVector(Sgn * kCarpetHalf, Y, 0), Up)); KB.Add(Y); }
			Zip(White, A, KA, B, KB);
		}
		// The south strip: the south wall's foot (and the riser across the door) to the carpet's end.
		{
			TArray<int32> A, B;
			TArray<double> KA, KB;
			for (const double X : SouthSt) { A.Add(White.Vertex(FVector(X, kYS, 0), Up)); KA.Add(X); }
			for (const double X : {-kHW, -kCarpetHalf, kCarpetHalf, kHW}) { B.Add(White.Vertex(FVector(X, kCarpetSouth, 0), Up)); KB.Add(X); }
			Zip(White, A, KA, B, KB);
		}
		// The north strip: from the carpet's end to the screen's line.
		{
			TArray<int32> A, B;
			TArray<double> KA, KB;
			for (const double X : {-kHW, -kCarpetHalf, kCarpetHalf, kHW}) { A.Add(White.Vertex(FVector(X, kCarpetNorth, 0), Up)); KA.Add(X); }
			for (const double X : {-kHW, -kCarpetHalf, kCarpetHalf, kHW}) { B.Add(White.Vertex(FVector(X, kYN, 0), Up)); KB.Add(X); }
			Zip(White, A, KA, B, KB);
		}
		// The tribune: pattern, the white border to the wall, the threshold under the lintel.
		const TArray<double> Angles = TribuneAngles();
		TArray<double> Rim;
		for (int32 k = 0; k < kRound; ++k) { Rim.Add(k * kStep); }
		Rim.Append({PhiE(), PhiW()});
		Rim = Unique(Rim, 1e-9);
		TribunePattern(Hall, Rim);
		const FVector2D C = TC();
		auto At = [&C](double R, double A) { return FVector(C.X + R * FMath::Cos(A), C.Y + R * FMath::Sin(A), 0); };
		{
			// The border from the jamb W round the north to the jamb E.
			TArray<int32> A, B;
			TArray<double> KA, KB;
			for (const double P : Angles)
			{
				if (P < PhiW() - 1e-9) { continue; }
				A.Add(White.Vertex(At(kRo, P), Up));
				KA.Add(P);
			}
			for (const double P0 : Rim)
			{
				double P = P0;
				if (P < PhiW() - 1e-9) { P += 2.0 * Pi; }
				if (P > PhiE() + 2.0 * Pi + 1e-9) { continue; }
				B.Add(White.Vertex(At(kPattern, P), Up));
				KB.Add(P);
			}
			// Sort the second chain by angle.
			TArray<int32> Order;
			for (int32 i = 0; i < KB.Num(); ++i) { Order.Add(i); }
			Order.Sort([&KB](int32 L, int32 R) { return KB[L] < KB[R]; });
			TArray<int32> B2;
			TArray<double> KB2;
			for (const int32 i : Order) { B2.Add(B[i]); KB2.Add(KB[i]); }
			Zip(White, A, KA, B2, KB2);
		}
		{
			// The threshold under the lintel: the screen's line, the jambs, the pattern's rim between the jamb angles.
			TArray<FVector> Pts;
			for (const double X : {kHW, kCarpetHalf, -kCarpetHalf, -kHW}) { Pts.Add(FVector(X, kYN, 0)); }
			for (const double Y : Within(LongSt, JambY(), kYN)) { if (Y < kYN - 1e-9) { Pts.Add(FVector(-kHW, Y, 0)); } }
			TArray<double> Mid;
			for (const double P : Rim) { if (P >= PhiE() - 1e-9 && P <= PhiW() + 1e-9) { Mid.Add(P); } }
			for (int32 i = Mid.Num() - 1; i >= 0; --i) { Pts.Add(At(kPattern, Mid[i])); }
			TArray<double> East = Within(LongSt, JambY(), kYN);
			for (const double Y : East) { if (Y < kYN - 1e-9) { Pts.Add(FVector(kHW, Y, 0)); } }
			Poly(White, Pts, Up);
		}
	}

	// ---------------------------------------------------------------------------------------------- the lintel and the screen

	TArray<double> NorthStations() { return Unique(Us(EndArc()), 1e-7); }

	void Lintel(FHall& Hall, const TArray<double>& LongSt, const FWallFace& Tribune)
	{
		// The face over the screen (towards the gallery), from the soffit to the vault.
		FWallFace F;
		F.Place = [](double U, double H) { return FVector(U, kYN, H); };
		F.Normal = [](double, double) { return FVector(0, 1, 0); };
		F.Bottom = [](double, double) { return kSoffit; };
		const TArray<FVector2D> Arc = EndArc();
		F.Top = [Arc](double U, double) { return Interp(Arc, U); };
		F.Stations = NorthStations();
		FMesh Unused;
		F.Build(Unused, Hall[EPart::Wall]);
		// The soffit: the screen's line (east to west), the west jamb's top, the tribune's wall from the west jamb's corner
		// round to the east one's, the east jamb's top.
		TArray<double> Mid;
		for (const double U : Tribune.Stations) { const double A = U / kRo; if (A >= PhiE() - 1e-9 && A <= PhiW() + 1e-9) { Mid.Add(A); } }
		TArray<FVector> Soffit;
		for (int32 i = F.Stations.Num() - 1; i >= 0; --i) { Soffit.Add(FVector(F.Stations[i], kYN, kSoffit)); }
		const TArray<double> Jamb = Within(LongSt, JambY(), kYN);
		for (int32 i = Jamb.Num() - 1; i >= 0; --i) { if (Jamb[i] < kYN - 1e-9 && Jamb[i] > JambY() + 1e-9) { Soffit.Add(FVector(-kHW, Jamb[i], kSoffit)); } }
		for (int32 i = Mid.Num() - 1; i >= 0; --i) { Soffit.Add(TP(Mid[i], kRo, kSoffit)); }
		for (int32 i = 0; i < Jamb.Num(); ++i) { if (Jamb[i] < kYN - 1e-9 && Jamb[i] > JambY() + 1e-9) { Soffit.Add(FVector(kHW, Jamb[i], kSoffit)); } }
		Poly(Hall[EPart::Wall], Soffit, FVector(0, 0, -1));
	}

	// ---------------------------------------------------------------------------------------------- the shell

	void Shell(FHall& Hall, const FWallFace& Lip)
	{
		FMesh& M = Hall[EPart::Shell];
		// The south face with the sleeve's opening.
		FWallFace S;
		S.Place = [](double U, double H) { return FVector(U, kShellSouth, H); };
		S.Normal = [](double, double) { return FVector(0, 1, 0); };
		S.Bottom = [](double, double) { return kBottom; };
		S.Top = [](double, double) { return kRoof; };
		TArray<double> St = Us(SleeveHead());
		St.Append({-kShellHalf, kShellHalf});
		S.Stations = Unique(St, 1e-7);
		FOpening O;
		O.U0 = -kSleeveHalf;
		O.U1 = kSleeveHalf;
		O.Sill = kBottom;
		O.Head = SleeveHead();
		S.Openings.Add(O);
		FMesh Unused;
		S.Build(Unused, M);
		Sleeve(Hall, Lip, S);
		// The drum round the tribune and the gallery's box: the footprint's outline.
		const double YJ = kTY + FMath::Sqrt(kShellRadius * kShellRadius - kShellHalf * kShellHalf);
		const double AE = FMath::Atan2(YJ - kTY, kShellHalf), AW = Pi - AE;
		TArray<double> Arc;   // from the west point round the north to the east point
		{
			const int32 N = FMath::CeilToInt((AE + 2.0 * Pi - AW) / Rad(1.5));
			for (int32 i = 0; i <= N; ++i) { Arc.Add(AW + (AE + 2.0 * Pi - AW) * i / N); }
		}
		TArray<FVector> Foot;   // anticlockwise seen from above? any order: the polygon helper handles either
		Foot.Add(FVector(kShellHalf, kShellSouth, 0));
		Foot.Add(FVector(-kShellHalf, kShellSouth, 0));
		for (const double A : Arc) { Foot.Add(TP(A, kShellRadius, 0)); }
		// Remove the duplicate corners (the arc's ends are the box's corners at YJ).
		// The roof: the south face's top stations along its south edge.
		{
			TArray<FVector> Roof;
			for (int32 i = S.Stations.Num() - 1; i >= 0; --i) { Roof.Add(FVector(S.Stations[i], kShellSouth, kRoof)); }
			for (const double A : Arc) { Roof.Add(TP(A, kShellRadius, kRoof)); }
			Poly(M, Roof, FVector(0, 0, 1));
		}
		// The bottom, with the sleeve's floor running south to the lip.
		{
			TArray<FVector> Bot;
			Bot.Add(FVector(kShellHalf, kShellSouth, kBottom));
			Bot.Add(FVector(kSleeveHalf, kShellSouth, kBottom));
			const TArray<double> LipSt = Within(Lip.Stations, -kSleeveHalf, kSleeveHalf);
			for (int32 i = LipSt.Num() - 1; i >= 0; --i) { Bot.Add(Lip.Place(LipSt[i], kBottom)); }
			Bot.Add(FVector(-kSleeveHalf, kShellSouth, kBottom));
			Bot.Add(FVector(-kShellHalf, kShellSouth, kBottom));
			for (const double A : Arc) { Bot.Add(TP(A, kShellRadius, kBottom)); }
			Poly(M, Bot, FVector(0, 0, -1));
		}
		// The sides: the box's two faces, and the drum.
		for (const double Sgn : {-1.0, 1.0})
		{
			const FVector N(Sgn, 0, 0);
			const int32 a0 = M.Vertex(FVector(Sgn * kShellHalf, kShellSouth, kBottom), N), a1 = M.Vertex(FVector(Sgn * kShellHalf, YJ, kBottom), N);
			const int32 b1 = M.Vertex(FVector(Sgn * kShellHalf, YJ, kRoof), N), b0 = M.Vertex(FVector(Sgn * kShellHalf, kShellSouth, kRoof), N);
			M.Quad(a0, a1, b1, b0);
		}
		for (int32 i = 0; i + 1 < Arc.Num(); ++i)
		{
			const double A0 = Arc[i], A1 = Arc[i + 1];
			const int32 a0 = M.Vertex(TP(A0, kShellRadius, kBottom), Radial(A0)), a1 = M.Vertex(TP(A1, kShellRadius, kBottom), Radial(A1));
			const int32 b1 = M.Vertex(TP(A1, kShellRadius, kRoof), Radial(A1)), b0 = M.Vertex(TP(A0, kShellRadius, kRoof), Radial(A0));
			M.Quad(a0, a1, b1, b0);
		}
	}

	// ---------------------------------------------------------------------------------------------- the order in place

	/** The gallery's entablature: west wall north, the screen east, the east wall south; ressauts over the pairs. */
	void GalleryEntablature(FHall& Hall)
	{
		FMesh& C = Hall[EPart::Carved];
		const double X = kHW - kWallA0, XR = kHW - kRessautA0;
		TArray<FVector2D> Path;
		// The west wall, from inside the south wall northwards.
		Path.Add(FVector2D(-X, kYS + kSink));
		for (int32 k = 1; k < kBays; ++k)
		{
			const double YP = PairCentre(k);
			Path.Append({FVector2D(-X, YP + kRessautHalf), FVector2D(-XR, YP + kRessautHalf), FVector2D(-XR, YP - kRessautHalf), FVector2D(-X, YP - kRessautHalf)});
		}
		const double YScreen = kYN + kWallA0;
		Path.Add(FVector2D(-X, YScreen));
		Path.Add(FVector2D(X, YScreen));
		for (int32 k = kBays - 1; k >= 1; --k)
		{
			const double YP = PairCentre(k);
			Path.Append({FVector2D(X, YP - kRessautHalf), FVector2D(XR, YP - kRessautHalf), FVector2D(XR, YP + kRessautHalf), FVector2D(X, YP + kRessautHalf)});
		}
		Path.Add(FVector2D(X, kYS + kSink));
		SweepPlan(C, Path, false, Entablature(-(kWallA0 + kSink)), true, kArchitrave, 20.0);
		// Dentils on each straight run, and the fillers behind the ressauts.
		const double BandA = 0.045;
		for (int32 i = 0; i + 1 < Path.Num(); ++i)
		{
			const FVector2D A = Path[i], B = Path[i + 1];
			const FVector2D D = Unit((B - A));
			const FVector2D L(-D.Y, D.X);
			// Offset the run to the dentil band and shorten it at mitres by the band's offset.
			const FVector From(A.X + L.X * BandA, A.Y + L.Y * BandA, 0), To(B.X + L.X * BandA, B.Y + L.Y * BandA, 0);
			Dentils(C, From - FVector(L.X, L.Y, 0) * BandA, To - FVector(L.X, L.Y, 0) * BandA, FVector(L.X, L.Y, 0), BandA, 0.06);
		}
		for (int32 k = 1; k < kBays; ++k)
		{
			const double YP = PairCentre(k);
			for (const double Sgn : {-1.0, 1.0})
			{
				const double XBack = Sgn * (kHW + 2.0 * kSink);
				const double XFront = Sgn * (kHW - kRessautA0 + kWallA0 + kSink - 0.01);
				const FVector Lo(FMath::Min(XBack, XFront), YP - kRessautHalf + 0.07, kSoffit - 0.005);
				const FVector Hi(FMath::Max(XBack, XFront), YP + kRessautHalf - 0.07, kLedge - 0.015);
				Box(C, Lo, Hi);
			}
		}
	}

	/** The skirting and the dado rail: round the gallery and the tribune, from one side of the door to the other. */
	void WallMouldings(FHall& Hall)
	{
		FMesh& C = Hall[EPart::Carved];
		const double DoorEdge = kDoorHalf + 0.25;
		TArray<FVector2D> Path;
		Path.Add(FVector2D(-DoorEdge, kYS));
		Path.Add(FVector2D(-kHW, kYS));
		// Then round the tribune from the west jamb's corner (−3, JambY) to the east one.
		const TArray<double> Angles = TribuneAngles();
		for (const double A : Angles)
		{
			if (A < PhiW() - 1e-9) { continue; }
			const FVector P = TP(A, kRo, 0);
			Path.Add(FVector2D(P.X, P.Y));
		}
		Path.Add(FVector2D(kHW, kYS));
		Path.Add(FVector2D(DoorEdge, kYS));
		SweepPlan(C, Path, false, Skirting(), true, 0.0, 20.0);
		SweepPlan(C, Path, false, DadoRail(), true, 0.0, 20.0);
	}

	/** The tribune's outer entablature: round the wall (over the opening too, the lintel's face). */
	void TribuneEntablature(FHall& Hall)
	{
		const FProfile E = Entablature(-(kWallA0 + kSink));
		Lathe(Hall[EPart::Carved], FVector(0, kTY, 0), FVector(0, 0, 1), FVector(1, 0, 0), ToLathe(E, kRo - kWallA0, -1.0, kArchitrave), 0.0, 2.0 * Pi, kRound,
			  true);
		DentilRing(Hall[EPart::Carved], kRo - kWallA0 - 0.045, -1.0);
	}

	// ---------------------------------------------------------------------------------------------- fittings

	/** A niche's statue plinth: low, set back into the niche (its middle over the half-dome's highest reach for the statue). */
	double NichePlinthHeight(const FNiche& N) { return N.bCurved ? 0.08 : 0.12; }
	FVector NichePlinthCentre(const FNiche& N) { return N.Centre + N.In * (N.R * 0.42); }

	/** A moulded plinth: a block with a base (fillet and cyma) and a cap (cyma and fillet), on the floor at Base. */
	void Plinth(FMesh& M, const FVector& Base, const FVector& AxisIn, double HalfA, double HalfB, double Height)
	{
		const FVector Up(0, 0, 1);
		const FVector A = Unit(FVector(AxisIn.X, AxisIn.Y, 0));
		const FVector B = FVector::CrossProduct(Up, A);
		TArray<FVector2D> Sq;
		for (const FVector2D& Q : {FVector2D(-1, -1), FVector2D(1, -1), FVector2D(1, 1), FVector2D(-1, 1)})
		{
			const FVector P = Base + A * (Q.X * HalfA) + B * (Q.Y * HalfB);
			Sq.Add(FVector2D(P.X, P.Y));
		}
		double Area = 0.0;
		for (int32 i = 0; i < 4; ++i) { Area += Cross2(Sq[i], Sq[(i + 1) % 4]); }
		if (Area > 0) { Sq = {Sq[3], Sq[2], Sq[1], Sq[0]}; }   // clockwise: A outwards
		FProfile Pr;
		Pr.Add(0.03, -kSink).Add(0.03, 0.06).Cyma(0.0, 0.1, 3, true).Add(0.0, Height - 0.075).Cyma(0.03, Height - 0.02, 3, false).Add(0.03, Height);
		SweepPlan(M, Sq, true, Pr, false, Base.Z, 20.0);
		Poly(M, PlanOutline(Sq, true, FVector2D(0.03, -kSink), Base.Z), -Up);
		Poly(M, PlanOutline(Sq, true, FVector2D(0.03, Height), Base.Z), Up);
	}

	/** A herm for a bust: a pillar tapering downwards on a base, under a moulded cap. Returns its top. */
	FVector Herm(FMesh& M, const FVector& Base, const FVector& Facing)
	{
		const FVector Up(0, 0, 1);
		const FVector A = Unit(FVector(Facing.X, Facing.Y, 0));
		const FVector B = FVector::CrossProduct(Up, A);
		Plinth(M, Base, A, 0.2, 0.2, 0.14);
		// The shaft: 0.26 square at its foot, 0.32 under the cap (a herm widens upwards).
		const double H0 = 0.13, H1 = 1.17;
		TArray<FVector> Lo, Hi;
		for (const FVector2D& Q : {FVector2D(-1, -1), FVector2D(1, -1), FVector2D(1, 1), FVector2D(-1, 1)})
		{
			Lo.Add(Base + A * (Q.X * 0.13) + B * (Q.Y * 0.13) + Up * H0);
			Hi.Add(Base + A * (Q.X * 0.16) + B * (Q.Y * 0.16) + Up * H1);
		}
		for (int32 i = 0; i < 4; ++i)
		{
			const int32 j = (i + 1) % 4;
			const FVector N = Unit(FVector::CrossProduct(Hi[j] - Lo[i], Lo[j] - Lo[i]));
			const FVector Mid = (Lo[i] + Lo[j] + Hi[i] + Hi[j]) * 0.25;
			const FVector Out = FVector::DotProduct(N, Mid - (Base + Up * 0.6)) > 0 ? N : -N;
			Poly(M, {Lo[i], Lo[j], Hi[j], Hi[i]}, Out);
		}
		Poly(M, Lo, -Up);
		Poly(M, Hi, Up);
		Plinth(M, Base + Up * (H1 - kSink), A, 0.2, 0.2, 0.09);
		return Base + Up * (H1 - kSink + 0.09);
	}

	/**
	 * A keystone: a block widening upwards from the arch's crown, on a wall at O (the crown's point on the wall's face,
	 * h 0), Proj proud of it and its back Sink into it.
	 */
	void Keystone(FMesh& M, const FVector& O, const FVector& Along, const FVector& Out, double H0, double H1, double W0, double W1, double Proj, double Sink)
	{
		const FVector Up(0, 0, 1);
		auto At = [&O, &Along, &Out, &Up](double S, double H, double D) { return O + Along * S + Up * H + Out * D; };
		const FVector C = At(0, (H0 + H1) * 0.5, (Proj - Sink) * 0.5);
		const FVector BL0 = At(-W0 / 2, H0, -Sink), BR0 = At(W0 / 2, H0, -Sink), BL1 = At(-W0 / 2, H0, Proj), BR1 = At(W0 / 2, H0, Proj);
		const FVector TL0 = At(-W1 / 2, H1, -Sink), TR0 = At(W1 / 2, H1, -Sink), TL1 = At(-W1 / 2, H1, Proj), TR1 = At(W1 / 2, H1, Proj);
		auto Face = [&M, &C](const TArray<FVector>& Q)
		{
			FVector N = Unit(FVector::CrossProduct(Q[1] - Q[0], Q[2] - Q[0]));
			const FVector Mid = (Q[0] + Q[1] + Q[2] + Q[3]) * 0.25;
			if (FVector::DotProduct(N, Mid - C) < 0) { N = -N; }
			Poly(M, Q, N);
		};
		Face({BL0, BR0, BR1, BL1});
		Face({TL0, TR0, TR1, TL1});
		Face({BL1, BR1, TR1, TL1});
		Face({BL0, BR0, TR0, TL0});
		Face({BL0, BL1, TL1, TL0});
		Face({BR0, BR1, TR1, TR0});
	}

	/** A pavonazzetto panel in a moulded frame on a wall (O at its bottom middle on the wall's face), 2 cm proud. */
	void FramedPanel(FHall& Hall, const FVector& O, const FVector& Out, const FVector& Along, double HalfW, double H0, double H1, double Sunk)
	{
		const FVector Up(0, 0, 1);
		Box(Hall[EPart::Panel], O + Up * ((H0 + H1) * 0.5) + Out * ((0.02 - kSink - Sunk) * 0.5), Along, Out, Up, FVector(HalfW, (0.02 + kSink + Sunk) * 0.5, (H1 - H0) * 0.5));
		// The frame (A outwards in the wall's plane from the panel's edge, B out of the wall), its back deeper than the panel's.
		TArray<FVector2D> Loop = {FVector2D(-HalfW, H0), FVector2D(-HalfW, H1), FVector2D(HalfW, H1), FVector2D(HalfW, H0)};
		FProfile Fr = PanelFrame();
		for (FVector2D& P : Fr.P) { if (P.Y < 0) { P.Y = -2.0 * kSink - Sunk; } }
		SweepRuns(Hall[EPart::Carved], PlaneRuns(Loop, true, O, Along, Up, Out, 20.0), Fr, true);
	}

	/** A niche's architrave band, keystone and statue plinth. Returns the plinth's top centre. */
	FVector NicheFittings(FHall& Hall, const FNiche& N)
	{
		FMesh& C = Hall[EPart::Carved];
		const FVector Up(0, 0, 1);
		// The band runs on the mouth plane (a flat wall) or on the tangent plane pushed onto the curve (curved walls): a
		// curved wall's band is built on the plane through the jambs' wall points, sunk deeper so it meets the wall.
		const double Sunk = N.bCurved ? kRo - FMath::Sqrt(kRo * kRo - (N.R + kNicheBand) * (N.R + kNicheBand)) : 0.0;
		const FVector O = N.Centre - N.In * Sunk;
		TArray<FVector2D> Path;
		Path.Add(FVector2D(-N.R, kDadoTop + kSink));
		{
			const TArray<FVector2D> Head = ArchHead(0.0, N.R, N.Spring, kArchSegments);
			for (const FVector2D& P : Head) { Path.Add(P); }
		}
		Path.Add(FVector2D(N.R, kDadoTop + kSink));
		// Remove the doubled jamb tops (the arch's ends are the jambs' tops).
		TArray<FVector2D> Clean;
		for (const FVector2D& P : Path) { if (Clean.Num() == 0 || FVector2D::Distance(Clean.Last(), P) > 1e-9) { Clean.Add(P); } }
		FProfile Band = Surround(kNicheBand, 0.035 + Sunk);
		// The section's back must reach the wall: on a curved wall the band's ends are Sunk behind the tangent plane.
		SweepRuns(C, PlaneRuns(Clean, false, O, N.Lateral, Up, -N.In, 30.0), Band, false);
		// The keystone, from the crown up into the architrave.
		const double Crown = N.Spring + N.R;
		Keystone(C, O, N.Lateral, -N.In, Crown - 0.04, kArchitrave + kSink, 0.1, 0.15, 0.055, Sunk + 2.5 * kSink);
		// The plinth: a moulded block inside the niche.
		const double PW = N.bCurved ? 0.35 : 0.28, PD = N.bCurved ? 0.26 : 0.2, PH = NichePlinthHeight(N);
		const FVector PC = NichePlinthCentre(N);
		TArray<FVector2D> Sq;
		for (const FVector2D& Q : {FVector2D(-1, -1), FVector2D(1, -1), FVector2D(1, 1), FVector2D(-1, 1)})
		{
			const FVector P = PC + N.Lateral * (Q.X * PW) + N.In * (Q.Y * PD);
			Sq.Add(FVector2D(P.X, P.Y));
		}
		double Area = 0.0;
		for (int32 i = 0; i < 4; ++i) { Area += Cross2(Sq[i], Sq[(i + 1) % 4]); }
		if (Area > 0) { Sq = {Sq[3], Sq[2], Sq[1], Sq[0]}; }   // clockwise: A outwards
		FProfile Pl;
		Pl.Add(0.015, -0.005).Add(0.015, 0.025).Cyma(0.0, 0.045, 3, true).Add(0.0, PH - 0.03).Cyma(0.02, PH - 0.005, 3, false).Add(0.02, PH);
		const double Z0 = N.Sill;
		SweepPlan(C, Sq, true, Pl, false, Z0, 20.0);
		Poly(C, PlanOutline(Sq, true, FVector2D(0.015, -0.005), Z0), -Up);
		Poly(C, PlanOutline(Sq, true, FVector2D(0.02, PH), Z0), Up);
		return PC + FVector(0, 0, N.Sill + PH);
	}

	/** A pavonazzetto panel in a moulded frame on a wall plane (O at the panel's bottom middle), and a console for a bust. */
	FVector PanelAndConsole(FHall& Hall, const FVector& O, const FVector& Out, const FVector& Along, double Sunk)
	{
		const FVector Up(0, 0, 1);
		FramedPanel(Hall, O, Out, Along, 0.4, 0.85, 2.75, Sunk);
		// The console (a quadrant bracket) at the panel's middle height.
		const double Top = 1.55, Proj = 0.30;
		FProfile Co;
		Co.Add(-3.0 * kSink - Sunk, Top - 0.41).Add(0.06, Top - 0.41);
		Co.Arc(0.06, Top - 0.05, 0.21, 0.36, -90.0, 0.0, 8);
		Co.Add(Proj, Top - 0.05).Add(Proj, Top).Add(-3.0 * kSink - Sunk, Top);
		TArray<FSweepFrame> Run;
		for (const double S : {-0.18, 0.18})
		{
			FSweepFrame F;
			F.Origin = O + Along * S;
			F.AxisA = F.NormA = Out;
			F.AxisB = F.NormB = Up;
			F.S = S;
			Run.Add(F);
		}
		SweepSolid(Hall[EPart::Carved], Run, Co);
		return O + Out * (Proj * 0.5) + Up * Top;
	}

	/** The door from the shaft: its architrave, plinth blocks and keystone on the gallery's side. */
	void DoorFrame(FHall& Hall)
	{
		FMesh& C = Hall[EPart::Carved];
		const FVector Up(0, 0, 1), Out(0, -1, 0), Along(1, 0, 0);
		const FVector O(0, kYS, 0);
		const double Plinth = 0.32;
		TArray<FVector2D> Path = {FVector2D(kDoorHalf, Plinth)};
		const TArray<FVector2D> Head = ArchHead(0.0, kDoorHalf, kDoorSpring, kArchSegments);
		for (int32 i = Head.Num() - 1; i >= 0; --i) { Path.Add(Head[i]); }
		Path.Add(FVector2D(-kDoorHalf, Plinth));
		// Traverse so that the section's A points away from the opening: up the east jamb, over, down the west.
		SweepRuns(C, PlaneRuns(Path, false, O, -Along, Up, Out, 30.0), DoorArchitrave(), false);
		for (const double Sgn : {-1.0, 1.0})
		{
			const double X0 = Sgn * (kDoorHalf - 0.02), X1 = Sgn * (kDoorHalf + 0.27);
			Box(C, FVector(FMath::Min(X0, X1), kYS - 0.085, -kSink), FVector(FMath::Max(X0, X1), kYS + 2.0 * kSink, Plinth));
		}
		const double Crown = kDoorSpring + kDoorHalf;
		Keystone(C, O, Along, Out, Crown - 0.02, Crown + 0.46, 0.22, 0.3, 0.1, 2.0 * kSink);
	}

	void Pedestal(FHall& Hall)
	{
		FProfile P;
		P.Add(0.0, -kSink).Add(1.1, -kSink).Add(1.1, 0.03);
		P.Round(1.08, 0.07, 4, true);
		P.Add(1.05, 0.07).Add(1.05, 0.24);
		P.Cyma(1.1, 0.285, 3, true);
		P.Add(1.1, 0.3).Add(0.0, 0.3);
		Lathe(Hall[EPart::Carved], FVector(0, kTY, 0), FVector(0, 0, 1), FVector(1, 0, 0), P, 0.0, 2.0 * Pi, 120);
	}

	// ---------------------------------------------------------------------------------------------- the spots

	/** The pilasters: at the gallery's corners (clear of the end walls by 8 cm, the antae at the screen) and in the tribune. */
	void Pilasters(FHall& Hall)
	{
		for (const double Sgn : {-1.0, 1.0})
		{
			Pilaster(Hall, FVector(Sgn * kHW, kYS - 0.08 - kPilasterHalf, 0), FVector(-Sgn, 0, 0), EPart::ShaftGiallo);
			Pilaster(Hall, FVector(Sgn * kHW, kYN + kPilasterHalf, 0), FVector(-Sgn, 0, 0), EPart::ShaftGiallo);
		}
		const double Flank = (kPilasterHalf + 0.012) / kRo;
		for (const double A : {0.0, PhiE() - Flank, PhiW() + Flank, Pi, Rad(240.0), Rad(300.0)})
		{
			Pilaster(Hall, TP(A, kRo, 0), -Radial(A), EPart::ShaftPavonazzetto);
		}
	}

	struct FColumnSpec
	{
		FVector Base;
		FVector Front;
		EPart Shaft;
	};

	/** Every column: the gallery's pairs, the screen's two, the tribune's ring of twelve. */
	TArray<FColumnSpec> ColumnSpecs()
	{
		TArray<FColumnSpec> Out;
		for (int32 k = 1; k < kBays; ++k)
		{
			for (const double Sgn : {-1.0, 1.0})
			{
				for (const double Off : {-kPairHalf, kPairHalf}) { Out.Add({FVector(Sgn * kColumnAxis, PairCentre(k) + Off, 0), FVector(-Sgn, 0, 0), EPart::ShaftGiallo}); }
			}
		}
		for (const double X : {-1.1, 1.1}) { Out.Add({FVector(X, kYN + kWallA0 - kRTop, 0), FVector(0, 1, 0), EPart::ShaftGiallo}); }
		for (int32 k = 0; k < CH::RingColumns; ++k)
		{
			const double A = Rad(15.0 + 30.0 * k);
			Out.Add({TP(A, kRc, 0), Radial(A), EPart::ShaftPavonazzetto});
		}
		return Out;
	}

	void Columns(FHall& Hall)
	{
		for (const FColumnSpec& C : ColumnSpecs()) { Column(Hall, C.Base, C.Front, C.Shaft); }
	}

	// ---------------------------------------------------------------------------------------------- the works' places

	constexpr double kPedestalTop = 0.3;
	constexpr double kAmbulatoryR = 5.1;

	struct FPlinthSpec
	{
		FVector Base;        // on the floor, at its middle
		FVector Axis;        // its A side runs along this
		double HalfA = 0.5, HalfB = 0.5, Height = 0.5;
	};

	/** Low, long plinths on the gallery's axis in bays 2 and 3, for reclining figures (below the eye: the view runs on over them). */
	TArray<FPlinthSpec> AxisPlinths()
	{
		TArray<FPlinthSpec> Out;
		for (const int32 k : {1, 2}) { Out.Add({FVector(0, BayCentre(k), 0), FVector(0, 1, 0), 1.15, 0.52, 0.55}); }
		return Out;
	}

	/** Square plinths in the ambulatory on the tribune's east and west axes, for works seen in the round. */
	TArray<FPlinthSpec> AmbulatoryPlinths()
	{
		TArray<FPlinthSpec> Out;
		for (const double A : {0.0, Pi}) { Out.Add({TP(A, kAmbulatoryR, 0), Radial(A), 0.45, 0.45, 0.5}); }
		return Out;
	}

	/** Herms either side of the door from the stair, against the gallery's south wall. */
	TArray<FVector> HermBases() { return {FVector(-2.45, kYS - 0.27, 0), FVector(2.45, kYS - 0.27, 0)}; }

	/**
	 * Simple solids for the walk: octagonal prisms round the columns and the pedestal, boxes for plinths and herms, and a
	 * thin guard across each niche's mouth (a niche's sill is a visitor's step high). Never drawn.
	 */
	void Blockers(FHall& Hall)
	{
		FMesh& M = Hall[EPart::Collision];
		auto Prism = [&M](const FVector& C, double R, double H0, double H1)
		{
			TArray<FVector> Lo, Hi;
			for (int32 i = 0; i < 8; ++i)
			{
				const double A = 2.0 * Pi * (i + 0.5) / 8;
				Lo.Add(C + FVector(R * FMath::Cos(A), R * FMath::Sin(A), H0));
				Hi.Add(C + FVector(R * FMath::Cos(A), R * FMath::Sin(A), H1));
			}
			for (int32 i = 0; i < 8; ++i)
			{
				const int32 j = (i + 1) % 8;
				const double A = 2.0 * Pi * (i + 1.0) / 8;
				Poly(M, {Lo[i], Lo[j], Hi[j], Hi[i]}, FVector(FMath::Cos(A), FMath::Sin(A), 0));
			}
			Poly(M, Lo, FVector(0, 0, -1));
			Poly(M, Hi, FVector(0, 0, 1));
		};
		for (const FColumnSpec& C : ColumnSpecs()) { Prism(C.Base, 0.24, 0.0, 3.0); }
		Prism(FVector(0, kTY, 0), 1.15, 0.0, kPedestalTop);
		auto Block = [&M](const FPlinthSpec& P)
		{
			Box(M, P.Base + FVector(0, 0, P.Height * 0.5), P.Axis, FVector::CrossProduct(FVector::UpVector, P.Axis), FVector::UpVector,
				FVector(P.HalfA + 0.03, P.HalfB + 0.03, P.Height * 0.5));
		};
		for (const FPlinthSpec& P : AxisPlinths()) { Block(P); }
		for (const FPlinthSpec& P : AmbulatoryPlinths()) { Block(P); }
		for (const FVector& B : HermBases()) { Box(M, B + FVector(0, 0, 0.62), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FVector(0.2, 0.2, 0.62)); }
		auto Guard = [&M](const FNiche& N)
		{
			Box(M, N.Centre + N.In * 0.03 + FVector(0, 0, N.Sill + 0.5), N.Lateral, N.In, FVector::UpVector, FVector(N.R, 0.02, 0.5));
		};
		for (const double Sgn : {-1.0, 1.0})
		{
			for (const FNiche& N : GalleryNiches(Sgn)) { Guard(N); }
		}
		for (const FNiche& N : TribuneNiches()) { Guard(N); }
	}

	/** The plinths, herms and pedestal for the works (the niches' plinths and the consoles are built with them). */
	void WorksFittings(FHall& Hall)
	{
		FMesh& C = Hall[EPart::Carved];
		Pedestal(Hall);
		for (const FPlinthSpec& P : AxisPlinths()) { Plinth(C, P.Base, P.Axis, P.HalfA, P.HalfB, P.Height); }
		for (const FPlinthSpec& P : AmbulatoryPlinths()) { Plinth(C, P.Base, P.Axis, P.HalfA, P.HalfB, P.Height); }
		for (const FVector& B : HermBases()) { Herm(C, B, FVector(0, -1, 0)); }
	}

	/** Compass names for the tribune's niches (plan angles from east towards south). */
	const TCHAR* Compass(double Degrees)
	{
		const int32 D = FMath::RoundToInt(Degrees) % 360;
		switch (D)
		{
		case 30: return TEXT("SE");
		case 150: return TEXT("SW");
		case 210: return TEXT("NW");
		case 270: return TEXT("N");
		case 330: return TEXT("NE");
		case 0: return TEXT("E");
		case 180: return TEXT("W");
		default: return TEXT("X");
		}
	}

	/** The gallery vault's height (the coffers' ribs) above a point x across the gallery. */
	double VaultAt(double X) { return kGZ + FMath::Sqrt(kGR * kGR - X * X); }

	/** The annular vault's lower face at radius R. */
	double AnnularAt(double R)
	{
		const FAnnular AV;
		const double C = (R - AV.RM) / AV.RA;
		return AV.HM + AV.RA * FMath::Sqrt(FMath::Max(0.0, 1.0 - C * C));
	}

	double DomeAt(double R)
	{
		const FDome DM;
		return DM.ZC + FMath::Sqrt(DM.RS * DM.RS - R * R);
	}

	TArray<ClassicalHallGeometry::FSpot> SpotsLocal()
	{
		using ClassicalHallGeometry::FSpot;
		TArray<FSpot> Out;
		const FVector Up(0, 0, 1);
		auto Add = [&Out](const FString& Name, const TCHAR* Kind, const FVector& At, const FVector& Facing, double Height, double Width, const TCHAR* Who)
		{
			FSpot S;
			S.Name = Name;
			S.Kind = Kind;
			S.Location = At;
			S.Facing = Unit(Facing);
			S.Height = Height;
			S.MaxWidth = Width;
			S.Suggested = Who;
			Out.Add(S);
		};
		// The gallery's niches and busts, east wall then west, south to north.
		for (const double Sgn : {1.0, -1.0})
		{
			const TCHAR* Wall = Sgn > 0 ? TEXT("E") : TEXT("W");
			const TArray<FNiche> Niches = GalleryNiches(Sgn);
			for (int32 k = 0; k < Niches.Num(); ++k)
			{
				const FNiche& N = Niches[k];
				const TCHAR* Who = TEXT("");
				if (Sgn > 0 && k == 0) { Who = TEXT("Augustus of Prima Porta"); }
				if (Sgn < 0 && k == 0) { Who = TEXT("Doryphoros"); }
				if (Sgn > 0 && k == 1) { Who = TEXT("Capitoline Venus"); }
				Add(FString::Printf(TEXT("Gallery.Niche.%s%d"), Wall, k + 1), TEXT("Niche"), NichePlinthCentre(N) + Up * (N.Sill + NichePlinthHeight(N)), -N.In,
					2.25, 1.2, Who);
			}
			for (int32 k = 0; k < kBays; ++k)
			{
				for (const double Off : {1.6, -1.6})
				{
					const FVector At(Sgn * (kHW - 0.15), BayCentre(k) + Off, 1.55);
					Add(FString::Printf(TEXT("Gallery.Bust.%s%d%s"), Wall, k + 1, Off > 0 ? TEXT("S") : TEXT("N")), TEXT("Bust"), At, FVector(-Sgn, 0, 0), 0.8, 0.5,
						TEXT(""));
				}
			}
		}
		// The herms by the door.
		{
			const TArray<FVector> H = HermBases();
			Add(TEXT("Gallery.Herm.W"), TEXT("Herm"), H[0] + Up * (1.17 - kSink + 0.09), FVector(0, -1, 0), 0.75, 0.45, TEXT("Homer"));
			Add(TEXT("Gallery.Herm.E"), TEXT("Herm"), H[1] + Up * (1.17 - kSink + 0.09), FVector(0, -1, 0), 0.75, 0.45, TEXT(""));
		}
		// The reclining figures on the gallery's axis (their plinths run north-south).
		{
			const TArray<FPlinthSpec> P = AxisPlinths();
			Add(TEXT("Gallery.Plinth.2"), TEXT("Reclining"), P[0].Base + Up * P[0].Height, FVector(1, 0, 0), 1.2, 2.2, TEXT("Sleeping Ariadne"));
			Add(TEXT("Gallery.Plinth.3"), TEXT("Reclining"), P[1].Base + Up * P[1].Height, FVector(-1, 0, 0), 1.2, 2.2, TEXT("Dying Gaul"));
		}
		// The tribune: its niches, and a bust either side of each.
		for (const FNiche& N : TribuneNiches())
		{
			const double Deg = FMath::RadiansToDegrees(N.Phi);
			const TCHAR* Who = TEXT("");
			if (FMath::Abs(Deg - 270.0) < 1) { Who = TEXT("Apollo Belvedere"); }
			if (FMath::Abs(Deg - 330.0) < 1) { Who = TEXT("Laocoon and His Sons"); }
			if (FMath::Abs(Deg - 210.0) < 1) { Who = TEXT("Venus de Milo"); }
			if (FMath::Abs(Deg - 390.0) < 1) { Who = TEXT("Barberini Faun"); }
			Add(FString::Printf(TEXT("Tribune.Niche.%s"), Compass(Deg)), TEXT("Niche"), NichePlinthCentre(N) + Up * (N.Sill + NichePlinthHeight(N)), -N.In, 2.3,
				1.55, Who);
		}
		for (const FNiche& N : TribuneNiches())
		{
			for (const double Off : {-1.0, 1.0})
			{
				const double A = N.Phi + Off * Rad(15.0);
				Add(FString::Printf(TEXT("Tribune.Bust.%s%s"), Compass(FMath::RadiansToDegrees(N.Phi)), Off < 0 ? TEXT("a") : TEXT("b")), TEXT("Bust"),
					TP(A, kRo - 0.17, 1.55), -Radial(A), 0.8, 0.5, TEXT(""));
			}
		}
		// The works in the round in the ambulatory: east and west.
		{
			const TArray<FPlinthSpec> P = AmbulatoryPlinths();
			Add(TEXT("Tribune.Plinth.E"), TEXT("Freestanding"), P[0].Base + Up * P[0].Height, -P[0].Axis, 2.3, 2.4, TEXT("Discobolus"));
			Add(TEXT("Tribune.Plinth.W"), TEXT("Freestanding"), P[1].Base + Up * P[1].Height, -P[1].Axis, 2.3, 2.4,
				TEXT("Artemision Bronze (its arms along the ambulatory)"));
		}
		for (FSpot& S : Out) { S.Location.Z += CH::FloorZ; }
		return Out;
	}

	ClassicalHallGeometry::FSpot CentreLocal()
	{
		ClassicalHallGeometry::FSpot S;
		S.Name = TEXT("Tribune.Centrepiece");
		S.Kind = TEXT("Centrepiece");
		S.Location = FVector(0, kTY, CH::FloorZ + kPedestalTop);
		S.Facing = FVector(0, 1, 0);   // towards the gallery and the stair
		S.Height = 2.4;
		S.MaxWidth = 2.2;
		S.Suggested = TEXT("Belvedere Torso (so it stands in the Sala delle Muse), or a porphyry basin up to 2.2 m across");
		return S;
	}

	// ---------------------------------------------------------------------------------------------- the lights

	/** Luminance of the laylights' diffusers (cd/m²): daylight through a diffuser; the rect lights behind them match. */
	constexpr double kLaylightNits = 1300.0;
	/** The coves: lumens per metre washing the vaults from the cornices' ledges. */
	constexpr double kCoveLumensPerMetre = 1600.0;
	/** Accents: illuminance on the works (lux), over the rooms' general light. The centrepiece has three spots. */
	constexpr double kStatueLux = 450.0, kBustLux = 350.0, kCentreLux = 300.0;
	constexpr double kKelvin = 3800.0;
	/** The tribune's wall-washers (candela each): about 150 lux on the outer wall. */
	constexpr double kWasherCandela = 1400.0;

	TArray<ClassicalHallGeometry::FLight> LightsLocal()
	{
		using ClassicalHallGeometry::FLight;
		TArray<FLight> Out;
		const FVector Up(0, 0, 1);
		const FVector Floor(0, 0, CH::FloorZ);
		auto Rect = [&Out, &Floor](const FString& Name, const FVector& At, const FVector& Dir, const FVector& Across, double W, double H, double Candela, bool bLay)
		{
			FLight L;
			L.Type = FLight::EType::Rect;
			L.Name = Name;
			L.Location = At + Floor;
			L.Direction = Unit(Dir);
			L.Across = Unit(Across);
			L.Width = W;
			L.Height = H;
			L.Candela = Candela;
			L.Kelvin = kKelvin;
			L.bLaylight = bLay;
			L.Range = bLay ? 16.0 : 9.0;
			Out.Add(L);
		};
		// Laylights: the gallery's four lanterns and the tribune's eye.
		for (int32 k = 0; k < kBays; ++k)
		{
			const double HalfX = ArcX(1.2 - 0.06), HalfY = 1.2 - 0.06;
			Rect(FString::Printf(TEXT("Laylight%d"), k + 1), FVector(0, BayCentre(k), kLaylight - 0.02), -Up, FVector(1, 0, 0), 2 * HalfX, 2 * HalfY,
				 kLaylightNits * 4 * HalfX * HalfY, true);
		}
		{
			const double Side = FMath::Sqrt(Pi) * kOculus;
			Rect(TEXT("LaylightEye"), FVector(0, kTY, kLaylight - 0.02), -Up, FVector(1, 0, 0), Side, Side, kLaylightNits * Pi * kOculus * kOculus, true);
		}
		// The gallery's coves: each wall's runs between the ressauts, the light on the ledge behind the lip.
		for (const double Sgn : {-1.0, 1.0})
		{
			TArray<FVector2D> Runs;   // (south y, north y)
			double South = kYS;
			for (int32 k = 1; k < kBays; ++k)
			{
				Runs.Add(FVector2D(South, PairCentre(k) + kRessautHalf));
				South = PairCentre(k) - kRessautHalf;
			}
			Runs.Add(FVector2D(South, kYN));
			for (int32 i = 0; i < Runs.Num(); ++i)
			{
				const double L = Runs[i].X - Runs[i].Y - 0.1;
				Rect(FString::Printf(TEXT("Cove%s%d"), Sgn > 0 ? TEXT("E") : TEXT("W"), i + 1), FVector(Sgn * (kHW - 0.13), (Runs[i].X + Runs[i].Y) * 0.5, kLedge + 0.03),
					 FVector(-Sgn * 0.45, 0, 0.9), FVector(0, 1, 0), L, 0.04, kCoveLumensPerMetre * L / Pi, false);
			}
		}
		// The tribune's coves: round the outer wall (washing the annular vault), and both sides of the ring beam.
		for (int32 k = 0; k < 18; ++k)
		{
			const double A = Rad(10.0 + 20.0 * k), R = kRo - 0.13;
			const double L = 2 * R * FMath::Sin(Rad(10.0)) - 0.05;
			Rect(FString::Printf(TEXT("CoveOuter%02d"), k + 1), TP(A, R, kLedge + 0.03), -Radial(A) * 0.45 + Up * 0.9, FVector(-FMath::Sin(A), FMath::Cos(A), 0), L, 0.04,
				 kCoveLumensPerMetre * L / Pi, false);
		}
		for (int32 k = 0; k < 12; ++k)
		{
			const double A = Rad(30.0 * k);
			const double RO = 3.7, RI = 3.1;
			const double LO = 2 * RO * FMath::Sin(Rad(15.0)) - 0.05, LI = 2 * RI * FMath::Sin(Rad(15.0)) - 0.05;
			const FVector T(-FMath::Sin(A), FMath::Cos(A), 0);
			Rect(FString::Printf(TEXT("CoveRingOut%02d"), k + 1), TP(A, RO, kLedge + 0.03), Radial(A) * 0.45 + Up * 0.9, T, LO, 0.04, kCoveLumensPerMetre * LO / Pi, false);
			Rect(FString::Printf(TEXT("CoveRingIn%02d"), k + 1), TP(A, RI, kLedge + 0.03), -Radial(A) * 0.45 + Up * 0.9, T, LI, 0.04, kCoveLumensPerMetre * LI / Pi, false);
		}
		// Wall-washers for the tribune's outer wall (its daylight is mostly the dome's, which the ring beam shades): on the
		// ring beam's top, just over its outer lip, aimed out between the columns and down at the wall.
		for (int32 k = 0; k < 12; ++k)
		{
			const double A = Rad(30.0 * k);
			const FVector Dir = Radial(A) * FMath::Cos(Rad(33.0)) - Up * FMath::Sin(Rad(33.0));
			Rect(FString::Printf(TEXT("WashOuter%02d"), k + 1), TP(A, kRc + 0.38, kLedge + 0.16), Dir, FVector(-FMath::Sin(A), FMath::Cos(A), 0), 1.4, 0.05,
				 kWasherCandela, false);
			Out.Last().BarnDoorAngle = 50.0;
			Out.Last().BarnDoorLength = 0.2;
			Out.Last().Range = 7.0;
		}
		// Accents: a concealed spot for each place, from a coffer across the room, 30-55° down.
		auto Spot = [&Out, &Floor](const FString& Name, const FVector& From, const FVector& Target, double Radius, double Lux)
		{
			FLight L;
			L.Type = FLight::EType::Spot;
			L.Name = Name;
			L.Location = From + Floor;
			const FVector D = Target - From;
			const double Dist = D.Size();
			L.Direction = Unit(D);
			L.OuterCone = FMath::RadiansToDegrees(FMath::Atan2(Radius, Dist)) + 4.0;
			L.InnerCone = L.OuterCone * 0.4;
			L.Candela = Lux * Dist * Dist / 0.85;
			L.Range = Dist + 3.0;
			L.SourceRadius = 0.03;
			L.Kelvin = kKelvin;
			Out.Add(L);
		};
		TArray<ClassicalHallGeometry::FSpot> Places = SpotsLocal();
		Places.Add(CentreLocal());
		for (ClassicalHallGeometry::FSpot S : Places)
		{
			S.Location.Z -= CH::FloorZ;   // back to heights above the floor
			const FString Name = TEXT("Accent.") + S.Name;
			const bool bSmall = S.Kind == TEXT("Bust") || S.Kind == TEXT("Herm");
			if (S.Kind == TEXT("Centrepiece"))
			{
				// Three spots from the dome's lowest coffers, 120° apart.
				for (int32 k = 0; k < 3; ++k)
				{
					const double A = Rad(90.0 + 120.0 * k), R = 2.3;
					Spot(FString::Printf(TEXT("%s.%d"), *Name, k + 1), TP(A, R, DomeAt(R) - 0.06), S.Location + Up * 0.9, 1.3, kCentreLux);
				}
			}
			else if (S.Name.StartsWith(TEXT("Gallery.")))
			{
				// From the vault across the gallery (a coffer beside the lantern), a little along the gallery.
				if (S.Kind == TEXT("Reclining"))
				{
					for (const double Sx : {-1.0, 1.0})
					{
						const FVector From(Sx * 1.45, S.Location.Y - Sx * 0.9, VaultAt(1.45) - 0.06);
						Spot(Name + (Sx < 0 ? TEXT(".W") : TEXT(".E")), From, S.Location + Up * 0.45, 1.4, kStatueLux * 0.7);
					}
					continue;
				}
				const double Side = S.Location.X > 0 ? 1.0 : -1.0;
				const double Along = S.Kind == TEXT("Herm") ? -1.3 : 0.9;
				const FVector From(-Side * 1.45, S.Location.Y + Along, VaultAt(1.45) - 0.06);
				const FVector Target = S.Location + Up * (bSmall ? 0.35 : 1.1) + S.Facing * 0.1;
				Spot(Name, From, Target, bSmall ? 0.45 : 1.3, bSmall ? kBustLux : kStatueLux);
			}
			else
			{
				// From the annular vault's inner slope, swung aside so the light rakes.
				const double A = FMath::Atan2(S.Location.Y - kTY, S.Location.X);
				if (S.Kind == TEXT("Freestanding"))
				{
					for (const double Sw : {-1.0, 1.0})
					{
						const double R = 4.25, B = A + Sw * Rad(28.0);
						Spot(Name + (Sw < 0 ? TEXT(".a") : TEXT(".b")), TP(B, R, AnnularAt(R) - 0.06), S.Location + Up * 1.1, 1.5, kStatueLux * 0.7);
					}
					continue;
				}
				const double R = bSmall ? 4.4 : 4.2, B = A + Rad(bSmall ? 7.0 : 11.0);
				const FVector Target = S.Location + Up * (bSmall ? 0.35 : 1.15) + S.Facing * 0.15;
				Spot(Name, TP(B, R, AnnularAt(R) - 0.06), Target, bSmall ? 0.45 : 1.3, bSmall ? kBustLux : kStatueLux);
			}
		}
		return Out;
	}

	/** Everything, in plan metres, lifted to the hall's floor. */
	FHall BuildAll()
	{
		FHall Hall;
		// Walls.
		const TArray<double> LongSt = LongStations();
		LongWall(Hall, 1.0, LongSt);
		LongWall(Hall, -1.0, LongSt);
		FWallFace South = SouthWallFace();
		South.Build(Hall[EPart::Dado], Hall[EPart::Wall]);
		FWallFace Lip = LipFace();
		FMesh LipMesh;
		Lip.Build(LipMesh, LipMesh);
		Hall[EPart::Shell].Append(LipMesh);
		Passage(Hall, Lip, South);
		FWallFace Tribune = TribuneWall();
		Tribune.Build(Hall[EPart::Dado], Hall[EPart::Wall]);
		{
			const TArray<FNiche> Niches = TribuneNiches();
			for (int32 k = 0; k < Niches.Num(); ++k) { BuildNiche(Hall, Niches[k], Tribune.Openings[k], Tribune.Stations); }
		}
		Lintel(Hall, LongSt, Tribune);
		// Vaults.
		GalleryVault(Hall, LongSt, South.Stations, NorthStations());
		TArray<double> Angles;
		for (const double U : Tribune.Stations) { Angles.Add(U / kRo); }
		AnnularVault(Hall, Angles);
		Dome(Hall);
		RingBeam(Hall);
		// Floors and the box.
		Floors(Hall, LongSt, South.Stations, Tribune);
		Shell(Hall, Lip);
		// The order and the fittings.
		Columns(Hall);
		Pilasters(Hall);
		GalleryEntablature(Hall);
		TribuneEntablature(Hall);
		WallMouldings(Hall);
		DoorFrame(Hall);
		// A panel in the lunette over the screen, on the gallery's axis.
		FramedPanel(Hall, FVector(0, kYN, 0), FVector(0, 1, 0), FVector(-1, 0, 0), 1.3, 3.98, 4.78, 0.0);
		WorksFittings(Hall);
		Blockers(Hall);
		for (const double Sgn : {-1.0, 1.0})
		{
			for (const FNiche& N : GalleryNiches(Sgn)) { NicheFittings(Hall, N); }
			for (int32 k = 0; k < kBays; ++k)
			{
				for (const double Off : {-1.6, 1.6})
				{
					PanelAndConsole(Hall, FVector(Sgn * kHW, BayCentre(k) + Off, 0), FVector(-Sgn, 0, 0), FVector(0, 1, 0), 0.0);
				}
			}
		}
		for (const FNiche& N : TribuneNiches())
		{
			NicheFittings(Hall, N);
			for (const double Off : {-1.0, 1.0})
			{
				const double A = N.Phi + Off * Rad(15.0);
				const double Sag = 0.0;
				PanelAndConsole(Hall, TP(A, kRo, 0), -Radial(A), FVector(-FMath::Sin(A), FMath::Cos(A), 0), Sag + 0.02);
			}
		}
		// Lift everything to the hall's floor.
		for (FMesh& M : Hall.Parts)
		{
			for (FVector& P : M.Positions) { P.Z += CH::FloorZ; }
		}
		return Hall;
	}
}

namespace ClassicalHallGeometry
{
	FHall Build() { return ClassicalHallBuild::BuildAll(); }
	TArray<FSpot> StatueSpots() { return ClassicalHallBuild::SpotsLocal(); }
	FSpot Centrepiece() { return ClassicalHallBuild::CentreLocal(); }
	TArray<FLight> Lights() { return ClassicalHallBuild::LightsLocal(); }
	double LaylightNits() { return ClassicalHallBuild::kLaylightNits; }
}
