#include "ElanExterior/ElanExteriorStructure.h"

#include "MuseeVision.h"
#include "Plan/MuseePlan.h"
#include "Geometry/MuseeMesh.h"
#include "Geometry/MuseeBake.h"
#include "Algo/Reverse.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "ProceduralMeshComponent.h"

/**
 * The exterior's geometry kit. A named namespace (the module builds in unity files). Positions in
 * metres in the actor's frame: the Élan's axis at the origin, x east, y south, z up from the ground.
 */
namespace ElanExteriorKit
{
	namespace E = MuseePlan::Elan;
	namespace X = MuseePlan::ElanExterior;

	constexpr double Turn = 2.0 * UE_DOUBLE_PI;
	constexpr double Deg = UE_DOUBLE_PI / 180.0;
	constexpr int32 Round = 192;                       // segments round the drum, as the interior's
	constexpr double Equator = E::SphereCentreHeight;  // 22
	constexpr double Ground = -0.10;                   // every solid starts 10 cm under the ground
	constexpr double Back = 0.02;                      // how far a moulding's back runs into what it stands on

	// The plinth's coping is cut back round the portal (±22° about west), where the portal's body takes it.
	constexpr double PortalCutDegrees = 22.0;
	// Anything within this of west stands on the portal's top, not on the plinth.
	constexpr double PortalBayDegrees = 25.0;
	constexpr double PortalEastRadius = 14.05;         // the portal's east face, just outside the drum's skin
	constexpr double HallVaultCentre = 2.2;            // the Hall of Light's vault: R 5.3 about h 2.2

	// The dome's lattice: members 8 cm wide, 13 cm deep, their centre line at NodeR.
	constexpr double MemberWidth = 0.08, MemberDepth = 0.13;
	constexpr double NodeR = X::LatticeInner + MemberDepth / 2;
	constexpr int32 LowerNodes = 48, UpperNodes = 24;  // 7.5° apart to r 7.15 m (polar 30°), then 15°
	constexpr int32 LowerRows = 20, UpperRows = 7;     // the last row lies under the crown ring
	constexpr double TransitionPolar = 30.0;           // degrees from the top
	constexpr double MeridianTopR = 3.2;               // the ribs run into the crown ring
	constexpr double CrownOuter = 3.35;
	constexpr double CrownSeat = 14.28;                // the crown's underside, on the pane's sphere + 3 cm

	// The lantern: glass Ø 4.4 m from the crown ring to h 38.7, twelve colonnettes, an entablature, a cap.
	constexpr double LanternR = 2.20, LanternGlassTop = 38.70, LanternEntablature = 38.68, LanternCapBase = 2.48;

	constexpr double FirstMeridian = -90.0;            // north; meridians every 15°, fins at −75° + 30°k

	/** A 2D outline (a lathe profile, a moulding's section, a prism's face) whose points are smooth or sharp. */
	struct FOutline
	{
		TArray<FVector2D> P;
		TArray<bool> Smooth;

		void Add(double A, double B, bool bSmooth = false) { P.Add(FVector2D(A, B)); Smooth.Add(bSmooth); }

		/** An arc about (CA, CB) with radii (RA, RB) from angle From to To (degrees), after the point already at From. */
		void Arc(double CA, double CB, double RA, double RB, double From, double To, int32 Steps)
		{
			for (int32 i = 1; i <= Steps; ++i)
			{
				const double T = (From + (To - From) * i / Steps) * Deg;
				Add(CA + RA * FMath::Cos(T), CB + RB * FMath::Sin(T), i < Steps);
			}
		}

		int32 Num() const { return P.Num(); }

		double Area() const
		{
			double A = 0;
			for (int32 i = 0; i < P.Num(); ++i)
			{
				const FVector2D& U = P[i];
				const FVector2D& V = P[(i + 1) % P.Num()];
				A += U.X * V.Y - V.X * U.Y;
			}
			return 0.5 * A;
		}

		/** Closed outlines run counter-clockwise, so each edge's outward normal is to the right of travel. */
		void MakeCounterClockwise()
		{
			if (Area() < 0)
			{
				Algo::Reverse(P);
				Algo::Reverse(Smooth);
			}
		}
	};

	FVector2D EdgeNormal(const FOutline& O, int32 K)
	{
		const FVector2D D = O.P[(K + 1) % O.Num()] - O.P[K];
		return FVector2D(D.Y, -D.X).GetSafeNormal();
	}

	/** Point K's normal on edge Edge: the edge's own, or the mean of its two edges where K is smooth. */
	FVector2D PointNormal(const FOutline& O, int32 K, int32 Edge, bool bClosed)
	{
		if (!O.Smooth[K]) { return EdgeNormal(O, Edge); }
		const int32 N = O.Num(), Edges = bClosed ? N : N - 1;
		const int32 Prev = K > 0 ? K - 1 : (bClosed ? N - 1 : -1);
		const int32 Next = K < Edges ? K : -1;
		FVector2D Sum(0, 0);
		if (Prev >= 0) { Sum += EdgeNormal(O, Prev); }
		if (Next >= 0) { Sum += EdgeNormal(O, Next); }
		return Sum.GetSafeNormal();
	}

	TArray<double> Lengths(const TArray<FVector2D>& P, bool bClosed)
	{
		TArray<double> S = {0.0};
		for (int32 i = 1; i < P.Num(); ++i) { S.Add(S.Last() + FVector2D::Distance(P[i - 1], P[i])); }
		if (bClosed) { S.Add(S.Last() + FVector2D::Distance(P.Last(), P[0])); }
		return S;
	}

	/** Ear clipping of a simple polygon (either winding): index triples. */
	TArray<int32> Triangulate(const TArray<FVector2D>& Poly)
	{
		TArray<int32> Out;
		const int32 N = Poly.Num();
		if (N < 3) { return Out; }
		TArray<int32> V;
		for (int32 i = 0; i < N; ++i) { V.Add(i); }
		double Area = 0;
		for (int32 i = 0; i < N; ++i) { Area += Poly[i].X * Poly[(i + 1) % N].Y - Poly[(i + 1) % N].X * Poly[i].Y; }
		if (Area < 0) { Algo::Reverse(V); }
		auto Cross = [](const FVector2D& A, const FVector2D& B, const FVector2D& C) { return (B.X - A.X) * (C.Y - A.Y) - (B.Y - A.Y) * (C.X - A.X); };
		int32 Guard = 0;
		while (V.Num() > 3 && Guard++ < 100000)
		{
			bool bClipped = false;
			for (int32 i = 0; i < V.Num(); ++i)
			{
				const int32 I0 = V[(i + V.Num() - 1) % V.Num()], I1 = V[i], I2 = V[(i + 1) % V.Num()];
				const FVector2D &A = Poly[I0], &B = Poly[I1], &C = Poly[I2];
				if (Cross(A, B, C) <= 1e-12) { continue; }   // reflex or flat: not an ear
				bool bBlocked = false;
				for (const int32 J : V)
				{
					if (J == I0 || J == I1 || J == I2) { continue; }
					const FVector2D& Q = Poly[J];
					if (Cross(A, B, Q) > 1e-12 && Cross(B, C, Q) > 1e-12 && Cross(C, A, Q) > 1e-12) { bBlocked = true; break; }
				}
				if (bBlocked) { continue; }
				Out.Append({I0, I1, I2});
				V.RemoveAt(i);
				bClipped = true;
				break;
			}
			if (!bClipped)
			{
				// Only flat corners left: drop one (its triangle has no area).
				V.RemoveAt(0);
			}
		}
		if (V.Num() == 3) { Out.Append({V[0], V[1], V[2]}); }
		return Out;
	}

