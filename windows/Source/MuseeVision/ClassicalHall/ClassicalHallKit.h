#pragma once

#include "CoreMinimal.h"

/**
 * Geometry kit for the classical hall (ClassicalHallGeometry.cpp): an indexed mesh in metres, polygons, swept and
 * turned mouldings, zipped strips between chains of vertices, and wall faces pierced by openings.
 *
 * Positions are plan metres (X east, Y south, Z up); UVs are metres. Every triangle is wound to face along its
 * vertices' normals (Unreal's front faces: cross(b - a, c - a) points away from the viewer), so a helper only has to
 * give normals that face the viewer. Where two surfaces meet they share vertices: the helpers take and return chains of
 * vertex positions so that neighbours are built from the same points.
 *
 * Only CoreMinimal is used (no UObjects), so the geometry can also be built and checked outside the engine.
 */
namespace ClassicalHallKit
{
	constexpr double Pi = UE_DOUBLE_PI;
	inline double Rad(double Degrees) { return Degrees * (Pi / 180.0); }
	inline double Cross2(const FVector2D& A, const FVector2D& B) { return A.X * B.Y - A.Y * B.X; }

	/**
	 * Unit vectors of any length. (GetSafeNormal returns zero below a squared length of 1e-8: a direction taken from
	 * points a tenth of a millimetre apart, in metres, would vanish.)
	 */
	inline FVector Unit(const FVector& V)
	{
		const double S = V.Size();
		return S > 1e-30 ? V / S : FVector::ZeroVector;
	}
	inline FVector2D Unit(const FVector2D& V)
	{
		const double S = V.Size();
		return S > 1e-30 ? V / S : FVector2D::ZeroVector;
	}

	/** UVs in metres for a flat face: plan (X, Y) on floors and ceilings, (along the face, down) on walls. */
	inline FVector2D FaceUV(const FVector& P, const FVector& N)
	{
		if (FMath::Abs(N.Z) > 0.7) { return FVector2D(P.X, P.Y); }
		const FVector Along = Unit(FVector(-N.Y, N.X, 0.0));
		return FVector2D(FVector::DotProduct(P, Along), -P.Z);
	}

	struct FMesh
	{
		TArray<FVector> Positions;   // metres
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<int32> Indices;

		int32 Vertex(const FVector& P, const FVector& N, const FVector2D& UV)
		{
			Positions.Add(P);
			Normals.Add(Unit(N));
			return UVs.Add(UV);
		}

		/** A vertex with face UVs. */
		int32 Vertex(const FVector& P, const FVector& N) { return Vertex(P, N, FaceUV(P, N)); }

		/** A triangle facing along its vertices' normals; triangles without area are dropped. */
		void Tri(int32 A, int32 B, int32 C)
		{
			const FVector X = FVector::CrossProduct(Positions[B] - Positions[A], Positions[C] - Positions[A]);
			if (X.SizeSquared() < 1e-20) { return; }
			if (FVector::DotProduct(X, Normals[A] + Normals[B] + Normals[C]) < 0.0) { Indices.Append({A, B, C}); }
			else { Indices.Append({A, C, B}); }
		}

		void Quad(int32 A, int32 B, int32 C, int32 D)
		{
			Tri(A, B, C);
			Tri(A, C, D);
		}

		void Append(const FMesh& Other)
		{
			const int32 Base = Positions.Num();
			Positions.Append(Other.Positions);
			Normals.Append(Other.Normals);
			UVs.Append(Other.UVs);
			for (const int32 I : Other.Indices) { Indices.Add(Base + I); }
		}

		int32 NumTriangles() const { return Indices.Num() / 3; }
	};

