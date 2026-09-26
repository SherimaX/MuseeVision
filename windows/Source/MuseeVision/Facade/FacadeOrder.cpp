#include "Facade/FacadeOrder.h"

namespace FacadeOrder
{
	using namespace FacadeKit;
	namespace Mo = FacadeKit::Moulding;

	// ================================================================================================ profiles

	FProfile PodiumProfile()
	{
		FProfile P;
		// The base course: a plinth 0.54 m proud, a rounded arris and a hollow up to the rustication.
		P.Add(-0.02, -0.12).Add(0.54, -0.12).Add(0.54, 0.36);
		P.Arc(0.50, 0.36, 0.04, 0.04, 0, 90, 4);
		P.Add(0.44, 0.40);
		P.Arc(0.44, 0.50, 0.06, 0.10, -90, -180, 5);
		P.Add(0.38, 0.56);
		// Two courses of banded rustication; the V joints on the stone's own coursing (0.6 m).
		P.Add(0.33, 0.60).Add(0.37, 0.64).Add(0.37, 1.16).Add(0.33, 1.20).Add(0.37, 1.24).Add(0.37, 1.76).Add(0.33, 1.80);
		// The cap: a fillet, an ovolo, a corona, its top washed back to the wall.
		P.Add(0.40, 1.80).Add(0.40, 1.84);
		Mo::Ovolo(P, 0.07, 0.08);
		P.Add(0.50, 1.92).Add(0.50, 2.00).Add(-0.02, 2.00);
		return P;
	}

	/** From (Fr, Zf), the frieze's face at its top: bed moulding, dentil band, ovolo, corona, sima; back to A = Back. */
	static void AppendCornice(FProfile& P, const FOrder& O, double Fr, double Zf, double Back)
	{
		const double D = O.D;
		Mo::CymaReversa(P, 0.07 * D, 0.085 * D);
		P.Add(Fr + 0.07 * D, Zf + 0.265 * D);
		P.Add(Fr + 0.21 * D, Zf + 0.265 * D);
		Mo::Ovolo(P, 0.14 * D, 0.12 * D);
		P.Add(Fr + 0.80 * D, Zf + 0.385 * D);
		P.Add(Fr + 0.80 * D, Zf + 0.615 * D);
		P.Add(Fr + 0.82 * D, Zf + 0.615 * D).Add(Fr + 0.82 * D, Zf + 0.645 * D);
		Mo::CymaRecta(P, 0.12 * D, 0.20 * D);
		P.Add(Fr + 0.94 * D, Zf + 0.875 * D);
		P.Add(Back, Zf + 0.875 * D + 0.02);
	}

	FProfile EntablatureProfile(const FOrder& O, double Face, bool bClosed)
	{
		const double D = O.D, Z = O.EntBottom();
		const double F3 = Face, F2 = F3 - 0.018 * D, F1 = F3 - 0.036 * D, Fr = F3 - 0.01 * D;
		FProfile P;
		P.bClosed = bClosed;
		P.Add(-0.02, Z);
		// The architrave: three fasciae and a crowning cyma.
		P.Add(F1, Z).Add(F1, Z + 0.16 * D).Add(F2, Z + 0.16 * D).Add(F2, Z + 0.33 * D).Add(F3, Z + 0.33 * D).Add(F3, Z + 0.50 * D);
		Mo::CymaReversa(P, 0.07 * D, 0.10 * D);
		P.Add(F3 + 0.07 * D, Z + 0.625 * D);
		// The frieze.
		P.Add(Fr, Z + 0.625 * D).Add(Fr, Z + 1.375 * D);
		AppendCornice(P, O, Fr, Z + 1.375 * D, -0.02);
		return P;
	}

	FProfile CorniceSection(const FOrder& O, double Back)
	{
		FProfile P;
		P.Add(-Back, 0.0).Add(0.0, 0.0);
		AppendCornice(P, O, 0.0, 0.0, -Back);
		return P;
	}

	FProfile SmallCornice(double Face, double Z0, double H)
	{
		FProfile P;
		P.Add(-0.02, Z0).Add(Face, Z0).Add(Face, Z0 + 0.3 * H);
		Mo::Ovolo(P, 0.15 * H, 0.15 * H);
		P.Add(Face + 0.55 * H, Z0 + 0.45 * H).Add(Face + 0.55 * H, Z0 + 0.72 * H);
		Mo::CymaRecta(P, 0.2 * H, 0.23 * H);
		P.Add(Face + 0.75 * H, Z0 + H).Add(-0.02, Z0 + H + 0.01);
		return P;
	}