	/**
	 * Turn an outline (r, z) about the vertical axis from angle A0 to A1 (radians). A closed outline makes a
	 * solid (capped where the turn isn't whole); an open one a surface, facing right of travel. Points on the
	 * axis close the surface there in triangles. UVs in metres: round the circle at each point's radius, and
	 * along the outline.
	 */
	void Lathe(FMuseeMesh& M, FOutline O, bool bClosed, double A0, double A1, int32 Segments)
	{
		if (bClosed) { O.MakeCounterClockwise(); }
		const int32 N = O.Num(), Edges = bClosed ? N : N - 1;
		const TArray<double> S = Lengths(O.P, bClosed);
		auto Pos = [](const FVector2D& Q, double T) { return FVector(Q.X * FMath::Cos(T), Q.X * FMath::Sin(T), Q.Y); };
		auto UV = [](const FVector2D& Q, double T, double V) { return FVector2D(Q.X * T, V); };
		for (int32 K = 0; K < Edges; ++K)
		{
			const int32 K1 = (K + 1) % N;
			const FVector2D A = O.P[K], B = O.P[K1];
			if (A.X < 1e-9 && B.X < 1e-9) { continue; }   // along the axis
			const FVector2D NA = PointNormal(O, K, K, bClosed), NB = PointNormal(O, K1, K, bClosed);
			const double VA = S[K], VB = S[K + 1];
			for (int32 i = 0; i < Segments; ++i)
			{
				const double T0 = A0 + (A1 - A0) * i / Segments, T1 = A0 + (A1 - A0) * (i + 1) / Segments;
				if (A.X < 1e-9)
				{
					M.Tri(Pos(A, 0.5 * (T0 + T1)), Pos(B, T1), Pos(B, T0), Pos(NA, 0.5 * (T0 + T1)), Pos(NB, T1), Pos(NB, T0),
						  UV(A, T0, VA), UV(B, T1, VB), UV(B, T0, VB));
				}
				else if (B.X < 1e-9)
				{
					M.Tri(Pos(A, T0), Pos(A, T1), Pos(B, 0.5 * (T0 + T1)), Pos(NA, T0), Pos(NA, T1), Pos(NB, 0.5 * (T0 + T1)),
						  UV(A, T0, VA), UV(A, T1, VA), UV(B, T0, VB));
				}
				else
				{
					M.Quad(Pos(A, T0), Pos(A, T1), Pos(B, T1), Pos(B, T0), Pos(NA, T0), Pos(NA, T1), Pos(NB, T1), Pos(NB, T0),
						   UV(A, T0, VA), UV(A, T1, VA), UV(B, T1, VB), UV(B, T0, VB));
				}
			}
		}
		if (bClosed && FMath::Abs(A1 - A0) < Turn - 1e-6)
		{
			const TArray<int32> Tris = Triangulate(O.P);
			for (const double T : {A0, A1})
			{
				const FVector Along(-FMath::Sin(T), FMath::Cos(T), 0);
				const FVector Out = (T == A0) == (A1 > A0) ? -Along : Along;
				for (int32 i = 0; i + 2 < Tris.Num(); i += 3)
				{
					const FVector2D &P0 = O.P[Tris[i]], &P1 = O.P[Tris[i + 1]], &P2 = O.P[Tris[i + 2]];
					M.Tri(Pos(P0, T), Pos(P1, T), Pos(P2, T), Out, Out, Out, P0, P1, P2);
				}
			}
		}
	}

	/** A prism: a closed outline in the plane (U, V) through Origin, from W0 to W1 along W. UVs in metres. */
	void Prism(FMuseeMesh& M, FOutline O, const FVector& Origin, const FVector& U, const FVector& V, const FVector& W, double W0, double W1)
	{
		O.MakeCounterClockwise();
		const int32 N = O.Num();
		const TArray<double> S = Lengths(O.P, true);
		auto P3 = [&](const FVector2D& Q, double Depth) { return Origin + U * Q.X + V * Q.Y + W * Depth; };
		auto N3 = [&](const FVector2D& Q) { return U * Q.X + V * Q.Y; };
		for (int32 K = 0; K < N; ++K)
		{
			const int32 K1 = (K + 1) % N;
			const FVector2D A = O.P[K], B = O.P[K1];
			const FVector NA = N3(PointNormal(O, K, K, true)), NB = N3(PointNormal(O, K1, K, true));
			M.Quad(P3(A, W0), P3(B, W0), P3(B, W1), P3(A, W1), NA, NB, NB, NA,
				   FVector2D(S[K], W0), FVector2D(S[K + 1], W0), FVector2D(S[K + 1], W1), FVector2D(S[K], W1));
		}
		const TArray<int32> Tris = Triangulate(O.P);
		for (const double Depth : {W0, W1})
		{
			const FVector Out = Depth == W0 ? -W : W;
			for (int32 i = 0; i + 2 < Tris.Num(); i += 3)
			{
				const FVector2D &P0 = O.P[Tris[i]], &P1 = O.P[Tris[i + 1]], &P2 = O.P[Tris[i + 2]];
				M.Tri(P3(P0, Depth), P3(P1, Depth), P3(P2, Depth), Out, Out, Out, P0, P1, P2);
			}
		}
	}

	/**
	 * Sweep a closed outline (a, b) along a path: each point Path[i] + a·A[i] + b·B[i], with A and B already
	 * mitred at the path's corners. Normals from the unit frames: each segment's own at a sharp corner, the
	 * point's where the path is smooth. Capped at both ends. UVs in metres (along the path, round the outline).
	 */
	void Sweep(FMuseeMesh& M, FOutline O, const TArray<FVector>& Path, const TArray<FVector>& A, const TArray<FVector>& B, const TArray<bool>& PathSmooth)
	{
		O.MakeCounterClockwise();
		const int32 N = O.Num(), L = Path.Num();
		const TArray<double> S = Lengths(O.P, true);
		TArray<double> Along = {0.0};
		for (int32 i = 1; i < L; ++i) { Along.Add(Along.Last() + FVector::Distance(Path[i - 1], Path[i])); }
		auto At = [&](int32 i, const FVector2D& Q) { return Path[i] + A[i] * Q.X + B[i] * Q.Y; };
		for (int32 i = 0; i + 1 < L; ++i)
		{
			const FVector Dir = (Path[i + 1] - Path[i]).GetSafeNormal();
			auto Frame = [&](int32 j, FVector& FA, FVector& FB)
			{
				if (PathSmooth[j]) { FA = A[j].GetSafeNormal(); FB = B[j].GetSafeNormal(); }
				else
				{
					FA = (A[j] - Dir * FVector::DotProduct(A[j], Dir)).GetSafeNormal();
					FB = (B[j] - Dir * FVector::DotProduct(B[j], Dir)).GetSafeNormal();
				}
			};
			FVector A0, B0, A1, B1;
			Frame(i, A0, B0);
			Frame(i + 1, A1, B1);
			for (int32 K = 0; K < N; ++K)
			{
				const int32 K1 = (K + 1) % N;
				const FVector2D P = O.P[K], Q = O.P[K1];
				const FVector2D NP = PointNormal(O, K, K, true), NQ = PointNormal(O, K1, K, true);
				M.Quad(At(i, P), At(i + 1, P), At(i + 1, Q), At(i, Q),
					   A0 * NP.X + B0 * NP.Y, A1 * NP.X + B1 * NP.Y, A1 * NQ.X + B1 * NQ.Y, A0 * NQ.X + B0 * NQ.Y,
					   FVector2D(Along[i], S[K]), FVector2D(Along[i + 1], S[K]), FVector2D(Along[i + 1], S[K + 1]), FVector2D(Along[i], S[K + 1]));
			}
		}
		const TArray<int32> Tris = Triangulate(O.P);
		for (const int32 End : {0, L - 1})
		{
			const FVector Out = End == 0 ? (Path[0] - Path[1]).GetSafeNormal() : (Path[L - 1] - Path[L - 2]).GetSafeNormal();
			for (int32 i = 0; i + 2 < Tris.Num(); i += 3)
			{
				const FVector2D &P0 = O.P[Tris[i]], &P1 = O.P[Tris[i + 1]], &P2 = O.P[Tris[i + 2]];
				M.Tri(At(End, P0), At(End, P1), At(End, P2), Out, Out, Out, P0, P1, P2);
			}
		}
	}

	/** The mitred offsets at a path's points from its segments' unit normals (Seg[i]: from point i to i + 1). */
	TArray<FVector> Mitre(const TArray<FVector>& Seg)
	{
		TArray<FVector> Out = {Seg[0]};
		for (int32 i = 1; i < Seg.Num(); ++i) { Out.Add((Seg[i - 1] + Seg[i]) / (1.0 + FVector::DotProduct(Seg[i - 1], Seg[i]))); }
		Out.Add(Seg.Last());
		return Out;
	}

	/** A straight member from A to B, Width across, its depth from D0 to D1 along Up (made square to the member); past each end by half its width. */
	void Beam(FMuseeMesh& M, const FVector& A, const FVector& B, const FVector& Up, double Width, double D0, double D1)
	{
		const FVector Axis = (B - A).GetSafeNormal();
		const FVector Deep = (Up - Axis * FVector::DotProduct(Up, Axis)).GetSafeNormal();
		const FVector Side = FVector::CrossProduct(Axis, Deep).GetSafeNormal();
		FOutline R;
		R.Add(-Width / 2, D0); R.Add(Width / 2, D0); R.Add(Width / 2, D1); R.Add(-Width / 2, D1);
		const FVector Ext = Axis * (Width / 2);
		Sweep(M, R, {A - Ext, B + Ext}, {Side, Side}, {Deep, Deep}, {false, false});
	}

	/** A point on the sphere about h 22: radius Rs, polar angle Polar from the top, azimuth Phi (radians). */
	FVector OnSphere(double Rs, double Polar, double Phi)
	{
		return FVector(Rs * FMath::Sin(Polar) * FMath::Cos(Phi), Rs * FMath::Sin(Polar) * FMath::Sin(Phi), Equator + Rs * FMath::Cos(Polar));
	}

	FVector Radial(double Theta) { return FVector(FMath::Cos(Theta), FMath::Sin(Theta), 0); }
	FVector Around(double Theta) { return FVector(-FMath::Sin(Theta), FMath::Cos(Theta), 0); }

	/** Degrees between an azimuth (degrees) and west. */
	double FromWest(double Degrees) { return FMath::Abs(FMath::FindDeltaAngleDegrees(Degrees, 180.0)); }

	/** A fin's, a mullion's or the lantern's bearings as azimuths (degrees): north −90, east 0, south 90. */
	double FinAzimuth(int32 K) { return FirstMeridian + 15.0 + 30.0 * K; }
	double MullionAzimuth(int32 K) { return FirstMeridian + 30.0 * K; }

	// ------------------------------------------------------------------------------------------------
	// The plinth and the entablature

