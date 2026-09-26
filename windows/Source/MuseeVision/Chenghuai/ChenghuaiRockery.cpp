#include "Chenghuai/ChenghuaiRockery.h"

#include "Async/ParallelFor.h"
#include "Chenghuai/ChenghuaiGardenPlan.h"
#include "Chenghuai/ChenghuaiHall.h"
#include "Chenghuai/ChenghuaiPlan.h"
#include "Chenghuai/ChenghuaiRoof.h"
#include "Nature/NatureMesh.h"

namespace ChenghuaiRockery
{
	namespace Kit = ChenghuaiKit;
	namespace CB = ChenghuaiBuild;
	namespace CH = Chenghuai;
	namespace GP = ChenghuaiGardenPlan;
	namespace MN = MuseeNature;
	using Kit::FMeshData;
	using Kit::FProfile;

	namespace RockImpl
	{
		constexpr double Course = 0.3;          // the stones' courses
		constexpr double Summit = 2.6;          // the plateau under the pavilion
		constexpr double PathHalf = 0.55;
		constexpr double Riser = 0.15;

		TArray<FVector2D> Poly(const double (*Pts)[2], int32 N)
		{
			TArray<FVector2D> Out;
			for (int32 i = 0; i < N; ++i) { Out.Add(FVector2D(Pts[i][0], Pts[i][1])); }
			return Out;
		}

		/** Signed distance to a closed outline (negative inside). */
		double SignedDistance(const TArray<FVector2D>& P, const FVector2D& Q)
		{
			bool bIn = false;
			double Best = TNumericLimits<double>::Max();
			for (int32 i = 0, j = P.Num() - 1; i < P.Num(); j = i++)
			{
				const FVector2D& A = P[i];
				const FVector2D& B = P[j];
				if (((A.Y > Q.Y) != (B.Y > Q.Y)) && (Q.X < (B.X - A.X) * (Q.Y - A.Y) / (B.Y - A.Y) + A.X)) { bIn = !bIn; }
				const FVector2D AB = B - A;
				const double T = FMath::Clamp(FVector2D::DotProduct(Q - A, AB) / FMath::Max(AB.SizeSquared(), 1e-12), 0.0, 1.0);
				Best = FMath::Min(Best, (A + AB * T - Q).Size());
			}
			return bIn ? -Best : Best;
		}

		struct FPathInfo
		{
			TArray<FVector2D> P;
			TArray<double> S;
			double SummitS = 0.0;
			double Total = 0.0;
		};

		const FPathInfo& Path()
		{
			static const FPathInfo Info = []()
			{
				FPathInfo I;
				I.P = Poly(GP::RockeryPath, GP::RockeryPathCount);
				I.S.Add(0.0);
				double Best = TNumericLimits<double>::Max();
				for (int32 k = 1; k < I.P.Num(); ++k) { I.S.Add(I.S.Last() + FVector2D::Distance(I.P[k], I.P[k - 1])); }
				for (int32 k = 0; k < I.P.Num(); ++k)
				{
					const double D = FVector2D::Distance(I.P[k], FVector2D(CH::HexCX, CH::HexCY));
					if (D < Best) { Best = D; I.SummitS = I.S[k]; }
				}
				I.Total = I.S.Last();
				return I;
			}();
			return Info;
		}

		/** Nearest point of the path: its distance and arc length. */
		void NearestOnPath(const FVector2D& Q, double& OutDist, double& OutS)
		{
			const FPathInfo& I = Path();
			OutDist = TNumericLimits<double>::Max();
			OutS = 0.0;
			for (int32 k = 0; k + 1 < I.P.Num(); ++k)
			{
				const FVector2D A = I.P[k], AB = I.P[k + 1] - A;
				const double T = FMath::Clamp(FVector2D::DotProduct(Q - A, AB) / FMath::Max(AB.SizeSquared(), 1e-12), 0.0, 1.0);
				const double D = (A + AB * T - Q).Size();
				if (D < OutDist) { OutDist = D; OutS = I.S[k] + T * (I.S[k + 1] - I.S[k]); }
			}
		}

		/** The path's walking height at arc length S: up in steps to the summit, down again gently. */
		double PathHeight(double S)
		{
			const FPathInfo& I = Path();
			const double Up = FMath::Clamp(S / FMath::Max(I.SummitS - 1.9, 0.1), 0.0, 1.0);
			const double Down = FMath::Clamp((I.Total - S) / FMath::Max(I.Total - I.SummitS - 1.9, 0.1), 0.0, 1.0);
			const double Z = Summit * FMath::Min(Up, Down) + 0.03;
			return FMath::Floor(Z / Riser + 0.5) * Riser + 0.02;
		}

