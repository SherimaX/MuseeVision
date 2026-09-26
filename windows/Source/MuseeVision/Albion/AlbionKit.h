#pragma once

#include "CoreMinimal.h"
#include "Algo/Reverse.h"
#include "Furniture/FurnitureKit.h"
#include "Salon/SalonKit.h"

/**
 * Geometry kit for Albion (AAlbionStructure and its companions): SalonKit's indexed mesh (positions in plan metres,
 * written in centimetres; UVs in metres; every triangle wound to face along its vertex normals), and the solids the
 * court is built from: walls with openings cut in bands of stone, pointed arcs, members swept along paths, lathes,
 * tori and carved leaves.
 */
namespace AlbionKit
{
	using SalonKit::FMeshData;

	constexpr double kPi = UE_DOUBLE_PI;

	inline double Cross2(const FVector2D& A, const FVector2D& B) { return A.X * B.Y - A.Y * B.X; }

	inline double Area2(const TArray<FVector2D>& P)
	{
		double A = 0.0;
		for (int32 i = 0, j = P.Num() - 1; i < P.Num(); j = i++) { A += Cross2(P[j], P[i]); }
		return A;
	}

	inline bool StrictlyInside(const FVector2D& Q, const FVector2D& A, const FVector2D& B, const FVector2D& C)
	{
		constexpr double Eps = 1e-12;
		return Cross2(B - A, Q - A) > Eps && Cross2(C - B, Q - B) > Eps && Cross2(A - C, Q - C) > Eps;
	}

	/** Ear clipping of a simple outline (either winding): index triples into P. */
	inline TArray<int32> EarClip(const TArray<FVector2D>& P)
	{
		TArray<int32> Out;
		const int32 N = P.Num();
		if (N < 3) { return Out; }
		TArray<int32> V;
		V.Reserve(N);
		const bool bCCW = Area2(P) > 0.0;
		for (int32 i = 0; i < N; ++i) { V.Add(bCCW ? i : N - 1 - i); }
		while (V.Num() > 3)
		{
			const int32 Count = V.Num();
			int32 Ear = INDEX_NONE;
			for (int32 e = 0; e < Count && Ear == INDEX_NONE; ++e)
			{
				const int32 A = V[(e + Count - 1) % Count], B = V[e], C = V[(e + 1) % Count];
				if (Cross2(P[B] - P[A], P[C] - P[B]) <= 1e-14) { continue; }
				bool bClear = true;
				for (const int32 K : V)
				{
					if (K == A || K == B || K == C) { continue; }
					if (P[K].Equals(P[A], 1e-12) || P[K].Equals(P[B], 1e-12) || P[K].Equals(P[C], 1e-12)) { continue; }
					if (StrictlyInside(P[K], P[A], P[B], P[C])) { bClear = false; break; }
				}
				if (bClear) { Ear = e; }
			}
			if (Ear == INDEX_NONE)
			{
				double Best = TNumericLimits<double>::Max();
				for (int32 e = 0; e < Count; ++e)
				{
					const int32 A = V[(e + Count - 1) % Count], B = V[e], C = V[(e + 1) % Count];
					const double Turn = FMath::Abs(Cross2(P[B] - P[A], P[C] - P[B]));
					if (Turn < Best) { Best = Turn; Ear = e; }
				}
			}
			Out.Append({V[(Ear + Count - 1) % Count], V[Ear], V[(Ear + 1) % Count]});
			V.RemoveAt(Ear);
		}
		Out.Append({V[0], V[1], V[2]});
		return Out;
	}

	/** A flat outline on the plane Origin + AxisU x + AxisV y, facing N; metre UVs from the face (SalonKit::FaceUV) or given. */
	inline void Planar(FMeshData& M, const TArray<FVector2D>& Outline, const FVector& Origin, const FVector& AxisU, const FVector& AxisV,
					   const FVector& N, TFunction<FVector2D(const FVector&)> UVOf = nullptr)
	{
		const TArray<int32> T = EarClip(Outline);
		const int32 Base = M.Positions.Num();
		for (const FVector2D& Q : Outline)
		{
			const FVector P = Origin + AxisU * Q.X + AxisV * Q.Y;
			M.Vertex(P, N, UVOf ? UVOf(P) : SalonKit::FaceUV(P, N));
		}
		for (int32 t = 0; t + 2 < T.Num(); t += 3) { M.Tri(Base + T[t], Base + T[t + 1], Base + T[t + 2]); }
	}

