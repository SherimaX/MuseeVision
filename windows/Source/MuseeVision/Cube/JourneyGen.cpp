#include "Cube/JourneyGen.h"
#include "Cube/JourneySeaGen.h"

#include "Math/RandomStream.h"

/**
 * The journeys' generated things. Metres in each thing's own frame (x east, y south, z up), written in centimetres by
 * CubeMesh::FMesh. A named namespace: the module builds in unity files.
 */
namespace JourneyGenKit
{
	using CubeMesh::FMesh;
	using CubeMesh::Block;
	using CubeMesh::Rod;
	constexpr double Pi = UE_DOUBLE_PI;

	double Noise(double X, double Y, double Z) { return FMath::PerlinNoise3D(FVector(X, Y, Z)); }

	/** Fractal noise, Octaves octaves from frequency F (per metre). */
	double Fbm(const FVector& P, double F, int32 Octaves, double Seed)
	{
		double Sum = 0, Amp = 1, Norm = 0;
		for (int32 o = 0; o < Octaves; ++o)
		{
			Sum += Amp * Noise(P.X * F + Seed * 13.1, P.Y * F - Seed * 7.7, P.Z * F + Seed * 3.3);
			Norm += Amp;
			Amp *= 0.5;
			F *= 2.03;
		}
		return Sum / Norm;
	}

	/** A grid surface (NU+1 × NV+1 points) from a function, triangulated and smoothed. Normals face Facing(P). */
	void Grid(FMesh& M, int32 NU, int32 NV, TFunctionRef<FVector(int32, int32)> Pos, TFunctionRef<FVector(const FVector&)> Facing,
			  TFunctionRef<FLinearColor(int32, int32, const FVector&)> Colour, TFunctionRef<FVector2D(int32, int32, const FVector&)> Uv)
	{
		const int32 Base = M.Positions.Num();
		for (int32 j = 0; j <= NV; ++j)
		{
			for (int32 i = 0; i <= NU; ++i)
			{
				const FVector P = Pos(i, j);
				M.V(P, Facing(P), Uv(i, j, P), Colour(i, j, P));
			}
		}
		for (int32 j = 0; j < NV; ++j)
		{
			for (int32 i = 0; i < NU; ++i)
			{
				const int32 A = Base + j * (NU + 1) + i;
				M.Quad(A, A + 1, A + NU + 2, A + NU + 1);
			}
		}
	}

	// ---------------------------------------------------------------------------------------------- the crevasse

	/**
	 * A crevasse in the Theodul glacier (place 2), the eyes at the origin: it runs north–south (y), 90 m long, closing at
	 * both ends; 6 m across at the eyes, the lips 16 m above opening to 9 m and hung with snow, narrowing below to a
	 * dark slot 28 m down. Its walls wander (a meander of a metre or two), bulge and hollow (half-metre undulations),
	 * are scalloped by melt (dimples a hand across) and, near the top, banded by the firn's yearly layers and a dirt band.
	 */
	namespace Crevasse
	{
		constexpr double Top = 16.0, Bottom = -28.0, HalfLength = 45.0;

		double Meander(double S) { return 1.3 * FMath::Sin(S / 17.0) + 0.5 * FMath::Sin(S / 7.3 + 1.0); }

		/** Half the width at s (north, m) and z. */
		double HalfWidth(double S, double Z)
		{
			const double Ends = FMath::SmoothStep(0.0, 1.0, FMath::Clamp((HalfLength - FMath::Abs(S)) / 16.0, 0.0, 1.0));
			double Profile;
			if (Z >= 0) { Profile = 1.0 + 0.55 * FMath::Pow(Z / Top, 1.6); }
			else { Profile = FMath::Max(0.03, 1.0 - 0.97 * FMath::Pow(-Z / -Bottom, 0.8)); }
			const double Vary = 1.0 + 0.12 * Noise(S * 0.05, Z * 0.05, 3.1);
			return 3.05 * Profile * Vary * Ends;
		}

		/** The wall's relief (m, into the gap positive): undulations, scallops, firn bands. */
		double Relief(double S, double Z, double Side)
		{
			const FVector P(S, Z, Side * 10.0);
			const double Undulate = 0.35 * Fbm(P, 0.25, 3, Side + 1);
			// Melt scallops: a field of shallow dimples (the abs of noise, inverted), a hand to a forearm across.
			const double Scallop = -0.07 * FMath::Abs(Fbm(P, 2.4, 2, Side + 5));
			// The firn's layers near the top: thin ledges and grooves every 40–70 cm.
			const double Layers = Z > Top - 7 ? 0.025 * FMath::Sin(Z * 2 * Pi / 0.55 + 2.0 * Noise(S * 0.1, Z * 0.1, 9)) : 0.0;
			return Undulate + Scallop + Layers;
		}

		void Wall(FMesh& M, double Side)
		{
			constexpr double Step = 0.12;
			const int32 NU = FMath::RoundToInt32(2 * HalfLength / Step), NV = FMath::RoundToInt32((Top - Bottom) / Step);
			auto Pos = [&](int32 i, int32 j)
			{
				const double S = -HalfLength + 2 * HalfLength * i / NU;
				const double Z = Bottom + (Top - Bottom) * j / NV;
				const double HW = HalfWidth(S, Z);
				const double X = Meander(S) + Side * FMath::Max(0.02, HW - Relief(S, Z, Side));
				return FVector(X, -S, Z);
			};
			auto Facing = [&](const FVector& P) { return FVector(-Side, 0, 0); };
			auto Colour = [&](int32 i, int32 j, const FVector& P)
			{
				const double Depth = FMath::Clamp((Top - P.Z) / (Top - Bottom), 0.0, 1.0);
				// A dirt band 5 m below the lip (last century's dust), tilted with the glacier's flow.
				const double Dirt = FMath::Exp(-FMath::Square((P.Z - (Top - 5.2 + 0.04 * P.Y)) / 0.25));
				const double Firn = FMath::SmoothStep(Top - 9.0, Top - 2.5, P.Z);
				return FLinearColor(float(Depth), float(Dirt), float(Firn), 1.f);
			};
			auto Uv = [&](int32 i, int32 j, const FVector& P) { return FVector2D(-P.Y, P.Z); };
			Grid(M, NU, NV, Pos, Facing, Colour, Uv);
		}

		/** The glacier's surface either side of the lips: snow, 60 m out, gently wind-rippled. */
		void Surface(FMesh& M, double Side)
		{
			constexpr int32 NU = 360, NV = 90;
			auto Pos = [&](int32 i, int32 j)
			{
				const double S = -HalfLength - 30 + (2 * HalfLength + 60) * i / NU;
				const double Lip = Meander(FMath::Clamp(S, -HalfLength, HalfLength)) + Side * FMath::Max(0.02, HalfWidth(FMath::Clamp(S, -HalfLength + 0.01, HalfLength - 0.01), Top));
				const double D = 60.0 * FMath::Pow(double(j) / NV, 1.5);   // out from the lip
				const double X = Lip + Side * D;
				// The lip rounded over (a cornice), the snow rising a little away from it, sastrugi.
				const double Z = Top + 0.6 * (1.0 - FMath::Exp(-D / 1.2)) + 0.004 * D + 0.06 * Fbm(FVector(X, S, 0), 1.2, 2, 21);
				return FVector(X, -S, Z);
			};
			auto Facing = [](const FVector&) { return FVector(0, 0, 1); };
			auto Colour = [](int32, int32, const FVector&) { return FLinearColor(0, 0, 1, 1); };
			auto Uv = [](int32, int32, const FVector& P) { return FVector2D(P.X, -P.Y); };
			Grid(M, NU, NV, Pos, Facing, Colour, Uv);
		}

