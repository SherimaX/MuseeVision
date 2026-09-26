#include "Albion/AlbionBuild.h"

/**
 * The ten polished columns and the four half-columns (AlbionBuild.h): a moulded base, a polished shaft of its own
 * British stone, a necking, and a capital carved with the plants the Details board gives it (after the O'Sheas' carving
 * at Oxford: real leaves, undercut, curling out under the abacus). The carving is geometry: each leaf a thin solid laid on
 * the capital's bell and lifting clear of it towards its tip, with a raised midrib, sunk veins and its species' outline;
 * flowers, fruits and berries between.
 */
namespace AlbionColumnsImpl
{
	namespace AP = AlbionPlan;
	using namespace AlbionKit;
	using namespace AlbionBuild;

	constexpr double kNeck = AP::ShaftTop;           // 5.5: the astragal
	constexpr double kBell0 = 5.56, kBell1 = 6.08;   // the bell under the abacus
	constexpr double kR0 = 0.255, kR1 = 0.40;

	/** The bell's radius at height Z (a chalice: slow at first, flaring under the abacus). */
	double BellR(double Z)
	{
		const double T = FMath::Clamp((Z - kBell0) / (kBell1 - kBell0), 0.0, 1.0);
		return kR0 + (kR1 - kR0) * FMath::Pow(T, 1.8);
	}

	double Smooth(double E0, double E1, double X)
	{
		const double T = FMath::Clamp((X - E0) / (E1 - E0), 0.0, 1.0);
		return T * T * (3.0 - 2.0 * T);
	}

	/** A small deterministic random stream. */
	struct FRand
	{
		uint32 S;
		explicit FRand(uint32 Seed) : S(Seed * 2654435761u + 12345u) {}
		double Next() { S = S * 1664525u + 1013904223u; return double((S >> 8) & 0xFFFFFF) / double(0x1000000); }
		double Range(double A, double B) { return A + (B - A) * Next(); }
	};

	/** A leaf's outline and relief. U runs from the stalk (0) to the tip (1), V across (−1 … 1). */
	struct FLeaf
	{
		double Length = 0.3, Width = 0.08;
		double Base = 0.25, Tip = 0.9;           // the outline's fullness: pow(u, Base) · pow(1 − u, Tip), normalised
		int32 Lobes = 0;                         // rounded lobes along each side (oak)
		double LobeDepth = 0.0;
		int32 Teeth = 0;                         // saw teeth along each side
		double ToothDepth = 0.0;
		double Palmate = 0.0;                    // a broad lower lobe each side (ivy, violet's heart)
		double Thick = 0.018;                    // the leaf's own thickness at its middle
		double Rib = 0.006;                      // the raised midrib
		int32 Veins = 6;
		double VeinDepth = 0.003;
		double Curl = 0.08;                      // how far the tip lifts off the bell
		double Droop = 0.04;                     // how far the tip turns down (a crocket)
		double Ruffle = 0.0;                     // a wavy edge (cabbage, acanthus)
		double Cup = 0.2;                        // the leaf's cross-section dished (its edges raised)
		double Roll = 0.0;                       // how far the tip rolls over (radians; 0: from Droop), as the O'Sheas' leaves do
		double RollSpan = 0.28;                  // the share of the length that rolls
	};

	double HalfWidth(const FLeaf& L, double U)
	{
		U = FMath::Clamp(U, 0.0, 1.0);
		const double Peak = L.Base / (L.Base + L.Tip);
		const double Norm = FMath::Pow(Peak, L.Base) * FMath::Pow(1.0 - Peak, L.Tip);
		double W = FMath::Pow(U, L.Base) * FMath::Pow(1.0 - U, L.Tip) / Norm;
		if (L.Palmate > 0.0) { W += L.Palmate * FMath::Exp(-FMath::Square((U - 0.18) / 0.12)); }
		if (L.Lobes > 0)
		{
			const double Phase = FMath::Frac(U * L.Lobes + 0.25);
			W *= 1.0 - L.LobeDepth * FMath::Pow(FMath::Abs(FMath::Sin(kPi * Phase)), 0.6) * Smooth(0.08, 0.2, U);
		}
		if (L.Teeth > 0)
		{
			W *= 1.0 - L.ToothDepth * FMath::Frac(U * L.Teeth) * Smooth(0.1, 0.25, U);
		}
		return L.Width * 0.5 * FMath::Max(W, 0.0);
	}

	/** Where a leaf on the bell starts: its foot's angle and height, its tilt (radians off the vertical, + leans right). */
	struct FPlace
	{
		double Theta = 0.0, Z = kBell0, Tilt = 0.0, Scale = 1.0;
		bool bHalf = false;     // on a half-column: skip what falls behind the wall
	};

	/** A smooth grid (as FMeshData::Patch) whose normals are turned to face Toward(i, j): a rolled leaf's faces turn
	 *  through more than a right angle, so no single direction from its position can say which side is which. */
	void GridPatch(FMeshData& M, int32 NU, int32 NV, TFunctionRef<FVector(int32, int32)> Pos, TFunctionRef<FVector(int32, int32)> Toward)
	{
		const int32 W = NV + 1;
		TArray<FVector> G;
		G.SetNumUninitialized((NU + 1) * W);
		for (int32 i = 0; i <= NU; ++i)
		{
			for (int32 j = 0; j <= NV; ++j) { G[i * W + j] = Pos(i, j); }
		}
		const int32 Base = M.Positions.Num();
		for (int32 i = 0; i <= NU; ++i)
		{
			for (int32 j = 0; j <= NV; ++j)
			{
				const FVector& P = G[i * W + j];
				const FVector DU = G[FMath::Min(i + 1, NU) * W + j] - G[FMath::Max(i - 1, 0) * W + j];
				const FVector DV = G[i * W + FMath::Min(j + 1, NV)] - G[i * W + FMath::Max(j - 1, 0)];
				const FVector Hint = Toward(i, j);
				FVector N = FVector::CrossProduct(DU, DV);
				if (!N.Normalize(1e-24)) { N = Hint.GetSafeNormal(); }
				else if (FVector::DotProduct(N, Hint) < 0.0) { N = -N; }
				M.Vertex(P, N, FVector2D(P.X + P.Y, P.Z));
			}
		}
		for (int32 i = 0; i < NU; ++i)
		{
			for (int32 j = 0; j < NV; ++j) { M.Quad(Base + i * W + j, Base + (i + 1) * W + j, Base + (i + 1) * W + j + 1, Base + i * W + j + 1); }
		}
	}

