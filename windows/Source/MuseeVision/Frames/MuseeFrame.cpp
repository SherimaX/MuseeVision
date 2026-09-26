#include "Frames/MuseeFrame.h"

#include "MuseeVision.h"
#include "ProceduralMeshComponent.h"
#include "Geometry/MuseeBake.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Math/RotationMatrix.h"
#include "Misc/PackageName.h"
#include "StaticMeshResources.h"

/*
 * Geometry, in metres in the frame's own space: X out of the wall (towards the viewer), Y along the
 * width, Z up; the origin at the middle of the back. A profile point is (s, h): s outward from the
 * sight line (negative over the canvas: the lip), h in front of the back. Each side of the sight
 * rectangle sweeps the profile from mitre to mitre (the mitre at along-side t = ±(L + s)).
 *
 * A named namespace (not an anonymous one) so unity builds can't mix these up with another file's.
 */
namespace MuseeFrameImpl
{
	constexpr double MetresToCm = 100.0;

	enum EBand : uint8
	{
		BandFace = 0,     // gilt (or the photograph frame's wood)
		BandSanded = 1,   // the sanded frieze: gilt, matte
		BandBack = 2,     // raw wood: the back and the rebate
	};

	FColor Paint(double Exposure, bool bSanded, bool bOrnament)
	{
		const uint8 R = uint8(FMath::Clamp(Exposure, 0.0, 1.0) * 255.0 + 0.5);
		const uint8 G = bSanded ? uint8(255) : uint8(0);
		const uint8 B = bOrnament ? uint8(255) : uint8(0);
		return FColor(R, G, B, uint8(255));
	}

	/** One mesh section; vertices are shared where the caller reuses an index. */
	struct FSectionBuilder
	{
		TArray<FVector> Positions;
		TArray<int32> Indices;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<FColor> Colors;
		TArray<FProcMeshTangent> Tangents;

		int32 Vert(const FVector& PosMetres, const FVector& Nrm, const FVector2D& Uv, const FColor& Col, const FVector& Tan)
		{
			Positions.Add(PosMetres * MetresToCm);
			const FVector SafeN = Nrm.GetSafeNormal(1e-30);
			Normals.Add(SafeN.IsNearlyZero() ? FVector::XAxisVector : SafeN);
			UVs.Add(Uv);
			Colors.Add(Col);
			const FVector SafeT = Tan.GetSafeNormal(1e-30);
			Tangents.Add(FProcMeshTangent(SafeT.IsNearlyZero() ? FVector::YAxisVector : SafeT, false));
			return Positions.Num() - 1;
		}

		/** Unreal's front faces: cross(b − a, c − a) points away from the side they face (as FMuseeMesh). */
		void Tri(int32 IA, int32 IB, int32 IC)
		{
			const FVector Face = FVector::CrossProduct(Positions[IB] - Positions[IA], Positions[IC] - Positions[IA]);
			if (Face.SizeSquared() < 1e-12) { return; }   // a collapsed tip (cm²)
			if (FVector::DotProduct(Face, Normals[IA] + Normals[IB] + Normals[IC]) < 0) { Indices.Append({IA, IB, IC}); }
			else { Indices.Append({IA, IC, IB}); }
		}

		void Quad(int32 IA, int32 IB, int32 IC, int32 ID)
		{
			Tri(IA, IB, IC);
			Tri(IA, IC, ID);
		}

		bool IsEmpty() const { return Indices.Num() == 0; }

		void Write(UProceduralMeshComponent* Target, int32 Section, bool bWithCollision) const
		{
			Target->CreateMeshSection(Section, Positions, Indices, Normals, UVs, Colors, Tangents, bWithCollision);
		}
	};

	// ------------------------------------------------------------------------------ the profile

	struct FProfilePoint
	{
		FVector2D P = FVector2D::ZeroVector;   // (s, h), metres
		bool bHard = true;                     // a crease here; else the normal is smoothed across it
		uint8 Band = BandFace;                 // of the segment that starts here
	};

	struct FProfile
	{
		TArray<FProfilePoint> Pts;
		TArray<FVector2D> SegN;    // each segment's normal, (−dh, ds): out of the solid
		TArray<double> Sigma;      // arc length at each point

		void Line(double S, double H, bool bHard = true, uint8 Band = BandFace)
		{
			FProfilePoint& Added = Pts.AddDefaulted_GetRef();
			Added.P = FVector2D(S, H);
			Added.bHard = bHard;
			Added.Band = Band;
		}

		/** An elliptical arc from A0 to A1 degrees; it merges with the last point if it starts there. */
		void Arc(const FVector2D& Centre, double RX, double RY, double A0, double A1, int32 Steps, bool bHardStart = true, bool bHardEnd = true)
		{
			for (int32 Step = 0; Step <= Steps; ++Step)
			{
				const double Ang = FMath::DegreesToRadians(A0 + (A1 - A0) * Step / Steps);
				const FVector2D Q = Centre + FVector2D(RX * FMath::Cos(Ang), RY * FMath::Sin(Ang));
				const bool bHardHere = Step == 0 ? bHardStart : (Step == Steps ? bHardEnd : false);
				if (Step == 0 && Pts.Num() > 0 && FVector2D::Distance(Pts.Last().P, Q) < 1e-6)
				{
					Pts.Last().bHard = bHardStart;
					continue;
				}
				Line(Q.X, Q.Y, bHardHere, BandFace);
			}
		}

		void Mark(uint8 Band) { Pts.Last().Band = Band; }

		void Finish()
		{
			const int32 NumPts = Pts.Num();
			SegN.SetNum(FMath::Max(0, NumPts - 1));
			Sigma.SetNum(NumPts);
			if (NumPts == 0) { return; }
			Sigma[0] = 0;
			for (int32 K = 0; K + 1 < NumPts; ++K)
			{
				const FVector2D D = Pts[K + 1].P - Pts[K].P;
				SegN[K] = FVector2D(-D.Y, D.X).GetSafeNormal();
				Sigma[K + 1] = Sigma[K] + D.Size();
			}
		}

		FVector2D StartN(int32 K) const
		{
			return (K == 0 || Pts[K].bHard) ? SegN[K] : (SegN[K - 1] + SegN[K]).GetSafeNormal();
		}

		FVector2D EndN(int32 K) const
		{
			return (K + 2 >= Pts.Num() || Pts[K + 1].bHard) ? SegN[K] : (SegN[K] + SegN[K + 1]).GetSafeNormal();
		}

		bool IsBackOnly(int32 I) const
		{
			return Pts[I].Band == BandBack && (I == 0 || Pts[I - 1].Band == BandBack);
		}
	};

	/** A height (h) as a function of s, sampled. */
	struct FHeightTable
	{
		double S0 = 0, S1 = 1;
		TArray<double> H;

		double At(double S) const
		{
			if (H.Num() < 2) { return 0; }
			const double F = FMath::Clamp((S - S0) / FMath::Max(1e-9, S1 - S0), 0.0, 1.0) * (H.Num() - 1);
			const int32 I0 = FMath::Min(int32(F), H.Num() - 2);
			return FMath::Lerp(H[I0], H[I0 + 1], F - I0);
		}
	};

	/** The frame's front surface as seen from the room: the highest point of the face at each s. */
	FHeightTable TopSurface(const FProfile& Prof, int32 Samples)
	{
		FHeightTable Table;
		double Lo = TNumericLimits<double>::Max(), Hi = -TNumericLimits<double>::Max();
		for (int32 I = 0; I < Prof.Pts.Num(); ++I)
		{
			if (Prof.IsBackOnly(I)) { continue; }
			Lo = FMath::Min(Lo, Prof.Pts[I].P.X);
			Hi = FMath::Max(Hi, Prof.Pts[I].P.X);
		}
		if (Hi <= Lo) { Hi = Lo + 0.01; }
		Table.S0 = Lo;
		Table.S1 = Hi;
		Table.H.Init(-1e9, Samples);
		const double Step = (Hi - Lo) / (Samples - 1);
		for (int32 K = 0; K + 1 < Prof.Pts.Num(); ++K)
		{
			if (Prof.Pts[K].Band == BandBack) { continue; }
			const FVector2D P0 = Prof.Pts[K].P, P1 = Prof.Pts[K + 1].P;
			const double SegLo = FMath::Min(P0.X, P1.X), SegHi = FMath::Max(P0.X, P1.X);
			const int32 J0 = FMath::Clamp(FMath::CeilToInt((SegLo - Lo) / Step), 0, Samples - 1);
			const int32 J1 = FMath::Clamp(FMath::FloorToInt((SegHi - Lo) / Step), 0, Samples - 1);
			if (J1 < J0)
			{
				const int32 JN = FMath::Clamp(FMath::RoundToInt((0.5 * (SegLo + SegHi) - Lo) / Step), 0, Samples - 1);
				Table.H[JN] = FMath::Max(Table.H[JN], FMath::Max(P0.Y, P1.Y));
				continue;
			}
			for (int32 J = J0; J <= J1; ++J)
			{
				const double S = Lo + Step * J;
				const double Hs = FMath::Abs(P1.X - P0.X) < 1e-9 ? FMath::Max(P0.Y, P1.Y)
					: FMath::Lerp(P0.Y, P1.Y, FMath::Clamp((S - P0.X) / (P1.X - P0.X), 0.0, 1.0));
				Table.H[J] = FMath::Max(Table.H[J], Hs);
			}
		}
		for (int32 J = 1; J < Samples; ++J) { if (Table.H[J] < -1e8) { Table.H[J] = Table.H[J - 1]; } }
		for (int32 J = Samples - 2; J >= 0; --J) { if (Table.H[J] < -1e8) { Table.H[J] = Table.H[J + 1]; } }
		return Table;
	}

