#include "SunClock/SunClockBuild.h"

#include "Algo/Reverse.h"

/**
 * The drum, in its own frame (the dial at z 0; raised 3.5 m, its sill is on the Rotunda's floor).
 *
 * A marble wall 0.36 m thick (r 6.545–6.90) from the ring under the sill (−3.56) to the dial's
 * underside, all of it inside r 6.99 so it slides into its pocket: a moulded base (plinth, torus,
 * fillet, astragal) and an entablature (a two-fascia architrave, a plain frieze, an ovolo under the
 * corona that is the dial's edge); twelve fluted pilasters with pedestal blocks and capitals; one
 * round-arched door to the west, 2.6 × 2.95 m, its jambs radial (so every moulding ends on them), a
 * moulded archivolt running down the jambs to the base, imposts and a keystone.
 *
 * Its underside, seen from the stair: stone coffers in five rings (8, 12, 16, 24 and 32 to the
 * round), each sunk in two steps between radial and ring ribs; a bronze rim round the edge and a
 * gilt pendant boss over the well.
 */
namespace SunClockBuild
{
	constexpr double WallFace = SC::DrumWallFace;
	constexpr double WallInner = SC::DrumWallInner;
	constexpr double SillZ = -SC::Lift;

	static FProfile DrumEntablature()
	{
		FProfile P;
		P.Add(WallFace, ArchitraveZ).Add(6.930, ArchitraveZ).Add(6.930, -0.395).Add(6.940, -0.390).Add(6.940, -0.335)
			.Add(6.952, -0.330).Add(6.952, -0.315).Add(6.915, -0.315).Add(6.915, -0.195)
			.Arc(6.915, -0.150, 0.045, 0.045, -90, 0, 8)
			.Add(SC::DrumOuter, -0.150).Add(SC::DrumOuter, 0.0);
		return P;
	}

	/** The base above the sill: the plinth, a torus, a fillet, an astragal, up to the wall face. */
	static FProfile DrumBaseUpper()
	{
		FProfile P;
		P.Add(SC::DrumOuter, SillZ).Add(SC::DrumOuter, -3.300).Add(6.955, -3.300)
			.Arc(6.955, -3.265, 0.035, 0.035, -90, 90, 12)
			.Add(6.925, -3.230).Add(6.925, -3.215).Add(6.915, -3.215)
			.Arc(6.915, -3.200, 0.015, 0.015, -90, 90, 6)
			.Add(6.905, -3.175).Add(6.905, -3.100).Add(WallFace, BaseTopZ);
		return P;
	}