		/** The rockery's massing: rising from its foot to its upper outline and the summit (plan metres, height over the ground). */
		double Envelope(const FVector2D& Q, double& OutDF)
		{
			static const TArray<FVector2D> Foot = Poly(GP::RockeryFoot, GP::RockeryFootCount);
			static const TArray<FVector2D> Upper = Poly(GP::RockeryUpper, GP::RockeryUpperCount);
			OutDF = SignedDistance(Foot, Q);
			const double DU = SignedDistance(Upper, Q);
			const double RP = FVector2D::Distance(Q, FVector2D(CH::HexCX, CH::HexCY));
			return 1.2 * FMath::SmoothStep(-0.2, 1.8, -OutDF) + 1.05 * FMath::SmoothStep(-0.4, 1.3, -DU) + 0.5 * FMath::SmoothStep(0.0, 1.4, 3.2 - RP);
		}

		/**
		 * The natural mass (before the path is cut), as a Suzhou rockery is heaped: stones about a metre across (a jittered
		 * Voronoi of them), each set at its own height over the massing (-0.25 ... +0.3 m) and leaning its own way (up to
		 * 9 degrees), its top rounded off towards its edges and a dark crevice between it and the next; so the flanks break
		 * into ledges, overhangs and clefts of different heights instead of the rings of a stepped cake (the courses of
		 * the first build).
		 */
		double Natural(const FVector2D& Q)
		{
			double DF = 0.0;
			const double H = Envelope(Q, DF);
			if (DF > 0.3) { return -0.3; }
			const double Cell = 0.95;
			const int32 CI = FMath::FloorToInt32(Q.X / Cell), CJ = FMath::FloorToInt32(Q.Y / Cell);
			double D1 = TNumericLimits<double>::Max(), D2 = TNumericLimits<double>::Max();
			int32 BI = 0, BJ = 0;
			FVector2D C1 = Q;
			for (int32 di = -2; di <= 2; ++di)
			{
				for (int32 dj = -2; dj <= 2; ++dj)
				{
					const int32 I = CI + di, J = CJ + dj;
					const FVector2D C((I + 0.1 + 0.8 * Kit::Hash01(I, J * 7 + 1)) * Cell, (J + 0.1 + 0.8 * Kit::Hash01(I * 3 + 5, J)) * Cell);
					const double D = FVector2D::DistSquared(C, Q);
					if (D < D1) { D2 = D1; D1 = D; BI = I; BJ = J; C1 = C; }
					else if (D < D2) { D2 = D; }
				}
			}
			const double Edge = 0.5 * (FMath::Sqrt(D2) - FMath::Sqrt(D1));   // to the stone's boundary
			double DFc = 0.0;
			const double Hc = Envelope(C1, DFc);
			const double Lift = (Kit::Hash01(BI * 11 + 3, BJ * 5 + 9) - 0.45) * 0.55;
			const FVector2D Lean((Kit::Hash01(BI, BJ + 13) - 0.5) * 0.3, (Kit::Hash01(BI + 17, BJ) - 0.5) * 0.3);
			const double Top = Hc + Lift + FVector2D::DotProduct(Lean, Q - C1) - 0.16 * (1.0 - FMath::SmoothStep(0.0, 0.24, Edge));
			// At the foot the stones settle into the ground (none floats); over the mass they stand on their own.
			const double S = FMath::Lerp(H, Top, FMath::SmoothStep(0.05, 0.45, H));
			return FMath::Max(S, H * 0.2) - 0.06 * FMath::Max(0.0, DF + 0.2);
		}

		/** The surface's height: the mass with the path's ledge cut into it and the summit levelled for the pavilion. */
		double Height(const FVector2D& Q)
		{
			const double RP = FVector2D::Distance(Q, FVector2D(CH::HexCX, CH::HexCY));
			double H = Natural(Q);
			if (RP < 2.05) { return Summit; }
			if (RP < 2.6) { H = FMath::Lerp(Summit, H, (RP - 2.05) / 0.55); }
			double D = 0.0, S = 0.0;
			NearestOnPath(Q, D, S);
			if (D < PathHalf + 0.4)
			{
				const double Z = PathHeight(S);
				if (D < PathHalf) { return Z; }
				const double T = (D - PathHalf) / 0.4;
				// Beside the path the stone stands at least a kerb's height over it (a low edge), or falls away.
				H = FMath::Lerp(FMath::Max(Z + 0.2, H), H, T * T);
			}
			return H;
		}
	}
	using namespace RockImpl;

	double RockeryHeight(const FVector2D& P) { return FMath::Max(0.0, Height(P)); }