	/** A smooth upper envelope of the surface, for ornament that bridges its steps and hollows. */
	FHeightTable Envelope(const FHeightTable& Top, double Radius)
	{
		FHeightTable Env = Top;
		const int32 Num = Top.H.Num();
		if (Num < 3) { return Env; }
		const double Step = (Top.S1 - Top.S0) / (Num - 1);
		const int32 Win = FMath::Max(2, FMath::CeilToInt(Radius / FMath::Max(1e-9, Step)));
		for (int32 J = 0; J < Num; ++J)
		{
			double Mx = -1e9;
			for (int32 Q = FMath::Max(0, J - Win); Q <= FMath::Min(Num - 1, J + Win); ++Q) { Mx = FMath::Max(Mx, Top.H[Q]); }
			Env.H[J] = Mx;
		}
		for (int32 Pass = 0; Pass < 2; ++Pass)
		{
			const TArray<double> Prev = Env.H;
			for (int32 J = 1; J + 1 < Num; ++J) { Env.H[J] = 0.25 * Prev[J - 1] + 0.5 * Prev[J] + 0.25 * Prev[J + 1]; }
		}
		for (int32 J = 0; J < Num; ++J) { Env.H[J] = FMath::Max(Env.H[J], Top.H[J]); }
		return Env;
	}

	/** How exposed each profile point is: 0 at the bottom of its neighbourhood (where dirt sits), 1 on top. */
	TArray<double> ProfileExposure(const FProfile& Prof, const FHeightTable& Top, double Radius)
	{
		TArray<double> Out;
		Out.SetNum(Prof.Pts.Num());
		for (int32 I = 0; I < Prof.Pts.Num(); ++I)
		{
			if (Prof.IsBackOnly(I)) { Out[I] = 0.3; continue; }
			const FVector2D P = Prof.Pts[I].P;
			double Lo = P.Y, Hi = P.Y;
			for (int32 J = 0; J <= 12; ++J)
			{
				const double Hs = Top.At(P.X - Radius + 2.0 * Radius * J / 12.0);
				Lo = FMath::Min(Lo, Hs);
				Hi = FMath::Max(Hi, Hs);
			}
			Out[I] = 0.2 + 0.8 * FMath::Clamp((P.Y - Lo) / FMath::Max(1e-6, Hi - Lo), 0.0, 1.0);
		}
		return Out;
	}

	// ------------------------------------------------------------------------------ the styles

	enum class ECornerKind : uint8 { None, Shell, Rosette };

	struct FOrnament
	{
		// Leaf tips running along a band of the profile (point indices), tips towards the sight.
		int32 LeafFrom = INDEX_NONE, LeafTo = INDEX_NONE;
		double LeafPitch = 0, LeafHeight = 0;
		double LeafRunEnd = 0;          // leaf tips run over |t| ≤ L + LeafRunEnd
		// Beads and reels on a half-round.
		bool bBeads = false;
		FVector2D RibbonCentre = FVector2D::ZeroVector;
		double RibbonRadius = 0, BeadPitch = 0, BeadRunEnd = 0;
		// Corners.
		ECornerKind Corner = ECornerKind::None;
		double ShellHinge = 0, ShellRadius = 0, ShellHeight = 0, ShellSpread = 60;
		int32 ShellFlutes = 9;
		double AcanthusS = 0, AcanthusFrom = 0, AcanthusLength = 0, AcanthusHalfWidth = 0, AcanthusHeight = 0;
		double RosetteS = 0, RosetteRadius = 0, RosetteHeight = 0;
		int32 RosettePetals = 8;
		// A cartouche at the middle of each side at least 2 × CentreMinHalfLength long.
		bool bCentre = false;
		double CentreRadius = 0, CentreHeight = 0, CentreSpread = 70, CentreMinHalfLength = 0;
		int32 CentreFlutes = 7;
		double CentreLeafFrom = 0, CentreLeafLength = 0, CentreLeafHalfWidth = 0, CentreLeafHeight = 0;
		double CentreClearLeaf = 0, CentreClearBead = 0;
	};

	struct FStyleSpec
	{
		FProfile Profile;
		FOrnament Orn;
		bool bGiltFace = true;
		double Outer = 0;          // s of the outside edge
	};