	/**
	 * Ear-clips a simple polygon of either winding into index triples. A vertex in line with its neighbours is never an
	 * ear, and a candidate ear that has any other vertex inside it or on its edges is refused, so every vertex on the
	 * outline keeps its edges (no T-junctions with the neighbours that share the outline). Returns false if it gave up.
	 */
	inline bool Triangulate(const TArray<FVector2D>& Poly, TArray<int32>& Out)
	{
		const int32 N = Poly.Num();
		if (N < 3) { return false; }
		double Area = 0.0;
		FVector2D Lo = Poly[0], Hi = Poly[0];
		for (int32 i = 0; i < N; ++i)
		{
			Area += Cross2(Poly[i], Poly[(i + 1) % N]);
			Lo = FVector2D(FMath::Min(Lo.X, Poly[i].X), FMath::Min(Lo.Y, Poly[i].Y));
			Hi = FVector2D(FMath::Max(Hi.X, Poly[i].X), FMath::Max(Hi.Y, Poly[i].Y));
		}
		const double Sign = Area > 0.0 ? 1.0 : -1.0;
		const double Scale = FMath::Max(1e-9, (Hi - Lo).Size());
		const double TurnEps = 1e-12 * Scale * Scale;
		const double EdgeEps = 1e-9 * Scale;
		TArray<int32> V;
		for (int32 i = 0; i < N; ++i) { V.Add(i); }
		auto Turn = [&Poly, Sign](int32 A, int32 B, int32 C) { return Cross2(Poly[B] - Poly[A], Poly[C] - Poly[B]) * Sign; };
		auto Blocks = [&Poly, Sign, EdgeEps](const FVector2D& P, int32 A, int32 B, int32 C)
		{
			const FVector2D& PA = Poly[A];
			const FVector2D& PB = Poly[B];
			const FVector2D& PC = Poly[C];
			if (FVector2D::Distance(P, PA) < EdgeEps || FVector2D::Distance(P, PB) < EdgeEps || FVector2D::Distance(P, PC) < EdgeEps) { return false; }
			auto Side = [Sign](const FVector2D& E0, const FVector2D& E1, const FVector2D& Q)
			{
				const FVector2D D = E1 - E0;
				const double Len = FMath::Max(D.Size(), 1e-30);
				return Cross2(D, Q - E0) * Sign / Len;   // distance to the left (inside) of the edge
			};
			return Side(PA, PB, P) >= -EdgeEps && Side(PB, PC, P) >= -EdgeEps && Side(PC, PA, P) >= -EdgeEps;
		};
		int32 Failures = 0;
		int32 k = 0;
		while (V.Num() > 3)
		{
			const int32 M = V.Num();
			k %= M;
			const int32 A = V[(k + M - 1) % M], B = V[k], C = V[(k + 1) % M];
			bool bEar = Turn(A, B, C) > TurnEps;
			if (bEar)
			{
				for (const int32 Other : V)
				{
					if (Other == A || Other == B || Other == C) { continue; }
					if (Blocks(Poly[Other], A, B, C)) { bEar = false; break; }
				}
			}
			if (bEar)
			{
				Out.Append({A, B, C});
				V.RemoveAt(k);
				Failures = 0;
			}
			else
			{
				++k;
				if (++Failures > M) { return false; }
			}
		}
		Out.Append({V[0], V[1], V[2]});
		return true;
	}

	/** A flat polygon (points in order, either winding) facing N; UVs from UV (face UVs if null). */
	inline bool Poly(FMesh& M, const TArray<FVector>& Pts, const FVector& N, TFunctionRef<FVector2D(const FVector&)> UV)
	{
		if (Pts.Num() < 3) { return false; }
		const FVector Nn = Unit(N);
		const FVector U = Unit((FMath::Abs(Nn.Z) < 0.9 ? FVector::CrossProduct(FVector::UpVector, Nn) : FVector::CrossProduct(FVector::ForwardVector, Nn)));
		const FVector W = FVector::CrossProduct(Nn, U);
		TArray<FVector2D> P2;
		for (const FVector& P : Pts) { P2.Add(FVector2D(FVector::DotProduct(P, U), FVector::DotProduct(P, W))); }
		TArray<int32> Tris;
		const bool bOk = Triangulate(P2, Tris);
		const int32 Base = M.Positions.Num();
		for (const FVector& P : Pts) { M.Vertex(P, Nn, UV(P)); }
		for (int32 t = 0; t + 2 < Tris.Num(); t += 3) { M.Tri(Base + Tris[t], Base + Tris[t + 1], Base + Tris[t + 2]); }
		return bOk;
	}

	inline bool Poly(FMesh& M, const TArray<FVector>& Pts, const FVector& N)
	{
		const FVector Nn = Unit(N);
		return Poly(M, Pts, Nn, [&Nn](const FVector& P) { return FaceUV(P, Nn); });
	}