	void Dentils(FMeshData& M, const FOrder& O, const FFaceLine& F, double U0, double U1, double Face)
	{
		const double D = O.D, Fr = Face - 0.01 * D, Zf = O.FriezeTop();
		const double W = 0.10 * D, Pitch = 0.16 * D;
		const int32 N = FMath::FloorToInt32((U1 - U0 - W) / Pitch);
		if (N < 1) { return; }
		const double Start = 0.5 * (U0 + U1) - 0.5 * N * Pitch;
		for (int32 i = 0; i <= N; ++i)
		{
			const FLocal L = F.Local(Start + i * Pitch);
			LBox(M, L, -W / 2, W / 2, Fr + 0.05 * D, Fr + 0.20 * D, Zf + 0.085 * D, Zf + 0.267 * D,
				 FMeshData::NegX | FMeshData::PosX | FMeshData::PosY | FMeshData::NegZ);
		}
	}

	// ================================================================================================ the order

	/** The Attic base: plinth, torus, scotia, torus; A from the shaft's face, B from the foot. */
	static FProfile AtticBase(double D, double Shift = 0.0, double Z0 = 0.0)
	{
		FProfile P;
		P.Add(-0.03, -0.02).Add(0.19 * D, -0.02).Add(0.19 * D, 0.16 * D).Add(0.13 * D, 0.16 * D);
		Mo::Torus(P, 0.06 * D);
		P.Add(0.10 * D, 0.28 * D).Add(0.10 * D, 0.30 * D);
		Mo::Scotia(P, 0.05 * D, 0.08 * D);
		P.Add(0.06 * D, 0.38 * D);
		Mo::Torus(P, 0.045 * D);
		P.Add(0.02 * D, 0.47 * D).Add(0.0, 0.50 * D).Add(-0.03, 0.50 * D);
		return Mo::Transform(P, 1.0, 1.0, Shift, Z0);
	}

	void Pilaster(FSink& S, const FOrder& O, const FLocal& L, double Bottom, bool bFluted)
	{
		const double D = O.D, H = D / 2, Fc = O.Face(), Bk = O.Skin - 0.04;
		auto Around = [Bk](double Half, double Front)
		{
			return TArray<FVector2D>{FVector2D(-Half, Bk), FVector2D(-Half, Front), FVector2D(Half, Front), FVector2D(Half, Bk)};
		};
		LocalSweep(S.Carved, L, Around(H, Fc), false, AtticBase(D), Bottom);

		// The shaft: its plan section (seven flutes) swept up.
		FProfile Sec;
		Sec.bClosed = true;
		Sec.Add(H, Bk).Add(H, Fc);
		if (bFluted)
		{
			const double Margin = 0.078 * D, Fw = 0.094 * D, Fillet = 0.031 * D, Depth = 0.033 * D;
			double A = H - Margin;
			Sec.Add(A, Fc);
			for (int32 f = 0; f < 7; ++f)
			{
				for (int32 s = 1; s <= 6; ++s)
				{
					const double T = s / 6.0;
					Sec.Add(A - Fw * T, Fc - Depth * FMath::Sin(Pi * T), s < 6);
				}
				A -= Fw;
				if (f < 6)
				{
					A -= Fillet;
					Sec.Add(A, Fc);
				}
			}
		}
		Sec.Add(-H, Fc).Add(-H, Bk);
		TArray<FSweepFrame> Run;
		for (const double Z : {Bottom + 0.48 * D, O.ShaftTop() + 0.01})
		{
			FSweepFrame F;
			F.Origin = L.At(0.0, 0.0, Z);
			F.AxisA = F.NormA = L.U;
			F.AxisB = F.NormB = L.N;
			F.S = Z;
			Run.Add(F);
		}
		SalonKit::Sweep(S.Shafts, Run, Sec);

		// The astragal, the echinus, the canalis and its volutes, the abacus.
		const double Zc = O.ShaftTop();
		FProfile Bead;
		Bead.Add(-0.03, 0.0).Add(0.0, 0.0);
		Mo::Torus(Bead, 0.03 * D, 6);
		Bead.Add(-0.03, 0.06 * D);
		LocalSweep(S.Carved, L, Around(H, Fc), false, Bead, Zc - 0.075 * D);
		FProfile Echinus;
		Echinus.Add(-0.03, 0.0).Add(0.0, 0.0);
		Mo::Ovolo(Echinus, 0.07 * D, 0.12 * D);
		Echinus.Add(0.07 * D, 0.13 * D).Add(-0.03, 0.13 * D);
		LocalSweep(S.Carved, L, Around(H, Fc), false, Echinus, Zc);
		LBox(S.Carved, L, -0.52 * D, 0.52 * D, Bk, Fc + 0.085 * D, Zc + 0.12 * D, Zc + 0.325 * D);
		for (const double Side : {-1.0, 1.0})
		{
			Volute(S.Carved, L, Side * 0.52 * D, Zc + 0.10 * D, 0.22 * D, Side, Bk, Fc + 0.10 * D, 0.014 * D, false);
		}
		FProfile Abacus;
		Abacus.Add(-0.03, 0.0).Add(0.0, 0.0).Add(0.0, 0.05 * D);
		Mo::Ovolo(Abacus, 0.035 * D, 0.05 * D);
		Abacus.Add(0.035 * D, 0.13 * D).Add(-0.03, 0.13 * D);
		LocalSweep(S.Carved, L, Around(0.58 * D, Fc + 0.10 * D), false, Abacus, Zc + 0.32 * D);
	}