	/**
	 * W: moulding width, Base: the height the frame sits on (the canvas's face, or the mat's), Lip:
	 * how far the sight edge reaches over the canvas. Proportions from Barbizon / Louis XV frames.
	 */
	FStyleSpec MakeSpec(EMuseeFrameStyle Style, double W, double Base, double Lip)
	{
		FStyleSpec Spec;
		FProfile& Pr = Spec.Profile;
		FOrnament& Orn = Spec.Orn;
		auto U = [W](double X) { return X * W; };
		auto V = [W, Base](double Y) { return Base + Y * W; };

		if (Style == EMuseeFrameStyle::SalonGilt)
		{
			Pr.Line(0, 0, true, BandBack);                  // the rebate, behind the canvas
			Pr.Line(0, Base);                                // under the lip
			const double Rb = 0.04 * W;                      // the sight-edge bead
			Pr.Arc(FVector2D(-Lip + 0.95 * Rb, Base + 0.0015 + 0.34 * Rb), Rb, Rb, 200, -20, 8);
			Pr.Mark(BandSanded);
			Pr.Line(U(0.25), V(0.06));                       // the sanded frieze, rising a little
			Pr.Line(U(0.25), V(0.085));                      // step
			Pr.Arc(FVector2D(U(0.28), V(0.085)), 0.03 * W, 0.03 * W, 180, 60, 5);   // astragal
			Pr.Line(U(0.31), V(0.10));                       // quirk
			Pr.Arc(FVector2D(U(0.31), V(0.44)), 0.25 * W, 0.34 * W, -90, -12, 12);  // the deep cove
			Pr.Line(U(0.5545), V(0.40));                     // fillet up to the carved ogee
			const double Ro = 0.085 * W;
			const FVector2D OgeeStart(U(0.5545), V(0.40));
			const double A195 = FMath::DegreesToRadians(195.0);
			const FVector2D OgeeCentre = OgeeStart - FVector2D(Ro * FMath::Cos(A195), Ro * FMath::Sin(A195));
			Orn.LeafFrom = Pr.Pts.Num() - 1;
			Pr.Arc(OgeeCentre, Ro, Ro, 195, 25, 14);         // the carved ogee (leaf tips)
			Orn.LeafTo = Pr.Pts.Num() - 1;
			Pr.Line(U(0.722), V(0.425));                     // a hollow
			const double Rr = 0.06 * W;
			const FVector2D RibbonCentre(U(0.782), V(0.425));
			Pr.Arc(RibbonCentre, Rr, Rr, 180, 0, 12);        // the raised ribbon (bead and reel)
			Pr.Line(U(0.862), V(0.405));
			Pr.Arc(FVector2D(U(0.922), V(0.405)), 0.06 * W, 0.06 * W, 180, 270, 6);   // outer cavetto
			Pr.Arc(FVector2D(U(0.922), V(0.31)), 0.035 * W, 0.035 * W, 90, 0, 5);     // back-edge bead
			Pr.Line(U(0.957), 0, true, BandBack);            // the outside, down to the wall
			Pr.Line(0, 0, true, BandBack);                   // the back
			Spec.Outer = U(0.957);

			Orn.LeafPitch = 0.24 * W;
			Orn.LeafHeight = 0.03 * W;
			Orn.LeafRunEnd = -1.30 * W;
			Orn.bBeads = true;
			Orn.RibbonCentre = RibbonCentre;
			Orn.RibbonRadius = Rr;
			Orn.BeadPitch = 0.22 * W;
			Orn.BeadRunEnd = 0.06 * W;
			Orn.Corner = ECornerKind::Shell;
			Orn.ShellHinge = 0.30 * W;
			Orn.ShellRadius = 0.60 * W;
			Orn.ShellHeight = 0.10 * W;
			Orn.ShellSpread = 64;
			Orn.ShellFlutes = 9;
			Orn.AcanthusS = 0.55 * W;
			Orn.AcanthusFrom = 0.28 * W;
			Orn.AcanthusLength = 0.95 * W;
			Orn.AcanthusHalfWidth = 0.17 * W;
			Orn.AcanthusHeight = 0.07 * W;
			Orn.bCentre = true;
			Orn.CentreRadius = 0.46 * W;
			Orn.CentreHeight = 0.08 * W;
			Orn.CentreSpread = 72;
			Orn.CentreFlutes = 7;
			Orn.CentreMinHalfLength = 2.6 * W;
			Orn.CentreLeafFrom = 0.20 * W;
			Orn.CentreLeafLength = 0.50 * W;
			Orn.CentreLeafHalfWidth = 0.13 * W;
			Orn.CentreLeafHeight = 0.05 * W;
			Orn.CentreClearLeaf = 0.72 * W;
			Orn.CentreClearBead = 0.25 * W;
		}
		else if (Style == EMuseeFrameStyle::CabinetGilt)
		{
			Pr.Line(0, 0, true, BandBack);
			Pr.Line(0, Base);
			const double Rb = 0.06 * W;
			Pr.Arc(FVector2D(-Lip + 0.95 * Rb, Base + 0.0015 + 0.34 * Rb), Rb, Rb, 200, -20, 7);
			Pr.Mark(BandSanded);
			Pr.Line(U(0.28), V(0.05));                       // sanded flat
			Pr.Line(U(0.28), V(0.08));                       // step
			Pr.Arc(FVector2D(U(0.28), V(0.36)), 0.24 * W, 0.28 * W, -90, -10, 10);  // cove
			const double CoveEndS = 0.28 + 0.24 * FMath::Cos(FMath::DegreesToRadians(-10.0));
			Pr.Line(U(CoveEndS), V(0.34));                   // fillet
			Orn.LeafFrom = Pr.Pts.Num() - 1;
			Pr.Arc(FVector2D(U(CoveEndS + 0.16), V(0.34)), 0.16 * W, 0.16 * W, 180, 0, 14);   // the leaf-tip torus
			Orn.LeafTo = Pr.Pts.Num() - 1;
			Pr.Line(U(0.86), V(0.30));
			Pr.Arc(FVector2D(U(0.86), V(0.26)), 0.04 * W, 0.04 * W, 90, 0, 5);          // back-edge bead
			Pr.Line(U(0.90), 0, true, BandBack);
			Pr.Line(0, 0, true, BandBack);
			Spec.Outer = U(0.90);

			Orn.LeafPitch = 0.34 * W;
			Orn.LeafHeight = 0.045 * W;
			Orn.LeafRunEnd = 0.30 * W;
			Orn.Corner = ECornerKind::Rosette;
			Orn.RosetteS = 0.62 * W;
			Orn.RosetteRadius = 0.26 * W;
			Orn.RosetteHeight = 0.10 * W;
			Orn.RosettePetals = 8;
		}
		else if (Style == EMuseeFrameStyle::ReededGilt)
		{
			// Albion: the reeded frame (Rossetti, Madox Brown): flat gilded oak, reeds and roundels.
			Pr.Line(0, 0, true, BandBack);
			Pr.Line(0, Base);
			const double Rb = 0.035 * W;
			Pr.Arc(FVector2D(-Lip + 0.95 * Rb, Base + 0.0015 + 0.34 * Rb), Rb, Rb, 200, -20, 7);
			Pr.Mark(BandSanded);
			Pr.Line(U(0.26), V(0.10));                       // the inner flat, rising
			for (int32 R = 0; R < 4; ++R)                     // four reeds
			{
				const double S0 = 0.26 + 0.07 * R;
				Pr.Arc(FVector2D(U(S0 + 0.035), V(0.10)), 0.035 * W, 0.035 * W, 180, 0, 8, true, true);
			}
			Pr.Line(U(0.56), V(0.13));                       // the broad flat (the roundels sit on it)
			Pr.Mark(BandSanded);
			Pr.Line(U(0.86), V(0.13));
			for (int32 R = 0; R < 2; ++R)                     // two outer reeds
			{
				const double S0 = 0.86 + 0.06 * R;
				Pr.Arc(FVector2D(U(S0 + 0.03), V(0.13)), 0.03 * W, 0.03 * W, 180, 0, 7, true, true);
			}
			Pr.Line(U(0.99), V(0.13));
			Pr.Arc(FVector2D(U(0.99), V(0.10)), 0.03 * W, 0.03 * W, 90, 0, 4);
			Pr.Line(U(1.02), 0, true, BandBack);
			Pr.Line(0, 0, true, BandBack);
			Spec.Outer = U(1.02);
			Orn.Corner = ECornerKind::Rosette;
			Orn.RosetteS = 0.71 * W;
			Orn.RosetteRadius = 0.13 * W;
			Orn.RosetteHeight = 0.05 * W;
			Orn.RosettePetals = 16;
		}
		else if (Style == EMuseeFrameStyle::WattsGilt)
		{
			// Albion: a Watts frame: a sight moulding, a broad sanded cassetta, an outer carved ogee.
			Pr.Line(0, 0, true, BandBack);
			Pr.Line(0, Base);
			const double Rb = 0.03 * W;
			Pr.Arc(FVector2D(-Lip + 0.95 * Rb, Base + 0.0015 + 0.34 * Rb), Rb, Rb, 200, -20, 7);
			Pr.Line(U(0.07), V(0.06));
			Pr.Arc(FVector2D(U(0.10), V(0.06)), 0.03 * W, 0.03 * W, 180, 0, 6);   // a small torus
			Pr.Line(U(0.14), V(0.05));
			Pr.Mark(BandSanded);
			Pr.Line(U(0.60), V(0.07));                       // the cassetta's flat
			Pr.Line(U(0.60), V(0.10));
			Orn.LeafFrom = Pr.Pts.Num() - 1;
			Pr.Arc(FVector2D(U(0.72), V(0.10)), 0.12 * W, 0.16 * W, 180, 60, 12);   // the carved ogee
			Orn.LeafTo = Pr.Pts.Num() - 1;
			Pr.Line(U(0.86), V(0.24));
			const double Rr = 0.04 * W;
			const FVector2D RibbonCentre(U(0.90), V(0.24));
			Pr.Arc(RibbonCentre, Rr, Rr, 180, 0, 10);       // beads
			Pr.Line(U(0.96), V(0.20));
			Pr.Arc(FVector2D(U(0.96), V(0.16)), 0.04 * W, 0.04 * W, 90, 0, 5);
			Pr.Line(U(1.0), 0, true, BandBack);
			Pr.Line(0, 0, true, BandBack);
			Spec.Outer = U(1.0);
			Orn.LeafPitch = 0.20 * W;
			Orn.LeafHeight = 0.03 * W;
			Orn.LeafRunEnd = 0.2 * W;
			Orn.bBeads = true;
			Orn.RibbonCentre = RibbonCentre;
			Orn.RibbonRadius = Rr;
			Orn.BeadPitch = 0.14 * W;
			Orn.BeadRunEnd = 0.05 * W;
			Orn.Corner = ECornerKind::Rosette;
			Orn.RosetteS = 0.36 * W;
			Orn.RosetteRadius = 0.12 * W;
			Orn.RosetteHeight = 0.05 * W;
			Orn.RosettePetals = 8;
		}
		else   // Photograph: a flat dark oak moulding with softened edges
		{
			Spec.bGiltFace = false;
			const double Face = Base + FMath::Max(0.016, 0.8 * W);
			Pr.Line(0, 0, true, BandBack);
			Pr.Line(0, Base);
			Pr.Line(-Lip, Base + 0.001);
			Pr.Line(-Lip, Face - 0.003);
			Pr.Arc(FVector2D(-Lip + 0.003, Face - 0.003), 0.003, 0.003, 180, 90, 4, false, false);
			Pr.Line(W - 0.004, Face + 0.0005, false);
			Pr.Arc(FVector2D(W - 0.004, Face - 0.0035), 0.004, 0.004, 90, 0, 4, false, true);
			Pr.Line(W, 0, true, BandBack);
			Pr.Line(0, 0, true, BandBack);
			Spec.Outer = W;
		}
		return Spec;
	}

	// ------------------------------------------------------------------------------ sweeping

	/** One side of the sight rectangle: outward O, along T (in the YZ plane), half-length L, offset Off. */
	struct FSide
	{
		FVector O;
		FVector T;
		double L;
		double Off;
	};

	FSide MakeSide(int32 Index, double A, double B)
	{
		switch (Index)
		{
		case 0: return {FVector(0, 0, 1), FVector(0, 1, 0), A, B};     // top
		case 1: return {FVector(0, 1, 0), FVector(0, 0, -1), B, A};    // +Y
		case 2: return {FVector(0, 0, -1), FVector(0, -1, 0), A, B};   // bottom
		default: return {FVector(0, -1, 0), FVector(0, 0, 1), B, A};   // −Y
		}
	}

	FVector OnSide(const FSide& Sd, double S, double Along, double H)
	{
		return Sd.O * (Sd.Off + S) + Sd.T * Along + FVector::XAxisVector * H;
	}

	FVector NormalOnSide(const FSide& Sd, const FVector2D& N2)
	{
		return Sd.O * N2.X + FVector::XAxisVector * N2.Y;
	}

	FVector2D InPlane(const FVector& V3) { return FVector2D(V3.Y, V3.Z); }

	void SweepProfile(const FProfile& Prof, double A, double B, const TArray<double>& Exposure, FSectionBuilder& Face, FSectionBuilder& Back)
	{
		for (int32 SideIndex = 0; SideIndex < 4; ++SideIndex)
		{
			const FSide Sd = MakeSide(SideIndex, A, B);
			for (int32 K = 0; K + 1 < Prof.Pts.Num(); ++K)
			{
				const FProfilePoint& P0 = Prof.Pts[K];
				const FProfilePoint& P1 = Prof.Pts[K + 1];
				FSectionBuilder& Out = P0.Band == BandBack ? Back : Face;
				const bool bSanded = P0.Band == BandSanded;
				const double T0 = Sd.L + P0.P.X, T1 = Sd.L + P1.P.X;
				if (T0 <= 0 || T1 <= 0) { continue; }
				const FVector N0 = NormalOnSide(Sd, Prof.StartN(K)), N1 = NormalOnSide(Sd, Prof.EndN(K));
				const FColor C0 = Paint(Exposure[K], bSanded, false), C1 = Paint(Exposure[K + 1], bSanded, false);
				const double V0 = Prof.Sigma[K], V1 = Prof.Sigma[K + 1];
				const int32 I00 = Out.Vert(OnSide(Sd, P0.P.X, -T0, P0.P.Y), N0, FVector2D(-T0, V0), C0, Sd.T);
				const int32 I01 = Out.Vert(OnSide(Sd, P0.P.X, T0, P0.P.Y), N0, FVector2D(T0, V0), C0, Sd.T);
				const int32 I11 = Out.Vert(OnSide(Sd, P1.P.X, T1, P1.P.Y), N1, FVector2D(T1, V1), C1, Sd.T);
				const int32 I10 = Out.Vert(OnSide(Sd, P1.P.X, -T1, P1.P.Y), N1, FVector2D(-T1, V1), C1, Sd.T);
				Out.Quad(I00, I01, I11, I10);
			}
		}
	}