	/**
	 * A flat polygon with one hole (both loops in order, either winding), facing N: the hole joins the outline by a bridge
	 * between the hole's vertex furthest along one axis and the nearest outline vertex it can see, and the whole is
	 * ear-clipped. Every vertex of both loops keeps its edges.
	 */
	inline bool PolyWithHole(FMesh& M, TArray<FVector> Outer, TArray<FVector> Hole, const FVector& N)
	{
		const FVector Nn = Unit(N);
		const FVector U = Unit(FMath::Abs(Nn.Z) < 0.9 ? FVector::CrossProduct(FVector::UpVector, Nn) : FVector::CrossProduct(FVector::ForwardVector, Nn));
		const FVector W = FVector::CrossProduct(Nn, U);
		auto To2 = [&U, &W](const FVector& P) { return FVector2D(FVector::DotProduct(P, U), FVector::DotProduct(P, W)); };
		auto Area = [&To2](const TArray<FVector>& L)
		{
			double A = 0.0;
			for (int32 i = 0; i < L.Num(); ++i) { A += Cross2(To2(L[i]), To2(L[(i + 1) % L.Num()])); }
			return A;
		};
		auto Reverse = [](TArray<FVector>& L)
		{
			TArray<FVector> R;
			for (int32 i = L.Num() - 1; i >= 0; --i) { R.Add(L[i]); }
			L = R;
		};
		if (Area(Outer) < 0) { Reverse(Outer); }
		if (Area(Hole) > 0) { Reverse(Hole); }
		int32 Hi = 0;
		for (int32 i = 1; i < Hole.Num(); ++i) { if (To2(Hole[i]).X > To2(Hole[Hi]).X) { Hi = i; } }
		const FVector2D A = To2(Hole[Hi]);
		// Proper crossing of segments (A, B) and (C, D), ignoring shared endpoints.
		auto Crosses = [](const FVector2D& P0, const FVector2D& P1, const FVector2D& Q0, const FVector2D& Q1)
		{
			const double Eps = 1e-12;
			auto Same = [](const FVector2D& X, const FVector2D& Y) { return FVector2D::DistSquared(X, Y) < 1e-18; };
			if (Same(P0, Q0) || Same(P0, Q1) || Same(P1, Q0) || Same(P1, Q1)) { return false; }
			const double D1 = Cross2(P1 - P0, Q0 - P0), D2 = Cross2(P1 - P0, Q1 - P0);
			const double D3 = Cross2(Q1 - Q0, P0 - Q0), D4 = Cross2(Q1 - Q0, P1 - Q0);
			return ((D1 > Eps && D2 < -Eps) || (D1 < -Eps && D2 > Eps)) && ((D3 > Eps && D4 < -Eps) || (D3 < -Eps && D4 > Eps));
		};
		TArray<int32> Order;
		for (int32 i = 0; i < Outer.Num(); ++i) { Order.Add(i); }
		Order.Sort([&](int32 L, int32 R) { return FVector2D::DistSquared(To2(Outer[L]), A) < FVector2D::DistSquared(To2(Outer[R]), A); });
		int32 Oi = Order[0];
		for (const int32 Cand : Order)
		{
			const FVector2D B = To2(Outer[Cand]);
			bool bClear = true;
			for (const TArray<FVector>* L : {&Outer, &Hole})
			{
				for (int32 i = 0; i < L->Num() && bClear; ++i)
				{
					if (Crosses(A, B, To2((*L)[i]), To2((*L)[(i + 1) % L->Num()]))) { bClear = false; }
				}
			}
			if (bClear) { Oi = Cand; break; }
		}
		TArray<FVector> Pts;
		for (int32 i = 0; i <= Oi; ++i) { Pts.Add(Outer[i]); }
		for (int32 k = 0; k <= Hole.Num(); ++k) { Pts.Add(Hole[(Hi + k) % Hole.Num()]); }
		for (int32 i = Oi; i < Outer.Num(); ++i) { Pts.Add(Outer[i]); }
		return Poly(M, Pts, Nn);
	}

	/**
	 * Triangulates the band between two chains of vertices already in the mesh, each ordered by a key that grows along
	 * the band (height up a wall strip, distance along a path, angle round a centre): every triangle takes one edge
	 * from a chain and a vertex from the other. The chains' first vertices face each other, and so do their last.
	 */
	inline void Zip(FMesh& M, const TArray<int32>& A, const TArray<double>& KeyA, const TArray<int32>& B, const TArray<double>& KeyB)
	{
		int32 i = 0, j = 0;
		while (i + 1 < A.Num() || j + 1 < B.Num())
		{
			bool bAdvanceA;
			if (i + 1 >= A.Num()) { bAdvanceA = false; }
			else if (j + 1 >= B.Num()) { bAdvanceA = true; }
			else { bAdvanceA = KeyA[i + 1] <= KeyB[j + 1]; }
			if (bAdvanceA)
			{
				M.Tri(A[i], A[i + 1], B[j]);
				++i;
			}
			else
			{
				M.Tri(A[i], B[j + 1], B[j]);
				++j;
			}
		}
	}

	/** One point of a moulding's section. A smooth point averages the normals of the two segments that meet there. */
	struct FProfile
	{
		TArray<FVector2D> P;
		TArray<bool> Smooth;

		FProfile& Add(double A, double B, bool bSmooth = false)
		{
			P.Add(FVector2D(A, B));
			Smooth.Add(bSmooth);
			return *this;
		}

		/** The ellipse arc (CA + RA cos t, CB + RB sin t) from T0 to T1 degrees, after its first point (already added). */
		FProfile& Arc(double CA, double CB, double RA, double RB, double T0, double T1, int32 Segments)
		{
			for (int32 i = 1; i <= Segments; ++i)
			{
				const double T = Rad(T0 + (T1 - T0) * i / Segments);
				Add(CA + RA * FMath::Cos(T), CB + RB * FMath::Sin(T), i < Segments);
			}
			return *this;
		}