	void Column(FSink& S, const FOrder& O, const FLocal& L)
	{
		const double D = O.D, R = D / 2, RTop = 5.0 / 12.0 * D, Zb = O.Base;
		const FVector Foot = L.At(0.0, 0.0, 0.0);
		// The square plinth and the base's mouldings, turned.
		LBox(S.Carved, L, -0.69 * D, 0.69 * D, -0.69 * D, 0.69 * D, Zb - 0.02, Zb + 0.16 * D);
		{
			FProfile B;
			B.Add(0.0, Zb + 0.16 * D).Add(R + 0.13 * D, Zb + 0.16 * D);
			Mo::Torus(B, 0.06 * D);
			B.Add(R + 0.10 * D, Zb + 0.28 * D).Add(R + 0.10 * D, Zb + 0.30 * D);
			Mo::Scotia(B, 0.05 * D, 0.08 * D);
			B.Add(R + 0.06 * D, Zb + 0.38 * D);
			Mo::Torus(B, 0.045 * D);
			B.Add(R + 0.02 * D, Zb + 0.47 * D).Add(R, Zb + 0.50 * D).Add(0.0, Zb + 0.50 * D);
			Lathe(S.Carved, Foot, B, 40);
		}
		// The shaft: 24 flutes with fillets, straight for its lower third, then diminishing to 5/6 D (Vignola's entasis).
		{
			const double Z0 = Zb + 0.49 * D, Z1 = O.ShaftTop() + 0.01;
			constexpr int32 Flutes = 24, PerFlute = 5, NV = 30;
			const int32 NU = Flutes * PerFlute;
			const double Depth = 0.03 * D, Fillet = 0.2, Pitch = 2.0 * Pi / Flutes;
			const double Phase = FMath::Atan2(-L.N.Y, -L.N.X);   // the seam at the back
			auto Radius = [&](double Z)
			{
				const double T = FMath::Clamp((Z - Z0) / (Z1 - Z0), 0.0, 1.0);
				if (T <= 1.0 / 3.0) { return R; }
				return R - (R - RTop) * FMath::Pow((T - 1.0 / 3.0) / (2.0 / 3.0), 1.4);
			};
			auto Angle = [&](int32 i, double& OutDepth)
			{
				const int32 f = i / PerFlute, j = i % PerFlute;
				if (j == 0)
				{
					OutDepth = 0.0;
					return Phase + f * Pitch;
				}
				const double T = (j - 1) / double(PerFlute - 1);
				OutDepth = Depth * FMath::Sin(Pi * T);
				return Phase + f * Pitch + Pitch * (Fillet + (1.0 - Fillet) * T);
			};
			S.Shafts.Patch(NU, NV,
				[&](int32 i, int32 j)
				{
					double Dp = 0.0;
					const double A = Angle(i % NU, Dp) + (i == NU ? 2.0 * Pi : 0.0);
					const double Z = Z0 + (Z1 - Z0) * j / NV;
					const double Rz = Radius(Z);
					const double Rr = Rz - Dp * Rz / R;
					return Foot + FVector(Rr * FMath::Cos(A), Rr * FMath::Sin(A), Z);
				},
				[](const FVector& P) { return FVector2D(P.X + P.Y, -P.Z); },
				[Foot](const FVector& P) { return FVector(P.X - Foot.X, P.Y - Foot.Y, 0.0); });
		}
		// The astragal, the echinus, the cushion with its volutes (front and back), the abacus.
		const double Zc = O.ShaftTop();
		{
			FProfile Bead;
			Bead.Add(0.0, Zc - 0.075 * D).Add(RTop, Zc - 0.075 * D);
			Mo::Torus(Bead, 0.03 * D, 6);
			Bead.Add(0.0, Zc - 0.015 * D);
			Lathe(S.Carved, Foot, Bead, 40);
			FProfile Ech;
			Ech.Add(0.0, Zc - 0.02).Add(RTop, Zc - 0.02).Add(RTop, Zc);
			Mo::Ovolo(Ech, 0.10 * D, 0.13 * D);
			Ech.Add(0.0, Zc + 0.13 * D);
			Lathe(S.Carved, Foot, Ech, 40);
		}
		LBox(S.Carved, L, -0.52 * D, 0.52 * D, -0.44 * D, 0.44 * D, Zc + 0.12 * D, Zc + 0.325 * D);
		for (const double Side : {-1.0, 1.0})
		{
			Volute(S.Carved, L, Side * 0.52 * D, Zc + 0.10 * D, 0.22 * D, Side, -0.46 * D, 0.46 * D, 0.014 * D, true);
		}
		LBox(S.Carved, L, -0.57 * D, 0.57 * D, -0.57 * D, 0.57 * D, Zc + 0.32 * D, Zc + 0.40 * D);
		LBox(S.Carved, L, -0.60 * D, 0.60 * D, -0.60 * D, 0.60 * D, Zc + 0.40 * D, Zc + 0.45 * D);
	}

