// The Nymphéas pond's edge (the pond-edge redesign, 2026-09-26): the moulded travertine coping with its gilt-bronze lip,
// the carved lily frieze, the lining, the planting shelves and the tray (ASalonPond); the planting at the ends of the long
// axis (ASalonPondPlants). See Salon/SalonInterior.h.

#include "Salon/SalonInterior.h"

#include "Geometry/MuseeBake.h"
#include "Materials/MaterialInterface.h"
#include "Nature/NatureMesh.h"
#include "Async/ParallelFor.h"
#include "ProceduralMeshComponent.h"
#include "Salon/SalonKit.h"

/** Plan metres throughout (x east, plan y south, z up above the oval's floor), written in centimetres by FMeshData. */
namespace SalonPondBuild
{
	using SalonKit::FMeshData;

	constexpr double kCX = -86.5;                 // the pond's centre (plan x; y 0)
	constexpr double kA = 3.9, kB = 2.0;          // the water's edge (the imported water: h 0.38)
	constexpr double kBed = 0.06;                 // the dark bed
	constexpr double kShelfX = 3.2, kShelfTop = 0.26;   // the planting shelves: |x| beyond 3.2 m, 12 cm under the water
	constexpr double kDie = 0.385;                // the frieze's ground, out from the water's edge (5.5 cm under the torus)
	constexpr double kBand0 = 0.075, kBand1 = 0.2725, kBandH = kBand1 - kBand0;
	constexpr double kOuter = 0.44;               // the torus's face: the coping's width
	constexpr double kSeat = 0.42;
	constexpr double kToe = 0.425;
	constexpr int32 kBlocks = 16;
	constexpr double kReliefStep = 0.0025;        // the frieze's grid
	constexpr int32 kCopingEvery = 4;             // the coping's stations: every 4th of the frieze's (1 cm)
	constexpr double kMortar = 0.0015, kChamfer = 0.0045, kJointDepth = 0.003;
	constexpr int32 kRimLengths = 32;
	const FVector kUp(0.0, 0.0, 1.0);

	inline FVector V3(const FVector2D& P, double Z) { return FVector(P.X, P.Y, Z); }

	/** The water's edge by its parameter t (0 at the east end, increasing towards the south): point, outward normal, tangent. */
	struct FPondEdge
	{
		FVector2D P, N, T;
		double Speed = 1.0, Kappa = 0.0;
	};

	inline FPondEdge EdgeAt(double t)
	{
		FPondEdge E;
		const double C = FMath::Cos(t), S = FMath::Sin(t);
		E.P = FVector2D(kCX + kA * C, kB * S);
		const FVector2D D(-kA * S, kB * C);
		E.Speed = D.Size();
		E.T = D / E.Speed;
		E.N = FVector2D(E.T.Y, -E.T.X);
		E.Kappa = kA * kB / (E.Speed * E.Speed * E.Speed);
		return E;
	}

	/** Arc lengths round the edge and its parallels: U(t, d) = s(t) + d·θ(t), θ the tangent's turning. */
	struct FArc
	{
		static constexpr int32 N = 40000;
		TArray<double> S, Th;

		static const FArc& Get()
		{
			static const FArc Arc = []
			{
				FArc A;
				A.S.SetNumZeroed(N + 1);
				A.Th.SetNumZeroed(N + 1);
				const double Dt = 2.0 * UE_DOUBLE_PI / N;
				FPondEdge Prev = EdgeAt(0.0);
				for (int32 i = 1; i <= N; ++i)
				{
					const FPondEdge E = EdgeAt(Dt * i);
					A.S[i] = A.S[i - 1] + 0.5 * (E.Speed + Prev.Speed) * Dt;
					A.Th[i] = A.Th[i - 1] + 0.5 * (E.Speed * E.Kappa + Prev.Speed * Prev.Kappa) * Dt;
					Prev = E;
				}
				return A;
			}();
			return Arc;
		}

		double Total(double D) const { return S[N] + D * Th[N]; }

		/** U at t (any t: whole turns add the total). */
		double At(double T, double D) const
		{
			const double Turn = 2.0 * UE_DOUBLE_PI;
			const double K = FMath::FloorToDouble(T / Turn);
			const double X = (T - K * Turn) / Turn * N;
			const int32 I = FMath::Clamp(int32(X), 0, N - 1);
			const double F = X - I;
			const double U = FMath::Lerp(S[I], S[I + 1], F) + D * FMath::Lerp(Th[I], Th[I + 1], F);
			return U + K * Total(D);
		}

		/** t at U (any U). */
		double TAt(double U, double D) const
		{
			const double Tot = Total(D);
			const double K = FMath::FloorToDouble(U / Tot);
			const double R = U - K * Tot;
			int32 Lo = 0, Hi = N;
			while (Hi - Lo > 1)
			{
				const int32 Mid = (Lo + Hi) / 2;
				(S[Mid] + D * Th[Mid] <= R ? Lo : Hi) = Mid;
			}
			const double A = S[Lo] + D * Th[Lo], B = S[Hi] + D * Th[Hi];
			const double F = (R - A) / FMath::Max(1e-12, B - A);
			return (Lo + F) * 2.0 * UE_DOUBLE_PI / N + K * 2.0 * UE_DOUBLE_PI;
		}
	};

	inline double Hash01(int32 A, int32 B, int32 C)
	{
		uint32 H = uint32(A) * 0x8da6b343u ^ uint32(B) * 0xd8163841u ^ uint32(C) * 0xcb1ab31fu;
		H ^= H >> 13;
		H *= 0x5bd1e995u;
		H ^= H >> 15;
		return double(H & 0xFFFFFFu) / double(0xFFFFFFu);
	}

	// ================================================================================================ the blocks' stations

	/** One station of a block: its t, the distance from the nearest joint (at the die), which side of it (+1 after). */
	struct FStation
	{
		double T = 0.0;
		double Delta = 1.0;
		double Side = 1.0;
		bool bCoping = true;   // the coping uses it too (every joint station and every 4th of the others)
	};

	/** The inset of a joint (m) at Delta from its centre: a 3 mm mortar line 3 mm down, 45° chamfers either side. */
	inline double JointInset(double Delta) { return FMath::Clamp(kChamfer - Delta, 0.0, kJointDepth); }

	/** The stations of block K (centred on U = K · total / 16 at the die; block 0 on the east end, 8 on the west). */
	TArray<FStation> BlockStations(int32 K)
	{
		const FArc& Arc = FArc::Get();
		const double Tot = Arc.Total(kDie), Len = Tot / kBlocks;
		const double U0 = (K - 0.5) * Len, U1 = (K + 0.5) * Len;
		TArray<double> Us;
		static const double Near[] = {0.0, kMortar, 0.003, kChamfer, 0.0065};
		for (const double D : Near) { Us.Add(U0 + D); }
		const double A = U0 + 0.0065, B = U1 - 0.0065;
		const int32 Steps = FMath::Max(1, FMath::CeilToInt((B - A) / kReliefStep));
		for (int32 i = 1; i < Steps; ++i) { Us.Add(A + (B - A) * i / Steps); }
		for (int32 i = int32(UE_ARRAY_COUNT(Near)) - 1; i >= 0; --i) { Us.Add(U1 - Near[i]); }
		TArray<FStation> Out;
		const int32 Joint = int32(UE_ARRAY_COUNT(Near));
		for (int32 i = 0; i < Us.Num(); ++i)
		{
			FStation St;
			St.T = Arc.TAt(Us[i], kDie);
			const double D0 = Us[i] - U0, D1 = U1 - Us[i];
			St.Delta = FMath::Min(D0, D1);
			St.Side = D0 <= D1 ? 1.0 : -1.0;
			const bool bJointZone = i < Joint || i >= Us.Num() - Joint;
			St.bCoping = bJointZone || ((i - Joint) % kCopingEvery == 0);
			Out.Add(St);
		}
		return Out;
	}

	// ================================================================================================ the carved frieze

	/** A shape of the relief in the frieze's own plane: u along (from the east keystone), v up from the band's foot. */
	struct FShape
	{
		enum EKind : uint8 { Pad, Petal, Stem, Bump } Kind = Bump;
		double X = 0, Y = 0, A = 0, B = 0, H = 0, Ang = 0, Rot = 0, Fat = 0.7, Base = 0.55;
		bool bGroove = true;
		TArray<FVector2D> Pts;
		double W0 = 0, W1 = 0;
		FBox2D Box = FBox2D(ForceInit);
	};

	inline double Shoulder(double Inside, double R)
	{
		const double T = FMath::Clamp(Inside / R, 0.0, 1.0);
		return FMath::Sqrt(1.0 - (1.0 - T) * (1.0 - T));
	}

	inline double WrapAngle(double A) { return FMath::Abs(FMath::UnwindRadians(A)); }

