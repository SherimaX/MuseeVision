#pragma once

#include "CoreMinimal.h"
#include "Salon/SalonKit.h"

/**
 * Geometry kit for the museum's exterior dress (AMuseeFacadeStructure) and its grounds (AMuseeLandscape), on top of
 * SalonKit's indexed builder (FMeshData: positions in plan metres written as centimetres, UVs in metres, triangles
 * wound to face along their normals; FProfile and Sweep: mouldings traced with the solid on their left).
 *
 * - FFaceLine: an existing outer face in plan (straight, an arc of the drum, the oval's offset ellipse) sampled with
 *   its arc length; d is measured out from it, z up from the ground floor. Its outward normal is the left normal of
 *   the direction it runs in (plan x east, y south): run the south faces east, the east faces north, and so on.
 * - FLocal: a flat frame on a face (a pilaster's, a window's): A along the face, D out of it, Z up.
 * - Skin: the ashlar skin over a face, with openings (rectangular, round-arched, round) whose reveals run back to a
 *   backing panel, so the blind windows read as recessed.
 * - Sweeps in plan, in a local frame's plan and in a face's plane; lathes; profile caps (ear clipping); Ionic volutes.
 */
namespace FacadeKit
{
	using SalonKit::FFrame;
	/** SalonKit's sweep frame (unqualified FFrame is also the engine's script frame). */
	using FSweepFrame = SalonKit::FFrame;
	using SalonKit::FMeshData;
	using SalonKit::FProfile;

	constexpr double Pi = UE_DOUBLE_PI;

	inline double Cross2(const FVector2D& A, const FVector2D& B) { return A.X * B.Y - A.Y * B.X; }
	inline FVector2D Left(const FVector2D& T) { return FVector2D(-T.Y, T.X); }
	inline FVector Flat(const FVector2D& V, double Z = 0.0) { return FVector(V.X, V.Y, Z); }

	/** Appends points to a path, dropping any that repeat the last one. */
	inline void AppendPath(TArray<FVector2D>& Path, const TArray<FVector2D>& Pts)
	{
		for (const FVector2D& P : Pts)
		{
			if (Path.Num() == 0 || FVector2D::Distance(Path.Last(), P) > 1e-4) { Path.Add(P); }
		}
	}

	/** Points on a circle from Deg0 to Deg1 (plan angles from east towards south), at most MaxStep metres apart. */
	inline TArray<FVector2D> ArcPoints(const FVector2D& C, double R, double Deg0, double Deg1, double MaxStep = 0.25)
	{
		const double Span = FMath::DegreesToRadians(FMath::Abs(Deg1 - Deg0)) * R;
		const int32 N = FMath::Max(1, FMath::CeilToInt32(Span / MaxStep));
		TArray<FVector2D> Out;
		for (int32 i = 0; i <= N; ++i)
		{
			const double A = FMath::DegreesToRadians(Deg0 + (Deg1 - Deg0) * i / N);
			Out.Add(C + FVector2D(FMath::Cos(A), FMath::Sin(A)) * R);
		}
		return Out;
	}

	/** A flat frame on a face: A along it, D out of it, Z up (metres). */
	struct FLocal
	{
		FVector O = FVector::ZeroVector;
		FVector U = FVector(1, 0, 0);
		FVector N = FVector(0, 1, 0);

		FVector At(double A, double D, double Z) const { return O + U * A + N * D + FVector(0, 0, Z); }
		FVector Dir(double A, double D, double Z) const { return U * A + N * D + FVector(0, 0, Z); }

		static FLocal Make(const FVector2D& Origin, const FVector2D& Along)
		{
			FLocal L;
			const FVector2D T = Along.GetSafeNormal();
			L.O = Flat(Origin);
			L.U = Flat(T);
			L.N = Flat(Left(T));
			return L;
		}
	};

	/** An existing outer face in plan, with arc length; d out of it (its left normal), z up. */
	struct FFaceLine
	{
		TArray<FVector2D> P;
		TArray<FVector2D> T;
		TArray<double> S;
		bool bCurved = false;

		double Length() const { return S.Num() ? S.Last() : 0.0; }

		static FFaceLine FromPoints(const TArray<FVector2D>& InPts, bool bCurved)
		{
			FFaceLine F;
			AppendPath(F.P, InPts);
			F.bCurved = bCurved;
			const int32 N = F.P.Num();
			F.S.Add(0.0);
			for (int32 i = 1; i < N; ++i) { F.S.Add(F.S.Last() + FVector2D::Distance(F.P[i - 1], F.P[i])); }
			for (int32 i = 0; i < N; ++i)
			{
				const FVector2D In = i > 0 ? (F.P[i] - F.P[i - 1]).GetSafeNormal() : FVector2D::ZeroVector;
				const FVector2D Out = i + 1 < N ? (F.P[i + 1] - F.P[i]).GetSafeNormal() : FVector2D::ZeroVector;
				F.T.Add((In + Out).GetSafeNormal());
			}
			return F;
		}

		static FFaceLine Straight(const FVector2D& A, const FVector2D& B) { return FromPoints({A, B}, false); }

