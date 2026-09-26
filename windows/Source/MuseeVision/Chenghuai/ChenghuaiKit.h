#pragma once

#include "CoreMinimal.h"
#include "Algo/Reverse.h"
#include "Salon/SalonKit.h"

/**
 * Geometry kit for AChenghuaiStructure (copied from the Chinese Wing's ChineseWingKit, which is being retired): SalonKit's indexed mesh (positions in
 * plan metres written in centimetres, UVs in metres, every triangle wound to face along its vertex normals), a record
 * of the solids each section is made of (for the checks in Scripts: every solid closed, normals outwards), and the
 * solids the wing is built from: boxes and grid solids, prisms, extruded outlines, lathes and swept sections.
 */
namespace ChenghuaiKit
{
	using SalonKit::FMeshData;
	using SalonKit::FFrame;
	using SalonKit::FProfile;

	/** A run of one section's triangles that makes one solid (bClosed) or an open sheet; Assembly groups runs across sections. */
	struct FGroup
	{
		FString Name;
		FString Assembly;
		int32 FirstIndex = 0;
		bool bClosed = true;
	};

	/** One mesh section and the solids in it. */
	struct FPart
	{
		FMeshData M;
		TArray<FGroup> Groups;

		/** Starts the next solid. Runs of different sections with the same Assembly are checked as one solid. */
		void Begin(const FString& Name, bool bClosed = true, const FString& Assembly = FString())
		{
			FGroup G;
			G.Name = Name;
			G.Assembly = Assembly.IsEmpty() ? Name : Assembly;
			G.FirstIndex = M.Indices.Num();
			G.bClosed = bClosed;
			Groups.Add(G);
		}
	};

	inline double Cross2(const FVector2D& A, const FVector2D& B) { return A.X * B.Y - A.Y * B.X; }

	/** Twice the signed area of an outline (positive: counter-clockwise, x right and y up). */
	inline double Area2(const TArray<FVector2D>& P)
	{
		double A = 0.0;
		for (int32 i = 0, j = P.Num() - 1; i < P.Num(); j = i++) { A += Cross2(P[j], P[i]); }
		return A;
	}

	/** Strictly inside the counter-clockwise triangle A, B, C (a point on an edge doesn't count). */
	inline bool StrictlyInside(const FVector2D& Q, const FVector2D& A, const FVector2D& B, const FVector2D& C)
	{
		constexpr double Eps = 1e-12;
		return Cross2(B - A, Q - A) > Eps && Cross2(C - B, Q - B) > Eps && Cross2(A - C, Q - C) > Eps;
	}

	/** Ear clipping of a simple outline (either winding; repeated points from bridged holes are fine): index triples into P. */
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
				// Only flat corners left (or rounding): clip the flattest.
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

	/** Whether segments AB and CD cross (touching at an end doesn't count). */
	inline bool SegmentsCross(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FVector2D& D)
	{
		const double D1 = Cross2(B - A, C - A), D2 = Cross2(B - A, D - A), D3 = Cross2(D - C, A - C), D4 = Cross2(D - C, B - C);
		return ((D1 > 1e-12 && D2 < -1e-12) || (D1 < -1e-12 && D2 > 1e-12)) && ((D3 > 1e-12 && D4 < -1e-12) || (D3 < -1e-12 && D4 > 1e-12));
	}

	/** One outline from an outer ring and its holes, each joined to the outer ring by a bridge, for EarClip. */
	inline TArray<FVector2D> Bridge(TArray<FVector2D> Outer, const TArray<TArray<FVector2D>>& Holes)
	{
		if (Area2(Outer) < 0.0) { Algo::Reverse(Outer); }
		for (TArray<FVector2D> Hole : Holes)
		{
			if (Hole.Num() < 3) { continue; }
			if (Area2(Hole) > 0.0) { Algo::Reverse(Hole); }
			int32 From = 0;
			for (int32 h = 1; h < Hole.Num(); ++h) { if (Hole[h].X > Hole[From].X) { From = h; } }
			int32 To = INDEX_NONE;
			double BestDist = TNumericLimits<double>::Max();
			for (int32 o = 0; o < Outer.Num(); ++o)
			{
				const double Dist = FVector2D::DistSquared(Outer[o], Hole[From]);
				if (Dist >= BestDist) { continue; }
				bool bBlocked = false;
				for (const TArray<FVector2D>* Ring : {&Outer, &Hole})
				{
					for (int32 k = 0; k < Ring->Num() && !bBlocked; ++k)
					{
						bBlocked = SegmentsCross(Outer[o], Hole[From], (*Ring)[k], (*Ring)[(k + 1) % Ring->Num()]);
					}
				}
				if (!bBlocked) { To = o; BestDist = Dist; }
			}
			if (To == INDEX_NONE) { continue; }
			TArray<FVector2D> Merged;
			for (int32 o = 0; o <= To; ++o) { Merged.Add(Outer[o]); }
			for (int32 h = 0; h <= Hole.Num(); ++h) { Merged.Add(Hole[(From + h) % Hole.Num()]); }
			for (int32 o = To; o < Outer.Num(); ++o) { Merged.Add(Outer[o]); }
			Outer = MoveTemp(Merged);
		}
		return Outer;
	}