	/** The relief's height (m) of one shape (the design: scratchpad pondwork/frieze2.py, the same functions). */
	double EvalShape(const FShape& S, double U, double V)
	{
		switch (S.Kind)
		{
		case FShape::Pad:
		{
			// A floating pad seen low: a broad plate carved 40–55 mm proud, its edge cut square (the undercut's shadow),
			// its rim a little rolled, its veins grooved, the slit open to the ground.
			const double X = U - S.X, Y = V - S.Y;
			const double C = FMath::Cos(S.Rot), Sn = FMath::Sin(S.Rot);
			const double RX = C * X + Sn * Y, RY = -Sn * X + C * Y;
			const double Q = FMath::Sqrt(FMath::Square(RX / S.A) + FMath::Square(RY / S.B));
			if (Q >= 1.0) { return 0.0; }
			const double Inside = (1.0 - Q) * S.B;
			const double Ang = FMath::Atan2(RY / S.B, RX / S.A);
			const double DAng = WrapAngle(Ang - S.Ang);
			const double SlitHalf = 0.16 * Q + 0.015;
			if (Q > 0.12 && DAng < SlitHalf) { return 0.0; }
			const double SlitEdge = FMath::Clamp((DAng - SlitHalf) * Q * S.B / 0.003, 0.0, 1.0);
			const double Factor = Q > 0.12 ? FMath::Min(1.0, 0.25 + SlitEdge) : 1.0;
			// (The texture audit: domed, with a 4 mm rounded shoulder, the pads read as cushions close up. A lily pad is a
			// thin leaf: the carver leaves its face flat to faintly dished, lifts its rim, cuts its edge crisp and sinks
			// its radial veins; a gentle ripple runs round it.)
			const double Dish = 0.93 + 0.07 * Q * Q;
			const double Ripple = 0.035 * Q * FMath::Sin(3.0 * Ang + 7.0 * S.Rot + S.X * 40.0);
			const double Rim = 0.09 * FMath::Exp(-FMath::Square((1.0 - Q) * S.B / 0.0045));
			const double VeinA = WrapAngle(Ang * 11.0);
			const double Veins = -0.075 * FMath::Exp(-FMath::Square(VeinA / 0.16)) * FMath::Clamp((Q - 0.1) / 0.3, 0.0, 1.0)
				* FMath::Clamp((0.97 - Q) / 0.1, 0.0, 1.0);
			const double Navel = -0.05 * FMath::Exp(-FMath::Square(Q / 0.07));
			return S.H * Shoulder(Inside, 0.0015) * (Dish + Ripple + Rim + Veins + Navel) * Factor;
		}
		case FShape::Petal:
		{
			// A pointed petal from its base along Ang, length A, width B: domed across, a groove down its middle.
			const double DX = FMath::Cos(S.Ang), DY = FMath::Sin(S.Ang);
			const double X = U - S.X, Y = V - S.Y;
			const double Along = (X * DX + Y * DY) / S.A;
			if (Along <= 0.0 || Along >= 1.0) { return 0.0; }
			const double Lat = -X * DY + Y * DX;
			const double Half = 0.5 * S.B * FMath::Pow(FMath::Sin(UE_DOUBLE_PI * FMath::Pow(Along, 0.62)), S.Fat);
			const double Inside = FMath::Min(Half - FMath::Abs(Lat), FMath::Min(Along, 1.0 - Along) * S.A * 0.5);
			if (Inside <= 0.0) { return 0.0; }
			const double Dome = FMath::Sqrt(FMath::Clamp(1.0 - FMath::Square(Lat / FMath::Max(Half, 1e-4)), 0.0, 1.0));
			const double Prof = S.Base + (1.0 - S.Base) * FMath::Sin(UE_DOUBLE_PI * FMath::Clamp(Along * 0.9 + 0.1, 0.0, 1.0));
			const double G = (S.bGroove && Along > 0.15) ? 1.0 - 0.18 * FMath::Exp(-FMath::Square(Lat / 0.0025)) : 1.0;
			return S.H * Shoulder(Inside, 0.003) * (0.6 + 0.4 * Dome) * Prof * G;
		}
		case FShape::Stem:
		{
			double Best = 1e9, TBest = 0.0;
			const int32 N = S.Pts.Num() - 1;
			for (int32 i = 0; i < N; ++i)
			{
				const FVector2D A = S.Pts[i], D = S.Pts[i + 1] - S.Pts[i];
				const double T = FMath::Clamp(((U - A.X) * D.X + (V - A.Y) * D.Y) / FMath::Max(1e-12, D.SizeSquared()), 0.0, 1.0);
				const double Dist = FVector2D::Distance(FVector2D(U, V), A + D * T);
				if (Dist < Best) { Best = Dist; TBest = (i + T) / N; }
			}
			const double W = FMath::Lerp(S.W0, S.W1, TBest);
			if (Best >= W) { return 0.0; }
			return S.H * FMath::Sqrt(1.0 - FMath::Square(Best / W));
		}
		default:
		{
			const double D = FVector2D::Distance(FVector2D(U, V), FVector2D(S.X, S.Y));
			return D >= S.A ? 0.0 : S.H * FMath::Sqrt(1.0 - FMath::Square(D / S.A));
		}
		}
	}

	/**
	 * The frieze's design (after the main session's review: large motifs, deep relief, one flowing composition): along one
	 * side from the east keystone (u 0) to the west one (u Half). Each repeat (about 1 m): a whiplash rhizome running on
	 * through the band, two overlapping pads 21–25 cm across and a third lifted on its stalk, an open lily 13–15 cm on its
	 * own stem, a bud high on the rhizome. At each end of the long axis a lily from the front, 19 cm across, with large pads
	 * and buds either side.
	 */
	struct FFrieze
	{
		static constexpr double Key = 0.36;
		static constexpr double Bin = 0.05;
		double Half = 10.8, Lambda = 1.0;
		TArray<FShape> Shapes;
		TArray<TArray<int32>> Bins;

		static double Hash1(int32 K, int32 Salt)
		{
			const double X = FMath::Sin(K * 12.9898 + Salt * 78.233) * 43758.5453;
			return X - FMath::FloorToDouble(X);
		}

		void AddPad(double X, double Y, double A, double B, double H, double Notch, double Rot)
		{
			FShape S;
			S.Kind = FShape::Pad;
			S.X = X; S.Y = Y; S.A = A; S.B = B; S.H = H; S.Ang = Notch; S.Rot = Rot;
			const double R = FMath::Max(A, B);
			S.Box = FBox2D(FVector2D(X - R, Y - R), FVector2D(X + R, Y + R));
			Shapes.Add(S);
		}

		void AddPetal(double X, double Y, double Ang, double L, double W, double H, bool bGroove = true, double Fat = 0.7, double Base = 0.55)
		{
			FShape S;
			S.Kind = FShape::Petal;
			S.X = X; S.Y = Y; S.Ang = Ang; S.A = L; S.B = W; S.H = H; S.bGroove = bGroove; S.Fat = Fat; S.Base = Base;
			const FVector2D Tip(X + L * FMath::Cos(Ang), Y + L * FMath::Sin(Ang));
			S.Box = FBox2D(FVector2D(FMath::Min(X, Tip.X) - W, FMath::Min(Y, Tip.Y) - W), FVector2D(FMath::Max(X, Tip.X) + W, FMath::Max(Y, Tip.Y) + W));
			Shapes.Add(S);
		}

		void AddBump(double X, double Y, double R, double H)
		{
			FShape S;
			S.Kind = FShape::Bump;
			S.X = X; S.Y = Y; S.A = R; S.H = H;
			S.Box = FBox2D(FVector2D(X - R, Y - R), FVector2D(X + R, Y + R));
			Shapes.Add(S);
		}

		void AddStem(const FVector2D& P0, const FVector2D& P1, const FVector2D& P2, const FVector2D& P3, double W0, double W1, double H)
		{
			FShape S;
			S.Kind = FShape::Stem;
			S.W0 = W0; S.W1 = W1; S.H = H;
			constexpr int32 N = 24;
			for (int32 i = 0; i <= N; ++i)
			{
				const double T = double(i) / N, M = 1.0 - T;
				const FVector2D P = P0 * (M * M * M) + P1 * (3.0 * M * M * T) + P2 * (3.0 * M * T * T) + P3 * (T * T * T);
				S.Pts.Add(P);
				S.Box += P;
			}
			S.Box = S.Box.ExpandBy(FMath::Max(W0, W1));
			Shapes.Add(S);
		}

		/** An open Nymphaea from the side: a back row of petals, the front row fanning over it, two sepals, the receptacle. */
		void FlowerSide(double CX, double CY, double Size, double H, double Lean, int32 K, bool bHalfOpen)
		{
			const TArray<double> Back = bHalfOpen ? TArray<double>{-30, -10, 10, 30} : TArray<double>{-58, -34, -11, 11, 34, 58};
			for (const double A : Back)
			{
				AddPetal(CX, CY + 0.006, FMath::DegreesToRadians(90.0 + A + Lean), Size * 0.92, Size * 0.36, H * 0.62, false, 0.6);
			}
			const TArray<double> Front = bHalfOpen ? TArray<double>{-40, -20, 0, 20, 40} : TArray<double>{-70, -46, -23, 0, 23, 46, 70};
			for (int32 i = 0; i < Front.Num(); ++i)
			{
				const double A = Front[i];
				const double F = 1.0 - FMath::Abs(A) / 120.0;
				const double L = Size * (0.8 + 0.2 * FMath::Cos(FMath::DegreesToRadians(A))) * (0.96 + 0.08 * Hash1(K * 7 + i, 3));
				AddPetal(CX, CY, FMath::DegreesToRadians(90.0 + A + Lean), L, Size * (bHalfOpen ? 0.40 : 0.44), H * (0.78 + 0.22 * F), true, 0.62);
			}
			for (const double A : {-116.0, 116.0})
			{
				AddPetal(CX, CY - 0.006, FMath::DegreesToRadians(90.0 + A + Lean), Size * 0.66, Size * 0.32, H * 0.7);
			}
			AddBump(CX, CY - 0.008, Size * 0.22, H * 0.72);
		}

		void Bud(double X, double Y, double Ang, double L, double W, double H)
		{
			AddPetal(X, Y, Ang, L, W, H, false, 0.5, 0.75);
			for (const double Off : {-0.2, 0.2}) { AddPetal(X, Y, Ang + Off, L * 0.7, W * 0.5, H * 1.06, false); }
		}