	// ================================================================================================ windows and the door

	static TArray<FVector2D> OpeningPath(const FOpening& O, double Foot, bool& bClosed)
	{
		TArray<FVector2D> Path;
		const double H = O.Half;
		bClosed = false;
		switch (O.Kind)
		{
		case FOpening::EKind::Rect:
			AppendPath(Path, {FVector2D(-H, Foot), FVector2D(-H, O.Spring), FVector2D(H, O.Spring), FVector2D(H, Foot)});
			break;
		case FOpening::EKind::Arch:
		{
			TArray<FVector2D> Pts = {FVector2D(-H, Foot)};
			for (int32 k = 0; k <= FOpening::Segs; ++k)
			{
				const double T = Pi - Pi * k / FOpening::Segs;
				Pts.Add(FVector2D(H * FMath::Cos(T), O.Spring + H * FMath::Sin(T)));
			}
			Pts.Add(FVector2D(H, Foot));
			AppendPath(Path, Pts);
			break;
		}
		case FOpening::EKind::Round:
			bClosed = true;
			for (int32 k = 0; k < 2 * FOpening::Segs; ++k)
			{
				const double T = Pi - Pi * k / FOpening::Segs;
				Path.Add(FVector2D(H * FMath::Cos(T), O.Sill + H * FMath::Sin(T)));
			}
			break;
		}
		return Path;
	}

	/** An architrave's section: A out of the skin, B away from the opening; two fasciae and an outer cyma. */
	static FProfile ArchitraveProfile(double Fw, double Proj)
	{
		const double K = Proj / 0.15;
		FProfile P;
		P.Add(-0.07, 0.0).Add(0.09 * K, 0.0).Add(0.09 * K, 0.42 * Fw).Add(0.11 * K, 0.42 * Fw).Add(0.11 * K, 0.78 * Fw);
		Mo::CymaReversa(P, 0.04 * K, 0.18 * Fw);
		P.Add(0.15 * K, Fw).Add(-0.07, Fw);
		return P;
	}

	/** A glazing bar between two points of the face plane, W wide, from D0 to D1. */
	static void Bar(FMeshData& M, const FLocal& L, const FVector2D& A, const FVector2D& B, double W, double D0, double D1)
	{
		const FVector2D T = (B - A).GetSafeNormal(), N(-T.Y, T.X);
		FacePrism(M, L, {A - N * W / 2, B - N * W / 2, B + N * W / 2, A + N * W / 2}, D0, D1, false);
	}

