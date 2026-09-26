#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"

/**
 * Geometry kit for ASalonStructure (Salon/SalonStructure.h): an indexed mesh builder with UV-derived tangents,
 * smooth patches, and swept mouldings.
 *
 * Positions are plan metres (X east, Y south, Z up), written in centimetres; UVs are in metres. Every triangle is
 * wound to face along its vertex normals (Unreal's front faces: cross(b - a, c - a) points away from the viewer),
 * so a helper only has to give normals that face the viewer. Where two surfaces meet they share vertices, or the
 * one that stops runs a couple of millimetres on into the solid it meets.
 */
namespace SalonKit
{
	constexpr double MetresToCm = 100.0;

	inline double Cross2(const FVector2D& A, const FVector2D& B) { return A.X * B.Y - A.Y * B.X; }

	/** UVs in metres for a flat face: plan (X, Y) on floors and ceilings, (along the face, down) on walls. */
	inline FVector2D FaceUV(const FVector& P, const FVector& N)
	{
		if (FMath::Abs(N.Z) > 0.7) { return FVector2D(P.X, P.Y); }
		const FVector Along = FVector(-N.Y, N.X, 0.0).GetSafeNormal();
		return FVector2D(FVector::DotProduct(P, Along), -P.Z);
	}

	/** One point of a moulding profile. A smooth point averages the normals of the two segments that meet there. */
	struct FProfilePoint
	{
		FVector2D P = FVector2D::ZeroVector;
		bool bSmooth = false;
	};

	/**
	 * A moulding profile in its sweep frame's (A, B) plane. Its segments face (dB, -dA): trace it with the solid on
	 * the left (for a wall moulding with A out of the wall and B up: bottom first, going out, up and back).
	 */
	struct FProfile
	{
		TArray<FProfilePoint> Points;
		bool bClosed = false;

		FProfile& Add(double A, double B, bool bSmooth = false)
		{
			FProfilePoint Pt;
			Pt.P = FVector2D(A, B);
			Pt.bSmooth = bSmooth;
			Points.Add(Pt);
			return *this;
		}

		/** Makes the last point smooth (where two arcs of an S-curve meet). */
		FProfile& SmoothLast()
		{
			if (Points.Num() > 0) { Points.Last().bSmooth = true; }
			return *this;
		}

		/** The ellipse arc (CA + RA cos t, CB + RB sin t) from T0 to T1 degrees, after its first point (already added). */
		FProfile& Arc(double CA, double CB, double RA, double RB, double T0, double T1, int32 Segments)
		{
			for (int32 i = 1; i <= Segments; ++i)
			{
				const double T = FMath::DegreesToRadians(T0 + (T1 - T0) * i / Segments);
				Add(CA + RA * FMath::Cos(T), CB + RB * FMath::Sin(T), i < Segments);
			}
			return *this;
		}
	};

	/** Where a profile lands along a sweep: (A, B) goes to Origin + A * AxisA + B * AxisB; its normals use NormA, NormB. */
	struct FFrame
	{
		FVector Origin = FVector::ZeroVector;
		FVector AxisA = FVector::ForwardVector;
		FVector AxisB = FVector::UpVector;
		FVector NormA = FVector::ForwardVector;
		FVector NormB = FVector::UpVector;
		double S = 0.0;   // distance along the path, m (the U of the UVs)

		FVector At(const FVector2D& Q) const { return Origin + AxisA * Q.X + AxisB * Q.Y; }
	};

	struct FMeshData
	{
		TArray<FVector> Positions;   // cm
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<int32> Indices;
		/** Optional vertex colours (sRGB): empty for most meshes; Paint() fills them for the vertices added since a mark. */
		TArray<FColor> Colors;

		/** Colours every vertex from First on (earlier ones without a colour get white). */
		void Paint(int32 First, const FColor& C)
		{
			while (Colors.Num() < First) { Colors.Add(FColor::White); }
			while (Colors.Num() < Positions.Num()) { Colors.Add(C); }
			for (int32 i = First; i < Positions.Num(); ++i) { Colors[i] = C; }
		}

		int32 Vertex(const FVector& P, const FVector& N, const FVector2D& UV)
		{
			Normals.Add(N.GetSafeNormal());
			UVs.Add(UV);
			return Positions.Add(P * MetresToCm);
		}

		/** A triangle facing along its vertices' normals (degenerate ones are dropped). */
		void Tri(int32 A, int32 B, int32 C)
		{
			const FVector X = FVector::CrossProduct(Positions[B] - Positions[A], Positions[C] - Positions[A]);
			if (X.SizeSquared() < 1e-10) { return; }
			if (FVector::DotProduct(X, Normals[A] + Normals[B] + Normals[C]) < 0.0) { Indices.Append({A, B, C}); }
			else { Indices.Append({A, C, B}); }
		}