	/**
	 * A leaf laid on the bell: its front, its back where it lifts off, the rims along its edges and across its end.
	 * The leaf rises up the bell, lifts clear of it, and its last RollSpan rolls outward and over (a circle of the
	 * roll's radius), so from below one sees the rounded underside of a curled leaf, as in carved Gothic foliage, not a
	 * blade's point. (Half-columns: the carving's callers skip leaves behind the wall.)
	 */
	void Leaf(FMeshData& M, const FVector& Axis, const FLeaf& L0, const FPlace& Pl, FRand& R)
	{
		FLeaf L = L0;
		L.Length *= Pl.Scale;
		L.Width *= Pl.Scale;
		// A leaf of the bell's rings stays close to the bell (the carving's envelope is the bell flaring to the abacus, as at
		// Oxford): a small lift and a tight curl at its tip. Only the corner leaves (their Roll given) roll right over.
		const bool bVolute = L0.Roll > 0.0;
		// Every leaf is cut as a solid of stone (3 cm and more, not a shell), lifts off the bell and rolls over at its tip into a
		// rounded lip: seen from the side it stands off the bell with a shadow under it, seen from below it shows a knob, not a point.
		if (!bVolute) { L.Roll = 2.0; L.RollSpan = 0.3; L.Curl = FMath::Clamp(L.Curl, 0.05, 0.08); }
		L.Thick = FMath::Max(L.Thick, bVolute ? 0.04 : 0.028) * Pl.Scale;
		if (bVolute) { L.Width = FMath::Max(L.Width, 0.2 * Pl.Scale); }
		// Each leaf springs broad from the astragal's collar (a narrow foot left a ring of flames round the shaft's top).
		L.Base = FMath::Min(L.Base, 0.15);
		constexpr int32 NU = 28, NV = 10;
		const double U0 = 1.0 - L.RollSpan;
		const double Rc = L.RollSpan * L.Length / L.Roll;          // the roll's radius
		const double TipTrim = 0.78;                                 // a rolled leaf ends blunt: its outline stops short of the point
		const FVector Up(0, 0, 1);

		// The unrolled leaf in the (out, up) plane: (O, Z) of its midsurface at (U, V).
		auto Flat = [&](double U, double V, double& O, double& Z)
		{
			const double HW = HalfWidth(L, U * TipTrim);
			const double Along = L.Length * U, Across = HW * V;
			Z = Pl.Z + Along * FMath::Cos(Pl.Tilt) - Across * FMath::Sin(Pl.Tilt);
			const double Lift = 0.8 * L.Curl * Pl.Scale * FMath::Square(Smooth(0.25, U0, U)) + 0.004;
			O = BellR(FMath::Clamp(Z, kBell0, kBell1)) + Lift;
		};
		// The midsurface point, the front's normal (in the (out, up) plane: NO, NZ) and the tangent angle A there.
		auto Mid = [&](double U, double V, double& O, double& Z, double& A)
		{
			double O0, Z0, Ob, Zb;
			Flat(FMath::Min(U, U0), V, O0, Z0);
			Flat(U0 - 0.01, V, Ob, Zb);
			const double A0 = FMath::Atan2(O0 - Ob, FMath::Max(Z0 - Zb, 1e-6));   // the tangent's angle off the vertical at U0
			if (U <= U0)
			{
				O = O0;
				Z = Z0;
				A = A0 * Smooth(0.2, U0, U);
				return;
			}
			const double Phi = (U - U0) / L.RollSpan * L.Roll;
			O = O0 + Rc * (FMath::Cos(A0) - FMath::Cos(Phi + A0));
			Z = Z0 + Rc * (FMath::Sin(Phi + A0) - FMath::Sin(A0));
			A = Phi + A0;
		};
		auto Frame = [&](int32 i, int32 j, FVector& Radial, FVector& Normal, FVector& Point, double& N, bool& bRolled)
		{
			const double U = double(i) / NU;
			const double V = -1.0 + 2.0 * j / NV;
			double O, Z, A;
			Mid(U, V, O, Z, A);
			const double HW = HalfWidth(L, U * TipTrim);
			const double Side = L.Length * U * FMath::Sin(Pl.Tilt) + HW * V * FMath::Cos(Pl.Tilt);
			const double Theta = Pl.Theta + Side / FMath::Max(O, 0.1);
			Radial = FVector(FMath::Cos(Theta), FMath::Sin(Theta), 0.0);
			Normal = Radial * FMath::Cos(A) - Up * FMath::Sin(A);
			Point = Axis + Radial * O + Up * Z;
			N = L.Thick * FMath::Sqrt(FMath::Max(0.0, 1.0 - V * V)) * (0.6 + 0.4 * (1.0 - U));
			N += L.Rib * FMath::Exp(-FMath::Square(V / 0.12)) * (1.0 - 0.7 * U);
			N += L.Cup * L.Thick * V * V;
			const double VeinPhase = U * L.Veins - FMath::Abs(V) * 0.9;
			N -= L.VeinDepth * FMath::Pow(FMath::Max(0.0, FMath::Cos(2.0 * kPi * VeinPhase)), 10.0) * (1.0 - FMath::Abs(V) * 0.6);
			N += L.Ruffle * FMath::Sin(kPi * 5.0 * U + V * 2.0) * V * V * Pl.Scale;
			bRolled = U > U0;
		};
		auto Pos = [&](int32 i, int32 j, bool bBack) -> FVector
		{
			FVector Radial, Normal, Point;
			double N;
			bool bRolled;
			Frame(i, j, Radial, Normal, Point, N, bRolled);
			if (!bBack) { return Point + Normal * N; }
			const double Back = N - L.Thick * 0.9 - 0.004;
			if (bRolled) { return Point + Normal * Back; }
			// On the bell the back never sinks into it.
			const FVector P = Point + Normal * Back;
			const double O = FVector(P.X - Axis.X, P.Y - Axis.Y, 0.0).Size();
			const double Bell = BellR(FMath::Clamp(P.Z - Axis.Z, kBell0, kBell1)) - 0.004;
			return O >= Bell ? P : FVector(Axis.X, Axis.Y, P.Z) + Radial * Bell;
		};
		auto NormalAt = [&](int32 i, int32 j)
		{
			FVector Radial, Normal, Point;
			double N;
			bool bRolled;
			Frame(i, j, Radial, Normal, Point, N, bRolled);
			return Normal;
		};
		GridPatch(M, NU, NV, [&](int32 i, int32 j) { return Pos(i, j, false); }, [&](int32 i, int32 j) { return NormalAt(i, j); });
		GridPatch(M, NU, NV, [&](int32 i, int32 j) { return Pos(i, j, true); }, [&](int32 i, int32 j) { return -NormalAt(i, j); });
		// The rims along both edges (facing across the leaf) and across its blunt end (facing along it).
		for (const int32 J : {0, NV})
		{
			GridPatch(M, NU, 1, [&](int32 i, int32 k) { return Pos(i, J, k == 1); },
					  [&](int32 i, int32) { return (Pos(i, J, false) - Pos(i, NV / 2, false)).GetSafeNormal(); });
		}
		GridPatch(M, 1, NV, [&](int32 k, int32 j) { return Pos(NU, j, k == 1); },
				  [&](int32, int32 j) { return (Pos(NU, j, false) - Pos(NU - 1, j, false)).GetSafeNormal(); });
	}