		/** The lily from the front at an end of the long axis: three rings of petals, cupped, the stamen crown; pads and buds. */
		void Keystone(double CX, bool bMirror)
		{
			const double CY = 0.1;
			for (const int32 Side : {-1, 1})
			{
				if ((!bMirror && Side < 0) || (bMirror && Side > 0)) { continue; }
				AddPad(CX + Side * 0.2, 0.058, 0.125, 0.046, 0.047, -UE_DOUBLE_PI / 2 + Side * 0.6, Side * 0.06);
				AddStem(FVector2D(CX + Side * 0.07, 0.03), FVector2D(CX + Side * 0.13, 0.05), FVector2D(CX + Side * 0.12, 0.13),
						FVector2D(CX + Side * 0.17, 0.155), 0.0065, 0.0055, 0.028);
				Bud(CX + Side * 0.17, 0.155, FMath::DegreesToRadians(90.0 - Side * 30.0), 0.06, 0.024, 0.04);
			}
			for (int32 i = 0; i < 12; ++i) { AddPetal(CX, CY, 2.0 * UE_DOUBLE_PI * (i + 0.5) / 12, 0.094, 0.042, 0.03, true, 0.55, 0.8); }
			for (int32 i = 0; i < 10; ++i) { AddPetal(CX, CY, 2.0 * UE_DOUBLE_PI * i / 10, 0.072, 0.036, 0.042, true, 0.55, 0.85); }
			for (int32 i = 0; i < 8; ++i) { AddPetal(CX, CY, 2.0 * UE_DOUBLE_PI * (i + 0.5) / 8, 0.048, 0.03, 0.05, true, 0.55, 0.9); }
			AddBump(CX, CY, 0.024, 0.05);
			for (int32 i = 0; i < 16; ++i)
			{
				const double A = 2.0 * UE_DOUBLE_PI * i / 16;
				AddBump(CX + 0.018 * FMath::Cos(A), CY + 0.018 * FMath::Sin(A), 0.006, 0.056);
			}
			for (int32 i = 0; i < 6; ++i)
			{
				const double A = 2.0 * UE_DOUBLE_PI * i / 6 + 0.5;
				AddBump(CX + 0.007 * FMath::Cos(A), CY + 0.007 * FMath::Sin(A), 0.0045, 0.056);
			}
		}

		void Repeat(double X0, int32 K)
		{
			auto J = [K](int32 S) { return Hash1(K, S) - 0.5; };
			auto P = [X0](double X, double Y) { return FVector2D(X0 + X, Y); };
			const double Pi = UE_DOUBLE_PI, L = Lambda;
			const bool bHalfOpen = (K % 3) == 2;
			// The rhizome: two cubics, tangent-continuous across the joins, low at the repeat's ends, high in its middle.
			AddStem(P(0.0, 0.03), P(0.25, 0.0), P(0.3, 0.15), P(0.5, 0.15), 0.0085, 0.0085, 0.02);
			AddStem(P(0.5, 0.15), P(0.7, 0.15), P(L - 0.25, 0.06), P(L, 0.03), 0.0085, 0.0085, 0.02);
			// A bud high on the rhizome's rise, leaning back.
			AddStem(P(0.36, 0.14), P(0.33, 0.155), P(0.3, 0.165), P(0.27, 0.17), 0.0062, 0.0055, 0.03);
			Bud(X0 + 0.27, 0.17, FMath::DegreesToRadians(150.0 + 8.0 * J(12)), 0.05, 0.021, 0.04);
			// The pads: a big one, a second overlapping it in front, a third lifted and tilted on its stalk.
			AddPad(X0 + 0.15 + 0.02 * J(1), 0.062, 0.125, 0.047, 0.046, -Pi / 2 + 0.5 + 0.3 * J(2), 0.04 * J(3));
			AddPad(X0 + 0.36 + 0.02 * J(4), 0.046, 0.108, 0.041, 0.055, -Pi / 2 - 0.5 + 0.3 * J(5), -0.05);
			AddStem(P(0.78, 0.04), P(0.8, 0.06), P(0.82, 0.08), P(0.84, 0.095), 0.0065, 0.006, 0.03);
			AddPad(X0 + 0.87 + 0.02 * J(6), 0.1, 0.1, 0.042, 0.044, -Pi / 2 + 0.8, 0.28 + 0.06 * J(11));
			// The open lily on its own stem, rising from behind the pads.
			const double FX = 0.6 + 0.02 * J(7), FY = 0.092;
			AddStem(P(0.5, 0.004), P(0.57, 0.02), P(0.555, 0.06), P(FX, FY - 0.01), 0.0072, 0.0065, 0.032);
			FlowerSide(X0 + FX, FY, bHalfOpen ? 0.074 : 0.084, 0.052, -5.0 + 6.0 * J(8), K, bHalfOpen);
		}

		void Build(double InHalf)
		{
			Half = InHalf;
			Shapes.Reset();
			Keystone(0.0, false);
			Keystone(Half, true);
			const double Run = Half - 2.0 * Key;
			const int32 N = FMath::Max(1, FMath::RoundToInt(Run / 1.0));
			Lambda = Run / N;
			for (int32 K = 0; K < N; ++K) { Repeat(Key + K * Lambda, K); }
			const int32 NB = FMath::CeilToInt((Half + 0.6) / Bin) + 1;
			Bins.SetNum(NB);
			for (int32 i = 0; i < Shapes.Num(); ++i)
			{
				const int32 B0 = FMath::Clamp(FMath::FloorToInt((Shapes[i].Box.Min.X + 0.3) / Bin), 0, NB - 1);
				const int32 B1 = FMath::Clamp(FMath::FloorToInt((Shapes[i].Box.Max.X + 0.3) / Bin), 0, NB - 1);
				for (int32 b = B0; b <= B1; ++b) { Bins[b].Add(i); }
			}
		}

		/** The relief's height (m) at u (0 … Half) and v (0 … the band's height). */
		double Height(double U, double V) const
		{
			double H = 0.0;
			const int32 B = FMath::Clamp(FMath::FloorToInt((U + 0.3) / Bin), 0, Bins.Num() - 1);
			for (const int32 i : Bins[B])
			{
				const FShape& S = Shapes[i];
				if (U < S.Box.Min.X || U > S.Box.Max.X || V < S.Box.Min.Y || V > S.Box.Max.Y) { continue; }
				H = FMath::Max(H, EvalShape(S, U, V));
			}
			return H * FMath::Clamp(FMath::Min(V, kBandH - V) / 0.004, 0.0, 1.0);
		}
	};

	// ================================================================================================ the coping's profile

	/** A profile in (d out from the water's edge, z up), traced with the stone on the right (its normals face out of it). */
	struct FPondProfile
	{
		TArray<FVector2D> P;
		TArray<bool> Smooth;
		TArray<FVector2D> Inset;   // per point: the direction a joint insets it (mitred, so both faces meet)
		TArray<double> Len;        // cumulative length (the UV's v)
		bool bPinFirst = false, bPinLast = false;   // ends shared with the frieze: inset horizontally, no lippage

		FPondProfile& Add(double D, double Z, bool bSmooth = false) { P.Add(FVector2D(D, Z)); Smooth.Add(bSmooth); return *this; }

		FPondProfile& Arc(double CD, double CZ, double R, double A0, double A1, int32 Segs)
		{
			for (int32 i = 1; i <= Segs; ++i)
			{
				const double A = FMath::DegreesToRadians(A0 + (A1 - A0) * i / Segs);
				Add(CD + R * FMath::Cos(A), CZ + R * FMath::Sin(A), i < Segs);
			}
			return *this;
		}

		FVector2D SegNormal(int32 j) const
		{
			const FVector2D D = P[j + 1] - P[j];
			return FVector2D(-D.Y, D.X).GetSafeNormal();
		}

		void Finish(double VStart, bool bFirstHorizontal, bool bLastHorizontal)
		{
			const int32 N = P.Num();
			Inset.SetNum(N);
			Len.SetNum(N);
			Len[0] = VStart;
			for (int32 i = 1; i < N; ++i) { Len[i] = Len[i - 1] + FVector2D::Distance(P[i - 1], P[i]); }
			for (int32 i = 0; i < N; ++i)
			{
				if (i == 0) { Inset[i] = SegNormal(0); }
				else if (i == N - 1) { Inset[i] = SegNormal(N - 2); }
				else
				{
					const FVector2D A = SegNormal(i - 1), B = SegNormal(i);
					Inset[i] = (A + B) / FMath::Max(0.3, 1.0 + FVector2D::DotProduct(A, B));
				}
			}
			if (bFirstHorizontal) { Inset[0] = FVector2D(1.0, 0.0); }
			if (bLastHorizontal) { Inset[N - 1] = FVector2D(1.0, 0.0); }
			bPinFirst = bFirstHorizontal;
			bPinLast = bLastHorizontal;
		}
	};

	/** The coping above the frieze: the stone behind the lip, the seat, the torus, the fillet and the cavetto. */
	FPondProfile UpperProfile()
	{
		FPondProfile F;
		F.Add(0.000, 0.300).Add(0.000, 0.398);
		F.Arc(0.012, 0.398, 0.012, 180.0, 90.0, 4);                 // the inner arris, eased (under the lip's bead)
		F.Add(0.050, 0.4194, true).Add(0.120, 0.4200, true);         // a wash to the seat
		F.Add(0.385, 0.4200);
		F.Arc(0.385, 0.365, 0.055, 90.0, -60.0, 20);                  // the torus
		F.Add(0.4125, 0.300);                                         // the fillet
		F.Arc(0.4125, 0.2725, 0.0275, 90.0, 180.0, 8);                // the cavetto, down to the die
		F.P.Last() = FVector2D(kDie, kBand1);
		F.Finish(0.0, false, true);
		return F;
	}

	/** Below the frieze: a small ledge, an ovolo, the plinth, the toe recess at the floor. */
	FPondProfile LowerProfile(double VStart)
	{
		FPondProfile F;
		F.Add(kDie, kBand0).Add(0.425, kBand0);                       // the band's foot: a ledge 4 cm out (it frames the band)
		F.Arc(0.425, 0.060, 0.015, 90.0, 0.0, 6);
		F.Add(0.440, 0.018).Add(kToe, 0.018).Add(kToe, -0.002);
		F.Finish(VStart, true, false);
		return F;
	}

	/** Per block: its slice of the travertine, its tone and its polish. */
	struct FBlockLook
	{
		FVector2D Slice;
		uint8 Tone = 128, Polish = 128;
		double Lippage = 0.0;   // a fraction of a millimetre proud or shy of its neighbours
	};

	FBlockLook LookOf(int32 K)
	{
		FBlockLook L;
		L.Slice = FVector2D(1.7 * Hash01(K, 31, 1), 1.1 * Hash01(K, 31, 2));
		L.Tone = uint8(FMath::RoundToInt(255.0 * Hash01(K, 31, 3)));
		L.Polish = uint8(FMath::RoundToInt(255.0 * Hash01(K, 31, 4)));
		L.Lippage = 0.0005 * (Hash01(K, 31, 5) - 0.5);
		return L;
	}