	void EdgeRocks(FPart& Part, const TArray<FVector2D>& Edge, double Ground, double Water)
	{
		const int32 N = Edge.Num();
		double Carry = 0.0;
		int32 Id = 0;
		for (int32 i = 0; i < N; ++i)
		{
			const FVector2D A = Edge[i], B = Edge[(i + 1) % N];
			const double Len = FVector2D::Distance(A, B);
			double S = Carry;
			while (S < Len)
			{
				const FVector2D P = A + (B - A) * (S / Len);
				const FVector2D Along = (B - A) / Len;
				// Two stacked stones: a broad one at the water, a smaller one over it, each an irregular outline.
				for (int32 Layer = 0; Layer < 2; ++Layer)
				{
					const double R = (Layer == 0 ? 0.42 : 0.3) * (0.8 + 0.4 * Kit::Hash01(Id, 1 + Layer));
					TArray<FVector2D> Outline;
					const int32 NS = 7;
					const double Rot = Kit::Hash01(Id, 9 + Layer) * 6.28;
					for (int32 k = 0; k < NS; ++k)
					{
						const double T = Rot + 6.2831853 * k / NS;
						const double Rr = R * (0.7 + 0.45 * Kit::Hash01(Id * 13 + k, 3 + Layer));
						Outline.Add(P + FVector2D(FMath::Cos(T) * Rr * 1.3, FMath::Sin(T) * Rr) .GetRotated(FMath::RadiansToDegrees(FMath::Atan2(Along.Y, Along.X))));
					}
					const double Z0 = Layer == 0 ? Water - 0.35 : Ground - 0.05;
					const double Z1 = Layer == 0 ? Ground - 0.02 + 0.08 * Kit::Hash01(Id, 5) : Ground + 0.1 + 0.3 * Kit::Hash01(Id, 7);
					if (Layer == 1 && Kit::Hash01(Id, 11) < 0.45) { continue; }   // not every stone has a second
					Kit::Prism(Part, TEXT("Edge rock"), Outline, Z0, Z1, true);
				}
				++Id;
				S += 0.6 + 0.35 * Kit::Hash01(Id, 2);
			}
			Carry = S - Len;
		}
	}

	namespace RockMesh
	{
		/** The Taihu stones' hollows: spheres that the water wore out of the flanks, 0.25-0.45 m. */
		struct FHole { FVector C; double R; };
		/** Standing peaks set on the upper mass: tall, waisted, leaning stones 1.3-2.1 m high. */
		struct FPeak { FVector2D C; double Base, Tall, R, Squash, Turn; int32 Seed; };

		const TArray<FHole>& Holes()
		{
			static const TArray<FHole> Out = []()
			{
				TArray<FHole> H;
				const TArray<FVector2D> Foot = Poly(GP::RockeryFoot, GP::RockeryFootCount);
				for (int32 k = 0; k < 1400 && H.Num() < 110; ++k)
				{
					const FVector2D Q(4.9 + 11.0 * Kit::Hash01(k, 401), -36.6 + 16.6 * Kit::Hash01(k, 402));
					const double DF = SignedDistance(Foot, Q);
					if (DF > -0.25 || DF < -2.6) { continue; }
					double D = 0.0, S = 0.0;
					NearestOnPath(Q, D, S);
					if (D < PathHalf + 0.7 || FVector2D::Distance(Q, FVector2D(CH::HexCX, CH::HexCY)) < 3.0) { continue; }
					const double Z = Height(Q);
					if (Z < 0.45) { continue; }
					const double R = 0.26 + 0.26 * Kit::Hash01(k, 403);
					H.Add({FVector(Q.X, Q.Y, Z * (0.35 + 0.45 * Kit::Hash01(k, 404))), R});
				}
				return H;
			}();
			return Out;
		}

		const TArray<FPeak>& Peaks()
		{
			static const TArray<FPeak> Out = []()
			{
				TArray<FPeak> P;
				const TArray<FVector2D> Foot = Poly(GP::RockeryFoot, GP::RockeryFootCount);
				for (int32 k = 0; k < 600 && P.Num() < 6; ++k)
				{
					const FVector2D Q(5.2 + 10.4 * Kit::Hash01(k, 501), -36.2 + 15.8 * Kit::Hash01(k, 502));
					const double DF = SignedDistance(Foot, Q);
					if (DF > -0.9 || DF < -2.8) { continue; }
					double D = 0.0, S = 0.0;
					NearestOnPath(Q, D, S);
					if (D < PathHalf + 0.95 || FVector2D::Distance(Q, FVector2D(CH::HexCX, CH::HexCY)) < 3.3) { continue; }
					bool bNear = false;
					for (const FPeak& O : P) { bNear |= FVector2D::Distance(O.C, Q) < 2.6; }
					if (bNear) { continue; }
					const double Z = Height(Q);
					P.Add({Q, Z - 0.3, 1.3 + 0.8 * Kit::Hash01(k, 503), 0.36 + 0.2 * Kit::Hash01(k, 504), 0.62 + 0.25 * Kit::Hash01(k, 505),
						   6.28 * Kit::Hash01(k, 506), k});
				}
				return P;
			}();
			return Out;
		}