		static FFaceLine Arc(const FVector2D& C, double R, double Deg0, double Deg1, double MaxStep = 0.25)
		{
			return FromPoints(ArcPoints(C, R, Deg0, Deg1, MaxStep), true);
		}

		void Eval(double U, FVector2D& OutP, FVector2D& OutT) const
		{
			const int32 N = P.Num();
			if (U <= 0.0 || N < 2)
			{
				OutT = T[0];
				OutP = P[0] + T[0] * U;
				return;
			}
			if (U >= Length())
			{
				OutT = T.Last();
				OutP = P.Last() + T.Last() * (U - Length());
				return;
			}
			int32 Lo = 0, Hi = N - 1;
			while (Hi - Lo > 1)
			{
				const int32 Mid = (Lo + Hi) / 2;
				if (S[Mid] <= U) { Lo = Mid; }
				else { Hi = Mid; }
			}
			const double F = (U - S[Lo]) / FMath::Max(1e-9, S[Hi] - S[Lo]);
			OutP = FMath::Lerp(P[Lo], P[Hi], F);
			OutT = FMath::Lerp(T[Lo], T[Hi], F).GetSafeNormal();
		}

		FVector At(double U, double D, double Z) const
		{
			FVector2D Q, Tn;
			Eval(U, Q, Tn);
			const FVector2D Nn = Left(Tn);
			return FVector(Q.X + Nn.X * D, Q.Y + Nn.Y * D, Z);
		}

		FVector Normal(double U) const
		{
			FVector2D Q, Tn;
			Eval(U, Q, Tn);
			return Flat(Left(Tn));
		}

		FVector Tangent(double U) const
		{
			FVector2D Q, Tn;
			Eval(U, Q, Tn);
			return Flat(Tn);
		}

		FLocal Local(double U) const
		{
			FVector2D Q, Tn;
			Eval(U, Q, Tn);
			return FLocal::Make(Q, Tn);
		}

		/** The plan points of the face from U0 to U1 (for sweeps), extrapolated past the ends. */
		TArray<FVector2D> Points(double U0, double U1) const
		{
			TArray<FVector2D> Out;
			FVector2D Q, Tn;
			Eval(U0, Q, Tn);
			Out.Add(Q);
			for (int32 i = 0; i < P.Num(); ++i)
			{
				if (S[i] > U0 + 1e-4 && S[i] < U1 - 1e-4) { Out.Add(P[i]); }
			}
			Eval(U1, Q, Tn);
			Out.Add(Q);
			return Out;
		}
	};

	// ------------------------------------------------------------------------------------------------ boxes

	/** A box in a local frame (A0..A1 along, D0..D1 out, Z0..Z1 up); Faces as FMeshData::NegX … (NegX = −A, NegY = −D). */
	inline void LBox(FMeshData& M, const FLocal& L, double A0, double A1, double D0, double D1, double Z0, double Z1,
					 int32 Faces = FMeshData::AllFaces)
	{
		auto P = [&L](double A, double D, double Z) { return L.At(A, D, Z); };
		const FVector Up(0, 0, 1);
		if (Faces & FMeshData::NegX) { M.Rect(P(A0, D0, Z0), P(A0, D1, Z0), P(A0, D1, Z1), P(A0, D0, Z1), -L.U); }
		if (Faces & FMeshData::PosX) { M.Rect(P(A1, D0, Z0), P(A1, D1, Z0), P(A1, D1, Z1), P(A1, D0, Z1), L.U); }
		if (Faces & FMeshData::NegY) { M.Rect(P(A0, D0, Z0), P(A1, D0, Z0), P(A1, D0, Z1), P(A0, D0, Z1), -L.N); }
		if (Faces & FMeshData::PosY) { M.Rect(P(A0, D1, Z0), P(A1, D1, Z0), P(A1, D1, Z1), P(A0, D1, Z1), L.N); }
		if (Faces & FMeshData::NegZ) { M.Rect(P(A0, D0, Z0), P(A1, D0, Z0), P(A1, D1, Z0), P(A0, D1, Z0), -Up); }
		if (Faces & FMeshData::PosZ) { M.Rect(P(A0, D0, Z1), P(A1, D0, Z1), P(A1, D1, Z1), P(A0, D1, Z1), Up); }
	}

	/** An axis-aligned box in plan metres. */
	inline void WBox(FMeshData& M, double X0, double X1, double Y0, double Y1, double Z0, double Z1, int32 Faces = FMeshData::AllFaces)
	{
		M.Box(FVector(X0, Y0, Z0), FVector(X1, Y1, Z1), Faces);
	}

	// ------------------------------------------------------------------------------------------------ profiles