	FColor StoneColour(const FBlockLook& L, double Delta)
	{
		if (Delta < kMortar) { return FColor(128, 128, 255, 255); }
		const double B = Delta < kChamfer ? 0.55 * (kChamfer - Delta) / (kChamfer - kMortar) : 0.0;
		return FColor(L.Tone, L.Polish, uint8(FMath::RoundToInt(255.0 * B)), 255);
	}

	/** Sweeps a profile through a block's stations, with the joints' insets and chamfers at its ends. */
	void SweepBlock(FMeshData& M, const FPondProfile& Prof, const TArray<FStation>& Sts, const FBlockLook& Look)
	{
		const FArc& Arc = FArc::Get();
		const int32 NP = Prof.P.Num();
		for (int32 j = 0; j + 1 < NP; ++j)
		{
			const FVector2D SegN = Prof.SegNormal(j);
			auto PointNormal = [&](int32 i)
			{
				if (!Prof.Smooth[i]) { return SegN; }
				const int32 Other = i == j ? j - 1 : j + 1;
				if (Other < 0 || Other + 1 >= NP) { return SegN; }
				return (SegN + Prof.SegNormal(Other)).GetSafeNormal();
			};
			const int32 Base = M.Positions.Num();
			int32 Count = 0;
			for (const FStation& St : Sts)
			{
				if (!St.bCoping) { continue; }
				const FPondEdge E = EdgeAt(St.T);
				const double J = JointInset(St.Delta);
				const double Lip = St.Delta < kChamfer ? 0.0 : Look.Lippage;
				const bool bChamfer = St.Delta > kMortar && St.Delta < kChamfer;
				for (const int32 i : {j, j + 1})
				{
					const bool bPinned = (i == 0 && Prof.bPinFirst) || (i == NP - 1 && Prof.bPinLast);
					const FVector2D Q = Prof.P[i] - Prof.Inset[i] * (J - (bPinned ? 0.0 : Lip));
					const FVector2D N2 = PointNormal(i);
					const FVector P3 = V3(E.P + E.N * Q.X, Q.Y);
					FVector N3 = FVector(E.N.X * N2.X, E.N.Y * N2.X, N2.Y);
					if (bChamfer)
					{
						const double R = (1.0 + Q.X * E.Kappa) / (1.0 + kDie * E.Kappa);
						N3 = (N3 - FVector(E.T.X, E.T.Y, 0.0) * (St.Side / R)).GetSafeNormal();
					}
					const FVector2D UV(Arc.At(St.T, Prof.P[i].X) + Look.Slice.X, Prof.Len[i] + Look.Slice.Y);
					M.Vertex(P3, N3, UV);
					M.Colors.SetNum(M.Positions.Num() - 1);
					M.Colors.Add(StoneColour(Look, St.Delta));
				}
				++Count;
			}
			for (int32 k = 0; k + 1 < Count; ++k)
			{
				const int32 A = Base + 2 * k;
				M.Quad(A, A + 2, A + 3, A + 1);
			}
		}
	}

	/** A closed profile swept once round without joints (the lip, the lining, the hit block), stations every Step at the edge. */
	void SweepLoop(FMeshData& M, const FPondProfile& Prof, double Step, TFunctionRef<FVector2D(const FVector&, int32)> UV)
	{
		const FArc& Arc = FArc::Get();
		const double Tot = Arc.Total(0.0);
		const int32 NS = FMath::CeilToInt(Tot / Step);
		const int32 NP = Prof.P.Num();
		for (int32 j = 0; j + 1 < NP; ++j)
		{
			const FVector2D SegN = Prof.SegNormal(j);
			const int32 Base = M.Positions.Num();
			for (int32 s = 0; s <= NS; ++s)
			{
				const FPondEdge E = EdgeAt(2.0 * UE_DOUBLE_PI * s / NS);
				for (const int32 i : {j, j + 1})
				{
					FVector2D N2 = SegN;
					if (Prof.Smooth[i])
					{
						const int32 Other = i == j ? j - 1 : j + 1;
						if (Other >= 0 && Other + 1 < NP) { N2 = (SegN + Prof.SegNormal(Other)).GetSafeNormal(); }
					}
					const FVector P3 = V3(E.P + E.N * Prof.P[i].X, Prof.P[i].Y);
					M.Vertex(P3, FVector(E.N.X * N2.X, E.N.Y * N2.X, N2.Y), UV(P3, s));
				}
			}
			for (int32 k = 0; k < NS; ++k)
			{
				const int32 A = Base + 2 * k;
				M.Quad(A, A + 2, A + 3, A + 1);
			}
		}
	}

	/** A flat fan over the region inside the parallel at D (plan), at height Z, facing N. */
	void Disc(FMeshData& M, double D, double Z, const FVector& N, int32 Segs)
	{
		const int32 Hub = M.Vertex(FVector(kCX, 0.0, Z), N, FVector2D(kCX, 0.0));
		for (int32 i = 0; i < Segs; ++i)
		{
			const FPondEdge E = EdgeAt(2.0 * UE_DOUBLE_PI * i / Segs);
			const FVector2D P = E.P + E.N * D;
			M.Vertex(V3(P, Z), N, FVector2D(P.X, P.Y));
		}
		for (int32 i = 0; i < Segs; ++i) { M.Tri(Hub, Hub + 1 + i, Hub + 1 + (i + 1) % Segs); }
	}
}

// ==================================================================== ASalonPond

ASalonPond::ASalonPond()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);   // it rises with the pond
	auto Make = [this](const TCHAR* Name)
	{
		UProceduralMeshComponent* M = CreateDefaultSubobject<UProceduralMeshComponent>(Name);
		M->SetupAttachment(RootComponent);
		M->SetMobility(EComponentMobility::Movable);
		M->bUseAsyncCooking = true;
		M->bUseComplexAsSimpleCollision = true;
		return M;
	};
	Basin = Make(TEXT("Basin"));
	Basin->SetCollisionProfileName(TEXT("BlockAll"));
	Basin->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	RimLight = Make(TEXT("RimLight"));
	RimLight->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RimLight->SetCastShadow(false);
	MuseeBake::NoBake(RimLight);
	Guard = Make(TEXT("Guard"));
	Guard->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Guard->SetCollisionObjectType(ECC_WorldStatic);
	Guard->SetCollisionResponseToAllChannels(ECR_Ignore);
	Guard->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Guard->SetHiddenInGame(true);
	Guard->SetVisibility(false);
	Guard->SetCastShadow(false);
	MuseeBake::NoBake(Guard);
	auto Soft = [](const TCHAR* Path) { return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(Path)); };
	Materials = {
		Soft(TEXT("/Game/Museum/Materials/Salon/MI_Salon_Ashlar.MI_Salon_Ashlar")),       // honed travertine (S1): slice, tone, polish, joints
		Soft(TEXT("/Game/Museum/Materials/M_SquareFloorDark.M_SquareFloorDark")),
		Soft(TEXT("/Game/Museum/Materials/Salon/MI_Salon_PondTray.MI_Salon_PondTray")),   // the tray: brushed bronze, dulled
		Soft(TEXT("/Game/Museum/Materials/M_Gilt_Aged.M_Gilt_Aged")),
		Soft(TEXT("/Game/Museum/Materials/Salon/MI_Salon_AshlarCarved.MI_Salon_AshlarCarved"))};   // the frieze: the carver's close-grained stone
	RimLightMaterial = Soft(TEXT("/Game/Museum/Materials/Salon/MI_Salon_RimLight.MI_Salon_RimLight"));
	Tags.AddUnique(FName(TEXT("musee.building")));
}

void ASalonPond::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
	ApplyMaterials();
}

void ASalonPond::BeginPlay()
{
	Super::BeginPlay();
	Build();
	ApplyMaterials();
	SetRimReveal(0.f);
}

bool ASalonPond::IsOver(const FVector2D& Plan, double Margin)
{
	using namespace SalonPondBuild;
	// The parallel at d lies outside the ellipse (a + d, b + d) by a few centimetres at most: grow it a little more.
	const double A = kA + kOuter + Margin + 0.04, B = kB + kOuter + Margin + 0.04;
	return FMath::Square((Plan.X - kCX) / A) + FMath::Square(Plan.Y / B) < 1.0;
}

FVector2D ASalonPond::PushOut(const FVector2D& Plan, double Margin)
{
	using namespace SalonPondBuild;
	const double A = kA + kOuter + Margin + 0.04, B = kB + kOuter + Margin + 0.04;
	FVector2D R(Plan.X - kCX, Plan.Y);
	if (R.IsNearlyZero()) { R = FVector2D(0.0, 1.0); }
	const double Q = FMath::Sqrt(FMath::Square(R.X / A) + FMath::Square(R.Y / B));
	return FVector2D(kCX, 0.0) + R / FMath::Max(Q, 1e-6) * 1.001;
}

void ASalonPond::SetRimReveal(float Fraction)
{
	if (!RimLight) { return; }
	const int32 Lit = FMath::Clamp(FMath::FloorToInt(Fraction * SalonPondBuild::kRimLengths + 1e-3f), 0, SalonPondBuild::kRimLengths);
	for (int32 k = 0; k < RimLight->GetNumSections(); ++k) { RimLight->SetMeshSectionVisible(k, k < Lit); }
}