		void Quad(int32 A, int32 B, int32 C, int32 D)
		{
			Tri(A, B, C);
			Tri(A, C, D);
		}

		/** A flat convex polygon facing N, fanned from its first point, with metre UVs. */
		void Poly(const TArray<FVector>& Pts, const FVector& N)
		{
			if (Pts.Num() < 3) { return; }
			const int32 Base = Positions.Num();
			for (const FVector& P : Pts) { Vertex(P, N, FaceUV(P, N)); }
			for (int32 i = 1; i + 1 < Pts.Num(); ++i) { Tri(Base, Base + i, Base + i + 1); }
		}

		void Rect(const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& N)
		{
			Poly(TArray<FVector>({A, B, C, D}), N);
		}

		static constexpr int32 NegX = 1, PosX = 2, NegY = 4, PosY = 8, NegZ = 16, PosZ = 32, AllFaces = 63;

		/** The chosen faces of an axis-aligned box, facing out. */
		void Box(const FVector& Lo, const FVector& Hi, int32 Faces)
		{
			if (Faces & NegX) { Rect(FVector(Lo.X, Lo.Y, Lo.Z), FVector(Lo.X, Hi.Y, Lo.Z), FVector(Lo.X, Hi.Y, Hi.Z), FVector(Lo.X, Lo.Y, Hi.Z), FVector(-1, 0, 0)); }
			if (Faces & PosX) { Rect(FVector(Hi.X, Lo.Y, Lo.Z), FVector(Hi.X, Hi.Y, Lo.Z), FVector(Hi.X, Hi.Y, Hi.Z), FVector(Hi.X, Lo.Y, Hi.Z), FVector(1, 0, 0)); }
			if (Faces & NegY) { Rect(FVector(Lo.X, Lo.Y, Lo.Z), FVector(Hi.X, Lo.Y, Lo.Z), FVector(Hi.X, Lo.Y, Hi.Z), FVector(Lo.X, Lo.Y, Hi.Z), FVector(0, -1, 0)); }
			if (Faces & PosY) { Rect(FVector(Lo.X, Hi.Y, Lo.Z), FVector(Hi.X, Hi.Y, Lo.Z), FVector(Hi.X, Hi.Y, Hi.Z), FVector(Lo.X, Hi.Y, Hi.Z), FVector(0, 1, 0)); }
			if (Faces & NegZ) { Rect(FVector(Lo.X, Lo.Y, Lo.Z), FVector(Hi.X, Lo.Y, Lo.Z), FVector(Hi.X, Hi.Y, Lo.Z), FVector(Lo.X, Hi.Y, Lo.Z), FVector(0, 0, -1)); }
			if (Faces & PosZ) { Rect(FVector(Lo.X, Lo.Y, Hi.Z), FVector(Hi.X, Lo.Y, Hi.Z), FVector(Hi.X, Hi.Y, Hi.Z), FVector(Lo.X, Hi.Y, Hi.Z), FVector(0, 0, 1)); }
		}

		/**
		 * A smooth (NU + 1) x (NV + 1) grid of points. Normals come from the grid's own differences (so they are smooth
		 * across it and crisp at its edges) and are turned to face Toward(P).
		 */
		void Patch(int32 NU, int32 NV, TFunctionRef<FVector(int32, int32)> Pos, TFunctionRef<FVector2D(const FVector&)> UV,
				   TFunctionRef<FVector(const FVector&)> Toward)
		{
			const int32 W = NV + 1;
			TArray<FVector> G;
			G.SetNumUninitialized((NU + 1) * W);
			for (int32 i = 0; i <= NU; ++i)
			{
				for (int32 j = 0; j <= NV; ++j) { G[i * W + j] = Pos(i, j); }
			}
			const int32 Base = Positions.Num();
			for (int32 i = 0; i <= NU; ++i)
			{
				for (int32 j = 0; j <= NV; ++j)
				{
					const FVector& P = G[i * W + j];
					const FVector DU = G[FMath::Min(i + 1, NU) * W + j] - G[FMath::Max(i - 1, 0) * W + j];
					const FVector DV = G[i * W + FMath::Min(j + 1, NV)] - G[i * W + FMath::Max(j - 1, 0)];
					const FVector Hint = Toward(P);
					FVector N = FVector::CrossProduct(DU, DV);
					if (!N.Normalize(1e-24)) { N = Hint.GetSafeNormal(); }
					else if (FVector::DotProduct(N, Hint) < 0.0) { N = -N; }
					Vertex(P, N, UV(P));
				}
			}
			for (int32 i = 0; i < NU; ++i)
			{
				for (int32 j = 0; j < NV; ++j)
				{
					Quad(Base + i * W + j, Base + (i + 1) * W + j, Base + (i + 1) * W + j + 1, Base + i * W + j + 1);
				}
			}
		}