	void WindowDress(FSink& S, const FFaceLine& F, const FWindow& W, double T, double Back)
	{
		const FOpening& O = W.Open;
		const FLocal L = F.Local(O.U);
		const double H = O.Half, Fw = W.Frame, K = Fw / 0.24;
		const bool bRound = O.Kind == FOpening::EKind::Round;

		// The architrave round the opening.
		bool bClosed = false;
		const TArray<FVector2D> Path = OpeningPath(O, O.Sill - 0.02, bClosed);
		FaceSweep(S.Carved, L, Path, bClosed, T, ArchitraveProfile(Fw, 0.15 * K));

		// The keystone at the crown.
		if (W.bKeystone && O.Kind != FOpening::EKind::Rect)
		{
			const double Crown = O.Top();
			FacePrism(S.Carved, L, {FVector2D(-0.5 * Fw, Crown - 0.08 * K), FVector2D(0.5 * Fw, Crown - 0.08 * K), FVector2D(0.72 * Fw, Crown + 1.9 * Fw),
									FVector2D(-0.72 * Fw, Crown + 1.9 * Fw)}, T - 0.08, T + 0.19 * K);
		}
		// The sill on two consoles, and the apron panel under it.
		if (W.bSill && !bRound)
		{
			const double SillH = 0.15 * K, Reach = H + Fw + 0.12 * K;
			LBox(S.Carved, L, -Reach, Reach, T - 0.12, T + 0.21 * K, O.Sill - SillH, O.Sill + 0.012);
			LBox(S.Carved, L, -Reach - 0.02, Reach + 0.02, T + 0.14 * K, T + 0.23 * K, O.Sill - SillH - 0.02, O.Sill - SillH + 0.05 * K);
			for (const double Side : {-1.0, 1.0})
			{
				const double C = Side * (H + 0.5 * Fw);
				LBox(S.Carved, L, C - 0.09 * K, C + 0.09 * K, T - 0.08, T + 0.15 * K, O.Sill - SillH - 0.42 * K, O.Sill - SillH + 0.01);
			}
			if (W.bApron)
			{
				LBox(S.Carved, L, -H, H, T - 0.03, T + 0.035 * K, O.Sill - SillH - 1.05 * K, O.Sill - SillH - 0.20 * K);
			}
		}
		// Glazing bars in the opening.
		if (O.bGlazed)
		{
			const double BW = 0.045, D0 = Back + 0.015, D1 = Back + 0.07;
			const double Lo = O.Bottom();
			for (int32 m = 1; m <= W.Mullions; ++m)
			{
				const double A = -H + 2.0 * H * m / (W.Mullions + 1);
				const double Top = bRound ? O.Upper(O.U + A) : O.Spring;
				const double Bot = bRound ? O.Lower(O.U + A) : Lo;
				Bar(S.Bronze, L, FVector2D(A, Bot - 0.01), FVector2D(A, Top + 0.01), BW, D0, D1);
			}
			if (bRound)
			{
				Bar(S.Bronze, L, FVector2D(-H - 0.01, O.Sill), FVector2D(H + 0.01, O.Sill), BW, D0, D1);
			}
			else
			{
				const double Head = O.Spring;
				for (double Z = Lo + W.Transom; Z < Head - 0.25; Z += W.Transom)
				{
					Bar(S.Bronze, L, FVector2D(-H - 0.01, Z), FVector2D(H + 0.01, Z), BW, D0, D1);
				}
				if (O.Kind == FOpening::EKind::Arch)
				{
					// The fanlight: a bar on the springing, radial bars, and a small half ring.
					const FVector2D C(0.0, O.Spring);
					Bar(S.Bronze, L, FVector2D(-H - 0.01, O.Spring), FVector2D(H + 0.01, O.Spring), BW, D0, D1);
					const double R0 = 0.32 * H;
					for (const double Deg : {30.0, 60.0, 90.0, 120.0, 150.0})
					{
						const double Rad = FMath::DegreesToRadians(Deg);
						const FVector2D Dir(FMath::Cos(Rad), FMath::Sin(Rad));
						Bar(S.Bronze, L, C + Dir * R0, C + Dir * (H + 0.01), BW * 0.8, D0, D1);
					}
					TArray<FVector2D> Ring;
					constexpr int32 RS = 16;
					for (int32 k = 0; k <= RS; ++k) { Ring.Add(C + FVector2D(FMath::Cos(Pi * k / RS), FMath::Sin(Pi * k / RS)) * (R0 + BW / 2)); }
					for (int32 k = RS; k >= 0; --k) { Ring.Add(C + FVector2D(FMath::Cos(Pi * k / RS), FMath::Sin(Pi * k / RS)) * (R0 - BW / 2)); }
					FacePrism(S.Bronze, L, Ring, D0, D1, false);
				}
				else if (W.Mullions > 0)
				{
					Bar(S.Bronze, L, FVector2D(-H - 0.01, O.Spring - 0.02), FVector2D(H + 0.01, O.Spring - 0.02), BW, D0, D1);
				}
			}
		}
		if (W.Medallion > 0.0) { Medallion(S.Carved, L, T, W.MedallionZ, W.Medallion); }
	}

	void DoorDress(FSink& S, const FFaceLine& F, const FOpening& Door, double T, double Back)
	{
		const FLocal L = F.Local(Door.U);
		const double H = Door.Half, Fw = 0.36, Head = Door.Spring, Sill = Door.Sill;
		bool bClosed = false;
		FaceSweep(S.Carved, L, OpeningPath(Door, Sill - 0.02, bClosed), false, T, ArchitraveProfile(Fw, 0.18));

		// Two bronze leaves with raised and fielded panels, a meeting stile, ring pulls.
		for (const double Side : {-1.0, 1.0})
		{
			const double A0 = Side < 0 ? -H : 0.004, A1 = Side < 0 ? -0.004 : H;
			LBox(S.Bronze, L, A0, A1, Back - 0.01, Back + 0.06, Sill, Head + 0.01);
			const double Heights[4][2] = {{Sill + 0.25, Sill + 1.25}, {Sill + 1.50, Sill + 3.55}, {Sill + 3.80, Head - 0.30}, {0, 0}};
			for (int32 p = 0; p < 3; ++p)
			{
				const double Z0 = Heights[p][0], Z1 = Heights[p][1];
				if (Z1 - Z0 < 0.2) { continue; }
				LBox(S.Bronze, L, A0 + 0.14, A1 - 0.14, Back + 0.06, Back + 0.085, Z0, Z1, FMeshData::AllFaces & ~FMeshData::NegY);
				LBox(S.Bronze, L, A0 + 0.22, A1 - 0.22, Back + 0.085, Back + 0.10, Z0 + 0.08, Z1 - 0.08, FMeshData::AllFaces & ~FMeshData::NegY);
			}
			// A domed boss for the pull.
			FProfile Boss;
			Boss.Add(0.035, 0.0).Add(0.035, 0.035);
			Boss.Arc(0.0, 0.035, 0.035, 0.035, 0, 90, 6);
			Boss.Add(-0.01, 0.07);
			FaceLathe(S.Bronze, L, Side * 0.22, Back + 0.10, Sill + 1.35, Boss, 16);
		}
		LBox(S.Bronze, L, -0.03, 0.03, Back + 0.06, Back + 0.11, Sill, Head);

		// The door's head: a frieze and a cornice on two consoles.
		const double Hc = H + Fw + 0.28, Zh = Head + Fw + 0.02;
		FProfile Cornice;
		Cornice.Add(-0.29, Zh).Add(0.0, Zh).Add(0.0, Zh + 0.55);
		Mo::CymaReversa(Cornice, 0.06, 0.08);
		Cornice.Add(0.06, Zh + 0.70);
		Mo::Ovolo(Cornice, 0.10, 0.10);
		Cornice.Add(0.42, Zh + 0.80).Add(0.42, Zh + 0.98);
		Mo::CymaRecta(Cornice, 0.12, 0.18);
		Cornice.Add(0.54, Zh + 1.20).Add(-0.29, Zh + 1.21);
		LocalSweep(S.Carved, L, {FVector2D(-Hc, T - 0.03), FVector2D(-Hc, T + 0.26), FVector2D(Hc, T + 0.26), FVector2D(Hc, T - 0.03)}, false, Cornice, 0.0);
		for (const double Side : {-1.0, 1.0})
		{
			const double C = Side * (Hc - 0.14);
			LBox(S.Carved, L, C - 0.13, C + 0.13, T - 0.03, T + 0.36, Zh - 0.85, Zh + 0.02);
			LBox(S.Carved, L, C - 0.10, C + 0.10, T - 0.03, T + 0.28, Zh - 1.05, Zh - 0.84);
		}
	}