	/** A flower facing out from the bell at (Theta, Z): a centre boss and Petals petals, cupped by Cup. */
	void Flower(FMeshData& M, const FVector& Axis, double Theta, double Z, double Radius, int32 Petals, double PetalW, double Cup, double Boss,
				FRand& R, double Stand = 0.02)
	{
		const FVector Radial(FMath::Cos(Theta), FMath::Sin(Theta), 0.0);
		const FVector Tangent(-FMath::Sin(Theta), FMath::Cos(Theta), 0.0);
		const FVector Up(0, 0, 1);
		const FVector C = Axis + Radial * (BellR(Z) + Stand) + Up * Z;
		const double Spin = R.Range(0.0, 2.0 * kPi);
		for (int32 p = 0; p < Petals; ++p)
		{
			const double A = Spin + 2.0 * kPi * p / Petals;
			const FVector Dir = Tangent * FMath::Cos(A) + Up * FMath::Sin(A);
			const FVector Side = FVector::CrossProduct(Radial, Dir);
			constexpr int32 NU = 8, NV = 4;
			auto Pos = [&](int32 i, int32 j, double Off)
			{
				const double U = double(i) / NU, V = -1.0 + 2.0 * j / NV;
				const double W = PetalW * Radius * FMath::Sin(kPi * FMath::Pow(U, 0.7)) * (0.35 + 0.65 * U);
				return C + Dir * (Radius * U) + Side * (W * V) + Radial * (Cup * Radius * U * U + 0.004 * (1.0 - V * V) - Off);
			};
			auto UVf = [](const FVector& P) { return FVector2D(P.X + P.Y, P.Z); };
			M.Patch(NU, NV, [&](int32 i, int32 j) { return Pos(i, j, 0.0); }, UVf, [&](const FVector&) { return Radial; });
			M.Patch(NU, NV, [&](int32 i, int32 j) { return Pos(i, j, 0.006); }, UVf, [&](const FVector&) { return -Radial; });
		}
		if (Boss > 0.0) { Ball(M, C + Radial * (Boss * 0.3), Boss, 12, 0.6); }
	}