		/** A quarter or half round from the last point to (A, B), bulging to the right of the direction of travel (convex). */
		FProfile& Round(double A, double B, int32 Segments, bool bConvex = true)
		{
			const FVector2D S = P.Last();
			const FVector2D E(A, B);
			const FVector2D Mid = (S + E) * 0.5;
			const FVector2D D = E - S;
			const double HalfChord = D.Size() * 0.5;
			// A quarter circle: the centre is off the chord's midpoint by half the chord.
			FVector2D Side(D.Y, -D.X);
			Side = Unit(Side);
			const FVector2D Centre = Mid - Side * (bConvex ? HalfChord : -HalfChord);
			const double R = (S - Centre).Size();
			const double T0 = FMath::Atan2(S.Y - Centre.Y, S.X - Centre.X);
			double T1 = FMath::Atan2(E.Y - Centre.Y, E.X - Centre.X);
			// Go the short way round.
			while (T1 - T0 > Pi) { T1 -= 2 * Pi; }
			while (T1 - T0 < -Pi) { T1 += 2 * Pi; }
			for (int32 i = 1; i <= Segments; ++i)
			{
				const double T = T0 + (T1 - T0) * i / Segments;
				Add(Centre.X + R * FMath::Cos(T), Centre.Y + R * FMath::Sin(T), i < Segments);
			}
			return *this;
		}

		/** An S-curve (cyma) from the last point to (A, B): two quarter rounds, the first convex if bConvexFirst. */
		FProfile& Cyma(double A, double B, int32 Segments, bool bConvexFirst)
		{
			const FVector2D S = P.Last();
			const FVector2D Mid = (S + FVector2D(A, B)) * 0.5;
			Round(Mid.X, Mid.Y, Segments, bConvexFirst);
			Smooth.Last() = true;
			return Round(A, B, Segments, !bConvexFirst);
		}

		int32 Num() const { return P.Num(); }
	};

	/** Where a section lands along a sweep: (A, B) goes to Origin + A * AxisA + B * AxisB; its normals use NormA, NormB. */
	struct FSweepFrame
	{
		FVector Origin = FVector::ZeroVector;
		FVector AxisA = FVector::ForwardVector;
		FVector AxisB = FVector::UpVector;
		FVector NormA = FVector::ForwardVector;
		FVector NormB = FVector::UpVector;
		double S = 0.0;   // distance along the path, m (the U of the UVs)

		FVector At(const FVector2D& Q) const { return Origin + AxisA * Q.X + AxisB * Q.Y; }
	};

	/** The outward 2D normals of a profile's segments ((dB, -dA): the solid on the left), and smooth point normals. */
	inline void ProfileNormals(const FProfile& Profile, bool bClosed, TArray<FVector2D>& SegN, TArray<FVector2D>& Start, TArray<FVector2D>& End)
	{
		const int32 NP = Profile.P.Num();
		const int32 NSeg = bClosed ? NP : NP - 1;
		for (int32 j = 0; j < NSeg; ++j)
		{
			const FVector2D D = Profile.P[(j + 1) % NP] - Profile.P[j];
			SegN.Add(Unit(FVector2D(D.Y, -D.X)));
		}
		for (int32 j = 0; j < NSeg; ++j)
		{
			FVector2D N0 = SegN[j], N1 = SegN[j];
			const int32 P0 = j, P1 = (j + 1) % NP;
			if (Profile.Smooth[P0])
			{
				const int32 Prev = j - 1;
				if (Prev >= 0) { N0 = Unit((SegN[j] + SegN[Prev])); }
				else if (bClosed) { N0 = Unit((SegN[j] + SegN[NSeg - 1])); }
			}
			if (Profile.Smooth[P1])
			{
				const int32 Next = j + 1;
				if (Next < NSeg) { N1 = Unit((SegN[j] + SegN[Next])); }
				else if (bClosed) { N1 = Unit((SegN[j] + SegN[0])); }
			}
			Start.Add(N0);
			End.Add(N1);
		}
	}

	/**
	 * Sweeps a section through a run of frames: smooth along the run, crisp or smooth across the section. Segments of the
	 * section for which UseAlt is true go to Alt (another material), sharing the rings where they meet.
	 */
	inline void Sweep(FMesh& MainMesh, const TArray<FSweepFrame>& Run, const FProfile& Profile, bool bClosedProfile, FMesh* Alt = nullptr,
					  TFunction<bool(int32)> UseAlt = nullptr)
	{
		const int32 NP = Profile.P.Num();
		if (Run.Num() < 2 || NP < 2) { return; }
		TArray<FVector2D> SegN, N0s, N1s;
		ProfileNormals(Profile, bClosedProfile, SegN, N0s, N1s);
		double Len = 0.0;
		for (int32 j = 0; j < SegN.Num(); ++j)
		{
			const FVector2D P0 = Profile.P[j], P1 = Profile.P[(j + 1) % NP];
			const double L1 = Len + (P1 - P0).Size();
			const FVector2D N0 = N0s[j], N1 = N1s[j];
			FMesh& M = (Alt && UseAlt && UseAlt(j)) ? *Alt : MainMesh;
			const int32 Base = M.Positions.Num();
			for (const FSweepFrame& F : Run)
			{
				M.Vertex(F.At(P0), F.NormA * N0.X + F.NormB * N0.Y, FVector2D(F.S, Len));
				M.Vertex(F.At(P1), F.NormA * N1.X + F.NormB * N1.Y, FVector2D(F.S, L1));
			}
			for (int32 k = 0; k + 1 < Run.Num(); ++k)
			{
				const int32 A = Base + 2 * k;
				M.Quad(A, A + 2, A + 3, A + 1);
			}
			Len = L1;
		}
	}