	/** Moulding elements appended from a profile's last point (A out, B up; the solid on the left). */
	namespace Moulding
	{
		/** Convex quarter round, out and up by (W, H). */
		inline void Ovolo(FProfile& P, double W, double H, int32 Segs = 6)
		{
			const FVector2D S = P.Points.Last().P;
			P.Arc(S.X, S.Y + H, W, H, -90, 0, Segs);
		}
		/** Concave quarter round, out and up by (W, H). */
		inline void Cavetto(FProfile& P, double W, double H, int32 Segs = 6)
		{
			const FVector2D S = P.Points.Last().P;
			P.Arc(S.X + W, S.Y, W, H, 180, 90, Segs);
		}
		/** Cyma recta (hollow below, round above), out and up by (W, H). */
		inline void CymaRecta(FProfile& P, double W, double H, int32 Segs = 5)
		{
			const FVector2D S = P.Points.Last().P;
			P.Arc(S.X + W / 2, S.Y, W / 2, H / 2, 180, 90, Segs);
			P.SmoothLast();
			P.Arc(S.X + W / 2, S.Y + H, W / 2, H / 2, -90, 0, Segs);
		}
		/** Cyma reversa (round below, hollow above), out and up by (W, H). */
		inline void CymaReversa(FProfile& P, double W, double H, int32 Segs = 5)
		{
			const FVector2D S = P.Points.Last().P;
			P.Arc(S.X, S.Y + H / 2, W / 2, H / 2, -90, 0, Segs);
			P.SmoothLast();
			P.Arc(S.X + W, S.Y + H / 2, W / 2, H / 2, 180, 90, Segs);
		}
		/** A half round standing out by R, from the last point up by 2R. */
		inline void Torus(FProfile& P, double R, int32 Segs = 8)
		{
			const FVector2D S = P.Points.Last().P;
			P.Arc(S.X, S.Y + R, R, R, -90, 90, Segs);
		}
		/** A hollow (scotia) cut in by W, from the last point up by H. */
		inline void Scotia(FProfile& P, double W, double H, int32 Segs = 8)
		{
			const FVector2D S = P.Points.Last().P;
			P.Arc(S.X, S.Y + H / 2, W, H / 2, -90, -270, Segs);
		}
		/** Scales a profile's A and B about the origin and shifts it. */
		inline FProfile Transform(const FProfile& In, double SA, double SB, double OA, double OB)
		{
			FProfile Out = In;
			for (SalonKit::FProfilePoint& Pt : Out.Points) { Pt.P = FVector2D(Pt.P.X * SA + OA, Pt.P.Y * SB + OB); }
			return Out;
		}
	}

	// ------------------------------------------------------------------------------------------------ caps

	inline bool InTri(const FVector2D& P, const FVector2D& A, const FVector2D& B, const FVector2D& C, double Sign)
	{
		const double C0 = Cross2(B - A, P - A) * Sign, C1 = Cross2(C - B, P - B) * Sign, C2 = Cross2(A - C, P - C) * Sign;
		return C0 >= -1e-12 && C1 >= -1e-12 && C2 >= -1e-12;
	}

	/** Ear-clips a simple polygon; triangles as indices into it. */
	inline TArray<int32> EarClip(const TArray<FVector2D>& Poly)
	{
		TArray<int32> Idx;
		for (int32 i = 0; i < Poly.Num(); ++i)
		{
			if (Idx.Num() && FVector2D::DistSquared(Poly[Idx.Last()], Poly[i]) < 1e-12) { continue; }
			Idx.Add(i);
		}
		if (Idx.Num() > 1 && FVector2D::DistSquared(Poly[Idx[0]], Poly[Idx.Last()]) < 1e-12) { Idx.Pop(); }
		double Area = 0.0;
		for (int32 i = 0; i < Idx.Num(); ++i) { Area += Cross2(Poly[Idx[i]], Poly[Idx[(i + 1) % Idx.Num()]]); }
		const double Sign = Area >= 0.0 ? 1.0 : -1.0;
		TArray<int32> Out;
		int32 Guard = 0;
		while (Idx.Num() > 3 && Guard++ < 20000)
		{
			bool bClipped = false;
			const int32 N = Idx.Num();
			for (int32 i = 0; i < N; ++i)
			{
				const int32 I0 = Idx[(i + N - 1) % N], I1 = Idx[i], I2 = Idx[(i + 1) % N];
				const FVector2D A = Poly[I0], B = Poly[I1], C = Poly[I2];
				const double Cr = Cross2(B - A, C - B) * Sign;
				if (FMath::Abs(Cr) < 1e-14)
				{
					Idx.RemoveAt(i);   // collinear: drop it
					bClipped = true;
					break;
				}
				if (Cr < 0.0) { continue; }
				bool bEmpty = true;
				for (const int32 J : Idx)
				{
					if (J == I0 || J == I1 || J == I2) { continue; }
					if (InTri(Poly[J], A, B, C, Sign)) { bEmpty = false; break; }
				}
				if (!bEmpty) { continue; }
				Out.Append({I0, I1, I2});
				Idx.RemoveAt(i);
				bClipped = true;
				break;
			}
			if (!bClipped) { break; }
		}
		for (int32 i = 1; i + 1 < Idx.Num(); ++i) { Out.Append({Idx[0], Idx[i], Idx[i + 1]}); }
		return Out;
	}