	/** The capital's designs (the Details board): 0 oak, 1 poppies/pansies/violets, 2 cabbages in nets, 3 ivy, 4 bramble and
	 *  hawthorn, 5 daisy, 6 trellis roses, 7 pomegranates, 8 acanthus, 9 willow. */
	void Carving(FMeshData& M, const FVector& Axis, int32 Design, double A0, double A1, uint32 Seed)
	{
		FRand R(Seed);
		const bool bHalf = (A1 - A0) < 2.0 * kPi - 1e-3;
		auto Visible = [&](double Theta, double Margin)
		{
			if (!bHalf) { return true; }
			double T = Theta;
			while (T < A0) { T += 2.0 * kPi; }
			while (T > A0 + 2.0 * kPi) { T -= 2.0 * kPi; }
			return T >= A0 + Margin && T <= A1 - Margin;
		};
		auto Ring = [&](const FLeaf& L0, int32 Count, double Z, double Offset, double TiltJitter, double ScaleJitter)
		{
			// Each ring clothes the bell: its leaves broad enough to overlap their neighbours (a carver's leaves touch; a gap
			// showed the bare bell and read as a row of strips) and long enough to climb most of the way to the abacus.
			FLeaf L = L0;
			const double Circ = 2.0 * kPi * BellR(FMath::Min(Z + 0.12, kBell1));
			L.Width = FMath::Max(L0.Width, 1.25 * Circ / Count / 1.3);
			L.Length = FMath::Max(L0.Length, 0.55 * (kBell1 - Z) / 1.3);
			for (int32 k = 0; k < Count; ++k)
			{
				FPlace Pl;
				Pl.Theta = Offset + 2.0 * kPi * k / Count + R.Range(-0.05, 0.05);
				Pl.Z = Z + R.Range(-0.01, 0.01);
				Pl.Tilt = 0.3 * R.Range(-TiltJitter, TiltJitter);   // (a carver sets his leaves up the bell, not at random angles)
				Pl.Scale = 1.3 * (1.0 + 0.5 * R.Range(-ScaleJitter, ScaleJitter));   // (full enough that the bell hardly shows)
				if (!Visible(Pl.Theta, 0.12)) { continue; }
				Leaf(M, Axis, L, Pl, R);
			}
		};
		switch (Design)
		{
		case 0:   // Fallen oak leaves (Mariana): lobed leaves at every angle, acorns between
		{
			FLeaf Oak;
			Oak.Length = 0.24; Oak.Width = 0.12; Oak.Base = 0.5; Oak.Tip = 0.6; Oak.Lobes = 4; Oak.LobeDepth = 0.45; Oak.Curl = 0.07; Oak.Droop = 0.05;
			Ring(Oak, 7, kBell0 + 0.02, 0.2, 0.6, 0.12);
			Oak.Length = 0.30; Oak.Width = 0.14; Oak.Curl = 0.11; Oak.Droop = 0.08;
			Ring(Oak, 7, kBell0 + 0.18, 0.65, 0.5, 0.1);
			for (int32 k = 0; k < 7; ++k)
			{
				const double T = 0.45 + 2.0 * kPi * k / 7.0;
				if (!Visible(T, 0.1)) { continue; }
				const double Z = kBell0 + 0.14 + R.Range(0.0, 0.08);
				const FVector Rad(FMath::Cos(T), FMath::Sin(T), 0);
				const FVector C = Axis + Rad * (BellR(Z) + 0.03) + FVector(0, 0, Z);
				Ball(M, C, 0.016, 12, 1.35);                                  // the nut
				Ball(M, C + FVector(0, 0, 0.018), 0.019, 12, 0.55);           // its cup
			}
			break;
		}
		case 1:   // Poppies, pansies and violets (Ophelia)
		{
			FLeaf Violet;
			Violet.Length = 0.16; Violet.Width = 0.14; Violet.Base = 0.25; Violet.Tip = 1.0; Violet.Palmate = 0.25; Violet.Teeth = 9; Violet.ToothDepth = 0.08;
			Violet.Curl = 0.04; Violet.Droop = 0.02;
			Ring(Violet, 9, kBell0 + 0.02, 0.0, 0.35, 0.15);
			FLeaf Poppy;
			Poppy.Length = 0.30; Poppy.Width = 0.10; Poppy.Base = 0.4; Poppy.Tip = 0.7; Poppy.Lobes = 5; Poppy.LobeDepth = 0.55; Poppy.Curl = 0.1; Poppy.Droop = 0.07;
			Ring(Poppy, 6, kBell0 + 0.15, 0.3, 0.3, 0.1);
			for (int32 k = 0; k < 6; ++k)
			{
				const double T = 0.8 + 2.0 * kPi * k / 6.0;
				if (!Visible(T, 0.12)) { continue; }
				const double Z = kBell0 + 0.30 + R.Range(0.0, 0.05);
				if (k % 2 == 0) { Flower(M, Axis, T, Z, 0.07, 4, 1.1, 0.5, 0.018, R, 0.04); }        // a poppy, cupped
				else
				{
					const FVector Rad(FMath::Cos(T), FMath::Sin(T), 0);
					const FVector C = Axis + Rad * (BellR(Z) + 0.05) + FVector(0, 0, Z);
					Ball(M, C, 0.028, 14, 1.2);                                                       // a seed head
					Ball(M, C + FVector(0, 0, 0.032), 0.022, 12, 0.25);                               // its crown
				}
				Flower(M, Axis, T + 0.35, kBell0 + 0.12, 0.04, 5, 1.2, 0.15, 0.008, R, 0.03);        // a pansy low down
			}
			break;
		}
		case 2:   // Cabbages in their nets (The Last of England)
		{
			FLeaf Cab;
			Cab.Length = 0.26; Cab.Width = 0.22; Cab.Base = 0.3; Cab.Tip = 0.35; Cab.Ruffle = 0.012; Cab.Curl = 0.08; Cab.Droop = 0.04; Cab.Cup = 0.6;
			Cab.Veins = 5; Cab.VeinDepth = 0.004;
			Ring(Cab, 6, kBell0 + 0.01, 0.0, 0.25, 0.1);
			for (int32 k = 0; k < 4; ++k)
			{
				const double T = 0.52 + 2.0 * kPi * k / 4.0;
				if (!Visible(T, 0.15)) { continue; }
				const double Z = kBell0 + 0.30;
				const FVector Rad(FMath::Cos(T), FMath::Sin(T), 0);
				const FVector C = Axis + Rad * (BellR(Z) + 0.05) + FVector(0, 0, Z);
				Ball(M, C, 0.075, 18, 0.9);                                    // the head
				// The net: cords gathered over its face (meridians through the front) and two rings round it.
				const FVector Tan(-Rad.Y, Rad.X, 0.0), Up(0, 0, 1);
				for (int32 c = 0; c < 4; ++c)
				{
					const double a = kPi * c / 4.0;
					Torus(M, C, Tan * FMath::Cos(a) + Up * FMath::Sin(a), 0.071, 0.0035, 24, 4);
				}
				for (const double Off : {0.035, -0.02})
				{
					Torus(M, C + Rad * Off, Rad, FMath::Sqrt(0.071 * 0.071 - Off * Off) + 0.002, 0.0035, 24, 4);
				}
			}
			break;
		}
		case 3:   // Ivy (The Long Engagement)
		{
			FLeaf Ivy;
			Ivy.Length = 0.17; Ivy.Width = 0.15; Ivy.Base = 0.3; Ivy.Tip = 0.9; Ivy.Palmate = 0.35; Ivy.Lobes = 2; Ivy.LobeDepth = 0.35; Ivy.Curl = 0.05;
			Ivy.Droop = 0.02; Ivy.Veins = 3;
			Ring(Ivy, 8, kBell0 + 0.02, 0.0, 0.9, 0.15);
			Ring(Ivy, 8, kBell0 + 0.16, 0.4, 0.9, 0.15);
			Ring(Ivy, 8, kBell0 + 0.30, 0.1, 0.7, 0.12);
			for (int32 k = 0; k < 5; ++k)
			{
				const double T = 1.1 + 2.0 * kPi * k / 5.0;
				if (!Visible(T, 0.12)) { continue; }
				const double Z = kBell0 + 0.24;
				const FVector Rad(FMath::Cos(T), FMath::Sin(T), 0);
				for (int32 b = 0; b < 7; ++b)
				{
					const FVector C = Axis + Rad * (BellR(Z) + 0.035) + FVector(0, 0, Z) + FVector(R.Range(-0.02, 0.02), R.Range(-0.02, 0.02), R.Range(-0.02, 0.02));
					Ball(M, C, 0.011, 8);
				}
			}
			break;
		}
		case 4:   // Bramble and hawthorn of the hedgerow
		{
			FLeaf Bramble;
			Bramble.Length = 0.20; Bramble.Width = 0.11; Bramble.Base = 0.5; Bramble.Tip = 0.9; Bramble.Teeth = 14; Bramble.ToothDepth = 0.14; Bramble.Curl = 0.07;
			Ring(Bramble, 9, kBell0 + 0.02, 0.0, 0.6, 0.15);
			FLeaf Haw;
			Haw.Length = 0.14; Haw.Width = 0.12; Haw.Base = 0.5; Haw.Tip = 0.5; Haw.Lobes = 3; Haw.LobeDepth = 0.6; Haw.Curl = 0.06;
			Ring(Haw, 9, kBell0 + 0.22, 0.35, 0.8, 0.15);
			for (int32 k = 0; k < 6; ++k)
			{
				const double T = 0.2 + 2.0 * kPi * k / 6.0;
				if (!Visible(T, 0.12)) { continue; }
				const double Z = kBell0 + 0.18 + R.Range(0.0, 0.1);
				const FVector Rad(FMath::Cos(T), FMath::Sin(T), 0);
				const FVector C = Axis + Rad * (BellR(Z) + 0.04) + FVector(0, 0, Z);
				for (int32 d = 0; d < 12; ++d)   // a blackberry's drupelets
				{
					const double a = 2.0 * kPi * d / 12.0;
					Ball(M, C + FVector(0.012 * FMath::Cos(a), 0.012 * FMath::Sin(a), 0.01 * FMath::Sin(3.0 * a)), 0.009, 8);
				}
				Ball(M, C + FVector(0, 0, 0.004), 0.012, 10);
			}
			break;
		}
		case 5:   // Daisy (Morris's first wallpaper, 1864): tufts of leaves and flowers
		{
			FLeaf Tuft;
			Tuft.Length = 0.18; Tuft.Width = 0.05; Tuft.Base = 0.6; Tuft.Tip = 0.4; Tuft.Teeth = 5; Tuft.ToothDepth = 0.25; Tuft.Curl = 0.06; Tuft.Droop = 0.05;
			Ring(Tuft, 14, kBell0 + 0.02, 0.0, 0.7, 0.2);
			for (int32 k = 0; k < 10; ++k)
			{
				const double T = 0.3 + 2.0 * kPi * k / 10.0;
				if (!Visible(T, 0.1)) { continue; }
				const double Z = kBell0 + 0.20 + ((k % 2) ? 0.14 : 0.02);
				Flower(M, Axis, T, Z, 0.045, 16, 0.35, 0.1, 0.012, R, 0.03);
			}
			break;
		}
		case 6:   // Trellis roses (Morris, 1864): a lattice on the bell, roses and leaves through it
		{
			for (int32 b = 0; b < 16; ++b)
			{
				const double T = 2.0 * kPi * b / 16.0;
				if (!Visible(T, 0.05)) { continue; }
				TArray<FStation> St;
				for (int32 i = 0; i <= 10; ++i)
				{
					const double Z = kBell0 + (kBell1 - kBell0) * i / 10.0;
					const FVector Rad(FMath::Cos(T), FMath::Sin(T), 0);
					St.Add({Axis + Rad * (BellR(Z) + 0.012) + FVector(0, 0, Z), Rad, FVector(-Rad.Y, Rad.X, 0)});
				}
				SweepStations(M, St, RectSection(-0.012, 0.012, -0.009, 0.009));
			}
			for (const double Zr : {kBell0 + 0.15, kBell0 + 0.33})
			{
				TArray<FVector> Hoop;
				for (int32 i = 0; i <= 64; ++i)
				{
					const double T = 2.0 * kPi * i / 64.0;
					Hoop.Add(Axis + FVector(FMath::Cos(T), FMath::Sin(T), 0) * (BellR(Zr) + 0.014) + FVector(0, 0, Zr));
				}
				if (!bHalf) { FurnitureKit::Bar(M, FurnitureKit::FPath(Hoop, FVector(0, 0, 1)), FurnitureKit::RectSection(0, 0, 0.012, 0.009, 0.003)); }
			}
			FLeaf RoseLeaf;
			RoseLeaf.Length = 0.10; RoseLeaf.Width = 0.06; RoseLeaf.Base = 0.5; RoseLeaf.Tip = 0.8; RoseLeaf.Teeth = 10; RoseLeaf.ToothDepth = 0.1; RoseLeaf.Curl = 0.03;
			Ring(RoseLeaf, 12, kBell0 + 0.04, 0.1, 1.2, 0.2);
			Ring(RoseLeaf, 12, kBell0 + 0.24, 0.4, 1.2, 0.2);
			for (int32 k = 0; k < 8; ++k)
			{
				const double T = 0.2 + 2.0 * kPi * k / 8.0;
				if (!Visible(T, 0.1)) { continue; }
				const double Z = kBell0 + ((k % 2) ? 0.36 : 0.20);
				Flower(M, Axis, T, Z, 0.05, 5, 1.3, 0.35, 0.0, R, 0.035);
				Flower(M, Axis, T + 0.05, Z, 0.032, 5, 1.2, 0.6, 0.012, R, 0.045);
			}
			break;
		}
		case 7:   // Pomegranates (Morris's Fruit, 1866)
		{
			FLeaf Lance;
			Lance.Length = 0.20; Lance.Width = 0.06; Lance.Base = 0.5; Lance.Tip = 0.9; Lance.Curl = 0.07; Lance.Droop = 0.04;
			Ring(Lance, 12, kBell0 + 0.02, 0.0, 0.5, 0.15);
			Ring(Lance, 10, kBell0 + 0.20, 0.3, 0.6, 0.15);
			for (int32 k = 0; k < 5; ++k)
			{
				const double T = 0.6 + 2.0 * kPi * k / 5.0;
				if (!Visible(T, 0.2)) { continue; }
				const double Z = kBell0 + 0.28;
				const FVector Rad(FMath::Cos(T), FMath::Sin(T), 0);
				const FVector C = Axis + Rad * (BellR(Z) + 0.065) + FVector(0, 0, Z);
				Ball(M, C, 0.06, 18, 0.92);
				for (int32 s = 0; s < 6; ++s)   // the calyx crown
				{
					const double a = 2.0 * kPi * s / 6.0;
					FStation S0{C + FVector(0, 0, 0.05), FVector(FMath::Cos(a), FMath::Sin(a), 0), FVector(-FMath::Sin(a), FMath::Cos(a), 0)};
					FStation S1{C + FVector(0.016 * FMath::Cos(a), 0.016 * FMath::Sin(a), 0.082), S0.A, S0.B};
					SweepStations(M, {S0, S1}, RectSection(-0.006, 0.006, -0.004, 0.004));
				}
			}
			break;
		}
		case 8:   // Acanthus (Morris, 1875): two tiers of deeply cut leaves, their tips rolled over
		{
			FLeaf Ac;
			Ac.Length = 0.26; Ac.Width = 0.16; Ac.Base = 0.4; Ac.Tip = 0.5; Ac.Lobes = 4; Ac.LobeDepth = 0.6; Ac.Teeth = 16; Ac.ToothDepth = 0.18;
			Ac.Curl = 0.1; Ac.Droop = 0.1; Ac.Ruffle = 0.006; Ac.Veins = 4;
			Ring(Ac, 8, kBell0 + 0.01, 0.0, 0.1, 0.05);
			Ac.Length = 0.36; Ac.Curl = 0.14; Ac.Droop = 0.13;
			Ring(Ac, 8, kBell0 + 0.10, kPi / 8.0, 0.1, 0.05);
			break;
		}
		default:  // Willow Bough (Morris, 1887): sprays of long narrow leaves
		{
			FLeaf Willow;
			Willow.Length = 0.24; Willow.Width = 0.035; Willow.Base = 0.5; Willow.Tip = 1.2; Willow.Curl = 0.05; Willow.Droop = 0.03; Willow.Thick = 0.012;
			Willow.Rib = 0.004; Willow.Veins = 0;
			Ring(Willow, 18, kBell0 + 0.01, 0.0, 0.7, 0.2);
			Ring(Willow, 18, kBell0 + 0.14, 0.17, 0.8, 0.2);
			Ring(Willow, 16, kBell0 + 0.26, 0.09, 0.9, 0.2);
			break;
		}
		}
		// The corner leaves every design shares (as at Oxford): four big leaves of the capital's own plant springing from the
		// bell's middle and rolling out under the abacus's corners, carrying the square on the round; smaller ones under the
		// middle of each face.
		FLeaf Corner;
		switch (Design)
		{
		case 0: Corner.Lobes = 4; Corner.LobeDepth = 0.45; Corner.Base = 0.5; Corner.Tip = 0.6; Corner.Width = 0.2; break;
		case 1: Corner.Lobes = 5; Corner.LobeDepth = 0.5; Corner.Width = 0.17; break;
		case 2: Corner.Ruffle = 0.014; Corner.Cup = 0.6; Corner.Width = 0.3; Corner.Base = 0.3; Corner.Tip = 0.35; Corner.Veins = 5; break;
		case 3: Corner.Palmate = 0.35; Corner.Lobes = 2; Corner.LobeDepth = 0.35; Corner.Width = 0.24; Corner.Veins = 3; break;
		case 4: Corner.Teeth = 16; Corner.ToothDepth = 0.14; Corner.Width = 0.18; break;
		case 5: Corner.Teeth = 7; Corner.ToothDepth = 0.25; Corner.Width = 0.12; break;
		case 6: Corner.Teeth = 12; Corner.ToothDepth = 0.1; Corner.Width = 0.15; break;
		case 7: Corner.Width = 0.12; Corner.Tip = 0.9; break;
		case 8: Corner.Lobes = 4; Corner.LobeDepth = 0.6; Corner.Teeth = 16; Corner.ToothDepth = 0.18; Corner.Ruffle = 0.008; Corner.Width = 0.24; Corner.Veins = 4; break;
		default: Corner.Width = 0.07; Corner.Tip = 1.2; Corner.Thick = 0.014; Corner.Veins = 0; break;
		}
		// (Long enough to reach under the abacus's corner and roll over there: a volute of about 9 cm.)
		// The upper tier (every design): the capital's own leaf, eight of them between the lower ones, rising to the abacus
		// and rolling out under it, so the bell's upper half is carved too.
		FLeaf Upper = Corner;
		Upper.Length = 0.26; Upper.Droop = 0.05; Upper.Curl = 0.07;
		Ring(Upper, 8, kBell0 + 0.20, kPi / 8.0, 0.1, 0.05);
		Corner.Length = 0.44; Corner.Curl = 0.15; Corner.Droop = 0.0; Corner.Roll = 3.1; Corner.RollSpan = 0.32;
		for (int32 k = 0; k < 8; ++k)
		{
			FPlace Pl;
			const bool bCorner = (k % 2) == 0;
			Pl.Theta = kPi / 4.0 + kPi / 4.0 * k;
			Pl.Z = kBell0 + (bCorner ? 0.08 : 0.03) + R.Range(-0.01, 0.01);
			Pl.Tilt = R.Range(-0.08, 0.08);
			Pl.Scale = bCorner ? 1.0 : 0.72;
			if (!Visible(Pl.Theta, 0.2)) { continue; }
			Leaf(M, Axis, Corner, Pl, R);
		}
	}