void ASalonPond::Build()
{
	using namespace SalonPondBuild;
	// The rim's light and the guard stay procedural (never baked): build them whatever.
	{
		// The line of light: 32 lengths in a slot of the lip's bronze band, just over the water, from the west end (y +0.5)
		// on northwards (as the imported rim lights ran).
		const FArc& Arc = FArc::Get();
		const double Tot = Arc.Total(0.0);
		const double T0 = UE_DOUBLE_PI - FMath::Asin(0.25);
		const double U0 = Arc.At(T0, 0.0);
		RimLight->ClearAllMeshSections();
		for (int32 k = 0; k < kRimLengths; ++k)
		{
			FMeshData L;
			const double A = U0 + Tot * k / kRimLengths + 0.004, B = U0 + Tot * (k + 1) / kRimLengths - 0.004;
			constexpr int32 N = 24;
			for (int32 i = 0; i <= N; ++i)
			{
				const FPondEdge E = EdgeAt(Arc.TAt(FMath::Lerp(A, B, double(i) / N), 0.0));
				const FVector N3(-E.N.X, -E.N.Y, 0.0);
				const FVector2D P = E.P - E.N * 0.0058;
				L.Vertex(V3(P, 0.3845), N3, FVector2D(double(i) / N, 0.0));
				L.Vertex(V3(P, 0.3905), N3, FVector2D(double(i) / N, 1.0));
			}
			for (int32 i = 0; i < N; ++i) { L.Quad(2 * i, 2 * i + 2, 2 * i + 3, 2 * i + 1); }
			L.Write(RimLight, k, false);
		}
		// The guard: a thin band round the water's edge from the lip up to 1.9 m (the visitor only).
		FMeshData G;
		FPondProfile GP;
		GP.Add(-0.012, 0.40).Add(-0.012, 1.90).Add(0.0, 1.90).Add(0.0, 0.40).Add(-0.012, 0.40);
		GP.Finish(0.0, false, false);
		SweepLoop(G, GP, 0.05, [](const FVector& P, int32) { return FVector2D(P.X, P.Z); });
		Guard->ClearAllMeshSections();
		G.Write(Guard, 0, true);
		Guard->SetMeshSectionVisible(0, false);
	}
	if (MuseeBake::IsBaked(this)) { return; }

	FMeshData Coping, Dark, Tray, Lip, Relief, Hit;
	const FArc& Arc = FArc::Get();
	const double TotDie = Arc.Total(kDie);

	// ---------------------------------------------------------------- the coping, block by block
	const FPondProfile Upper = UpperProfile();
	const FPondProfile Lower = LowerProfile(Upper.Len.Last() + kBandH);
	TArray<TArray<FStation>> Blocks;
	for (int32 K = 0; K < kBlocks; ++K)
	{
		Blocks.Add(BlockStations(K));
		const FBlockLook Look = LookOf(K);
		SweepBlock(Coping, Upper, Blocks.Last(), Look);
		SweepBlock(Coping, Lower, Blocks.Last(), Look);
	}

	// ---------------------------------------------------------------- the frieze: one height field round the die
	{
		FFrieze Frieze;
		Frieze.Build(0.5 * TotDie);
		const int32 NR = FMath::RoundToInt(kBandH / kReliefStep);
		// Every station round the loop once (each block's last station is the next one's first).
		struct FRef { int32 Block, Index; };
		TArray<FRef> Ring;
		for (int32 K = 0; K < kBlocks; ++K)
		{
			for (int32 i = 0; i + 1 < Blocks[K].Num(); ++i) { Ring.Add({K, i}); }
		}
		const int32 NS = Ring.Num();
		TArray<FVector> Pos;
		TArray<float> Hgt;
		Pos.SetNumUninitialized(NS * (NR + 1));
		Hgt.SetNumUninitialized(NS * (NR + 1));
		ParallelFor(NS, [&](int32 s)
		{
			const FStation& St = Blocks[Ring[s].Block][Ring[s].Index];
			const FPondEdge E = EdgeAt(St.T);
			double U = Arc.At(St.T, kDie);
			U = FMath::Fmod(U + 4.0 * TotDie, TotDie);
			const double UF = FMath::Min(U, TotDie - U);
			const FBlockLook Look = LookOf(Ring[s].Block);
			for (int32 r = 0; r <= NR; ++r)
			{
				const double V = kBandH * r / NR;
				const double H = Frieze.Height(UF, V);
				double Off = H + (St.Delta < kChamfer ? 0.0 : Look.Lippage * (r > 0 && r < NR ? 1.0 : 0.0));
				if (St.Delta < kMortar) { Off = -kJointDepth; }
				else if (St.Delta < kChamfer) { Off = FMath::Lerp(-kJointDepth, H, (St.Delta - kMortar) / (kChamfer - kMortar)); }
				Pos[s * (NR + 1) + r] = V3(E.P + E.N * (kDie + Off), kBand0 + V);
				Hgt[s * (NR + 1) + r] = float(H);
			}
		});
		// The undercut: where a motif's edge is cut square, its top arris is carried out over the ground by up to 3 mm
		// (the carver's undercut: a shadow line under every pad and petal). Not at the joints, not on the band's edges.
		{
			TArray<FVector> Shift;
			Shift.Init(FVector::ZeroVector, Pos.Num());
			ParallelFor(NS, [&](int32 s)
			{
				const FStation& St = Blocks[Ring[s].Block][Ring[s].Index];
				if (St.Delta < kChamfer + 0.004) { return; }
				const int32 S0 = (s + NS - 1) % NS, S1 = (s + 1) % NS;
				const FPondEdge E = EdgeAt(St.T);
				const FVector Along(E.T.X, E.T.Y, 0.0);
				const double DU = FMath::Max(1e-4, (Pos[S1 * (NR + 1)] - Pos[S0 * (NR + 1)]).Size() * 0.5);
				for (int32 r = 2; r <= NR - 2; ++r)
				{
					const int32 G = s * (NR + 1) + r;
					const double GU = (Hgt[S1 * (NR + 1) + r] - Hgt[S0 * (NR + 1) + r]) / (2.0 * DU);
					const double GV = (Hgt[G + 1] - Hgt[G - 1]) / (2.0 * kBandH / NR);
					const double Slope = FMath::Sqrt(GU * GU + GV * GV);
					const double Rim = FMath::Clamp((Slope - 1.5) / 3.0, 0.0, 1.0) * FMath::Clamp(Hgt[G] / 0.03, 0.0, 1.0);
					if (Rim <= 0.0) { continue; }
					Shift[G] = -(Along * GU + FVector(0, 0, 1) * GV) / Slope * (0.003 * Rim);
				}
			});
			for (int32 i = 0; i < Pos.Num(); ++i) { Pos[i] += Shift[i]; }
		}
		// The top and bottom rows meet the coping exactly (no lippage there, the inset horizontal).
		// Normals from the ring's neighbours (so they are welded across the joints and the blocks).
		TArray<FVector> Nrm;
		Nrm.SetNumUninitialized(Pos.Num());
		ParallelFor(NS, [&](int32 s)
		{
			const int32 S0 = (s + NS - 1) % NS, S1 = (s + 1) % NS;
			for (int32 r = 0; r <= NR; ++r)
			{
				const FVector DS = Pos[S1 * (NR + 1) + r] - Pos[S0 * (NR + 1) + r];
				const FVector DR = Pos[s * (NR + 1) + FMath::Min(r + 1, NR)] - Pos[s * (NR + 1) + FMath::Max(r - 1, 0)];
				Nrm[s * (NR + 1) + r] = FVector::CrossProduct(DS, DR).GetSafeNormal(1e-20, FVector(1, 0, 0));
			}
		});
		// Dirt in the carving's hollows: where the ground is low beside a raised motif (within 5 mm).
		TArray<uint8> Dirt;
		Dirt.SetNumZeroed(Pos.Num());
		ParallelFor(NS, [&](int32 s)
		{
			for (int32 r = 0; r <= NR; ++r)
			{
				float Max = 0.f;
				for (int32 ds = -3; ds <= 3; ++ds)
				{
					const int32 S2 = (s + ds + NS) % NS;
					for (int32 dr = -3; dr <= 3; ++dr)
					{
						const int32 R2 = FMath::Clamp(r + dr, 0, NR);
						Max = FMath::Max(Max, Hgt[S2 * (NR + 1) + R2]);
					}
				}
				const double D = FMath::Clamp((Max - Hgt[s * (NR + 1) + r]) / 0.02, 0.0, 1.0);
				Dirt[s * (NR + 1) + r] = uint8(FMath::RoundToInt(255.0 * 0.45 * D));
			}
		});
		// Each block's own vertices (its slice and tone), the joint stations shared in position.
		const double VTop = Upper.Len.Last();
		for (int32 K = 0; K < kBlocks; ++K)
		{
			const FBlockLook Look = LookOf(K);
			const TArray<FStation>& Sts = Blocks[K];
			const int32 First = [&] { int32 F = 0; for (int32 k = 0; k < K; ++k) { F += Blocks[k].Num() - 1; } return F; }();
			const int32 Base = Relief.Positions.Num();
			for (int32 i = 0; i < Sts.Num(); ++i)
			{
				const int32 s = (First + i) % NS;
				const double U = Arc.At(Sts[i].T, kDie) + Look.Slice.X;
				for (int32 r = 0; r <= NR; ++r)
				{
					const int32 G = s * (NR + 1) + r;
					Relief.Vertex(Pos[G], Nrm[G], FVector2D(U, VTop + kBandH * (1.0 - double(r) / NR) + Look.Slice.Y));
					FColor C = StoneColour(Look, Sts[i].Delta);
					C.B = FMath::Max(C.B, Dirt[G]);
					Relief.Colors.SetNum(Relief.Positions.Num() - 1);
					Relief.Colors.Add(C);
				}
			}
			for (int32 i = 0; i + 1 < Sts.Num(); ++i)
			{
				for (int32 r = 0; r < NR; ++r)
				{
					const int32 A = Base + i * (NR + 1) + r, B = Base + (i + 1) * (NR + 1) + r;
					Relief.Quad(A, B, B + 1, A + 1);
				}
			}
		}
	}

	// ---------------------------------------------------------------- the gilt-bronze lip and the line of light's slot
	{
		FPondProfile LP;
		LP.Add(-0.005, 0.355).Add(-0.005, 0.403);
		LP.Arc(0.004, 0.403, 0.009, 180.0, 0.0, 10);
		LP.Add(0.013, 0.394);
		LP.Finish(0.0, false, false);
		SweepLoop(Lip, LP, 0.01, [](const FVector& P, int32) { return FVector2D(P.X * 0.7 + P.Y * 0.7, P.Z); });
	}

	// ---------------------------------------------------------------- the lining, the bed, the planting shelves
	{
		FPondProfile LN;
		LN.Add(-0.003, kBed - 0.002).Add(-0.003, 0.358);
		LN.Finish(0.0, false, false);
		SweepLoop(Dark, LN, 0.02, [](const FVector& P, int32) { return FVector2D(P.X + P.Y, P.Z); });
		Disc(Dark, -0.003, kBed, kUp, 256);
		for (const double SX : {-1.0, 1.0})
		{
			// The shelf's top: the ellipse beyond |x| = kShelfX (a fan from the chord's middle), and its riser.
			const double T0 = FMath::Acos(kShelfX / (kA - 0.003));
			const double YC = (kB - 0.003) * FMath::Sin(T0);
			const FVector Mid(kCX + SX * kShelfX, 0.0, kShelfTop);
			const int32 Hub = Dark.Vertex(Mid, kUp, FVector2D(Mid.X, Mid.Y));
			constexpr int32 N = 48;
			for (int32 i = 0; i <= N; ++i)
			{
				const double T = -T0 + 2.0 * T0 * i / N;
				const FVector P(kCX + SX * (kA - 0.003) * FMath::Cos(T), (kB - 0.003) * FMath::Sin(T), kShelfTop);
				Dark.Vertex(P, kUp, FVector2D(P.X, P.Y));
			}
			const int32 C0 = Dark.Vertex(FVector(kCX + SX * kShelfX, -YC, kShelfTop), kUp, FVector2D(kCX + SX * kShelfX, -YC));
			const int32 C1 = Dark.Vertex(FVector(kCX + SX * kShelfX, YC, kShelfTop), kUp, FVector2D(kCX + SX * kShelfX, YC));
			for (int32 i = 0; i < N; ++i) { Dark.Tri(Hub, Hub + 1 + i, Hub + 2 + i); }
			Dark.Tri(Hub, C0, Hub + 1);
			Dark.Tri(Hub, Hub + 1 + N, C1);
			const FVector RN(-SX, 0.0, 0.0);
			Dark.Rect(FVector(kCX + SX * kShelfX, -YC, kBed), FVector(kCX + SX * kShelfX, YC, kBed), FVector(kCX + SX * kShelfX, YC, kShelfTop),
					  FVector(kCX + SX * kShelfX, -YC, kShelfTop), RN);
		}
	}

	// ---------------------------------------------------------------- the tray under it all (bronze), to the toe
	Disc(Tray, kToe, -0.002, -kUp, 512);

	// ---------------------------------------------------------------- the hit block: to sit and stand on, no ledge
	{
		FPondProfile HP;
		HP.Add(0.0, -0.002).Add(0.0, kSeat).Add(kOuter, kSeat).Add(kOuter, -0.002).Add(0.0, -0.002);
		HP.Finish(0.0, false, false);
		SweepLoop(Hit, HP, 0.03, [](const FVector& P, int32) { return FVector2D(P.X, P.Y); });
	}

	Basin->ClearAllMeshSections();
	Coping.Write(Basin, 0, false);
	Dark.Write(Basin, 1, false);
	Tray.Write(Basin, 2, false);
	Lip.Write(Basin, 3, false);
	Relief.Write(Basin, 4, false);
	Hit.Write(Basin, 5, true);
	Basin->SetMeshSectionVisible(5, false);
}