	/** Closes a profile's end at a frame (any simple profile), facing Facing. */
	inline void CapProfile(FMeshData& M, const FFrame& F, const FProfile& Profile, const FVector& Facing)
	{
		TArray<FVector2D> Poly;
		for (const SalonKit::FProfilePoint& Pt : Profile.Points) { Poly.Add(Pt.P); }
		const TArray<int32> Tris = EarClip(Poly);
		const int32 Base = M.Positions.Num();
		for (const FVector2D& Q : Poly)
		{
			const FVector P = F.At(Q);
			M.Vertex(P, Facing, SalonKit::FaceUV(P, Facing));
		}
		for (int32 t = 0; t + 2 < Tris.Num(); t += 3) { M.Tri(Base + Tris[t], Base + Tris[t + 1], Base + Tris[t + 2]); }
	}

	/** A flat polygon in plan at height Z (any simple polygon), facing up or down. */
	inline void PlanPoly(FMeshData& M, const TArray<FVector2D>& Poly, double Z, bool bUp)
	{
		const TArray<int32> Tris = EarClip(Poly);
		const FVector N(0, 0, bUp ? 1 : -1);
		const int32 Base = M.Positions.Num();
		for (const FVector2D& Q : Poly) { M.Vertex(FVector(Q.X, Q.Y, Z), N, Q); }
		for (int32 t = 0; t + 2 < Tris.Num(); t += 3) { M.Tri(Base + Tris[t], Base + Tris[t + 1], Base + Tris[t + 2]); }
	}

	// ------------------------------------------------------------------------------------------------ sweeps

	/** Sweeps a profile (A out of the path's left side, B absolute height) along a plan path. Returns the runs. */
	inline TArray<TArray<FFrame>> PlanSweep(FMeshData& M, const TArray<FVector2D>& Path, bool bClosed, const FProfile& Profile,
											double Hard = 20.0)
	{
		TArray<TArray<FFrame>> Runs = SalonKit::PlanRuns(Path, bClosed, Hard);
		for (const TArray<FFrame>& Run : Runs) { SalonKit::Sweep(M, Run, Profile); }
		return Runs;
	}

	/** Caps an open plan sweep's two ends. */
	inline void CapEnds(FMeshData& M, const TArray<TArray<FFrame>>& Runs, const FProfile& Profile, bool bStart = true, bool bEnd = true)
	{
		if (Runs.Num() == 0) { return; }
		const TArray<FFrame>& First = Runs[0];
		const TArray<FFrame>& Last = Runs.Last();
		if (bStart && First.Num() > 1)
		{
			const FVector Dir = (First[1].Origin - First[0].Origin).GetSafeNormal();
			CapProfile(M, First[0], Profile, -Dir);
		}
		if (bEnd && Last.Num() > 1)
		{
			const FVector Dir = (Last.Last().Origin - Last[Last.Num() - 2].Origin).GetSafeNormal();
			CapProfile(M, Last.Last(), Profile, Dir);
		}
	}

	/** Sweeps a profile along a path in a local frame's plan ((A, D) points; profile A out of the path's left, B = Z0 + up). */
	inline void LocalSweep(FMeshData& M, const FLocal& L, const TArray<FVector2D>& Path, bool bClosed, const FProfile& Profile,
						   double Z0, double Hard = 20.0)
	{
		for (TArray<FFrame> Run : SalonKit::PlanRuns(Path, bClosed, Hard))
		{
			for (FFrame& F : Run)
			{
				F.Origin = L.At(F.Origin.X, F.Origin.Y, Z0);
				F.AxisA = L.Dir(F.AxisA.X, F.AxisA.Y, 0.0);
				F.NormA = L.Dir(F.NormA.X, F.NormA.Y, 0.0).GetSafeNormal();
				F.AxisB = F.NormB = FVector::UpVector;
			}
			SalonKit::Sweep(M, Run, Profile);
		}
	}

	/**
	 * Frames along a path in a face's plane ((A, Z) points), at D0 out: profile A runs out of the face, profile B along
	 * the path's left normal in the face's plane (away from an opening traced up its left jamb, across, down its right).
	 */
	inline TArray<TArray<FFrame>> FaceRuns(const FLocal& L, const TArray<FVector2D>& Path, bool bClosed, double D0, double Hard = 20.0)
	{
		TArray<TArray<FFrame>> Runs = SalonKit::PlanRuns(Path, bClosed, Hard);
		for (TArray<FFrame>& Run : Runs)
		{
			for (FFrame& F : Run)
			{
				const FVector2D O(F.Origin.X, F.Origin.Y), A(F.AxisA.X, F.AxisA.Y), NA(F.NormA.X, F.NormA.Y);
				F.Origin = L.At(O.X, D0, O.Y);
				F.AxisB = L.Dir(A.X, 0.0, A.Y);
				F.NormB = L.Dir(NA.X, 0.0, NA.Y).GetSafeNormal();
				F.AxisA = F.NormA = L.N;
			}
		}
		return Runs;
	}

	inline void FaceSweep(FMeshData& M, const FLocal& L, const TArray<FVector2D>& Path, bool bClosed, double D0, const FProfile& Profile,
						  double Hard = 20.0)
	{
		for (const TArray<FFrame>& Run : FaceRuns(L, Path, bClosed, D0, Hard)) { SalonKit::Sweep(M, Run, Profile); }
	}