	/** The plinth's section (r, z): socle, torus, scotia, the ashlar face, the coping (ovolo, corona, bead, wash). */
	FOutline PlinthSection()
	{
		FOutline O;
		O.Add(14.82, Ground); O.Add(X::PlinthSocle, Ground); O.Add(X::PlinthSocle, 0.40); O.Add(15.26, 0.44); O.Add(15.14, 0.44);
		O.Arc(15.14, 0.54, 0.10, 0.10, -90, 90, 10);          // torus
		O.Add(15.08, 0.64); O.Add(15.08, 0.68);
		O.Arc(15.08, 0.86, 0.18, 0.18, -90, -180, 10);        // scotia
		O.Add(X::PlinthFace, 0.88);
		O.Add(X::PlinthFace, 5.20); O.Add(14.94, 5.20); O.Add(14.94, 5.28);
		O.Arc(14.94, 5.48, 0.20, 0.20, -90, 0, 10);           // ovolo
		O.Add(X::PlinthCorona, 5.48); O.Add(X::PlinthCorona, 5.86); O.Add(15.36, 5.86); O.Add(15.36, 5.89);
		O.Arc(15.24, 5.89, 0.12, 0.12, 0, 90, 8);             // bead
		O.Add(15.20, 6.01); O.Add(15.20, 6.05);
		O.Add(14.06, X::PlinthTop);                           // the wash, up to the drum
		O.Add(14.06, 6.02); O.Add(14.82, 6.02);               // over the imported coping (h 6), 2 cm clear
		return O;
	}

	/** The entablature at the equator (r, z): architrave in two fasciae, taenia, frieze, ovolo, corona, bead, wash. */
	FOutline EntablatureSection()
	{
		FOutline O;
		O.Add(14.06, X::CorniceBottom); O.Add(14.62, X::CorniceBottom); O.Add(14.62, 21.10); O.Add(14.70, 21.10); O.Add(14.70, 21.34);
		O.Add(14.76, 21.34); O.Add(14.76, 21.40); O.Add(14.66, 21.40); O.Add(14.66, 21.84); O.Add(14.70, 21.84);
		O.Arc(14.70, 22.05, 0.21, 0.21, -90, 0, 10);          // ovolo
		O.Add(X::CorniceProjection, 22.05); O.Add(X::CorniceProjection, 22.30); O.Add(15.66, 22.30); O.Add(15.66, 22.33);
		O.Arc(15.52, 22.33, 0.14, 0.14, 0, 90, 8);            // bead
		O.Add(15.48, 22.47); O.Add(15.48, 22.50);
		O.Add(14.06, X::CorniceTop);                          // the wash, up to the Sphere
		return O;
	}

	/** The base moulding's section (a out from the face, z): the plinth's, for the portal. */
	FOutline SocleSection()
	{
		FOutline O;
		O.Add(-Back, Ground); O.Add(0.42, Ground); O.Add(0.42, 0.40); O.Add(0.38, 0.44); O.Add(0.26, 0.44);
		O.Arc(0.26, 0.54, 0.10, 0.10, -90, 90, 10);
		O.Add(0.20, 0.64); O.Add(0.20, 0.68);
		O.Arc(0.20, 0.86, 0.18, 0.18, -90, -180, 10);
		O.Add(0.0, 0.88); O.Add(-Back, 0.86);
		return O;
	}

	/** The portal's cornice (a, z): ovolo, corona, bead; its top flush with the portal's roof. */
	FOutline PortalCorniceSection()
	{
		const double T = X::PortalTop;
		FOutline O;
		O.Add(0, T - 0.75); O.Add(0.05, T - 0.75); O.Add(0.05, T - 0.68);
		O.Arc(0.05, T - 0.50, 0.18, 0.18, -90, 0, 10);
		O.Add(0.45, T - 0.50); O.Add(0.45, T - 0.18); O.Add(0.41, T - 0.18); O.Add(0.41, T - 0.15);
		O.Arc(0.30, T - 0.15, 0.11, 0.11, 0, 90, 8);
		O.Add(0.26, T - 0.04); O.Add(0.26, T); O.Add(0.0, T); O.Add(-Back, T - Back); O.Add(-Back, T - 0.73);
		return O;
	}

	/** The architrave round the portal's opening (a out of the face, b away from the opening): two fasciae and an ovolo. */
	FOutline ArchitraveSection()
	{
		FOutline O;
		O.Add(-Back, 0.03); O.Add(0.06, 0.03); O.Add(0.06, 0.12); O.Add(0.08, 0.12); O.Add(0.08, 0.36);
		O.Arc(0.0, 0.36, 0.08, 0.08, 0, 90, 8);
		O.Add(-Back, 0.44);
		return O;
	}