		/** Snow bridges: a thick arch of old snow spanning the gap. */
		void Bridge(FMesh& M, double S, double Z, double Thick)
		{
			const double C = Meander(S), HW = HalfWidth(S, Z) + 0.6;
			constexpr int32 NU = 40, NV = 16;
			for (const double Face : {-1.0, 1.0})
			{
				auto Pos = [&](int32 i, int32 j)
				{
					const double T = double(i) / NU;          // across the gap
					const double U = -1.4 + 2.8 * j / NV;     // along the crevasse (width of the bridge)
					const double Sag = -0.8 * FMath::Sin(T * Pi);
					const double Lump = 0.15 * Fbm(FVector(T * 6, U * 2, Face), 1.0, 2, 33);
					const double Zc = Z + Sag + Face * (Thick * 0.5 * (1.0 - 0.3 * FMath::Square(U / 1.4)) + Lump);
					return FVector(C - HW + 2 * HW * T, -(S + U), Zc);
				};
				auto Facing = [Face](const FVector&) { return FVector(0, 0, Face); };
				auto Colour = [](int32, int32, const FVector&) { return FLinearColor(0.1f, 0, 0.8f, 1); };
				auto Uv = [](int32, int32, const FVector& P) { return FVector2D(P.X, -P.Y); };
				Grid(M, NU, NV, Pos, Facing, Colour, Uv);
			}
		}

		/** Icicles under the lips and the bridges: cones, some forked. */
		void Icicles(FMesh& M, FRandomStream& R)
		{
			for (int32 k = 0; k < 90; ++k)
			{
				const double Side = R.FRand() < 0.5 ? -1.0 : 1.0;
				const double S = R.FRandRange(-30, 30);
				const double Z = Top - R.FRandRange(0.4, 2.0);
				const double X = Meander(S) + Side * (HalfWidth(S, Z) - Relief(S, Z, Side) - 0.05);
				const double L = FMath::Pow(R.FRand(), 2.0) * 1.6 + 0.15, Rad = 0.02 + 0.05 * L / 1.6;
				const FVector Tip(X - Side * 0.02, -S, Z - L), Root(X, -S, Z + 0.05);
				constexpr int32 Around = 8;
				const int32 Base = M.Positions.Num();
				for (int32 a = 0; a < Around; ++a)
				{
					const double A = 2 * Pi * a / Around;
					const FVector D(FMath::Cos(A), FMath::Sin(A), 0);
					M.V(Root + D * Rad, D, FVector2D(A, 0), FLinearColor(0.2f, 0, 0.3f, 1));
				}
				const int32 TipId = M.V(Tip, FVector(0, 0, -1), FVector2D(0, L), FLinearColor(0.2f, 0, 0.3f, 1));
				for (int32 a = 0; a < Around; ++a) { M.Tri(Base + a, Base + (a + 1) % Around, TipId); }
			}
		}
	}
}

namespace JourneyGenKit
{
	// ---------------------------------------------------------------------------------------------- buildings

	namespace Arch
	{
		struct FHole { double U0, V0, U1, V1; bool bDoor = false; };

		/** The parts a building's sections collect. */
		struct FParts
		{
			FMesh Render, Cladding, Frames, Glass, Roof, Stone, Timber, Steel, Solar, Flag, Paint;
		};

		/**
		 * A wall face: from O (its lower left, metres) along U (unit, the face's width W) and V (up, height H), facing N,
		 * with openings (windows and doors) recessed Depth into the wall: the face round them, their reveals, a frame
		 * (FrameW wide, 7 cm deep) near the glass, the glass a little behind it, a sill under windows. UV0: metres (u, v).
		 */
		void Facade(FMesh& Wall, FParts& P, const FVector& O, const FVector& U, const FVector& V, const FVector& N, double W, double H,
					const TArray<FHole>& Holes, double Depth, double FrameW, bool bSills)
		{
			TArray<double> Us = {0.0, W}, Vs = {0.0, H};
			for (const FHole& Hl : Holes) { Us.AddUnique(Hl.U0); Us.AddUnique(Hl.U1); Vs.AddUnique(Hl.V0); Vs.AddUnique(Hl.V1); }
			Us.Sort();
			Vs.Sort();
			auto In = [&Holes](double u, double v)
			{
				for (const FHole& Hl : Holes) { if (u > Hl.U0 && u < Hl.U1 && v > Hl.V0 && v < Hl.V1) { return true; } }
				return false;
			};
			auto At = [&](double u, double v) { return O + U * u + V * v; };
			for (int32 i = 0; i + 1 < Us.Num(); ++i)
			{
				for (int32 j = 0; j + 1 < Vs.Num(); ++j)
				{
					const double u0 = Us[i], u1 = Us[i + 1], v0 = Vs[j], v1 = Vs[j + 1];
					if (u1 - u0 < 1e-4 || v1 - v0 < 1e-4 || In(0.5 * (u0 + u1), 0.5 * (v0 + v1))) { continue; }
					Wall.Quad(Wall.V(At(u0, v0), N, FVector2D(u0, v0), FLinearColor::White, FVector2D(0, 0), U),
							  Wall.V(At(u1, v0), N, FVector2D(u1, v0), FLinearColor::White, FVector2D(0, 0), U),
							  Wall.V(At(u1, v1), N, FVector2D(u1, v1), FLinearColor::White, FVector2D(0, 0), U),
							  Wall.V(At(u0, v1), N, FVector2D(u0, v1), FLinearColor::White, FVector2D(0, 0), U));
				}
			}
			for (const FHole& Hl : Holes)
			{
				const FVector In_ = -N * Depth;
				// Reveals (facing into the opening).
				auto Reveal = [&](const FVector& A, const FVector& B, const FVector& Nr)
				{
					Wall.Quad(Wall.V(A, Nr, FVector2D(0, 0)), Wall.V(B, Nr, FVector2D(1, 0)), Wall.V(B + In_, Nr, FVector2D(1, Depth)), Wall.V(A + In_, Nr, FVector2D(0, Depth)));
				};
				Reveal(At(Hl.U0, Hl.V0), At(Hl.U0, Hl.V1), U);
				Reveal(At(Hl.U1, Hl.V0), At(Hl.U1, Hl.V1), -U);
				Reveal(At(Hl.U0, Hl.V1), At(Hl.U1, Hl.V1), -V);
				Reveal(At(Hl.U0, Hl.V0), At(Hl.U1, Hl.V0), V);
				// The frame: four members round the opening, 7 cm deep, set back from the face; a mullion if wide.
				const double Back = Depth - 0.08, T = 0.035;
				const FVector C0 = At(0.5 * (Hl.U0 + Hl.U1), 0.5 * (Hl.V0 + Hl.V1)) - N * Back;
				const double HW = 0.5 * (Hl.U1 - Hl.U0), HH = 0.5 * (Hl.V1 - Hl.V0);
				Block(P.Frames, C0 + V * (HH - FrameW / 2), U, V, N, FVector(HW, FrameW / 2, T));
				Block(P.Frames, C0 - V * (HH - FrameW / 2), U, V, N, FVector(HW, FrameW / 2, T));
				Block(P.Frames, C0 + U * (HW - FrameW / 2), U, V, N, FVector(FrameW / 2, HH, T));
				Block(P.Frames, C0 - U * (HW - FrameW / 2), U, V, N, FVector(FrameW / 2, HH, T));
				if (!Hl.bDoor && HW * 2 > 0.95) { Block(P.Frames, C0, U, V, N, FVector(FrameW * 0.45, HH, T)); }
				if (!Hl.bDoor && HH * 2 > 1.3) { Block(P.Frames, C0 + V * (HH * 0.35), U, V, N, FVector(HW, FrameW * 0.4, T)); }
				// The glass (or a door's panel), just behind the frame's middle.
				const FVector G0 = At(Hl.U0, Hl.V0) - N * (Back + 0.01), G1 = At(Hl.U1, Hl.V0) - N * (Back + 0.01);
				FMesh& Pane = Hl.bDoor ? P.Frames : P.Glass;
				Pane.Quad(Pane.V(G0, N, FVector2D(Hl.U0, Hl.V0)), Pane.V(G1, N, FVector2D(Hl.U1, Hl.V0)),
						  Pane.V(G1 + V * (Hl.V1 - Hl.V0), N, FVector2D(Hl.U1, Hl.V1)), Pane.V(G0 + V * (Hl.V1 - Hl.V0), N, FVector2D(Hl.U0, Hl.V1)));
				if (bSills && !Hl.bDoor)
				{
					Block(P.Steel, At(0.5 * (Hl.U0 + Hl.U1), Hl.V0 - 0.02) - N * (Depth * 0.5 - 0.04), U, V, N, FVector(HW + 0.03, 0.02, Depth * 0.5 + 0.04));
				}
			}
		}