	static void DrumWalls(FSunClockMeshes& M)
	{
		FMeshData& S = M.DrumStone;
		const TArray<FStation>& Stations = DrumStations();
		TArray<double> All;
		for (const FStation& St : Stations) { All.Add(St.Phi); }
		const TArray<double> RunAngles = DrumRunAngles();
		const double Spring = DoorSpring();

		// The entablature (all round), the base's plinth under the sill (all round), the base above it (not
		// across the door), and the ring's underside.
		Revolve(S, DrumEntablature(), All, true);
		FProfile Lower;
		Lower.Add(SC::DrumOuter, DrumBottom).Add(SC::DrumOuter, SillZ);
		Revolve(S, Lower, All, true);
		const FProfile Upper = DrumBaseUpper();
		Revolve(S, Upper, RunAngles, false);
		Annulus(S, WallInner, All, SC::DrumOuter, All, DrumBottom, false);

		// The wall's faces, cut round the door.
		TArray<FColumn> Outer, Inner;
		for (const FStation& St : Stations)
		{
			FColumn Full, FullIn;
			Full.Phi = FullIn.Phi = St.Phi;
			Full.Spans.Add({BaseTopZ, Spring, ArchitraveZ});
			FullIn.Spans.Add({DrumBottom, SillZ, Spring, GrooveZ});
			FColumn Door, DoorIn;
			Door.Phi = DoorIn.Phi = St.Phi;
			if (St.Arch != INDEX_NONE)
			{
				Door.Spans.Add({DoorZ(WallFace, St.Arch), ArchitraveZ});
				DoorIn.Spans.Add({DrumBottom, SillZ});
				DoorIn.Spans.Add({DoorZ(WallInner, St.Arch), GrooveZ});
			}
			if (St.Arch == INDEX_NONE)
			{
				Outer.Add(Full);
				Inner.Add(FullIn);
			}
			else if (St.Arch == 0)
			{
				// The south jamb: coming from the wall into the door.
				Outer.Append({Full, Door});
				Inner.Append({FullIn, DoorIn});
			}
			else if (St.Arch == ArchSegments)
			{
				Outer.Append({Door, Full});
				Inner.Append({DoorIn, FullIn});
			}
			else
			{
				Outer.Add(Door);
				Inner.Add(DoorIn);
			}
		}
		CylinderColumns(S, WallFace, Outer, true, true);
		CylinderColumns(S, WallInner, Inner, false, true);

		// The door's jambs (radial), its conical intrados and its sill.
		const double Half = DoorHalfAngle();
		for (const int32 Side : {0, 1})
		{
			const double Phi = Side == 0 ? UE_DOUBLE_PI - Half : UE_DOUBLE_PI + Half;
			const FVector N = TangentDir(Phi) * (Side == 0 ? 1.0 : -1.0);
			TArray<FVector2D> Outline;
			Outline.Add(FVector2D(WallInner, SillZ));
			for (const SalonKit::FProfilePoint& P : Upper.Points) { Outline.Add(P.P); }
			Outline.Add(FVector2D(WallFace, Spring));
			Outline.Add(FVector2D(WallInner, Spring));
			FlatPolygon(S, Outline, [Phi](const FVector2D& Q) { return Polar(Q.X, Phi, Q.Y); }, N);
		}
		S.Patch(ArchSegments, 1,
			[](int32 I, int32 J) { const double R = J == 0 ? WallInner : WallFace; return Polar(R, DoorPhi(I), DoorZ(R, I)); },
			[](const FVector& P) { return FVector2D(P.Y, -P.Z); },
			[Spring](const FVector& P) { return Polar(FVector2D(P.X, P.Y).Size(), UE_DOUBLE_PI, Spring) - P; });
		S.Patch(ArchSegments, 1,
			[](int32 I, int32 J) { return Polar(J == 0 ? WallInner : SC::DrumOuter, DoorPhi(I), SillZ); },
			[](const FVector& P) { return FVector2D(P.X, P.Y); },
			[](const FVector&) { return FVector::UpVector; });
	}

	/** A point on the wall face's developed plane about the door: u across (south +), z up, d out of the face. */
	static FVector DoorFace(double U, double Z, double D)
	{
		const double R = WallFace + D;
		return Polar(R, UE_DOUBLE_PI - FMath::Asin(FMath::Clamp(U / WallFace, -1.0, 1.0)), Z);
	}

	/** A block on the door's face (its back sunk into the wall): u, z, d ranges. */
	static void DoorBlock(FMeshData& M, const TArray<FVector2D>& Front, double D0, double D1)
	{
		// Front face, sides round the outline (anticlockwise in (u, z)).
		TArray<FVector2D> Poly = Front;
		double Area = 0;
		for (int32 I = 0; I < Poly.Num(); ++I) { Area += SalonKit::Cross2(Poly[I], Poly[(I + 1) % Poly.Num()]); }
		if (Area < 0) { Algo::Reverse(Poly); }
		const FVector2D C = [&Poly] { FVector2D Sum(0, 0); for (const FVector2D& P : Poly) { Sum += P; } return Sum / Poly.Num(); }();
		const FVector Out = (DoorFace(C.X, C.Y, 1.0) - DoorFace(C.X, C.Y, 0.0)).GetSafeNormal();
		FlatPolygon(M, Poly, [D1](const FVector2D& Q) { return DoorFace(Q.X, Q.Y, D1); }, Out);
		for (int32 I = 0; I < Poly.Num(); ++I)
		{
			const FVector2D A = Poly[I], B = Poly[(I + 1) % Poly.Num()];
			const FVector P0 = DoorFace(A.X, A.Y, D0), P1 = DoorFace(B.X, B.Y, D0), P2 = DoorFace(B.X, B.Y, D1), P3 = DoorFace(A.X, A.Y, D1);
			// Outward: to the right of the edge in (u, z), mapped.
			const FVector2D Right(B.Y - A.Y, A.X - B.X);
			const FVector2D Mid = (A + B) / 2;
			const FVector N = (DoorFace(Mid.X + Right.X * 0.01, Mid.Y + Right.Y * 0.01, D1) - DoorFace(Mid.X, Mid.Y, D1)).GetSafeNormal();
			M.Rect(P0, P1, P2, P3, N);
		}
	}