	/** Closes a (closed) section's end at a frame, facing Facing. */
	inline void Cap(FMesh& M, const FSweepFrame& F, const FProfile& Profile, const FVector& Facing)
	{
		TArray<FVector> Pts;
		for (const FVector2D& Q : Profile.P) { Pts.Add(F.At(Q)); }
		Poly(M, Pts, Facing);
	}

	/** Sweeps a closed section along a run and caps both ends: a closed solid. */
	inline void SweepSolid(FMesh& M, const TArray<FSweepFrame>& Run, const FProfile& Profile)
	{
		if (Run.Num() < 2) { return; }
		Sweep(M, Run, Profile, true);
		const FVector D0 = Unit((Run[0].Origin - Run[1].Origin));
		const FVector D1 = Unit((Run.Last().Origin - Run[Run.Num() - 2].Origin));
		Cap(M, Run[0], Profile, D0);
		Cap(M, Run.Last(), Profile, D1);
	}

	/**
	 * Frames along a horizontal plan path (metres, at height Z): section A along the path's left normal (-dy, dx),
	 * mitred at corners; B up. Corners turning more than HardDegrees split the path into runs (crisp); gentler ones
	 * are smooth. A closed path's runs close on themselves.
	 */
	inline TArray<TArray<FSweepFrame>> PlanRuns(const TArray<FVector2D>& Path, bool bClosed, double Z = 0.0, double HardDegrees = 20.0)
	{
		TArray<TArray<FSweepFrame>> Runs;
		const int32 N = Path.Num();
		if (N < 2) { return Runs; }
		const int32 NSeg = bClosed ? N : N - 1;
		TArray<FVector2D> SegNrm;
		for (int32 i = 0; i < NSeg; ++i)
		{
			const FVector2D D = Unit((Path[(i + 1) % N] - Path[i]));
			SegNrm.Add(FVector2D(-D.Y, D.X));
		}
		auto InN = [&](int32 v) -> FVector2D { return (!bClosed && v == 0) ? SegNrm[0] : SegNrm[(v - 1 + NSeg) % NSeg]; };
		auto OutN = [&](int32 v) -> FVector2D { return (!bClosed && v == N - 1) ? SegNrm[NSeg - 1] : SegNrm[v % NSeg]; };
		const double HardCos = FMath::Cos(Rad(HardDegrees));
		auto IsHard = [&](int32 v) -> bool
		{
			if (!bClosed && (v == 0 || v == N - 1)) { return false; }
			return FVector2D::DotProduct(InN(v), OutN(v)) < HardCos;
		};
		auto MakeFrame = [&](int32 v, const FVector2D& NA, double PathS) -> FSweepFrame
		{
			const FVector2D Mitre = Unit((InN(v) + OutN(v)));
			const double Dot = FMath::Max(0.2, FVector2D::DotProduct(Mitre, InN(v)));
			FSweepFrame F;
			F.Origin = FVector(Path[v].X, Path[v].Y, Z);
			F.AxisA = FVector(Mitre.X, Mitre.Y, 0.0) / Dot;
			F.NormA = Unit(FVector(NA.X, NA.Y, 0.0));
			F.AxisB = F.NormB = FVector::UpVector;
			F.S = PathS;
			return F;
		};
		int32 Start = 0;
		if (bClosed)
		{
			for (int32 v = 0; v < N; ++v)
			{
				if (IsHard(v)) { Start = v; break; }
			}
		}
		const int32 Steps = bClosed ? N : N - 1;
		TArray<FSweepFrame> Cur;
		double Dist = 0.0;
		for (int32 k = 0; k <= Steps; ++k)
		{
			const int32 v = (Start + k) % N;
			if (k > 0) { Dist += FVector2D::Distance(Path[(Start + k - 1) % N], Path[v]); }
			const FVector2D SmoothN = Unit((InN(v) + OutN(v)));
			if (k == 0) { Cur.Add(MakeFrame(v, IsHard(v) ? OutN(v) : SmoothN, Dist)); }
			else if (k == Steps)
			{
				Cur.Add(MakeFrame(v, IsHard(v) ? InN(v) : SmoothN, Dist));
				Runs.Add(Cur);
			}
			else if (IsHard(v))
			{
				Cur.Add(MakeFrame(v, InN(v), Dist));
				Runs.Add(Cur);
				Cur.Reset();
				Cur.Add(MakeFrame(v, OutN(v), Dist));
			}
			else { Cur.Add(MakeFrame(v, SmoothN, Dist)); }
		}
		return Runs;
	}