		/** Rows of openings: Floors floors of Count openings each on a face W wide, each Ww × Wh with its sill at SillZ over the floor. */
		TArray<FHole> Grid(double W, int32 Count, double Ww, double Wh, const TArray<double>& FloorZ, double SillZ, double Margin)
		{
			TArray<FHole> Out;
			for (const double F : FloorZ)
			{
				for (int32 k = 0; k < Count; ++k)
				{
					const double C = Margin + (W - 2 * Margin) * (Count == 1 ? 0.5 : double(k) / (Count - 1));
					Out.Add({C - Ww / 2, F + SillZ, C + Ww / 2, F + SillZ + Wh});
				}
			}
			return Out;
		}

		/** A box's four walls (centre C at its foot, half-sizes HX, HY along the local x and y, height H), each with its openings. */
		void Walls(FMesh& Wall, FParts& P, const FVector& C, double HX, double HY, double Z0, double H, const TArray<FHole> Faces[4], double Depth, double FrameW, bool bSills)
		{
			const FVector X(1, 0, 0), Y(0, 1, 0), Z(0, 0, 1);
			// +y face (looking from +y, u runs −x → +x reversed so the face reads left to right from outside)
			Facade(Wall, P, C + FVector(HX, HY, Z0), -X, Z, Y, 2 * HX, H, Faces[0], Depth, FrameW, bSills);
			Facade(Wall, P, C + FVector(-HX, -HY, Z0), X, Z, -Y, 2 * HX, H, Faces[1], Depth, FrameW, bSills);
			Facade(Wall, P, C + FVector(HX, -HY, Z0), Y, Z, X, 2 * HY, H, Faces[2], Depth, FrameW, bSills);
			Facade(Wall, P, C + FVector(-HX, HY, Z0), -Y, Z, -X, 2 * HY, H, Faces[3], Depth, FrameW, bSills);
		}

		/** A hip roof over a W × D rectangle (centre C, eaves at Z), pitch Deg, overhang O; a fascia under its edge. */
		void HipRoof(FMesh& Roof, FMesh& Fascia, const FVector& C, double W, double D, double Z, double Deg, double O)
		{
			const double HW = W / 2 + O, HD = D / 2 + O;
			const double Rise = HD * FMath::Tan(FMath::DegreesToRadians(Deg));
			const double R = HW - HD;   // half the ridge
			const FVector A(C.X - HW, C.Y - HD, Z), B(C.X + HW, C.Y - HD, Z), Cc(C.X + HW, C.Y + HD, Z), Dd(C.X - HW, C.Y + HD, Z);
			const FVector R0(C.X - R, C.Y, Z + Rise), R1(C.X + R, C.Y, Z + Rise);
			auto Face = [&Roof](const TArray<FVector>& Pts)
			{
				const FVector N = FVector::CrossProduct(Pts[1] - Pts[0], Pts[2] - Pts[0]).GetSafeNormal() * (FVector::CrossProduct(Pts[1] - Pts[0], Pts[2] - Pts[0]).Z < 0 ? -1.0 : 1.0);
				const FVector T = (Pts[1] - Pts[0]).GetSafeNormal();
				TArray<int32> Ids;
				for (const FVector& P : Pts) { Ids.Add(Roof.V(P, N, FVector2D(FVector::DotProduct(P, T), P.Z), FLinearColor::White, FVector2D::ZeroVector, T)); }
				for (int32 i = 1; i + 1 < Ids.Num(); ++i) { Roof.Tri(Ids[0], Ids[i], Ids[i + 1]); }
			};
			Face({A, B, R1, R0});
			Face({Cc, Dd, R0, R1});
			Face({B, Cc, R1});
			Face({Dd, A, R0});
			// The underside of the overhang and the fascia board.
			const FVector Down(0, 0, -1);
			for (const auto& E : TArray<TPair<FVector, FVector>>{{A, B}, {B, Cc}, {Cc, Dd}, {Dd, A}})
			{
				const FVector M = (E.Key + E.Value) * 0.5 + FVector(0, 0, -0.1);
				const FVector Along = (E.Value - E.Key).GetSafeNormal();
				const FVector Out = FVector::CrossProduct(Along, FVector(0, 0, 1)).GetSafeNormal() * ((M - C).Dot(FVector::CrossProduct(Along, FVector(0, 0, 1))) < 0 ? -1.0 : 1.0);
				Block(Fascia, M - Out * (O * 0.5), Along, Out, FVector(0, 0, 1), FVector((E.Value - E.Key).Size() * 0.5, O * 0.5 + 0.03, 0.1));
			}
		}

		/** A flat roof's slab with a parapet (a coping), W × D centred on C at height Z. */
		void FlatRoof(FMesh& Roof, FMesh& Coping, const FVector& C, double W, double D, double Z, double Parapet)
		{
			Block(Roof, C + FVector(0, 0, Z - 0.1), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FVector(W / 2, D / 2, 0.1));
			const double T = 0.25;
			for (const double S : {-1.0, 1.0})
			{
				Block(Coping, C + FVector(S * (W / 2 - T / 2), 0, Z + Parapet / 2), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FVector(T / 2, D / 2, Parapet / 2));
				Block(Coping, C + FVector(0, S * (D / 2 - T / 2), Z + Parapet / 2), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FVector(W / 2, T / 2, Parapet / 2));
			}
		}