	/** A flat or sloping ring between two rectangles (a mat, a liner, a bevel), in the YZ plane. */
	void RectRing(FSectionBuilder& Out, double InA, double InB, double InH, double OutA, double OutB, double OutH, double Exposure)
	{
		for (int32 SideIndex = 0; SideIndex < 4; ++SideIndex)
		{
			const FSide Inner = MakeSide(SideIndex, InA, InB);
			const FSide Outer = MakeSide(SideIndex, OutA, OutB);
			const FVector2D Prof(Outer.Off - Inner.Off, OutH - InH);
			const FVector N = NormalOnSide(Inner, FVector2D(-Prof.Y, Prof.X).GetSafeNormal());
			const FColor Col = Paint(Exposure, false, false);
			const int32 I0 = Out.Vert(OnSide(Inner, 0, -Inner.L, InH), N, FVector2D(-Inner.L, 0), Col, Inner.T);
			const int32 I1 = Out.Vert(OnSide(Inner, 0, Inner.L, InH), N, FVector2D(Inner.L, 0), Col, Inner.T);
			const int32 I2 = Out.Vert(OnSide(Outer, 0, Outer.L, OutH), N, FVector2D(Outer.L, Prof.X), Col, Inner.T);
			const int32 I3 = Out.Vert(OnSide(Outer, 0, -Outer.L, OutH), N, FVector2D(-Outer.L, Prof.X), Col, Inner.T);
			Out.Quad(I0, I1, I2, I3);
		}
	}

	void Box(FSectionBuilder& Out, const FVector& Lo, const FVector& Hi)
	{
		const FColor Col = Paint(0.5, false, false);
		for (int32 Axis = 0; Axis < 3; ++Axis)
		{
			for (int32 Sign = -1; Sign <= 1; Sign += 2)
			{
				FVector N = FVector::ZeroVector;
				N[Axis] = Sign;
				const int32 UA = (Axis + 1) % 3, VA = (Axis + 2) % 3;
				FVector Corner[4];
				for (int32 C = 0; C < 4; ++C)
				{
					Corner[C][Axis] = Sign > 0 ? Hi[Axis] : Lo[Axis];
					Corner[C][UA] = (C == 1 || C == 2) ? Hi[UA] : Lo[UA];
					Corner[C][VA] = (C >= 2) ? Hi[VA] : Lo[VA];
				}
				const int32 I0 = Out.Vert(Corner[0], N, FVector2D(0, 0), Col, FVector::YAxisVector);
				const int32 I1 = Out.Vert(Corner[1], N, FVector2D(1, 0), Col, FVector::YAxisVector);
				const int32 I2 = Out.Vert(Corner[2], N, FVector2D(1, 1), Col, FVector::YAxisVector);
				const int32 I3 = Out.Vert(Corner[3], N, FVector2D(0, 1), Col, FVector::YAxisVector);
				Out.Quad(I0, I1, I2, I3);
			}
		}
	}

	// ------------------------------------------------------------------------------ ornament

	/** A band of the profile (points From..To), evaluated by arc length with smooth normals. */
	struct FBand
	{
		const FProfile& Prof;
		int32 From;
		int32 To;

		double Sigma0() const { return Prof.Sigma[From]; }
		double Sigma1() const { return Prof.Sigma[To]; }

		void Eval(double Sg, FVector2D& OutP, FVector2D& OutN) const
		{
			int32 K = From;
			while (K + 1 < To && Prof.Sigma[K + 1] < Sg) { ++K; }
			const double Len = FMath::Max(1e-9, Prof.Sigma[K + 1] - Prof.Sigma[K]);
			const double F = FMath::Clamp((Sg - Prof.Sigma[K]) / Len, 0.0, 1.0);
			OutP = FMath::Lerp(Prof.Pts[K].P, Prof.Pts[K + 1].P, F);
			OutN = FMath::Lerp(Prof.StartN(K), Prof.EndN(K), F).GetSafeNormal();
		}
	};

	/** Items at a regular pitch filling |t| ≤ Half, leaving |t| < Clear free; Item(t, pitch), Boundary(t, pitch). */
	template <typename ItemFn, typename BoundaryFn>
	void ForEachInRuns(double Half, double Clear, double Pitch, const ItemFn& Item, const BoundaryFn& Boundary)
	{
		if (Pitch <= 0 || Half <= 0) { return; }
		TArray<FVector2D, TInlineAllocator<2>> Runs;
		if (Clear > 0) { Runs.Add(FVector2D(-Half, -Clear)); Runs.Add(FVector2D(Clear, Half)); }
		else { Runs.Add(FVector2D(-Half, Half)); }
		for (const FVector2D& Run : Runs)
		{
			const double Len = Run.Y - Run.X;
			if (Len < 0.6 * Pitch) { continue; }
			const int32 Count = FMath::Max(1, FMath::RoundToInt(Len / Pitch));
			const double P = Len / Count;
			for (int32 I = 0; I < Count; ++I) { Item(Run.X + P * (I + 0.5), P); }
			for (int32 I = 0; I <= Count; ++I) { Boundary(Run.X + P * I, P); }
		}
	}

	/** A leaf tip carved on a band of one side, displaced along the moulding's normal; it sinks into the surface at its edge. */
	void LeafTip(FSectionBuilder& Out, const FSide& Sd, const FBand& Band, double TCentre, double HalfWidth, double Height, int32 NA, int32 NB)
	{
		auto Pos = [&](double Pa, double Pb, FVector* OutBaseN)
		{
			const double Sg = FMath::Lerp(Band.Sigma0(), Band.Sigma1(), 0.05 + 0.9 * Pa);
			FVector2D P2, N2;
			Band.Eval(Sg, P2, N2);
			const double Shape = FMath::Pow(Pa, 0.55) * FMath::Sqrt(FMath::Max(0.0, 1.0 - Pa * Pa * Pa));
			const double Along = TCentre + Pb * HalfWidth * Shape;
			const double Across = FMath::Sqrt(FMath::Max(0.0, 1.0 - Pb * Pb));
			const double Midrib = 1.0 - 0.35 * FMath::Exp(-FMath::Square(Pb / 0.18));
			const double D = Height * Across * FMath::Pow(FMath::Sin(UE_DOUBLE_PI * Pa), 0.6) * Midrib - 0.0004;
			const FVector BaseN = NormalOnSide(Sd, N2);
			if (OutBaseN) { *OutBaseN = BaseN; }
			return OnSide(Sd, P2.X, Along, P2.Y) + BaseN * D;
		};
		const double Eps = 2e-3;
		TArray<int32> Grid;
		Grid.SetNum((NA + 1) * (NB + 1));
		for (int32 IA = 0; IA <= NA; ++IA)
		{
			for (int32 IB = 0; IB <= NB; ++IB)
			{
				const double Pa = double(IA) / NA, Pb = -1.0 + 2.0 * IB / NB;
				FVector BaseN;
				const FVector P = Pos(Pa, Pb, &BaseN);
				const FVector DA = Pos(FMath::Min(1.0, Pa + Eps), Pb, nullptr) - Pos(FMath::Max(0.0, Pa - Eps), Pb, nullptr);
				const FVector DB = Pos(Pa, FMath::Min(1.0, Pb + Eps), nullptr) - Pos(Pa, FMath::Max(-1.0, Pb - Eps), nullptr);
				FVector N = FVector::CrossProduct(DA.GetSafeNormal(1e-30), DB.GetSafeNormal(1e-30));
				if (N.SizeSquared() < 1e-6) { N = BaseN; }
				if (FVector::DotProduct(N, BaseN) < 0) { N = -N; }
				const double Rel = FMath::Clamp(FMath::Sqrt(FMath::Max(0.0, 1.0 - Pb * Pb)) * FMath::Sin(UE_DOUBLE_PI * Pa), 0.0, 1.0);
				Grid[IA * (NB + 1) + IB] = Out.Vert(P, N, FVector2D(TCentre + Pb * HalfWidth, Pa * 0.02), Paint(0.45 + 0.55 * Rel, false, true), Sd.T);
			}
		}
		for (int32 IA = 0; IA < NA; ++IA)
		{
			for (int32 IB = 0; IB < NB; ++IB)
			{
				const int32 G00 = Grid[IA * (NB + 1) + IB], G01 = Grid[IA * (NB + 1) + IB + 1];
				const int32 G10 = Grid[(IA + 1) * (NB + 1) + IB], G11 = Grid[(IA + 1) * (NB + 1) + IB + 1];
				Out.Quad(G00, G01, G11, G10);
			}
		}
	}

	/** Part of a spheroid (a bead, a reel): long axis Along, round Up (φ = 0) towards Outward. */
	void Spheroid(FSectionBuilder& Out, const FVector& Centre, const FVector& Along, const FVector& Up, const FVector& Outward,
				  double HalfLen, double Radius, double PhiMaxDeg, int32 NPsi, int32 NPhi)
	{
		TArray<int32> Grid;
		Grid.SetNum((NPsi + 1) * (NPhi + 1));
		const double PhiMax = FMath::DegreesToRadians(PhiMaxDeg);
		for (int32 IPsi = 0; IPsi <= NPsi; ++IPsi)
		{
			const double Psi = -0.5 * UE_DOUBLE_PI + UE_DOUBLE_PI * IPsi / NPsi;
			for (int32 IPhi = 0; IPhi <= NPhi; ++IPhi)
			{
				const double Phi = -PhiMax + 2.0 * PhiMax * IPhi / NPhi;
				const FVector Radial = Up * FMath::Cos(Phi) + Outward * FMath::Sin(Phi);
				const FVector P = Centre + Along * (HalfLen * FMath::Sin(Psi)) + Radial * (Radius * FMath::Cos(Psi));
				const FVector N = Along * (FMath::Sin(Psi) / HalfLen) + Radial * (FMath::Cos(Psi) / Radius);
				const double Exposure = 0.45 + 0.55 * FMath::Max(0.0, FMath::Cos(Phi) * FMath::Cos(Psi));
				Grid[IPsi * (NPhi + 1) + IPhi] = Out.Vert(P, N, FVector2D(Psi * HalfLen, Phi * Radius), Paint(Exposure, false, true), Along);
			}
		}
		for (int32 IPsi = 0; IPsi < NPsi; ++IPsi)
		{
			for (int32 IPhi = 0; IPhi < NPhi; ++IPhi)
			{
				Out.Quad(Grid[IPsi * (NPhi + 1) + IPhi], Grid[IPsi * (NPhi + 1) + IPhi + 1],
						 Grid[(IPsi + 1) * (NPhi + 1) + IPhi + 1], Grid[(IPsi + 1) * (NPhi + 1) + IPhi]);
			}
		}
	}