	/** Turns a profile (A = radius, B = height from the foot) about a vertical axis through Foot. */
	inline void Lathe(FMeshData& M, const FVector& Foot, const FProfile& Profile, int32 Segments, double Phase = 0.0)
	{
		TArray<FFrame> Run;
		double MaxR = 0.1;
		for (const SalonKit::FProfilePoint& Pt : Profile.Points) { MaxR = FMath::Max(MaxR, Pt.P.X); }
		for (int32 i = 0; i <= Segments; ++i)
		{
			const double A = Phase + 2.0 * Pi * i / Segments;
			FFrame F;
			F.Origin = Foot;
			F.AxisA = F.NormA = FVector(FMath::Cos(A), FMath::Sin(A), 0.0);
			F.AxisB = F.NormB = FVector::UpVector;
			F.S = MaxR * (A - Phase);
			Run.Add(F);
		}
		SalonKit::Sweep(M, Run, Profile);
	}

	/** Turns a profile (A = out of the face, B = radius) about an axis normal to a face through Centre (medallions). */
	inline void FaceLathe(FMeshData& M, const FLocal& L, double A0, double D0, double Z0, const FProfile& Profile, int32 Segments)
	{
		TArray<FFrame> Run;
		for (int32 i = 0; i <= Segments; ++i)
		{
			const double T = 2.0 * Pi * i / Segments;
			FFrame F;
			F.Origin = L.At(A0, D0, Z0);
			F.AxisA = F.NormA = L.N;
			F.AxisB = F.NormB = L.Dir(FMath::Cos(T), 0.0, FMath::Sin(T));
			F.S = T;
			Run.Add(F);
		}
		SalonKit::Sweep(M, Run, Profile);
	}

	/** A solid prism: a simple polygon in a local frame's face plane ((A, Z) points), from D0 to D1 out. */
	inline void FacePrism(FMeshData& M, const FLocal& L, const TArray<FVector2D>& Poly, double D0, double D1, bool bBack = true)
	{
		const TArray<int32> Tris = EarClip(Poly);
		for (const bool bFront : {true, false})
		{
			if (!bFront && !bBack) { continue; }
			const double D = bFront ? D1 : D0;
			const FVector N = bFront ? L.N : -L.N;
			const int32 Base = M.Positions.Num();
			for (const FVector2D& Q : Poly) { M.Vertex(L.At(Q.X, D, Q.Y), N, FVector2D(Q.X, -Q.Y)); }
			for (int32 t = 0; t + 2 < Tris.Num(); t += 3) { M.Tri(Base + Tris[t], Base + Tris[t + 1], Base + Tris[t + 2]); }
		}
		double Area = 0.0;
		for (int32 i = 0; i < Poly.Num(); ++i) { Area += Cross2(Poly[i], Poly[(i + 1) % Poly.Num()]); }
		for (int32 i = 0; i < Poly.Num(); ++i)
		{
			const FVector2D A = Poly[i], B = Poly[(i + 1) % Poly.Num()];
			const FVector2D E = (B - A).GetSafeNormal();
			const FVector2D Out2 = Area >= 0 ? FVector2D(E.Y, -E.X) : FVector2D(-E.Y, E.X);
			const FVector N = L.Dir(Out2.X, 0.0, Out2.Y).GetSafeNormal();
			M.Rect(L.At(A.X, D0, A.Y), L.At(B.X, D0, B.Y), L.At(B.X, D1, B.Y), L.At(A.X, D1, A.Y), N);
		}
	}

	// ------------------------------------------------------------------------------------------------ the skin

	/** An opening in the skin, in the face's (U, Z). */
	struct FOpening
	{
		enum class EKind : uint8 { Rect, Arch, Round };
		EKind Kind = EKind::Arch;
		double U = 0.0;        // centre along the face
		double Half = 0.8;     // half width (Round: the radius)
		double Sill = 3.0;     // Round: the centre's height
		double Spring = 6.0;   // Rect: the head; Arch: the springing
		bool bBacked = true;   // a backing panel at Back (a door brings its own leaves)
		bool bGlazed = true;   // glass in front of the backing

		static FOpening Make(EKind Kind, double U, double Half, double Sill, double Spring)
		{
			FOpening O;
			O.Kind = Kind;
			O.U = U;
			O.Half = Half;
			O.Sill = Sill;
			O.Spring = Spring;
			return O;
		}

		double Lower(double X) const
		{
			if (Kind != EKind::Round) { return Sill; }
			return Sill - FMath::Sqrt(FMath::Max(0.0, Half * Half - (X - U) * (X - U)));
		}
		double Upper(double X) const
		{
			if (Kind == EKind::Rect) { return Spring; }
			const double R = FMath::Sqrt(FMath::Max(0.0, Half * Half - (X - U) * (X - U)));
			return Kind == EKind::Arch ? Spring + R : Sill + R;
		}
		double Top() const { return Kind == EKind::Rect ? Spring : Kind == EKind::Arch ? Spring + Half : Sill + Half; }
		double Bottom() const { return Kind == EKind::Round ? Sill - Half : Sill; }