	// ================================================================================================ balustrades

	static FProfile RailProfile(const FBalustrade& B)
	{
		const double F = B.Front, K = B.Front - B.Width, Z1 = B.Z0 + B.H, Zr = B.Z0 + 0.8 * B.H;
		FProfile P;
		P.bClosed = true;
		P.Add(K + 0.01, Zr).Add(F - 0.01, Zr);
		Mo::Ovolo(P, 0.05, 0.05, 4);
		P.Add(F + 0.04, Z1 - 0.03).Add(F + 0.01, Z1).Add(K - 0.01, Z1).Add(K - 0.04, Z1 - 0.03).Add(K - 0.04, Zr + 0.05);
		return P;
	}

	static FProfile PlinthProfile(const FBalustrade& B)
	{
		const double F = B.Front, K = B.Front - B.Width, Z0 = B.Z0, H = B.H;
		FProfile P;
		P.bClosed = true;
		P.Add(K - 0.03, Z0 - 0.03).Add(F + 0.03, Z0 - 0.03).Add(F + 0.03, Z0 + 0.10 * H).Add(F, Z0 + 0.14 * H).Add(F - 0.005, Z0 + 0.2 * H)
			.Add(K + 0.005, Z0 + 0.2 * H).Add(K, Z0 + 0.14 * H).Add(K - 0.03, Z0 + 0.10 * H);
		return P;
	}

	void BalustradeRails(FSink& S, const FBalustrade& B, const TArray<FVector2D>& Path, bool bClosed)
	{
		PlanSweep(S.Carved, Path, bClosed, RailProfile(B));
		PlanSweep(S.Carved, Path, bClosed, PlinthProfile(B));
	}

	static const FProfile& BalusterTemplate()
	{
		static const FProfile P = []
		{
			FProfile Q;
			const double Pts[][2] = {{0, 0}, {0.078, 0}, {0.078, 0.06}, {0.062, 0.075}, {0.08, 0.11}, {0.098, 0.17}, {0.104, 0.23}, {0.098, 0.29},
									 {0.082, 0.35}, {0.062, 0.41}, {0.048, 0.46}, {0.044, 0.50}, {0.05, 0.52}, {0.066, 0.53}, {0.066, 0.57},
									 {0.05, 0.58}, {0.058, 0.62}, {0.078, 0.635}, {0.078, 0.69}, {0, 0.69}};
			for (int32 i = 0; i < UE_ARRAY_COUNT(Pts); ++i) { Q.Add(Pts[i][0], Pts[i][1], i >= 4 && i <= 11); }
			return Q;
		}();
		return P;
	}

	void Balusters(FSink& S, const FBalustrade& B, const FFaceLine& F, double U0, double U1)
	{
		const double Hb = 0.6 * B.H, Sc = Hb / 0.69;
		const int32 N = FMath::FloorToInt32((U1 - U0) / B.Pitch);
		if (N < 1) { return; }
		const double Step = (U1 - U0) / N;
		const FProfile P = Mo::Transform(BalusterTemplate(), Sc, Sc, 0.0, 0.0);
		for (int32 i = 0; i < N; ++i)
		{
			const FVector At = F.At(U0 + Step * (i + 0.5), B.Front - 0.5 * B.Width, B.Z0 + 0.2 * B.H - 0.005);
			Lathe(S.Carved, At, P, 8);
		}
	}

