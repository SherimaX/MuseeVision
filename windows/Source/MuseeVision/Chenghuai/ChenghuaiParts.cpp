#include "Chenghuai/ChenghuaiParts.h"

namespace ChenghuaiParts
{
	namespace Kit = ChenghuaiKit;
	namespace CB = ChenghuaiBuild;
	using Kit::FMeshData;
	using Kit::FPart;
	using Kit::FProfile;

	namespace PartsImpl
	{
		constexpr double Skin = 0.015;

		/** A closed convex outline in (u, z) of a lattice window's opening. */
		TArray<FVector2D> WindowShape(int32 Pattern, double CU, double CZ, double R)
		{
			TArray<FVector2D> P;
			int32 N = 48;
			double Phase = 0.0;
			switch (Pattern)
			{
			case 0: N = 48; break;                                  // a circle (ice-crack)
			case 1: N = 4; Phase = 45.0; R *= 1.41421356; break;   // a square (coins)
			case 2: N = 6; Phase = 0.0; break;                      // a hexagon
			case 4: N = 8; Phase = 22.5; break;                     // an octagon
			default: N = 48; break;
			}
			for (int32 k = 0; k < N; ++k)
			{
				const double T = FMath::DegreesToRadians(Phase + 360.0 * k / N);
				P.Add(FVector2D(CU + R * FMath::Cos(T), CZ + R * FMath::Sin(T)));
			}
			return P;
		}

		/** Clips the segment A-B to a convex outline (either winding); false if nothing is left. */
		bool ClipToConvex(const TArray<FVector2D>& Poly, FVector2D& A, FVector2D& B)
		{
			const double Sign = Kit::Area2(Poly) > 0.0 ? 1.0 : -1.0;
			double T0 = 0.0, T1 = 1.0;
			const FVector2D D = B - A;
			for (int32 i = 0; i < Poly.Num(); ++i)
			{
				const FVector2D P = Poly[i], Q = Poly[(i + 1) % Poly.Num()];
				const FVector2D E = Q - P;
				// Inside: cross(E, X − P)·Sign ≥ 0.
				const double FA = Kit::Cross2(E, A - P) * Sign, FD = Kit::Cross2(E, D) * Sign;
				if (FMath::Abs(FD) < 1e-12) { if (FA < 0.0) { return false; } continue; }
				const double T = -FA / FD;
				if (FD > 0.0) { T0 = FMath::Max(T0, T); }
				else { T1 = FMath::Min(T1, T); }
				if (T0 > T1) { return false; }
			}
			const FVector2D A0 = A;
			A = A0 + D * T0;
			B = A0 + D * T1;
			return (B - A).Size() > 0.01;
		}

		/** A bar of the lattice from A to B in the wall's (u, z) plane, centred at v = VC. */
		void Bar(FPart& Part, const FLocal& L, const FVector2D& A, const FVector2D& B, double VC, double Width, double Depth)
		{
			const FVector PA = L.P(A.X, VC, A.Y), PB = L.P(B.X, VC, B.Y);
			Kit::Member(Part, TEXT("Window bar"), PA, PB, L.V3(), Depth, Width);
		}