		/** A railing along A → B: posts every 1.4 m, a top rail and two lower rails (steel), H high. */
		void Railing(FMesh& Steel, const FVector& A, const FVector& B, double H)
		{
			const FVector D = B - A;
			const int32 N = FMath::Max(1, FMath::CeilToInt32(D.Size() / 1.4));
			for (int32 i = 0; i <= N; ++i) { Rod(Steel, A + D * (double(i) / N), A + D * (double(i) / N) + FVector(0, 0, H), 0.022, 8); }
			for (const double Hh : {H, H * 0.62, H * 0.3}) { Rod(Steel, A + FVector(0, 0, Hh), B + FVector(0, 0, Hh), Hh == H ? 0.024 : 0.012, 8); }
		}

		/** A mountain-hut table with its two benches (larch, weathered), at C, turned by Yaw (radians). */
		void Table(FMesh& Timber, const FVector& C, double Yaw)
		{
			const FVector X(FMath::Cos(Yaw), FMath::Sin(Yaw), 0), Y(-FMath::Sin(Yaw), FMath::Cos(Yaw), 0), Z(0, 0, 1);
			Block(Timber, C + Z * 0.74, X, Y, Z, FVector(1.0, 0.36, 0.025));
			for (const double S : {-1.0, 1.0})
			{
				Block(Timber, C + X * (S * 0.8) + Z * 0.36, X, Y, Z, FVector(0.03, 0.3, 0.36));
				Block(Timber, C + Y * (S * 0.62) + Z * 0.45, X, Y, Z, FVector(1.0, 0.13, 0.022));
				for (const double T : {-0.8, 0.8}) { Block(Timber, C + Y * (S * 0.62) + X * T + Z * 0.22, X, Y, Z, FVector(0.025, 0.1, 0.22)); }
			}
		}
	}