	static void DrumDoorDressing(FSunClockMeshes& M)
	{
		FMeshData& S = M.DrumStone;
		const double Spring = DoorSpring();
		// The archivolt, run down the jambs to the base: a U from the south leg's foot round to the north's.
		constexpr double Band = 1.355;   // its centre line: 1 cm clear of the opening, 9 cm wide
		struct FPathPoint { FVector2D P, Lateral; };
		TArray<FPathPoint> Path;
		Path.Add({FVector2D(Band, -3.10), FVector2D(1, 0)});
		for (int32 I = 0; I <= ArchSegments; ++I)
		{
			const double Psi = UE_DOUBLE_PI * I / ArchSegments;
			const FVector2D L(FMath::Cos(Psi), FMath::Sin(Psi));
			Path.Add({FVector2D(0, Spring) + L * Band, L});
		}
		Path.Add({FVector2D(-Band, -3.10), FVector2D(-1, 0)});
		TArray<FFrame> Run;
		double Along = 0;
		for (int32 I = 0; I < Path.Num(); ++I)
		{
			const FPathPoint& Pt = Path[I];
			if (I > 0) { Along += FVector2D::Distance(Path[I - 1].P, Pt.P); }
			FFrame F;
			F.Origin = DoorFace(Pt.P.X, Pt.P.Y, 0);
			F.AxisA = F.NormA = (DoorFace(Pt.P.X + Pt.Lateral.X * 0.01, Pt.P.Y + Pt.Lateral.Y * 0.01, 0) - F.Origin).GetSafeNormal();
			F.AxisB = F.NormB = (DoorFace(Pt.P.X, Pt.P.Y, 1.0) - F.Origin).GetSafeNormal();
			F.S = Along;
			Run.Add(F);
		}
		FProfile Section;
		Section.Add(0.045, -0.010).Add(0.045, 0.036).Arc(0.033, 0.036, 0.012, 0.012, 0, 180, 8)
			.Add(0.010, 0.036).Add(0.010, 0.030).Add(-0.045, 0.030).Add(-0.045, -0.010);
		SalonKit::Sweep(S, Run, Section);

		// Imposts at the springing, and the keystone.
		for (const double Sign : {1.0, -1.0})
		{
			const double U0 = 1.300 * Sign, U1 = 1.430 * Sign;
			DoorBlock(S, {FVector2D(U0, Spring - 0.07), FVector2D(U1, Spring - 0.07), FVector2D(U1, Spring + 0.01), FVector2D(U0, Spring + 0.01)}, -0.01, 0.060);
		}
		DoorBlock(S, {FVector2D(-0.075, -0.545), FVector2D(0.075, -0.545), FVector2D(0.100, -0.440), FVector2D(-0.100, -0.440)}, -0.01, 0.070);
	}

	static void DrumPilasters(FSunClockMeshes& M)
	{
		FMeshData& S = M.DrumStone;
		// Five flutes between narrow fillets, a flat margin at each arris.
		FProfile Fluted;
		constexpr double HalfW = 0.22, Front = 0.055, Margin = 0.03, Fillet = 0.016;
		const double Flute = (2 * HalfW - 2 * Margin - 4 * Fillet) / 5;
		const double Sag = 0.018, Radius = (Flute * Flute / 4 + Sag * Sag) / (2 * Sag);
		Fluted.Add(HalfW, -0.01).Add(HalfW, Front).Add(HalfW - Margin, Front);
		double A = HalfW - Margin;
		for (int32 F = 0; F < 5; ++F)
		{
			// A shallow flute from A to A − Flute (the solid on the left, so the hollow faces out).
			// The arc's centre is above the front; the angle is measured from straight down.
			const double Mid = A - Flute / 2, ArcZ = Front + Radius - Sag;
			const double Half = FMath::Asin(Flute / 2 / Radius);
			for (int32 I = 1; I <= 8; ++I)
			{
				const double T = Half - 2 * Half * I / 8;
				Fluted.Add(Mid + Radius * FMath::Sin(T), ArcZ - Radius * FMath::Cos(T), I < 8);
			}
			A -= Flute;
			if (F < 4)
			{
				A -= Fillet;
				Fluted.Add(A, Front);
			}
		}
		Fluted.Add(-HalfW, Front).Add(-HalfW, -0.01);

		for (int32 K = 0; K < 12; ++K)
		{
			const double Phi = (15.0 + 30.0 * K) * Deg;
			const FVector O = Polar(WallFace, Phi, 0), T = TangentDir(Phi), R = RadialDir(Phi), Up = FVector::UpVector;
			constexpr int32 NoBack = FMeshData::AllFaces & ~FMeshData::NegY;
			// Pedestal block, the fluted shaft, the capital (necking, echinus, abacus).
			FrameBox(S, O, T, R, Up, FVector(-0.25, -0.01, -3.10), FVector(0.25, 0.075, -2.98), NoBack);
			TArray<FFrame> Run;
			for (const double Z : {-2.99, -0.61})
			{
				FFrame F;
				F.Origin = O + Up * Z;
				F.AxisA = F.NormA = T;
				F.AxisB = F.NormB = R;
				F.S = Z;
				Run.Add(F);
			}
			SalonKit::Sweep(S, Run, Fluted);
			FrameBox(S, O, T, R, Up, FVector(-0.235, -0.01, -0.62), FVector(0.235, 0.064, -0.56), NoBack);
			FrameBox(S, O, T, R, Up, FVector(-0.245, -0.01, -0.56), FVector(0.245, 0.071, -0.53), NoBack);
			FrameBox(S, O, T, R, Up, FVector(-0.255, -0.01, -0.53), FVector(0.255, 0.080, -0.44), NoBack);
		}
	}