		/** The lattice inside a window's outline, by pattern. */
		void WindowLattice(FChMeshes& Out, const FLocal& L, const TArray<FVector2D>& Shape, int32 Pattern, double CU, double CZ, double R, double VC, double Depth)
		{
			FPart& P = Out[CB::BrickFine];
			constexpr double W = 0.028;
			auto Seg = [&](FVector2D A, FVector2D B) { if (ClipToConvex(Shape, A, B)) { Bar(P, L, A, B, VC, W, Depth); } };
			if (Pattern == 1)
			{
				// Coins (钱纹): interlocking rings on a grid, each an octagon of bars.
				const double S = 0.2, Rr = 0.12;
				for (double U = CU - R; U <= CU + R + 1e-6; U += S)
				{
					for (double Z = CZ - R; Z <= CZ + R + 1e-6; Z += S)
					{
						for (int32 k = 0; k < 8; ++k)
						{
							const double T0 = FMath::DegreesToRadians(22.5 + 45.0 * k), T1 = FMath::DegreesToRadians(22.5 + 45.0 * (k + 1));
							Seg(FVector2D(U + Rr * FMath::Cos(T0), Z + Rr * FMath::Sin(T0)), FVector2D(U + Rr * FMath::Cos(T1), Z + Rr * FMath::Sin(T1)));
						}
					}
				}
			}
			else if (Pattern == 2)
			{
				// Hexagons (六角): a honeycomb.
				const double Hs = 0.11, Wd = Hs * 1.7320508;
				int32 Row = 0;
				for (double Z = CZ - R - Hs; Z <= CZ + R + Hs; Z += 1.5 * Hs, ++Row)
				{
					for (double U = CU - R - Wd + (Row % 2) * 0.5 * Wd; U <= CU + R + Wd; U += Wd)
					{
						for (int32 k = 0; k < 6; ++k)
						{
							const double T0 = FMath::DegreesToRadians(30.0 + 60.0 * k), T1 = FMath::DegreesToRadians(30.0 + 60.0 * (k + 1));
							Seg(FVector2D(U + Hs * FMath::Cos(T0), Z + Hs * FMath::Sin(T0)), FVector2D(U + Hs * FMath::Cos(T1), Z + Hs * FMath::Sin(T1)));
						}
					}
				}
			}
			else
			{
				// Ice-crack (冰裂): straight cracks that branch, seeded so each window is its own.
				TArray<TPair<FVector2D, FVector2D>> Cracks;
				int32 Seed = Pattern * 17 + 3;
				auto Rnd = [&Seed]() { Seed = (Seed * 1103515245 + 12345) & 0x7fffffff; return double(Seed % 10000) / 10000.0; };
				for (int32 k = 0; k < 9; ++k)
				{
					const double T = Rnd() * UE_DOUBLE_PI;
					const FVector2D C(CU + (Rnd() - 0.5) * R, CZ + (Rnd() - 0.5) * R);
					const FVector2D D(FMath::Cos(T), FMath::Sin(T));
					Cracks.Add(TPair<FVector2D, FVector2D>(C - D * 2.0 * R, C + D * 2.0 * R));
				}
				for (const TPair<FVector2D, FVector2D>& C : Cracks) { Seg(C.Key, C.Value); }
			}
			// The rim round the shape.
			for (int32 i = 0; i < Shape.Num(); ++i) { Bar(P, L, Shape[i], Shape[(i + 1) % Shape.Num()], VC, 0.05, Depth + 0.01); }
		}

		/**
		 * A panel of wall (u0 … u1, z0 … z1, v0 … v1) with a hole (a closed outline in (u, z)): both faces with the hole cut,
		 * the reveal round the hole, the panel's edges.
		 */
		void PanelWithHole(FChMeshes& Out, const FLocal& L, double U0, double U1, double Z0, double Z1, double V0, double V1,
						   const TArray<FVector2D>& Hole, int32 FrontPart, int32 BackPart, int32 RevealPart)
		{
			const TArray<FVector2D> Outer = {FVector2D(U0, Z0), FVector2D(U1, Z0), FVector2D(U1, Z1), FVector2D(U0, Z1)};
			const FString Asm = TEXT("Wall panel");
			auto UVOf = [](const FVector& P) { return FVector2D(P.X + P.Y, -P.Z); };
			Out[FrontPart].Begin(TEXT("Panel face"), false, Asm);
			Kit::Planar(Out[FrontPart].M, Outer, {Hole}, L.P(0.0, V0, 0.0), L.U3(), FVector::UpVector, -L.V3(), UVOf);
			Out[BackPart].Begin(TEXT("Panel face"), false, Asm);
			Kit::Planar(Out[BackPart].M, Outer, {Hole}, L.P(0.0, V1, 0.0), L.U3(), FVector::UpVector, L.V3(), UVOf);
			// The reveal: facing the hole's centre (in along its normal).
			FVector2D C = FVector2D::ZeroVector;
			for (const FVector2D& P : Hole) { C += P; }
			C /= Hole.Num();
			Out[RevealPart].Begin(TEXT("Reveal"), false, Asm);
			FMeshData& M = Out[RevealPart].M;
			for (int32 i = 0; i < Hole.Num(); ++i)
			{
				const FVector2D A = Hole[i], B = Hole[(i + 1) % Hole.Num()];
				const FVector2D Mid = 0.5 * (A + B);
				FVector2D E = (B - A).GetSafeNormal();
				FVector2D Nn(E.Y, -E.X);
				if (FVector2D::DotProduct(Nn, C - Mid) < 0.0) { Nn = -Nn; }
				const FVector N = L.U3() * Nn.X + FVector::UpVector * Nn.Y;
				M.Rect(L.P(A.X, V0, A.Y), L.P(B.X, V0, B.Y), L.P(B.X, V1, B.Y), L.P(A.X, V1, A.Y), N);
			}
			// The panel's own edges.
			Out[RevealPart].Begin(TEXT("Panel edge"), false, Asm);
			FMeshData& E = Out[RevealPart].M;
			E.Rect(L.P(U0, V0, Z0), L.P(U0, V1, Z0), L.P(U0, V1, Z1), L.P(U0, V0, Z1), -L.U3());
			E.Rect(L.P(U1, V0, Z0), L.P(U1, V1, Z0), L.P(U1, V1, Z1), L.P(U1, V0, Z1), L.U3());
			E.Rect(L.P(U0, V0, Z1), L.P(U1, V0, Z1), L.P(U1, V1, Z1), L.P(U0, V1, Z1), FVector::UpVector);
			E.Rect(L.P(U0, V0, Z0), L.P(U1, V0, Z0), L.P(U1, V1, Z0), L.P(U0, V1, Z0), -FVector::UpVector);
		}