	/**
	 * The portal: a travertine block from the front (x = PortalFront) back to r 14.05 about the axis, 12.1 m
	 * wide and 9.5 m high, its opening the Hall of Light's section (walls to h 5.22, an arc R 5.8 about h 2.2).
	 */
	void Portal(FMuseeMesh& Body, FMuseeMesh& Mould)
	{
		const double XF = X::PortalFrontX - E::CentreX;
		const double WP = X::PortalHalfWidth, WO = X::PortalClearHalfWidth, RO = X::PortalClearRadius, ZT = X::PortalTop;
		const double ZC = HallVaultCentre, ZB = Ground;
		auto XE = [](double Y) { return -FMath::Sqrt(PortalEastRadius * PortalEastRadius - Y * Y); };
		const double PhiS = FMath::Asin(WO / RO);
		const double ZS = ZC + RO * FMath::Cos(PhiS);
		constexpr int32 ArcSteps = 48, PierSteps = 4;

		TArray<double> ArcPhi;
		TArray<FVector2D> Arc;   // (y, z), north to south
		for (int32 i = 0; i <= ArcSteps; ++i)
		{
			const double Phi = -PhiS + 2 * PhiS * i / ArcSteps;
			ArcPhi.Add(Phi);
			Arc.Add(FVector2D(RO * FMath::Sin(Phi), ZC + RO * FMath::Cos(Phi)));
		}

		// The strips across the roof and the piers' bottoms: the piers in PierSteps, the arc's points between.
		TArray<double> Ys;
		for (int32 i = 0; i <= PierSteps; ++i) { Ys.Add(-WP + (WP - WO) * i / PierSteps); }
		for (int32 i = 1; i < ArcSteps; ++i) { Ys.Add(Arc[i].X); }
		for (int32 i = 0; i <= PierSteps; ++i) { Ys.Add(WO + (WP - WO) * i / PierSteps); }

		// The front, facing west: the outline counter-clockwise in (y, z), through every vertex of the faces
		// it meets (the piers' bottoms, the sides split at the springing, the roof's strips), so no T-joints.
		{
			TArray<FVector2D> Front;
			for (int32 i = 0; i <= PierSteps; ++i) { Front.Add(FVector2D(-WP + (WP - WO) * i / PierSteps, ZB)); }
			Front.Append(Arc);
			for (int32 i = 0; i <= PierSteps; ++i) { Front.Add(FVector2D(WO + (WP - WO) * i / PierSteps, ZB)); }
			Front.Add(FVector2D(WP, ZS));
			for (int32 i = Ys.Num() - 1; i >= 0; --i) { Front.Add(FVector2D(Ys[i], ZT)); }
			Front.Add(FVector2D(-WP, ZS));
			const TArray<int32> Tris = Triangulate(Front);
			const FVector West(-1, 0, 0);
			for (int32 i = 0; i + 2 < Tris.Num(); i += 3)
			{
				const FVector2D &A = Front[Tris[i]], &B = Front[Tris[i + 1]], &C = Front[Tris[i + 2]];
				Body.Tri(FVector(XF, A.X, A.Y), FVector(XF, B.X, B.Y), FVector(XF, C.X, C.Y), West, West, West, A, B, C);
			}
		}

		auto Inward = [&XE](double Y) { return FVector(-XE(Y), -Y, 0) / PortalEastRadius; };
		// The piers: east face, bottom (under the ground), and the outer and inner sides.
		for (const double Sd : {-1.0, 1.0})
		{
			for (int32 i = 0; i < PierSteps; ++i)
			{
				const double Y0 = Sd * (WO + (WP - WO) * i / PierSteps), Y1 = Sd * (WO + (WP - WO) * (i + 1) / PierSteps);
				for (const TPair<double, double> Z : {TPair<double, double>(ZB, ZS), TPair<double, double>(ZS, ZT)})
				{
					Body.Quad(FVector(XE(Y0), Y0, Z.Key), FVector(XE(Y1), Y1, Z.Key), FVector(XE(Y1), Y1, Z.Value), FVector(XE(Y0), Y0, Z.Value),
							  Inward(Y0), Inward(Y1), Inward(Y1), Inward(Y0), FVector2D(Y0, Z.Key), FVector2D(Y1, Z.Key), FVector2D(Y1, Z.Value), FVector2D(Y0, Z.Value));
				}
				Body.Quad(FVector(XF, Y0, ZB), FVector(XE(Y0), Y0, ZB), FVector(XE(Y1), Y1, ZB), FVector(XF, Y1, ZB), FVector(0, 0, -1),
						  FVector(0, 0, -1), FVector(0, 0, -1), FVector(0, 0, -1), FVector2D(XF, Y0), FVector2D(XE(Y0), Y0), FVector2D(XE(Y1), Y1), FVector2D(XF, Y1));
			}
			const double YO = Sd * WP, YI = Sd * WO;
			for (const TPair<double, double> Z : {TPair<double, double>(ZB, ZS), TPair<double, double>(ZS, ZT)})
			{
				Body.Quad(FVector(XF, YO, Z.Key), FVector(XE(YO), YO, Z.Key), FVector(XE(YO), YO, Z.Value), FVector(XF, YO, Z.Value), FVector(0, Sd, 0),
						  FVector(0, Sd, 0), FVector(0, Sd, 0), FVector(0, Sd, 0), FVector2D(XF, Z.Key), FVector2D(XE(YO), Z.Key), FVector2D(XE(YO), Z.Value), FVector2D(XF, Z.Value));
			}
			Body.Quad(FVector(XF, YI, ZB), FVector(XE(YI), YI, ZB), FVector(XE(YI), YI, ZS), FVector(XF, YI, ZS), FVector(0, -Sd, 0),
					  FVector(0, -Sd, 0), FVector(0, -Sd, 0), FVector(0, -Sd, 0), FVector2D(XF, ZB), FVector2D(XE(YI), ZB), FVector2D(XE(YI), ZS), FVector2D(XF, ZS));
		}
		// Over the opening: the soffit (facing the arc's centre) and the east face above it.
		for (int32 i = 0; i < ArcSteps; ++i)
		{
			const FVector2D P0 = Arc[i], P1 = Arc[i + 1];
			const FVector N0(0, -FMath::Sin(ArcPhi[i]), -FMath::Cos(ArcPhi[i])), N1(0, -FMath::Sin(ArcPhi[i + 1]), -FMath::Cos(ArcPhi[i + 1]));
			const double S0 = RO * ArcPhi[i], S1 = RO * ArcPhi[i + 1];
			Body.Quad(FVector(XF, P0.X, P0.Y), FVector(XE(P0.X), P0.X, P0.Y), FVector(XE(P1.X), P1.X, P1.Y), FVector(XF, P1.X, P1.Y),
					  N0, N0, N1, N1, FVector2D(XF, S0), FVector2D(XE(P0.X), S0), FVector2D(XE(P1.X), S1), FVector2D(XF, S1));
			Body.Quad(FVector(XE(P0.X), P0.X, P0.Y), FVector(XE(P1.X), P1.X, P1.Y), FVector(XE(P1.X), P1.X, ZT), FVector(XE(P0.X), P0.X, ZT),
					  Inward(P0.X), Inward(P1.X), Inward(P1.X), Inward(P0.X), P0, P1, FVector2D(P1.X, ZT), FVector2D(P0.X, ZT));
		}
		// The roof, in strips across.
		for (int32 i = 0; i + 1 < Ys.Num(); ++i)
		{
			const double Y0 = Ys[i], Y1 = Ys[i + 1];
			const FVector Up(0, 0, 1);
			Body.Quad(FVector(XF, Y0, ZT), FVector(XE(Y0), Y0, ZT), FVector(XE(Y1), Y1, ZT), FVector(XF, Y1, ZT), Up, Up, Up, Up,
					  FVector2D(XF, Y0), FVector2D(XE(Y0), Y0), FVector2D(XE(Y1), Y1), FVector2D(XF, Y1));
		}

		// The socle, round the front of each pier from where it leaves the plinth to the opening's architrave.
		const FVector Up(0, 0, 1);
		{
			const double XIn = -13.3;   // well inside the plinth: its end is hidden
			const TArray<FVector> North = {FVector(XIn, -WP, 0), FVector(XF, -WP, 0), FVector(XF, -(WO + 0.03), 0)};
			const TArray<FVector> South = {FVector(XF, WO + 0.03, 0), FVector(XF, WP, 0), FVector(XIn, WP, 0)};
			Sweep(Mould, SocleSection(), North, Mitre({FVector(0, -1, 0), FVector(-1, 0, 0)}), {Up, Up, Up}, {false, false, false});
			Sweep(Mould, SocleSection(), South, Mitre({FVector(-1, 0, 0), FVector(0, 1, 0)}), {Up, Up, Up}, {false, false, false});
		}
		// The cornice, from the drum round the front and back to the drum.
		{
			const double XD = XE(WP);
			const TArray<FVector> Path = {FVector(XD, -WP, 0), FVector(XF, -WP, 0), FVector(XF, WP, 0), FVector(XD, WP, 0)};
			Sweep(Mould, PortalCorniceSection(), Path, Mitre({FVector(0, -1, 0), FVector(-1, 0, 0), FVector(0, 1, 0)}), {Up, Up, Up, Up},
				  {false, false, false, false});
		}
		// The architrave round the opening, on the socle, and the keystone.
		{
			const double Foot = 0.70;   // inside the socle's scotia
			TArray<FVector> Path = {FVector(XF, -WO, Foot)};
			for (const FVector2D& P : Arc) { Path.Add(FVector(XF, P.X, P.Y)); }
			Path.Add(FVector(XF, WO, Foot));
			TArray<FVector> Seg = {FVector(0, -1, 0)};
			for (int32 i = 0; i < ArcSteps; ++i)
			{
				const double Phi = 0.5 * (ArcPhi[i] + ArcPhi[i + 1]);
				Seg.Add(FVector(0, FMath::Sin(Phi), FMath::Cos(Phi)));
			}
			Seg.Add(FVector(0, 1, 0));
			TArray<FVector> B = Mitre(Seg);
			TArray<bool> Smooth = {false};
			for (int32 i = 0; i <= ArcSteps; ++i)
			{
				const bool bInner = i > 0 && i < ArcSteps;
				if (bInner) { B[i + 1] = FVector(0, FMath::Sin(ArcPhi[i]), FMath::Cos(ArcPhi[i])); }
				Smooth.Add(bInner);
			}
			Smooth.Add(false);
			TArray<FVector> A;
			A.Init(FVector(-1, 0, 0), Path.Num());
			Sweep(Mould, ArchitraveSection(), Path, A, B, Smooth);

			FOutline Key;
			Key.Add(-0.22, ZC + RO + 0.02); Key.Add(0.22, ZC + RO + 0.02); Key.Add(0.31, ZT - 0.60); Key.Add(-0.31, ZT - 0.60);
			Prism(Mould, Key, FVector(XF, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FVector(-1, 0, 0), -Back, 0.13);
		}
	}

	// ------------------------------------------------------------------------------------------------
	// The drum's order

	/** Where a vertical of the drum's order starts: on the plinth's coping, or on the portal's roof. */
	double OrderBase(double AzimuthDegrees, bool bPortal)
	{
		return bPortal && FromWest(AzimuthDegrees) < PortalBayDegrees ? X::PortalTop - 0.05 : 6.05;
	}

	/** A fin in (r, z): a tapering shaft that flares into a bracket under the corona. */
	FOutline FinSection(double Base)
	{
		FOutline O;
		O.Add(X::OrderRoot, Base + 0.80); O.Add(15.00, Base + 0.80);
		O.Add(14.80, 19.40, true);
		O.Arc(15.60, 19.40, 0.80, 2.68, 180, 90, 16);         // → (15.60, 22.08), into the corona
		O.Add(X::OrderRoot, 22.08);
		return O;
	}

	FOutline PedestalSection(double Base)
	{
		FOutline O;
		O.Add(X::OrderRoot, Base); O.Add(15.10, Base); O.Add(15.10, Base + 0.78); O.Add(15.03, Base + 0.85); O.Add(X::OrderRoot, Base + 0.85);
		return O;
	}

	FOutline MullionSection(double Base)
	{
		FOutline O;
		O.Add(X::OrderRoot, Base); O.Add(14.45, Base); O.Add(14.45, X::CorniceBottom + 0.05); O.Add(X::OrderRoot, X::CorniceBottom + 0.05);
		return O;
	}

	// ------------------------------------------------------------------------------------------------
	// The dome's lattice

	struct FRow { double Polar; int32 Nodes; double Offset; };   // Offset in node steps

	/** The lattice's rows from the equator up: equal steps of the conformal coordinate (square diamonds). */
	TArray<FRow> LatticeRows()
	{
		auto Mercator = [](double Polar) { return FMath::Loge(FMath::Tan(Polar / 2)); };
		auto FromMercator = [](double U) { return 2 * FMath::Atan(FMath::Exp(U)); };
		const double UT = Mercator(TransitionPolar * Deg);
		TArray<FRow> Rows;
		for (int32 i = 0; i <= LowerRows; ++i) { Rows.Add({FromMercator(UT * i / LowerRows), LowerNodes, (i % 2) * 0.5}); }
		const double Step = UE_DOUBLE_PI / UpperNodes;
		for (int32 k = 1; k <= UpperRows; ++k) { Rows.Add({FromMercator(UT - Step * k), UpperNodes, (k % 2) * 0.5}); }
		return Rows;
	}

	FVector Node(const FRow& Row, int32 j)
	{
		return OnSphere(NodeR, Row.Polar, FirstMeridian * Deg + (j + Row.Offset) * Turn / Row.Nodes);
	}

	void Lattice(FMuseeMesh& M)
	{
		const TArray<FRow> Rows = LatticeRows();
		const FVector Centre(0, 0, Equator);
		auto Member = [&M, &Centre](const FVector& A, const FVector& B, double Width, double D0, double D1)
		{
			Beam(M, A, B, ((A + B) / 2 - Centre).GetSafeNormal(), Width, D0, D1);
		};
		auto Wrap = [](int32 j, int32 N) { return ((j % N) + N) % N; };
		for (int32 r = 1; r < Rows.Num(); ++r)
		{
			const FRow& Here = Rows[r];
			const FRow& Below = Rows[r - 1];
			for (int32 j = 0; j < Here.Nodes; ++j)
			{
				// The two nodes below either side: the rows alternate by half a step; where the count halves
				// (the transition), the row below has twice the nodes, every other one under this row's.
				const double T = (j + Here.Offset) * Below.Nodes / double(Here.Nodes) - Below.Offset;
				const double Half = 0.5 * Below.Nodes / double(Here.Nodes);
				const int32 Left = FMath::RoundToInt32(T - Half), Right = FMath::RoundToInt32(T + Half);
				Member(Node(Here, j), Node(Below, Wrap(Left, Below.Nodes)), MemberWidth, -MemberDepth / 2, MemberDepth / 2);
				Member(Node(Here, j), Node(Below, Wrap(Right, Below.Nodes)), MemberWidth, -MemberDepth / 2, MemberDepth / 2);
			}
		}
		// Two rings: halfway up the lower lattice, and at the transition (where every other node ends).
		for (const int32 r : {LowerRows / 2, LowerRows})
		{
			const FRow& Row = Rows[r];
			for (int32 j = 0; j < Row.Nodes; ++j) { Member(Node(Row, j), Node(Row, j + 1), 0.10, -MemberDepth / 2, MemberDepth / 2 + 0.03); }
		}
	}

	/** A meridian rib from the equator (inside the entablature) to the crown ring, standing off the pane. */
	void Meridian(FMuseeMesh& M, double Phi, double Width, double Depth)
	{
		constexpr int32 Steps = 40;
		const double Top = FMath::Acos(MeridianTopR / X::LatticeInner);   // elevation where it enters the crown
		TArray<FVector> Path, A, B;
		TArray<bool> Smooth;
		for (int32 i = 0; i <= Steps; ++i)
		{
			const double El = Top * i / Steps;
			const FVector Out(FMath::Cos(El) * FMath::Cos(Phi), FMath::Cos(El) * FMath::Sin(Phi), FMath::Sin(El));
			Path.Add(FVector(0, 0, Equator) + Out * X::LatticeInner);
			A.Add(Out);
			B.Add(Around(Phi));
			Smooth.Add(true);
		}
		FOutline R;
		const double C = FMath::Min(0.04, Width / 4);
		R.Add(0, -Width / 2); R.Add(Depth - C, -Width / 2); R.Add(Depth, -Width / 2 + C); R.Add(Depth, Width / 2 - C); R.Add(Depth - C, Width / 2); R.Add(0, Width / 2);
		Sweep(M, R, Path, A, B, Smooth);
	}

	// ------------------------------------------------------------------------------------------------
	// The crown

	FOutline CrownSection()
	{
		FOutline O;
		O.Add(0, Equator + CrownSeat);
		constexpr int32 Steps = 12;
		for (int32 i = 1; i <= Steps; ++i)
		{
			const double R = CrownOuter * i / Steps;
			O.Add(R, Equator + FMath::Sqrt(CrownSeat * CrownSeat - R * R), i < Steps);
		}
		O.Add(CrownOuter, 36.18); O.Add(3.40, 36.18);
		O.Arc(3.40, 36.26, 0.08, 0.08, -90, 90, 10);          // bead
		O.Add(3.28, 36.34);
		O.Arc(3.28, 36.54, 0.20, 0.20, -90, -180, 10);        // cavetto
		O.Add(3.08, X::CrownTop);
		O.Add(0, X::CrownTop);
		return O;
	}

	/** The lantern's entablature and its pearl cap (a spherical cap on r 2.48, up to LanternTop). */
	FOutline LanternCapSection()
	{
		const double A = LanternCapBase, Z0 = LanternEntablature + 0.37, H = X::LanternTop - Z0;
		const double R = (A * A + H * H) / (2 * H), Zc = X::LanternTop - R;
		const double E0 = LanternEntablature;
		FOutline O;
		O.Add(0, E0); O.Add(2.50, E0); O.Add(2.50, E0 + 0.20); O.Add(2.60, E0 + 0.20); O.Add(2.60, E0 + 0.32); O.Add(A, Z0);
		O.Arc(0, Zc, R, R, FMath::RadiansToDegrees(FMath::Atan2(Z0 - Zc, A)), 90, 16);
		return O;
	}
}

namespace EXK = ElanExteriorKit;

// ----------------------------------------------------------------------------------------------------
// Checks from the console (nothing is saved): a preview before the actor is placed in the map, the
// closed solids' open edges, and the viewpoints.

namespace ElanExteriorCheck
{
	AElanExteriorStructure* Find(UWorld* World)
	{
		for (TActorIterator<AElanExteriorStructure> It(World); It; ++It) { return *It; }
		return nullptr;
	}