		static constexpr int32 Segs = 32;   // per half circle

		/** The stations across it where its outline turns. */
		void Stations(TArray<double>& Out) const
		{
			Out.Add(U - Half);
			Out.Add(U + Half);
			if (Kind != EKind::Rect)
			{
				for (int32 k = 1; k < Segs; ++k) { Out.Add(U + Half * FMath::Cos(Pi * k / Segs)); }
			}
		}

		/** Its outline in (U, Z), anticlockwise seen from outside (up the right jamb first). */
		TArray<FVector2D> Outline() const
		{
			TArray<FVector2D> O;
			if (Kind == EKind::Round)
			{
				for (int32 k = 0; k < 2 * Segs; ++k)
				{
					const double T = -Pi / 2 + Pi * k / Segs;
					O.Add(FVector2D(U + Half * FMath::Cos(T), Sill + Half * FMath::Sin(T)));
				}
				return O;
			}
			O.Add(FVector2D(U - Half, Sill));
			O.Add(FVector2D(U + Half, Sill));
			O.Add(FVector2D(U + Half, Spring));
			if (Kind == EKind::Arch)
			{
				for (int32 k = 1; k < Segs; ++k)
				{
					const double T = Pi * k / Segs;
					O.Add(FVector2D(U + Half * FMath::Cos(T), Spring + Half * FMath::Sin(T)));
				}
			}
			O.Add(FVector2D(U - Half, Spring));
			return O;
		}
	};