	void Pedestal(FSink& S, const FBalustrade& B, const FLocal& L, double Extra)
	{
		const double W = B.PedestalWidth / 2, F = B.Front, K = B.Front - B.Width, H = B.H;
		LBox(S.Carved, L, -W - 0.04, W + 0.04, K - 0.07, F + 0.07, B.Z0 - 0.03, B.Z0 + 0.12 * H);
		LBox(S.Carved, L, -W, W, K - 0.03, F + 0.03, B.Z0 + 0.10 * H, B.Z0 + 0.86 * H);
		LBox(S.Carved, L, -W - 0.06, W + 0.06, K - 0.09, F + 0.09, B.Z0 + 0.84 * H, B.Z0 + H + 0.04 + Extra);
	}

	void Urn(FMeshData& M, const FVector& Foot, double Height)
	{
		const double Sc = Height / 1.2;
		const double Pts[][2] = {{0, 0}, {0.20, 0}, {0.20, 0.07}, {0.12, 0.12}, {0.09, 0.18}, {0.10, 0.22}, {0.16, 0.27}, {0.25, 0.36}, {0.30, 0.47},
								 {0.30, 0.60}, {0.25, 0.72}, {0.18, 0.79}, {0.15, 0.84}, {0.21, 0.87}, {0.22, 0.93}, {0.15, 0.97}, {0.12, 1.02},
								 {0.07, 1.07}, {0.09, 1.11}, {0.06, 1.17}, {0, 1.20}};
		FProfile P;
		for (int32 i = 0; i < UE_ARRAY_COUNT(Pts); ++i) { P.Add(Pts[i][0] * Sc, Pts[i][1] * Sc, (i >= 5 && i <= 11) || i == 18); }
		Lathe(M, Foot, P, 28);
	}

	void Medallion(FMeshData& M, const FLocal& L, double T, double Z, double R)
	{
		const double Tr = 0.13 * R;
		FProfile P;
		P.Add(0.025, 0.0).Add(0.025, R - 2.0 * Tr);
		P.Arc(0.025, R - Tr, 0.8 * Tr, Tr, -90, 90, 8);
		P.Add(-0.07, R);
		FaceLathe(M, L, 0.0, T, Z, P, 40);
	}

	// ================================================================================================ the inscription

	namespace Glyphs
	{
		using FPoly = TArray<FVector2D>;
		constexpr double Thick = 0.15, Thin = 0.065, Serif = 0.05;

		void Rect(TArray<FPoly>& P, double X0, double X1, double Y0, double Y1)
		{
			P.Add({FVector2D(X0, Y0), FVector2D(X1, Y0), FVector2D(X1, Y1), FVector2D(X0, Y1)});
		}

		/** A diagonal stroke cut level at both ends, W wide measured level. */
		void Diag(TArray<FPoly>& P, const FVector2D& A, const FVector2D& B, double W)
		{
			const FVector2D H(0.5 * W, 0.0);
			P.Add({A - H, A + H, B + H, B - H});
		}

		/** A curved stroke along a centreline, thick where it runs up and down, thin where it runs across (the Roman stress). */
		void Ribbon(TArray<FPoly>& P, const TArray<FVector2D>& Line)
		{
			const int32 N = Line.Num();
			TArray<FVector2D> Nrm;
			TArray<double> W;
			for (int32 i = 0; i < N; ++i)
			{
				const FVector2D T = (Line[FMath::Min(N - 1, i + 1)] - Line[FMath::Max(0, i - 1)]).GetSafeNormal();
				Nrm.Add(FVector2D(-T.Y, T.X));
				W.Add(Thin + (Thick - Thin) * FMath::Abs(T.Y));
			}
			for (int32 i = 0; i + 1 < N; ++i)
			{
				P.Add({Line[i] - Nrm[i] * 0.5 * W[i], Line[i + 1] - Nrm[i + 1] * 0.5 * W[i + 1], Line[i + 1] + Nrm[i + 1] * 0.5 * W[i + 1],
					   Line[i] + Nrm[i] * 0.5 * W[i]});
			}
		}

		TArray<FVector2D> Ellipse(const FVector2D& C, double RX, double RY, double Deg0, double Deg1, int32 Steps)
		{
			TArray<FVector2D> Out;
			for (int32 i = 0; i <= Steps; ++i)
			{
				const double A = FMath::DegreesToRadians(Deg0 + (Deg1 - Deg0) * i / Steps);
				Out.Add(C + FVector2D(RX * FMath::Cos(A), RY * FMath::Sin(A)));
			}
			return Out;
		}