	/** What the plane reliefs stand on: the envelope of the frame's surface, s = max(|y| − A, |z| − B). */
	struct FGround
	{
		const FHeightTable& Env;
		double A;
		double B;
		double SkirtH;     // the skirts go down to here, inside the frame

		double At(const FVector2D& YZ) const { return Env.At(FMath::Max(FMath::Abs(YZ.X) - A, FMath::Abs(YZ.Y) - B)); }
	};

	/**
	 * A carved relief laid on the frame (it may cross a mitre): a height field over Foot(a, b) (a in
	 * [0, 1], b in [−1, 1]) at Lift above the envelope, with a skirt round its edge down into the frame,
	 * like the undercut edge of carving or applied compo.
	 */
	template <typename FootFn, typename LiftFn>
	void PlaneRelief(FSectionBuilder& Out, const FGround& Ground, int32 NA, int32 NB, const FootFn& Foot, const LiftFn& Lift, double LiftMax, bool bSkirtSides)
	{
		auto Pos = [&](double Pa, double Pb)
		{
			const FVector2D YZ = Foot(Pa, Pb);
			return FVector(Ground.At(YZ) + Lift(Pa, Pb), YZ.X, YZ.Y);
		};
		const double Eps = 2e-3;
		TArray<int32> Grid;
		TArray<FVector> Top;
		Grid.SetNum((NA + 1) * (NB + 1));
		Top.SetNum((NA + 1) * (NB + 1));
		FVector Centroid = FVector::ZeroVector;
		for (int32 IA = 0; IA <= NA; ++IA)
		{
			for (int32 IB = 0; IB <= NB; ++IB)
			{
				const double Pa = double(IA) / NA, Pb = -1.0 + 2.0 * IB / NB;
				const FVector P = Pos(Pa, Pb);
				const FVector DA = Pos(FMath::Min(1.0, Pa + Eps), Pb) - Pos(FMath::Max(0.0, Pa - Eps), Pb);
				const FVector DB = Pos(Pa, FMath::Min(1.0, Pb + Eps)) - Pos(Pa, FMath::Max(-1.0, Pb - Eps));
				FVector N = FVector::CrossProduct(DA.GetSafeNormal(1e-30), DB.GetSafeNormal(1e-30));
				if (N.SizeSquared() < 1e-6) { N = FVector::XAxisVector; }
				if (N.X < 0) { N = -N; }
				const double Exposure = 0.35 + 0.65 * FMath::Clamp(Lift(Pa, Pb) / FMath::Max(1e-6, LiftMax), 0.0, 1.0);
				const int32 Index = IA * (NB + 1) + IB;
				Top[Index] = P;
				Grid[Index] = Out.Vert(P, N, FVector2D(P.Y, P.Z), Paint(Exposure, false, true), DA);
				Centroid += P;
			}
		}
		Centroid /= Top.Num();
		for (int32 IA = 0; IA < NA; ++IA)
		{
			for (int32 IB = 0; IB < NB; ++IB)
			{
				Out.Quad(Grid[IA * (NB + 1) + IB], Grid[IA * (NB + 1) + IB + 1], Grid[(IA + 1) * (NB + 1) + IB + 1], Grid[(IA + 1) * (NB + 1) + IB]);
			}
		}

		// The skirt, round the boundary of the grid.
		TArray<int32> Loop;
		for (int32 IB = 0; IB <= NB; ++IB) { Loop.Add(IB); }                                     // a = 0
		for (int32 IA = 1; IA <= NA; ++IA) { Loop.Add(IA * (NB + 1) + NB); }                     // b = 1
		for (int32 IB = NB - 1; IB >= 0; --IB) { Loop.Add(NA * (NB + 1) + IB); }                 // a = 1
		for (int32 IA = NA - 1; IA >= 1; --IA) { Loop.Add(IA * (NB + 1)); }                      // b = −1
		const int32 Corner0 = NB, Corner1 = NB + NA, Corner2 = 2 * NB + NA;                     // loop positions of the grid corners
		for (int32 L = 0; L < Loop.Num(); ++L)
		{
			const int32 Next = (L + 1) % Loop.Num();
			const bool bOnBSide = (L >= Corner0 && L < Corner1) || (L >= Corner2);
			if (!bSkirtSides && bOnBSide) { continue; }
			const FVector P = Top[Loop[L]], Q = Top[Loop[Next]];
			const FVector Edge = Q - P;
			if (FVector2D(Edge.Y, Edge.Z).SizeSquared() < 1e-12) { continue; }
			if (P.X <= Ground.SkirtH && Q.X <= Ground.SkirtH) { continue; }
			FVector N(0, -Edge.Z, Edge.Y);
			N.Normalize();
			const FVector Mid = 0.5 * (P + Q);
			if (FVector::DotProduct(N, FVector(0, Mid.Y - Centroid.Y, Mid.Z - Centroid.Z)) < 0) { N = -N; }
			const FColor Col = Paint(0.2, false, true);
			const FVector Pb(Ground.SkirtH, P.Y, P.Z), Qb(Ground.SkirtH, Q.Y, Q.Z);
			const int32 I0 = Out.Vert(P, N, FVector2D(0, P.X), Col, Edge);
			const int32 I1 = Out.Vert(Q, N, FVector2D(Edge.Size(), Q.X), Col, Edge);
			const int32 I2 = Out.Vert(Qb, N, FVector2D(Edge.Size(), Qb.X), Col, Edge);
			const int32 I3 = Out.Vert(Pb, N, FVector2D(0, Pb.X), Col, Edge);
			Out.Quad(I0, I1, I2, I3);
		}
	}

	double Flute(double Pb, int32 Flutes)
	{
		const double Phase = (Pb + 1.0) * 0.5 * Flutes;
		return 0.5 - 0.5 * FMath::Cos(2.0 * UE_DOUBLE_PI * Phase);
	}

	/** A scallop shell (coquille): flutes fanning out from a hinge along Dir, a scalloped rolled lip. */
	void Shell(FSectionBuilder& Out, const FGround& Ground, const FVector2D& Hinge, const FVector2D& Dir, const FVector2D& Perp,
			   double Radius, double Height, double SpreadDeg, int32 Flutes, int32 NA, int32 NB)
	{
		const double Spread = FMath::DegreesToRadians(SpreadDeg);
		auto Foot = [&](double Pa, double Pb)
		{
			const double Th = Spread * Pb;
			const double R = Radius * (0.88 + 0.12 * Flute(Pb, Flutes));
			return Hinge + (Dir * FMath::Cos(Th) + Perp * FMath::Sin(Th)) * (R * Pa);
		};
		auto Lift = [&](double Pa, double Pb)
		{
			const double Ridge = FMath::Pow(Flute(Pb, Flutes), 0.7);
			const double Radial = FMath::Pow(FMath::Sin(0.5 * UE_DOUBLE_PI * Pa), 0.8) * (1.0 - 0.5 * FMath::SmoothStep(0.8, 1.0, Pa));
			const double Lateral = FMath::Sqrt(FMath::Max(0.0, 1.0 - FMath::Pow(FMath::Abs(Pb), 6.0)));
			return Height * (0.15 + 0.85 * Radial * (0.55 + 0.45 * Ridge)) * Lateral;
		};
		PlaneRelief(Out, Ground, NA, NB, Foot, Lift, Height, true);
	}

	/** An acanthus leaf lying along a side from Origin + U·From, lobed, with a midrib; its tip curls towards the sight. */
	void Acanthus(FSectionBuilder& Out, const FGround& Ground, const FVector2D& Origin, const FVector2D& U, const FVector2D& Outward,
				  double S, double From, double Length, double HalfWidth, double Height, int32 NA, int32 NB)
	{
		auto Foot = [&](double Pa, double Pb)
		{
			const double Dist = From + Length * Pa;
			const double Lobes = 1.0 - 0.22 * (0.5 - 0.5 * FMath::Cos(2.0 * UE_DOUBLE_PI * 3.5 * Pa));
			const double Hw = HalfWidth * FMath::Pow(FMath::Sin(UE_DOUBLE_PI * (0.12 + 0.88 * Pa)), 0.8) * Lobes;
			const double Sc = S - 0.08 * Length * Pa * Pa;
			return Origin + U * Dist + Outward * (Sc + Pb * Hw);
		};
		auto Lift = [&](double Pa, double Pb)
		{
			const double Across = FMath::Sqrt(FMath::Max(0.0, 1.0 - Pb * Pb));
			const double Along = 0.35 + 0.65 * FMath::Pow(FMath::Sin(UE_DOUBLE_PI * FMath::Min(1.0, 0.1 + Pa)), 0.7);
			const double Rib = 1.0 - 0.3 * FMath::Exp(-FMath::Square(Pb / 0.15));
			const double Veins = 1.0 - 0.12 * FMath::Square(FMath::Sin(UE_DOUBLE_PI * 7.0 * Pa)) * FMath::Abs(Pb);
			return Height * Across * Along * Rib * Veins;
		};
		PlaneRelief(Out, Ground, NA, NB, Foot, Lift, Height, true);
	}

	/** A round flower (the cabinet frames' corners): petals round a boss. */
	void Rosette(FSectionBuilder& Out, const FGround& Ground, const FVector2D& Centre, double Radius, double Height, int32 Petals, int32 NA, int32 NB)
	{
		auto Petal = [Petals](double Pb) { return FMath::Abs(FMath::Cos(0.5 * Petals * UE_DOUBLE_PI * Pb)); };
		auto Foot = [&](double Pa, double Pb)
		{
			const double Th = UE_DOUBLE_PI * Pb;
			return Centre + FVector2D(FMath::Cos(Th), FMath::Sin(Th)) * (Radius * (0.8 + 0.2 * Petal(Pb)) * Pa);
		};
		auto Lift = [&](double Pa, double Pb)
		{
			const double Dome = FMath::Sqrt(FMath::Max(0.0, 1.0 - FMath::Pow(Pa, 4.0)));
			const double Boss = 0.4 * FMath::Exp(-FMath::Square(Pa / 0.25));
			return Height * (0.2 + 0.5 * Dome * (0.6 + 0.4 * Petal(Pb)) + Boss);
		};
		PlaneRelief(Out, Ground, NA, NB, Foot, Lift, 1.1 * Height, false);
	}