	/** A column's base (0 … 0.35 m): a square plinth with chamfered corners, a hollow and two rolls. */
	void ColumnBase(FMeshData& M, const FVector& C, double A0, double A1)
	{
		const bool bHalf = (A1 - A0) < 2.0 * kPi - 1e-3;
		// The square plinth (0 … 0.12), its corners chamfered; on a half-column, its half against the wall.
		const double H = AP::BaseRadius;
		TArray<FVector2D> Sq;
		const double Ch = 0.07;
		Sq = {FVector2D(-H + Ch, -H), FVector2D(H - Ch, -H), FVector2D(H, -H + Ch), FVector2D(H, H - Ch), FVector2D(H - Ch, H), FVector2D(-H + Ch, H),
			  FVector2D(-H, H - Ch), FVector2D(-H, -H + Ch)};
		TArray<FVector2D> Outline;
		for (const FVector2D& P : Sq)
		{
			if (bHalf)
			{
				// Keep the side that faces the court.
				const FVector2D Dir(FMath::Cos(0.5 * (A0 + A1)), FMath::Sin(0.5 * (A0 + A1)));
				if (FVector2D::DotProduct(P, Dir) < -0.01) { continue; }
			}
			Outline.Add(FVector2D(C.X, C.Y) + P);
		}
		if (bHalf)
		{
			// Close the half along the wall's face.
			const FVector2D Dir(FMath::Cos(0.5 * (A0 + A1)), FMath::Sin(0.5 * (A0 + A1)));
			const FVector2D Along(-Dir.Y, Dir.X);
			Outline = {FVector2D(C.X, C.Y) + Along * H, FVector2D(C.X, C.Y) + Along * H + Dir * (H - Ch), FVector2D(C.X, C.Y) + Along * (H - Ch) + Dir * H,
					   FVector2D(C.X, C.Y) - Along * (H - Ch) + Dir * H, FVector2D(C.X, C.Y) - Along * H + Dir * (H - Ch), FVector2D(C.X, C.Y) - Along * H};
		}
		Prism(M, Outline, -0.005, 0.12);
		TArray<FLathePoint> P = {
			{0.0, 0.12, false}, {0.345, 0.12, false}, {0.345, 0.135, false},
			{0.345, 0.16, true}, {0.33, 0.185, true}, {0.30, 0.195, false},       // the lower roll
			{0.285, 0.20, false}, {0.275, 0.235, true}, {0.285, 0.26, false},      // the hollow
			{0.29, 0.27, true}, {0.30, 0.29, true}, {0.29, 0.31, true}, {0.27, 0.318, false},   // the upper roll
			{0.262, 0.33, false}, {0.262, 0.355, false}, {0.0, 0.355, false},
		};
		Lathe(M, FVector(C.X, C.Y, 0.0), P, bHalf ? 24 : 48, 0.0, A0, A1);
	}