	/** Edges used by an odd number of triangles once the vertices are welded (0.1 mm): 0 for closed solids. */
	int32 OpenEdges(const FProcMeshSection& S)
	{
		TMap<FIntVector, int32> Weld;
		TArray<int32> Id;
		for (const FProcMeshVertex& V : S.ProcVertexBuffer)
		{
			const FIntVector Key(FMath::RoundToInt32(V.Position.X * 100), FMath::RoundToInt32(V.Position.Y * 100), FMath::RoundToInt32(V.Position.Z * 100));
			const int32* Found = Weld.Find(Key);
			Id.Add(Found ? *Found : Weld.Add(Key, Weld.Num()));
		}
		TMap<TPair<int32, int32>, int32> Uses;
		for (int32 i = 0; i + 2 < S.ProcIndexBuffer.Num(); i += 3)
		{
			for (int32 e = 0; e < 3; ++e)
			{
				const int32 A = Id[S.ProcIndexBuffer[i + e]], B = Id[S.ProcIndexBuffer[i + (e + 1) % 3]];
				if (A == B) { continue; }
				Uses.FindOrAdd(TPair<int32, int32>(FMath::Min(A, B), FMath::Max(A, B)))++;
			}
		}
		int32 Odd = 0;
		for (const auto& U : Uses) { Odd += U.Value % 2; }
		return Odd;
	}

	/**
	 * The section's connected pieces (welded at 0.1 mm), and those that enclose a negative volume: turned
	 * inside out. Unreal's front faces wind clockwise seen from outside, so a sound solid's signed volume
	 * Σ a·(b × c) / 6 over its triangles is negative.
	 */
	TPair<int32, int32> InsideOut(const FProcMeshSection& S)
	{
		TMap<FIntVector, int32> Weld;
		TArray<int32> Id;
		for (const FProcMeshVertex& V : S.ProcVertexBuffer)
		{
			const FIntVector Key(FMath::RoundToInt32(V.Position.X * 100), FMath::RoundToInt32(V.Position.Y * 100), FMath::RoundToInt32(V.Position.Z * 100));
			const int32* Found = Weld.Find(Key);
			Id.Add(Found ? *Found : Weld.Add(Key, Weld.Num()));
		}
		TArray<int32> Parent;
		for (int32 i = 0; i < Weld.Num(); ++i) { Parent.Add(i); }
		TFunction<int32(int32)> Root = [&Parent, &Root](int32 i) { return Parent[i] == i ? i : (Parent[i] = Root(Parent[i])); };
		for (int32 i = 0; i + 2 < S.ProcIndexBuffer.Num(); i += 3)
		{
			const int32 A = Root(Id[S.ProcIndexBuffer[i]]);
			Parent[Root(Id[S.ProcIndexBuffer[i + 1]])] = A;
			Parent[Root(Id[S.ProcIndexBuffer[i + 2]])] = A;
		}
		TMap<int32, double> Volume;
		for (int32 i = 0; i + 2 < S.ProcIndexBuffer.Num(); i += 3)
		{
			const FVector A(S.ProcVertexBuffer[S.ProcIndexBuffer[i]].Position), B(S.ProcVertexBuffer[S.ProcIndexBuffer[i + 1]].Position),
				C(S.ProcVertexBuffer[S.ProcIndexBuffer[i + 2]].Position);
			// Relative to the actor's axis at mid-height, so the sums stay well conditioned.
			const FVector O(0, 0, 2000);
			Volume.FindOrAdd(Root(Id[S.ProcIndexBuffer[i]])) += FVector::DotProduct(A - O, FVector::CrossProduct(B - O, C - O)) / 6.0;
		}
		int32 Bad = 0;
		for (const auto& V : Volume) { Bad += V.Value > 1.0 ? 1 : 0; }   // cm³
		return TPair<int32, int32>(Volume.Num(), Bad);
	}