	void AddOrnament(const FStyleSpec& Spec, double A, double B, const FGround& Ground, double Detail, FSectionBuilder& Out)
	{
		const FOrnament& Orn = Spec.Orn;
		auto Res = [Detail](int32 Base) { return FMath::Max(2, FMath::RoundToInt(Base * Detail)); };
		for (int32 SideIndex = 0; SideIndex < 4; ++SideIndex)
		{
			const FSide Sd = MakeSide(SideIndex, A, B);
			const bool bCentreHere = Orn.bCentre && Sd.L >= Orn.CentreMinHalfLength;

			if (Orn.LeafFrom != INDEX_NONE && Orn.LeafTo > Orn.LeafFrom)
			{
				const FBand Band{Spec.Profile, Orn.LeafFrom, Orn.LeafTo};
				ForEachInRuns(Sd.L + Orn.LeafRunEnd, bCentreHere ? Orn.CentreClearLeaf : 0.0, Orn.LeafPitch,
					[&](double TCentre, double Pitch) { LeafTip(Out, Sd, Band, TCentre, 0.62 * Pitch, Orn.LeafHeight, Res(8), Res(6)); },
					[](double, double) {});
			}

			if (Orn.bBeads)
			{
				const double Rr = Orn.RibbonRadius;
				ForEachInRuns(Sd.L + Orn.BeadRunEnd, bCentreHere ? Orn.CentreClearBead : 0.0, Orn.BeadPitch,
					[&](double TCentre, double Pitch)
					{
						const FVector C = OnSide(Sd, Orn.RibbonCentre.X, TCentre, Orn.RibbonCentre.Y);
						Spheroid(Out, C, Sd.T, FVector::XAxisVector, Sd.O, 0.30 * Pitch, 1.12 * Rr, 105, Res(6), Res(5));
					},
					[&](double TEdge, double Pitch)
					{
						for (int32 Disc = -1; Disc <= 1; Disc += 2)
						{
							const FVector C = OnSide(Sd, Orn.RibbonCentre.X, TEdge + Disc * 0.085 * Pitch, Orn.RibbonCentre.Y);
							Spheroid(Out, C, Sd.T, FVector::XAxisVector, Sd.O, 0.05 * Pitch, 1.05 * Rr, 100, Res(2), Res(5));
						}
					});
			}

			// The corner at this side's +T end, shared with the next side's −T end.
			const FVector2D O2 = InPlane(Sd.O), T2 = InPlane(Sd.T);
			const FVector2D Corner = O2 * Sd.Off + T2 * Sd.L;
			if (Orn.Corner == ECornerKind::Shell)
			{
				const FSide NextSide = MakeSide((SideIndex + 1) % 4, A, B);
				Shell(Out, Ground, Corner + (O2 + T2) * Orn.ShellHinge, (O2 + T2).GetSafeNormal(), (O2 - T2).GetSafeNormal(),
					  Orn.ShellRadius, Orn.ShellHeight, Orn.ShellSpread, Orn.ShellFlutes, Res(10), Res(4 * Orn.ShellFlutes));
				if (Sd.L > Orn.AcanthusFrom + 0.6 * Orn.AcanthusLength)
				{
					Acanthus(Out, Ground, Corner, -T2, O2, Orn.AcanthusS, Orn.AcanthusFrom, FMath::Min(Orn.AcanthusLength, Sd.L - Orn.AcanthusFrom),
							 Orn.AcanthusHalfWidth, Orn.AcanthusHeight, Res(14), Res(6));
				}
				if (NextSide.L > Orn.AcanthusFrom + 0.6 * Orn.AcanthusLength)
				{
					Acanthus(Out, Ground, Corner, InPlane(NextSide.T), InPlane(NextSide.O), Orn.AcanthusS, Orn.AcanthusFrom,
							 FMath::Min(Orn.AcanthusLength, NextSide.L - Orn.AcanthusFrom), Orn.AcanthusHalfWidth, Orn.AcanthusHeight, Res(14), Res(6));
				}
			}
			else if (Orn.Corner == ECornerKind::Rosette)
			{
				Rosette(Out, Ground, Corner + (O2 + T2) * Orn.RosetteS, Orn.RosetteRadius, Orn.RosetteHeight, Orn.RosettePetals, Res(6), Res(4 * Orn.RosettePetals));
			}

			if (bCentreHere)
			{
				const FVector2D Mid = O2 * Sd.Off;
				Shell(Out, Ground, Mid + O2 * Orn.ShellHinge, O2, T2, Orn.CentreRadius, Orn.CentreHeight, Orn.CentreSpread, Orn.CentreFlutes,
					  Res(9), Res(4 * Orn.CentreFlutes));
				for (int32 Sign = -1; Sign <= 1; Sign += 2)
				{
					Acanthus(Out, Ground, Mid, T2 * double(Sign), O2, Orn.AcanthusS, Orn.CentreLeafFrom, Orn.CentreLeafLength,
							 Orn.CentreLeafHalfWidth, Orn.CentreLeafHeight, Res(10), Res(5));
				}
			}
		}
	}

	// ------------------------------------------------------------------------------ measuring

	/** World-space vertices of an actor's static meshes (LOD 0), and the sum of their normals. */
	void GatherVertices(const AActor* Source, TArray<FVector>& OutPoints, FVector* OutNormalSum)
	{
		if (!Source) { return; }
		TInlineComponentArray<UStaticMeshComponent*> Comps(Source);
		for (const UStaticMeshComponent* Comp : Comps)
		{
			if (!Comp) { continue; }
			const UStaticMesh* SM = Comp->GetStaticMesh();
			const FTransform& ToWorld = Comp->GetComponentTransform();
			const FStaticMeshRenderData* RD = SM ? SM->GetRenderData() : nullptr;
			bool bRead = false;
			if (RD && RD->LODResources.Num() > 0)
			{
				const FStaticMeshLODResources& LOD = RD->LODResources[0];
				const FPositionVertexBuffer& PosBuf = LOD.VertexBuffers.PositionVertexBuffer;
				const FStaticMeshVertexBuffer& VtxBuf = LOD.VertexBuffers.StaticMeshVertexBuffer;
				if (PosBuf.GetNumVertices() > 0 && PosBuf.GetVertexData() != nullptr)
				{
					const FMatrix NormalToWorld = ToWorld.ToMatrixWithScale().Inverse().GetTransposed();
					const bool bNormals = OutNormalSum && VtxBuf.GetTangentData() != nullptr && VtxBuf.GetNumVertices() == PosBuf.GetNumVertices();
					for (uint32 I = 0; I < PosBuf.GetNumVertices(); ++I)
					{
						const FVector3f& Local = PosBuf.VertexPosition(I);
						OutPoints.Add(ToWorld.TransformPosition(FVector(Local.X, Local.Y, Local.Z)));
						if (bNormals)
						{
							const FVector4f TZ = VtxBuf.VertexTangentZ(I);
							const FVector4 NW = NormalToWorld.TransformVector(FVector(TZ.X, TZ.Y, TZ.Z));
							*OutNormalSum += FVector(NW.X, NW.Y, NW.Z).GetSafeNormal();
						}
					}
					bRead = true;
				}
			}
			if (!bRead)
			{
				// No CPU copy of the mesh (a cooked build): the bounds' corners will do for the size.
				const FBox Bounds = Comp->Bounds.GetBox();
				for (int32 C = 0; C < 8; ++C)
				{
					OutPoints.Add(FVector((C & 1) ? Bounds.Max.X : Bounds.Min.X, (C & 2) ? Bounds.Max.Y : Bounds.Min.Y, (C & 4) ? Bounds.Max.Z : Bounds.Min.Z));
				}
			}
		}
	}
}

// ---------------------------------------------------------------------------------- the actor

AMuseeFrame::AMuseeFrame()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	GiltMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Museum/Materials/M_Gilt_Aged.M_Gilt_Aged")));
	GiltFallbackMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Museum/Materials/M_Frame_Gilt.M_Frame_Gilt")));
	WoodMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Museum/Materials/M_Frame_Wood_Dark.M_Frame_Wood_Dark")));
	MatMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Museum/Materials/M_Frame_Mat.M_Frame_Mat")));
	LinenMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Museum/Materials/M_Frame_Linen.M_Frame_Linen")));
	StretcherMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Museum/Materials/M_Frame_Pine.M_Frame_Pine")));
}

void AMuseeFrame::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
}

void AMuseeFrame::BeginPlay()
{
	Super::BeginPlay();
	// The meshes aren't saved: a game (or play in editor) builds them here.
	if (!bBuilt || !FrameMesh || !MatMesh) { Build(); }
}

void AMuseeFrame::Rebuild()
{
	Build();
}

float AMuseeFrame::DefaultFrameWidth(EMuseeFrameStyle ForStyle, float LongSide)
{
	switch (ForStyle)
	{
	case EMuseeFrameStyle::SalonGilt: return FMath::Clamp(0.09f + 0.015f * LongSide, 0.10f, 0.14f);
	case EMuseeFrameStyle::CabinetGilt: return FMath::Clamp(0.04f + 0.06f * LongSide, 0.05f, 0.075f);
	case EMuseeFrameStyle::Photograph: return 0.022f;
	case EMuseeFrameStyle::ReededGilt: return FMath::Clamp(0.08f + 0.03f * LongSide, 0.10f, 0.16f);   // Albion
	case EMuseeFrameStyle::WattsGilt: return FMath::Clamp(0.10f + 0.03f * LongSide, 0.12f, 0.18f);    // Albion
	default: return 0.f;
	}
}