	/** The necking, the bell, and the abacus (5.5 … 6.2 m). */
	void CapitalBody(FMeshData& M, const FVector& C, double A0, double A1)
	{
		const bool bHalf = (A1 - A0) < 2.0 * kPi - 1e-3;
		TArray<FLathePoint> P = {{0.0, kNeck - 0.01, false}, {0.258, kNeck - 0.01, false}, {0.262, kNeck, true}, {0.278, kNeck + 0.025, true},
								 {0.262, kNeck + 0.05, true}, {0.255, kNeck + 0.06, false}};
		for (int32 i = 1; i <= 12; ++i)
		{
			const double Z = kBell0 + (kBell1 - kBell0) * i / 12.0;
			P.Add({BellR(Z), Z, i < 12});
		}
		P.Add({0.0, kBell1, false});
		Lathe(M, FVector(C.X, C.Y, 0.0), P, bHalf ? 32 : 64, 0.0, A0, A1);
		// The abacus: square, 0.9 m, moulded underneath; on a half-column, the half before the wall.
		const double H = AP::AbacusHalf;
		const double Z0 = kBell1 - 0.005, Z1 = AP::CapitalTop;
		auto Q = [&](double X, double Y, double Z) { return FVector(C.X + X, C.Y + Y, Z); };
		const FVector2D Dir = bHalf ? FVector2D(FMath::Cos(0.5 * (A0 + A1)), FMath::Sin(0.5 * (A0 + A1))) : FVector2D::ZeroVector;
		const double XL = (bHalf && Dir.X < -0.5) ? 0.0 : -H, XH = (bHalf && Dir.X > 0.5) ? 0.0 : H;
		const double YL = (bHalf && Dir.Y > 0.5) ? 0.0 : -H, YH = (bHalf && Dir.Y < -0.5) ? 0.0 : H;
		const double YLo = (bHalf && Dir.Y > 0.5) ? 0.0 : -H, YHi = (bHalf && Dir.Y < -0.5) ? 0.0 : H;
		(void)YL; (void)YH;
		// The abacus's moulding, square in plan: from the bell's top (0.40) a hollow (cavetto) sweeps out under it to a
		// roll at its lower edge, a fillet, the upright face, and a small chamfer to its top (plan half-size, height).
		TArray<FVector2D> Prof = {FVector2D(0.36, Z0)};
		for (int32 i = 1; i <= 6; ++i)
		{
			const double T = double(i) / 6.0;   // a quarter-circle hollow, 0.36 → H − 0.035 out, Z0 → Z0 + 0.04 up
			Prof.Add(FVector2D(0.36 + (H - 0.035 - 0.36) * (1.0 - FMath::Cos(0.5 * kPi * T)), Z0 + 0.04 * FMath::Sin(0.5 * kPi * T)));
		}
		for (int32 i = 1; i <= 6; ++i)
		{
			const double A = -0.5 * kPi + kPi * i / 6.0;   // the roll: a half-round of 0.018 at the edge
			Prof.Add(FVector2D(H - 0.035 + 0.018 * FMath::Cos(A), Z0 + 0.058 + 0.018 * FMath::Sin(A)));
		}
		Prof.Add(FVector2D(H - 0.012, Z0 + 0.076));
		Prof.Add(FVector2D(H, Z0 + 0.078));
		Prof.Add(FVector2D(H, Z1 - 0.012));
		Prof.Add(FVector2D(H - 0.012, Z1));
		Prof.Add(FVector2D(0.0, Z1));
		// Each profile point a square ring (cut at the wall on a half-column: the wall's side left open against it).
		auto Corner = [&](double Sz, int32 k) -> FVector2D
		{
			const double X0 = XL == 0.0 ? 0.0 : -Sz, X1 = XH == 0.0 ? 0.0 : Sz, Y0 = YLo == 0.0 ? 0.0 : -Sz, Y1 = YHi == 0.0 ? 0.0 : Sz;
			const FVector2D Cs[4] = {FVector2D(X0, Y0), FVector2D(X1, Y0), FVector2D(X1, Y1), FVector2D(X0, Y1)};
			return Cs[k];
		};
		const FVector2D SideN[4] = {FVector2D(0, -1), FVector2D(1, 0), FVector2D(0, 1), FVector2D(-1, 0)};
		for (int32 k = 0; k < 4; ++k)
		{
			// Skip a side that lies in the wall's plane.
			const FVector2D A0 = Corner(1.0, k), A1 = Corner(1.0, (k + 1) % 4);
			if (FMath::IsNearlyZero(A0.X) && FMath::IsNearlyZero(A1.X)) { continue; }
			if (FMath::IsNearlyZero(A0.Y) && FMath::IsNearlyZero(A1.Y)) { continue; }
			for (int32 j = 0; j + 1 < Prof.Num(); ++j)
			{
				const FVector2D P0 = Prof[j], P1 = Prof[j + 1];
				const FVector2D D = P1 - P0;
				// The profile's outward normal in (out, up) (it climbs outwards: the solid on its left).
				const FVector2D N2 = FVector2D(D.Y, -D.X).GetSafeNormal();
				const FVector N = FVector(SideN[k].X * N2.X, SideN[k].Y * N2.X, N2.Y).GetSafeNormal();
				const FVector2D C00 = Corner(P0.X, k), C01 = Corner(P0.X, (k + 1) % 4), C10 = Corner(P1.X, k), C11 = Corner(P1.X, (k + 1) % 4);
				M.Poly({Q(C00.X, C00.Y, P0.Y), Q(C01.X, C01.Y, P0.Y), Q(C11.X, C11.Y, P1.Y), Q(C10.X, C10.Y, P1.Y)}, N);
			}
		}
	}