	/**
	 * The Hörnli hut (3,260 m), as it stands since 2015 (SWISSIMAGE 2017; photographs on Commons): the old Hotel
	 * Belvedere (1911), four storeys of cream lime render on a stone plinth under a grey sheet-metal hip roof, its
	 * windows in dark stained larch; the new annex to the south-west, a box clad in anthracite ribbed metal with tall slot
	 * windows and a glazed ground floor, its flat roof covered in solar panels; a low flat-roofed block to the north-east;
	 * the terrace to the south-east on a dry-stone wall, its tables, a railing and the Valais flag.
	 * Frame: x along the ridge's line (north-east), y south-east, z up from the Belvedere's ground floor.
	 */
	void HornliHut(Arch::FParts& P)
	{
		using namespace Arch;
		const TArray<double> Floors = {0.0, 3.05, 6.1, 9.15};
		const double HX = 6.7, HY = 5.95, H = 12.4, Sink = 7.0;
		// The Belvedere: render above a 1.1 m stone plinth; the plinth runs down into the slope.
		{
			TArray<FHole> F[4];
			F[0] = Grid(2 * HX, 5, 1.05, 1.45, {3.05, 6.1, 9.15}, 0.9, 1.3);          // south-east (the terrace)
			F[0].Append({{1.1, 0.25, 2.5, 2.55}, {3.6, 0.25, 5.0, 2.55}, {6.1, 0.0, 7.3, 2.45, true}, {8.4, 0.25, 9.8, 2.55}, {10.9, 0.25, 12.3, 2.55}});
			F[1] = Grid(2 * HX, 5, 1.05, 1.45, Floors, 0.9, 1.3);                        // north-west
			F[2] = Grid(2 * HY, 4, 1.05, 1.45, Floors, 0.9, 1.4);                        // north-east
			F[3] = Grid(2 * HY, 2, 1.05, 1.45, {6.1, 9.15}, 0.9, 3.0);                   // south-west (the annex covers the rest)
			Walls(P.Render, P, FVector(0, 0, 0), HX, HY, 1.1, H - 1.1, F, 0.28, 0.07, true);
			TArray<FHole> None[4];
			Walls(P.Stone, P, FVector(0, 0, 0), HX + 0.06, HY + 0.06, -Sink, Sink + 1.1, None, 0.2, 0.05, false);
			HipRoof(P.Roof, P.Frames, FVector(0, 0, 0), 2 * HX, 2 * HY, H, 23.0, 0.55);
			// Chimneys (render, a metal cap).
			for (const double X : {-2.5, 2.5}) { Block(P.Render, FVector(X, 0.8, H + 2.3), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FVector(0.35, 0.35, 1.2)); }
		}
		// The annex: anthracite ribbed metal; slot windows; its ground floor glazed towards the terrace.
		{
			const FVector C(-13.4, -0.6, 0);
			const double AX = 6.7, AY = 6.6, AH = 11.6;
			TArray<FHole> F[4];
			auto Slots = [](double W, const TArray<double>& Us, const TArray<double>& Zs)
			{
				TArray<FHole> Out;
				for (int32 i = 0; i < Us.Num(); ++i) { Out.Add({Us[i], Zs[i % Zs.Num()], Us[i] + 0.75, Zs[i % Zs.Num()] + 2.3}); }
				return Out;
			};
			F[0] = Slots(2 * AX, {1.2, 3.3, 4.4, 7.0, 9.3, 10.5}, {3.4, 6.5, 9.1});
			F[0].Append({{0.8, 0.2, 12.6, 2.9}});
			F[1] = Slots(2 * AX, {1.0, 2.1, 5.4, 8.1, 11.2}, {3.4, 6.5, 9.1, 0.4});
			F[2] = {};
			F[3] = Slots(2 * AY, {1.3, 4.0, 6.1, 9.7, 11.0}, {0.5, 3.4, 6.5, 9.1});
			Walls(P.Cladding, P, C, AX, AY, 0.0, AH, F, 0.2, 0.06, false);
			TArray<FHole> None[4];
			Walls(P.Stone, P, C, AX + 0.02, AY + 0.02, -Sink, Sink, None, 0.2, 0.05, false);
			FlatRoof(P.Roof, P.Cladding, C, 2 * AX, 2 * AY, AH, 0.35);
			// Solar panels in rows, tilted to the south.
			for (int32 r = 0; r < 6; ++r)
			{
				for (int32 c = 0; c < 5; ++c)
				{
					const FVector PC = C + FVector(-5.2 + c * 2.6, -4.8 + r * 1.9, AH + 0.35);
					const FVector Tilt = FVector(0, 1, 0.35).GetSafeNormal();
					Block(P.Solar, PC, FVector(1, 0, 0), Tilt, FVector::CrossProduct(FVector(1, 0, 0), Tilt), FVector(1.25, 0.8, 0.02));
					Block(P.Steel, PC - FVector(0, 0, 0.18), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FVector(1.2, 0.03, 0.16));
				}
			}
		}
		// The north-east block: one storey, render, flat roof.
		{
			const FVector C(12.3, 1.8, 0);
			TArray<FHole> F[4];
			F[0] = {{1.0, 0.9, 2.4, 2.3}, {5.0, 0.0, 6.2, 2.4, true}};
			F[2] = {{1.5, 0.9, 2.9, 2.3}, {5.5, 0.9, 6.9, 2.3}};
			F[1] = {{2.0, 0.9, 3.2, 2.1}};
			Walls(P.Render, P, C, 5.6, 4.4, 0.0, 3.8, F, 0.25, 0.06, true);
			TArray<FHole> None[4];
			Walls(P.Stone, P, C, 5.62, 4.42, -Sink, Sink, None, 0.2, 0.05, false);
			FlatRoof(P.Roof, P.Render, C, 11.2, 8.8, 3.8, 0.25);
		}
		// The terrace: a timber deck on a dry-stone wall, a railing round its open sides, tables, the flag.
		{
			const double X0 = -9.0, X1 = 5.5, Y0 = HY + 0.2, Y1 = 18.0, Z = -0.35;
			Block(P.Timber, FVector(0.5 * (X0 + X1), 0.5 * (Y0 + Y1), Z - 0.05), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1),
				  FVector(0.5 * (X1 - X0), 0.5 * (Y1 - Y0), 0.05));
			TArray<FHole> None[4];
			Walls(P.Stone, P, FVector(0.5 * (X0 + X1), 0.5 * (Y0 + Y1), 0), 0.5 * (X1 - X0), 0.5 * (Y1 - Y0), -Sink, Sink + Z - 0.1, None, 0.2, 0.05, false);
			Railing(P.Steel, FVector(X0, Y1, Z), FVector(X1, Y1, Z), 1.05);
			Railing(P.Steel, FVector(X1, Y0, Z), FVector(X1, Y1, Z), 1.05);
			Railing(P.Steel, FVector(X0, Y0, Z), FVector(X0, Y1, Z), 1.05);
			FRandomStream R(3260);
			for (int32 i = 0; i < 3; ++i)
			{
				for (int32 j = 0; j < 3; ++j)
				{
					Table(P.Timber, FVector(X0 + 2.4 + i * 4.3 + R.FRandRange(-0.3, 0.3), Y0 + 2.4 + j * 3.6, Z), R.FRandRange(-0.08, 0.08));
				}
			}
			// The flag pole at the terrace's outer corner, the Valais flag (half white, half red) hanging in the wind.
			const FVector Foot(X1 - 0.4, Y1 - 0.4, Z);
			Rod(P.Steel, Foot, Foot + FVector(0, 0, 8.0), 0.04, 10);
			const int32 NU = 16, NV = 10;
			const double FW = 1.8, FH = 1.2;
			TArray<int32> Ids;
			for (int32 j = 0; j <= NV; ++j)
			{
				for (int32 i = 0; i <= NU; ++i)
				{
					const double U = double(i) / NU, Vv = double(j) / NV;
					const FVector Pp = Foot + FVector(0, 0, 8.0 - FH) + FVector(-U * FW * 0.95, 0.18 * U * FMath::Sin(U * 7.0 + Vv * 1.5), Vv * FH - 0.1 * U * U);
					Ids.Add(P.Flag.V(Pp, FVector(0, 1, 0), FVector2D(U, Vv)));
				}
			}
			for (int32 j = 0; j < NV; ++j)
			{
				for (int32 i = 0; i < NU; ++i)
				{
					const int32 A = j * (NU + 1) + i;
					P.Flag.Quad(Ids[A], Ids[A + 1], Ids[A + NU + 2], Ids[A + NU + 1]);
				}
			}
			P.Flag.SmoothNormals(false);
		}
	}

	/**
	 * The Solvay hut (4,003 m), the emergency shelter on the Hörnli ridge (1915; photographs on Commons): a small box of
	 * red-brown painted boards, a door and two small windows on its east side, a shallow metal roof, on a platform of
	 * concrete and stone built out from the crest; wire stays to rock bolts. Frame: x along the crest (north-east), y
	 * south-east (the east face's side), z from its floor.
	 */
	void SolvayHut(Arch::FParts& P)
	{
		using namespace Arch;
		const double HX = 2.3, HY = 1.7, H = 2.7;
		TArray<FHole> F[4];
		F[0] = {{0.5, 0.0, 1.3, 1.9, true}, {2.3, 1.0, 3.0, 1.6}, {3.5, 1.0, 4.2, 1.6}};
		F[2] = {{1.3, 1.1, 1.9, 1.6}};
		Walls(P.Paint, P, FVector(0, 0, 0), HX, HY, 0.0, H, F, 0.08, 0.05, false);
		// The platform: concrete, built out from the crest, down into the rock.
		TArray<FHole> None[4];
		Walls(P.Stone, P, FVector(0, 0.3, 0), HX + 0.5, HY + 0.8, -3.5, 3.5, None, 0.2, 0.05, false);
		Block(P.Stone, FVector(0, 0.3, -0.1), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FVector(HX + 0.5, HY + 0.8, 0.1));
		// The roof, falling to the east a little, its edge overhanging.
		const FVector Tilt = FVector(0, 1, -0.12).GetSafeNormal();
		Block(P.Roof, FVector(0, 0.05, H + 0.12), FVector(1, 0, 0), Tilt, FVector::CrossProduct(FVector(1, 0, 0), Tilt), FVector(HX + 0.25, HY + 0.3, 0.05));
		// Corner posts and the boards' battens (painted with the walls).
		for (const double X : {-HX, HX}) { for (const double Y : {-HY, HY}) { Block(P.Paint, FVector(X, Y, H / 2), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FVector(0.06, 0.06, H / 2)); } }
		// Stays: wire from the roof's corners down to bolts in the rock.
		for (const double X : {-HX, HX}) { for (const double Y : {-HY, HY}) { Rod(P.Steel, FVector(X, Y, H), FVector(X * 1.8, Y * 2.2, -2.5), 0.006, 6, false); } }
		// A ladder on the platform's face and the radio mast.
		for (const double X : {-0.25, 0.25}) { Rod(P.Steel, FVector(X + 1.5, HY + 1.12, -3.0), FVector(X + 1.5, HY + 1.12, 0.0), 0.015, 6); }
		for (double Z = -2.8; Z < 0; Z += 0.3) { Rod(P.Steel, FVector(1.25, HY + 1.12, Z), FVector(1.75, HY + 1.12, Z), 0.012, 6); }
		Rod(P.Steel, FVector(-HX + 0.2, -HY + 0.2, H + 0.1), FVector(-HX + 0.2, -HY + 0.2, H + 2.2), 0.02, 6);
	}
}