float AMuseeFrame::GetResolvedFrameWidth() const
{
	return FrameWidth > 0.f ? FrameWidth : DefaultFrameWidth(Style, FMath::Max(SightWidth, SightHeight));
}

FVector2D AMuseeFrame::GetResolvedMatWidths() const
{
	if (Style == EMuseeFrameStyle::None) { return FVector2D::ZeroVector; }
	if (MatOuterSize.X > 0.0 && MatOuterSize.Y > 0.0)
	{
		return FVector2D(FMath::Max(0.0, 0.5 * (MatOuterSize.X - SightWidth)), FMath::Max(0.0, 0.5 * (MatOuterSize.Y - SightHeight)));
	}
	const double Width = MatWidth >= 0.f ? MatWidth : (Style == EMuseeFrameStyle::Photograph ? 0.07 : 0.0);
	return FVector2D(Width, Width);
}

FVector2D AMuseeFrame::GetOuterSize() const
{
	const double W = GetResolvedFrameWidth();
	const double OuterFactor = Style == EMuseeFrameStyle::SalonGilt ? 0.957 : (Style == EMuseeFrameStyle::CabinetGilt ? 0.90 :
		(Style == EMuseeFrameStyle::ReededGilt ? 1.02 : 1.0));   // Albion's reeded frame
	const FVector2D Mats = GetResolvedMatWidths();
	return FVector2D(SightWidth + 2.0 * (Mats.X + W * OuterFactor), SightHeight + 2.0 * (Mats.Y + W * OuterFactor));
}

FVector AMuseeFrame::GetTopEdgeWorld() const
{
	const double Top = bBuilt ? BuiltTop : 0.5 * GetOuterSize().Y;
	const double Front = bBuilt ? BuiltFront : CanvasOffset + 0.5 * GetResolvedFrameWidth();
	return GetActorTransform().TransformPosition(FVector(Front, 0.0, Top) * MuseeFrameImpl::MetresToCm);
}

bool AMuseeFrame::Configure(const FString& StyleName, float NewFrameWidth, float NewMatWidth, FVector2D NewMatOuterSize, bool bNewBackPanel)
{
	Modify();
	struct FStyleEntry
	{
		const TCHAR* Label;
		EMuseeFrameStyle Value;
	};
	static const FStyleEntry Styles[] = {
		{TEXT("SalonGilt"), EMuseeFrameStyle::SalonGilt},
		{TEXT("CabinetGilt"), EMuseeFrameStyle::CabinetGilt},
		{TEXT("Photograph"), EMuseeFrameStyle::Photograph},
		{TEXT("None"), EMuseeFrameStyle::None},
		{TEXT("ReededGilt"), EMuseeFrameStyle::ReededGilt},   // Albion
		{TEXT("WattsGilt"), EMuseeFrameStyle::WattsGilt},     // Albion
	};
	bool bKnown = false;
	for (const FStyleEntry& Entry : Styles)
	{
		if (StyleName.Equals(Entry.Label, ESearchCase::IgnoreCase))
		{
			Style = Entry.Value;
			bKnown = true;
		}
	}
	if (!bKnown) { UE_LOG(LogMusee, Warning, TEXT("MuseeFrame %s: unknown style '%s'"), *GetName(), *StyleName); }
	FrameWidth = FMath::Max(0.f, NewFrameWidth);
	MatWidth = NewMatWidth;
	MatOuterSize = NewMatOuterSize;
	bBackPanel = bNewBackPanel;
	Build();
	return bKnown;
}

bool AMuseeFrame::FitToCanvas(AActor* CanvasActor, AActor* BackActor, AActor* SightActor)
{
	using namespace MuseeFrameImpl;
	TArray<FVector> CanvasPts;
	FVector NormalSum = FVector::ZeroVector;
	GatherVertices(CanvasActor, CanvasPts, &NormalSum);
	if (CanvasPts.Num() < 3)
	{
		UE_LOG(LogMusee, Warning, TEXT("MuseeFrame %s: the canvas %s has no geometry"), *GetName(), CanvasActor ? *CanvasActor->GetName() : TEXT("(none)"));
		return false;
	}
	const FBox CanvasBox(CanvasPts);
	TArray<FVector> BackPts;
	GatherVertices(BackActor, BackPts, nullptr);

	FVector Facing = NormalSum.GetSafeNormal();
	if (Facing.IsNearlyZero())
	{
		// No normals: the canvas's thinnest axis, pointing away from what is behind it.
		const FVector Ext = CanvasBox.GetExtent();
		const int32 Axis = Ext.X <= Ext.Y && Ext.X <= Ext.Z ? 0 : (Ext.Y <= Ext.Z ? 1 : 2);
		Facing = FVector::ZeroVector;
		Facing[Axis] = 1.0;
		const FVector Behind = BackPts.Num() > 0 ? FBox(BackPts).GetCenter()
			: (CanvasActor->GetAttachParentActor() ? CanvasActor->GetAttachParentActor()->GetActorLocation() : CanvasBox.GetCenter());
		if (FVector::DotProduct(CanvasBox.GetCenter() - Behind, Facing) < 0) { Facing = -Facing; }
	}
	FVector UpHint = FVector::UpVector;
	if (FMath::Abs(FVector::DotProduct(Facing, UpHint)) > 0.9) { UpHint = FVector::ForwardVector; }
	const FMatrix Basis = FRotationMatrix::MakeFromXZ(Facing, UpHint);
	const FVector AxisY = Basis.GetScaledAxis(EAxis::Y), AxisZ = Basis.GetScaledAxis(EAxis::Z);

	TArray<FVector> SightPts;
	GatherVertices(SightActor, SightPts, nullptr);
	if (SightPts.Num() < 3) { SightPts = CanvasPts; }
	double MinY = TNumericLimits<double>::Max(), MaxY = -MinY, MinZ = MinY, MaxZ = -MinY, Front = -MinY, SightBack = MinY;
	for (const FVector& P : SightPts)
	{
		const double Y = FVector::DotProduct(P, AxisY), Z = FVector::DotProduct(P, AxisZ), X = FVector::DotProduct(P, Facing);
		MinY = FMath::Min(MinY, Y); MaxY = FMath::Max(MaxY, Y);
		MinZ = FMath::Min(MinZ, Z); MaxZ = FMath::Max(MaxZ, Z);
		Front = FMath::Max(Front, X); SightBack = FMath::Min(SightBack, X);
	}
	double BackPlane = Front - 4.5;
	if (BackPts.Num() > 0)
	{
		BackPlane = TNumericLimits<double>::Max();
		for (const FVector& P : BackPts) { BackPlane = FMath::Min(BackPlane, FVector::DotProduct(P, Facing)); }
	}
	else if (SightActor && SightPts.Num() >= 3 && SightBack < Front)
	{
		BackPlane = SightBack;
	}
	BackPlane = FMath::Min(BackPlane, Front);

	Modify();
	SightWidth = float((MaxY - MinY) / MetresToCm);
	SightHeight = float((MaxZ - MinZ) / MetresToCm);
	CanvasOffset = float((Front - BackPlane) / MetresToCm);
	const FVector Location = AxisY * (0.5 * (MinY + MaxY)) + AxisZ * (0.5 * (MinZ + MaxZ)) + Facing * BackPlane;
	SetActorLocationAndRotation(Location, Basis.Rotator());
	Build();
	return true;
}

UMaterialInterface* AMuseeFrame::LoadIfPresent(const TSoftObjectPtr<UMaterialInterface>& Soft)
{
	if (Soft.IsNull()) { return nullptr; }
	if (UMaterialInterface* Loaded = Soft.Get()) { return Loaded; }
	const FString Package = Soft.ToSoftObjectPath().GetLongPackageName();
	if (Package.IsEmpty() || !FPackageName::DoesPackageExist(Package)) { return nullptr; }
	return Soft.LoadSynchronous();
}

void AMuseeFrame::ResolveMaterials()
{
	// In the editor, look again each build (M_Gilt_Aged may have arrived); in a game, keep what the
	// editor found (hard references, so they are cooked).
	const UWorld* FrameWorld = GetWorld();
	const bool bRefresh = !(FrameWorld && FrameWorld->IsGameWorld());
	if (bRefresh || !ResolvedGilt)
	{
		if (UMaterialInterface* Aged = LoadIfPresent(GiltMaterial)) { ResolvedGilt = Aged; }
		else if (UMaterialInterface* Fallback = LoadIfPresent(GiltFallbackMaterial)) { ResolvedGilt = Fallback; }
	}
	if (bRefresh || !ResolvedWood)
	{
		if (UMaterialInterface* Wood = LoadIfPresent(WoodMaterial)) { ResolvedWood = Wood; }
	}
	if (bRefresh || !ResolvedMat)
	{
		if (UMaterialInterface* Board = LoadIfPresent(MatMaterial)) { ResolvedMat = Board; }
	}
	if (bRefresh || !ResolvedLinen)
	{
		if (UMaterialInterface* Linen = LoadIfPresent(LinenMaterial)) { ResolvedLinen = Linen; }
	}
	if (bRefresh || !ResolvedStretcher)
	{
		if (UMaterialInterface* Pine = LoadIfPresent(StretcherMaterial)) { ResolvedStretcher = Pine; }
	}
}

void AMuseeFrame::EnsureComponents()
{
	if (!GetWorld() || !RootComponent) { return; }
	auto Ensure = [this](TObjectPtr<UProceduralMeshComponent>& Slot, const TCHAR* SlotName)
	{
		if (!IsValid(Slot.Get()))
		{
			// Not saved with the level (RF_Transient), nor copied to play in editor (RF_DuplicateTransient).
			UProceduralMeshComponent* Existing = FindObjectFast<UProceduralMeshComponent>(this, FName(SlotName));
			if (Existing && IsValid(Existing))
			{
				Slot = Existing;
			}
			else
			{
				const FName NewName = Existing ? MakeUniqueObjectName(this, UProceduralMeshComponent::StaticClass(), FName(SlotName)) : FName(SlotName);
				Slot = NewObject<UProceduralMeshComponent>(this, NewName, RF_Transient | RF_TextExportTransient | RF_DuplicateTransient);
			}
			Slot->bUseAsyncCooking = true;
			Slot->SetCanEverAffectNavigation(false);
		}
		if (!Slot->IsRegistered())
		{
			if (!Slot->GetAttachParent()) { Slot->SetupAttachment(RootComponent); }
			Slot->RegisterComponent();
		}
	};
	Ensure(FrameMesh, TEXT("MuseeFrameMesh"));
	Ensure(MatMesh, TEXT("MuseeMatMesh"));
}