		/** Writes the section with tangents from the UVs (as UKismetProceduralMeshLibrary::CalculateTangentsForMesh). */
		void Write(UProceduralMeshComponent* Target, int32 Section, bool bCollision) const
		{
			if (!Target) { return; }
			if (Indices.Num() == 0)
			{
				Target->ClearMeshSection(Section);
				return;
			}
			TArray<FVector> SumX, SumY;
			SumX.Init(FVector::ZeroVector, Positions.Num());
			SumY.Init(FVector::ZeroVector, Positions.Num());
			for (int32 t = 0; t + 2 < Indices.Num(); t += 3)
			{
				const int32 I0 = Indices[t], I1 = Indices[t + 1], I2 = Indices[t + 2];
				const FVector E1 = Positions[I1] - Positions[I0], E2 = Positions[I2] - Positions[I0];
				const FVector2D D1 = UVs[I1] - UVs[I0], D2 = UVs[I2] - UVs[I0];
				const double Det = D1.X * D2.Y - D2.X * D1.Y;
				if (FMath::Abs(Det) < 1e-14) { continue; }
				const FVector TX = ((E1 * D2.Y - E2 * D1.Y) / Det).GetSafeNormal();
				const FVector TY = ((E2 * D1.X - E1 * D2.X) / Det).GetSafeNormal();
				SumX[I0] += TX; SumX[I1] += TX; SumX[I2] += TX;
				SumY[I0] += TY; SumY[I1] += TY; SumY[I2] += TY;
			}
			TArray<FProcMeshTangent> Tangents;
			Tangents.Reserve(Positions.Num());
			for (int32 i = 0; i < Positions.Num(); ++i)
			{
				const FVector& N = Normals[i];
				FVector TX = SumX[i] - N * FVector::DotProduct(N, SumX[i]);
				if (!TX.Normalize(1e-12))
				{
					TX = FVector::CrossProduct(N, FMath::Abs(N.Z) < 0.9 ? FVector::UpVector : FVector::ForwardVector).GetSafeNormal();
				}
				const bool bFlip = FVector::DotProduct(FVector::CrossProduct(N, TX), SumY[i]) < 0.0;
				Tangents.Add(FProcMeshTangent(TX, bFlip));
			}
			TArray<FColor> VertexColors;
			if (Colors.Num() > 0)
			{
				VertexColors = Colors;
				while (VertexColors.Num() < Positions.Num()) { VertexColors.Add(FColor::White); }
			}
			Target->CreateMeshSection(Section, Positions, Indices, Normals, UVs, VertexColors, Tangents, bCollision);
		}
	};

	/** Sweeps a profile through one run of frames: smooth along the run, crisp or smooth across the profile. */
	inline void Sweep(FMeshData& M, const TArray<FFrame>& Run, const FProfile& Profile)
	{
		const int32 NP = Profile.Points.Num();
		if (Run.Num() < 2 || NP < 2) { return; }
		const int32 NSeg = Profile.bClosed ? NP : NP - 1;
		TArray<FVector2D> SegN;
		TArray<double> Len;
		Len.Add(0.0);
		for (int32 j = 0; j < NSeg; ++j)
		{
			const FVector2D D = Profile.Points[(j + 1) % NP].P - Profile.Points[j].P;
			SegN.Add(FVector2D(D.Y, -D.X).GetSafeNormal());
			Len.Add(Len.Last() + D.Size());
		}
		// The 2D normal of segment j at its start (bEnd false) or its end.
		auto SegmentNormal = [&](int32 j, bool bEnd) -> FVector2D
		{
			const int32 PointIndex = bEnd ? (j + 1) % NP : j;
			if (!Profile.Points[PointIndex].bSmooth) { return SegN[j]; }
			int32 Other = bEnd ? j + 1 : j - 1;
			if (Other < 0 || Other >= NSeg)
			{
				if (!Profile.bClosed) { return SegN[j]; }
				Other = (Other + NSeg) % NSeg;
			}
			return (SegN[j] + SegN[Other]).GetSafeNormal();
		};
		for (int32 j = 0; j < NSeg; ++j)
		{
			const FVector2D P0 = Profile.Points[j].P, P1 = Profile.Points[(j + 1) % NP].P;
			const FVector2D N0 = SegmentNormal(j, false), N1 = SegmentNormal(j, true);
			const int32 Base = M.Positions.Num();
			for (const FFrame& F : Run)
			{
				M.Vertex(F.At(P0), F.NormA * N0.X + F.NormB * N0.Y, FVector2D(F.S, Len[j]));
				M.Vertex(F.At(P1), F.NormA * N1.X + F.NormB * N1.Y, FVector2D(F.S, Len[j + 1]));
			}
			for (int32 k = 0; k + 1 < Run.Num(); ++k)
			{
				const int32 A = Base + 2 * k;
				M.Quad(A, A + 2, A + 3, A + 1);
			}
		}
	}