		/** A peak's signed distance: an ellipse in plan, narrowing and waisting as it rises, leaning a little. */
		double PeakDistance(const FPeak& K, const FVector& P)
		{
			const double Z = P.Z - K.Base;
			const double T = FMath::Clamp(Z / K.Tall, 0.0, 1.0);
			const FVector2D Lean(0.25 * FMath::Cos(K.Turn * 1.7), 0.25 * FMath::Sin(K.Turn * 1.7));
			const FVector2D Rel = FVector2D(P.X, P.Y) - K.C - Lean * T;
			const double CT = FMath::Cos(K.Turn), ST = FMath::Sin(K.Turn);
			const FVector2D L(Rel.X * CT + Rel.Y * ST, (-Rel.X * ST + Rel.Y * CT) / K.Squash);
			const double R = K.R * (1.0 - 0.38 * T) * (1.0 + 0.2 * FMath::Sin(9.0 * T + K.Seed));
			const double Side = (L.Size() - R) * K.Squash;
			return FMath::Max(FMath::Max(Side, Z - K.Tall), -Z - 0.25);
		}

		/** Surface nets over the height field, with a stone's roughness on its faces. */
		void BuildMass(FMeshData& M)
		{
			const double Voxel = 0.1;
			const FVector Lo(4.6, -36.8, -0.25), Hi(16.2, -19.7, Summit + 2.3);
			const int32 NX = FMath::CeilToInt32((Hi.X - Lo.X) / Voxel) + 1;
			const int32 NY = FMath::CeilToInt32((Hi.Y - Lo.Y) / Voxel) + 1;
			const int32 NZ = FMath::CeilToInt32((Hi.Z - Lo.Z) / Voxel) + 1;
			// The height field on the plan grid first (the costly part), then the 3D field from it.
			TArray<double> HF;
			HF.SetNumUninitialized(NX * NY);
			ParallelFor(NY, [&](int32 J)
			{
				for (int32 I = 0; I < NX; ++I) { HF[J * NX + I] = Height(FVector2D(Lo.X + I * Voxel, Lo.Y + J * Voxel)); }
			});
			auto HAt = [&](double X, double Y)
			{
				const double FX = FMath::Clamp((X - Lo.X) / Voxel, 0.0, NX - 1.001), FY = FMath::Clamp((Y - Lo.Y) / Voxel, 0.0, NY - 1.001);
				const int32 I = FMath::FloorToInt32(FX), J = FMath::FloorToInt32(FY);
				const double TX = FX - I, TY = FY - J;
				return FMath::Lerp(FMath::Lerp(HF[J * NX + I], HF[J * NX + I + 1], TX), FMath::Lerp(HF[(J + 1) * NX + I], HF[(J + 1) * NX + I + 1], TX), TY);
			};
			const TArray<FHole>& HoleList = Holes();
			const TArray<FPeak>& PeakList = Peaks();
			auto Field = [&](const FVector& P)
			{
				const double H = HAt(P.X, P.Y);
				double F = P.Z - H;
				for (const FPeak& K : PeakList)
				{
					if (FVector2D::DistSquared(FVector2D(P.X, P.Y), K.C) < 1.6) { F = FMath::Min(F, PeakDistance(K, P)); }
				}
				for (const FHole& O : HoleList)
				{
					const double D = FVector::Dist(P, O.C);
					if (D < O.R + 0.3) { F = FMath::Max(F, O.R - D + 0.05 * (MN::Noise(P * 6.0, 79) - 0.5)); }
				}
				// Rough faces: the stone's lumps and the water's vertical wrinkles.
				const double Rough = 0.07 * (MN::Fbm(P * 2.2, 3, 71) - 0.5) + 0.025 * (MN::Noise(P * 7.0, 73) - 0.5)
					+ 0.055 * (MN::Noise(FVector(P.X * 5.0, P.Y * 5.0, P.Z * 1.2), 77) - 0.5);
				return FMath::Max(F + Rough, -(P.Z + 0.2));
			};
			auto Idx = [NX, NY](int32 I, int32 J, int32 K) { return (K * NY + J) * NX + I; };
			auto At = [&](int32 I, int32 J, int32 K) { return Lo + FVector(I, J, K) * Voxel; };
			TArray<float> Values;
			Values.SetNumUninitialized(NX * NY * NZ);
			ParallelFor(NZ, [&](int32 K)
			{
				for (int32 J = 0; J < NY; ++J) { for (int32 I = 0; I < NX; ++I) { Values[Idx(I, J, K)] = float(Field(At(I, J, K))); } }
			});
			const int32 Corner[8][3] = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}, {0, 0, 1}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1}};
			const int32 Edge[12][2] = {{0, 1}, {2, 3}, {4, 5}, {6, 7}, {0, 2}, {1, 3}, {4, 6}, {5, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
			TArray<int32> CellVertex;
			CellVertex.Init(INDEX_NONE, NX * NY * NZ);
			const double G = Voxel * 0.5;
			for (int32 K = 0; K + 1 < NZ; ++K)
			{
				for (int32 J = 0; J + 1 < NY; ++J)
				{
					for (int32 I = 0; I + 1 < NX; ++I)
					{
						float V[8];
						int32 Inside = 0;
						for (int32 c = 0; c < 8; ++c)
						{
							V[c] = Values[Idx(I + Corner[c][0], J + Corner[c][1], K + Corner[c][2])];
							Inside += V[c] < 0 ? 1 : 0;
						}
						if (Inside == 0 || Inside == 8) { continue; }
						FVector Sum = FVector::ZeroVector;
						int32 Count = 0;
						for (int32 e = 0; e < 12; ++e)
						{
							const float A = V[Edge[e][0]], B = V[Edge[e][1]];
							if ((A < 0) == (B < 0)) { continue; }
							const double T = A / (A - B);
							Sum += FMath::Lerp(At(I + Corner[Edge[e][0]][0], J + Corner[Edge[e][0]][1], K + Corner[Edge[e][0]][2]),
											   At(I + Corner[Edge[e][1]][0], J + Corner[Edge[e][1]][1], K + Corner[Edge[e][1]][2]), T);
							++Count;
						}
						const FVector P = Sum / FMath::Max(1, Count);
						const FVector N = FVector(Field(P + FVector(G, 0, 0)) - Field(P - FVector(G, 0, 0)), Field(P + FVector(0, G, 0)) - Field(P - FVector(0, G, 0)),
												  Field(P + FVector(0, 0, G)) - Field(P - FVector(0, 0, G))).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
						// Metres (the stone's material projects by world position; UV0 for the rest).
						CellVertex[Idx(I, J, K)] = M.Vertex(P, N, FVector2D(P.X + 0.3 * P.Z, P.Y + 0.7 * P.Z));
					}
				}
			}
			auto Cell = [&](int32 I, int32 J, int32 K) { return (I < 0 || J < 0 || K < 0 || I >= NX - 1 || J >= NY - 1 || K >= NZ - 1) ? INDEX_NONE : CellVertex[Idx(I, J, K)]; };
			auto Face = [&](int32 A, int32 B, int32 C, int32 D)
			{
				if (A == INDEX_NONE || B == INDEX_NONE || C == INDEX_NONE || D == INDEX_NONE) { return; }
				// Under the garden's earth nothing is seen.
				const double Zmax = FMath::Max(FMath::Max(M.Positions[A].Z, M.Positions[B].Z), FMath::Max(M.Positions[C].Z, M.Positions[D].Z)) / 100.0;
				if (Zmax < 0.0) { return; }
				M.Quad(A, B, C, D);
			};
			for (int32 K = 0; K < NZ; ++K)
			{
				for (int32 J = 0; J < NY; ++J)
				{
					for (int32 I = 0; I < NX; ++I)
					{
						const bool bIn = Values[Idx(I, J, K)] < 0;
						if (I + 1 < NX && (Values[Idx(I + 1, J, K)] < 0) != bIn) { Face(Cell(I, J - 1, K - 1), Cell(I, J, K - 1), Cell(I, J, K), Cell(I, J - 1, K)); }
						if (J + 1 < NY && (Values[Idx(I, J + 1, K)] < 0) != bIn) { Face(Cell(I - 1, J, K - 1), Cell(I, J, K - 1), Cell(I, J, K), Cell(I - 1, J, K)); }
						if (K + 1 < NZ && (Values[Idx(I, J, K + 1)] < 0) != bIn) { Face(Cell(I - 1, J - 1, K), Cell(I, J - 1, K), Cell(I, J, K), Cell(I - 1, J, K)); }
					}
				}
			}
		}
	}

	namespace Pavilion
	{
		constexpr double Floor = Summit + 0.15;
		constexpr double ColH = 2.35, ColR = 0.09;
		constexpr double EaveR = 2.35;      // the eave's corners, from the axis
		constexpr double Apex = Floor + ColH + 1.75;

		FVector2D Corner(int32 K, double R) { const double T = FMath::DegreesToRadians(60.0 * K); return FVector2D(CH::HexCX + R * FMath::Cos(T), CH::HexCY + R * FMath::Sin(T)); }

		/** The roof's surface over plan point Q: rising in a gentle concave curve to the apex, the corners swept up. */
		double RoofZ(const FVector2D& Q, double Lift = 0.45)
		{
			const FVector2D Rel = Q - FVector2D(CH::HexCX, CH::HexCY);
			const double Ang = FMath::Atan2(Rel.Y, Rel.X);
			const double Sector = FMath::Fmod(Ang + 2.0 * UE_DOUBLE_PI, UE_DOUBLE_PI / 3.0) - UE_DOUBLE_PI / 6.0;   // −30° … 30° about a face's middle
			const double Apothem = Rel.Size() * FMath::Cos(Sector);            // distance out along the face's normal
			const double Frac = FMath::Clamp(Apothem / (EaveR * FMath::Cos(UE_DOUBLE_PI / 6.0)), 0.0, 1.2);
			const double Base = Floor + ColH + 0.35;
			const double Z = Base + (Apex - Base) * FMath::Pow(FMath::Max(0.0, 1.0 - Frac), 1.35);
			const double Corner = FMath::Pow(FMath::Abs(Sector) / (UE_DOUBLE_PI / 6.0), 3.0);
			return Z + Lift * Corner * FMath::Square(Frac);
		}

		void Build(FChMeshes& Out)
		{
			const FVector2D C(CH::HexCX, CH::HexCY);
			// The platform: a hexagon of bluestone, a step up from the summit.
			TArray<FVector2D> Plat;
			for (int32 k = 0; k < 6; ++k) { Plat.Add(Corner(k, CH::HexR)); }
			Kit::Prism(Out[CB::BlueStone], TEXT("Pavilion platform"), Plat, Summit - 0.3, Floor, false);
			// Columns, lintels and hanging fretwork, seats with goose-neck backs (not on the entrance's side, the west).
			for (int32 k = 0; k < 6; ++k)
			{
				const FVector2D P = Corner(k, CH::HexColR), Q = Corner(k + 1, CH::HexColR);
				Kit::Box(Out[CB::BlueStone], TEXT("Column base"), FVector(P.X - 0.14, P.Y - 0.14, Floor - 0.02), FVector(P.X + 0.14, P.Y + 0.14, Floor + 0.03));
				Kit::Rod(Out[CB::Chestnut], TEXT("Pavilion column"), FVector(P.X, P.Y, Floor + 0.02), FVector(P.X, P.Y, Floor + ColH), ColR, 16, ColR * 0.95);
				const FVector2D D = (Q - P).GetSafeNormal();
				const FVector Side(-D.Y, D.X, 0.0);
				Kit::Member(Out[CB::Chestnut], TEXT("Lintel"), FVector(P.X, P.Y, Floor + ColH - 0.1), FVector(Q.X, Q.Y, Floor + ColH - 0.1), Side, 0.12, 0.2, 0.015);
				Kit::FLocal L;
				L.Origin = P;
				L.U = D;
				L.V = FVector2D(Side.X, Side.Y);
				const double Len = FVector2D::Distance(P, Q);
				ChenghuaiHall::LatticePanel(Out, L, ColR + 0.02, -0.012, Floor + ColH - 0.48, Len - 2.0 * ColR - 0.04, 0.28, ChenghuaiHall::ELattice::StepBrocade, CB::Chestnut, false);
				// Open where the path arrives (the west-north-west face, k = 3) and where it leaves, down the south side
				// back to the walk (the opposite face, k = 0): a seat there shut the loop (the MuseeDo walk stalled in here).
				if (k == 3 || k == 0) { continue; }
				// The seat: a plank on a panelled base; its back leaning out over the drop.
				const FVector2D Mid = 0.5 * (P + Q);
				const FVector2D In = (C - Mid).GetSafeNormal();
				const FVector2D S0 = P + D * (ColR + 0.01), S1 = Q - D * (ColR + 0.01);
				Kit::Member(Out[CB::Chestnut], TEXT("Seat"), FVector(S0.X + In.X * 0.18, S0.Y + In.Y * 0.18, Floor + 0.44), FVector(S1.X + In.X * 0.18, S1.Y + In.Y * 0.18, Floor + 0.44), FVector(In.X, In.Y, 0.0), 0.36, 0.05);
				Kit::Member(Out[CB::Chestnut], TEXT("Seat base"), FVector(S0.X + In.X * 0.18, S0.Y + In.Y * 0.18, Floor + 0.21), FVector(S1.X + In.X * 0.18, S1.Y + In.Y * 0.18, Floor + 0.21), FVector(In.X, In.Y, 0.0), 0.3, 0.4);
				const int32 NB = FMath::Max(3, FMath::RoundToInt32((S1 - S0).Size() / 0.13));
				for (int32 b = 0; b <= NB; ++b)
				{
					const FVector2D At = FMath::Lerp(S0, S1, double(b) / NB);
					TArray<FVector> Neck;
					for (int32 t = 0; t <= 6; ++t)
					{
						const double T = double(t) / 6;
						const double Outw = 0.02 + 0.2 * FMath::Sin(T * 1.9);
						Neck.Add(FVector(At.X - In.X * Outw, At.Y - In.Y * Outw, Floor + 0.46 + 0.42 * T));
					}
					Kit::SweepSolid(Out[CB::Chestnut], TEXT("Goose-neck baluster"), Kit::UprightFrames(Neck), Kit::RectProfile(0.035, 0.035));
				}
				const FVector2D T0 = S0 - In * 0.19, T1 = S1 - In * 0.19;
				Kit::Member(Out[CB::Chestnut], TEXT("Back rail"), FVector(T0.X, T0.Y, Floor + 0.89), FVector(T1.X, T1.Y, Floor + 0.89), FVector(In.X, In.Y, 0.0), 0.07, 0.05, 0.01);
				// The visitor's guard at the back's height (not drawn): the pavilion is on a 2.6 m rock.
				Kit::Member(Out[CB::Guard], TEXT("Seat guard"), FVector(T0.X, T0.Y, Floor + 0.7), FVector(T1.X, T1.Y, Floor + 0.7), FVector(In.X, In.Y, 0.0), 0.1, 1.4);
			}
			// The roof: six slopes of butterfly tiles between the hips, over a chestnut soffit.
			const double Thick = 0.14;
			for (int32 k = 0; k < 6; ++k)
			{
				const FVector2D P = Corner(k, EaveR), Q = Corner(k + 1, EaveR);
				const FVector2D Mid = 0.5 * (P + Q);
				const FVector2D Out2 = (Mid - C).GetSafeNormal();
				const FVector2D Along = (Q - P).GetSafeNormal();
				const double Ap = FVector2D::Distance(Mid, C);
				// The slab: a patch over the face's triangle (apex → eave), its top the bed, its underside the soffit.
				const int32 NU = 10, NV = 14;
				auto PlanAt = [&](int32 I, int32 J)
				{
					const double B = double(J) / NV;                 // 0 at the apex … 1 at the eave
					const double A = (double(I) / NU - 0.5) * 2.0;    // −1 … 1 across
					return C + Out2 * (Ap * B) + Along * (A * 0.5 * FVector2D::Distance(P, Q) * B);
				};
				Out[CB::RoofMortar].Begin(TEXT("Pavilion roof bed"), false, TEXT("Pavilion roof"));
				Kit::GridPatch(Out[CB::RoofMortar].M, NU, NV, [&](int32 I, int32 J, FVector& Pos, FVector& Nrm, FVector2D& UV)
				{
					const FVector2D Q2 = PlanAt(I, J);
					Pos = FVector(Q2.X, Q2.Y, RoofZ(Q2));
					const double E = 0.01;
					const double GX = (RoofZ(Q2 + FVector2D(E, 0)) - RoofZ(Q2 - FVector2D(E, 0))) / (2 * E);
					const double GY = (RoofZ(Q2 + FVector2D(0, E)) - RoofZ(Q2 - FVector2D(0, E))) / (2 * E);
					Nrm = FVector(-GX, -GY, 1.0).GetSafeNormal();
					UV = Q2;
				});
				Out[CB::Chestnut].Begin(TEXT("Pavilion soffit"), false, TEXT("Pavilion roof"));
				Kit::GridPatch(Out[CB::Chestnut].M, NU, NV, [&](int32 I, int32 J, FVector& Pos, FVector& Nrm, FVector2D& UV)
				{
					const FVector2D Q2 = PlanAt(I, J);
					Pos = FVector(Q2.X, Q2.Y, RoofZ(Q2) - Thick);
					Nrm = FVector(0, 0, -1);
					UV = Q2;
				});
				// The fascia along the eave.
				TArray<FVector> Upper, Lower;
				for (int32 I = 0; I <= NU; ++I)
				{
					const FVector2D Q2 = PlanAt(I, NV);
					Upper.Add(FVector(Q2.X, Q2.Y, RoofZ(Q2)));
					Lower.Add(FVector(Q2.X, Q2.Y, RoofZ(Q2) - Thick));
				}
				Out[CB::Chestnut].Begin(TEXT("Pavilion fascia"), false, TEXT("Pavilion roof"));
				Kit::Strip(Out[CB::Chestnut].M, Upper, Lower, FVector(Out2.X, Out2.Y, 0.0));
				// The tiles: rows running down the face, cut by the two hips.
				ChenghuaiRoof::FSlope S;
				S.Origin = C;
				S.DDir = Out2;
				S.SDir = Along;
				S.Bed = [C, Out2, Along](double D, double Sv) { return RoofZ(C + Out2 * D + Along * Sv) + 0.01; };
				S.From = 0.25;
				S.To = Ap;
				S.bOrnaments = true;
				// The hips (C→P and C→Q): keep between them.
				for (const FVector2D& Hip : {P, Q})
				{
					const FVector2D Dir = (Hip - C).GetSafeNormal();
					FVector2D Nn(-Dir.Y, Dir.X);
					if (FVector2D::DotProduct(Nn, Mid - C) > 0.0) { Nn = -Nn; }
					S.Limits.Add(FVector(Nn.X, Nn.Y, FVector2D::DotProduct(Nn, C) - 0.06));
				}
				ChenghuaiRoof::TileRows(Out[CB::Tiles], S, -3.0, 3.0, ChenghuaiRoof::FTileStyle::He(), 0.0);
				// The hip (垂脊) up this face's first edge, sweeping up to the corner (戗角).
				TArray<FVector> HipPath;
				for (int32 t = 0; t <= 16; ++t)
				{
					const double T = double(t) / 16;
					const FVector2D Q2 = FMath::Lerp(C, P, 0.06 + 0.97 * T);
					HipPath.Add(FVector(Q2.X, Q2.Y, RoofZ(Q2) + 0.04 + 0.12 * FMath::Pow(T, 6.0)));
				}
				FProfile HipP;
				HipP.bClosed = true;
				HipP.Add(-0.08, -0.06).Add(0.08, -0.06).Add(0.08, 0.08).Add(0.05, 0.12, true).Add(0.0, 0.13, true).Add(-0.05, 0.12, true).Add(-0.08, 0.08);
				Kit::SweepSolid(Out[CB::Ridges], TEXT("Hip"), Kit::UprightFrames(HipPath), HipP);
				// The corner beam (角梁) under the hip.
				const FVector2D CP0 = FMath::Lerp(C, P, 0.55), CP1 = FMath::Lerp(C, P, 1.02);
				Kit::Member(Out[CB::Chestnut], TEXT("Corner beam"), FVector(CP0.X, CP0.Y, RoofZ(CP0) - Thick - 0.08), FVector(CP1.X, CP1.Y, RoofZ(CP1) - Thick - 0.02),
							FVector(-(P - C).Y, (P - C).X, 0.0).GetSafeNormal(), 0.1, 0.16, 0.01);
			}
			// The finial (宝顶).
			FProfile Top;
			Top.Add(0.0, 0.0).Add(0.2, 0.0).Add(0.2, 0.08).Add(0.13, 0.14, true).Add(0.17, 0.3, true).Add(0.12, 0.45, true).Add(0.05, 0.58, true).Add(0.0, 0.62);
			Kit::Lathe(Out[CB::Ridges], TEXT("Finial"), FVector(C.X, C.Y, Apex - 0.12), FVector::UpVector, Top, 24, 0.2);
		}
	}

	void BuildRockery(FChMeshes& Out)
	{
		Out[CB::Rockery].Begin(TEXT("Rockery"), false);
		RockMesh::BuildMass(Out[CB::Rockery].M);
		Pavilion::Build(Out);
		// The visitor's guards along the path where it runs high over a drop (not drawn).
		const FPathInfo& I = Path();
		for (int32 k = 0; k + 1 < I.P.Num(); ++k)
		{
			const FVector2D A = I.P[k], B = I.P[k + 1];
			const FVector2D D = (B - A).GetSafeNormal(), Nn(-D.Y, D.X);
			for (const double S : {-1.0, 1.0})
			{
				const FVector2D PA = A + Nn * (S * (PathHalf + 0.25)), PB = B + Nn * (S * (PathHalf + 0.25));
				const double Z = PathHeight(0.5 * (I.S[k] + I.S[k + 1]));
				const double Drop = Z - 0.5 * (Height(PA) + Height(PB));
				if (Z < 1.0 || Drop < 0.6) { continue; }
				Kit::Member(Out[CB::Guard], TEXT("Path guard"), FVector(PA.X, PA.Y, Z + 0.5), FVector(PB.X, PB.Y, Z + 0.5), FVector(Nn.X, Nn.Y, 0.0), 0.1, 1.0);
			}
		}
	}
}