void ASalonPond::ApplyMaterials()
{
	const TArray<const TCHAR*> Fallbacks = {TEXT("/Game/Museum/Materials/M_Travertine_Honed.M_Travertine_Honed"),
		TEXT("/Game/Museum/Materials/M_SquareFloorDark.M_SquareFloorDark"), TEXT("/Game/Museum/Materials/M_Bronze_Brushed.M_Bronze_Brushed"),
		TEXT("/Game/Museum/Materials/M_Gilt.M_Gilt"), TEXT("/Game/Museum/Materials/Salon/MI_Salon_Ashlar.MI_Salon_Ashlar")};
	for (int32 s = 0; s < Materials.Num(); ++s)
	{
		UMaterialInterface* M = Materials[s].IsNull() ? nullptr : Materials[s].LoadSynchronous();
		if (!M && Fallbacks.IsValidIndex(s)) { M = Cast<UMaterialInterface>(FSoftObjectPath(Fallbacks[s]).TryLoad()); }
		if (M && Basin) { Basin->SetMaterial(s, M); }
	}
	UMaterialInterface* Glow = RimLightMaterial.IsNull() ? nullptr : RimLightMaterial.LoadSynchronous();
	if (!Glow) { Glow = Cast<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Museum/Nature/MI_Petal_GoldenLily.MI_Petal_GoldenLily")).TryLoad()); }
	if (Glow && RimLight)
	{
		for (int32 k = 0; k < RimLight->GetNumSections(); ++k) { RimLight->SetMaterial(k, Glow); }
	}
}

// ==================================================================== ASalonPondPlants

namespace SalonPondPlantsBuild
{
	namespace MN = MuseeNature;
	using SalonPondBuild::kA;
	using SalonPondBuild::kB;

	struct FOut
	{
		FNatureMesh Leaves, Petals, Stems, Sedge, FmnLeaves, FmnFlowers, Solids;
	};

	void Append(FNatureMesh& To, const FNatureMesh& From)
	{
		const int32 Base = To.Vertices.Num();
		To.Vertices.Append(From.Vertices);
		To.Normals.Append(From.Normals);
		To.Tangents.Append(From.Tangents);
		To.UV0.Append(From.UV0);
		To.UV1.Append(From.UV1);
		To.UV2.Append(From.UV2);
		To.UV3.Append(From.UV3);
		To.Colors.Append(From.Colors);
		for (const int32 I : From.Triangles) { To.Triangles.Add(Base + I); }
	}

	/** Inside the water's edge less Margin (metres from the pond's centre), or above the coping's top (4 cm over the water). */
	bool Clear(const FNatureMesh& M, double Margin)
	{
		for (const FVector& Cm : M.Vertices)
		{
			const FVector P = Cm / 100.0;
			if (P.Z > 0.075) { continue; }
			if (FMath::Square(P.X / (kA - Margin)) + FMath::Square(P.Y / (kB - Margin)) > 1.0) { return false; }
		}
		return true;
	}

	FNatureWind Still(double Phase)
	{
		FNatureWind W;
		W.Bend = 0.01;
		W.Phase = Phase;
		W.FlutterPhase = FMath::Frac(Phase * 5.3);
		return W;
	}

	/** A petal of the iris (a fall, a standard, a style arm): a curved grid from Base out along Radial, rising at Elev
	 *  degrees and curling by Curl towards the tip, its edge a little ruffled; Colour(u −1…1 across, s 0…1 along). */
	void IrisPetal(FNatureMesh& M, const FVector& Base, const FVector& Radial, double Length, double Width, double Elev, double Curl, double Cup,
				   double Ruffle, TFunctionRef<FLinearColor(double, double)> Colour, const FNatureWind& Wind0, int32 NX, int32 NY)
	{
		const FVector Up = FVector::UpVector;
		const FVector Side = FVector::CrossProduct(Up, Radial).GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector);
		TArray<FVector> Line, PlaneN;
		FVector P = Base;
		for (int32 j = 0; j <= NY; ++j)
		{
			const double S = double(j) / NY;
			const double E = FMath::DegreesToRadians(Elev + Curl * S * S);
			Line.Add(P);
			PlaneN.Add(-Radial * FMath::Sin(E) + Up * FMath::Cos(E));
			P += (Radial * FMath::Cos(E) + Up * FMath::Sin(E)) * (Length / NY);
		}
		const int32 Row = NX + 1;
		TArray<FVector> Pts;
		Pts.SetNum(Row * (NY + 1));
		for (int32 j = 0; j <= NY; ++j)
		{
			const double S = double(j) / NY;
			const double HalfW = 0.5 * Width * (0.16 * (1.0 - S) + FMath::Pow(FMath::Sin(UE_DOUBLE_PI * FMath::Pow(S, 0.72)), 0.55));
			for (int32 i = 0; i <= NX; ++i)
			{
				const double X = -1.0 + 2.0 * i / NX;
				const double Wave = Ruffle * Width * FMath::Sin(9.0 * X + 5.0 * S) * X * X * S;
				Pts[j * Row + i] = Line[j] + Side * (X * HalfW) + PlaneN[j] * (Cup * Width * X * X + Wave);
			}
		}
		TArray<int32> Index;
		Index.SetNum(Pts.Num());
		for (int32 j = 0; j <= NY; ++j)
		{
			const double S = double(j) / NY;
			for (int32 i = 0; i <= NX; ++i)
			{
				const FVector DX = Pts[j * Row + FMath::Min(i + 1, NX)] - Pts[j * Row + FMath::Max(i - 1, 0)];
				const FVector DY = Pts[FMath::Min(j + 1, NY) * Row + i] - Pts[FMath::Max(j - 1, 0) * Row + i];
				FVector N = FVector::CrossProduct(DX, DY).GetSafeNormal(UE_SMALL_NUMBER, PlaneN[j]);
				if (FVector::DotProduct(N, PlaneN[j]) < 0) { N = -N; }
				FNatureWind W = Wind0;
				W.Flutter = 0.05 * S;
				W.Height = Pts[j * Row + i].Z;
				Index[j * Row + i] = M.Vertex(Pts[j * Row + i], N, Side, FVector2D(double(i) / NX, S), W, Colour(-1.0 + 2.0 * i / NX, S), 0.8 + 0.2 * S);
			}
		}
		for (int32 j = 0; j < NY; ++j)
		{
			for (int32 i = 0; i < NX; ++i)
			{
				M.Quad(Index[j * Row + i], Index[j * Row + i + 1], Index[(j + 1) * Row + i + 1], Index[(j + 1) * Row + i]);
			}
		}
	}

	void Stalk(FNatureMesh& M, const FVector& From, const FVector& To, double Radius, const FVector& Bow, const FLinearColor& C0,
			   const FLinearColor& C1, const FNatureWind& Wind)
	{
		TArray<FNatureTubeRing> Rings;
		constexpr int32 Segs = 8;
		for (int32 s = 0; s <= Segs; ++s)
		{
			const double T = double(s) / Segs;
			FNatureTubeRing& R = Rings.AddDefaulted_GetRef();
			R.Centre = FMath::Lerp(From, To, T) + Bow * FMath::Sin(UE_DOUBLE_PI * T);
			R.Radius = Radius * (1.0 - 0.25 * T);
			R.Colour = MN::Mix(C0, C1, T);
			R.Wind = Wind;
			R.Wind.Bend = Wind.Bend * T * T;
			R.Wind.Flutter = 0;
			R.Wind.Height = R.Centre.Z;
		}
		for (int32 s = 0; s <= Segs; ++s)
		{
			Rings[s].Dir = (Rings[FMath::Min(Segs, s + 1)].Centre - Rings[FMath::Max(0, s - 1)].Centre).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
		}
		MN::AddTube(M, Rings, 6, 0.0, true);
	}

	/** Iris laevigata in flower: three falls (the white signal down each), three standards, three style arms; the spathe. */
	void IrisFlower(FOut& Out, const FVector& Top, const FVector& Axis, double Size, const FRandomStream& Rng, double Phase)
	{
		const FLinearColor Violet = MN::Vary(MN::Srgb(0x4C44A8), Rng, 6.0, 0.06, 0.1);
		const FLinearColor Deep = MN::Mix(Violet, MN::Srgb(0x2E2878), 0.55);
		const FLinearColor Signal = MN::Srgb(0xF6F2E4), Yellow = MN::Srgb(0xE8C850), Pale = MN::Mix(Violet, MN::Srgb(0xC8C0EC), 0.55);
		const FNatureWind W = Still(Phase);
		const double Az0 = Rng.FRandRange(0.0, MN::Tau);
		for (int32 k = 0; k < 3; ++k)
		{
			const double Az = Az0 + MN::Tau * k / 3 + Rng.FRandRange(-0.12, 0.12);
			const FVector Radial(FMath::Cos(Az), FMath::Sin(Az), 0.0);
			// The fall: broad, out and down, the signal a narrow white line (yellow at its throat) down its middle.
			IrisPetal(Out.Petals, Top + Radial * 0.006, Radial, 0.072 * Size, 0.042 * Size, 28.0 + Rng.FRandRange(-6.0, 6.0), -78.0, -0.06, 0.05,
					  [&](double X, double S)
					  {
						  const double Line = FMath::Clamp(1.0 - FMath::Abs(X) / 0.34, 0.0, 1.0) * FMath::Clamp((0.62 - S) / 0.2, 0.0, 1.0);
						  const FLinearColor Base = MN::Mix(Violet, Deep, 0.35 * S + 0.25 * FMath::Abs(X));
						  return MN::Mix(Base, MN::Mix(Yellow, Signal, FMath::Clamp(S / 0.25, 0.0, 1.0)), Line);
					  }, W, 8, 10);
			// The standard between the falls: narrow, upright.
			const double AzS = Az + MN::Tau / 6;
			const FVector RS(FMath::Cos(AzS), FMath::Sin(AzS), 0.0);
			IrisPetal(Out.Petals, Top + RS * 0.003 + FVector(0, 0, 0.004), RS, 0.05 * Size, 0.018 * Size, 72.0 + Rng.FRandRange(-6.0, 6.0), -22.0, 0.1, 0.02,
					  [&](double X, double S) { return MN::Mix(Violet, Deep, 0.3 * S); }, W, 4, 7);
			// The style arm over the fall's base: pale, petal-like, its two crests at the tip.
			IrisPetal(Out.Petals, Top + Radial * 0.004 + FVector(0, 0, 0.008), Radial, 0.034 * Size, 0.013 * Size, 22.0, -14.0, 0.25, 0.0,
					  [&](double X, double S) { return MN::Mix(Pale, Violet, 0.3 * S); }, W, 4, 5);
		}
		// The spathe: two green bracts clasping the ovary below the flower.
		for (const double Az : {Az0 + 0.5, Az0 + 0.5 + UE_DOUBLE_PI})
		{
			FNatureBlade B;
			B.Length = 0.07; B.Width = 0.014; B.Curl = -12.0; B.TipShape = 0.7; B.BaseWidth = 0.6; B.NX = 2; B.NY = 5;
			B.Root = MN::Srgb(0x5E7A3A); B.Tip = MN::Srgb(0x8A8A50); B.BendGain = 0.0; B.FlutterGain = 0.02;
			const FVector R(FMath::Cos(Az), FMath::Sin(Az), 0.0);
			MN::AddBlade(Out.Leaves, Top - Axis * 0.06 + R * 0.004, (Axis + R * 0.18).GetSafeNormal(), R, B, W);
		}
	}

	/** A clump of Iris laevigata: fans of sword leaves, flowers on stalks a little below the leaves' tips, a bud or two. */
	void IrisClump(FOut& Out, const FVector& At, const FVector2D& Inward, FRandomStream& Rng, int32 Flowers)
	{
		const int32 Fans = Rng.RandRange(5, 7);
		for (int32 f = 0; f < Fans; ++f)
		{
			const double R = Rng.FRandRange(0.0, 0.13);
			const double A = Rng.FRandRange(0.0, MN::Tau);
			const FVector Foot = At + FVector(R * FMath::Cos(A), R * FMath::Sin(A), 0.0);
			const double Plane = Rng.FRandRange(0.0, UE_DOUBLE_PI);
			const FVector Along(FMath::Cos(Plane), FMath::Sin(Plane), 0.0);     // the fan's plane: Along and up
			const FVector Across(-Along.Y, Along.X, 0.0);
			const int32 Leaves = Rng.RandRange(4, 7);
			const double Tall = Rng.FRandRange(0.62, 0.78);
			for (int32 l = 0; l < Leaves; ++l)
			{
				const double X = Leaves > 1 ? -1.0 + 2.0 * l / (Leaves - 1) : 0.0;
				for (int32 Try = 0; Try < 5; ++Try)
				{
					const double Spread = FMath::DegreesToRadians(X * 24.0 + Rng.FRandRange(-5.0, 5.0));
					FVector Dir = (FVector::UpVector * FMath::Cos(Spread) + Along * FMath::Sin(Spread)).GetSafeNormal();
					// Lean the whole clump a little inwards, more on each retry.
					Dir = (Dir + FVector(Inward.X, Inward.Y, 0.0) * (0.06 + 0.08 * Try)).GetSafeNormal();
					FNatureBlade B;
					B.Length = Tall * (1.0 - 0.28 * FMath::Abs(X)) * Rng.FRandRange(0.85, 1.05);
					B.Width = Rng.FRandRange(0.013, 0.018);
					B.Curl = (Rng.FRand() < 0.5 ? -1.0 : 1.0) * Rng.FRandRange(8.0, 30.0) * (0.5 + FMath::Abs(X));
					B.Twist = Rng.FRandRange(-12.0, 12.0);
					B.TipShape = 1.4; B.BaseWidth = 0.9; B.NX = 2; B.NY = 12; B.Cup = 0.02;
					const bool bBrown = Rng.FRand() < 0.1;
					B.Root = MN::Mix(MN::Srgb(0x8A7058), MN::Srgb(0x9AA070), Rng.FRand() * 0.5);
					B.Tip = bBrown ? MN::Srgb(0x9A8A5A) : MN::Vary(MN::Srgb(0x5A7E3C), Rng, 5.0, 0.08, 0.1);
					B.BendGain = 0.02; B.FlutterGain = 0.03;
					FNatureMesh Leaf;
					MN::AddBlade(Leaf, Foot + Along * (X * 0.006) - FVector(0, 0, 0.12), Dir, Across, B, Still(Rng.FRand()));
					if (Clear(Leaf, 0.02) || Try == 4) { Append(Out.Leaves, Leaf); break; }
				}
			}
		}
		for (int32 k = 0; k < Flowers; ++k)
		{
			const double A = Rng.FRandRange(0.0, MN::Tau);
			const FVector Foot = At + FVector(0.06 * FMath::Cos(A), 0.06 * FMath::Sin(A), -0.12);
			const double H = Rng.FRandRange(0.44, 0.56);
			const FVector Top = At + FVector(Inward.X, Inward.Y, 0.0) * Rng.FRandRange(0.04, 0.1) + FVector(Rng.FRandRange(-0.04, 0.04), Rng.FRandRange(-0.04, 0.04), H);
			const FVector Axis = (Top - Foot).GetSafeNormal();
			const FVector Bow = FVector(Rng.FRandRange(-1.0, 1.0), Rng.FRandRange(-1.0, 1.0), 0.0).GetSafeNormal() * 0.02;
			Stalk(Out.Stems, Foot, Top - Axis * 0.01, 0.0036, Bow, MN::Srgb(0x7A8A58), MN::Srgb(0x5E7C3A), Still(Rng.FRand()));
			IrisFlower(Out, Top, Axis, Rng.FRandRange(0.92, 1.08), Rng, Rng.FRand());
			if (Rng.FRand() < 0.6)
			{
				// A bud: a furled violet spike in its spathe, on a shorter stalk.
				const FVector BTop = Top + FVector(Rng.FRandRange(-0.06, 0.06), Rng.FRandRange(-0.06, 0.06), -Rng.FRandRange(0.06, 0.12));
				const FVector BAxis = (BTop - Foot).GetSafeNormal();
				Stalk(Out.Stems, Foot + FVector(0.01, 0.0, 0.0), BTop, 0.0032, -Bow, MN::Srgb(0x7A8A58), MN::Srgb(0x5E7C3A), Still(Rng.FRand()));
				MN::AddEllipsoid(Out.Petals, BTop + BAxis * 0.03, BAxis, 0.008, 0.032, 8, 6, Still(0.3), MN::Srgb(0x3E3690), 0.9);
				FNatureBlade B;
				B.Length = 0.06; B.Width = 0.013; B.Curl = -8.0; B.TipShape = 0.7; B.BaseWidth = 0.6; B.NX = 2; B.NY = 4;
				B.Root = MN::Srgb(0x5E7A3A); B.Tip = MN::Srgb(0x7E8A4A); B.BendGain = 0.0; B.FlutterGain = 0.0;
				MN::AddBlade(Out.Leaves, BTop - BAxis * 0.005, BAxis, FVector(BAxis.Y, -BAxis.X, 0.0).GetSafeNormal(UE_SMALL_NUMBER, FVector::ForwardVector), B, Still(0.2));
			}
		}
	}

	/** A tuft of sedge (Carex): narrow arching blades, a few culms with brown spikes. */
	void Sedge(FOut& Out, const FVector& At, const FVector2D& Inward, FRandomStream& Rng)
	{
		const int32 Blades = Rng.RandRange(34, 46);
		for (int32 b = 0; b < Blades; ++b)
		{
			for (int32 Try = 0; Try < 5; ++Try)
			{
				const double Az = Rng.FRandRange(0.0, MN::Tau);
				const FVector Out2(FMath::Cos(Az), FMath::Sin(Az), 0.0);
				FVector Dir = (FVector::UpVector + Out2 * Rng.FRandRange(0.15, 0.55) + FVector(Inward.X, Inward.Y, 0.0) * (0.25 * Try)).GetSafeNormal();
				FNatureBlade B;
				B.Length = Rng.FRandRange(0.32, 0.55);
				B.Width = Rng.FRandRange(0.004, 0.0065);
				B.Droop = Rng.FRandRange(2.0, 4.5);
				B.Twist = Rng.FRandRange(-40.0, 40.0);
				B.TipShape = 1.2; B.BaseWidth = 0.8; B.NX = 2; B.NY = 9; B.Cup = 0.15;
				B.Root = MN::Srgb(0x7C8452);
				B.Tip = Rng.FRand() < 0.15 ? MN::Srgb(0xA89A60) : MN::Vary(MN::Srgb(0x5E7436), Rng, 5.0, 0.08, 0.12);
				B.BendGain = 0.02; B.FlutterGain = 0.03;
				FNatureMesh Blade;
				MN::AddBlade(Blade, At + FVector(Rng.FRandRange(-0.03, 0.03), Rng.FRandRange(-0.03, 0.03), -0.12), Dir,
							 FVector(-Dir.X, -Dir.Y, 1.0 - Dir.Z).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector), B, Still(Rng.FRand()));
				if (Clear(Blade, 0.015) || Try == 4) { Append(Out.Sedge, Blade); break; }
			}
		}
		const int32 Culms = Rng.RandRange(3, 5);
		for (int32 c = 0; c < Culms; ++c)
		{
			const FVector Foot = At + FVector(Rng.FRandRange(-0.02, 0.02), Rng.FRandRange(-0.02, 0.02), -0.12);
			const FVector Top = At + FVector(Inward.X, Inward.Y, 0.0) * Rng.FRandRange(0.0, 0.08) +
				FVector(Rng.FRandRange(-0.06, 0.06), Rng.FRandRange(-0.06, 0.06), Rng.FRandRange(0.30, 0.44));
			Stalk(Out.Stems, Foot, Top, 0.0018, FVector(0.01, 0.0, 0.0), MN::Srgb(0x7A8A50), MN::Srgb(0x6A7A40), Still(Rng.FRand()));
			const FVector Axis = (Top - Foot).GetSafeNormal();
			for (int32 s = 0; s < 3; ++s)
			{
				const FVector Side = FVector(Rng.FRandRange(-1.0, 1.0), Rng.FRandRange(-1.0, 1.0), 0.0).GetSafeNormal();
				const FVector C = Top - Axis * (0.03 * s) + Side * 0.006;
				MN::AddEllipsoid(Out.Solids, C, (Axis + Side * 0.4).GetSafeNormal(), 0.0032, 0.014, 6, 5, Still(0.1),
								 s == 0 ? MN::Srgb(0x6A5230) : MN::Srgb(0x5A4A2C), 0.9);
			}
		}
	}

	/** A patch of water forget-me-nots: low leafy stems, sky-blue flowers with a yellow eye in coiled sprays, pink buds. */
	void ForgetMeNots(FOut& Out, const FVector& At, double Radius, const FVector2D& Inward, FRandomStream& Rng)
	{
		const int32 Stems = Rng.RandRange(10, 15);
		for (int32 k = 0; k < Stems; ++k)
		{
			const double A = Rng.FRandRange(0.0, MN::Tau), R = Radius * FMath::Sqrt(Rng.FRand());
			const FVector Foot = At + FVector(R * FMath::Cos(A), R * FMath::Sin(A), -0.05);
			const double Lean = Rng.FRandRange(0.1, 0.5);
			const FVector2D LD = (FVector2D(FMath::Cos(A), FMath::Sin(A)) * 0.5 + Inward).GetSafeNormal();
			const FVector Top = Foot + FVector(LD.X * Lean * 0.2, LD.Y * Lean * 0.2, Rng.FRandRange(0.12, 0.24));
			const FVector Axis = (Top - Foot).GetSafeNormal();
			Stalk(Out.Stems, Foot, Top, 0.0016, FVector(0, 0, 0.0), MN::Srgb(0x6A8A48), MN::Srgb(0x5E8040), Still(Rng.FRand()));
			// Leaves: oblong, alternate, up the stem.
			const int32 Leaves = Rng.RandRange(3, 5);
			for (int32 l = 0; l < Leaves; ++l)
			{
				const double T = 0.25 + 0.6 * l / Leaves;
				const FVector P = FMath::Lerp(Foot, Top, T);
				const double Az = Rng.FRandRange(0.0, MN::Tau);
				const FVector Dir = (FVector(FMath::Cos(Az), FMath::Sin(Az), 0.0) + FVector::UpVector * 0.5).GetSafeNormal();
				const FVector N = FVector::CrossProduct(Dir, FVector(-FMath::Sin(Az), FMath::Cos(Az), 0.0)).GetSafeNormal();
				MN::AddCard(Out.FmnLeaves, P, Dir, N.Z < 0 ? -N : N, Rng.FRandRange(0.03, 0.045), Rng.FRandRange(0.009, 0.013), 0.25, 0.15, 2,
							Still(0.1), Still(0.2), MN::Vary(MN::Srgb(0x587C38), Rng, 4.0, 0.08, 0.1), 0.9);
			}
			// The spray: flowers down a coiled tip, the youngest (pink buds) in the coil.
			const int32 Flowers = Rng.RandRange(5, 9);
			const double Az0 = Rng.FRandRange(0.0, MN::Tau);
			for (int32 f = 0; f < Flowers; ++f)
			{
				const double T = double(f) / Flowers;
				const double Ang = Az0 + T * 3.4;
				const FVector C = Top + FVector(FMath::Cos(Ang), FMath::Sin(Ang), 0.0) * (0.012 * (1.0 - 0.6 * T)) + FVector(0, 0, 0.012 * T - 0.004 * (f % 2));
				const bool bBud = T > 0.7;
				const FVector N = (FVector::UpVector + FVector(FMath::Cos(Ang), FMath::Sin(Ang), 0.0) * 0.7).GetSafeNormal();
				if (bBud)
				{
					MN::AddEllipsoid(Out.Solids, C, N, 0.0018, 0.0025, 5, 4, Still(0.2), MN::Srgb(0xD890B0), 1.0);
				}
				else
				{
					MN::AddFlowerCard(Out.FmnFlowers, C, N, Rng.FRandRange(0.0, MN::Tau), Rng.FRandRange(0.0085, 0.0105), 0.12, Still(0.2),
									  MN::Vary(MN::Srgb(0x78A8E8), Rng, 6.0, 0.08, 0.06), 1.0);
				}
			}
		}
	}
}