	/** The winding's normals against the given ones: triangles facing the other way. */
	int32 Flipped(const FProcMeshSection& S)
	{
		int32 Bad = 0;
		for (int32 i = 0; i + 2 < S.ProcIndexBuffer.Num(); i += 3)
		{
			const FProcMeshVertex &A = S.ProcVertexBuffer[S.ProcIndexBuffer[i]], &B = S.ProcVertexBuffer[S.ProcIndexBuffer[i + 1]], &C = S.ProcVertexBuffer[S.ProcIndexBuffer[i + 2]];
			const FVector Face = FVector::CrossProduct(FVector(B.Position - A.Position), FVector(C.Position - A.Position));
			if (Face.SizeSquared() < 1e-8) { continue; }
			// Unreal's front faces: cross(b − a, c − a) points away from the side they face.
			if (FVector::DotProduct(Face, FVector(A.Normal + B.Normal + C.Normal)) > 0) { ++Bad; }
		}
		return Bad;
	}
}

static FAutoConsoleCommandWithWorld GElanExteriorSpawn(
	TEXT("musee.ElanExterior.Spawn"),
	TEXT("Spawn the Élan's exterior at (5400, 0, 0) for this session only, if the map has none (a preview; nothing is saved)."),
	FConsoleCommandWithWorldDelegate::CreateStatic([](UWorld* World)
	{
		if (!World || ElanExteriorCheck::Find(World)) { return; }
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		World->SpawnActor<AElanExteriorStructure>(MuseePlan::Elan::Centre(), FRotator::ZeroRotator, Params);
		UE_LOG(LogMusee, Log, TEXT("Élan exterior: spawned for this session at (5400, 0, 0)."));
	}));

static FAutoConsoleCommandWithWorld GElanExteriorCheck(
	TEXT("musee.ElanExterior.Check"),
	TEXT("Log the Élan exterior's closed solids' open edges and flipped triangles (0 and 0 when sound), and its viewpoints."),
	FConsoleCommandWithWorldDelegate::CreateStatic([](UWorld* World)
	{
		const AElanExteriorStructure* A = World ? ElanExteriorCheck::Find(World) : nullptr;
		if (!A) { UE_LOG(LogMusee, Warning, TEXT("Élan exterior: none in this world (musee.ElanExterior.Spawn)."));  return; }
		const TPair<const TCHAR*, UProceduralMeshComponent*> Parts[] = {{TEXT("Stone"), A->Stone}, {TEXT("Steel"), A->Steel}, {TEXT("Gilt"), A->Gilt},
																		{TEXT("Skin"), A->Skin}, {TEXT("Glaze"), A->Glaze}};
		for (const auto& Part : Parts)
		{
			for (int32 s = 0; s < Part.Value->GetNumSections(); ++s)
			{
				const FProcMeshSection* S = Part.Value->GetProcMeshSection(s);
				if (!S || S->ProcIndexBuffer.Num() == 0) { continue; }
				const TPair<int32, int32> Pieces = ElanExteriorCheck::InsideOut(*S);
				UE_LOG(LogMusee, Log, TEXT("Élan exterior: %s %d: %d triangles, %d open edges, %d flipped, %d pieces (%d inside out)."), Part.Key, s,
					   S->ProcIndexBuffer.Num() / 3, ElanExteriorCheck::OpenEdges(*S), ElanExteriorCheck::Flipped(*S), Pieces.Key, Pieces.Value);
			}
		}
		for (const FElanExteriorViewpoint& V : AElanExteriorStructure::GetViewpoints())
		{
			UE_LOG(LogMusee, Log, TEXT("Élan exterior view: %s: %s"), *V.Name, *V.CommandLine());
		}
	}));

// ----------------------------------------------------------------------------------------------------

FString FElanExteriorViewpoint::Pose() const
{
	return FString::Printf(TEXT("%g,%g,%g,%g,%g"), X, Y, Feet, Yaw, Pitch);
}

FString FElanExteriorViewpoint::CommandLine() const
{
	return FString::Printf(TEXT("-MuseePose=%s -MuseeFov=%g \"-ExecCmds=musee.Hour %g\""), *Pose(), Fov, Hour);
}

TArray<FElanExteriorViewpoint> AElanExteriorStructure::GetViewpoints()
{
	auto V = [](const TCHAR* Name, double X, double Y, double Yaw, double Pitch, float Fov, float Hour, const TCHAR* Note)
	{
		FElanExteriorViewpoint P;
		P.Name = Name;
		P.X = X; P.Y = Y; P.Feet = 0; P.Yaw = Yaw; P.Pitch = Pitch; P.Fov = Fov; P.Hour = Hour;
		P.Note = Note;
		return P;
	};
	return {
		V(TEXT("Hall of Light, the axis (day)"), 12.5, 0, 0, 19, 90, 15,
		  TEXT("From the Rotunda's door: the portal closes the vista; the drum and the pearl dome rise over the glass vault.")),
		V(TEXT("Hall of Light, the axis (night)"), 12.5, 0, 0, 19, 90, 21.5,
		  TEXT("The drum glows through its order, the fins lit from their feet, the lantern warm over the pearl.")),
		V(TEXT("Hall of Light, under the dome"), 33.5, 0, 0, 33, 90, 15,
		  TEXT("Near the east end, looking up through the vault: the portal's cornice, the fins, the entablature, the dome.")),
		V(TEXT("South lawn, the portal side (day)"), 30, 30, -51.3, 18, 90, 16,
		  TEXT("Past the orchard's east end: the whole hall, the portal taking in the Hall of Light, the afternoon sun on the plinth.")),
		V(TEXT("South lawn, the portal side (night)"), 30, 30, -51.3, 18, 90, 21.5,
		  TEXT("The pearl over the orchard at night.")),
		V(TEXT("Chinese court, over the east cloister (day)"), -4, 20, -19, 22, 60, 10,
		  TEXT("From the garden, over the cloister's roof: the upper drum, the entablature, the dome and the crown in the morning sun.")),
		V(TEXT("Chinese court, over the east cloister (night)"), -4, 20, -19, 22, 60, 21.5,
		  TEXT("The dome and the lantern glow over the roofs.")),
		V(TEXT("North lawn, over the meadow"), 30, -30, 51.3, 18, 90, 11,
		  TEXT("Against the sun: the drum's glass and the dome's lattice in silhouette, the birches in front.")),
		V(TEXT("East lawn, the elevation (a check, not a visitor's view)"), 125, 0, 180, 15, 60, 9,
		  TEXT("The whole height square on, lit by the morning sun: plinth, drum, entablature, dome, lantern.")),
	};
}

// ----------------------------------------------------------------------------------------------------

AElanExteriorStructure::AElanExteriorStructure()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.5f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	auto Make = [this](const TCHAR* Name)
	{
		UProceduralMeshComponent* M = CreateDefaultSubobject<UProceduralMeshComponent>(Name);
		M->SetupAttachment(RootComponent);
		M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		M->bUseAsyncCooking = true;
		// The exterior's lights are on channel 1 only, so they light nothing inside (the interior is on 0).
		M->SetLightingChannels(true, true, false);
		return M;
	};
	Stone = Make(TEXT("Stone"));
	Stone->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	Stone->bUseComplexAsSimpleCollision = true;
	Steel = Make(TEXT("Steel"));
	Gilt = Make(TEXT("Gilt"));
	Skin = Make(TEXT("Skin"));
	Glaze = Make(TEXT("Glaze"));
	Glaze->SetCastShadow(false);
	MuseeBake::NoBake(Skin);   // its glowing materials are made at BeginPlay (ApplyMaterials): stays procedural

	auto Spot = [this](const FString& Name)
	{
		USpotLightComponent* L = CreateDefaultSubobject<USpotLightComponent>(*Name);
		L->SetupAttachment(RootComponent);
		L->SetMobility(EComponentMobility::Movable);
		return L;
	};
	for (int32 k = 0; k < MuseePlan::ElanExterior::Fins; ++k)
	{
		FinUplights.Add(Spot(FString::Printf(TEXT("FinUplight%02d"), k + 1)));
		DomeWashes.Add(Spot(FString::Printf(TEXT("DomeWash%02d"), k + 1)));
	}
	CrownLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CrownLight"));
	CrownLight->SetupAttachment(RootComponent);
	CrownLight->SetMobility(EComponentMobility::Movable);

	auto Path = [](const TCHAR* Name) { return FSoftObjectPath(FString::Printf(TEXT("/Game/Museum/Materials/%s.%s"), Name, Name)); };
	// The exterior's weathered travertine (Scripts/exterior_materials.py).
	StoneMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Museum/Materials/Exterior/MI_Ext_Elan.MI_Ext_Elan")));
	MouldingMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Museum/Materials/Exterior/MI_Ext_Elan.MI_Ext_Elan")));
	SteelMaterial = TSoftObjectPtr<UMaterialInterface>(Path(TEXT("M_RibPearl")));
	GiltMaterial = TSoftObjectPtr<UMaterialInterface>(Path(TEXT("M_Gilt_Aged")));
	SkinMaterial = TSoftObjectPtr<UMaterialInterface>(Path(TEXT("M_MistyGlass")));
	LanternMaterial = TSoftObjectPtr<UMaterialInterface>(Path(TEXT("M_OpalGlobe")));
	GlazeMaterial = TSoftObjectPtr<UMaterialInterface>(Path(TEXT("M_Glass")));
	ShellMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Museum/Materials/Exterior/MI_Ext_Pearl.MI_Ext_Pearl")));
	Parameters = TSoftObjectPtr<UMaterialParameterCollection>(Path(TEXT("MPC_Musee")));