	static void DrumSoffitCoffers(FSunClockMeshes& M)
	{
		FMeshData& S = M.DrumSoffit;
		constexpr double Step = -0.115, Back = -0.10, Inset = 0.04;   // the coffers' step and panel, up into the slab
		const FVector Down = -FVector::UpVector;
		const TArray<double> Grid = GridAngles();
		// The ring ribs (the centre too, under the pendant).
		Annulus(S, 0.0, Grid, 0.55, Grid, RibZ, false);
		for (const FVector2D Rib : {FVector2D(1.25, 1.35), FVector2D(2.25, 2.35), FVector2D(3.45, 3.55), FVector2D(4.75, 4.85)})
		{
			Annulus(S, Rib.X, Grid, Rib.Y, Grid, RibZ, false);
		}
		struct FBand { double R0, R1; int32 Coffers, HalfRib; };
		const FBand Bands[] = {{0.55, 1.25, 8, 7}, {1.35, 2.25, 12, 3}, {2.35, 3.45, 16, 2}, {3.55, 4.75, 24, 1}, {4.85, 6.20, 32, 1}};
		for (const FBand& B : Bands)
		{
			const int32 Per = Around / B.Coffers;
			for (int32 K = 0; K < B.Coffers; ++K)
			{
				const int32 G0 = K * Per + B.HalfRib, G1 = (K + 1) * Per - B.HalfRib;
				// The radial rib after this coffer.
				TArray<double> RibArc;
				for (int32 G = G1; G <= G1 + 2 * B.HalfRib; ++G) { RibArc.Add(GridAngle(G)); }
				Sector(S, B.R0, RibArc, B.R1, RibArc, RibZ, false);
				// The coffer: its outline on the ribs, a step 40 mm in at −0.115, the panel at −0.10.
				const int32 Segs = G1 - G0;
				auto Outline = [&](double Shrink, double Z)
				{
					// Inner arc (G0 → G1), then outer arc (G1 → G0); Shrink moves every side in.
					TArray<FVector> Pts;
					const double RI = B.R0 + Shrink, RO = B.R1 - Shrink;
					for (int32 Side = 0; Side < 2; ++Side)
					{
						const double R = Side == 0 ? RI : RO;
						const double Off = Shrink > 0 ? FMath::Asin(Shrink / R) : 0.0;
						const double A0 = GridAngle(G0) + Off, A1 = GridAngle(G1) - Off;
						for (int32 I = 0; I <= Segs; ++I)
						{
							const double T = Side == 0 ? double(I) / Segs : 1.0 - double(I) / Segs;
							Pts.Add(Polar(R, FMath::Lerp(A0, A1, T), Z));
						}
					}
					return Pts;
				};
				const TArray<FVector> Rim0 = Outline(0, RibZ), Rim1 = Outline(0, Step), Step0 = Outline(Inset, Step), Step1 = Outline(Inset, Back);
				const FVector Middle = Polar((B.R0 + B.R1) / 2, GridAngle((G0 + G1) / 2), Step);
				const int32 N = Rim0.Num();
				auto Wall = [&S, &Middle, N](const TArray<FVector>& Lo, const TArray<FVector>& Hi, int32 Segs0)
				{
					// Four walls, crisp at the corners (indices Segs0 and 2·Segs0 + 1 start the sides).
					for (int32 I = 0; I < N; ++I)
					{
						const int32 J = (I + 1) % N;
						FVector Mid = (Lo[I] + Lo[J]) / 2;
						FVector ToCentre = Middle - Mid;
						ToCentre.Z = 0;
						const FVector Edge = Lo[J] - Lo[I];
						FVector Nrm = FVector::CrossProduct(Edge, FVector::UpVector).GetSafeNormal();
						if (FVector::DotProduct(Nrm, ToCentre) < 0) { Nrm = -Nrm; }
						// Along the arcs, radial normals for a smooth curve.
						auto NormalAt = [&](const FVector& P)
						{
							const bool bArc = I != Segs0 && I != N - 1;
							if (!bArc) { return Nrm; }
							FVector Rad(P.X, P.Y, 0);
							Rad.Normalize();
							return FVector::DotProduct(Rad, Nrm) > 0 ? Rad : -Rad;
						};
						S.Quad(S.Vertex(Lo[I], NormalAt(Lo[I]), FVector2D(Lo[I].X, Lo[I].Y)), S.Vertex(Lo[J], NormalAt(Lo[J]), FVector2D(Lo[J].X, Lo[J].Y)),
							   S.Vertex(Hi[J], NormalAt(Hi[J]), FVector2D(Hi[J].X, Hi[J].Y + 0.03)), S.Vertex(Hi[I], NormalAt(Hi[I]), FVector2D(Hi[I].X, Hi[I].Y + 0.03)));
					}
				};
				Wall(Rim0, Rim1, Segs);
				Wall(Step0, Step1, Segs);
				// The step's face (a frame), and the panel.
				for (int32 I = 0; I < N; ++I)
				{
					const int32 J = (I + 1) % N;
					S.Quad(S.Vertex(Rim1[I], Down, FVector2D(Rim1[I].X, Rim1[I].Y)), S.Vertex(Rim1[J], Down, FVector2D(Rim1[J].X, Rim1[J].Y)),
						   S.Vertex(Step0[J], Down, FVector2D(Step0[J].X, Step0[J].Y)), S.Vertex(Step0[I], Down, FVector2D(Step0[I].X, Step0[I].Y)));
				}
				for (int32 I = 0; I < Segs; ++I)
				{
					const FVector A = Step1[I], Bq = Step1[I + 1], C = Step1[N - 2 - I], D = Step1[N - 1 - I];
					S.Quad(S.Vertex(A, Down, FVector2D(A.X, A.Y)), S.Vertex(Bq, Down, FVector2D(Bq.X, Bq.Y)),
						   S.Vertex(C, Down, FVector2D(C.X, C.Y)), S.Vertex(D, Down, FVector2D(D.X, D.Y)));
				}
			}
		}
	}