ASalonPondPlants::ASalonPondPlants()
{
	GetRootComponent()->SetMobility(EComponentMobility::Movable);
	Seed = 7;
}

void ASalonPondPlants::BuildPlant()
{
	using namespace SalonPondPlantsBuild;
	FOut Out;
	FRandomStream Rng(Seed * 7919 + 11);
	const double Z = -double(ShelfDepth) + 0.12;   // the plants' feet are at −0.12 in their own functions
	for (const double SX : {1.0, -1.0})
	{
		const FVector2D In(-SX, 0.0);
		// Two iris clumps flanking the axis (the axis itself left open), sedge behind them and at the end, forget-me-nots at
		// the shelf's front and between.
		IrisClump(Out, FVector(SX * 3.50, -0.60 + Rng.FRandRange(-0.03, 0.03), Z), (In + FVector2D(0.0, 0.35)).GetSafeNormal(), Rng, 2);
		IrisClump(Out, FVector(SX * 3.47, 0.62 + Rng.FRandRange(-0.03, 0.03), Z), (In + FVector2D(0.0, -0.35)).GetSafeNormal(), Rng, SX > 0 ? 3 : 2);
		Sedge(Out, FVector(SX * 3.74, 0.04 * SX, Z), In, Rng);
		Sedge(Out, FVector(SX * 3.33, -0.92, Z), (In + FVector2D(0.0, 0.6)).GetSafeNormal(), Rng);
		Sedge(Out, FVector(SX * 3.33, 0.93, Z), (In + FVector2D(0.0, -0.6)).GetSafeNormal(), Rng);
		ForgetMeNots(Out, FVector(SX * 3.30, 0.0, Z), 0.14, In, Rng);
		ForgetMeNots(Out, FVector(SX * 3.62, -0.25, Z), 0.08, In, Rng);
		ForgetMeNots(Out, FVector(SX * 3.60, 0.30, Z), 0.08, In, Rng);
	}
	WriteSection(0, Out.Leaves, TEXT("MI_Salon_IrisLeaf"));
	WriteSection(1, Out.Petals, TEXT("MI_Salon_IrisPetal"));
	WriteSection(2, Out.Stems, TEXT("MI_Stem_Green"));
	WriteSection(3, Out.Sedge, TEXT("MI_Salon_Sedge"));
	WriteSection(4, Out.FmnLeaves, TEXT("MI_Leaf_Meadow"));
	WriteSection(5, Out.FmnFlowers, TEXT("MI_Salon_ForgetMeNot"));
	WriteSection(6, Out.Solids, TEXT("MI_Receptacle"));
}