namespace JourneyGenKit
{
	/**
	 * A climber (1.75 m) on the ridge, seen from a few hundred metres: legs in dark trousers, a coloured shell jacket, a
	 * rucksack, a helmet, an ice axe; walking up the local +x, a little stooped. Sections: jacket, trousers and pack,
	 * helmet, metal.
	 */
	void Climber(FMesh& Jacket, FMesh& Dark, FMesh& Helmet, FMesh& Metal, double Stride)
	{
		const FVector Hip(0, 0, 0.92);
		for (const double S : {-1.0, 1.0})
		{
			const FVector Foot(S * Stride * 0.25, S * 0.12, 0.05);
			const FVector Knee = (Hip + FVector(0, S * 0.1, 0) + Foot) * 0.5 + FVector(0.08, 0, 0);
			Rod(Dark, Hip + FVector(0, S * 0.1, 0), Knee, 0.085, 10);
			Rod(Dark, Knee, Foot, 0.07, 10);
			Block(Dark, Foot + FVector(0.06, 0, -0.02), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FVector(0.15, 0.06, 0.06));
		}
		// Torso leaning forward, shoulders, arms (one holding the axe).
		const FVector Chest = Hip + FVector(0.1, 0, 0.42);
		Rod(Jacket, Hip + FVector(0, 0, 0.05), Chest, 0.19, 12);
		Rod(Jacket, Chest - FVector(0, 0.2, 0), Chest + FVector(0, 0.2, 0), 0.1, 10);
		for (const double S : {-1.0, 1.0})
		{
			const FVector Sh = Chest + FVector(0, S * 0.22, 0);
			const FVector El = Sh + FVector(0.15, S * 0.05, -0.28);
			const FVector Hand = El + FVector(0.2, 0, -0.15 + (S > 0 ? 0.05 : 0.0));
			Rod(Jacket, Sh, El, 0.06, 8);
			Rod(Jacket, El, Hand, 0.055, 8);
			if (S > 0) { Rod(Metal, Hand + FVector(0, 0, 0.1), Hand + FVector(0.05, 0, -0.6), 0.013, 6); }
		}
		// Rucksack and head with helmet.
		Block(Dark, Chest + FVector(-0.24, 0, -0.1), FVector(1, 0, 0.25).GetSafeNormal(), FVector(0, 1, 0), FVector(-0.25, 0, 1).GetSafeNormal(), FVector(0.12, 0.17, 0.26));
		const FVector Head = Chest + FVector(0.1, 0, 0.3);
		CubeMesh::FMesh Tmp;
		for (int32 i = 0; i < 10; ++i)
		{
			for (int32 j = 0; j < 16; ++j)
			{
				auto P = [&](int32 a, int32 b)
				{
					const double La = -UE_DOUBLE_PI / 2 + UE_DOUBLE_PI * a / 10, Lo = 2 * UE_DOUBLE_PI * b / 16;
					return FVector(FMath::Cos(La) * FMath::Cos(Lo), FMath::Cos(La) * FMath::Sin(Lo), FMath::Sin(La));
				};
				const FVector D00 = P(i, j), D01 = P(i, j + 1), D11 = P(i + 1, j + 1), D10 = P(i + 1, j);
				FMesh& M = i >= 5 ? Helmet : Jacket;
				const double R = i >= 5 ? 0.13 : 0.11;
				M.Quad(M.V(Head + D00 * R, D00), M.V(Head + D01 * R, D01), M.V(Head + D11 * R, D11), M.V(Head + D10 * R, D10));
			}
		}
	}
}

namespace JourneyGenKit
{
	/**
	 * A boulder of gneiss (the Matterhorn's rock, split along its foliation): an icosphere squashed to a slab, cleaved by
	 * a few planes (the flat faces and sharp arrises of broken rock), then weathered by fractal noise. Its foot sinks
	 * 15 % under z = 0. Vertex colour R: how much the face looks up (lichen and dust gather there).
	 */
	/** A lattice beam of four angle chords from A to B, square section W, laced with diagonals on its four faces. */
	void Lattice(FMesh& M, const FVector& A, const FVector& B, const FVector& Side, double W)
	{
		const FVector Ax = (B - A).GetSafeNormal();
		const FVector U = (Side - Ax * FVector::DotProduct(Side, Ax)).GetSafeNormal();
		const FVector V = FVector::CrossProduct(Ax, U);
		const double L = (B - A).Size(), H = W * 0.5;
		const FVector Corner[4] = {U * H + V * H, -U * H + V * H, -U * H - V * H, U * H - V * H};
		for (int32 c = 0; c < 4; ++c)
		{
			CubeMesh::Block(M, (A + B) * 0.5 + Corner[c] * 0.9, Ax, U, V, FVector(L * 0.5, 0.012, 0.012));
		}
		const int32 Bays = FMath::Max(1, FMath::RoundToInt(L / W));
		for (int32 b = 0; b < Bays; ++b)
		{
			const FVector P0 = A + Ax * (L * b / Bays), P1 = A + Ax * (L * (b + 1) / Bays);
			for (int32 c = 0; c < 4; ++c)
			{
				const FVector C0 = Corner[c] * 0.9, C1 = Corner[(c + 1) % 4] * 0.9;
				CubeMesh::Rod(M, P0 + ((b % 2) ? C0 : C1), P1 + ((b % 2) ? C1 : C0), 0.006, 6);
			}
		}
	}

	void Boulder(FMesh& M, int32 Seed, double Size)
	{
		FRandomStream R(Seed);
		// An icosphere, subdivided four times.
		TArray<FVector> V;
		TArray<FIntVector> F;
		const double t = (1.0 + FMath::Sqrt(5.0)) / 2.0;
		for (const FVector& P : {FVector(-1, t, 0), FVector(1, t, 0), FVector(-1, -t, 0), FVector(1, -t, 0), FVector(0, -1, t), FVector(0, 1, t),
								 FVector(0, -1, -t), FVector(0, 1, -t), FVector(t, 0, -1), FVector(t, 0, 1), FVector(-t, 0, -1), FVector(-t, 0, 1)})
		{
			V.Add(P.GetSafeNormal());
		}
		F = {{0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11}, {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
			 {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9}, {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}};
		for (int32 L = 0; L < 4; ++L)
		{
			TMap<uint64, int32> Mid;
			auto MidOf = [&](int32 A, int32 B)
			{
				const uint64 K = (uint64(FMath::Min(A, B)) << 32) | uint64(FMath::Max(A, B));
				if (const int32* Found = Mid.Find(K)) { return *Found; }
				const int32 I = V.Add(((V[A] + V[B]) * 0.5).GetSafeNormal());
				Mid.Add(K, I);
				return I;
			};
			TArray<FIntVector> Next;
			for (const FIntVector& Tr : F)
			{
				const int32 A = MidOf(Tr.X, Tr.Y), B = MidOf(Tr.Y, Tr.Z), C = MidOf(Tr.Z, Tr.X);
				Next.Append({FIntVector(Tr.X, A, C), FIntVector(Tr.Y, B, A), FIntVector(Tr.Z, C, B), FIntVector(A, B, C)});
			}
			F = MoveTemp(Next);
		}
		// Squash, cleave, weather.
		const FVector Scale(Size * R.FRandRange(0.9, 1.1), Size * R.FRandRange(0.6, 0.85), Size * R.FRandRange(0.35, 0.55));
		TArray<FPlane> Cuts;
		for (int32 k = 0; k < 7; ++k)
		{
			const FVector N = FVector(R.FRandRange(-1, 1), R.FRandRange(-1, 1), R.FRandRange(-0.6, 1)).GetSafeNormal();
			Cuts.Add(FPlane(N, R.FRandRange(0.55, 0.85)));
		}
		for (FVector& P : V)
		{
			FVector Q = P;
			for (const FPlane& C : Cuts)
			{
				const double D = FVector::DotProduct(Q, FVector(C.X, C.Y, C.Z)) - C.W;
				if (D > 0) { Q -= FVector(C.X, C.Y, C.Z) * D; }
			}
			const double N1 = Fbm(Q * 1.0, 1.7, 4, Seed);
			Q *= 1.0 + 0.12 * N1;
			P = FVector(Q.X * Scale.X, Q.Y * Scale.Y, Q.Z * Scale.Z - Scale.Z * 0.15);
		}
		const int32 Base = M.Positions.Num();
		for (const FVector& P : V)
		{
			const FVector N = P.GetSafeNormal();
			M.V(P, N, FVector2D(P.X + P.Z, P.Y + P.Z), FLinearColor(float(FMath::Clamp(N.Z, 0.0, 1.0)), float(R.FRand()), 0.f, 1.f));
		}
		for (const FIntVector& Tr : F) { M.Tri(Base + Tr.X, Base + Tr.Y, Base + Tr.Z); }
	}
}

namespace JourneyGen
{
	using namespace JourneyGenKit;