	/** A flat outline (with holes) on the plane Origin + AxisU x + AxisV y, facing N; UVs from UVOf(P). */
	inline void Planar(FMeshData& M, const TArray<FVector2D>& Outer, const TArray<TArray<FVector2D>>& Holes, const FVector& Origin,
					   const FVector& AxisU, const FVector& AxisV, const FVector& N, TFunctionRef<FVector2D(const FVector&)> UVOf)
	{
		const TArray<FVector2D> Pts = Holes.Num() > 0 ? Bridge(Outer, Holes) : Outer;
		const TArray<int32> T = EarClip(Pts);
		const int32 Base = M.Positions.Num();
		for (const FVector2D& Q : Pts)
		{
			const FVector P = Origin + AxisU * Q.X + AxisV * Q.Y;
			M.Vertex(P, N, UVOf(P));
		}
		for (int32 t = 0; t + 2 < T.Num(); t += 3) { M.Tri(Base + T[t], Base + T[t + 1], Base + T[t + 2]); }
	}

	/** Plan UVs (floors, roofs' tops) and the metre UVs of a flat face. */
	inline FVector2D PlanUV(const FVector& P) { return FVector2D(P.X, P.Y); }

	/** A level outline (with holes) at height Z, facing up or down. */
	inline void Level(FMeshData& M, const TArray<FVector2D>& Outer, const TArray<TArray<FVector2D>>& Holes, double Z, bool bUp)
	{
		Planar(M, Outer, Holes, FVector(0, 0, Z), FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, bUp ? 1 : -1), &PlanUV);
	}

	/** A closed box. */
	inline void Box(FPart& Part, const FString& Name, const FVector& Lo, const FVector& Hi)
	{
		Part.Begin(Name);
		Part.M.Box(Lo, Hi, FMeshData::AllFaces);
	}

	/**
	 * A solid made of the cells of a grid (cuts along x, y and z): every face between a filled cell and an empty one
	 * (or the outside), facing out. Closed, and free of T-junctions, whatever the cells.
	 */
	inline void GridSolid(FPart& Part, const FString& Name, const TArray<double>& Xs, const TArray<double>& Ys, const TArray<double>& Zs,
						  TFunctionRef<bool(int32, int32, int32)> Filled)
	{
		Part.Begin(Name);
		FMeshData& M = Part.M;
		auto In = [&](int32 I, int32 J, int32 K)
		{
			return I >= 0 && J >= 0 && K >= 0 && I + 1 < Xs.Num() && J + 1 < Ys.Num() && K + 1 < Zs.Num() && Filled(I, J, K);
		};
		const TArray<double>* Axes[3] = {&Xs, &Ys, &Zs};
		for (int32 I = 0; I + 1 < Xs.Num(); ++I)
		{
			for (int32 J = 0; J + 1 < Ys.Num(); ++J)
			{
				for (int32 K = 0; K + 1 < Zs.Num(); ++K)
				{
					if (!Filled(I, J, K)) { continue; }
					const int32 Cell[3] = {I, J, K};
					for (int32 Axis = 0; Axis < 3; ++Axis)
					{
						for (const int32 Dir : {-1, 1})
						{
							int32 Next[3] = {I, J, K};
							Next[Axis] += Dir;
							if (In(Next[0], Next[1], Next[2])) { continue; }
							const int32 UAxis = (Axis + 1) % 3, VAxis = (Axis + 2) % 3;
							const double Plane = (*Axes[Axis])[Cell[Axis] + (Dir > 0 ? 1 : 0)];
							const double U0 = (*Axes[UAxis])[Cell[UAxis]], U1 = (*Axes[UAxis])[Cell[UAxis] + 1];
							const double V0 = (*Axes[VAxis])[Cell[VAxis]], V1 = (*Axes[VAxis])[Cell[VAxis] + 1];
							FVector Nrm = FVector::ZeroVector;
							Nrm[Axis] = Dir;
							auto Corner = [&](double U, double V)
							{
								FVector P;
								P[Axis] = Plane;
								P[UAxis] = U;
								P[VAxis] = V;
								return M.Vertex(P, Nrm, SalonKit::FaceUV(P, Nrm));
							};
							M.Quad(Corner(U0, V0), Corner(U1, V0), Corner(U1, V1), Corner(U0, V1));
						}
					}
				}
			}
		}
	}

	/**
	 * A closed prism: an outline in the plane Origin + AxisU u + AxisV v, from W0 to W1 along AxisW. With bSmoothSides
	 * the side normals are blended round the outline (a rounded stone), otherwise every side is flat.
	 */
	inline void Extrude(FPart& Part, const FString& Name, const TArray<FVector2D>& Outline, const FVector& Origin, const FVector& AxisU,
						const FVector& AxisV, const FVector& AxisW, double W0, double W1, bool bSmoothSides = false)
	{
		Part.Begin(Name);
		FMeshData& M = Part.M;
		const FVector Wn = AxisW.GetSafeNormal();
		auto FaceUVOf = [](const FVector& Facing) { return [Facing](const FVector& P) { return SalonKit::FaceUV(P, Facing); }; };
		Planar(M, Outline, {}, Origin + AxisW * W1, AxisU, AxisV, Wn, FaceUVOf(Wn));
		Planar(M, Outline, {}, Origin + AxisW * W0, AxisU, AxisV, -Wn, FaceUVOf(-Wn));
		const int32 N = Outline.Num();
		const double Sign = Area2(Outline) > 0.0 ? 1.0 : -1.0;
		TArray<FVector> EdgeN;
		for (int32 e = 0; e < N; ++e)
		{
			const FVector2D D = (Outline[(e + 1) % N] - Outline[e]).GetSafeNormal();
			EdgeN.Add((AxisU * D.Y - AxisV * D.X).GetSafeNormal() * Sign);
		}
		double Along = 0.0;
		const double Depth = (W1 - W0) * AxisW.Size();
		for (int32 e = 0; e < N; ++e)
		{
			const FVector2D A2 = Outline[e], B2 = Outline[(e + 1) % N];
			const FVector NA = bSmoothSides ? (EdgeN[(e + N - 1) % N] + EdgeN[e]).GetSafeNormal() : EdgeN[e];
			const FVector NB = bSmoothSides ? (EdgeN[e] + EdgeN[(e + 1) % N]).GetSafeNormal() : EdgeN[e];
			const FVector A = Origin + AxisU * A2.X + AxisV * A2.Y, B = Origin + AxisU * B2.X + AxisV * B2.Y;
			const double L = FVector2D::Distance(A2, B2);
			const int32 V0 = M.Vertex(A + AxisW * W0, NA, FVector2D(Along, 0.0));
			const int32 V1 = M.Vertex(B + AxisW * W0, NB, FVector2D(Along + L, 0.0));
			const int32 V2 = M.Vertex(B + AxisW * W1, NB, FVector2D(Along + L, Depth));
			const int32 V3 = M.Vertex(A + AxisW * W1, NA, FVector2D(Along, Depth));
			M.Quad(V0, V1, V2, V3);
			Along += L;
		}
	}

	/** A vertical prism: a plan outline from Z0 to Z1. */
	inline void Prism(FPart& Part, const FString& Name, const TArray<FVector2D>& Outline, double Z0, double Z1, bool bSmoothSides = false)
	{
		Extrude(Part, Name, Outline, FVector::ZeroVector, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), Z0, Z1, bSmoothSides);
	}

	/**
	 * A profile (radius, height along Axis) turned round Axis from Base. Trace it from the axis at the bottom outwards,
	 * up and back to the axis (the solid on its left): the ends on the axis close it.
	 */
	/** Each timber member starts its U at its own multiple of 40 m (from where it stands), so the oak's material gives it
	 *  its own slice of the veneer and its own tone (M_Wood BoardLength 40): no two beams alike, as no two timbers are. */
	inline void OffsetMemberU(FMeshData& M, int32 FirstVertex, const FVector& At)
	{
		const double MemberU = 40.0 * (FMath::Abs(FMath::FloorToInt(At.X * 97.0 + At.Y * 57.0 + At.Z * 31.0)) % 29);
		for (int32 i = FirstVertex; i < M.UVs.Num(); ++i) { M.UVs[i].X += MemberU; }
	}

	/** bUAlongAxis: U runs up the profile and V round it (a timber column's grain runs along U, as a swept member's does). */
	inline void Lathe(FPart& Part, const FString& Name, const FVector& Base, const FVector& Axis, const FProfile& Profile, int32 Segments, double UVRadius,
					  bool bUAlongAxis = false)
	{
		Part.Begin(Name);
		const int32 FirstVertex = Part.M.UVs.Num();
		const FVector W = Axis.GetSafeNormal();
		const FVector Ref = FMath::Abs(W.Z) < 0.9 ? FVector::UpVector : FVector::ForwardVector;
		const FVector E1 = FVector::CrossProduct(W, Ref).GetSafeNormal();
		const FVector E2 = FVector::CrossProduct(W, E1).GetSafeNormal();
		TArray<FFrame> Run;
		for (int32 k = 0; k <= Segments; ++k)
		{
			const double T = 2.0 * UE_DOUBLE_PI * (k % Segments) / Segments;   // the last frame is the first again, exactly
			FFrame F;
			F.Origin = Base;
			F.AxisA = F.NormA = E1 * FMath::Cos(T) + E2 * FMath::Sin(T);
			F.AxisB = F.NormB = W;
			F.S = UVRadius * 2.0 * UE_DOUBLE_PI * k / Segments;
			Run.Add(F);
		}
		SalonKit::Sweep(Part.M, Run, Profile);
		if (bUAlongAxis)
		{
			for (int32 i = FirstVertex; i < Part.M.UVs.Num(); ++i) { Part.M.UVs[i] = FVector2D(Part.M.UVs[i].Y, Part.M.UVs[i].X); }
			OffsetMemberU(Part.M, FirstVertex, Base);
		}
	}

	/** A closed section swept through a run of frames and capped at both ends (the section must be convex). */
	inline void SweepSolid(FPart& Part, const FString& Name, const TArray<FFrame>& Run, const FProfile& Profile)
	{
		if (Run.Num() < 2) { return; }
		Part.Begin(Name);
		const int32 FirstVertex = Part.M.UVs.Num();
		SalonKit::Sweep(Part.M, Run, Profile);
		OffsetMemberU(Part.M, FirstVertex, Run[0].Origin);
		const FVector T0 = (Run[1].Origin - Run[0].Origin).GetSafeNormal();
		const FVector T1 = (Run.Last().Origin - Run[Run.Num() - 2].Origin).GetSafeNormal();
		SalonKit::CapConvex(Part.M, Run[0], Profile, -T0);
		SalonKit::CapConvex(Part.M, Run.Last(), Profile, T1);
	}

	/** Frames along a path with an upright section: A level and across the path (to its left), B straight up. */
	inline TArray<FFrame> UprightFrames(const TArray<FVector>& Path)
	{
		TArray<FFrame> Run;
		double S = 0.0;
		const int32 N = Path.Num();
		for (int32 k = 0; k < N; ++k)
		{
			const FVector T = (Path[FMath::Min(k + 1, N - 1)] - Path[FMath::Max(k - 1, 0)]).GetSafeNormal();
			const FVector Across = FVector(-T.Y, T.X, 0.0).GetSafeNormal();
			if (k > 0) { S += FVector::Distance(Path[k], Path[k - 1]); }
			FFrame F;
			F.Origin = Path[k];
			F.AxisA = F.NormA = Across;
			F.AxisB = FVector::UpVector;
			F.NormB = (FVector::UpVector - T * FVector::DotProduct(T, FVector::UpVector)).GetSafeNormal();
			F.S = S;
			Run.Add(F);
		}
		return Run;
	}

	/** Frames along a path in the plane of Side and up, the section square to it: A along Side, B across the path in that plane. */
	inline TArray<FFrame> PlaneFrames(const TArray<FVector>& Path, const FVector& Side)
	{
		TArray<FFrame> Run;
		double S = 0.0;
		const int32 N = Path.Num();
		const FVector A = Side.GetSafeNormal();
		for (int32 k = 0; k < N; ++k)
		{
			const FVector T = (Path[FMath::Min(k + 1, N - 1)] - Path[FMath::Max(k - 1, 0)]).GetSafeNormal();
			if (k > 0) { S += FVector::Distance(Path[k], Path[k - 1]); }
			FFrame F;
			F.Origin = Path[k];
			F.AxisA = F.NormA = A;
			F.AxisB = F.NormB = FVector::CrossProduct(T, A).GetSafeNormal();
			F.S = S;
			Run.Add(F);
		}
		return Run;
	}

	/** A rectangle W × H centred on the frame's origin (counter-clockwise in A, B). */
	inline FProfile RectProfile(double W, double H, double CentreB = 0.0)
	{
		FProfile P;
		P.bClosed = true;
		P.Add(-W / 2, CentreB - H / 2).Add(W / 2, CentreB - H / 2).Add(W / 2, CentreB + H / 2).Add(-W / 2, CentreB + H / 2);
		return P;
	}

	/** A circle of radius R (smooth), centred on the frame's origin. */
	inline FProfile CircleProfile(double R, int32 Segments)
	{
		FProfile P;
		P.bClosed = true;
		for (int32 k = 0; k < Segments; ++k)
		{
			const double T = 2.0 * UE_DOUBLE_PI * k / Segments;
			P.Add(R * FMath::Cos(T), R * FMath::Sin(T), true);
		}
		return P;
	}

	/** A rectangle W × H with its corners rounded to R (counter-clockwise, smooth round the corners). */
	inline FProfile RoundedRectProfile(double W, double H, double R, int32 CornerSegments = 3)
	{
		FProfile P;
		P.bClosed = true;
		const double X = W / 2 - R, Y = H / 2 - R;
		const FVector2D Centres[4] = {FVector2D(X, -Y), FVector2D(X, Y), FVector2D(-X, Y), FVector2D(-X, -Y)};
		for (int32 c = 0; c < 4; ++c)
		{
			const double T0 = -90.0 + 90.0 * c;
			for (int32 k = 0; k <= CornerSegments; ++k)
			{
				const double T = FMath::DegreesToRadians(T0 + 90.0 * k / CornerSegments);
				P.Add(Centres[c].X + R * FMath::Cos(T), Centres[c].Y + R * FMath::Sin(T), k > 0 && k < CornerSegments);
			}
		}
		return P;
	}

	// ---------------------------------------------------------------------------------------------- Chenghuai additions

	const FVector Zenith(0, 0, 1);

	/** A repeatable number in [0, 1) for item I, facet K. */
	inline double Hash01(int32 I, int32 K)
	{
		const double V = FMath::Sin(I * 12.9898 + K * 78.233 + 0.5) * 43758.5453;
		return V - FMath::FloorToDouble(V);
	}

	/** Clips the line P0 + Dir t, t in [Lo, Hi], to the half-planes c·p ≤ k (X, Y: c; Z: k). */
	inline void ClipLine(const TArray<FVector>& Limits, const FVector2D& P0, const FVector2D& Dir, double& Lo, double& Hi)
	{
		for (const FVector& Lim : Limits)
		{
			const FVector2D C(Lim.X, Lim.Y);
			const double CD = FVector2D::DotProduct(C, Dir), Rhs = Lim.Z - FVector2D::DotProduct(C, P0);
			if (CD > 1e-12) { Hi = FMath::Min(Hi, Rhs / CD); }
			else if (CD < -1e-12) { Lo = FMath::Max(Lo, Rhs / CD); }
			else if (Rhs < 0.0) { Hi = Lo - 1.0; }
		}
	}

	/** A (NU + 1) × (NV + 1) grid of shared vertices and its quads. */
	inline void GridPatch(FMeshData& M, int32 NU, int32 NV, TFunctionRef<void(int32, int32, FVector&, FVector&, FVector2D&)> Vert)
	{
		const int32 Base = M.Positions.Num();
		for (int32 i = 0; i <= NU; ++i)
		{
			for (int32 j = 0; j <= NV; ++j)
			{
				FVector P, N;
				FVector2D UV;
				Vert(i, j, P, N, UV);
				M.Vertex(P, N, UV);
			}
		}
		const int32 Row = NV + 1;
		for (int32 i = 0; i < NU; ++i)
		{
			for (int32 j = 0; j < NV; ++j) { M.Quad(Base + i * Row + j, Base + (i + 1) * Row + j, Base + (i + 1) * Row + j + 1, Base + i * Row + j + 1); }
		}
	}

	/** A strip between two polylines (same count), facing N everywhere; U along it, V down it. */
	inline void Strip(FMeshData& M, const TArray<FVector>& Upper, const TArray<FVector>& Lower, const FVector& N)
	{
		const int32 Base = M.Positions.Num();
		double U = 0.0;
		for (int32 k = 0; k < Upper.Num(); ++k)
		{
			if (k > 0) { U += FVector::Distance(Upper[k], Upper[k - 1]); }
			M.Vertex(Upper[k], N, FVector2D(U, -Upper[k].Z));
			M.Vertex(Lower[k], N, FVector2D(U, -Lower[k].Z));
		}
		for (int32 k = 0; k + 1 < Upper.Num(); ++k) { M.Quad(Base + 2 * k, Base + 2 * k + 2, Base + 2 * k + 3, Base + 2 * k + 1); }
	}

	/**
	 * A building's own frame in plan: u along its front (UDir), v from the front back (VDir), z up. Every building of the
	 * house is square to the plan, so a box in the frame is a box in the world.
	 */
	struct FLocal
	{
		FVector2D Origin = FVector2D::ZeroVector;
		FVector2D U = FVector2D(1, 0);
		FVector2D V = FVector2D(0, 1);

		FVector2D Plan(double A, double B) const { return Origin + U * A + V * B; }
		FVector P(double A, double B, double Z) const { const FVector2D Q = Plan(A, B); return FVector(Q.X, Q.Y, Z); }
		FVector U3() const { return FVector(U.X, U.Y, 0.0); }
		FVector V3() const { return FVector(V.X, V.Y, 0.0); }
		/** A direction given in the frame (du, dv, dz). */
		FVector Dir(double DU, double DV, double DZ) const { return U3() * DU + V3() * DV + FVector(0, 0, DZ); }
	};

	/** A closed box between (u0, v0, z0) and (u1, v1, z1) of a frame. */
	inline void LBox(FPart& Part, const FString& Name, const FLocal& L, double U0, double U1, double V0, double V1, double Z0, double Z1)
	{
		const FVector A = L.P(U0, V0, Z0), B = L.P(U1, V1, Z1);
		Box(Part, Name, FVector(FMath::Min(A.X, B.X), FMath::Min(A.Y, B.Y), FMath::Min(A.Z, B.Z)),
			FVector(FMath::Max(A.X, B.X), FMath::Max(A.Y, B.Y), FMath::Max(A.Z, B.Z)));
	}

	/** Frames along A → B with a section whose A axis is Side (square to the run) and B axis the run × Side. */
	inline TArray<FFrame> RunFrames(const FVector& A, const FVector& B, const FVector& Side)
	{
		const FVector T = (B - A).GetSafeNormal();
		FVector S = (Side - T * FVector::DotProduct(Side, T)).GetSafeNormal();
		FVector Up = FVector::CrossProduct(T, S).GetSafeNormal();
		// Keep the section's B axis upward (a mirrored building gives the run and side the other handedness).
		if (Up.Z < -1e-6) { S = -S; Up = -Up; }
		TArray<FFrame> Run;
		for (int32 k = 0; k < 2; ++k)
		{
			FFrame F;
			F.Origin = k == 0 ? A : B;
			F.AxisA = F.NormA = S;
			F.AxisB = F.NormB = Up;
			F.S = k == 0 ? 0.0 : FVector::Distance(A, B);
			Run.Add(F);
		}
		return Run;
	}

	/** A straight timber (or stone) member A → B: a W × H section (W along Side, H across), its arrises rounded to R. */
	inline void Member(FPart& Part, const FString& Name, const FVector& A, const FVector& B, const FVector& Side, double W, double H, double R = 0.0)
	{
		const TArray<FFrame> Run = RunFrames(A, B, Side);
		SweepSolid(Part, Name, Run, R > 0.0 ? RoundedRectProfile(W, H, FMath::Min(R, 0.49 * FMath::Min(W, H)), 2) : RectProfile(W, H));
	}

	/** A round member A → B (a purlin, a rafter, a rod): radius R at A and RB at B (0: R), Segments round. */
	inline void Rod(FPart& Part, const FString& Name, const FVector& A, const FVector& B, double R, int32 Segments, double RB = 0.0)
	{
		const double L = FVector::Distance(A, B);
		if (L < 1e-4) { return; }
		FProfile P;
		const double R1 = RB > 0.0 ? RB : R;
		P.Add(0.0, 0.0).Add(R, 0.0).Add(R1, L).Add(0.0, L);
		Lathe(Part, Name, A, B - A, P, Segments, R, true);
	}

	/** A flat outline (plan, counter-clockwise or not) extruded from Z0 to Z1. */
	inline void Slab(FPart& Part, const FString& Name, const TArray<FVector2D>& Outline, double Z0, double Z1)
	{
		Prism(Part, Name, Outline, Z0, Z1, false);
	}
}