	static void DrumSoffitBronze(FSunClockMeshes& M)
	{
		TArray<double> All;
		for (const FStation& St : DrumStations()) { All.Add(St.Phi); }
		// The rim: its inner face sunk into the slab, a rounded arris, a fine groove, flat, and at the wall the
		// deep groove the stair's wall stands in while the dial is down.
		FProfile Rim;
		Rim.Add(6.19, -0.09).Add(6.19, -0.125).Arc(6.205, -0.125, 0.015, 0.015, 180, 270, 6)
.Add(6.32, SoffitZ).Add(6.33, -0.132).Add(6.34, SoffitZ)
			.Add(GrooveInner, SoffitZ).Add(GrooveInner, GrooveZ).Add(WallInner, GrooveZ);
		Revolve(M.DrumBronze, Rim, All, true);
		// The pendant boss over the well (gilt): a low dome, a bead, an ogee up into the slab.
		TArray<double> Coarse;
		for (int32 I = 0; I < Around; I += 4) { Coarse.Add(GridAngle(I)); }
		FProfile Boss;
		Boss.Add(0.0, -0.265);
		for (int32 I = 1; I <= 10; ++I)
		{
			const double T = FMath::DegreesToRadians(90.0 * I / 10);
			Boss.Add(0.11 * FMath::Sin(T), -0.265 + 0.04 * (1 - FMath::Cos(T)), true);
		}
		Boss.Add(0.11, -0.225).Arc(0.125, -0.225, 0.015, 0.015, 180, 0, 8).Add(0.14, -0.215)
			.Add(0.24, -0.19, true).Add(0.34, -0.170, true).Add(0.40, -0.158).Arc(0.40, -0.148, 0.01, 0.01, -90, 90, 6)
			.Add(0.40, -0.12);
		Revolve(M.DrumGilt, Boss, Coarse, true);
	}

	void BuildDrum(FSunClockMeshes& M)
	{
		DrumWalls(M);
		DrumDoorDressing(M);
		DrumPilasters(M);
		DrumSoffitCoffers(M);
		DrumSoffitBronze(M);
	}
}