	PlaceLights();
	AddTags();
}

void AElanExteriorStructure::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
	ApplyMaterials(false);
	PlaceLights();
	SetNight(0.f, 1.f);   // the editor shows the day
	AddTags();
}

void AElanExteriorStructure::BeginPlay()
{
	Super::BeginPlay();
	// A placed actor doesn't rerun its construction when the map loads: rebuild, so the map never shows
	// an older build; then the skins' own glow, and the lights by the daylight.
	Build();
	ApplyMaterials(true);
	PlaceLights();
	LoadedParameters = Parameters.LoadSynchronous();
	LastDaylight = -1.f;
	float Daylight = 1.f;
	if (LoadedParameters) { Daylight = UKismetMaterialLibrary::GetScalarParameterValue(this, LoadedParameters, TEXT("Daylight")); }
	SetNight(1.f - FMath::SmoothStep(NightLightsFadeFrom, NightLightsOffAt, Daylight), Daylight);
}

void AElanExteriorStructure::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!LoadedParameters) { return; }
	const float Daylight = UKismetMaterialLibrary::GetScalarParameterValue(this, LoadedParameters, TEXT("Daylight"));
	if (FMath::Abs(Daylight - LastDaylight) < 0.005f) { return; }
	SetNight(1.f - FMath::SmoothStep(NightLightsFadeFrom, NightLightsOffAt, Daylight), Daylight);
}

void AElanExteriorStructure::AddTags()
{
	Tags.AddUnique(FName(TEXT("musee.building")));
	Tags.AddUnique(FName(TEXT("musee.wing:Elan")));
	Tags.AddUnique(FName(TEXT("musee.elan.exterior")));
	// It ticks only to dim its own lights and the lantern's glass: all but the Skin (musee.nobake) may be baked.
	Tags.AddUnique(MuseeBake::BakeableTag());
}

void AElanExteriorStructure::SetNight(float Level, float Daylight)
{
	NightLevel = FMath::Clamp(Level, 0.f, 1.f);
	LastDaylight = Daylight;
	const bool bOn = NightLevel > 0.001f;
	for (USpotLightComponent* L : FinUplights) { if (L) { L->SetIntensity(FinUplightCandela * NightLevel); L->SetVisibility(bOn); } }
	// On the pearl shell the washes only graze it softly (a white shell under the lattice's washes read as a lamp).
	const float Wash = bPearlShell ? 0.3f : 1.f;
	for (USpotLightComponent* L : DomeWashes) { if (L) { L->SetIntensity(DomeWashCandela * Wash * NightLevel); L->SetVisibility(bOn); } }
	if (CrownLight) { CrownLight->SetIntensity(CrownLightCandela * NightLevel); CrownLight->SetVisibility(bOn && bLantern && !bPearlShell); }
	if (LanternGlow)
	{
		LanternGlow->SetScalarParameterValue(TEXT("Luminance"), FMath::Lerp(LanternNightNits, LanternDayNits, FMath::Clamp(Daylight, 0.f, 1.f)));
	}
}

void AElanExteriorStructure::PlaceLights()
{
	namespace X = MuseePlan::ElanExterior;
	auto Setup = [this](ULocalLightComponent* L, double Attenuation)
	{
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetUseTemperature(true);
		L->SetTemperature(LightKelvin);
		L->SetLightColor(FLinearColor::White);
		L->SetAttenuationRadius(static_cast<float>(Attenuation * MuseePlan::Cm));
		L->SetCastShadows(false);
		L->SetLightingChannels(false, true, false);
	};
	for (int32 k = 0; k < FinUplights.Num(); ++k)
	{
		USpotLightComponent* L = FinUplights[k];
		if (!L) { continue; }
		const double Az = EXK::FinAzimuth(k), Theta = Az * EXK::Deg;
		const double Base = EXK::OrderBase(Az, bPortal);
		const FVector At = EXK::Radial(Theta) * 15.22 + FVector(0, 0, Base + 0.11);
		const FVector Aim = EXK::Radial(Theta) * 14.90 + FVector(0, 0, 19.0);
		L->SetRelativeLocationAndRotation(At * MuseePlan::Cm, (Aim - At).Rotation());
		L->SetInnerConeAngle(5.f);
		L->SetOuterConeAngle(10.f);
		L->SetSourceRadius(3.f);
		Setup(L, 22.0);
	}
	for (int32 k = 0; k < DomeWashes.Num(); ++k)
	{
		USpotLightComponent* L = DomeWashes[k];
		if (!L) { continue; }
		const double Theta = EXK::MullionAzimuth(k) * EXK::Deg;
		const FVector At = EXK::Radial(Theta) * 15.35 + FVector(0, 0, 22.62);
		const FVector Aim = EXK::Radial(Theta) * 10.95 + FVector(0, 0, 31.19);
		L->SetRelativeLocationAndRotation(At * MuseePlan::Cm, (Aim - At).Rotation());
		L->SetInnerConeAngle(25.f);
		L->SetOuterConeAngle(40.f);
		L->SetSourceRadius(4.f);
		Setup(L, 20.0);
	}
	if (CrownLight)
	{
		CrownLight->SetRelativeLocation(FVector(0, 0, 37.65) * MuseePlan::Cm);
		CrownLight->SetSourceRadius(40.f);
		Setup(CrownLight, 15.0);
	}
}

void AElanExteriorStructure::ApplyMaterials(bool bInstances)
{
	auto Load = [](const TSoftObjectPtr<UMaterialInterface>& P) -> UMaterialInterface* { return P.IsNull() ? nullptr : P.LoadSynchronous(); };
	UMaterialInterface* StoneM = Load(StoneMaterial);
	UMaterialInterface* MouldM = Load(MouldingMaterial);
	UMaterialInterface* SteelM = Load(SteelMaterial);
	UMaterialInterface* GiltM = Load(GiltMaterial);
	UMaterialInterface* GlazeM = Load(GlazeMaterial);
	UMaterialInterface* SkinM = Load(SkinMaterial);
	UMaterialInterface* LanternM = Load(LanternMaterial);
	if (!LanternM) { LanternM = SkinM; }

	if (StoneM) { Stone->SetMaterial(0, StoneM); }
	if (MouldM) { Stone->SetMaterial(1, MouldM); }
	if (SteelM) { for (int32 s = 0; s < 3; ++s) { Steel->SetMaterial(s, SteelM); } }
	if (bPearlShell) { if (UMaterialInterface* ShellM = Load(ShellMaterial)) { Steel->SetMaterial(1, ShellM); } }
	if (GiltM) { Gilt->SetMaterial(0, GiltM); }
	if (GlazeM) { Glaze->SetMaterial(0, GlazeM); Glaze->SetMaterial(1, GlazeM); }

	UMaterialInterface* Drum = SkinM;
	UMaterialInterface* Dome = SkinM;
	UMaterialInterface* Lantern = LanternM;
	if (bInstances)
	{
		// Play only (never saved with the map): each skin's own luminance, × max(Daylight, its night floor).
		auto Glow = [this](UMaterialInterface* Base, float Day, float Night) -> UMaterialInstanceDynamic*
		{
			if (!Base) { return nullptr; }
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, this);
			MID->SetFlags(RF_Transient);
			MID->SetScalarParameterValue(TEXT("Luminance"), Day);
			MID->SetScalarParameterValue(TEXT("NightFloor"), Day > 0.f ? FMath::Clamp(Night / Day, 0.f, 1.f) : 1.f);
			return MID;
		};
		DrumGlow = Glow(SkinM, DrumDayNits, DrumNightNits);
		DomeGlow = Glow(SkinM, DomeDayNits, DomeNightNits);
		// The lantern follows the daylight in SetNight (its own floor 1, if it is misty glass).
		LanternGlow = Glow(LanternM, LanternDayNits, LanternDayNits);
		if (DrumGlow) { Drum = DrumGlow; }
		if (DomeGlow) { Dome = DomeGlow; }
		if (LanternGlow) { Lantern = LanternGlow; }
	}
	if (Drum) { Skin->SetMaterial(0, Drum); }
	if (Dome) { Skin->SetMaterial(1, Dome); }
	if (Lantern) { Skin->SetMaterial(2, Lantern); }
}