	/**
	 * Frames along a horizontal plan path (metres): profile A along the path's left normal (-dy, dx), mitred at
	 * corners; B up. Corners turning more than HardDegrees split the path into runs (crisp); gentler ones are smooth.
	 */
	inline TArray<TArray<FFrame>> PlanRuns(const TArray<FVector2D>& Path, bool bClosed, double HardDegrees = 20.0)
	{
		TArray<TArray<FFrame>> Runs;
		const int32 N = Path.Num();
		if (N < 2) { return Runs; }
		const int32 NSeg = bClosed ? N : N - 1;
		TArray<FVector2D> SegNrm;
		for (int32 i = 0; i < NSeg; ++i)
		{
			const FVector2D D = (Path[(i + 1) % N] - Path[i]).GetSafeNormal();
			SegNrm.Add(FVector2D(-D.Y, D.X));
		}
		auto InN = [&](int32 v) -> FVector2D { return (!bClosed && v == 0) ? SegNrm[0] : SegNrm[(v - 1 + NSeg) % NSeg]; };
		auto OutN = [&](int32 v) -> FVector2D { return (!bClosed && v == N - 1) ? SegNrm[NSeg - 1] : SegNrm[v % NSeg]; };
		const double HardCos = FMath::Cos(FMath::DegreesToRadians(HardDegrees));
		auto IsHard = [&](int32 v) -> bool
		{
			if (!bClosed && (v == 0 || v == N - 1)) { return false; }
			return FVector2D::DotProduct(InN(v), OutN(v)) < HardCos;
		};
		auto MakeFrame = [&](int32 v, const FVector2D& NA, double S) -> FFrame
		{
			const FVector2D Mitre = (InN(v) + OutN(v)).GetSafeNormal();
			const double Dot = FMath::Max(0.2, FVector2D::DotProduct(Mitre, InN(v)));
			FFrame F;
			F.Origin = FVector(Path[v].X, Path[v].Y, 0.0);
			F.AxisA = FVector(Mitre.X, Mitre.Y, 0.0) / Dot;
			F.NormA = FVector(NA.X, NA.Y, 0.0).GetSafeNormal();
			F.AxisB = F.NormB = FVector::UpVector;
			F.S = S;
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
		TArray<FFrame> Cur;
		double S = 0.0;
		for (int32 k = 0; k <= Steps; ++k)
		{
			const int32 v = (Start + k) % N;
			if (k > 0) { S += FVector2D::Distance(Path[(Start + k - 1) % N], Path[v]); }
			const FVector2D Smooth = (InN(v) + OutN(v)).GetSafeNormal();
			if (k == 0)
			{
				Cur.Add(MakeFrame(v, IsHard(v) ? OutN(v) : Smooth, S));
			}
			else if (k == Steps)
			{
				Cur.Add(MakeFrame(v, IsHard(v) ? InN(v) : Smooth, S));
				Runs.Add(Cur);
			}
			else if (IsHard(v))
			{
				Cur.Add(MakeFrame(v, InN(v), S));
				Runs.Add(Cur);
				Cur.Reset();
				Cur.Add(MakeFrame(v, OutN(v), S));
			}
			else
			{
				Cur.Add(MakeFrame(v, Smooth, S));
			}
		}
		return Runs;
	}

	/** Sweeps a profile along a plan path (see PlanRuns). */
	inline void SweepPlan(FMeshData& M, const TArray<FVector2D>& Path, bool bClosed, const FProfile& Profile, double HardDegrees = 20.0)
	{
		for (const TArray<FFrame>& Run : PlanRuns(Path, bClosed, HardDegrees)) { Sweep(M, Run, Profile); }
	}

	/** Closes a convex profile's end at a frame, facing Facing. */
	inline void CapConvex(FMeshData& M, const FFrame& F, const FProfile& Profile, const FVector& Facing)
	{
		TArray<FVector> Pts;
		for (const FProfilePoint& Pt : Profile.Points) { Pts.Add(F.At(Pt.P)); }
		M.Poly(Pts, Facing);
	}
}