		/** A letter's strokes in cap heights (x right, y up from the baseline); returns its advance. */
		double Build(TCHAR C, TArray<FPoly>& P)
		{
			const double S = Serif;
			switch (C)
			{
			case TEXT('I'):
				Rect(P, 0.08, 0.08 + Thick, 0.0, 1.0);
				Rect(P, 0.0, 0.31, 0.0, S);
				Rect(P, 0.0, 0.31, 1.0 - S, 1.0);
				return 0.31;
			case TEXT('E'):
			case 0x00C9:
				Rect(P, 0.08, 0.08 + Thick, 0.0, 1.0);
				Rect(P, 0.0, 0.25, 0.0, S);
				Rect(P, 0.0, 0.25, 1.0 - S, 1.0);
				Rect(P, 0.08, 0.60, 0.0, 0.075);
				Rect(P, 0.08, 0.57, 0.93, 1.0);
				Rect(P, 0.08, 0.48, 0.47, 0.535);
				Rect(P, 0.555, 0.605, 0.0, 0.2);
				Rect(P, 0.525, 0.57, 0.82, 1.0);
				Rect(P, 0.445, 0.485, 0.42, 0.585);
				if (C != TEXT('E')) { Diag(P, FVector2D(0.29, 1.09), FVector2D(0.44, 1.25), 0.075); }
				return 0.62;
			case TEXT('M'):
				Rect(P, 0.08, 0.08 + Thin, 0.0, 1.0);
				Diag(P, FVector2D(0.14, 1.0), FVector2D(0.47, 0.0), Thick);
				Diag(P, FVector2D(0.52, 0.0), FVector2D(0.845, 1.0), Thin);
				Rect(P, 0.80, 0.80 + Thick, 0.0, 1.0);
				Rect(P, 0.0, 0.23, 0.0, S);
				Rect(P, 0.72, 1.03, 0.0, S);
				Rect(P, 0.0, 0.17, 1.0 - S, 1.0);
				Rect(P, 0.80, 1.03, 1.0 - S, 1.0);
				return 1.03;
			case TEXT('N'):
				Rect(P, 0.08, 0.08 + Thin, 0.0, 1.0);
				Diag(P, FVector2D(0.16, 1.0), FVector2D(0.73, 0.0), Thick);
				Rect(P, 0.74, 0.74 + Thin, 0.0, 1.0);
				Rect(P, 0.0, 0.23, 0.0, S);
				Rect(P, 0.0, 0.2, 1.0 - S, 1.0);
				Rect(P, 0.66, 0.89, 1.0 - S, 1.0);
				return 0.89;
			case TEXT('V'):
				Diag(P, FVector2D(0.13, 1.0), FVector2D(0.43, 0.0), Thick);
				Diag(P, FVector2D(0.45, 0.0), FVector2D(0.77, 1.0), Thin);
				Rect(P, 0.0, 0.27, 1.0 - S, 1.0);
				Rect(P, 0.66, 0.88, 1.0 - S, 1.0);
				return 0.88;
			case TEXT('O'):
			{
				const FVector2D Cn(0.45, 0.5);
				const TArray<FVector2D> Out = Ellipse(Cn, 0.44, 0.515, 0, 360, 48), In = Ellipse(Cn, 0.285, 0.43, 0, 360, 48);
				for (int32 i = 0; i < 48; ++i) { P.Add({In[i], Out[i], Out[i + 1], In[i + 1]}); }
				return 0.9;
			}
			case TEXT('S'):
			{
				TArray<FVector2D> Line = Ellipse(FVector2D(0.29, 0.745), 0.215, 0.21, 25, 270, 20);
				Line.Pop();
				Line.Append(Ellipse(FVector2D(0.30, 0.275), 0.24, 0.26, 90, -150, 22));
				Ribbon(P, Line);
				Rect(P, 0.46, 0.51, 0.72, 0.9);     // the terminals' beaks
				Rect(P, 0.06, 0.11, 0.09, 0.27);
				return 0.6;
			}
			case 0x00B7:
				P.Add({FVector2D(0.1, 0.44), FVector2D(0.18, 0.52), FVector2D(0.1, 0.60), FVector2D(0.02, 0.52)});
				return 0.2;
			default:
				return 0.32;   // a space
			}
		}
	}

	void Inscription(FMeshData& M, const FLocal& L, double CentreA, double Baseline, double CapH, double D0, double D1, const TCHAR* Text, double Tracking)
	{
		TArray<TArray<Glyphs::FPoly>> Letters;
		TArray<double> Advances;
		double Total = 0.0;
		for (const TCHAR* C = Text; *C; ++C)
		{
			TArray<Glyphs::FPoly> P;
			const double W = Glyphs::Build(*C, P);
			Letters.Add(P);
			Advances.Add(W);
			Total += W + (C[1] ? Tracking : 0.0);
		}
		double Pen = CentreA / CapH - 0.5 * Total;
		for (int32 i = 0; i < Letters.Num(); ++i)
		{
			for (const Glyphs::FPoly& Poly : Letters[i])
			{
				TArray<FVector2D> Q;
				for (const FVector2D& V : Poly) { Q.Add(FVector2D((Pen + V.X) * CapH, Baseline + V.Y * CapH)); }
				FacePrism(M, L, Q, D0, D1, false);
			}
			Pen += Advances[i] + Tracking;
		}
	}
}