	/** A closed prism of a plan outline from Z0 to Z1 (flat sides). */
	inline void Prism(FMeshData& M, const TArray<FVector2D>& Outline, double Z0, double Z1)
	{
		Planar(M, Outline, FVector(0, 0, Z1), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1));
		Planar(M, Outline, FVector(0, 0, Z0), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, -1));
		const int32 N = Outline.Num();
		const double Sign = Area2(Outline) > 0.0 ? 1.0 : -1.0;
		for (int32 e = 0; e < N; ++e)
		{
			const FVector2D A = Outline[e], B = Outline[(e + 1) % N];
			const FVector2D D = (B - A).GetSafeNormal();
			const FVector Nrm = FVector(D.Y, -D.X, 0.0) * Sign;
			M.Rect(FVector(A.X, A.Y, Z0), FVector(B.X, B.Y, Z0), FVector(B.X, B.Y, Z1), FVector(A.X, A.Y, Z1), Nrm);
		}
	}

	inline void Box(FMeshData& M, const FVector& Lo, const FVector& Hi, int32 Faces = FMeshData::AllFaces) { M.Box(Lo, Hi, Faces); }

	/**
	 * A profile (radius, height) turned about the vertical through Base: points from the bottom outwards and up (the solid
	 * on the left of the path as it climbs). A point marked smooth averages its two segments' normals. U runs round
	 * (metres at the profile's own radius), V up the profile, so a stone's veins run up a shaft.
	 */
	struct FLathePoint { double R, Z; bool bSmooth; };

	inline void Lathe(FMeshData& M, const FVector& Base, const TArray<FLathePoint>& Profile, int32 Segments, double U0 = 0.0,
					  double A0 = 0.0, double A1 = 2.0 * kPi)
	{
		const int32 NP = Profile.Num();
		if (NP < 2) { return; }
		const bool bFull = FMath::IsNearlyEqual(A1 - A0, 2.0 * kPi);
		TArray<FVector2D> SegN;
		TArray<double> Len;
		Len.Add(0.0);
		for (int32 j = 0; j + 1 < NP; ++j)
		{
			const FVector2D D(Profile[j + 1].R - Profile[j].R, Profile[j + 1].Z - Profile[j].Z);
			SegN.Add(FVector2D(D.Y, -D.X).GetSafeNormal());
			Len.Add(Len.Last() + D.Size());
		}
		auto NormalAt = [&](int32 j, bool bEnd) -> FVector2D
		{
			const int32 PointIndex = bEnd ? j + 1 : j;
			if (!Profile[PointIndex].bSmooth) { return SegN[j]; }
			const int32 Other = bEnd ? j + 1 : j - 1;
			if (Other < 0 || Other >= SegN.Num()) { return SegN[j]; }
			return (SegN[j] + SegN[Other]).GetSafeNormal();
		};
		for (int32 j = 0; j + 1 < NP; ++j)
		{
			const FLathePoint& P0 = Profile[j];
			const FLathePoint& P1 = Profile[j + 1];
			if (FMath::Abs(P0.R - P1.R) < 1e-9 && FMath::Abs(P0.Z - P1.Z) < 1e-9) { continue; }
			const FVector2D N0 = NormalAt(j, false), N1 = NormalAt(j, true);
			const int32 Base0 = M.Positions.Num();
			for (int32 k = 0; k <= Segments; ++k)
			{
				const double T = A0 + (A1 - A0) * (bFull ? (k % Segments) : k) / Segments;
				const FVector Radial(FMath::Cos(T), FMath::Sin(T), 0.0);
				const double Along = (A1 - A0) * k / Segments;
				M.Vertex(Base + Radial * P0.R + FVector(0, 0, P0.Z), Radial * N0.X + FVector(0, 0, N0.Y), FVector2D(U0 + Along * FMath::Max(P0.R, 0.05), Len[j]));
				M.Vertex(Base + Radial * P1.R + FVector(0, 0, P1.Z), Radial * N1.X + FVector(0, 0, N1.Y), FVector2D(U0 + Along * FMath::Max(P1.R, 0.05), Len[j + 1]));
			}
			for (int32 k = 0; k < Segments; ++k)
			{
				const int32 A = Base0 + 2 * k;
				M.Quad(A, A + 2, A + 3, A + 1);
			}
		}
	}

	/** A sphere (a fruit, a berry, a boss), squashed by Squash along z. */
	inline void Ball(FMeshData& M, const FVector& C, double R, int32 Seg = 16, double Squash = 1.0)
	{
		TArray<FLathePoint> P;
		const int32 Rings = FMath::Max(4, Seg / 2);
		for (int32 i = 0; i <= Rings; ++i)
		{
			const double T = -0.5 * kPi + kPi * i / Rings;
			P.Add({R * FMath::Cos(T), R * Squash * FMath::Sin(T), i > 0 && i < Rings});
		}
		Lathe(M, C, P, Seg);
	}

	/** An upright cylinder (closed) about (C.X, C.Y) from Z0 to Z1. */
	inline void Cylinder(FMeshData& M, const FVector2D& C, double R, double Z0, double Z1, int32 Segments, bool bCapTop = true, bool bCapBottom = true)
	{
		TArray<FLathePoint> P;
		if (bCapBottom) { P.Add({0.0, Z0, false}); }
		P.Add({R, Z0, false});
		P.Add({R, Z1, false});
		if (bCapTop) { P.Add({0.0, Z1, false}); }
		Lathe(M, FVector(C.X, C.Y, 0.0), P, Segments);
	}

	/** A torus: a ring of radius Major round Axis through Centre, its tube Minor. U round the ring, V round the tube. */
	inline void Torus(FMeshData& M, const FVector& Centre, const FVector& Axis, double Major, double Minor, int32 SegMajor, int32 SegMinor)
	{
		const FVector W = Axis.GetSafeNormal();
		const FVector E1 = FVector::CrossProduct(W, FMath::Abs(W.Z) < 0.9 ? FVector::UpVector : FVector::ForwardVector).GetSafeNormal();
		const FVector E2 = FVector::CrossProduct(W, E1);
		const int32 Base = M.Positions.Num();
		for (int32 i = 0; i <= SegMajor; ++i)
		{
			const double A = 2.0 * kPi * (i % SegMajor) / SegMajor;
			const FVector Radial = E1 * FMath::Cos(A) + E2 * FMath::Sin(A);
			for (int32 j = 0; j <= SegMinor; ++j)
			{
				const double B = 2.0 * kPi * (j % SegMinor) / SegMinor;
				const FVector N = Radial * FMath::Cos(B) + W * FMath::Sin(B);
				M.Vertex(Centre + Radial * Major + N * Minor, N, FVector2D(Major * 2.0 * kPi * i / SegMajor, Minor * 2.0 * kPi * j / SegMinor));
			}
		}
		const int32 W1 = SegMinor + 1;
		for (int32 i = 0; i < SegMajor; ++i)
		{
			for (int32 j = 0; j < SegMinor; ++j) { M.Quad(Base + i * W1 + j, Base + (i + 1) * W1 + j, Base + (i + 1) * W1 + j + 1, Base + i * W1 + j + 1); }
		}
	}

	/**
	 * A closed section (a convex polygon in (A, B), counter-clockwise) carried through stations, each an origin and its
	 * axes; the rings are joined and capped (FurnitureKit::Rings). U runs along the stations (metres).
	 */
	struct FStation
	{
		FVector O;
		FVector A;
		FVector B;
	};

	inline void SweepStations(FMeshData& M, const TArray<FStation>& Stations, const TArray<FVector2D>& Section, bool bCap0 = true, bool bCap1 = true,
							  double U0 = 0.0)
	{
		if (Stations.Num() < 2 || Section.Num() < 3) { return; }
		TArray<TArray<FVector>> R;
		TArray<double> U;
		double S = U0;
		for (int32 i = 0; i < Stations.Num(); ++i)
		{
			if (i > 0) { S += FVector::Distance(Stations[i - 1].O, Stations[i].O); }
			TArray<FVector> Ring;
			for (const FVector2D& Q : Section) { Ring.Add(Stations[i].O + Stations[i].A * Q.X + Stations[i].B * Q.Y); }
			R.Add(MoveTemp(Ring));
			U.Add(S);
		}
		FurnitureKit::Rings(M, R, U, false, bCap0, bCap1);
	}

	/** A rectangle A0 … A1 × B0 … B1, counter-clockwise in (A, B). */
	inline TArray<FVector2D> RectSection(double A0, double A1, double B0, double B1)
	{
		return {FVector2D(A0, B0), FVector2D(A1, B0), FVector2D(A1, B1), FVector2D(A0, B1)};
	}

	/** A bar of rectangular section between two points: A across (made square to the bar), B completing the frame. */
	inline void Beam(FMeshData& M, const FVector& P0, const FVector& P1, const FVector& AHint, double HalfA, double HalfB)
	{
		const FVector T = (P1 - P0).GetSafeNormal();
		const FVector A = (AHint - T * FVector::DotProduct(AHint, T)).GetSafeNormal();
		const FVector B = FVector::CrossProduct(T, A);
		SweepStations(M, {{P0, A, B}, {P1, A, B}}, RectSection(-HalfA, HalfA, -HalfB, HalfB));
	}

	/**
	 * One half of a pointed vault or arch in a vertical plane: from the springing (SpringX, SpringZ), whose tangent is
	 * vertical, round a circle of radius R centred level with it (CentreX), up to the crown at CrownX. X is the
	 * coordinate across the plane (plan x for the transverse vaults, plan y for the arcade's arches).
	 */
	struct FArcHalf
	{
		double SpringX = 0.0, CentreX = 0.0, R = 1.0, SpringZ = 0.0, CrownX = 0.0;

		double Sign() const { return SpringX >= CentreX ? 1.0 : -1.0; }
		/** The angle at the crown (from the springing's 0). */
		double CrownAngle() const { return FMath::Acos(FMath::Clamp(FMath::Abs(CrownX - CentreX) / R, -1.0, 1.0)); }
		double Length() const { return R * CrownAngle(); }
		/** The point at angle T, Inset towards the centre (negative: outwards). (x across, z up.) */
		FVector2D At(double T, double Inset = 0.0) const
		{
			const double Rr = R - Inset;
			return FVector2D(CentreX + Sign() * Rr * FMath::Cos(T), SpringZ + Rr * FMath::Sin(T));
		}
		/** The outward normal at angle T (away from the centre). */
		FVector2D Normal(double T) const { return FVector2D(Sign() * FMath::Cos(T), FMath::Sin(T)); }
		/** The tangent at T, climbing towards the crown. */
		FVector2D Tangent(double T) const { return FVector2D(-Sign() * FMath::Sin(T), FMath::Cos(T)); }
		/** The angle at which the arc (radius R − Inset) reaches height Z, clamped to the arc. */
		double AngleAtHeight(double Z, double Inset = 0.0) const
		{
			return FMath::Asin(FMath::Clamp((Z - SpringZ) / (R - Inset), 0.0, 1.0));
		}
		/** The half-width from the arch's axis (CrownX) at height Z, on the arc of radius R − Inset (0 above it). */
		double HalfWidthAt(double Z, double Inset = 0.0) const
		{
			const double Rr = R - Inset;
			const double Dz = Z - SpringZ;
			if (Dz <= 0.0) { return FMath::Abs(SpringX - CrownX) + Inset * 0.0; }
			if (Dz >= Rr) { return 0.0; }
			const double Dx = FMath::Sqrt(Rr * Rr - Dz * Dz);
			return FMath::Max(0.0, Dx - FMath::Abs(CentreX - CrownX));
		}
	};

	/** A symmetric pointed arch about AxisX: half-span S, springing at SpringZ, crown at CrownZ. */
	inline FArcHalf PointedHalf(double AxisX, double HalfSpan, double SpringZ, double CrownZ, bool bRightHalf)
	{
		const double H = CrownZ - SpringZ;
		const double R = (H * H + HalfSpan * HalfSpan) / (2.0 * HalfSpan);
		FArcHalf A;
		A.R = R;
		A.SpringZ = SpringZ;
		A.CrownX = AxisX;
		A.SpringX = AxisX + (bRightHalf ? HalfSpan : -HalfSpan);
		A.CentreX = A.SpringX + (bRightHalf ? -R : R);
		return A;
	}

	// ============================================================================================ walls with openings

	/**
	 * An opening through a wall face: centred at U (along the face), from Sill to its head, its half-width at height z
	 * HalfAt(z) (0 or less above the head). Stations: the heights at which its outline turns (the arch's points), which
	 * the face is cut at so it meets the reveal vertex for vertex.
	 */
	struct FOpening
	{
		double U = 0.0;
		double Sill = 0.0;
		double Head = 1.0;
		TFunction<double(double)> HalfAt;
		TArray<double> Stations;
	};

	/** A round-headed or pointed opening's outline stations (Sill, the springing, then N points up the arch to the head). */
	inline FOpening ArchedOpening(double U, double Half, double Sill, double Spring, double Head, int32 N, double Grow = 0.0)
	{
		FOpening O;
		O.U = U;
		O.Sill = Sill;
		O.Head = Head + Grow;
		const double H = Head - Spring;
		const bool bRound = FMath::IsNearlyEqual(H, Half, 1e-4);
		// Two-centred (pointed), or a semicircle when the rise equals the half-span.
		const double R = bRound ? Half : (H * H + Half * Half) / (2.0 * Half);
		const double CentreOff = R - Half;   // each arc's centre, across the axis
		const double RG = R + Grow, HalfG = Half + Grow;
		O.HalfAt = [=](double Z) -> double
		{
			if (Z <= Spring) { return HalfG; }
			const double Dz = Z - Spring;
			if (Dz >= RG) { return 0.0; }
			return FMath::Max(0.0, FMath::Sqrt(RG * RG - Dz * Dz) - CentreOff);
		};
		O.Stations.Add(Sill);
		O.Stations.Add(Spring);
		const double TopAngle = FMath::Acos(FMath::Clamp(CentreOff / RG, -1.0, 1.0));
		for (int32 i = 1; i <= N; ++i)
		{
			const double T = TopAngle * i / N;
			O.Stations.Add(Spring + RG * FMath::Sin(T));
		}
		O.Head = O.Stations.Last();
		return O;
	}

	/** A rectangular opening. */
	inline FOpening RectOpening(double U, double Half, double Sill, double Head)
	{
		FOpening O;
		O.U = U;
		O.Sill = Sill;
		O.Head = Head;
		O.HalfAt = [Half](double) { return Half; };
		O.Stations = {Sill, Head};
		return O;
	}

	/**
	 * A wall face: the plane through Origin with U along it and N its facing (Z up), from U0 to U1 and Z0 to Z1, less the
	 * openings, cut at every height in Cuts (the bands) and every opening's station. Each strip between two cuts goes to
	 * the mesh Pick(zMid) returns (or none, if null): the bands of stone are one plane cut into courses of material.
	 */
	inline void WallFace(TFunctionRef<FMeshData*(double)> Pick, const FVector& Origin, const FVector& U, const FVector& N,
						 double U0, double U1, double Z0, double Z1, const TArray<FOpening>& Openings, const TArray<double>& Cuts)
	{
		TArray<double> Zs = {Z0, Z1};
		for (double C : Cuts) { if (C > Z0 + 1e-6 && C < Z1 - 1e-6) { Zs.Add(C); } }
		for (const FOpening& O : Openings)
		{
			for (double S : O.Stations) { if (S > Z0 + 1e-6 && S < Z1 - 1e-6) { Zs.Add(S); } }
		}
		Zs.Sort();
		TArray<double> Clean;
		for (double Z : Zs) { if (Clean.Num() == 0 || Z - Clean.Last() > 1e-6) { Clean.Add(Z); } }
		const FVector Up(0, 0, 1);
		auto P = [&](double Along, double Z) { return Origin + U * Along + Up * Z; };
		for (int32 k = 0; k + 1 < Clean.Num(); ++k)
		{
			const double Za = Clean[k], Zb = Clean[k + 1];
			FMeshData* M = Pick(0.5 * (Za + Zb));
			if (!M) { continue; }
			// The openings live in this strip, left to right, with their edges at Za and Zb.
			struct FSpan { double La, Lb, Ra, Rb; };
			TArray<FSpan> Spans;
			for (const FOpening& O : Openings)
			{
				if (O.Sill >= Zb - 1e-7 || O.Head <= Za + 1e-7) { continue; }
				const double Ha = O.HalfAt(FMath::Max(Za, O.Sill) + 1e-9), Hb = O.HalfAt(FMath::Min(Zb, O.Head) - 1e-9);
				// At an arch's point the half-width is 0 at the head: take the exact value there.
				const double HaE = Za <= O.Sill + 1e-7 ? O.HalfAt(O.Sill + 1e-6) : O.HalfAt(Za);
				const double HbE = Zb >= O.Head - 1e-7 ? 0.0 : O.HalfAt(Zb);
				const double UseA = FMath::Max(0.0, Za < O.Sill ? Ha : HaE), UseB = FMath::Max(0.0, Zb > O.Head ? Hb : HbE);
				Spans.Add({O.U - UseA, O.U - UseB, O.U + UseA, O.U + UseB});
			}
			Spans.Sort([](const FSpan& A, const FSpan& B) { return A.La + A.Lb < B.La + B.Lb; });
			double LeftA = U0, LeftB = U0;
			for (const FSpan& S : Spans)
			{
				M->Poly({P(LeftA, Za), P(S.La, Za), P(S.Lb, Zb), P(LeftB, Zb)}, N);
				LeftA = S.Ra;
				LeftB = S.Rb;
			}
			M->Poly({P(LeftA, Za), P(U1, Za), P(U1, Zb), P(LeftB, Zb)}, N);
		}
	}

	/**
	 * The reveal of an opening between two parallel faces (the face at Origin and one Depth behind it, along −N): its
	 * jambs and soffit from the sill to the head, facing into the opening. The sill itself is left to the caller.
	 */
	inline void Reveal(FMeshData& M, const FVector& Origin, const FVector& U, const FVector& N, const FOpening& O, double Depth0, double Depth1)
	{
		const FVector Up(0, 0, 1);
		TArray<double> Zs;
		for (double S : O.Stations) { Zs.Add(S); }
		Zs.Sort();
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			for (int32 k = 0; k + 1 < Zs.Num(); ++k)
			{
				const double Za = Zs[k], Zb = Zs[k + 1];
				const double Ha = Za <= O.Sill + 1e-7 ? O.HalfAt(O.Sill + 1e-6) : O.HalfAt(Za);
				const double Hb = Zb >= O.Head - 1e-7 ? 0.0 : O.HalfAt(Zb);
				const FVector A = Origin + U * (O.U + Side * Ha) + Up * Za, B = Origin + U * (O.U + Side * Hb) + Up * Zb;
				// Facing into the opening: towards the axis and down under the arch.
				const FVector Edge = (B - A).GetSafeNormal();
				FVector Nrm = FVector::CrossProduct(Edge, N).GetSafeNormal();
				if (FVector::DotProduct(Nrm, U * (-Side)) + FVector::DotProduct(Nrm, -Up) * 0.001 < 0.0) { Nrm = -Nrm; }
				M.Poly({A - N * Depth0, B - N * Depth0, B - N * Depth1, A - N * Depth1}, Nrm);
			}
		}
	}
}