void AElanExteriorStructure::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }   // baked into static meshes (Geometry/MuseeBake.h)
	namespace X = MuseePlan::ElanExterior;
	using EXK::FOutline;
	const double Turn = EXK::Turn, Deg = EXK::Deg;

	for (UProceduralMeshComponent* M : {Stone.Get(), Steel.Get(), Gilt.Get(), Skin.Get(), Glaze.Get()}) { M->ClearAllMeshSections(); }

	// Round the portal the plinth and the lowest transoms stop inside its body.
	const double Cut = bPortal ? (180.0 - EXK::PortalCutDegrees) * Deg : UE_DOUBLE_PI;
	const int32 CutSegments = FMath::CeilToInt32(EXK::Round * (2 * Cut) / Turn);

	// Stone: the plinth and the portal's body; the entablature and the portal's mouldings.
	FMuseeMesh Plinth, Mould;
	EXK::Lathe(Plinth, EXK::PlinthSection(), true, -Cut, Cut, CutSegments);
	EXK::Lathe(Mould, EXK::EntablatureSection(), true, 0, Turn, EXK::Round);
	if (bPortal) { EXK::Portal(Plinth, Mould); }

	// The drum's order: twelve fins on pedestals, twelve mullions, three transoms.
	FMuseeMesh Order;
	const FVector Up(0, 0, 1);
	for (int32 k = 0; k < X::Fins; ++k)
	{
		const double FinAz = EXK::FinAzimuth(k), MullionAz = EXK::MullionAzimuth(k);
		const double FinBase = EXK::OrderBase(FinAz, bPortal), MullionBase = EXK::OrderBase(MullionAz, bPortal);
		const double T = FinAz * Deg, TM = MullionAz * Deg;
		EXK::Prism(Order, EXK::PedestalSection(FinBase), FVector::ZeroVector, EXK::Radial(T), Up, EXK::Around(T), -0.28, 0.28);
		EXK::Prism(Order, EXK::FinSection(FinBase), FVector::ZeroVector, EXK::Radial(T), Up, EXK::Around(T), -X::FinWidth / 2, X::FinWidth / 2);
		EXK::Prism(Order, EXK::MullionSection(MullionBase), FVector::ZeroVector, EXK::Radial(TM), Up, EXK::Around(TM), -X::MullionWidth / 2, X::MullionWidth / 2);
	}
	for (double Z = X::FirstTransom; Z < X::CorniceBottom - 0.5; Z += X::TransomStep)
	{
		FOutline Bar;
		Bar.Add(X::OrderRoot, Z - 0.05); Bar.Add(14.30, Z - 0.05); Bar.Add(14.30, Z + 0.05); Bar.Add(X::OrderRoot, Z + 0.05);
		const bool bBelowPortal = bPortal && Z < X::PortalTop;
		const double Span = bBelowPortal ? (180.0 - 23.0) * Deg : UE_DOUBLE_PI;
		EXK::Lathe(Order, Bar, true, -Span, Span, FMath::CeilToInt32(EXK::Round * (2 * Span) / Turn));
	}

	FMuseeMesh Lattice, Crown, LanternSteel, LanternGlass;
	if (bPearlShell)
	{
		// One seamless opaque shell, 6 cm of panel over its frame: out of the cornice (its foot inside the
		// entablature) up over the top; the panels' joints are the material's (hairlines on meridians and parallels).
		constexpr double Ro = 14.30, Ri = 14.24;
		FOutline Sh;
		Sh.Add(Ro * FMath::Cos(-0.3 * Deg), EXK::Equator + Ro * FMath::Sin(-0.3 * Deg));
		Sh.Arc(0, EXK::Equator, Ro, Ro, -0.3, 90, 96);
		Sh.Add(0, EXK::Equator + Ri);
		Sh.Arc(0, EXK::Equator, Ri, Ri, 90, -0.3, 96);
		EXK::Lathe(Lattice, Sh, true, 0, Turn, EXK::Round);
		// A hairline gilt ring where the shell rises out of the cornice: a 5 cm half-round, 3 cm proud, whose drip
		// throws the shell's water clear of the cornice's wash.
		const double Zr = X::CorniceTop + 0.015, Rr = FMath::Sqrt(Ro * Ro - FMath::Square(Zr - EXK::Equator));
		FOutline Ring;
		Ring.Add(Rr - 0.03, Zr - 0.025);
		Ring.Add(Rr + 0.005, Zr - 0.025);
		Ring.Arc(Rr + 0.005, Zr, 0.025, 0.025, -90, 90, 10);
		Ring.Add(Rr - 0.03, Zr + 0.025);
		EXK::Lathe(Crown, Ring, true, 0, Turn, EXK::Round);
		// At the top a low gilt boss and the gilt ball on it: a sphere on the Sphere.
		const double Top = EXK::Equator + Ro;
		FOutline Boss;
		Boss.Add(0, Top - 0.06); Boss.Add(0.55, Top - 0.06); Boss.Add(0.55, Top - 0.01);
		Boss.Arc(0.51, Top - 0.01, 0.04, 0.04, 0, 90, 6);
		Boss.Add(0.30, Top + 0.12); Boss.Add(0.16, Top + 0.30); Boss.Add(0, Top + 0.30);
		EXK::Lathe(Crown, Boss, true, 0, Turn, 64);
		FOutline Ball;
		Ball.Add(0, Top + 0.24);
		Ball.Arc(0, Top + 0.24 + 0.45, 0.45, 0.45, -90, 90, 20);
		EXK::Lathe(Crown, Ball, true, 0, Turn, 48);
	}
	else
	{
		// The dome's lattice: 24 meridians (the fins' heavier), the lamella's diamonds and two rings.
		for (int32 k = 0; k < 24; ++k)
		{
			const bool bFin = k % 2 == 1;
			EXK::Meridian(Lattice, (EXK::FirstMeridian + 15.0 * k) * Deg, bFin ? 0.30 : 0.16, bFin ? 0.33 : 0.19);
		}
		EXK::Lattice(Lattice);
		// The crown ring (the lantern, its cap and the ball follow).
		EXK::Lathe(Crown, EXK::CrownSection(), true, 0, Turn, 96);
	}
	if (bLantern && !bPearlShell)
	{
		FOutline Band;
		const double GR = EXK::LanternR, GT = EXK::LanternGlassTop + 0.02;
		Band.Add(GR - 0.02, 36.58); Band.Add(GR, 36.58); Band.Add(GR, GT); Band.Add(GR - 0.02, GT);
		EXK::Lathe(LanternGlass, Band, true, 0, Turn, 96);
		for (int32 k = 0; k < X::Fins; ++k)
		{
			const double T = EXK::FinAzimuth(k) * Deg;
			FOutline Col;
			Col.Add(EXK::LanternR - 0.01, 36.58); Col.Add(EXK::LanternR + 0.20, 36.58); Col.Add(EXK::LanternR + 0.20, EXK::LanternGlassTop + 0.02);
			Col.Add(EXK::LanternR - 0.01, EXK::LanternGlassTop + 0.02);
			EXK::Prism(LanternSteel, Col, FVector::ZeroVector, EXK::Radial(T), Up, EXK::Around(T), -0.08, 0.08);
		}
		EXK::Lathe(LanternSteel, EXK::LanternCapSection(), true, 0, Turn, 96);
		FOutline Stem;
		Stem.Add(0, X::LanternTop - 0.05); Stem.Add(0.08, X::LanternTop - 0.05); Stem.Add(0.08, X::FinialTop - 0.60); Stem.Add(0, X::FinialTop - 0.60);
		EXK::Lathe(Crown, Stem, true, 0, Turn, 24);
		FOutline Ball;
		Ball.Add(0, X::FinialTop - 0.64);
		Ball.Arc(0, X::FinialTop - 0.32, 0.32, 0.32, -90, 90, 16);
		EXK::Lathe(Crown, Ball, true, 0, Turn, 32);
	}

	// The skins: misty glass on the drum and the dome; the clear panes over them.
	FMuseeMesh DrumSkin, DomeSkin, DrumPane, DomePane;
	auto Column = [](double R, double Z0, double Z1) { FOutline O; O.Add(R, Z0); O.Add(R, Z1); return O; };
	auto Shell = [](double R)
	{
		FOutline O;
		O.Add(R * FMath::Cos(-0.2 * EXK::Deg), EXK::Equator + R * FMath::Sin(-0.2 * EXK::Deg));   // inside the entablature
		O.Arc(0, EXK::Equator, R, R, -0.2, 90, 64);
		return O;
	};
	if (bDrumSkin) { EXK::Lathe(DrumSkin, Column(X::DrumSkin, 6.01, EXK::Equator), false, 0, Turn, EXK::Round); }
	if (!bPearlShell) { EXK::Lathe(DomeSkin, Shell(X::DomeSkin), false, 0, Turn, EXK::Round); }
	if (bGlaze)
	{
		EXK::Lathe(DrumPane, Column(X::DrumGlaze, 6.08, X::CorniceBottom + 0.05), false, 0, Turn, EXK::Round);
		if (!bPearlShell) { EXK::Lathe(DomePane, Shell(X::DomeGlaze), false, 0, Turn, EXK::Round); }
	}

	auto Write = [](UProceduralMeshComponent* C, int32 Index, const FMuseeMesh& M, bool bCollision)
	{
		if (M.Vertices.Num() == 0) { return; }
		C->CreateMeshSection(Index, M.Vertices, M.Triangles, M.Normals, M.UVs, TArray<FColor>(), M.Tangents, bCollision);
	};
	Write(Stone, 0, Plinth, true);
	Write(Stone, 1, Mould, true);
	Write(Steel, 0, Order, false);
	Write(Steel, 1, Lattice, false);
	Write(Steel, 2, LanternSteel, false);
	Write(Gilt, 0, Crown, false);
	Write(Skin, 0, DrumSkin, false);
	Write(Skin, 1, DomeSkin, false);
	Write(Skin, 2, LanternGlass, false);
	Write(Glaze, 0, DrumPane, false);
	Write(Glaze, 1, DomePane, false);

	UE_LOG(LogMusee, Verbose, TEXT("Élan exterior: %d stone, %d steel, %d lattice triangles."),
		   (Plinth.Triangles.Num() + Mould.Triangles.Num()) / 3, Order.Triangles.Num() / 3, Lattice.Triangles.Num() / 3);
}