void AMuseeFrame::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }   // baked into static meshes (Geometry/MuseeBake.h)
	using namespace MuseeFrameImpl;
	EnsureComponents();
	if (!FrameMesh || !MatMesh) { return; }
	FrameMesh->ClearAllMeshSections();
	MatMesh->ClearAllMeshSections();
	MatMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	bBuilt = true;
	if (Style == EMuseeFrameStyle::None)
	{
		FrameMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return;
	}

	const double W = GetResolvedFrameWidth();
	const FVector2D Mats = GetResolvedMatWidths();
	const double HalfW = 0.5 * SightWidth, HalfH = 0.5 * SightHeight;
	const double Hc = FMath::Max(0.0, double(CanvasOffset));
	const double Detail = FMath::Clamp(double(OrnamentDetail), 0.0, 2.0);
	const bool bPhoto = Style == EMuseeFrameStyle::Photograph;
	const double Lip = bPhoto ? FMath::Min(double(SightLip), 0.004) : double(SightLip);

	// The mat (a bevelled board) or the linen liner (sloping up to the frame).
	FSectionBuilder MatOut;
	double FrameBase = Hc;
	if (Mats.X > 0.0 || Mats.Y > 0.0)
	{
		if (bPhoto)
		{
			const double Board = 0.0035, Overlap = 0.004;
			const double WinA = FMath::Max(0.01, HalfW - Overlap), WinB = FMath::Max(0.01, HalfH - Overlap);
			RectRing(MatOut, WinA, WinB, Hc + 0.0005, WinA + Board, WinB + Board, Hc + Board, 0.85);                       // the bevel
			RectRing(MatOut, WinA + Board, WinB + Board, Hc + Board, HalfW + Mats.X + 0.002, HalfH + Mats.Y + 0.002, Hc + Board, 1.0);
			FrameBase = Hc + Board;
		}
		else
		{
			const double Rise = 0.25 * FMath::Max(Mats.X, Mats.Y);
			RectRing(MatOut, FMath::Max(0.01, HalfW - 0.003), FMath::Max(0.01, HalfH - 0.003), Hc + 0.001,
					 HalfW + Mats.X + 0.002, HalfH + Mats.Y + 0.002, Hc + Rise, 0.8);
			FrameBase = Hc + Rise;
		}
	}
	const double A = HalfW + Mats.X, B = HalfH + Mats.Y;

	FStyleSpec Spec = MakeSpec(Style, W, FrameBase, Lip);
	Spec.Profile.Finish();
	const FHeightTable Top = TopSurface(Spec.Profile, 256);
	const FHeightTable Env = Envelope(Top, 0.05 * W);
	const TArray<double> Exposure = ProfileExposure(Spec.Profile, Top, 0.18 * W);

	FSectionBuilder Face, Back, Hit, Linen, Stretcher;
	SweepProfile(Spec.Profile, A, B, Exposure, Face, Back);
	if (bBackPanel && !bPhoto)
	{
		// The painting's own back, as a visitor behind a stele or a rack sees it: the canvas's unbleached linen
		// across the whole opening, 1 mm behind the picture, and the pine stretcher it is tacked round (bars
		// 4.5 cm wide, 2 cm deep), a key wedged in each inside corner, and a cross-brace on a canvas over 0.7 m.
		const FColor Col = Paint(0.5, false, false);
		const double XL = FMath::Max(0.004, Hc - 0.001);
		const double Depth = FMath::Min(0.02, XL - 0.002), X0 = XL - Depth;
		const int32 I0 = Linen.Vert(FVector(XL, -A, -B), -FVector::XAxisVector, FVector2D(-A, -B), Col, FVector::YAxisVector);
		const int32 I1 = Linen.Vert(FVector(XL, A, -B), -FVector::XAxisVector, FVector2D(A, -B), Col, FVector::YAxisVector);
		const int32 I2 = Linen.Vert(FVector(XL, A, B), -FVector::XAxisVector, FVector2D(A, B), Col, FVector::YAxisVector);
		const int32 I3 = Linen.Vert(FVector(XL, -A, B), -FVector::XAxisVector, FVector2D(-A, B), Col, FVector::YAxisVector);
		Linen.Quad(I0, I1, I2, I3);
		const double Bar = FMath::Min(0.045, 0.2 * FMath::Min(A, B));
		const double Ai = HalfW, Bi = HalfH;   // the stretcher is the canvas's size, inside the rebate
		Box(Stretcher, FVector(X0, -Ai, -Bi), FVector(XL, Ai, -Bi + Bar));
		Box(Stretcher, FVector(X0, -Ai, Bi - Bar), FVector(XL, Ai, Bi));
		Box(Stretcher, FVector(X0, -Ai, -Bi + Bar), FVector(XL, -Ai + Bar, Bi - Bar));
		Box(Stretcher, FVector(X0, Ai - Bar, -Bi + Bar), FVector(XL, Ai, Bi - Bar));
		if (Ai > 0.35) { Box(Stretcher, FVector(X0 + 0.002, -Bar / 2, -Bi + Bar), FVector(XL, Bar / 2, Bi - Bar)); }
		if (Bi > 0.35) { Box(Stretcher, FVector(X0 + 0.002, -Ai + Bar, -Bar / 2), FVector(XL, Ai - Bar, Bar / 2)); }
		// The keys: small wedges (here blocks) in the inside corners, a little proud of the bars.
		const double K = 0.6 * Bar;
		for (const double Sy : {-1.0, 1.0})
		{
			for (const double Sz : {-1.0, 1.0})
			{
				const FVector C(X0 - 0.004, Sy * (Ai - Bar), Sz * (Bi - Bar));
				Box(Stretcher, C + FVector(0, Sy > 0 ? -K : 0, Sz > 0 ? -K : 0), C + FVector(0.006, Sy > 0 ? 0 : K, Sz > 0 ? 0 : K));
			}
		}
	}
	else if (bBackPanel)
	{
		// A photograph's backing board, facing the wall (or the glass behind).
		const FColor Col = Paint(0.3, false, false);
		const double X = 0.002;
		const int32 I0 = Back.Vert(FVector(X, -A, -B), -FVector::XAxisVector, FVector2D(-A, -B), Col, FVector::YAxisVector);
		const int32 I1 = Back.Vert(FVector(X, A, -B), -FVector::XAxisVector, FVector2D(A, -B), Col, FVector::YAxisVector);
		const int32 I2 = Back.Vert(FVector(X, A, B), -FVector::XAxisVector, FVector2D(A, B), Col, FVector::YAxisVector);
		const int32 I3 = Back.Vert(FVector(X, -A, B), -FVector::XAxisVector, FVector2D(-A, B), Col, FVector::YAxisVector);
		Back.Quad(I0, I1, I2, I3);
	}
	if (Detail > 0.0 && !bPhoto)
	{
		const FGround Ground{Env, A, B, FrameBase + 0.001};
		AddOrnament(Spec, A, B, Ground, Detail, Face);
	}
	double Depth = 0;
	for (const FProfilePoint& Pt : Spec.Profile.Pts) { Depth = FMath::Max(Depth, Pt.P.Y); }
	BuiltFront = Depth;
	BuiltTop = B + Spec.Outer;
	if (bCollision)
	{
		const double In = -Lip, Out = Spec.Outer;
		Box(Hit, FVector(0, -(A + Out), B + In), FVector(Depth, A + Out, B + Out));
		Box(Hit, FVector(0, -(A + Out), -(B + Out)), FVector(Depth, A + Out, -(B + In)));
		Box(Hit, FVector(0, A + In, -(B + In)), FVector(Depth, A + Out, B + In));
		Box(Hit, FVector(0, -(A + Out), -(B + In)), FVector(Depth, -(A + In), B + In));
	}

	Face.Write(FrameMesh, 0, false);
	Back.Write(FrameMesh, 1, false);
	if (!Hit.IsEmpty())
	{
		Hit.Write(FrameMesh, 2, true);
		FrameMesh->SetMeshSectionVisible(2, false);
		FrameMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	}
	else
	{
		FrameMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (!MatOut.IsEmpty()) { MatOut.Write(MatMesh, 0, false); }
	if (!Linen.IsEmpty())
	{
		if (Hit.IsEmpty()) { FSectionBuilder().Write(FrameMesh, 2, false); }   // keep the sections' numbers
		Linen.Write(FrameMesh, 3, false);
		Stretcher.Write(FrameMesh, 4, false);
	}

	ResolveMaterials();
	if (UMaterialInterface* FaceMat = Spec.bGiltFace ? ResolvedGilt.Get() : ResolvedWood.Get()) { FrameMesh->SetMaterial(0, FaceMat); }
	if (ResolvedWood) { FrameMesh->SetMaterial(1, ResolvedWood); }
	if (ResolvedMat) { MatMesh->SetMaterial(0, ResolvedMat); }
	if (!Linen.IsEmpty() && FrameMesh->GetNumSections() > 3) { FrameMesh->SetMaterial(3, ResolvedLinen ? ResolvedLinen.Get() : ResolvedMat.Get()); }
	if (!Stretcher.IsEmpty() && FrameMesh->GetNumSections() > 4) { FrameMesh->SetMaterial(4, ResolvedStretcher ? ResolvedStretcher.Get() : ResolvedWood.Get()); }
}