	/**
	 * Sweeps a moulding's section along a plan path. bSolid: the section is closed and an open path is capped at both
	 * ends (a closed solid). Otherwise the section is an open curve (a loop round a block whose top and bottom the caller
	 * closes).
	 */
	inline void SweepPlan(FMesh& M, const TArray<FVector2D>& Path, bool bClosedPath, const FProfile& Profile, bool bSolid, double Z = 0.0,
						  double HardDegrees = 20.0)
	{
		const TArray<TArray<FSweepFrame>> Runs = PlanRuns(Path, bClosedPath, Z, HardDegrees);
		for (const TArray<FSweepFrame>& Run : Runs) { Sweep(M, Run, Profile, bSolid); }
		if (bSolid && !bClosedPath && Runs.Num() > 0)
		{
			const TArray<FSweepFrame>& First = Runs[0];
			const TArray<FSweepFrame>& Last = Runs.Last();
			Cap(M, First[0], Profile, Unit((First[0].Origin - First[1].Origin)));
			Cap(M, Last.Last(), Profile, Unit((Last.Last().Origin - Last[Last.Num() - 2].Origin)));
		}
	}

	/** The frames' positions of one section point along a plan path's runs, in order (for closing a block's top or bottom). */
	inline TArray<FVector> PlanOutline(const TArray<FVector2D>& Path, bool bClosedPath, const FVector2D& Q, double Z = 0.0, double HardDegrees = 20.0)
	{
		TArray<FVector> Out;
		for (const TArray<FSweepFrame>& Run : PlanRuns(Path, bClosedPath, Z, HardDegrees))
		{
			for (int32 i = 0; i < Run.Num(); ++i)
			{
				const FVector P = Run[i].At(Q);
				if (Out.Num() > 0 && FVector::DistSquared(Out.Last(), P) < 1e-14) { continue; }
				Out.Add(P);
			}
		}
		if (bClosedPath && Out.Num() > 1 && FVector::DistSquared(Out[0], Out.Last()) < 1e-14) { Out.Pop(); }
		return Out;
	}

	/**
	 * Turns a section (radius, height) about an axis through Origin (Ref: the direction of angle 0, Axis: up), from A0 to
	 * A1 radians in Segments steps. Trace the section with the solid on its left (outward normals (dh, -dr)). A full turn
	 * shares its seam; a partial one is capped at both ends if bCapEnds (the section must then be closed).
	 * AltWrap > 0: the Alt part's U runs 0 … AltWrap (m) once round, whatever the radius, so a stone image whose repeat
	 * divides AltWrap closes without a seam (a column's shaft; it also draws the veins out along the shaft).
	 */
	inline void Lathe(FMesh& M, const FVector& Origin, const FVector& Axis, const FVector& Ref, const FProfile& Profile, double A0, double A1,
					  int32 Segments, bool bClosedProfile = false, bool bCapEnds = false, FMesh* Alt = nullptr, TFunction<bool(int32)> UseAlt = nullptr,
					  double AltWrap = 0.0)
	{
		const FVector Up = Unit(Axis);
		const FVector X0 = Unit((Ref - Up * FVector::DotProduct(Ref, Up)));
		const FVector Y0 = FVector::CrossProduct(Up, X0);
		TArray<FSweepFrame> Run;
		for (int32 i = 0; i <= Segments; ++i)
		{
			const double T = A0 + (A1 - A0) * i / Segments;
			FSweepFrame F;
			F.Origin = Origin;
			F.AxisA = F.NormA = X0 * FMath::Cos(T) + Y0 * FMath::Sin(T);
			F.AxisB = F.NormB = Up;
			F.S = T;
			Run.Add(F);
		}
		const int32 Base = M.Positions.Num(), AltBase = Alt ? Alt->Positions.Num() : 0;
		Sweep(M, Run, Profile, bClosedProfile, Alt, UseAlt);
		// U in metres round the axis at each vertex's own radius (the sweep wrote the angle).
		auto Metres = [&Origin, &Up](FMesh& Target, int32 From)
		{
			for (int32 i = From; i < Target.Positions.Num(); ++i)
			{
				const FVector D = Target.Positions[i] - Origin;
				const double R = (D - Up * FVector::DotProduct(D, Up)).Size();
				Target.UVs[i].X *= R;
			}
		};
		Metres(M, Base);
		if (Alt && AltWrap > 0.0)
		{
			for (int32 i = AltBase; i < Alt->Positions.Num(); ++i) { Alt->UVs[i].X *= AltWrap / (2.0 * Pi); }
		}
		else if (Alt) { Metres(*Alt, AltBase); }
		if (bCapEnds)
		{
			const FVector Side0 = FVector::CrossProduct(Run[0].AxisA, Up) * ((A1 > A0) ? 1.0 : -1.0);
			Cap(M, Run[0], Profile, Side0);
			Cap(M, Run.Last(), Profile, -FVector::CrossProduct(Run.Last().AxisA, Up) * ((A1 > A0) ? 1.0 : -1.0));
		}
	}