		/** The moon gate's opening: a circle of radius R about (CU, CZ), cut flat at z = Cut (its threshold). */
		TArray<FVector2D> MoonOutline(double CU, double CZ, double R, double Cut)
		{
			TArray<FVector2D> P;
			const double S = FMath::Clamp((CZ - Cut) / R, -1.0, 1.0);
			const double A0 = -UE_DOUBLE_PI / 2 + FMath::Acos(S);   // the angle where the circle meets the cut, right side
			constexpr int32 N = 72;
			for (int32 k = 0; k <= N; ++k)
			{
				const double T = A0 + (2.0 * UE_DOUBLE_PI - 2.0 * (A0 + UE_DOUBLE_PI / 2)) * k / N;
				P.Add(FVector2D(CU + R * FMath::Cos(T), CZ + R * FMath::Sin(T)));
			}
			return P;
		}
	}
	using namespace PartsImpl;

	TArray<FVector2D> Rect(double X0, double Y0, double X1, double Y1)
	{
		return {FVector2D(X0, Y0), FVector2D(X1, Y0), FVector2D(X1, Y1), FVector2D(X0, Y1)};
	}

	void Paving(FChMeshes& Out, int32 Part, const TArray<FVector2D>& Outline, double Z, double Depth)
	{
		Kit::Prism(Out[Part], TEXT("Paving"), Outline, Z - Depth, Z, false);
	}

	void KerbRect(FChMeshes& Out, double X0, double Y0, double X1, double Y1, double W, double Z0, double Z1, int32 Part)
	{
		Kit::Box(Out[Part], TEXT("Kerb"), FVector(X0, Y0, Z0), FVector(X1, Y0 + W, Z1));
		Kit::Box(Out[Part], TEXT("Kerb"), FVector(X0, Y1 - W, Z0), FVector(X1, Y1, Z1));
		Kit::Box(Out[Part], TEXT("Kerb"), FVector(X0, Y0 + W, Z0), FVector(X0 + W, Y1 - W, Z1));
		Kit::Box(Out[Part], TEXT("Kerb"), FVector(X1 - W, Y0 + W, Z0), FVector(X1, Y1 - W, Z1));
	}

	void Coping(FChMeshes& Out, const FLocal& L, double U0, double U1, double VC, double Thick, double Z)
	{
		const double Hf = 0.5 * Thick + 0.14, Rise = 0.13;
		Kit::LBox(Out[CB::BrickFine], TEXT("Coping corbel"), L, U0 - 0.02, U1 + 0.02, VC - 0.5 * Thick - 0.05, VC + 0.5 * Thick + 0.05, Z - 0.07, Z + 0.001);
		const TArray<FVector2D> Bed = {FVector2D(VC - Hf, Z), FVector2D(VC + Hf, Z), FVector2D(VC + Hf, Z + 0.05), FVector2D(VC, Z + 0.05 + Rise),
									   FVector2D(VC - Hf, Z + 0.05)};
		Kit::Extrude(Out[CB::RoofMortar], TEXT("Coping bed"), Bed, L.P(0.0, 0.0, 0.0), L.V3(), FVector::UpVector, L.U3(), U0, U1);
		const ChenghuaiRoof::FTileStyle Style = ChenghuaiRoof::FTileStyle::He();
		for (const double Sign : {-1.0, 1.0})
		{
			ChenghuaiRoof::FSlope S;
			S.Origin = L.Plan(0.0, VC);
			S.DDir = L.V * Sign;
			S.SDir = L.U;
			S.Bed = [Z, Rise, Hf](double D, double) { return Z + 0.05 + Rise * FMath::Clamp(1.0 - D / Hf, 0.0, 1.0); };
			S.From = 0.06;
			S.To = Hf;
			S.bOrnaments = true;
			ChenghuaiRoof::TileRows(Out[CB::Tiles], S, U0, U1, Style, U0 + Style.PanHalf);
		}
		FProfile Ridge;
		Ridge.bClosed = true;
		Ridge.Add(-0.08, -0.04).Add(0.08, -0.04).Add(0.08, 0.06).Add(0.05, 0.11, true).Add(0.0, 0.125, true).Add(-0.05, 0.11, true).Add(-0.08, 0.06);
		Kit::SweepSolid(Out[CB::Ridges], TEXT("Coping ridge"), Kit::RunFrames(L.P(U0, VC, Z + 0.05 + Rise), L.P(U1, VC, Z + 0.05 + Rise), L.V3()), Ridge);
	}