	/** A shaft: polished, Ø 0.5 m, from the base to the necking; U round it (m), V up. */
	void Shaft(FMeshData& M, const FVector& C, double A0, double A1, double UOffset)
	{
		const bool bHalf = (A1 - A0) < 2.0 * kPi - 1e-3;
		TArray<FLathePoint> P = {{AP::ShaftRadius, AP::BaseTop - 0.01, false}, {AP::ShaftRadius, kNeck + 0.005, false}};
		Lathe(M, FVector(C.X, C.Y, 0.0), P, bHalf ? 32 : 64, UOffset, A0, A1);
	}
}

namespace AlbionBuild
{
	using namespace AlbionColumnsImpl;

	void BuildColumns(FParts& P)
	{
		constexpr double Full0 = 0.0, Full1 = 2.0 * UE_DOUBLE_PI;
		for (int32 i = 0; i < 10; ++i)
		{
			const FStone& S = Stone(i);
			const FVector C(S.X, S.Y, 0.0);
			ColumnBase(P[SlotDressing], C, Full0, Full1);
			// Each shaft its own slice of its stone (the material repeats every few metres round and up).
			Shaft(P[SlotShaft0 + i], C, Full0, Full1, 3.7 * i);
			CapitalBody(P[SlotCarved], C, Full0, Full1);
			Carving(P[SlotCarved], C, S.Capital, Full0, Full1, 1000u + uint32(i) * 97u);
		}
		// The half-columns against the end walls (lines 1 and 7), in the dressed stone, their capitals carved half-round.
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			for (const bool bNorth : {true, false})
			{
				const FVector C(Side * AlbionPlan::ColumnX, bNorth ? AlbionPlan::Y0 : AlbionPlan::Y1, 0.0);
				const double A0 = bNorth ? 0.0 : UE_DOUBLE_PI, A1 = A0 + UE_DOUBLE_PI;
				ColumnBase(P[SlotDressing], C, A0, A1);
				Shaft(P[SlotDressing], C, A0, A1, 0.0);
				CapitalBody(P[SlotCarved], C, A0, A1);
				Carving(P[SlotCarved], C, (Side < 0 ? 0 : 5) + (bNorth ? 0 : 4), A0, A1, 2000u + (bNorth ? 7u : 13u) + uint32(Side + 1) * 31u);
			}
		}
	}
}