	/**
	 * The skin over a face from U0 to U1, from Bottom(u) to Top, T thick, with openings whose reveals run back to Back.
	 * The fronts, top, bottom and (optionally) ends go to Skin; each opening's backing panel (at Back) to Backing, and
	 * its glass (just in front of the backing) to Glass when given.
	 */
	inline void BuildSkin(FMeshData& Skin, FMeshData* Backing, FMeshData* Glass, const FFaceLine& F, double U0, double U1,
						  TFunctionRef<double(double)> Bottom, double Top, double T, double Back, const TArray<FOpening>& Openings,
						  bool bCapStart = false, bool bCapEnd = false, double GlassGap = 0.035)
	{
		TArray<double> Raw = {U0, U1};
		for (const FOpening& O : Openings) { O.Stations(Raw); }
		if (F.bCurved)
		{
			const int32 N = FMath::CeilToInt32((U1 - U0) / 0.3);
			for (int32 i = 1; i < N; ++i) { Raw.Add(U0 + (U1 - U0) * i / N); }
		}
		Raw.Sort();
		TArray<double> Us;
		for (const double V : Raw)
		{
			if (V < U0 - 1e-9 || V > U1 + 1e-9) { continue; }
			if (Us.Num() == 0 || V - Us.Last() > 1e-6) { Us.Add(V); }
		}
		auto V = [&F](FMeshData& M, double U, double D, double Z, const FVector& N) { return M.Vertex(F.At(U, D, Z), N, FVector2D(U, -Z)); };
		auto Strip = [&](FMeshData& M, double A, double B, double LoA, double LoB, double HiA, double HiB, double D)
		{
			if (HiA - LoA < 1e-6 && HiB - LoB < 1e-6) { return; }
			const FVector NA = F.Normal(A), NB = F.Normal(B);
			const int32 I0 = V(M, A, D, LoA, NA), I1 = V(M, B, D, LoB, NB), I2 = V(M, B, D, HiB, NB), I3 = V(M, A, D, HiA, NA);
			M.Quad(I0, I1, I2, I3);
		};
		const FVector Up(0, 0, 1);
		for (int32 i = 0; i + 1 < Us.Num(); ++i)
		{
			const double A = Us[i], B = Us[i + 1], Mid = 0.5 * (A + B);
			const FOpening* Hit = nullptr;
			for (const FOpening& O : Openings)
			{
				if (Mid > O.U - O.Half && Mid < O.U + O.Half) { Hit = &O; }
			}
			const double BotA = Bottom(A), BotB = Bottom(B);
			if (Hit)
			{
				Strip(Skin, A, B, BotA, BotB, FMath::Max(BotA, Hit->Lower(A)), FMath::Max(BotB, Hit->Lower(B)), T);
				Strip(Skin, A, B, FMath::Min(Top, Hit->Upper(A)), FMath::Min(Top, Hit->Upper(B)), Top, Top, T);
				if (Backing && Hit->bBacked) { Strip(*Backing, A, B, Hit->Lower(A), Hit->Lower(B), Hit->Upper(A), Hit->Upper(B), Back); }
				if (Glass && Hit->bGlazed) { Strip(*Glass, A, B, Hit->Lower(A), Hit->Lower(B), Hit->Upper(A), Hit->Upper(B), Back + GlassGap); }
			}
			else
			{
				Strip(Skin, A, B, BotA, BotB, Top, Top, T);
			}
			// Top and bottom.
			{
				const int32 I0 = Skin.Vertex(F.At(A, 0.0, Top), Up, FVector2D(F.At(A, 0.0, Top))), I1 = Skin.Vertex(F.At(B, 0.0, Top), Up, FVector2D(F.At(B, 0.0, Top)));
				const int32 I2 = Skin.Vertex(F.At(B, T, Top), Up, FVector2D(F.At(B, T, Top))), I3 = Skin.Vertex(F.At(A, T, Top), Up, FVector2D(F.At(A, T, Top)));
				Skin.Quad(I0, I1, I2, I3);
				const int32 J0 = Skin.Vertex(F.At(A, 0.0, BotA), -Up, FVector2D(F.At(A, 0.0, BotA))), J1 = Skin.Vertex(F.At(B, 0.0, BotB), -Up, FVector2D(F.At(B, 0.0, BotB)));
				const int32 J2 = Skin.Vertex(F.At(B, T, BotB), -Up, FVector2D(F.At(B, T, BotB))), J3 = Skin.Vertex(F.At(A, T, BotA), -Up, FVector2D(F.At(A, T, BotA)));
				Skin.Quad(J0, J1, J2, J3);
			}
		}
		// Ends.
		for (const bool bStart : {true, false})
		{
			if (bStart ? !bCapStart : !bCapEnd) { continue; }
			const double U = bStart ? U0 : U1;
			const FVector N = F.Tangent(U) * (bStart ? -1.0 : 1.0);
			const double Bot = Bottom(U);
			Skin.Rect(F.At(U, 0.0, Bot), F.At(U, T, Bot), F.At(U, T, Top), F.At(U, 0.0, Top), N);
		}
		// Reveals: from the skin's face back to the backing, facing into each opening.
		for (const FOpening& O : Openings)
		{
			const TArray<FVector2D> Line = O.Outline();
			const int32 N = Line.Num();
			const FVector C = F.At(O.U, T, 0.5 * (O.Bottom() + O.Top()));
			TArray<FVector> EdgeN;
			for (int32 k = 0; k < N; ++k)
			{
				const FVector2D P0 = Line[k], P1 = Line[(k + 1) % N];
				const FVector A = F.At(P0.X, T, P0.Y), B = F.At(P1.X, T, P1.Y);
				const FVector Mid = F.At(0.5 * (P0.X + P1.X), T, 0.5 * (P0.Y + P1.Y));
				FVector En = FVector::CrossProduct(B - A, F.Normal(0.5 * (P0.X + P1.X))).GetSafeNormal();
				if (FVector::DotProduct(En, C - Mid) < 0.0) { En = -En; }
				EdgeN.Add(En);
			}
			for (int32 k = 0; k < N; ++k)
			{
				const int32 K1 = (k + 1) % N;
				const FVector2D P0 = Line[k], P1 = Line[K1];
				// Smooth across gentle turns (the arch), crisp at corners.
				auto VertexN = [&](int32 Vtx, int32 Edge) -> FVector
				{
					const int32 Other = Vtx == k ? (k + N - 1) % N : K1;
					return FVector::DotProduct(EdgeN[Edge], EdgeN[Other]) > 0.85 ? (EdgeN[Edge] + EdgeN[Other]).GetSafeNormal() : EdgeN[Edge];
				};
				const FVector N0 = VertexN(k, k), N1 = VertexN(K1, k);
				const int32 I0 = Skin.Vertex(F.At(P0.X, Back, P0.Y), N0, FVector2D(0.0, 0.0));
				const int32 I1 = Skin.Vertex(F.At(P1.X, Back, P1.Y), N1, FVector2D(FVector2D::Distance(P0, P1), 0.0));
				const int32 I2 = Skin.Vertex(F.At(P1.X, T, P1.Y), N1, FVector2D(FVector2D::Distance(P0, P1), T - Back));
				const int32 I3 = Skin.Vertex(F.At(P0.X, T, P0.Y), N0, FVector2D(0.0, T - Back));
				Skin.Quad(I0, I1, I2, I3);
			}
		}
	}

	// ------------------------------------------------------------------------------------------------ volutes