	void Wall(FChMeshes& Out, const FLocal& L, double U0, double U1, double V0, double V1, const FWallSpec& W, const TArray<FHole>& InHoles)
	{
		TArray<FHole> Holes = InHoles;
		Holes.Sort([](const FHole& A, const FHole& B) { return A.U0 < B.U0; });
		const double F = W.Foot, ZB = F + W.BaseH, ZT = F + W.Height;
		const double BV0 = V0 + (W.FrontFace >= 0 ? Skin : 0.0), BV1 = V1 - (W.BackFace >= 0 ? Skin : 0.0);
		// A solid run of wall between Za and Zb (the base course below ZB, the body above, the skins on the faces).
		auto Run = [&](double A, double B, double Za, double Zb)
		{
			if (B - A < 0.005 || Zb - Za < 0.005) { return; }
			const double BaseTop = FMath::Min(Zb, ZB);
			if (Za < BaseTop) { Kit::LBox(Out[W.BasePart], TEXT("Wall base"), L, A, B, V0 - 0.012, V1 + 0.012, Za - (Za <= F + 1e-6 ? 0.1 : 0.0), BaseTop); }
			const double Z0 = FMath::Max(Za, ZB);
			if (Zb > Z0 + 0.005)
			{
				Kit::LBox(Out[W.BodyPart], TEXT("Wall"), L, A, B, BV0, BV1, Z0, Zb);
				if (W.FrontFace >= 0) { Kit::LBox(Out[W.FrontFace], TEXT("Wall skin"), L, A, B, V0, BV0, Z0, Zb); }
				if (W.BackFace >= 0) { Kit::LBox(Out[W.BackFace], TEXT("Wall skin"), L, A, B, BV1, V1, Z0, Zb); }
			}
		};
		double U = U0;
		constexpr double MoonMargin = 0.22;
		for (const FHole& H : Holes)
		{
			const bool bMoon = H.Kind == EHole::MoonGate;
			const double Ext0 = bMoon ? H.U0 - MoonMargin : H.U0, Ext1 = bMoon ? H.U1 + MoonMargin : H.U1;
			Run(U, Ext0, F, ZT);
			const double HA = H.U0, HB = H.U1;
			switch (H.Kind)
			{
			case EHole::Door:
			case EHole::DoorPlain:
			{
				Run(HA, HB, F + H.Z1, ZT);
				const int32 FramePart = H.Kind == EHole::Door ? CB::RedLacquer : CB::StoneKerb;
				const double FT = H.Kind == EHole::Door ? 0.1 : (V1 - V0) + 0.02;
				const double VC = 0.5 * (V0 + V1);
				Kit::LBox(Out[CB::StoneKerb], TEXT("Threshold"), L, HA - 0.05, HB + 0.05, V0 - 0.06, V1 + 0.06, F - 0.08, F + 0.06);
				Kit::LBox(Out[FramePart], TEXT("Door post"), L, HA, HA + 0.1, VC - 0.5 * FT, VC + 0.5 * FT, F + 0.06, F + H.Z1);
				Kit::LBox(Out[FramePart], TEXT("Door post"), L, HB - 0.1, HB, VC - 0.5 * FT, VC + 0.5 * FT, F + 0.06, F + H.Z1);
				Kit::LBox(Out[FramePart], TEXT("Door head"), L, HA, HB, VC - 0.5 * FT, VC + 0.5 * FT, F + H.Z1 - 0.12, F + H.Z1);
				if (H.Kind == EHole::Door)
				{
					// The leaves, open, folded back against the reveal.
					const double LW = 0.5 * (HB - HA - 0.2);
					Kit::LBox(Out[CB::RedLacquer], TEXT("Door leaf"), L, HA + 0.1, HA + 0.15, VC + 0.05, VC + 0.05 + LW, F + 0.07, F + H.Z1 - 0.13);
					Kit::LBox(Out[CB::RedLacquer], TEXT("Door leaf"), L, HB - 0.15, HB - 0.1, VC + 0.05, VC + 0.05 + LW, F + 0.07, F + H.Z1 - 0.13);
				}
				break;
			}
			case EHole::Window:
			{
				Run(HA, HB, F, F + H.Z0);
				Run(HA, HB, F + H.Z1, ZT);
				// A shaped opening in a panel filling the rectangle, and its lattice.
				const double CU = 0.5 * (HA + HB), CZ = F + 0.5 * (H.Z0 + H.Z1), R = 0.5 * FMath::Min(HB - HA, H.Z1 - H.Z0);
				const TArray<FVector2D> Shape = WindowShape(H.Pattern, CU, CZ, R * 0.92);
				PanelWithHole(Out, L, HA, HB, F + H.Z0, F + H.Z1, V0, V1, Shape, W.FrontFace >= 0 ? W.FrontFace : W.BodyPart,
							  W.BackFace >= 0 ? W.BackFace : W.BodyPart, CB::BrickFine);
				WindowLattice(Out, L, Shape, H.Pattern, CU, CZ, R * 0.92, 0.5 * (V0 + V1), 0.06);
				break;
			}
			case EHole::BlindWindow:
			{
				Run(HA, HB, F, ZT);
				const double CU = 0.5 * (HA + HB), CZ = F + 0.5 * (H.Z0 + H.Z1), R = 0.5 * FMath::Min(HB - HA, H.Z1 - H.Z0) * 0.92;
				const TArray<FVector2D> Shape = WindowShape(H.Pattern, CU, CZ, R);
				const double VC = H.bBlindOnFront ? V0 - 0.02 : V1 + 0.02;
				WindowLattice(Out, L, Shape, H.Pattern, CU, CZ, R, VC, 0.04);
				break;
			}
			case EHole::MoonGate:
			{
				const double CU = 0.5 * (HA + HB), R = 0.5 * (HB - HA);
				const double Cut = F + 0.1, CZ = Cut + R - 0.05;
				const double Margin = MoonMargin;
				// The panel with the round hole (the ring's square), and wall over it.
				const double PA = CU - R - Margin, PB = CU + R + Margin;
				const double PanelTop = FMath::Min(ZT, CZ + R + Margin);
				const TArray<FVector2D> Hole = MoonOutline(CU, CZ, R, Cut);
				PanelWithHole(Out, L, PA, PB, F - 0.1, PanelTop, V0, V1, Hole, W.FrontFace >= 0 ? W.FrontFace : W.BodyPart,
							  W.BackFace >= 0 ? W.BackFace : W.BodyPart, CB::BrickFine);
				Run(PA, PB, PanelTop, ZT);
				// The ring of rubbed grey brick standing proud of both faces, and the threshold stone.
				const TArray<FVector2D> RingOut = MoonOutline(CU, CZ, R + 0.13, Cut);
				for (const int32 Side : {0, 1})
				{
					const double VF = Side == 0 ? V0 : V1, Dir = Side == 0 ? -1.0 : 1.0;
					FPart& P = Out[CB::BrickFine];
					P.Begin(TEXT("Moon ring"), false, TEXT("Moon ring"));
					// An annulus: the outer and inner arcs joined along the cut (a flat band on the face).
					TArray<FVector2D> Band = RingOut;
					TArray<FVector2D> Inner = Hole;
					Algo::Reverse(Inner);
					Band.Append(Inner);
					Kit::Planar(P.M, Band, {}, L.P(0.0, VF + Dir * 0.025, 0.0), L.U3(), FVector::UpVector, L.V3() * Dir,
								[](const FVector& Q) { return FVector2D(Q.X + Q.Y, -Q.Z); });
					// Its outer edge.
					for (int32 i = 0; i + 1 < RingOut.Num(); ++i)
					{
						const FVector2D A = RingOut[i], B = RingOut[i + 1];
						const FVector2D Nn = ((A + B) * 0.5 - FVector2D(CU, CZ)).GetSafeNormal();
						P.M.Rect(L.P(A.X, VF, A.Y), L.P(B.X, VF, B.Y), L.P(B.X, VF + Dir * 0.025, B.Y), L.P(A.X, VF + Dir * 0.025, A.Y),
								 L.U3() * Nn.X + FVector::UpVector * Nn.Y);
					}
				}
				Kit::LBox(Out[CB::StoneKerb], TEXT("Moon gate sill"), L, CU - 0.75 * R, CU + 0.75 * R, V0 - 0.08, V1 + 0.08, F - 0.1, Cut + 0.002);
				break;
			}
			}
			U = Ext1;
		}
		Run(U, U1, F, ZT);
		if (W.bCoping) { Coping(Out, L, U0, U1, 0.5 * (V0 + V1), V1 - V0, ZT); }
	}
}