	/** A closed box on its own axes: centre, half extents along X, Y, Z of the given frame. */
	inline void Box(FMesh& M, const FVector& Centre, const FVector& AxisX, const FVector& AxisY, const FVector& AxisZ, const FVector& Half)
	{
		const FVector X = Unit(AxisX) * Half.X, Y = Unit(AxisY) * Half.Y, Z = Unit(AxisZ) * Half.Z;
		auto Corner = [&](int32 sx, int32 sy, int32 sz) { return Centre + X * sx + Y * sy + Z * sz; };
		const FVector Ns[6] = {-X, X, -Y, Y, -Z, Z};
		const int32 Faces[6][4][3] = {
			{{-1, -1, -1}, {-1, 1, -1}, {-1, 1, 1}, {-1, -1, 1}}, {{1, -1, -1}, {1, 1, -1}, {1, 1, 1}, {1, -1, 1}},
			{{-1, -1, -1}, {1, -1, -1}, {1, -1, 1}, {-1, -1, 1}}, {{-1, 1, -1}, {1, 1, -1}, {1, 1, 1}, {-1, 1, 1}},
			{{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}}, {{-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}}};
		for (int32 f = 0; f < 6; ++f)
		{
			const FVector N = Unit(Ns[f]);
			int32 Id[4];
			for (int32 c = 0; c < 4; ++c)
			{
				const FVector P = Corner(Faces[f][c][0], Faces[f][c][1], Faces[f][c][2]);
				Id[c] = M.Vertex(P, N);
			}
			M.Quad(Id[0], Id[1], Id[2], Id[3]);
		}
	}