	/**
	 * II · Sea Light's cells (Cube/SeaLight.h): 262,144 tiny quads (4 mm, so no build drops them), each a cell of the
	 * sea at rest round the car, densest in the reach and thinning out to 6 m (the field's reach), none above the surface
	 * (4.5 m over the eyes). UV0: the quad's corner (±0.5); UV1: the cell's two randoms. The material places and lights them.
	 */
	static void SeaLightCards(TArray<FJourneyThing>& Out)
	{
		FJourneyThing T;
		T.Name = TEXT("SM_SeaLight_Cards");
		T.Material = TEXT("M_SeaLight_Cells");
		T.bNanite = false;
		T.bCastShadow = false;
		CubeMesh::FMesh& M = T.Mesh;
		constexpr int32 Count = 262144;
		M.Positions.Reserve(Count * 4);
		M.Normals.Reserve(Count * 4);
		M.UV0.Reserve(Count * 4);
		M.UV1.Reserve(Count * 4);
		M.Colours.Reserve(Count * 4);
		M.Tangents.Reserve(Count * 4);
		M.Indices.Reserve(Count * 6);
		FRandomStream R(1843);
		const double S = 0.002;
		for (int32 i = 0; i < Count; ++i)
		{
			// Within the field (6 m round the eyes), densest in the reach; the four copies are mirrored in z, so the
			// layer is kept symmetric and under the surface (4.5 m up).
			const double Rr = 0.3 + 5.6 * FMath::Pow(R.FRand(), 1.35);
			FVector P = R.GetUnitVector() * Rr;
			P.X = FMath::Clamp(P.X, -5.9, 5.9);
			P.Y = FMath::Clamp(P.Y, -5.9, 5.9);
			P.Z = FMath::Clamp(P.Z, -4.4, 4.4);
			const FVector2D Rand(R.FRand(), R.FRand());
			const int32 A = M.V(P + FVector(-S, -S, 0), FVector::UpVector, FVector2D(-0.5, -0.5), FLinearColor::White, Rand, FVector::ForwardVector);
			M.V(P + FVector(S, -S, 0), FVector::UpVector, FVector2D(0.5, -0.5), FLinearColor::White, Rand, FVector::ForwardVector);
			M.V(P + FVector(S, S, 0), FVector::UpVector, FVector2D(0.5, 0.5), FLinearColor::White, Rand, FVector::ForwardVector);
			M.V(P + FVector(-S, S, 0), FVector::UpVector, FVector2D(-0.5, 0.5), FLinearColor::White, Rand, FVector::ForwardVector);
			M.Indices.Append({A, A + 2, A + 1, A, A + 3, A + 2});
		}
		Out.Add(MoveTemp(T));
	}

	/**
	 * Sea Light's scad (SM_SeaLight_Fish): a fish 30 cm long, head along +x, a spindle body (deeper than wide) and a
	 * forked tail. It is drawn only by the light on its skin (M_SeaLight_Fish), so the outline matters, not the detail.
	 */
	static void SeaLightFish(TArray<FJourneyThing>& Out)
	{
		FJourneyThing T;
		T.Name = TEXT("SM_SeaLight_Fish");
		T.Material = TEXT("M_SeaLight_Fish");
		T.bNanite = false;
		T.bCastShadow = false;
		CubeMesh::FMesh& M = T.Mesh;
		constexpr int32 Rings = 14, Round = 10;
		const double X0 = -0.12, X1 = 0.16;
		TArray<int32> Prev;
		for (int32 i = 0; i <= Rings; ++i)
		{
			const double u = double(i) / Rings;                // 0 at the tail's root, 1 at the snout
			const double X = FMath::Lerp(X0, X1, u);
			const double Shape = FMath::Pow(FMath::Sin(PI * FMath::Lerp(0.06, 1.0, u)), 0.7) * (u > 0.85 ? FMath::Sqrt(FMath::Max(0.0, (1.0 - u) / 0.15)) : 1.0);
			const double Hh = 0.034 * Shape + 0.004, Hw = 0.016 * Shape + 0.003;
			TArray<int32> Ring;
			for (int32 k = 0; k < Round; ++k)
			{
				const double A = 2.0 * PI * k / Round;
				const FVector N(0.0, FMath::Cos(A) / Hw, FMath::Sin(A) / Hh);
				Ring.Add(M.V(FVector(X, Hw * FMath::Cos(A), Hh * FMath::Sin(A)), N.GetSafeNormal(), FVector2D(u, double(k) / Round)));
			}
			if (Prev.Num())
			{
				for (int32 k = 0; k < Round; ++k) { M.Quad(Prev[k], Prev[(k + 1) % Round], Ring[(k + 1) % Round], Ring[k]); }
			}
			Prev = Ring;
		}
		// The forked tail: two thin lobes, both faces.
		for (const double S : {-1.0, 1.0})
		{
			const FVector Root(X0 + 0.005, 0.0, 0.0), Tip(X0 - 0.075, 0.0, 0.055 * S), Notch(X0 - 0.035, 0.0, 0.012 * S);
			for (const double Side : {-1.0, 1.0})
			{
				const FVector N(0.0, Side, 0.0);
				const int32 A = M.V(Root + FVector(0, 0.001 * Side, 0), N, FVector2D(0.0, 0.5));
				const int32 B = M.V(Tip + FVector(0, 0.001 * Side, 0), N, FVector2D(-0.3, 0.5));
				const int32 C = M.V(Notch + FVector(0, 0.001 * Side, 0), N, FVector2D(-0.15, 0.5));
				M.Tri(A, B, C);
			}
		}
		Out.Add(MoveTemp(T));
	}