	/**
	 * An Ionic volute in a local frame's face plane: a logarithmic spiral round an eye at (AEye, ZEye), starting at the
	 * top (R0 above the eye) and turning outward (Sign +1: towards +A, the right-hand volute) and inward; the scroll is a
	 * prism from D0 to D1, its spiral fillet standing Relief proud of the faces (front always; back if bBack).
	 */
	inline void Volute(FMeshData& M, const FLocal& L, double AEye, double ZEye, double R0, double Sign, double D0, double D1,
					   double Relief, bool bBack)
	{
		const double K = FMath::Loge(2.0) / (2.0 * Pi);   // halves each turn
		auto Pt = [&](double Phi, double Scale)
		{
			const double R = R0 * FMath::Exp(-K * Phi) * Scale;
			const double Psi = Pi / 2 - Sign * Phi;
			return FVector2D(AEye + R * FMath::Cos(Psi), ZEye + R * FMath::Sin(Psi));
		};
		constexpr int32 Steps = 32;
		TArray<FVector2D> Outline;
		for (int32 i = 0; i <= Steps; ++i) { Outline.Add(Pt(2.0 * Pi * i / Steps, 1.0)); }
		const FVector2D Eye(AEye, ZEye);
		// Faces: a fan from the eye.
		for (const bool bFront : {true, false})
		{
			const double D = bFront ? D1 : D0;
			const FVector N = bFront ? L.N : -L.N;
			const int32 Hub = M.Vertex(L.At(Eye.X, D, Eye.Y), N, FVector2D(Eye.X, -Eye.Y));
			const int32 Base = M.Positions.Num();
			for (const FVector2D& Q : Outline) { M.Vertex(L.At(Q.X, D, Q.Y), N, FVector2D(Q.X, -Q.Y)); }
			for (int32 i = 0; i + 1 < Outline.Num(); ++i) { M.Tri(Hub, Base + i, Base + i + 1); }
			M.Tri(Hub, Base + Outline.Num() - 1, Base);
		}
		// The scroll's rolled side, smooth round the spiral; the step where it closes, flat.
		const int32 Base = M.Positions.Num();
		for (int32 i = 0; i <= Steps; ++i)
		{
			const FVector2D Prev = Outline[FMath::Max(0, i - 1)], Next = Outline[FMath::Min(Steps, i + 1)];
			const FVector2D Tn = (Next - Prev).GetSafeNormal();
			FVector2D Out2(Tn.Y, -Tn.X);
			if (FVector2D::DotProduct(Out2, Outline[i] - Eye) < 0.0) { Out2 = -Out2; }
			const FVector N = L.Dir(Out2.X, 0.0, Out2.Y).GetSafeNormal();
			M.Vertex(L.At(Outline[i].X, D0, Outline[i].Y), N, FVector2D(i * 0.02, D0));
			M.Vertex(L.At(Outline[i].X, D1, Outline[i].Y), N, FVector2D(i * 0.02, D1));
		}
		for (int32 i = 0; i < Steps; ++i) { M.Quad(Base + 2 * i, Base + 2 * i + 2, Base + 2 * i + 3, Base + 2 * i + 1); }
		{
			const FVector2D A = Outline.Last(), B = Outline[0];
			const FVector N = L.Dir(-Sign, 0.0, 0.0);
			M.Rect(L.At(A.X, D0, A.Y), L.At(B.X, D0, B.Y), L.At(B.X, D1, B.Y), L.At(A.X, D1, A.Y), N);
		}
		// The spiral fillet (a narrow raised band along the spiral, two turns) and the eye, on the faces.
		for (const bool bFront : {true, false})
		{
			if (!bFront && !bBack) { continue; }
			const double D = bFront ? D1 : D0, Out = bFront ? Relief : -Relief;
			const FVector N = bFront ? L.N : -L.N;
			constexpr int32 RSteps = 60;
			const int32 RB = M.Positions.Num();
			TArray<FVector2D> OuterE, InnerE;
			for (int32 i = 0; i <= RSteps; ++i)
			{
				const double Phi = 4.0 * Pi * i / RSteps;
				OuterE.Add(Pt(Phi, 1.0));
				InnerE.Add(Pt(Phi, 0.86));
			}
			for (int32 i = 0; i <= RSteps; ++i)
			{
				M.Vertex(L.At(OuterE[i].X, D + Out, OuterE[i].Y), N, OuterE[i]);
				M.Vertex(L.At(InnerE[i].X, D + Out, InnerE[i].Y), N, InnerE[i]);
			}
			for (int32 i = 0; i < RSteps; ++i) { M.Quad(RB + 2 * i, RB + 2 * i + 2, RB + 2 * i + 3, RB + 2 * i + 1); }
			for (int32 i = 0; i < RSteps; ++i)
			{
				for (const bool bOuter : {true, false})
				{
					const FVector2D A = bOuter ? OuterE[i] : InnerE[i], B = bOuter ? OuterE[i + 1] : InnerE[i + 1];
					const FVector2D Tn = (B - A).GetSafeNormal();
					FVector2D O2(Tn.Y, -Tn.X);
					const bool bAway = FVector2D::DotProduct(O2, A - Eye) > 0.0;
					if (bAway != bOuter) { O2 = -O2; }
					const FVector SN = L.Dir(O2.X, 0.0, O2.Y).GetSafeNormal();
					M.Rect(L.At(A.X, D, A.Y), L.At(B.X, D, B.Y), L.At(B.X, D + Out, B.Y), L.At(A.X, D + Out, A.Y), SN);
				}
			}
			// The eye: a small raised disc.
			const double ER = R0 * 0.14;
			TArray<FVector2D> Disc;
			for (int32 i = 0; i < 16; ++i) { Disc.Add(Eye + FVector2D(FMath::Cos(2 * Pi * i / 16), FMath::Sin(2 * Pi * i / 16)) * ER); }
			FacePrism(M, L, Disc, bFront ? D - 0.001 : D - Relief * 1.4, bFront ? D + Relief * 1.4 : D + 0.001, !bFront);
		}
	}
}