	/** An axis-aligned box from Lo to Hi. */
	inline void Box(FMesh& M, const FVector& Lo, const FVector& Hi)
	{
		Box(M, (Lo + Hi) * 0.5, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), (Hi - Lo) * 0.5);
	}

	/**
	 * A smooth (NU + 1) x (NV + 1) grid of points. Normals come from the grid's own differences (smooth across it,
	 * crisp at its edges) and are turned to face Toward(P).
	 */
	inline void Patch(FMesh& M, int32 NU, int32 NV, TFunctionRef<FVector(int32, int32)> Pos, TFunctionRef<FVector2D(int32, int32)> UV,
					  TFunctionRef<FVector(const FVector&)> Toward)
	{
		const int32 W = NV + 1;
		TArray<FVector> G;
		G.SetNum((NU + 1) * W);
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
				const FVector Hint = Toward(P);
				FVector N = FVector::CrossProduct(DU, DV);
				if (N.SizeSquared() < 1e-24) { N = Hint; }
				else if (FVector::DotProduct(N, Hint) < 0.0) { N = -N; }
				M.Vertex(P, N, UV(i, j));
			}
		}
		for (int32 i = 0; i < NU; ++i)
		{
			for (int32 j = 0; j < NV; ++j) { M.Quad(Base + i * W + j, Base + (i + 1) * W + j, Base + (i + 1) * W + j + 1, Base + i * W + j + 1); }
		}
	}

	/** Sorted, without values closer than Eps (the first of a cluster is kept). */
	inline TArray<double> Unique(TArray<double> Values, double Eps = 1e-6)
	{
		Values.Sort();
		TArray<double> Out;
		for (const double V : Values)
		{
			if (Out.Num() == 0 || V - Out.Last() > Eps) { Out.Add(V); }
		}
		return Out;
	}

	/** The value of a polyline (X ascending) at X: exact at its vertices, linear between. */
	inline double Interp(const TArray<FVector2D>& Line, double X)
	{
		if (Line.Num() == 0) { return 0.0; }
		if (X <= Line[0].X) { return Line[0].Y; }
		for (int32 i = 0; i + 1 < Line.Num(); ++i)
		{
			if (FMath::Abs(X - Line[i].X) < 1e-9) { return Line[i].Y; }
			if (X < Line[i + 1].X)
			{
				const double T = (X - Line[i].X) / FMath::Max(Line[i + 1].X - Line[i].X, 1e-30);
				return FMath::Lerp(Line[i].Y, Line[i + 1].Y, T);
			}
		}
		return Line.Last().Y;
	}

	/**
	 * An opening in a wall face, in the face's (u along, h up) coordinates: jambs at U0 and U1 from Sill up, a head
	 * polyline from (U0, jamb top) to (U1, jamb top), u ascending. Every u of the head is a station of the face.
	 */
	struct FOpening
	{
		double U0 = 0.0, U1 = 0.0;
		double Sill = 0.0;
		TArray<FVector2D> Head;

		double HeadAt(double U) const { return Interp(Head, U); }
	};

	/**
	 * A wall face between stations (u ascending): in each strip the wall runs from Bottom to Top (piecewise linear, taken
	 * at the strip's ends; Mid, the strip's middle, picks the regime where they step), less any opening the strip lies in.
	 * The face is split at SplitH between two meshes (a dado and the field above it). Every station line carries the
	 * vertices of both strips beside it and of Extra(u), so edges shared with other surfaces (the floor, the vault, reveals,
	 * niches) meet vertex for vertex.
	 */
	struct FWallFace
	{
		TFunction<FVector(double, double)> Place;    // (u, h) → point
		TFunction<FVector(double, double)> Normal;   // facing the room
		TFunction<double(double, double)> Bottom;    // (u, strip middle) → h
		TFunction<double(double, double)> Top;
		TArray<FOpening> Openings;
		TArray<double> Stations;
		double SplitH = -1e9;
		TFunction<TArray<double>(double)> Extra;     // more levels on a station line
		mutable TArray<TArray<double>> Levels;        // each station line's vertex heights (after Build)

		const FOpening* OpeningAt(double Mid) const
		{
			for (const FOpening& O : Openings)
			{
				if (Mid > O.U0 && Mid < O.U1) { return &O; }
			}
			return nullptr;
		}

		/** The solid pieces (lo, hi) of the strip whose middle is Mid, evaluated at U. */
		TArray<FVector2D> Pieces(double U, double Mid) const
		{
			TArray<FVector2D> Out;
			const double B = Bottom(U, Mid), T = Top(U, Mid);
			const double BM = Bottom(Mid, Mid), TM = Top(Mid, Mid);
			if (const FOpening* O = OpeningAt(Mid))
			{
				if (O->Sill > BM + 1e-9) { Out.Add(FVector2D(B, O->Sill)); }
				if (TM > O->HeadAt(Mid) + 1e-9) { Out.Add(FVector2D(O->HeadAt(U), T)); }
			}
			else if (TM > BM + 1e-9) { Out.Add(FVector2D(B, T)); }
			return Out;
		}

		void Build(FMesh& Lower, FMesh& Upper) const
		{
			const int32 NS = Stations.Num();
			Levels.Reset();
			Levels.SetNum(NS);
			for (int32 i = 0; i + 1 < NS; ++i)
			{
				const double Mid = 0.5 * (Stations[i] + Stations[i + 1]);
				for (int32 e = 0; e < 2; ++e)
				{
					const double U = Stations[i + e];
					for (const FVector2D& Pc : Pieces(U, Mid))
					{
						Levels[i + e].Add(Pc.X);
						Levels[i + e].Add(Pc.Y);
						if (SplitH > Pc.X && SplitH < Pc.Y) { Levels[i + e].Add(SplitH); }
					}
				}
			}
			for (int32 i = 0; i < NS; ++i)
			{
				if (Extra) { Levels[i].Append(Extra(Stations[i])); }
				Levels[i] = Unique(Levels[i], 1e-7);
			}
			for (int32 i = 0; i + 1 < NS; ++i)
			{
				const double U0 = Stations[i], U1 = Stations[i + 1];
				const double Mid = 0.5 * (U0 + U1);
				const TArray<FVector2D> L = Pieces(U0, Mid), R = Pieces(U1, Mid);
				for (int32 p = 0; p < L.Num() && p < R.Num(); ++p)
				{
					// Split at SplitH into the two meshes.
					for (int32 Part = 0; Part < 2; ++Part)
					{
						const double Lo0 = Part == 0 ? L[p].X : FMath::Max(L[p].X, SplitH), Hi0 = Part == 0 ? FMath::Min(L[p].Y, SplitH) : L[p].Y;
						const double Lo1 = Part == 0 ? R[p].X : FMath::Max(R[p].X, SplitH), Hi1 = Part == 0 ? FMath::Min(R[p].Y, SplitH) : R[p].Y;
						if (Hi0 <= Lo0 + 1e-9 && Hi1 <= Lo1 + 1e-9) { continue; }
						FMesh& M = Part == 0 ? Lower : Upper;
						TArray<int32> CA, CB;
						TArray<double> KA, KB;
						for (const double H : Levels[i])
						{
							if (H >= Lo0 - 1e-9 && H <= Hi0 + 1e-9)
							{
								CA.Add(M.Vertex(Place(U0, H), Normal(U0, H), FVector2D(U0, -H)));
								KA.Add(H);
							}
						}
						for (const double H : Levels[i + 1])
						{
							if (H >= Lo1 - 1e-9 && H <= Hi1 + 1e-9)
							{
								CB.Add(M.Vertex(Place(U1, H), Normal(U1, H), FVector2D(U1, -H)));
								KB.Add(H);
							}
						}
						if (CA.Num() + CB.Num() >= 3) { Zip(M, CA, KA, CB, KB); }
					}
				}
			}
		}
	};
}