	/**
	 * Sea Light's reef manta (SM_SeaLight_Manta): 4.5 m across (x forward, y to the right wing tip at ±2.25), a flat body
	 * 0.35 m deep at the middle thinning to the wing edges, the head's lobes and a thin tail. M_SeaLight_Manta beats its
	 * wings (WPO) and keeps it dark.
	 */
	static void SeaLightManta(TArray<FJourneyThing>& Out)
	{
		FJourneyThing T;
		T.Name = TEXT("SM_SeaLight_Manta");
		T.Material = TEXT("M_SeaLight_Manta");
		T.bNanite = false;
		T.bCastShadow = false;
		CubeMesh::FMesh& M = T.Mesh;
		const TArray<FVector2D> Outline = {
			{1.0, 0.3}, {1.1, 0.14}, {0.95, 0.0}, {1.1, -0.14}, {1.0, -0.3},
			{0.62, -0.95}, {0.18, -1.85}, {-0.08, -2.25}, {-0.35, -1.6}, {-0.7, -0.6},
			{-0.95, -0.18}, {-2.6, -0.02}, {-2.6, 0.02}, {-0.95, 0.18},
			{-0.7, 0.6}, {-0.35, 1.6}, {-0.08, 2.25}, {0.18, 1.85}, {0.62, 0.95}};
		// Each outline point subdivided towards the middle in rings, so the wings can bend.
		constexpr int32 Steps = 6;
		for (const double Face : {1.0, -1.0})
		{
			const int32 Centre = M.V(FVector(0.05, 0.0, Face * (Face > 0 ? 0.2 : 0.14)), FVector(0, 0, Face), FVector2D(0.5, 0.5), FLinearColor(0.f, 0.f, 0.f, 1.f));
			TArray<TArray<int32>> Spokes;
			// Vertex colour G: the leading edges and tips (1), the head's lobes (0.4), the trailing edges (0).
			static const float Lead[] = {0.4f, 0.4f, 0.4f, 0.4f, 0.4f, 1.f, 1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f};
			for (int32 o = 0; o < Outline.Num(); ++o)
			{
				const FVector2D& Q = Outline[o];
				TArray<int32> Spoke;
				for (int32 s = 1; s <= Steps; ++s)
				{
					const double f = double(s) / Steps;
					const FVector2D P = FVector2D(0.05, 0.0) + (Q - FVector2D(0.05, 0.0)) * f;
					const double Thick = (Face > 0 ? 0.2 : 0.14) * (1.0 - f * f) + 0.006;
					// Vertex colour R: how near the edge (the material lights the edge by the sparks it stirs).
					Spoke.Add(M.V(FVector(P.X, P.Y, Face * Thick), FVector(0, 0, Face), FVector2D(0.5 + P.X / 5.0, 0.5 + P.Y / 5.0), FLinearColor(float(f), Lead[o], 0.f, 1.f)));
				}
				Spokes.Add(Spoke);
			}
			const int32 NO = Outline.Num();
			for (int32 i = 0; i < NO; ++i)
			{
				const TArray<int32>& A = Spokes[i];
				const TArray<int32>& B = Spokes[(i + 1) % NO];
				M.Tri(Centre, A[0], B[0]);
				for (int32 s = 0; s + 1 < Steps; ++s) { M.Quad(A[s], A[s + 1], B[s + 1], B[s]); }
			}
		}
		Out.Add(MoveTemp(T));
	}

	/** A thing of several sections from a building's parts (empty ones left out). */
	static void AddBuilding(TArray<FJourneyThing>& Out, const FString& Name, Arch::FParts& P)
	{
		FJourneyThing T;
		T.Name = Name;
		const TPair<CubeMesh::FMesh*, const TCHAR*> Parts[] = {
			{&P.Render, TEXT("MI_MH_Render")}, {&P.Cladding, TEXT("MI_MH_Cladding")}, {&P.Frames, TEXT("MI_MH_Frame")}, {&P.Glass, TEXT("MI_MH_WindowGlass")},
			{&P.Roof, TEXT("MI_MH_RoofMetal")}, {&P.Stone, TEXT("MI_MH_StoneWall")}, {&P.Timber, TEXT("MI_MH_Timber")}, {&P.Steel, TEXT("MI_MH_Steel")},
			{&P.Solar, TEXT("MI_MH_Solar")}, {&P.Flag, TEXT("MI_MH_Flag")}, {&P.Paint, TEXT("MI_MH_Paint")}};
		for (const auto& Part : Parts)
		{
			if (Part.Key->Positions.Num() == 0) { continue; }
			T.Sections.Add(MoveTemp(*Part.Key));
			T.Materials.Add(Part.Value);
		}
		Out.Add(MoveTemp(T));
	}

	void Build(const FString& Set, TArray<FJourneyThing>& Out)
	{
		if (Set == TEXT("sea")) { SeaGen::Build(Out); return; }
		if (Set == TEXT("sealight")) { SeaLightCards(Out); SeaLightFish(Out); SeaLightManta(Out); return; }
		if (Set == TEXT("matterhorn"))
		{
			FRandomStream R(1865);
			{
				FJourneyThing T;
				T.Name = TEXT("SM_MH_Crevasse_Walls");
				T.Material = TEXT("MI_MH_Ice");
				Crevasse::Wall(T.Mesh, -1.0);
				Crevasse::Wall(T.Mesh, 1.0);
				Crevasse::Icicles(T.Mesh, R);
				T.Mesh.SmoothNormals(false);
				Out.Add(MoveTemp(T));
			}
			{
				FJourneyThing T;
				T.Name = TEXT("SM_MH_Crevasse_Snow");
				T.Material = TEXT("MI_MH_Snow");
				Crevasse::Surface(T.Mesh, -1.0);
				Crevasse::Surface(T.Mesh, 1.0);
				Crevasse::Bridge(T.Mesh, 19.0, 11.5, 1.4);
				Crevasse::Bridge(T.Mesh, -27.0, 13.0, 1.8);
				T.Mesh.SmoothNormals(false);
				Out.Add(MoveTemp(T));
			}
			{
				Arch::FParts P;
				HornliHut(P);
				AddBuilding(Out, TEXT("SM_MH_HornliHut"), P);
			}
			{
				Arch::FParts P;
				SolvayHut(P);
				AddBuilding(Out, TEXT("SM_MH_SolvayHut"), P);
			}
			// Boulders, six shapes (scaled when placed).
			for (int32 k = 0; k < 6; ++k)
			{
				FJourneyThing T;
				T.Name = FString::Printf(TEXT("SM_MH_Boulder%d"), k);
				T.Material = TEXT("MI_MH_Boulder");
				Boulder(T.Mesh, 4478 + k * 17, 1.0);
				T.Mesh.SmoothNormals(false);
				Out.Add(MoveTemp(T));
			}
			// The iron cross on the Italian summit (2.6 m, lattice beams), its foot in a cairn of the boulders' stones.
			{
				FJourneyThing T;
				T.Name = TEXT("SM_MH_SummitCross");
				T.Material = TEXT("MI_MH_Steel");
				Lattice(T.Mesh, FVector(0, 0, -0.4), FVector(0, 0, 2.6), FVector(1, 0, 0), 0.16);
				Lattice(T.Mesh, FVector(-0.75, 0, 1.85), FVector(0.75, 0, 1.85), FVector(0, 0, 1), 0.14);
				Out.Add(MoveTemp(T));
			}
			// Climbers in three jackets.
			const TCHAR* Jackets[] = {TEXT("MI_MH_JacketRed"), TEXT("MI_MH_JacketBlue"), TEXT("MI_MH_JacketOrange")};
			for (int32 k = 0; k < 3; ++k)
			{
				FJourneyThing T;
				T.Name = FString::Printf(TEXT("SM_MH_Climber%d"), k);
				T.Sections.SetNum(4);
				Climber(T.Sections[0], T.Sections[1], T.Sections[2], T.Sections[3], 0.3 + 0.2 * k);
				T.Materials = {Jackets[k], TEXT("MI_MH_Clothes"), TEXT("MI_MH_Helmet"), TEXT("MI_MH_Steel")};
				Out.Add(MoveTemp(T));
			}
		}
	}
}
