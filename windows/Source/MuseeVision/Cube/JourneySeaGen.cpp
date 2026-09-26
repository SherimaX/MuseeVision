#include "Cube/JourneySeaGen.h"

#include "Math/RandomStream.h"

/**
 * The sea journey's generated things: Raja Ampat (West Papua, Indonesia), a day under the sea in six places. Metres in
 * each thing's own frame (x forward/east, y right/south, z up), written in centimetres by CubeMesh::FMesh; every
 * random choice from an FRandomStream with a fixed seed, so each build makes the same things. Vertex colours and UVs
 * carry data for the materials (each thing's comment says what). A named namespace: the module builds in unity files.
 */
namespace SeaGen
{
	using CubeMesh::FMesh;
	constexpr double Pi = UE_DOUBLE_PI;
	constexpr double TwoPi = 2.0 * UE_DOUBLE_PI;

	// ================================================================================================ the kit

	double Noise(double X, double Y, double Z) { return FMath::PerlinNoise3D(FVector(X, Y, Z)); }

	/** Fractal noise, Octaves octaves from frequency F (per metre); roughly -0.5 … 0.5. */
	double Fbm(const FVector& Pt, double F, int32 Octaves, double Seed)
	{
		double Sum = 0, Amp = 1, Norm = 0, Freq = F;
		for (int32 o = 0; o < Octaves; ++o)
		{
			Sum += Amp * Noise(Pt.X * Freq + Seed * 13.1, Pt.Y * Freq - Seed * 7.7, Pt.Z * Freq + Seed * 3.3);
			Norm += Amp;
			Amp *= 0.5;
			Freq *= 2.03;
		}
		return Sum / Norm;
	}

	/** Ridged noise: 0 … 1, sharp crests (1) along the noise's zero lines; for crags, cracks and ridges. */
	double Ridged(const FVector& Pt, double F, int32 Octaves, double Seed)
	{
		double Sum = 0, Amp = 1, Norm = 0, Freq = F;
		for (int32 o = 0; o < Octaves; ++o)
		{
			const double Crest = 1.0 - FMath::Abs(Noise(Pt.X * Freq + Seed * 5.3, Pt.Y * Freq + Seed * 11.9, Pt.Z * Freq - Seed * 2.1));
			Sum += Amp * Crest * Crest;
			Norm += Amp;
			Amp *= 0.5;
			Freq *= 2.1;
		}
		return Sum / Norm;
	}

	FLinearColor Rgba(double R, double G, double B, double A) { return FLinearColor(float(R), float(G), float(B), float(A)); }

	double Sat(double X) { return FMath::Clamp(X, 0.0, 1.0); }

	/** A smooth step from A to B (either way round: A > B gives a falling step). */
	double Ramp(double A, double B, double X)
	{
		const double T = Sat((X - A) / (B - A));
		return T * T * (3.0 - 2.0 * T);
	}

	/** Two unit vectors square to Dir and to each other. */
	void Basis(const FVector& Dir, FVector& U, FVector& W)
	{
		U = FVector::CrossProduct(FMath::Abs(Dir.Z) < 0.9 ? FVector(0, 0, 1) : FVector(1, 0, 0), Dir).GetSafeNormal();
		W = FVector::CrossProduct(Dir, U);
	}

	/** A smooth curve through keys (X ascending): Catmull–Rom over the keys' spacing, held level beyond the ends. */
	double Curve(const FVector2D* K, int32 N, double S)
	{
		if (N <= 0) { return 0.0; }
		if (N == 1 || S <= K[0].X) { return K[0].Y; }
		if (S >= K[N - 1].X) { return K[N - 1].Y; }
		int32 i = 0;
		while (i + 2 < N && S > K[i + 1].X) { ++i; }
		const FVector2D& P1 = K[i];
		const FVector2D& P2 = K[i + 1];
		const double H = FMath::Max(P2.X - P1.X, 1e-9);
		const double T = (S - P1.X) / H;
		const double M1 = (i > 0 ? (P2.Y - K[i - 1].Y) / (P2.X - K[i - 1].X) : (P2.Y - P1.Y) / H) * H;
		const double M2 = (i + 2 < N ? (K[i + 2].Y - P1.Y) / (K[i + 2].X - P1.X) : (P2.Y - P1.Y) / H) * H;
		const double T2 = T * T, T3 = T2 * T;
		return (2 * T3 - 3 * T2 + 1) * P1.Y + (T3 - 2 * T2 + T) * M1 + (-2 * T3 + 3 * T2) * P2.Y + (T3 - T2) * M2;
	}

	double Curve(const TArray<FVector2D>& K, double S) { return Curve(K.GetData(), K.Num(), S); }

	/** A cubic Bézier. */
	FVector Bezier(const FVector& A, const FVector& B, const FVector& C, const FVector& D, double T)
	{
		const double S = 1.0 - T;
		return A * (S * S * S) + B * (3.0 * S * S * T) + C * (3.0 * S * T * T) + D * (T * T * T);
	}

	/**
	 * Normals from the triangles (area weighted), kept on the side the given normals faced. WeldCm > 0: vertices within
	 * that distance (a grid's closing seam, a pole) share their normal, but only with those that faced the same side (so
	 * a thin sheet's two faces, which meet at its edge, keep theirs apart).
	 */
	void Smooth(FMesh& M, double WeldCm)
	{
		const int32 Num = M.Positions.Num();
		TArray<FVector> Acc;
		Acc.SetNumZeroed(Num);
		for (int32 t = 0; t + 2 < M.Indices.Num(); t += 3)
		{
			const int32 A = M.Indices[t], B = M.Indices[t + 1], C = M.Indices[t + 2];
			const FVector X = -FVector::CrossProduct(M.Positions[B] - M.Positions[A], M.Positions[C] - M.Positions[A]);
			Acc[A] += X;
			Acc[B] += X;
			Acc[C] += X;
		}
		if (WeldCm > 0.0)
		{
			const double Q = 1.0 / WeldCm;
			TMap<FIntVector, TArray<int32>> Groups;
			Groups.Reserve(Num);
			for (int32 i = 0; i < Num; ++i)
			{
				const FVector& Pt = M.Positions[i];
				Groups.FindOrAdd(FIntVector(FMath::RoundToInt32(Pt.X * Q), FMath::RoundToInt32(Pt.Y * Q), FMath::RoundToInt32(Pt.Z * Q))).Add(i);
			}
			TArray<FVector> Welded = Acc;
			for (const TPair<FIntVector, TArray<int32>>& Bucket : Groups)
			{
				const TArray<int32>& Ids = Bucket.Value;
				if (Ids.Num() < 2) { continue; }
				for (const int32 a : Ids)
				{
					FVector Sum = FVector::ZeroVector;
					for (const int32 b : Ids)
					{
						if (FVector::DotProduct(M.Normals[a], M.Normals[b]) > 0.0) { Sum += Acc[b]; }
					}
					Welded[a] = Sum;
				}
			}
			Acc = MoveTemp(Welded);
		}
		for (int32 i = 0; i < Num; ++i)
		{
			FVector N = Acc[i].GetSafeNormal();
			if (N.IsNearlyZero()) { continue; }
			if (FVector::DotProduct(N, M.Normals[i]) < 0.0) { N = -N; }
			M.Normals[i] = N;
			FVector Tg = M.Tangents[i].TangentX;
			Tg = (Tg - N * FVector::DotProduct(Tg, N)).GetSafeNormal();
			if (!Tg.IsNearlyZero()) { M.Tangents[i].TangentX = Tg; }
		}
	}

	/** A grid of (NU + 1) × (NV + 1) vertices from a function (position, facing normal, colour, UV), in quads. */
	using FVertexFn = TFunctionRef<void(int32, int32, FVector&, FVector&, FLinearColor&, FVector2D&)>;
	void Sheet(FMesh& M, int32 NU, int32 NV, FVertexFn Vert)
	{
		const int32 Base = M.Positions.Num();
		for (int32 j = 0; j <= NV; ++j)
		{
			for (int32 i = 0; i <= NU; ++i)
			{
				FVector Pt = FVector::ZeroVector, N(0, 0, 1);
				FLinearColor C = FLinearColor::White;
				FVector2D Uv = FVector2D::ZeroVector;
				Vert(i, j, Pt, N, C, Uv);
				M.V(Pt, N, Uv, C);
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

	/**
	 * An ellipsoid: centre C, unit axes Ax, Ay, Az, radii R (metres); NLon round, NLat pole to pole. Paint gives each
	 * vertex's colour and UV from its position and its unit-sphere direction (in the ellipsoid's axes).
	 */
	using FPaintFn = TFunctionRef<void(const FVector&, const FVector&, FLinearColor&, FVector2D&)>;
	void Ellipsoid(FMesh& M, const FVector& C, const FVector& Ax, const FVector& Ay, const FVector& Az, const FVector& R, int32 NLon, int32 NLat, FPaintFn Paint)
	{
		auto Emit = [&](const FVector& Unit)
		{
			const FVector Pt = C + Ax * (Unit.X * R.X) + Ay * (Unit.Y * R.Y) + Az * (Unit.Z * R.Z);
			const FVector Nrm = Ax * (Unit.X / R.X) + Ay * (Unit.Y / R.Y) + Az * (Unit.Z / R.Z);
			FLinearColor Col = FLinearColor::White;
			FVector2D Uv = FVector2D::ZeroVector;
			Paint(Pt, Unit, Col, Uv);
			return M.V(Pt, Nrm, Uv, Col);
		};
		const int32 South = Emit(FVector(0, 0, -1));
		for (int32 l = 1; l < NLat; ++l)
		{
			const double La = -0.5 * Pi + Pi * l / NLat;
			for (int32 o = 0; o < NLon; ++o)
			{
				const double Lo = TwoPi * o / NLon;
				Emit(FVector(FMath::Cos(La) * FMath::Cos(Lo), FMath::Cos(La) * FMath::Sin(Lo), FMath::Sin(La)));
			}
		}
		const int32 North = Emit(FVector(0, 0, 1));
		auto Id = [South, NLon](int32 Lat, int32 Lon) { return South + 1 + (Lat - 1) * NLon + (Lon % NLon); };
		for (int32 o = 0; o < NLon; ++o)
		{
			M.Tri(South, Id(1, o), Id(1, o + 1));
			M.Tri(North, Id(NLat - 1, o), Id(NLat - 1, o + 1));
			for (int32 l = 1; l + 1 < NLat; ++l) { M.Quad(Id(l, o), Id(l, o + 1), Id(l + 1, o + 1), Id(l + 1, o)); }
		}
	}

	/**
	 * A round tube along a path with a radius at each point: rings carried along by parallel transport (so it does not
	 * twist), the seam's vertices repeated at the same places; an optional flat start and an optional rounded
	 * (hemispherical) end. UV0 = (arc length from ArcStart, round the girth), metres. Paint(arc, fraction along, the
	 * centre point) colours each ring.
	 */
	using FTubePaint = TFunctionRef<FLinearColor(double, double, const FVector&)>;
	void Tube(FMesh& M, const TArray<FVector>& Path, const TArray<double>& Radii, int32 Around, bool bCapStart, bool bRoundEnd, double ArcStart, FTubePaint Paint)
	{
		const int32 Count = Path.Num();
		if (Count < 2 || Radii.Num() != Count || Around < 3) { return; }
		double Length = 0;
		for (int32 k = 1; k < Count; ++k) { Length += (Path[k] - Path[k - 1]).Size(); }
		if (Length <= 1e-9) { return; }
		TArray<FVector> Dir;
		Dir.SetNum(Count);
		for (int32 k = 0; k < Count; ++k) { Dir[k] = (Path[FMath::Min(k + 1, Count - 1)] - Path[FMath::Max(k - 1, 0)]).GetSafeNormal(); }
		FVector U, W;
		Basis(Dir[0], U, W);
		double Arc = ArcStart;
		TArray<int32> Rings;
		Rings.Reserve(Count);
		for (int32 k = 0; k < Count; ++k)
		{
			if (k > 0)
			{
				Arc += (Path[k] - Path[k - 1]).Size();
				const FVector Axis = FVector::CrossProduct(Dir[k - 1], Dir[k]);
				const double Sn = Axis.Size();
				if (Sn > 1e-9) { U = U.RotateAngleAxisRad(FMath::Atan2(Sn, FVector::DotProduct(Dir[k - 1], Dir[k])), Axis / Sn); }
				U = (U - Dir[k] * FVector::DotProduct(U, Dir[k])).GetSafeNormal();
				if (U.IsNearlyZero()) { Basis(Dir[k], U, W); }
				W = FVector::CrossProduct(Dir[k], U);
			}
			const FLinearColor Col = Paint(Arc, (Arc - ArcStart) / Length, Path[k]);
			Rings.Add(M.Positions.Num());
			for (int32 a = 0; a <= Around; ++a)
			{
				const double Ang = TwoPi * (a % Around) / Around;
				const FVector D = U * FMath::Cos(Ang) + W * FMath::Sin(Ang);
				M.V(Path[k] + D * Radii[k], D, FVector2D(Arc, TwoPi * a / Around * Radii[k]), Col, FVector2D::ZeroVector, Dir[k]);
			}
		}
		for (int32 k = 0; k + 1 < Count; ++k)
		{
			for (int32 a = 0; a < Around; ++a) { M.Quad(Rings[k] + a, Rings[k] + a + 1, Rings[k + 1] + a + 1, Rings[k + 1] + a); }
		}
		if (bCapStart)
		{
			const int32 Centre = M.V(Path[0], -Dir[0], FVector2D(ArcStart, 0), Paint(ArcStart, 0.0, Path[0]));
			for (int32 a = 0; a < Around; ++a) { M.Tri(Centre, Rings[0] + a + 1, Rings[0] + a); }
		}
		if (bRoundEnd)
		{
			const FVector E = Path[Count - 1];
			const FVector Te = Dir[Count - 1];
			const double Re = Radii[Count - 1];
			constexpr int32 Steps = 3;
			int32 Prev = Rings[Count - 1];
			for (int32 l = 1; l <= Steps; ++l)
			{
				const double Phi = 0.5 * Pi * l / (Steps + 1);
				const double Ahead = Re * FMath::Sin(Phi), Rr = Re * FMath::Cos(Phi);
				const FLinearColor Col = Paint(Arc + Ahead, 1.0, E + Te * Ahead);
				const int32 Cur = M.Positions.Num();
				for (int32 a = 0; a <= Around; ++a)
				{
					const double Ang = TwoPi * (a % Around) / Around;
					const FVector D = U * FMath::Cos(Ang) + W * FMath::Sin(Ang);
					M.V(E + Te * Ahead + D * Rr, D * FMath::Cos(Phi) + Te * FMath::Sin(Phi), FVector2D(Arc + Ahead, TwoPi * a / Around * Rr), Col, FVector2D::ZeroVector, Te);
				}
				for (int32 a = 0; a < Around; ++a) { M.Quad(Prev + a, Prev + a + 1, Cur + a + 1, Cur + a); }
				Prev = Cur;
			}
			const int32 Tip = M.V(E + Te * Re, Te, FVector2D(Arc + Re, 0), Paint(Arc + Re, 1.0, E + Te * Re));
			for (int32 a = 0; a < Around; ++a) { M.Tri(Prev + a, Prev + a + 1, Tip); }
		}
	}

	/** A polyline through Keys made smooth (uniform Catmull–Rom) and resampled evenly by length to Count + 1 points. */
	void SmoothResample(const TArray<FVector>& Keys, int32 Count, TArray<FVector>& OutPts)
	{
		OutPts.Reset();
		const int32 N = Keys.Num();
		if (N < 2)
		{
			for (int32 k = 0; k <= Count; ++k) { OutPts.Add(N == 1 ? Keys[0] : FVector::ZeroVector); }
			return;
		}
		constexpr int32 Sub = 6;
		TArray<FVector> Fine;
		for (int32 k = 0; k + 1 < N; ++k)
		{
			const FVector& P0 = Keys[FMath::Max(k - 1, 0)];
			const FVector& P1 = Keys[k];
			const FVector& P2 = Keys[k + 1];
			const FVector& P3 = Keys[FMath::Min(k + 2, N - 1)];
			for (int32 s = 0; s < Sub; ++s)
			{
				const double T = double(s) / Sub, T2 = T * T, T3 = T2 * T;
				Fine.Add((P1 * 2.0 + (P2 - P0) * T + (P0 * 2.0 - P1 * 5.0 + P2 * 4.0 - P3) * T2 + (P1 * 3.0 - P0 - P2 * 3.0 + P3) * T3) * 0.5);
			}
		}
		Fine.Add(Keys[N - 1]);
		TArray<double> Acc;
		Acc.Add(0.0);
		for (int32 k = 1; k < Fine.Num(); ++k) { Acc.Add(Acc.Last() + (Fine[k] - Fine[k - 1]).Size()); }
		const double Total = FMath::Max(Acc.Last(), 1e-9);
		int32 Seg = 0;
		for (int32 k = 0; k <= Count; ++k)
		{
			const double D = Total * k / Count;
			while (Seg + 2 < Fine.Num() && Acc[Seg + 1] < D) { ++Seg; }
			const double Span = FMath::Max(Acc[Seg + 1] - Acc[Seg], 1e-12);
			OutPts.Add(FMath::Lerp(Fine[Seg], Fine[Seg + 1], Sat((D - Acc[Seg]) / Span)));
		}
	}

	/**
	 * A thin sheet (a fin, a membrane) between a base line and a free edge (the same number of points), NV rows from base
	 * to edge, both faces (facing opposite ways) and a thickness Thick at the base tapering to a tenth of it at the edge.
	 */
	void Fin(FMesh& M, const TArray<FVector>& Base, const TArray<FVector>& Edge, double Thick, int32 NV, TFunctionRef<FLinearColor(const FVector&, FVector2D&)> Paint)
	{
		const int32 NU = Base.Num() - 1;
		if (NU < 1 || Edge.Num() != Base.Num() || NV < 1) { return; }
		FVector BaseMid = FVector::ZeroVector, EdgeMid = FVector::ZeroVector;
		for (int32 i = 0; i <= NU; ++i)
		{
			BaseMid += Base[i];
			EdgeMid += Edge[i];
		}
		FVector Nrm = FVector::CrossProduct(Base[NU] - Base[0], EdgeMid - BaseMid).GetSafeNormal();
		if (Nrm.IsNearlyZero()) { Nrm = FVector(0, 1, 0); }
		for (const double Face : {1.0, -1.0})
		{
			const int32 First = M.Positions.Num();
			for (int32 j = 0; j <= NV; ++j)
			{
				const double V = double(j) / NV;
				const double Half = 0.5 * Thick * (1.0 - 0.9 * V);
				for (int32 i = 0; i <= NU; ++i)
				{
					const FVector Pt = FMath::Lerp(Base[i], Edge[i], V) + Nrm * (Face * Half);
					FVector2D Uv = FVector2D::ZeroVector;
					const FLinearColor Col = Paint(Pt, Uv);
					M.V(Pt, Nrm * Face, Uv, Col);
				}
			}
			for (int32 j = 0; j < NV; ++j)
			{
				for (int32 i = 0; i < NU; ++i)
				{
					const int32 A = First + j * (NU + 1) + i;
					M.Quad(A, A + 1, A + NU + 2, A + NU + 1);
				}
			}
		}
	}

	// ================================================================================================ A. fishes

	/**
	 * The reef's fishes, each lofted from its side-view outline: the back's and the belly's profile and the half width
	 * along the body (s: 0 at the snout, 1 at the caudal fin's base; values in units of the total length L), cross
	 * sections superellipses between them; fins as thin double-sided sheets from a base on the body to a free edge (drawn
	 * as key points in the same units); eyes a little proud of the head. Swimming along +x, the snout at +x, the origin at
	 * the body's centre (mid-length, mid-height).
	 * Vertex colour: R = t, snout (0) to the tail fin's end (1); G = height across the body, -1 belly … +1 back, over the
	 * local half height (fins beyond it clamp to ±1); B = 1 on fins; A = 1 on the eyes. UV0 = (t, G).
	 */
	namespace Fish
	{
		enum class EFin : uint8 { Dorsal, Anal, Caudal, Paired };

		struct FFin
		{
			EFin Kind = EFin::Dorsal;
			double S0 = 0, S1 = 0;                   // median fins: the base along the body; paired: S0 the root
			TArray<FVector2D> Edge;                  // dorsal/anal: (s, height beyond the back/belly); caudal: (s, z from the axis); paired: (back, span)
			double Thick = 0.004;                    // at the base (L)
			double RootG = 0;                        // paired: the root's height across the flank (-1 … +1)
			double Droop = 20;                       // paired: the span's angle from straight down towards straight out (degrees)
			FVector2D BaseEnd = FVector2D(0, 0.03);  // paired: the base's far end (back, span)
		};

		struct FSpecies
		{
			FString Suffix;
			double L = 0.3, Bf = 0.8;                // total length (m); snout to caudal base over the total
			TArray<FVector2D> Top, Bot, Half;        // the back, the belly (from a reference line) and the half width, along s
			double Square = 2.0;                     // cross sections: superellipse exponent (2 an ellipse, more for a flatter belly)
			TArray<FFin> Fins;
			double EyeS = 0.1, EyeG = 0.3, EyeR = 0.03;
			bool bScutes = false;                    // the trevally's row of bony scutes along the tail
			TArray<double> Slits;                    // gill slits at these t (sharks)
			FVector KnobR = FVector::ZeroVector;     // a beak or thick lips at the snout (radii in L)
			double KnobDz = 0;
		};

		FFin Median(EFin Kind, double S0, double S1, const TArray<FVector2D>& Edge, double Thick)
		{
			FFin F;
			F.Kind = Kind;
			F.S0 = S0;
			F.S1 = S1;
			F.Edge = Edge;
			F.Thick = Thick;
			return F;
		}

		FFin Paired(double Root, double RootG, double Droop, const FVector2D& BaseEnd, const TArray<FVector2D>& Edge, double Thick)
		{
			FFin F;
			F.Kind = EFin::Paired;
			F.S0 = Root;
			F.S1 = Root;
			F.RootG = RootG;
			F.Droop = Droop;
			F.BaseEnd = BaseEnd;
			F.Edge = Edge;
			F.Thick = Thick;
			return F;
		}

		FMesh Make(const FSpecies& Sp)
		{
			FMesh M;
			const double L = Sp.L, Len = Sp.Bf * Sp.L;
			double ZLo = 1e9, ZHi = -1e9;
			for (int32 k = 0; k <= 200; ++k)
			{
				ZHi = FMath::Max(ZHi, Curve(Sp.Top, k / 200.0) * L);
				ZLo = FMath::Min(ZLo, Curve(Sp.Bot, k / 200.0) * L);
			}
			const double Z0 = 0.5 * (ZLo + ZHi);
			auto XAt = [&](double S) { return 0.5 * L - S * Len; };
			auto CentreAt = [&](double S) { const double Sc = Sat(S); return 0.5 * (Curve(Sp.Top, Sc) + Curve(Sp.Bot, Sc)) * L - Z0; };
			auto HalfHAt = [&](double S) { const double Sc = Sat(S); return FMath::Max(0.5 * (Curve(Sp.Top, Sc) - Curve(Sp.Bot, Sc)) * L, 1e-4 * L); };
			auto HalfWAt = [&](double S) { return FMath::Max(Curve(Sp.Half, Sat(S)) * L, 1e-4 * L); };
			// The body's surface at height fraction G on the flank: the superellipse |y/w|^n + |z/h|^n = 1.
			auto SurfaceY = [&](double S, double G)
			{
				const double Gc = FMath::Clamp(G, -1.0, 1.0);
				return HalfWAt(S) * FMath::Pow(FMath::Max(1.0 - FMath::Pow(FMath::Abs(Gc), Sp.Square), 0.0), 1.0 / Sp.Square);
			};
			// Colour and UV of a point (t provisional: made exact from the finished extent at the end).
			auto Tag = [&](const FVector& At, double IsFin, double IsEye, FVector2D& OutUv)
			{
				const double Sb = (0.5 * L - At.X) / Len;
				const double Tt = Sat((0.5 * L - At.X) / L);
				const double G = FMath::Clamp((At.Z - CentreAt(Sb)) / HalfHAt(Sb), -1.0, 1.0);
				OutUv = FVector2D(Tt, G);
				return Rgba(Tt, G, IsFin, IsEye);
			};
			const double Ex = 2.0 / Sp.Square;

			// ---- the body: rings from the snout to the caudal base, closer at both ends (and at the gill slits)
			constexpr int32 Around = 18, NR = 30;
			TArray<double> Ss;
			for (int32 i = 1; i <= NR; ++i) { Ss.Add(0.5 * (1.0 - FMath::Cos(Pi * i / NR))); }
			constexpr double SlitW = 0.0025;   // a gill slit's half width, in t
			for (const double Tk : Sp.Slits)
			{
				for (const double Off : {-1.6, -0.8, 0.0, 0.8, 1.6}) { Ss.Add((Tk + Off * SlitW) * L / Len); }
			}
			Ss.Sort();
			FVector2D Uv = FVector2D::ZeroVector;
			const FVector Snout(XAt(0.0), 0.0, CentreAt(0.0));
			const FLinearColor SnoutCol = Tag(Snout, 0.0, 0.0, Uv);
			const int32 Tip = M.V(Snout, FVector(1, 0, 0), Uv, SnoutCol);
			TArray<int32> RingIds;
			for (const double S : Ss)
			{
				const double X = XAt(S), Zc = CentreAt(S), Hh = HalfHAt(S), Hw = HalfWAt(S);
				const double Tb = S * Len / L;
				double Groove = 0;
				for (const double Tk : Sp.Slits) { Groove += FMath::Exp(-FMath::Square((Tb - Tk) / SlitW)); }
				RingIds.Add(M.Positions.Num());
				for (int32 a = 0; a < Around; ++a)
				{
					const double Th = TwoPi * a / Around;
					const double Sn = FMath::Sin(Th), Cs = FMath::Cos(Th);
					const double Cy = FMath::Sign(Sn) * FMath::Pow(FMath::Abs(Sn), Ex);
					const double Cz = FMath::Sign(Cs) * FMath::Pow(FMath::Abs(Cs), Ex);
					// Gill slits (sharks): shallow grooves across the flank's lower two thirds.
					const double Slit = Groove * Ramp(0.45, 0.75, FMath::Abs(Cy)) * Ramp(0.55, 0.3, Cz) * Ramp(-0.75, -0.5, Cz);
					const double Sink = 1.0 - 0.0035 * L * Slit / FMath::Max(0.5 * (Hh + Hw), 1e-6);
					const FVector Pt(X, Cy * Hw * Sink, Zc + Cz * Hh * Sink);
					const FLinearColor Col = Tag(Pt, 0.0, 0.0, Uv);
					M.V(Pt, FVector(0, Cy / Hw, Cz / Hh), Uv, Col);
				}
			}
			for (int32 a = 0; a < Around; ++a) { M.Tri(Tip, RingIds[0] + a, RingIds[0] + (a + 1) % Around); }
			for (int32 r = 0; r + 1 < RingIds.Num(); ++r)
			{
				for (int32 a = 0; a < Around; ++a)
				{
					const int32 A0 = RingIds[r] + a, A1 = RingIds[r] + (a + 1) % Around;
					const int32 B0 = RingIds[r + 1] + a, B1 = RingIds[r + 1] + (a + 1) % Around;
					M.Quad(A0, A1, B1, B0);
				}
			}
			// The caudal peduncle's end, closed (the tail fin's base hides it).
			{
				const FVector EndC(XAt(1.0), 0.0, CentreAt(1.0));
				const FLinearColor Col = Tag(EndC, 0.0, 0.0, Uv);
				const int32 EndId = M.V(EndC, FVector(-1, 0, 0), Uv, Col);
				const int32 Last = RingIds.Last();
				for (int32 a = 0; a < Around; ++a) { M.Tri(EndId, Last + a, Last + (a + 1) % Around); }
			}

			// ---- the fins
			auto FinPaint = [&](const FVector& At, FVector2D& OutUv) { return Tag(At, 1.0, 0.0, OutUv); };
			auto BodyPaint = [&](const FVector& At, const FVector&, FLinearColor& OutC, FVector2D& OutUv) { OutC = Tag(At, 0.0, 0.0, OutUv); };
			for (const FFin& F : Sp.Fins)
			{
				TArray<FVector> Base, Keys, Edge;
				if (F.Kind == EFin::Dorsal || F.Kind == EFin::Anal)
				{
					// In the mid-plane, the base 30 % of the half height inside the back (or belly).
					constexpr int32 NU = 12;
					const double Sg = F.Kind == EFin::Dorsal ? 1.0 : -1.0;
					for (int32 i = 0; i <= NU; ++i)
					{
						const double S = FMath::Lerp(F.S0, F.S1, double(i) / NU);
						Base.Add(FVector(XAt(S), 0.0, CentreAt(S) + Sg * 0.7 * HalfHAt(S)));
					}
					for (const FVector2D& K : F.Edge) { Keys.Add(FVector(XAt(K.X), 0.0, CentreAt(K.X) + Sg * (HalfHAt(K.X) + K.Y * L))); }
					SmoothResample(Keys, NU, Edge);
					Fin(M, Base, Edge, F.Thick * L, 5, FinPaint);
				}
				else if (F.Kind == EFin::Caudal)
				{
					// In the mid-plane, its base a vertical line inside the peduncle's end, its edge round the lobes.
					constexpr int32 NU = 16;
					const double Hc = HalfHAt(1.0), Cc = CentreAt(1.0);
					for (int32 i = 0; i <= NU; ++i) { Base.Add(FVector(XAt(F.S0), 0.0, Cc - 0.85 * Hc + 1.7 * Hc * i / NU)); }
					for (const FVector2D& K : F.Edge) { Keys.Add(FVector(XAt(K.X), 0.0, Cc + K.Y * L)); }
					SmoothResample(Keys, NU, Edge);
					Fin(M, Base, Edge, F.Thick * L, 7, FinPaint);
				}
				else
				{
					// A pair: rooted on the flank, spread in the plane of "back" (-x) and "span" (down, turned out by Droop).
					constexpr int32 NU = 10;
					const double Dr = FMath::DegreesToRadians(F.Droop);
					for (const double Side : {1.0, -1.0})
					{
						const FVector Back(-1, 0, 0), Span(0.0, Side * FMath::Sin(Dr), -FMath::Cos(Dr));
						const FVector Root(XAt(F.S0), Side * 0.92 * SurfaceY(F.S0, F.RootG), CentreAt(F.S0) + F.RootG * HalfHAt(F.S0));
						const FVector BaseTip = Root + (Back * F.BaseEnd.X + Span * F.BaseEnd.Y) * L;
						Base.Reset();
						Keys.Reset();
						for (int32 i = 0; i <= NU; ++i) { Base.Add(FMath::Lerp(Root, BaseTip, double(i) / NU)); }
						for (const FVector2D& K : F.Edge) { Keys.Add(Root + (Back * K.X + Span * K.Y) * L); }
						SmoothResample(Keys, NU, Edge);
						Fin(M, Base, Edge, F.Thick * L, 5, FinPaint);
					}
				}
			}

			// ---- the eyes, a little proud of the head
			for (const double Side : {1.0, -1.0})
			{
				const double Re = Sp.EyeR * L;
				const FVector Ec(XAt(Sp.EyeS), Side * (SurfaceY(Sp.EyeS, Sp.EyeG) - 0.35 * Re), CentreAt(Sp.EyeS) + Sp.EyeG * HalfHAt(Sp.EyeS));
				Ellipsoid(M, Ec, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FVector(Re, Re, Re), 12, 8,
						  [&](const FVector& At, const FVector&, FLinearColor& OutC, FVector2D& OutUv) { OutC = Tag(At, 0.0, 1.0, OutUv); });
			}

			// ---- the trevally's scutes: a row of bony shields along the straight rear of the lateral line
			if (Sp.bScutes)
			{
				for (const double Side : {1.0, -1.0})
				{
					for (int32 k = 0; k < 12; ++k)
					{
						const double S = (0.585 + 0.185 * k / 11.0) * L / Len;
						const double Sz = (0.006 + 0.004 * FMath::Sin(Pi * k / 11.0)) * L;
						const FVector Sc(XAt(S), Side * HalfWAt(S) * 0.97, CentreAt(S) + 0.05 * HalfHAt(S));
						Ellipsoid(M, Sc, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FVector(0.6 * Sz, 0.35 * Sz, Sz), 6, 3, BodyPaint);
					}
				}
			}

			// ---- a beak (parrotfish: the fused teeth) or thick lips (sweetlips) at the snout
			if (!Sp.KnobR.IsNearlyZero())
			{
				const FVector Kc(XAt(0.012), 0.0, CentreAt(0.012) + Sp.KnobDz * L);
				Ellipsoid(M, Kc, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), Sp.KnobR * L, 12, 8, BodyPaint);
			}

			Smooth(M, 0.0);

			// t exact: from the snout (the front-most point) to the tail fin's end; and the origin at mid-length.
			double XMax = -1e30, XMin = 1e30;
			for (const FVector& Pc : M.Positions)
			{
				XMax = FMath::Max(XMax, Pc.X);
				XMin = FMath::Min(XMin, Pc.X);
			}
			const double Shift = -0.5 * (XMax + XMin);
			for (int32 i = 0; i < M.Positions.Num(); ++i)
			{
				const double Tt = Sat((XMax - M.Positions[i].X) / FMath::Max(XMax - XMin, 1e-6));
				M.Positions[i].X += Shift;
				M.Colours[i].R = float(Tt);
				M.UV0[i].X = Tt;
			}
			return M;
		}

		/** The eleven species, with their real proportions (from FishBase's figures and photographs). */
		void AllSpecies(TArray<FSpecies>& List)
		{
			// Fusilier (Caesio teres, 28 cm): fusiform and fairly deep (a third of the standard length), a small
			// terminal mouth, a long low dorsal, a deeply forked tail with pointed lobes.
			{
				FSpecies S;
				S.Suffix = TEXT("Fusilier");
				S.L = 0.28;
				S.Bf = 0.8;
				S.Top = {{0.0, 0.0}, {0.03, 0.03}, {0.1, 0.07}, {0.22, 0.108}, {0.38, 0.13}, {0.55, 0.118}, {0.72, 0.08}, {0.86, 0.046}, {0.95, 0.034}, {1.0, 0.035}};
				S.Bot = {{0.0, -0.004}, {0.03, -0.028}, {0.1, -0.062}, {0.22, -0.098}, {0.38, -0.12}, {0.55, -0.106}, {0.72, -0.072}, {0.86, -0.042}, {0.95, -0.031}, {1.0, -0.032}};
				S.Half = {{0.0, 0.0}, {0.03, 0.022}, {0.12, 0.045}, {0.3, 0.058}, {0.5, 0.055}, {0.72, 0.038}, {0.9, 0.018}, {1.0, 0.012}};
				S.Fins.Add(Median(EFin::Dorsal, 0.3, 0.81, {{0.3, 0.0}, {0.33, 0.055}, {0.42, 0.06}, {0.55, 0.05}, {0.66, 0.055}, {0.76, 0.045}, {0.81, 0.0}}, 0.004));
				S.Fins.Add(Median(EFin::Anal, 0.62, 0.81, {{0.62, 0.0}, {0.65, 0.045}, {0.72, 0.04}, {0.8, 0.02}, {0.81, 0.0}}, 0.004));
				S.Fins.Add(Median(EFin::Caudal, 0.97, 1.0, {{1.0, -0.035}, {1.08, -0.07}, {1.17, -0.105}, {1.25, -0.135}, {1.19, -0.07}, {1.12, -0.02}, {1.1, 0.0},
															 {1.12, 0.02}, {1.19, 0.07}, {1.25, 0.135}, {1.17, 0.105}, {1.08, 0.07}, {1.0, 0.035}}, 0.005));
				S.Fins.Add(Paired(0.26, -0.1, 15, FVector2D(0.0, 0.045), {{0.0, 0.0}, {0.08, 0.0}, {0.15, 0.01}, {0.19, 0.022}, {0.13, 0.035}, {0.05, 0.045}, {0.0, 0.045}}, 0.004));
				S.Fins.Add(Paired(0.3, -0.85, 35, FVector2D(0.03, 0.005), {{0.0, 0.0}, {0.04, 0.05}, {0.09, 0.055}, {0.06, 0.03}, {0.03, 0.005}}, 0.004));
				S.EyeS = 0.1;
				S.EyeG = 0.2;
				S.EyeR = 0.03;
				List.Add(S);
			}
			// Anthias (Pseudanthias squamipinnis, 10 cm; the male): compressed, oblong; the dorsal's third spine long and
			// thin; a lunate tail whose lobes run out into filaments.
			{
				FSpecies S;
				S.Suffix = TEXT("Anthias");
				S.L = 0.10;
				S.Bf = 0.76;
				S.Top = {{0.0, 0.005}, {0.03, 0.04}, {0.1, 0.085}, {0.22, 0.13}, {0.38, 0.15}, {0.55, 0.135}, {0.72, 0.09}, {0.86, 0.055}, {0.95, 0.045}, {1.0, 0.046}};
				S.Bot = {{0.0, -0.005}, {0.03, -0.03}, {0.1, -0.07}, {0.22, -0.11}, {0.38, -0.13}, {0.55, -0.12}, {0.72, -0.08}, {0.86, -0.05}, {0.95, -0.042}, {1.0, -0.044}};
				S.Half = {{0.0, 0.0}, {0.03, 0.02}, {0.12, 0.04}, {0.3, 0.05}, {0.55, 0.045}, {0.8, 0.025}, {1.0, 0.012}};
				S.Fins.Add(Median(EFin::Dorsal, 0.2, 0.85, {{0.2, 0.0}, {0.22, 0.07}, {0.245, 0.08}, {0.26, 0.22}, {0.275, 0.08}, {0.3, 0.085}, {0.42, 0.09},
															 {0.56, 0.1}, {0.7, 0.11}, {0.8, 0.08}, {0.85, 0.0}}, 0.004));
				S.Fins.Add(Median(EFin::Anal, 0.62, 0.85, {{0.62, 0.0}, {0.65, 0.08}, {0.72, 0.1}, {0.8, 0.09}, {0.85, 0.0}}, 0.004));
				S.Fins.Add(Median(EFin::Caudal, 0.97, 1.0, {{1.0, -0.04}, {1.1, -0.09}, {1.22, -0.13}, {1.34, -0.2}, {1.24, -0.12}, {1.18, -0.06}, {1.16, 0.0},
															 {1.18, 0.06}, {1.24, 0.12}, {1.34, 0.2}, {1.22, 0.13}, {1.1, 0.09}, {1.0, 0.04}}, 0.005));
				S.Fins.Add(Paired(0.26, -0.1, 15, FVector2D(0.0, 0.05), {{0.0, 0.0}, {0.08, 0.0}, {0.16, 0.015}, {0.19, 0.03}, {0.14, 0.05}, {0.0, 0.05}}, 0.004));
				S.Fins.Add(Paired(0.3, -0.85, 30, FVector2D(0.03, 0.0), {{0.0, 0.0}, {0.06, 0.08}, {0.14, 0.11}, {0.08, 0.05}, {0.03, 0.0}}, 0.004));
				S.EyeS = 0.1;
				S.EyeG = 0.25;
				S.EyeR = 0.035;
				List.Add(S);
			}
			// Bigeye trevally (Caranx sexfasciatus, 65 cm): deep, compressed, a blunt head and a big eye; a small spiny
			// first dorsal, a falcate second dorsal and anal; long sickle pectorals; a slender peduncle armoured with
			// scutes; a deeply forked sickle tail.
			{
				FSpecies S;
				S.Suffix = TEXT("Trevally");
				S.L = 0.65;
				S.Bf = 0.78;
				S.Top = {{0.0, 0.0}, {0.04, 0.045}, {0.12, 0.1}, {0.25, 0.145}, {0.4, 0.155}, {0.55, 0.13}, {0.7, 0.08}, {0.84, 0.035}, {0.94, 0.022}, {1.0, 0.024}};
				S.Bot = {{0.0, -0.01}, {0.04, -0.045}, {0.12, -0.095}, {0.25, -0.13}, {0.4, -0.14}, {0.55, -0.12}, {0.7, -0.075}, {0.84, -0.032}, {0.94, -0.02}, {1.0, -0.022}};
				S.Half = {{0.0, 0.0}, {0.04, 0.03}, {0.15, 0.055}, {0.35, 0.065}, {0.6, 0.05}, {0.8, 0.025}, {0.95, 0.02}, {1.0, 0.016}};
				S.Fins.Add(Median(EFin::Dorsal, 0.28, 0.4, {{0.28, 0.0}, {0.3, 0.06}, {0.33, 0.07}, {0.37, 0.045}, {0.4, 0.0}}, 0.004));
				S.Fins.Add(Median(EFin::Dorsal, 0.42, 0.84, {{0.42, 0.0}, {0.44, 0.1}, {0.47, 0.12}, {0.52, 0.06}, {0.62, 0.04}, {0.74, 0.035}, {0.84, 0.0}}, 0.004));
				S.Fins.Add(Median(EFin::Anal, 0.54, 0.84, {{0.54, 0.0}, {0.56, 0.085}, {0.59, 0.1}, {0.64, 0.05}, {0.74, 0.035}, {0.84, 0.0}}, 0.004));
				S.Fins.Add(Median(EFin::Caudal, 0.97, 1.0, {{1.0, -0.025}, {1.08, -0.075}, {1.18, -0.13}, {1.28, -0.19}, {1.2, -0.11}, {1.12, -0.04}, {1.09, 0.0},
															 {1.12, 0.04}, {1.2, 0.11}, {1.28, 0.19}, {1.18, 0.13}, {1.08, 0.075}, {1.0, 0.025}}, 0.005));
				S.Fins.Add(Paired(0.25, -0.15, 20, FVector2D(0.0, 0.05), {{0.0, 0.0}, {0.08, -0.02}, {0.18, -0.01}, {0.28, 0.01}, {0.16, 0.03}, {0.06, 0.045}, {0.0, 0.05}}, 0.004));
				S.Fins.Add(Paired(0.28, -0.85, 35, FVector2D(0.03, 0.0), {{0.0, 0.0}, {0.04, 0.05}, {0.09, 0.06}, {0.06, 0.03}, {0.03, 0.0}}, 0.004));
				S.EyeS = 0.1;
				S.EyeG = 0.25;
				S.EyeR = 0.04;
				S.bScutes = true;
				List.Add(S);
			}
			// Blackfin barracuda (Sphyraena qenie, 1.1 m): long and nearly round, a pointed head with the lower jaw
			// jutting; two short dorsals far apart (the first over the pelvics, the second over the anal); a forked tail.
			{
				FSpecies S;
				S.Suffix = TEXT("Barracuda");
				S.L = 1.1;
				S.Bf = 0.84;
				S.Square = 2.2;
				S.Top = {{0.0, -0.005}, {0.04, 0.012}, {0.1, 0.03}, {0.18, 0.05}, {0.3, 0.062}, {0.45, 0.065}, {0.62, 0.058}, {0.8, 0.04}, {0.93, 0.026}, {1.0, 0.027}};
				S.Bot = {{0.0, -0.012}, {0.04, -0.022}, {0.1, -0.035}, {0.18, -0.05}, {0.3, -0.058}, {0.45, -0.06}, {0.62, -0.052}, {0.8, -0.035}, {0.93, -0.024}, {1.0, -0.025}};
				S.Half = {{0.0, 0.0}, {0.04, 0.012}, {0.1, 0.025}, {0.2, 0.04}, {0.4, 0.048}, {0.65, 0.042}, {0.85, 0.028}, {1.0, 0.018}};
				S.Fins.Add(Median(EFin::Dorsal, 0.4, 0.48, {{0.4, 0.0}, {0.415, 0.06}, {0.44, 0.055}, {0.47, 0.02}, {0.48, 0.0}}, 0.004));
				S.Fins.Add(Median(EFin::Dorsal, 0.74, 0.81, {{0.74, 0.0}, {0.75, 0.055}, {0.77, 0.045}, {0.8, 0.015}, {0.81, 0.0}}, 0.004));
				S.Fins.Add(Median(EFin::Anal, 0.76, 0.83, {{0.76, 0.0}, {0.77, 0.05}, {0.79, 0.04}, {0.82, 0.012}, {0.83, 0.0}}, 0.004));
				S.Fins.Add(Median(EFin::Caudal, 0.97, 1.0, {{1.0, -0.025}, {1.06, -0.06}, {1.13, -0.1}, {1.19, -0.12}, {1.13, -0.06}, {1.09, -0.015}, {1.08, 0.0},
															 {1.09, 0.015}, {1.13, 0.06}, {1.19, 0.12}, {1.13, 0.1}, {1.06, 0.06}, {1.0, 0.025}}, 0.005));
				S.Fins.Add(Paired(0.22, -0.2, 25, FVector2D(0.0, 0.03), {{0.0, 0.0}, {0.05, -0.005}, {0.09, 0.008}, {0.11, 0.02}, {0.06, 0.03}, {0.0, 0.03}}, 0.004));
				S.Fins.Add(Paired(0.4, -0.85, 35, FVector2D(0.02, 0.0), {{0.0, 0.0}, {0.03, 0.035}, {0.07, 0.04}, {0.05, 0.02}, {0.02, 0.0}}, 0.004));
				S.EyeS = 0.13;
				S.EyeG = 0.35;
				S.EyeR = 0.018;
				List.Add(S);
			}
			// Butterflyfish (Chaetodon, 15 cm): a thin disc, a short pointed snout; the dorsal and anal fins' soft rear
			// parts round the body's outline off; a truncate tail.
			{
				FSpecies S;
				S.Suffix = TEXT("Butterfly");
				S.L = 0.15;
				S.Bf = 0.84;
				S.Top = {{0.0, 0.02}, {0.03, 0.04}, {0.08, 0.07}, {0.16, 0.16}, {0.28, 0.25}, {0.42, 0.285}, {0.58, 0.27}, {0.72, 0.2}, {0.86, 0.1}, {0.95, 0.065}, {1.0, 0.065}};
				S.Bot = {{0.0, 0.0}, {0.03, -0.01}, {0.08, -0.04}, {0.16, -0.12}, {0.28, -0.2}, {0.42, -0.24}, {0.58, -0.23}, {0.72, -0.17}, {0.86, -0.09}, {0.95, -0.06}, {1.0, -0.06}};
				S.Half = {{0.0, 0.0}, {0.04, 0.012}, {0.12, 0.03}, {0.3, 0.05}, {0.5, 0.052}, {0.75, 0.035}, {0.9, 0.018}, {1.0, 0.012}};
				S.Fins.Add(Median(EFin::Dorsal, 0.22, 0.93, {{0.22, 0.0}, {0.26, 0.06}, {0.38, 0.08}, {0.52, 0.1}, {0.66, 0.13}, {0.78, 0.13}, {0.88, 0.09}, {0.93, 0.0}}, 0.005));
				S.Fins.Add(Median(EFin::Anal, 0.55, 0.93, {{0.55, 0.0}, {0.58, 0.06}, {0.66, 0.1}, {0.78, 0.12}, {0.88, 0.08}, {0.93, 0.0}}, 0.005));
				S.Fins.Add(Median(EFin::Caudal, 0.97, 1.0, {{1.0, -0.055}, {1.05, -0.085}, {1.12, -0.1}, {1.18, -0.09}, {1.2, -0.04}, {1.2, 0.0}, {1.2, 0.04},
															 {1.18, 0.09}, {1.12, 0.1}, {1.05, 0.085}, {1.0, 0.055}}, 0.005));
				S.Fins.Add(Paired(0.3, -0.1, 15, FVector2D(0.0, 0.06), {{0.0, 0.0}, {0.06, 0.0}, {0.12, 0.02}, {0.13, 0.04}, {0.07, 0.06}, {0.0, 0.06}}, 0.004));
				S.Fins.Add(Paired(0.34, -0.85, 25, FVector2D(0.03, 0.0), {{0.0, 0.0}, {0.03, 0.08}, {0.08, 0.1}, {0.06, 0.05}, {0.03, 0.0}}, 0.004));
				S.EyeS = 0.18;
				S.EyeG = 0.25;
				S.EyeR = 0.035;
				List.Add(S);
			}
			// Sweetlips (Plectorhinchus, 45 cm): oblong, moderately compressed, thick fleshy lips; a long continuous
			// dorsal notched between its spiny and soft parts; a truncate tail.
			{
				FSpecies S;
				S.Suffix = TEXT("Sweetlips");
				S.L = 0.45;
				S.Bf = 0.82;
				S.Top = {{0.0, 0.0}, {0.04, 0.04}, {0.12, 0.09}, {0.25, 0.14}, {0.4, 0.155}, {0.56, 0.145}, {0.72, 0.105}, {0.86, 0.06}, {0.95, 0.045}, {1.0, 0.046}};
				S.Bot = {{0.0, -0.02}, {0.04, -0.05}, {0.12, -0.085}, {0.25, -0.12}, {0.4, -0.13}, {0.56, -0.12}, {0.72, -0.09}, {0.86, -0.055}, {0.95, -0.042}, {1.0, -0.044}};
				S.Half = {{0.0, 0.0}, {0.04, 0.03}, {0.15, 0.06}, {0.35, 0.07}, {0.6, 0.06}, {0.85, 0.03}, {1.0, 0.018}};
				S.Fins.Add(Median(EFin::Dorsal, 0.24, 0.88, {{0.24, 0.0}, {0.27, 0.06}, {0.36, 0.08}, {0.5, 0.065}, {0.56, 0.06}, {0.62, 0.075}, {0.78, 0.075}, {0.88, 0.0}}, 0.004));
				S.Fins.Add(Median(EFin::Anal, 0.66, 0.88, {{0.66, 0.0}, {0.68, 0.07}, {0.76, 0.08}, {0.86, 0.04}, {0.88, 0.0}}, 0.004));
				S.Fins.Add(Median(EFin::Caudal, 0.97, 1.0, {{1.0, -0.045}, {1.06, -0.08}, {1.2, -0.11}, {1.22, -0.07}, {1.21, 0.0}, {1.22, 0.07}, {1.2, 0.11}, {1.06, 0.08}, {1.0, 0.045}}, 0.005));
				S.Fins.Add(Paired(0.27, -0.15, 18, FVector2D(0.0, 0.05), {{0.0, 0.0}, {0.07, 0.0}, {0.14, 0.015}, {0.16, 0.035}, {0.09, 0.05}, {0.0, 0.05}}, 0.004));
				S.Fins.Add(Paired(0.3, -0.85, 30, FVector2D(0.03, 0.0), {{0.0, 0.0}, {0.04, 0.06}, {0.1, 0.07}, {0.07, 0.035}, {0.03, 0.0}}, 0.004));
				S.EyeS = 0.12;
				S.EyeG = 0.3;
				S.EyeR = 0.025;
				S.KnobR = FVector(0.025, 0.035, 0.03);
				S.KnobDz = -0.01;
				List.Add(S);
			}
			// Longfin batfish (Platax teira, 45 cm long, 60 cm tall): a round, very thin body, its dorsal and anal fins
			// tall and swept back (so the fish is taller than long); long dark pelvics; a truncate tail.
			{
				FSpecies S;
				S.Suffix = TEXT("Batfish");
				S.L = 0.45;
				S.Bf = 0.82;
				S.Top = {{0.0, 0.02}, {0.03, 0.07}, {0.08, 0.15}, {0.16, 0.25}, {0.28, 0.31}, {0.42, 0.32}, {0.58, 0.28}, {0.72, 0.2}, {0.86, 0.1}, {0.95, 0.07}, {1.0, 0.07}};
				S.Bot = {{0.0, -0.02}, {0.03, -0.06}, {0.08, -0.13}, {0.16, -0.22}, {0.28, -0.285}, {0.42, -0.3}, {0.58, -0.26}, {0.72, -0.18}, {0.86, -0.09}, {0.95, -0.065}, {1.0, -0.065}};
				S.Half = {{0.0, 0.0}, {0.03, 0.02}, {0.12, 0.045}, {0.3, 0.055}, {0.55, 0.05}, {0.8, 0.03}, {1.0, 0.015}};
				S.Fins.Add(Median(EFin::Dorsal, 0.3, 0.92, {{0.3, 0.0}, {0.36, 0.2}, {0.44, 0.33}, {0.54, 0.37}, {0.66, 0.35}, {0.78, 0.26}, {0.88, 0.12}, {0.92, 0.0}}, 0.005));
				S.Fins.Add(Median(EFin::Anal, 0.4, 0.92, {{0.4, 0.0}, {0.46, 0.2}, {0.54, 0.31}, {0.64, 0.34}, {0.74, 0.3}, {0.84, 0.2}, {0.9, 0.08}, {0.92, 0.0}}, 0.005));
				S.Fins.Add(Median(EFin::Caudal, 0.97, 1.0, {{1.0, -0.065}, {1.06, -0.11}, {1.18, -0.14}, {1.21, -0.1}, {1.2, 0.0}, {1.21, 0.1}, {1.18, 0.14}, {1.06, 0.11}, {1.0, 0.065}}, 0.005));
				S.Fins.Add(Paired(0.28, 0.0, 12, FVector2D(0.0, 0.06), {{0.0, 0.0}, {0.06, 0.005}, {0.12, 0.025}, {0.12, 0.05}, {0.06, 0.065}, {0.0, 0.06}}, 0.004));
				S.Fins.Add(Paired(0.22, -0.9, 25, FVector2D(0.03, 0.0), {{0.0, 0.0}, {0.03, 0.1}, {0.07, 0.19}, {0.09, 0.2}, {0.07, 0.1}, {0.03, 0.0}}, 0.005));
				S.EyeS = 0.1;
				S.EyeG = 0.35;
				S.EyeR = 0.028;
				List.Add(S);
			}
			// Parrotfish (Chlorurus, 45 cm): robust and oblong, the forehead steep and rounded, the teeth fused into a
			// beak; a long even dorsal; the tail lunate in big adults.
			{
				FSpecies S;
				S.Suffix = TEXT("Parrot");
				S.L = 0.45;
				S.Bf = 0.82;
				S.Square = 2.2;
				S.Top = {{0.0, -0.01}, {0.03, 0.03}, {0.08, 0.08}, {0.16, 0.13}, {0.28, 0.155}, {0.44, 0.16}, {0.6, 0.145}, {0.74, 0.105}, {0.87, 0.06}, {0.95, 0.05}, {1.0, 0.05}};
				S.Bot = {{0.0, -0.03}, {0.03, -0.055}, {0.08, -0.085}, {0.16, -0.115}, {0.28, -0.135}, {0.44, -0.14}, {0.6, -0.125}, {0.74, -0.09}, {0.87, -0.055}, {0.95, -0.047}, {1.0, -0.047}};
				S.Half = {{0.0, 0.0}, {0.03, 0.035}, {0.12, 0.065}, {0.3, 0.08}, {0.55, 0.07}, {0.8, 0.04}, {1.0, 0.022}};
				S.Fins.Add(Median(EFin::Dorsal, 0.24, 0.86, {{0.24, 0.0}, {0.27, 0.05}, {0.4, 0.055}, {0.6, 0.055}, {0.78, 0.06}, {0.86, 0.0}}, 0.004));
				S.Fins.Add(Median(EFin::Anal, 0.68, 0.86, {{0.68, 0.0}, {0.7, 0.05}, {0.8, 0.055}, {0.86, 0.0}}, 0.004));
				S.Fins.Add(Median(EFin::Caudal, 0.97, 1.0, {{1.0, -0.05}, {1.08, -0.09}, {1.2, -0.14}, {1.22, -0.15}, {1.17, -0.07}, {1.15, 0.0}, {1.17, 0.07},
															 {1.22, 0.15}, {1.2, 0.14}, {1.08, 0.09}, {1.0, 0.05}}, 0.005));
				S.Fins.Add(Paired(0.26, -0.1, 18, FVector2D(0.0, 0.055), {{0.0, 0.0}, {0.07, 0.0}, {0.14, 0.015}, {0.16, 0.03}, {0.1, 0.05}, {0.0, 0.055}}, 0.004));
				S.Fins.Add(Paired(0.3, -0.85, 30, FVector2D(0.03, 0.0), {{0.0, 0.0}, {0.04, 0.05}, {0.09, 0.06}, {0.06, 0.03}, {0.03, 0.0}}, 0.004));
				S.EyeS = 0.14;
				S.EyeG = 0.4;
				S.EyeR = 0.02;
				S.KnobR = FVector(0.03, 0.04, 0.035);
				S.KnobDz = -0.015;
				List.Add(S);
			}
			// Blue-green chromis (Chromis viridis, 8 cm): a small deep-bodied damselfish, a forked tail.
			{
				FSpecies S;
				S.Suffix = TEXT("Chromis");
				S.L = 0.08;
				S.Bf = 0.76;
				S.Top = {{0.0, 0.0}, {0.03, 0.04}, {0.1, 0.1}, {0.22, 0.16}, {0.38, 0.18}, {0.55, 0.165}, {0.72, 0.11}, {0.86, 0.06}, {0.95, 0.05}, {1.0, 0.05}};
				S.Bot = {{0.0, -0.01}, {0.03, -0.04}, {0.1, -0.09}, {0.22, -0.15}, {0.38, -0.17}, {0.55, -0.15}, {0.72, -0.1}, {0.86, -0.055}, {0.95, -0.047}, {1.0, -0.047}};
				S.Half = {{0.0, 0.0}, {0.03, 0.025}, {0.12, 0.055}, {0.3, 0.07}, {0.55, 0.06}, {0.8, 0.03}, {1.0, 0.015}};
				S.Fins.Add(Median(EFin::Dorsal, 0.26, 0.86, {{0.26, 0.0}, {0.29, 0.09}, {0.38, 0.1}, {0.55, 0.09}, {0.7, 0.12}, {0.8, 0.1}, {0.86, 0.0}}, 0.005));
				S.Fins.Add(Median(EFin::Anal, 0.6, 0.86, {{0.6, 0.0}, {0.62, 0.1}, {0.72, 0.12}, {0.82, 0.08}, {0.86, 0.0}}, 0.005));
				S.Fins.Add(Median(EFin::Caudal, 0.97, 1.0, {{1.0, -0.045}, {1.1, -0.1}, {1.24, -0.16}, {1.32, -0.2}, {1.2, -0.09}, {1.13, -0.02}, {1.12, 0.0},
															 {1.13, 0.02}, {1.2, 0.09}, {1.32, 0.2}, {1.24, 0.16}, {1.1, 0.1}, {1.0, 0.045}}, 0.006));
				S.Fins.Add(Paired(0.27, -0.05, 15, FVector2D(0.0, 0.06), {{0.0, 0.0}, {0.08, 0.0}, {0.16, 0.02}, {0.18, 0.04}, {0.1, 0.06}, {0.0, 0.06}}, 0.005));
				S.Fins.Add(Paired(0.3, -0.85, 30, FVector2D(0.03, 0.0), {{0.0, 0.0}, {0.05, 0.08}, {0.12, 0.1}, {0.07, 0.05}, {0.03, 0.0}}, 0.005));
				S.EyeS = 0.1;
				S.EyeG = 0.3;
				S.EyeR = 0.045;
				List.Add(S);
			}
			// Blacktip reef shark (Carcharhinus melanopterus, 1.4 m): a short, bluntly rounded conical snout; five gill
			// slits over the pectoral's origin; broad falcate pectorals; a large first dorsal over the pectorals' free
			// tips, a small second dorsal over the anal; a heterocercal tail, its upper lobe long with a subterminal notch.
			{
				FSpecies S;
				S.Suffix = TEXT("BlacktipShark");
				S.L = 1.4;
				S.Bf = 0.73;
				S.Square = 2.3;
				S.Top = {{0.0, -0.005}, {0.02, 0.012}, {0.06, 0.03}, {0.14, 0.055}, {0.26, 0.075}, {0.38, 0.08}, {0.52, 0.065}, {0.68, 0.04}, {0.86, 0.022}, {0.95, 0.017}, {1.0, 0.018}};
				S.Bot = {{0.0, -0.012}, {0.02, -0.025}, {0.06, -0.04}, {0.14, -0.055}, {0.26, -0.07}, {0.38, -0.075}, {0.52, -0.06}, {0.68, -0.035}, {0.86, -0.018}, {0.95, -0.015}, {1.0, -0.016}};
				S.Half = {{0.0, 0.0}, {0.02, 0.02}, {0.06, 0.04}, {0.14, 0.06}, {0.28, 0.07}, {0.42, 0.066}, {0.6, 0.045}, {0.8, 0.025}, {1.0, 0.014}};
				S.Fins.Add(Median(EFin::Dorsal, 0.4, 0.51, {{0.4, 0.0}, {0.43, 0.06}, {0.47, 0.105}, {0.5, 0.07}, {0.52, 0.03}, {0.555, 0.012}, {0.53, 0.004}, {0.51, 0.0}}, 0.012));
				S.Fins.Add(Median(EFin::Dorsal, 0.8, 0.85, {{0.8, 0.0}, {0.81, 0.03}, {0.84, 0.028}, {0.87, 0.008}, {0.85, 0.0}}, 0.008));
				S.Fins.Add(Median(EFin::Anal, 0.78, 0.84, {{0.78, 0.0}, {0.795, 0.035}, {0.82, 0.03}, {0.86, 0.01}, {0.84, 0.0}}, 0.008));
				S.Fins.Add(Median(EFin::Caudal, 0.97, 1.0, {{1.0, -0.018}, {1.07, -0.05}, {1.14, -0.08}, {1.18, -0.095}, {1.16, -0.06}, {1.13, -0.01}, {1.17, 0.03},
															 {1.23, 0.06}, {1.28, 0.085}, {1.31, 0.095}, {1.33, 0.12}, {1.36, 0.14}, {1.28, 0.1}, {1.18, 0.06},
															 {1.08, 0.03}, {1.0, 0.018}}, 0.01));
				S.Fins.Add(Paired(0.3, -0.55, 62, FVector2D(0.07, 0.0), {{0.0, 0.0}, {0.03, 0.07}, {0.07, 0.14}, {0.1, 0.19}, {0.11, 0.15}, {0.1, 0.08}, {0.12, 0.03}, {0.07, 0.0}}, 0.012));
				S.Fins.Add(Paired(0.62, -0.8, 60, FVector2D(0.05, 0.0), {{0.0, 0.0}, {0.04, 0.05}, {0.07, 0.06}, {0.09, 0.035}, {0.05, 0.0}}, 0.008));
				S.EyeS = 0.1;
				S.EyeG = 0.25;
				S.EyeR = 0.009;
				S.Slits = {0.195, 0.207, 0.219, 0.231, 0.243};
				List.Add(S);
			}
			// Freycinet's epaulette shark (Hemiscyllium freycineti, 70 cm), the "walking shark" of Raja Ampat: slender, the
			// head a little flattened; the tail long and thick (more than half the length) with both dorsals far back on
			// it and a low caudal fin along its end; paddle-like pectorals and pelvics it walks on.
			{
				FSpecies S;
				S.Suffix = TEXT("WalkingShark");
				S.L = 0.7;
				S.Bf = 0.93;
				S.Square = 2.6;
				S.Top = {{0.0, -0.005}, {0.02, 0.012}, {0.06, 0.025}, {0.14, 0.037}, {0.25, 0.045}, {0.36, 0.045}, {0.5, 0.04}, {0.65, 0.032}, {0.8, 0.022}, {0.92, 0.013}, {1.0, 0.006}};
				S.Bot = {{0.0, -0.014}, {0.02, -0.022}, {0.06, -0.03}, {0.14, -0.037}, {0.25, -0.042}, {0.36, -0.042}, {0.5, -0.035}, {0.65, -0.028}, {0.8, -0.02}, {0.92, -0.012}, {1.0, -0.006}};
				S.Half = {{0.0, 0.0}, {0.02, 0.025}, {0.06, 0.045}, {0.14, 0.055}, {0.25, 0.055}, {0.36, 0.05}, {0.5, 0.036}, {0.65, 0.026}, {0.8, 0.018}, {0.92, 0.01}, {1.0, 0.004}};
				S.Fins.Add(Median(EFin::Dorsal, 0.48, 0.55, {{0.48, 0.0}, {0.505, 0.035}, {0.53, 0.045}, {0.555, 0.03}, {0.57, 0.01}, {0.55, 0.0}}, 0.01));
				S.Fins.Add(Median(EFin::Dorsal, 0.62, 0.68, {{0.62, 0.0}, {0.64, 0.03}, {0.665, 0.038}, {0.69, 0.02}, {0.7, 0.008}, {0.68, 0.0}}, 0.01));
				S.Fins.Add(Median(EFin::Anal, 0.84, 0.9, {{0.84, 0.0}, {0.86, 0.02}, {0.9, 0.02}, {0.92, 0.0}}, 0.008));
				// The caudal: a low fin along the underside of the tail's end, and a thin margin along its top.
				S.Fins.Add(Median(EFin::Anal, 0.9, 1.0, {{0.9, 0.0}, {0.93, 0.03}, {0.98, 0.035}, {1.03, 0.025}, {1.075, 0.0}}, 0.008));
				S.Fins.Add(Median(EFin::Dorsal, 0.93, 1.0, {{0.93, 0.0}, {0.97, 0.01}, {1.03, 0.008}, {1.075, 0.0}}, 0.006));
				S.Fins.Add(Paired(0.22, -0.7, 65, FVector2D(0.06, 0.0), {{0.0, 0.0}, {0.0, 0.05}, {0.03, 0.09}, {0.07, 0.1}, {0.1, 0.08}, {0.1, 0.04}, {0.06, 0.0}}, 0.012));
				S.Fins.Add(Paired(0.4, -0.75, 65, FVector2D(0.05, 0.0), {{0.0, 0.0}, {0.0, 0.04}, {0.03, 0.075}, {0.07, 0.08}, {0.09, 0.05}, {0.05, 0.0}}, 0.01));
				S.EyeS = 0.06;
				S.EyeG = 0.5;
				S.EyeR = 0.009;
				List.Add(S);
			}
		}
	}

	// ================================================================================================ B. the manta

	/**
	 * A reef manta (Mobula alfredi) as it glides: a disc 3.6 m across and 1.4 m long at the body, 28 cm thick at the
	 * centre thinning to a centimetre at the wing tips; the wings' leading edges bowed forward and swept back to pointed
	 * tips, their trailing edges concave; the two cephalic lobes rolled into scrolls either side of the broad terminal
	 * mouth; five gill slits a side under the front of the body; a small dorsal fin and a thin whip of a tail (0.9 m).
	 * Vertex colour: R = |y| / half span (the material flaps the wings with it); G = x from -1 (the tail's end) to +1 (the
	 * lobes' tips); B = 1 on the underside; A = 1 in the gill slits and the mouth. UV0 = (x, y) metres.
	 */
	namespace Manta
	{
		constexpr double HalfSpan = 1.8, FrontX = 0.88, TailEndX = -1.58;

		/** The leading edge's x at eta = |y| / half span: straight across the head, then bowed and swept to the tip. */
		double LeadX(double Eta)
		{
			const double A = Sat((Eta - 0.15) / 0.85);
			return 0.62 - 0.97 * FMath::Pow(A, 1.5);
		}

		/** The trailing edge: the body's rear (the pelvic region, where the tail starts), then the wing's concave edge. */
		double TrailX(double Eta)
		{
			if (Eta < 0.15) { return -0.7 + 0.15 * Ramp(0.0, 0.15, Eta); }
			const double A = Sat((Eta - 0.15) / 0.85);
			return -0.55 + 0.2 * A + 0.22 * FMath::Sin(Pi * A) * (1.0 - 0.3 * A);
		}

		/** Half the thickness at its chord's thickest: 14 cm on the body, falling to 5 mm at the tips. */
		double HalfThick(double Eta)
		{
			const double A = Sat((Eta - 0.15) / 0.85);
			return 0.005 + 0.025 * FMath::Pow(1.0 - A, 1.2) + 0.11 * FMath::Exp(-FMath::Square(Eta / 0.16));
		}

		/** The thickness along the chord (0 leading edge … 1 trailing edge): round-nosed, thickest at 38 %. */
		double ChordThickness(double C) { return 2.375 * FMath::Sqrt(Sat(C)) * FMath::Pow(Sat(1.0 - C), 0.8); }

		/** Half the mouth's gape at y: the slot is 7 cm high, as wide as the head between the lobes. */
		double MouthGap(double Y) { return 0.035 * (1.0 - Ramp(0.13, 0.18, FMath::Abs(Y))); }

		/** The gill slits under the body (0 … 1): five a side, 6.5 cm apart, each a curved groove 20 cm long. */
		double Gills(double X, double Y)
		{
			const double Ay = FMath::Abs(Y);
			const double Window = Ramp(0.1, 0.14, Ay) * (1.0 - Ramp(0.28, 0.33, Ay));
			if (Window <= 0.0) { return 0.0; }
			double G = 0;
			for (int32 k = 0; k < 5; ++k)
			{
				const double Xk = 0.26 - 0.065 * k - 0.1 * FMath::Square((Ay - 0.21) / 0.1);
				G = FMath::Max(G, FMath::Exp(-FMath::Square((X - Xk) / 0.011)));
			}
			return G * Window;
		}

		/** A point of the disc's upper or lower surface at span position Y and chord fraction C. */
		FVector Surface(double Y, double C, bool bTop)
		{
			const double Eta = FMath::Abs(Y) / HalfSpan;
			const double Xl = LeadX(Eta), Xt = TrailX(Eta);
			const double X = Xl + (Xt - Xl) * C;
			const double Th = HalfThick(Eta) * ChordThickness(C);
			double Z = -0.06 * Eta * Eta + (bTop ? 1.15 * Th : -0.85 * Th);   // the wings droop a little at rest; the belly flatter than the back
			const double Gap = MouthGap(Y) * Ramp(0.06, 0.0, C);
			Z += bTop ? Gap : -Gap;
			if (!bTop) { Z += 0.007 * Gills(X, Y); }
			return FVector(X, Y, Z);
		}

		FLinearColor Paint(const FVector& Pt, double Belly, double Slot)
		{
			const double G = FMath::Clamp(-1.0 + 2.0 * (Pt.X - TailEndX) / (FrontX - TailEndX), -1.0, 1.0);
			return Rgba(Sat(FMath::Abs(Pt.Y) / HalfSpan), G, Belly, Slot);
		}

		/** Span stations, closer near the body (the head, the lobes and the gills need them). */
		double SpanY(int32 I, int32 NU)
		{
			const double Q = -1.0 + 2.0 * I / NU;
			return HalfSpan * FMath::Sign(Q) * FMath::Pow(FMath::Abs(Q), 1.4);
		}

		/** Chord stations under the body: closer over the gills (chord 20 % … 52 %). */
		double BellyChord(double K)
		{
			if (K < 0.2) { return K; }
			if (K < 0.8) { return 0.2 + (K - 0.2) * (0.32 / 0.6); }
			return 0.52 + (K - 0.8) * (0.48 / 0.2);
		}

		FMesh Make()
		{
			FMesh M;
			constexpr int32 NU = 56, NTop = 24, NBottom = 40;
			// The back.
			Sheet(M, NU, NTop, [&](int32 I, int32 J, FVector& OutP, FVector& OutN, FLinearColor& OutC, FVector2D& OutUv)
			{
				const double Cf = double(J) / NTop;
				const double Yy = SpanY(I, NU);
				OutP = Surface(Yy, Cf, true);
				OutN = FVector(0, 0, 1);
				OutC = Paint(OutP, 0.0, Ramp(0.04, 0.0, Cf) * MouthGap(Yy) / 0.035);
				OutUv = FVector2D(OutP.X, OutP.Y);
			});
			// The belly.
			Sheet(M, NU, NBottom, [&](int32 I, int32 J, FVector& OutP, FVector& OutN, FLinearColor& OutC, FVector2D& OutUv)
			{
				const double Cf = BellyChord(double(J) / NBottom);
				const double Yy = SpanY(I, NU);
				OutP = Surface(Yy, Cf, false);
				OutN = FVector(0, 0, -1);
				OutC = Paint(OutP, 1.0, FMath::Max(Gills(OutP.X, Yy), Ramp(0.04, 0.0, Cf) * MouthGap(Yy) / 0.035));
				OutUv = FVector2D(OutP.X, OutP.Y);
			});
			// The mouth: a slot between the lips, 10 cm deep, across the span stations where the gape is open.
			{
				int32 I0 = NU, I1 = 0;
				for (int32 i = 0; i <= NU; ++i)
				{
					if (MouthGap(SpanY(i, NU)) > 1e-4)
					{
						I0 = FMath::Min(I0, i);
						I1 = FMath::Max(I1, i);
					}
				}
				I0 = FMath::Max(I0 - 1, 0);
				I1 = FMath::Min(I1 + 1, NU);
				if (I1 > I0)
				{
					const double Dx[5] = {0.0, 0.6, 1.0, 0.6, 0.0};
					const double Dz[5] = {1.0, 0.8, 0.0, -0.8, -1.0};
					Sheet(M, I1 - I0, 4, [&](int32 I, int32 J, FVector& OutP, FVector& OutN, FLinearColor& OutC, FVector2D& OutUv)
					{
						const double Yy = SpanY(I0 + I, NU);
						const double Gap = MouthGap(Yy);
						const double Eta = FMath::Abs(Yy) / HalfSpan;
						OutP = FVector(LeadX(Eta) - 0.1 * (Gap / 0.035) * Dx[J], Yy, -0.06 * Eta * Eta + Gap * Dz[J]);
						OutN = J < 2 ? FVector(0, 0, -1) : (J == 2 ? FVector(1, 0, 0) : FVector(0, 0, 1));
						OutC = Paint(OutP, J >= 2 ? 1.0 : 0.0, 1.0);
						OutUv = FVector2D(OutP.X, OutP.Y);
					});
				}
			}
			// The cephalic lobes: sheets 30 cm long rolled into scrolls, forward either side of the mouth; their inner
			// (ventral) faces pale.
			for (const double Side : {1.0, -1.0})
			{
				for (const double Face : {1.0, -1.0})
				{
					constexpr int32 NL = 16, NW = 12;
					Sheet(M, NL, NW, [&](int32 I, int32 J, FVector& OutP, FVector& OutN, FLinearColor& OutC, FVector2D& OutUv)
					{
						const double U = double(I) / NL, V = double(J) / NW;
						const double Phi = -0.5 * Pi + 1.7 * Pi * V;
						const double Rad = 0.045 * (1.0 - 0.55 * V) * (1.0 - 0.35 * U);
						const FVector Axis(0.57 + 0.3 * U, Side * (0.235 - 0.02 * U), 0.01 - 0.02 * U);
						const FVector Radial(0.0, Side * FMath::Cos(Phi), FMath::Sin(Phi));
						OutP = Axis + Radial * (Rad + Face * 0.003 * (1.0 - 0.6 * U));
						OutN = Radial * Face;
						OutC = Paint(OutP, Face < 0.0 ? 1.0 : 0.0, 0.0);
						OutUv = FVector2D(OutP.X, OutP.Y);
					});
				}
			}
			// The dorsal fin, small, over the tail's root.
			{
				auto TopAt = [](double X) { return Surface(0.0, (LeadX(0.0) - X) / (LeadX(0.0) - TrailX(0.0)), true).Z; };
				TArray<FVector> Base, Keys, Edge;
				for (int32 i = 0; i <= 8; ++i)
				{
					const double X = -0.5 - 0.14 * i / 8.0;
					Base.Add(FVector(X, 0.0, TopAt(X) - 0.01));
				}
				Keys = {FVector(-0.5, 0.0, TopAt(-0.5)), FVector(-0.55, 0.0, TopAt(-0.55) + 0.05), FVector(-0.6, 0.0, TopAt(-0.6) + 0.075),
						FVector(-0.64, 0.0, TopAt(-0.64) + 0.04), FVector(-0.66, 0.0, TopAt(-0.66))};
				SmoothResample(Keys, 8, Edge);
				Fin(M, Base, Edge, 0.01, 4, [](const FVector& At, FVector2D& OutUv) { OutUv = FVector2D(At.X, At.Y); return Paint(At, 0.0, 0.0); });
			}
			// The tail: a thin whip, 1.3 cm thick at its root.
			{
				TArray<FVector> Path;
				TArray<double> Rad;
				for (int32 k = 0; k <= 20; ++k)
				{
					const double T = k / 20.0;
					Path.Add(FVector(-0.66 - 0.92 * T, 0.0, -0.04 * T * T));
					Rad.Add(0.013 * (1.0 - 0.85 * T));
				}
				Tube(M, Path, Rad, 8, false, true, 0.0, [](double, double, const FVector& At) { return Paint(At, 0.0, 0.0); });
			}
			Smooth(M, 0.0);
			for (int32 i = 0; i < M.Positions.Num(); ++i) { M.UV0[i] = FVector2D(M.Positions[i].X / CubeMesh::Cm, M.Positions[i].Y / CubeMesh::Cm); }
			return M;
		}
	}

	// ================================================================================================ C. corals and sponges

	/**
	 * Each on its own origin at its foot, z up. Vertex colour: R = height fraction within the thing; G = "tip-ness"
	 * (branch tips, rims, polyps: where the growing tissue is paler); B = a random constant per variant; A = 1 on living
	 * tissue, 0 on the dead or attached base. UV0 = (arc length or u, v) in metres.
	 */
	namespace Coral
	{
		/**
		 * A table coral (Acropora hyacinthus / cytherea): a flat plate 6 cm thick on a short thick stalk, its top a dense
		 * field of short upright branchlets (1–4 cm, a couple of centimetres apart, leaning outwards towards the rim), its
		 * rim irregular and turned up a little; the stalk's foot dead and encrusted.
		 */
		FMesh Table(double Diameter, int32 Seed)
		{
			FMesh M;
			FRandomStream Rng(Seed);
			const double Var = Rng.FRand();
			const double R = 0.5 * Diameter;
			const double StalkH = 0.18 + 0.12 * Diameter, StalkR = 0.05 + 0.05 * Diameter;
			const FVector Off(Rng.FRandRange(-0.05, 0.05) * Diameter, Rng.FRandRange(-0.05, 0.05) * Diameter, 0.0);
			const double TiltX = Rng.FRandRange(-0.04, 0.04), TiltY = Rng.FRandRange(-0.04, 0.04);
			const double ZTop = StalkH + 0.13 * R + 0.08;
			const double Sd = Seed * 0.37;
			auto Rim = [&](double Ang)
			{
				const double C = FMath::Cos(Ang), S = FMath::Sin(Ang);
				return R * (1.0 + 0.08 * Noise(C * 1.3, S * 1.3, Sd) + 0.04 * Noise(C * 4.1, S * 4.1, Sd + 5.0));
			};
			auto Thickness = [](double Fr) { return 0.06 * (1.0 - 0.5 * Fr * Fr); };
			// The plate's top at radius fraction Fr of the rim, angle Ang.
			auto TopAt = [&](double Fr, double Ang)
			{
				const double Rr = Fr * Rim(Ang);
				const double X = Off.X + Rr * FMath::Cos(Ang), Y = Off.Y + Rr * FMath::Sin(Ang);
				const double Z = StalkH + 0.03 + 0.1 * R * FMath::Pow(Fr, 3.0) + TiltX * X + TiltY * Y + 0.012 * Fbm(FVector(X, Y, Sd), 3.0, 2, 1.0);
				return FVector(X, Y, Z);
			};
			constexpr int32 NA = 160, NR = 28;
			Sheet(M, NA, NR, [&](int32 I, int32 J, FVector& OutP, FVector& OutN, FLinearColor& OutC, FVector2D& OutUv)
			{
				const double Fr = double(J) / NR;
				OutP = TopAt(Fr, TwoPi * (I % NA) / NA);
				OutN = FVector(0, 0, 1);
				OutC = Rgba(Sat(OutP.Z / ZTop), 0.3 + 0.5 * Ramp(0.8, 1.0, Fr), Var, 1.0);
				OutUv = FVector2D(OutP.X, OutP.Y);
			});
			// The underside, following the top 6 cm below (3 cm at the rim), a little lumpy.
			Sheet(M, NA, NR / 2, [&](int32 I, int32 J, FVector& OutP, FVector& OutN, FLinearColor& OutC, FVector2D& OutUv)
			{
				const double Fr = double(J) / (NR / 2);
				const FVector Top = TopAt(Fr, TwoPi * (I % NA) / NA);
				OutP = Top - FVector(0, 0, Thickness(Fr) + 0.008 * (1.0 + Fbm(Top, 4.0, 2, Sd + 2.0)) * Fr);
				OutN = FVector(0, 0, -1);
				OutC = Rgba(Sat(OutP.Z / ZTop), 0.1, Var, 1.0);
				OutUv = FVector2D(OutP.X, OutP.Y);
			});
			// The rim: the growing edge, rounded, pale.
			Sheet(M, NA, 2, [&](int32 I, int32 J, FVector& OutP, FVector& OutN, FLinearColor& OutC, FVector2D& OutUv)
			{
				const double Ang = TwoPi * (I % NA) / NA;
				const FVector Top = TopAt(1.0, Ang);
				const FVector Out(FMath::Cos(Ang), FMath::Sin(Ang), 0.0);
				const double Th = Thickness(1.0);
				// (the bottom row exactly the underside's edge)
				OutP = J == 0 ? Top : (J == 1 ? Top + Out * 0.012 - FVector(0, 0, 0.5 * Th) : Top - FVector(0, 0, Th + 0.008 * (1.0 + Fbm(Top, 4.0, 2, Sd + 2.0))));
				OutN = Out;
				OutC = Rgba(Sat(OutP.Z / ZTop), 0.9, Var, 1.0);
				OutUv = FVector2D(Ang * R, OutP.Z);
			});
			// The stalk: leaning from its foot to under the plate's centre, flared at both ends; its foot dead.
			Sheet(M, 24, 10, [&](int32 I, int32 J, FVector& OutP, FVector& OutN, FLinearColor& OutC, FVector2D& OutUv)
			{
				const double Ang = TwoPi * (I % 24) / 24.0;
				const double Zf = double(J) / 10.0;
				const double Z = -0.05 + (StalkH + 0.02) * Zf;
				const double Rr = StalkR * (1.0 + 0.35 * Ramp(0.3, 0.0, Zf) + 0.8 * Ramp(0.6, 1.0, Zf)) * (1.0 + 0.06 * Noise(FMath::Cos(Ang) * 2.0, FMath::Sin(Ang) * 2.0, Z * 5.0 + Sd));
				const FVector Out(FMath::Cos(Ang), FMath::Sin(Ang), 0.0);
				OutP = Off * Sat(Z / StalkH) + Out * Rr + FVector(0, 0, Z);
				OutN = Out;
				OutC = Rgba(Sat(OutP.Z / ZTop), 0.0, Var, Ramp(0.0, 0.6 * StalkH, Z));
				OutUv = FVector2D(Ang * StalkR, Z);
			});
			// The branchlets: little cones on a sunflower spiral (evenly spread), a couple of centimetres apart.
			const int32 Count = FMath::Min(4000, FMath::RoundToInt32(Pi * R * R / (0.02 * 0.02)));
			constexpr double Golden = 2.39996322972865332;
			for (int32 k = 0; k < Count; ++k)
			{
				const double Fr = FMath::Sqrt((k + 0.5) / Count) * 0.985;
				const double Ang = k * Golden + Rng.FRandRange(-0.2, 0.2) / FMath::Max(Fr * 10.0, 1.0);
				const FVector Foot = TopAt(Fr, Ang) - FVector(0, 0, 0.004);
				const double Hgt = Rng.FRandRange(0.018, 0.04) * (1.0 - 0.3 * Fr);
				const double Rad = Rng.FRandRange(0.005, 0.008);
				const FVector Out(FMath::Cos(Ang), FMath::Sin(Ang), 0.0);
				const FVector Dir = (FVector(0, 0, 1) + Out * (0.9 * Fr * Fr) + FVector(Rng.FRandRange(-0.15, 0.15), Rng.FRandRange(-0.15, 0.15), 0.0)).GetSafeNormal();
				FVector Ub, Wb;
				Basis(Dir, Ub, Wb);
				const int32 First = M.Positions.Num();
				for (int32 a = 0; a < 5; ++a)
				{
					const double A5 = TwoPi * a / 5.0;
					const FVector D = Ub * FMath::Cos(A5) + Wb * FMath::Sin(A5);
					const FVector Pt = Foot + D * Rad;
					M.V(Pt, D, FVector2D(Pt.X, Pt.Y), Rgba(Sat(Pt.Z / ZTop), 0.4, Var, 1.0));
				}
				const FVector TipP = Foot + Dir * Hgt;
				const int32 Tip = M.V(TipP, Dir, FVector2D(TipP.X, TipP.Y), Rgba(Sat(TipP.Z / ZTop), 1.0, Var, 1.0));
				for (int32 a = 0; a < 5; ++a) { M.Tri(First + a, First + (a + 1) % 5, Tip); }
			}
			Smooth(M, 0.01);
			return M;
		}

		/** A branch of a thicket waiting to grow: where it starts, its direction, radius, generation and arc length so far. */
		struct FBranch
		{
			FVector Start;
			FVector Dir;
			double Radius;
			int32 Depth;
			double Arc;
		};

		/**
		 * A staghorn thicket (Acropora, the staghorn group): branching cylinders 1–1.5 cm in radius, 10–30 cm between
		 * forks, growing up and out from a few stems into an interlocking thicket Width across and Height tall; every
		 * branch ends round, the tips (the axial corallites) paler; the thicket's base dead.
		 */
		FMesh Staghorn(double Width, double Height, int32 Seed)
		{
			FMesh M;
			FRandomStream Rng(Seed);
			const double Var = Rng.FRand();
			TArray<FBranch> Queue;
			const int32 Stems = FMath::RoundToInt32(5.0 + 5.0 * Width);
			for (int32 k = 0; k < Stems; ++k)
			{
				const double Ang = Rng.FRandRange(0.0, TwoPi), Rr = 0.28 * Width * FMath::Sqrt(double(Rng.FRand()));
				const FVector Out(FMath::Cos(Ang), FMath::Sin(Ang), 0.0);
				const double Tilt = FMath::DegreesToRadians(Rng.FRandRange(15.0, 50.0));
				const FVector Dir = (FVector(0, 0, FMath::Cos(Tilt)) + Out * FMath::Sin(Tilt)).GetSafeNormal();
				Queue.Add({FVector(Rr * FMath::Cos(Ang), Rr * FMath::Sin(Ang), -0.03), Dir, Rng.FRandRange(0.013, 0.016), 0, 0.0});
			}
			// Breadth first, so that the budget of branches is spent evenly over the thicket.
			const int32 MaxSegments = FMath::RoundToInt32(110.0 + 90.0 * Width);
			int32 Made = 0;
			for (int32 q = 0; q < Queue.Num(); ++q)
			{
				const FBranch Br = Queue[q];
				const double SegLen = Rng.FRandRange(0.1, 0.28);
				const int32 Steps = FMath::Max(2, FMath::CeilToInt32(SegLen / 0.045));
				TArray<FVector> Path;
				TArray<double> Rad;
				FVector Pt = Br.Start, D = Br.Dir;
				Path.Add(Pt);
				Rad.Add(Br.Radius);
				double SegArc = 0;
				bool bTop = false;
				for (int32 s = 1; s <= Steps; ++s)
				{
					D = (D + FVector(Rng.FRandRange(-0.12, 0.12), Rng.FRandRange(-0.12, 0.12), Rng.FRandRange(-0.05, 0.1) + 0.06)).GetSafeNormal();
					if (FVector2D(Pt.X, Pt.Y).Size() > 0.45 * Width) { D = (D - FVector(Pt.X, Pt.Y, 0.0).GetSafeNormal() * 0.25).GetSafeNormal(); }
					const FVector Next = Pt + D * (SegLen / Steps);
					SegArc += (Next - Pt).Size();
					Pt = Next;
					Path.Add(Pt);
					Rad.Add(Br.Radius * (1.0 - 0.07 * s / Steps));
					if (Pt.Z > Height)
					{
						bTop = true;
						break;
					}
				}
				++Made;
				const bool bTip = bTop || Br.Depth >= 6 || Made + (Queue.Num() - q - 1) >= MaxSegments;
				if (!bTip)
				{
					const int32 Kids = Rng.FRand() < 0.25 ? 3 : 2;
					FVector Ud, Wd;
					Basis(D, Ud, Wd);
					const double Spin = Rng.FRandRange(0.0, TwoPi);
					for (int32 c = 0; c < Kids; ++c)
					{
						const double Turn = Spin + TwoPi * c / Kids;
						const double Ang = FMath::DegreesToRadians(c == 0 ? Rng.FRandRange(5.0, 20.0) : Rng.FRandRange(25.0, 45.0));
						const FVector Kd = (D * FMath::Cos(Ang) + (Ud * FMath::Cos(Turn) + Wd * FMath::Sin(Turn)) * FMath::Sin(Ang) + FVector(0, 0, 0.08)).GetSafeNormal();
						Queue.Add({Pt, Kd, Rad.Last() * 0.97, Br.Depth + 1, Br.Arc + SegArc});
					}
				}
				Tube(M, Path, Rad, 8, false, true, Br.Arc, [&](double, double Frac, const FVector& At)
				{
					return Rgba(Sat(At.Z / Height), bTip ? Ramp(0.55, 1.0, Frac) : 0.12, Var, Ramp(0.02, 0.14, At.Z));
				});
			}
			Smooth(M, 0.01);
			return M;
		}

		/** A lobe of a massive coral head: a direction from its centre, a height and an angular width. */
		struct FLobe
		{
			FVector Dir;
			double Amp;
			double Width;
		};

		/**
		 * A massive coral head (Porites lutea / lobata, or a brain coral): a lumpy dome Diameter across and about half as
		 * tall, with lobes and hummocks, sitting 10 cm sunk into the bottom (its sides a little undercut at the foot); dead
		 * patches here and there. UV0 is an azimuthal equidistant map from the top (arc length from the top along the
		 * surface, round the head), seamless, for the material's meandering grooves.
		 */
		FMesh Massive(double Diameter, int32 Seed)
		{
			FMesh M;
			FRandomStream Rng(Seed);
			const double Var = Rng.FRand();
			const double R = 0.5 * Diameter, H = 0.55 * Diameter, Sunk = 0.1;
			const double Sd = Seed * 0.53;
			TArray<FLobe> Lobes;
			const int32 NLobes = FMath::RoundToInt32(5.0 + 4.0 * Diameter);
			for (int32 k = 0; k < NLobes; ++k)
			{
				const double Az = Rng.FRandRange(0.0, TwoPi), El = FMath::DegreesToRadians(Rng.FRandRange(15.0, 80.0));
				Lobes.Add({FVector(FMath::Cos(El) * FMath::Cos(Az), FMath::Cos(El) * FMath::Sin(Az), FMath::Sin(El)), Rng.FRandRange(0.06, 0.16) * R, Rng.FRandRange(0.35, 0.6)});
			}
			const int32 NA = FMath::RoundToInt32(96.0 + 32.0 * Diameter), NV = NA / 2;
			// The profile (from the top, v = 0, to the buried foot, v = 1) and its arc length.
			TArray<FVector2D> Prof;
			TArray<double> Arc;
			for (int32 j = 0; j <= NV; ++j)
			{
				const double Psi = 0.5 * Pi * j / NV;
				const double Rho = R * FMath::Pow(FMath::Sin(Psi), 0.7) * (1.0 - 0.06 * Ramp(0.85, 1.0, double(j) / NV));
				const double Z = -Sunk + (H + Sunk) * FMath::Pow(FMath::Max(FMath::Cos(Psi), 0.0), 0.8);
				Prof.Add(FVector2D(Rho, Z));
				Arc.Add(j == 0 ? 0.0 : Arc.Last() + (Prof[j] - Prof[j - 1]).Size());
			}
			const FVector Centre(0, 0, 0.3 * H);
			Sheet(M, NA, NV, [&](int32 I, int32 J, FVector& OutP, FVector& OutN, FLinearColor& OutC, FVector2D& OutUv)
			{
				const double Phi = TwoPi * (I % NA) / NA;
				const double C = FMath::Cos(Phi), S = FMath::Sin(Phi);
				const FVector2D& Pr = Prof[J];
				const FVector2D Tg = Prof[FMath::Min(J + 1, NV)] - Prof[FMath::Max(J - 1, 0)];
				const FVector2D N2 = FVector2D(-Tg.Y, Tg.X).GetSafeNormal();
				const FVector Nrm = J == 0 ? FVector(0, 0, 1) : FVector(N2.X * C, N2.X * S, N2.Y);
				const FVector Pb(Pr.X * C, Pr.X * S, Pr.Y);
				const FVector Dv = (Pb - Centre).GetSafeNormal();
				double Lobe = 0;
				for (const FLobe& Lb : Lobes) { Lobe += Lb.Amp * FMath::Exp(-2.0 * (1.0 - FVector::DotProduct(Dv, Lb.Dir)) / FMath::Square(Lb.Width)); }
				const double Hummock = 0.035 * R * Fbm(Pb, 1.6 / R, 3, Sd) + 0.012 * R * Fbm(Pb, 6.0 / R, 2, Sd + 4.0);
				OutP = Pb + Nrm * ((Lobe + Hummock) * Ramp(-0.1, 0.05, Pb.Z));
				OutN = Nrm;
				const double Dead = Ramp(0.2, 0.35, Fbm(Pb, 1.2 / R, 2, Sd + 7.0));
				OutC = Rgba(Sat((OutP.Z + Sunk) / (H + Sunk + 0.1 * R)), Sat(0.7 * Lobe / (0.12 * R) + 0.3 * Ramp(-0.1, 0.3, Hummock / (0.035 * R))), Var,
							Ramp(-0.03, 0.08, OutP.Z) * (1.0 - Dead));
				OutUv = FVector2D(C, S) * Arc[J];
			});
			Smooth(M, 0.01);
			return M;
		}

		/**
		 * A Dendronephthya soft coral (a "carnation coral"): a translucent, thick, fleshy trunk dividing into branches and
		 * stalks, each ending in a bundle of small polyp balls (radius 4–8 mm): a bushy crown Height tall.
		 */
		FMesh SoftCoral(double Height, int32 Seed)
		{
			FMesh M;
			FRandomStream Rng(Seed);
			const double Var = Rng.FRand();
			const double TrunkH = 0.35 * Height, TrunkR = 0.07 * Height;
			double Tipness = 0.0, TipGain = 0.0;
			auto Paint = [&](double, double Frac, const FVector& At) { return Rgba(Sat(At.Z / Height), Tipness + TipGain * Frac, Var, Ramp(0.0, 0.02, At.Z)); };
			auto Bent = [&](const FVector& From, const FVector& Dir, double Len, double Bend, int32 Points, TArray<FVector>& OutPath)
			{
				OutPath.Reset();
				FVector Pt = From, D = Dir;
				OutPath.Add(Pt);
				for (int32 k = 1; k <= Points; ++k)
				{
					D = (D + FVector(Rng.FRandRange(-Bend, Bend), Rng.FRandRange(-Bend, Bend), Bend * 0.8)).GetSafeNormal();
					Pt += D * (Len / Points);
					OutPath.Add(Pt);
				}
			};
			auto Radii = [](double R0, double R1, int32 Num)
			{
				TArray<double> Out;
				for (int32 k = 0; k < Num; ++k) { Out.Add(FMath::Lerp(R0, R1, double(k) / FMath::Max(Num - 1, 1))); }
				return Out;
			};
			TArray<FVector> Path;
			// The trunk.
			Bent(FVector(0, 0, -0.02), FVector(Rng.FRandRange(-0.1, 0.1), Rng.FRandRange(-0.1, 0.1), 1.0).GetSafeNormal(), TrunkH + 0.02, 0.05, 6, Path);
			Tipness = 0.0;
			TipGain = 0.05;
			Tube(M, Path, Radii(TrunkR * 1.2, TrunkR * 0.85, Path.Num()), 12, true, true, 0.0, Paint);
			const FVector Crown = Path.Last();
			const int32 Mains = 3 + FMath::RoundToInt32(Height * 4.0);
			const double Spin = Rng.FRandRange(0.0, TwoPi);
			for (int32 m = 0; m < Mains; ++m)
			{
				const double Az = Spin + TwoPi * (m + Rng.FRandRange(-0.25, 0.25)) / Mains;
				const double Tilt = FMath::DegreesToRadians(Rng.FRandRange(25.0, 55.0));
				const FVector MainDir(FMath::Sin(Tilt) * FMath::Cos(Az), FMath::Sin(Tilt) * FMath::Sin(Az), FMath::Cos(Tilt));
				TArray<FVector> MainPath;
				Bent(Crown - MainDir * (0.5 * TrunkR), MainDir, Rng.FRandRange(0.22, 0.3) * Height, 0.12, 5, MainPath);
				Tipness = 0.15;
				TipGain = 0.1;
				Tube(M, MainPath, Radii(TrunkR * 0.55, TrunkR * 0.4, MainPath.Num()), 10, false, true, TrunkH, Paint);
				const int32 Subs = 3 + FMath::RoundToInt32(Height * 4.0);
				for (int32 s = 0; s < Subs; ++s)
				{
					const FVector From = MainPath[FMath::Clamp(2 + s * (MainPath.Num() - 2) / Subs, 1, MainPath.Num() - 1)];
					FVector U, W;
					Basis(MainDir, U, W);
					const double Ang = Rng.FRandRange(0.0, TwoPi), Off = FMath::DegreesToRadians(Rng.FRandRange(25.0, 50.0));
					const FVector SubDir = (MainDir * FMath::Cos(Off) + (U * FMath::Cos(Ang) + W * FMath::Sin(Ang)) * FMath::Sin(Off) + FVector(0, 0, 0.2)).GetSafeNormal();
					TArray<FVector> SubPath;
					Bent(From, SubDir, Rng.FRandRange(0.1, 0.16) * Height, 0.15, 4, SubPath);
					Tipness = 0.3;
					TipGain = 0.15;
					Tube(M, SubPath, Radii(TrunkR * 0.3, TrunkR * 0.22, SubPath.Num()), 8, false, true, TrunkH, Paint);
					const int32 Twigs = 2 + Rng.RandRange(0, 2);
					for (int32 t = 0; t < Twigs; ++t)
					{
						const FVector TwigFrom = SubPath[FMath::Clamp(SubPath.Num() - 1 - t, 1, SubPath.Num() - 1)];
						const FVector TwigDir = (SubDir + FVector(Rng.FRandRange(-0.6, 0.6), Rng.FRandRange(-0.6, 0.6), Rng.FRandRange(0.0, 0.5))).GetSafeNormal();
						TArray<FVector> TwigPath;
						Bent(TwigFrom, TwigDir, Rng.FRandRange(0.02, 0.05), 0.2, 2, TwigPath);
						Tipness = 0.6;
						TipGain = 0.2;
						Tube(M, TwigPath, Radii(0.003, 0.0025, TwigPath.Num()), 6, false, true, TrunkH, Paint);
						// The polyp bundle: 5–9 balls round the twig's end.
						const FVector End = TwigPath.Last();
						const int32 Balls = Rng.RandRange(5, 9);
						for (int32 b = 0; b < Balls; ++b)
						{
							const double Br = Rng.FRandRange(0.004, 0.008);
							const FVector Bc = End + TwigDir * 0.004 + FVector(Rng.FRandRange(-1.0, 1.0), Rng.FRandRange(-1.0, 1.0), Rng.FRandRange(-0.4, 1.0)).GetSafeNormal() * Rng.FRandRange(0.003, 0.012);
							Ellipsoid(M, Bc, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FVector(Br, Br, Br * 0.9), 6, 4,
									  [&](const FVector& At, const FVector& Unit, FLinearColor& OutC, FVector2D& OutUv)
									  {
										  OutC = Rgba(Sat(At.Z / Height), 1.0, Var, 1.0);
										  OutUv = FVector2D(Unit.X, Unit.Y) * Br;
									  });
						}
					}
				}
			}
			Smooth(M, 0.001);
			return M;
		}

		/** A node of a sea fan's net: where it is, its angle from the fan's axis, and whether it lies inside the outline. */
		struct FFanNode
		{
			FVector Pt;
			double Al;
			bool bValid;
		};

		/**
		 * A gorgonian sea fan (Annella mollis, Subergorgia): a short trunk from a holdfast, then a fan-shaped net of
		 * branches in the local x–z plane, Width across: branches radiating and forking as the fan widens, joined sideways
		 * into meshes; 8 mm in radius near the base tapering to 2 mm at the rim; the plane gently curved. The rim (where the
		 * fan grows) paler.
		 */
		FMesh SeaFan(double Width, int32 Seed)
		{
			FMesh M;
			FRandomStream Rng(Seed);
			const double Var = Rng.FRand();
			const double Cell = 0.012 * Width + 0.005;
			const double H0 = 0.08 * Width;
			const double HalfAngle = FMath::DegreesToRadians(80.0);
			const double RMax = 0.5 * Width / FMath::Sin(HalfAngle);
			const double Sd = Seed * 0.61;
			auto Outline = [&](double Al) { return RMax * (1.0 - 0.18 * FMath::Pow(FMath::Abs(Al) / HalfAngle, 6.0)) * (1.0 + 0.07 * Noise(Al * 1.7, Sd, 0.5)); };
			auto FanPoint = [&](double Rr, double Al)
			{
				const double X = Rr * FMath::Sin(Al), Z = H0 + Rr * FMath::Cos(Al);
				const double Y = 0.06 * Width * FMath::Square(X / (0.5 * Width)) + 0.015 * Width * Noise(X * 1.3, Z * 1.3, Sd + 0.29);
				return FVector(X, Y, Z);
			};
			auto Paint = [&](double, double, const FVector& At)
			{
				const double Rr = FMath::Sqrt(At.X * At.X + FMath::Square(At.Z - H0));
				const double Al = FMath::Atan2(At.X, At.Z - H0);
				return Rgba(Sat(At.Z / (H0 + RMax)), Ramp(0.65, 1.0, Rr / Outline(Al)), Var, Ramp(0.0, 0.05, At.Z));
			};
			// A straight piece of branch from A to B, a little longer at each end so the joints close.
			auto Link = [&](const FVector& A, const FVector& B, double R0, double R1, double ArcAt)
			{
				const FVector D = (B - A).GetSafeNormal();
				const TArray<FVector> Path = {A - D * R0 * 0.8, B + D * R1 * 0.8};
				const TArray<double> Rad = {R0, R1};
				Tube(M, Path, Rad, FMath::Max(R0, R1) > 0.004 ? 6 : 5, false, false, ArcAt, Paint);
			};
			// The trunk from the holdfast.
			{
				const FVector Hub(0, 0, H0);
				const TArray<FVector> Path = {FVector(0, 0, -0.03), FVector(0, 0, 0.3 * H0), FVector(0, 0.002, 0.7 * H0), Hub};
				const double Tr = 0.012 + 0.004 * Width;
				const TArray<double> Rad = {Tr * 1.6, Tr, Tr * 0.95, Tr * 0.9};
				Tube(M, Path, Rad, 10, false, true, 0.0, Paint);
			}
			// The net: rings of nodes Cell apart, each ring with as many as fit its arc.
			const double RFirst = 0.06 * Width;
			const int32 K = FMath::Max(1, FMath::CeilToInt32((RMax - RFirst) / Cell));
			TArray<TArray<FFanNode>> Rings;
			Rings.SetNum(K + 1);
			for (int32 k = 0; k <= K; ++k)
			{
				const double Rk = RFirst + k * Cell;
				const int32 Num = FMath::Max(3, FMath::RoundToInt32(2.0 * HalfAngle * Rk / Cell));
				for (int32 j = 0; j < Num; ++j)
				{
					const double Al = -HalfAngle + 2.0 * HalfAngle * (j + 0.5 + 0.3 * Rng.FRandRange(-1.0, 1.0)) / Num;
					const double Rr = Rk + Cell * 0.3 * Rng.FRandRange(-1.0, 1.0);
					Rings[k].Add({FanPoint(Rr, Al), Al, Rr <= Outline(Al)});
				}
			}
			auto Thick = [&](int32 Ring) { return 0.008 - 0.006 * FMath::Pow(double(Ring) / K, 0.6); };
			// The primaries, from the trunk's top to the first ring.
			for (const FFanNode& Nd : Rings[0])
			{
				if (Nd.bValid) { Link(FVector(0, 0, H0), Nd.Pt, 0.0085, Thick(0), 0.0); }
			}
			for (int32 k = 0; k < K; ++k)
			{
				const TArray<FFanNode>& In = Rings[k];
				const TArray<FFanNode>& Outer = Rings[k + 1];
				// Radial: each node to the nearest (by angle) living node of the ring inside it (so branches fork outwards).
				for (const FFanNode& Nd : Outer)
				{
					if (!Nd.bValid) { continue; }
					int32 Best = INDEX_NONE;
					double BestD = 1e9;
					for (int32 j = 0; j < In.Num(); ++j)
					{
						const double Dd = FMath::Abs(In[j].Al - Nd.Al);
						if (In[j].bValid && Dd < BestD)
						{
							BestD = Dd;
							Best = j;
						}
					}
					if (Best != INDEX_NONE) { Link(In[Best].Pt, Nd.Pt, Thick(k), Thick(k + 1), RFirst + k * Cell); }
				}
				// Sideways: neighbours on the ring, often, into the net's meshes.
				for (int32 j = 0; j + 1 < In.Num(); ++j)
				{
					if (In[j].bValid && In[j + 1].bValid && Rng.FRand() < 0.55)
					{
						const double Tk = FMath::Max(0.0022, 0.8 * Thick(k));
						Link(In[j].Pt, In[j + 1].Pt, Tk, Tk, RFirst + k * Cell);
					}
				}
			}
			Smooth(M, 0.005);
			return M;
		}

		/**
		 * A giant barrel sponge (Xestospongia testudinaria): a thick-walled vase Height tall and nearly as wide, the wall
		 * 10–12 cm thick, open at the top into a deep hollow; its outside cut by deep, sharp-crested, wandering vertical
		 * ridges with knobs on them (the inside smoother); the rim scalloped by the ridges. A lathe with its seam welded.
		 */
		FMesh BarrelSponge(double Height, int32 Seed)
		{
			FMesh M;
			FRandomStream Rng(Seed);
			const double Var = Rng.FRand();
			const double H = Height, RMax = 0.42 * Height, Wall = 0.12 * RMax + 0.03, FloorZ = 0.35 * Height;
			const double Sd = Seed * 0.43;
			const TArray<FVector2D> OuterK = {{0.0, 0.58}, {0.15, 0.75}, {0.35, 0.92}, {0.55, 1.0}, {0.75, 0.98}, {0.9, 0.93}, {1.0, 0.9}};
			auto Outer = [&](double Z) { return RMax * Curve(OuterK, Z / H); };
			const int32 NRidge = 14 + FMath::RoundToInt32(6.0 * Height);
			// The profile: up the outside, over the lip, down the inside, across the hollow's floor. (rho, z, part)
			TArray<FVector> Prof;
			for (int32 k = 0; k <= 44; ++k)
			{
				const double Z = -0.05 + (H + 0.05) * k / 44.0;
				Prof.Add(FVector(Outer(Z), Z, 0.0));
			}
			const double RimO = Outer(H);
			for (int32 k = 1; k < 8; ++k)
			{
				const double A = Pi * k / 8.0;
				Prof.Add(FVector(RimO - Wall * 0.5 * (1.0 - FMath::Cos(A)), H + 0.5 * Wall * FMath::Sin(A), 1.0));
			}
			for (int32 k = 0; k <= 28; ++k)
			{
				const double Z = H - (H - FloorZ) * k / 28.0;
				Prof.Add(FVector(Outer(Z) - Wall * (1.0 + 0.2 * (H - Z) / H), Z, 2.0));
			}
			const double InnerFloor = Outer(FloorZ) - Wall * (1.0 + 0.2 * (H - FloorZ) / H);
			for (int32 k = 1; k <= 9; ++k)
			{
				const double F = double(k) / 9.0;
				Prof.Add(FVector(InnerFloor * (1.0 - F), FloorZ - 0.05 * FMath::Sin(0.5 * Pi * F), 3.0));
			}
			const int32 NV = Prof.Num() - 1;
			TArray<double> Arc;
			for (int32 j = 0; j <= NV; ++j) { Arc.Add(j == 0 ? 0.0 : Arc.Last() + FVector2D(Prof[j].X - Prof[j - 1].X, Prof[j].Y - Prof[j - 1].Y).Size()); }
			auto RidgeAt = [&](double Phi, double Z)
			{
				const double C = FMath::Cos(Phi), S = FMath::Sin(Phi);
				const double Wander = 1.4 * Noise(C * 1.5, S * 1.5, Z * 2.2 / H + Sd) + 0.5 * Noise(C * 4.0, S * 4.0, Z * 5.0 / H + Sd);
				const double Crest = 1.0 - FMath::Pow(FMath::Abs(FMath::Sin(0.5 * (NRidge * Phi + 2.0 * Wander))), 0.6);
				return Crest * (0.65 + 0.35 * Noise(C * 3.0, S * 3.0, Z * 3.0 / H + 7.7 + Sd));
			};
			const int32 NA = FMath::RoundToInt32(128.0 + 32.0 * Height);
			Sheet(M, NA, NV, [&](int32 I, int32 J, FVector& OutP, FVector& OutN, FLinearColor& OutC, FVector2D& OutUv)
			{
				const double Phi = TwoPi * (I % NA) / NA;
				const double C = FMath::Cos(Phi), S = FMath::Sin(Phi);
				const FVector& Pr = Prof[J];
				const FVector Tg = Prof[FMath::Min(J + 1, NV)] - Prof[FMath::Max(J - 1, 0)];
				const FVector2D N2 = FVector2D(Tg.Y, -Tg.X).GetSafeNormal();
				const FVector Nrm = J == NV ? FVector(0, 0, 1) : FVector(N2.X * C, N2.X * S, N2.Y);
				const FVector Pb(Pr.X * C, Pr.X * S, Pr.Y);
				const int32 Part = FMath::RoundToInt32(Pr.Z);
				const double Crest = RidgeAt(Phi, FMath::Min(Pr.Y, H));
				const double Amp = 0.13 * RMax * Ramp(0.0, 0.25 * H, Pr.Y) * (Part == 0 ? 1.0 : (Part == 1 ? 0.6 : (Part == 2 ? 0.25 * Ramp(FloorZ, FloorZ + 0.15 * H, Pr.Y) : 0.0)));
				const double Knobs = 0.025 * RMax * Sat(Noise(Pb.X * 8.0 / RMax, Pb.Y * 8.0 / RMax, Pb.Z * 8.0 / RMax + Sd)) * Crest * (Part == 0 ? 1.0 : 0.0);
				OutP = Pb + Nrm * (Amp * Crest + Knobs);
				OutN = Nrm;
				const double Rim = Part == 1 ? 1.0 : Ramp(0.85 * H, H, Pr.Y);
				OutC = Rgba(Sat(OutP.Z / (H + Wall)), FMath::Max(Rim, 0.5 * Crest), Var, Ramp(0.0, 0.1, OutP.Z));
				OutUv = FVector2D(Phi * RMax, Arc[J]);
			});
			// The foot's underside (buried).
			{
				const int32 Centre = M.V(FVector(0, 0, -0.05), FVector(0, 0, -1), FVector2D::ZeroVector, Rgba(0, 0, Var, 0));
				for (int32 i = 0; i < NA; ++i) { M.Tri(Centre, i, i + 1); }
			}
			Smooth(M, 0.01);
			return M;
		}

		/**
		 * A leather coral (Sarcophyton): a smooth, pale stalk and a mushroom-shaped cap (the capitulum) CapD across, its
		 * margin thrown into wavy folds; the polyps on the cap's top (G = 1 there). A lathe from the stalk's foot, up and
		 * out under the cap, round its margin and in over its top.
		 */
		FMesh LeatherCoral(double CapD, int32 Seed)
		{
			FMesh M;
			FRandomStream Rng(Seed);
			const double Var = Rng.FRand();
			const double Rc = 0.5 * CapD, Rs = 0.28 * Rc, Hs = 0.45 * Rc + 0.05, Tc = 0.12 * Rc;
			const double Sd = Seed * 0.71;
			const int32 Folds = Rng.RandRange(5, 7);
			auto Under = [&](double Rho) { return Hs + 0.22 * Rc * FMath::Pow(Sat(Rho / Rc), 1.6); };
			// (rho, z, part: 0 stalk, 1 underside, 2 margin, 3 top)
			TArray<FVector> Prof;
			for (int32 k = 0; k <= 12; ++k)
			{
				const double Z = -0.03 + (Hs + 0.03) * k / 12.0;
				Prof.Add(FVector(Rs * (1.25 - 0.25 * Ramp(0.0, 0.3 * Hs, Z) + 0.3 * Ramp(0.6 * Hs, Hs, Z)), Z, 0.0));
			}
			for (int32 k = 1; k <= 16; ++k)
			{
				const double Rho = FMath::Lerp(1.3 * Rs, Rc, double(k) / 16.0);
				Prof.Add(FVector(Rho, Under(Rho), 1.0));
			}
			const double ZRim = Under(Rc);
			for (int32 k = 1; k < 6; ++k)
			{
				const double A = Pi * k / 6.0;
				Prof.Add(FVector(Rc + 0.5 * Tc * FMath::Sin(A), ZRim + 0.5 * Tc * (1.0 - FMath::Cos(A)), 2.0));
			}
			for (int32 k = 0; k <= 24; ++k)
			{
				const double Rho = Rc * (1.0 - double(k) / 24.0);
				const double Z = Under(Rho) + Tc * (1.0 + 0.8 * (1.0 - Rho / Rc)) - 0.04 * Rc * FMath::Exp(-FMath::Square(Rho / (0.3 * Rc)));
				Prof.Add(FVector(Rho, Z, 3.0));
			}
			const int32 NV = Prof.Num() - 1;
			TArray<double> Arc;
			for (int32 j = 0; j <= NV; ++j) { Arc.Add(j == 0 ? 0.0 : Arc.Last() + FVector2D(Prof[j].X - Prof[j - 1].X, Prof[j].Y - Prof[j - 1].Y).Size()); }
			const double ZMax = Hs + 0.22 * Rc + 2.0 * Tc + 0.3 * Rc;
			constexpr int32 NA = 128;
			Sheet(M, NA, NV, [&](int32 I, int32 J, FVector& OutP, FVector& OutN, FLinearColor& OutC, FVector2D& OutUv)
			{
				const double Phi = TwoPi * (I % NA) / NA;
				const double C = FMath::Cos(Phi), S = FMath::Sin(Phi);
				const FVector& Pr = Prof[J];
				const FVector Tg = Prof[FMath::Min(J + 1, NV)] - Prof[FMath::Max(J - 1, 0)];
				const FVector2D N2 = FVector2D(Tg.Y, -Tg.X).GetSafeNormal();
				const FVector Nrm = J == NV ? FVector(0, 0, 1) : FVector(N2.X * C, N2.X * S, N2.Y);
				const int32 Part = FMath::RoundToInt32(Pr.Z);
				// The folds: the cap's margin rises and falls (more towards the edge), the rim wobbles in and out.
				const double Wave = FMath::Sin(Folds * Phi + 1.4 * Noise(C * 1.2, S * 1.2, Sd));
				const double Fr = Part == 0 ? 0.0 : Sat((Pr.X - 1.3 * Rs) / (Rc - 1.3 * Rs));
				const double Dz = 0.28 * Rc * FMath::Pow(Fr, 2.2) * Wave;
				const double Rho = Pr.X * (1.0 + 0.05 * Fr * Fr * FMath::Sin((Folds + 3) * Phi + 2.0 * Noise(C, S, Sd + 3.0)));
				OutP = FVector(Rho * C, Rho * S, Pr.Y + Dz);
				OutN = Nrm;
				OutC = Rgba(Sat(OutP.Z / ZMax), Part == 3 ? 1.0 : (Part == 2 ? 0.6 : 0.0), Var, Ramp(0.0, 0.04, OutP.Z));
				OutUv = FVector2D(Phi * Rc * 0.5, Arc[J]);
			});
			{
				const int32 Centre = M.V(FVector(0, 0, -0.03), FVector(0, 0, -1), FVector2D::ZeroVector, Rgba(0, 0, Var, 0));
				for (int32 i = 0; i < NA; ++i) { M.Tri(Centre, i, i + 1); }
			}
			Smooth(M, 0.01);
			return M;
		}

		/**
		 * A reef rock or rubble mound (Size across): a cube-sphere squashed and made craggy with ridged noise, lifted to sit
		 * on the bottom with its underside flattened into it. Colour: G = crags (the proud parts), A = 0 (bare rock; the
		 * material encrusts it).
		 */
		FMesh Rock(double Size, int32 Seed)
		{
			FMesh M;
			FRandomStream Rng(Seed);
			const double Var = Rng.FRand();
			const FVector Radii(0.5 * Size * Rng.FRandRange(0.9, 1.1), 0.5 * Size * Rng.FRandRange(0.7, 0.9), 0.5 * Size * Rng.FRandRange(0.55, 0.75));
			const double Sd = Seed * 0.13;
			const FVector Axes[6][3] = {{FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1)}, {FVector(-1, 0, 0), FVector(0, 0, 1), FVector(0, 1, 0)},
										{FVector(0, 1, 0), FVector(0, 0, 1), FVector(1, 0, 0)}, {FVector(0, -1, 0), FVector(1, 0, 0), FVector(0, 0, 1)},
										{FVector(0, 0, 1), FVector(1, 0, 0), FVector(0, 1, 0)}, {FVector(0, 0, -1), FVector(0, 1, 0), FVector(1, 0, 0)}};
			constexpr int32 N = 44;
			for (int32 f = 0; f < 6; ++f)
			{
				Sheet(M, N, N, [&](int32 I, int32 J, FVector& OutP, FVector& OutN, FLinearColor& OutC, FVector2D& OutUv)
				{
					const FVector Cube = Axes[f][0] + Axes[f][1] * (-1.0 + 2.0 * I / N) + Axes[f][2] * (-1.0 + 2.0 * J / N);
					const FVector Dir = Cube.GetSafeNormal();
					const double Big = 0.22 * Fbm(Dir, 1.1, 3, Sd);
					const double Crag = 0.16 * (Ridged(Dir, 2.3, 3, Sd + 3.0) - 0.5);
					const double Grain = 0.04 * Fbm(Dir, 7.0, 2, Sd + 9.0);
					FVector Pt(Dir.X * Radii.X, Dir.Y * Radii.Y, Dir.Z * Radii.Z);
					Pt *= 1.0 + Big + Crag + Grain;
					Pt.Z += 0.35 * Radii.Z;
					if (Pt.Z < 0.0) { Pt.Z *= 0.25; }
					OutP = Pt;
					OutN = Dir;
					OutC = Rgba(Sat(Pt.Z / (1.6 * Radii.Z)), Sat(0.4 + 2.5 * (Big + Crag)), Var, 0.0);
					OutUv = FVector2D(Pt.X + 0.7 * Pt.Z, Pt.Y + 0.7 * Pt.Z);
				});
			}
			Smooth(M, 0.01);
			return M;
		}
	}

	// ================================================================================================ D. the pygmy seahorse

	/**
	 * Bargibant's pygmy seahorse (Hippocampus bargibanti), 2 cm long, which lives only on Muricella sea fans and matches
	 * their polyps: a very short snout, a big head, a bulbous trunk and a prehensile tail curled forward under it; the
	 * body covered with round tubercles. Upright, facing +x, the origin at its middle.
	 * Vertex colour: R = along the body, snout (0) to tail tip (1); A = 1 on the tubercles; B = 1 on the eyes.
	 * UV0 = (arc length, round the girth) on the body, (R, 0) on the tubercles and eyes.
	 */
	FMesh PygmySeahorse()
	{
		FMesh M;
		// The body's axis in millimetres (x forward, z up): snout, crown, neck, chest, belly, and the tail curling forward.
		const TArray<FVector> Keys = {FVector(4.2, 0, 7.2), FVector(2.8, 0, 7.4), FVector(1.4, 0, 7.6), FVector(0.4, 0, 6.8), FVector(0.2, 0, 5.4),
									  FVector(0.9, 0, 3.6), FVector(0.8, 0, 1.4), FVector(0.0, 0, -0.4), FVector(-0.6, 0, -2.4), FVector(-0.4, 0, -4.4),
									  FVector(0.8, 0, -5.8), FVector(2.4, 0, -5.8), FVector(3.2, 0, -4.6), FVector(2.8, 0, -3.4), FVector(1.9, 0, -3.3),
									  FVector(1.5, 0, -4.0)};
		// The girth (radius, mm) along the axis: snout, head, neck, the swollen trunk, the tapering tail.
		const TArray<FVector2D> Girth = {{0.0, 0.6}, {0.05, 0.75}, {0.1, 1.6}, {0.14, 2.0}, {0.2, 1.7}, {0.24, 1.5}, {0.32, 2.6}, {0.4, 3.1},
										 {0.47, 2.7}, {0.53, 1.9}, {0.62, 1.3}, {0.75, 0.9}, {0.9, 0.55}, {1.0, 0.35}};
		constexpr int32 NS = 90;
		TArray<FVector> Spine;
		SmoothResample(Keys, NS, Spine);
		double Length = 0;
		for (int32 k = 1; k <= NS; ++k) { Length += (Spine[k] - Spine[k - 1]).Size(); }
		const double Scale = 0.02 / Length;   // millimetres of the drawing to metres, the whole 2.0 cm long
		for (FVector& Pt : Spine) { Pt *= Scale; }
		TArray<double> Rad;
		for (int32 k = 0; k <= NS; ++k) { Rad.Add(Curve(Girth, double(k) / NS) * Scale); }
		Tube(M, Spine, Rad, 16, true, true, 0.0, [](double, double Frac, const FVector&) { return Rgba(Frac, 0.0, 0.0, 0.0); });
		// The snout's end, rounded.
		Ellipsoid(M, Spine[0], FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FVector(Rad[0], Rad[0], Rad[0]) * 1.05, 10, 6,
				  [](const FVector&, const FVector&, FLinearColor& OutC, FVector2D& OutUv) { OutC = Rgba(0, 0, 0, 0); OutUv = FVector2D::ZeroVector; });
		// A point on the body's surface at fraction S, turned Ang round the axis from its side (+y).
		auto OnBody = [&](double S, double Ang, double Out, FVector& OutDir)
		{
			const int32 Idx = FMath::Clamp(FMath::RoundToInt32(S * NS), 1, NS - 1);
			const FVector Tg = (Spine[Idx + 1] - Spine[Idx - 1]).GetSafeNormal();
			const FVector Side(0, 1, 0);
			const FVector InPlane = FVector::CrossProduct(Tg, Side).GetSafeNormal();
			OutDir = Side * FMath::Cos(Ang) + InPlane * FMath::Sin(Ang);
			return Spine[Idx] + OutDir * (Rad[Idx] + Out);
		};
		// The tubercles: round knobs a third to two thirds of a millimetre across, crowded on the head and trunk.
		FRandomStream Rng(2003);
		for (int32 k = 0; k < 64; ++k)
		{
			const double S = 0.03 + 0.85 * FMath::Pow(double(Rng.FRand()), 1.3);
			const double Tr = Rng.FRandRange(0.35, 0.75) * Scale * (0.6 + 0.4 * Curve(Girth, S) / 3.1);
			FVector Dir;
			const FVector Tc = OnBody(S, Rng.FRandRange(0.0, TwoPi), 0.3 * Tr, Dir);
			Ellipsoid(M, Tc, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FVector(Tr, Tr, Tr), 8, 5,
					  [S](const FVector&, const FVector&, FLinearColor& OutC, FVector2D& OutUv) { OutC = Rgba(S, 0.0, 0.0, 1.0); OutUv = FVector2D(S, 0.0); });
		}
		// The eyes, either side of the head behind the snout.
		for (const double Side : {1.0, -1.0})
		{
			const double Er = 0.6 * Scale;
			FVector Dir;
			const FVector Ec = OnBody(0.1, Side > 0.0 ? -0.35 : Pi + 0.35, -0.35 * Er, Dir);   // (a little above the side: the in-plane axis points down here)
			Ellipsoid(M, Ec, FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1), FVector(Er, Er, Er), 10, 6,
					  [](const FVector&, const FVector&, FLinearColor& OutC, FVector2D& OutUv) { OutC = Rgba(0.1, 0.0, 1.0, 0.0); OutUv = FVector2D(0.1, 0.0); });
		}
		Smooth(M, 0.0005);
		// The origin at its middle.
		FVector Lo(1e30, 1e30, 1e30), Hi(-1e30, -1e30, -1e30);
		for (const FVector& Pt : M.Positions)
		{
			Lo = Lo.ComponentMin(Pt);
			Hi = Hi.ComponentMax(Pt);
		}
		const FVector Mid = (Lo + Hi) * 0.5;
		for (FVector& Pt : M.Positions) { Pt -= Mid; }
		return M;
	}

	// ================================================================================================ E. the mangrove

	/**
	 * A red mangrove (Rhizophora apiculata / mucronata), as seen from under water. Its origin is the water surface over
	 * the trunk's foot: the trunk rises from among its roots to about 5.5 m, and a rough mass of foliage sits from 5 to
	 * 8 m; 28–38 prop roots (2.5–5 cm radius) spring from the trunk between 0.5 and 2.5 m, arch out and plunge into the
	 * bottom 1.5–3.5 m out, some forking once on the way down.
	 * Vertex colour: R = height over the whole tree (0 the roots' ends, 1 the canopy's top); G = 1 under water (z < 0);
	 * B = 1 on the canopy; A = nearness to the bottom (encrusting growth: 1 at the bottom, 0 from 1.8 m above it).
	 * Sections: the wood (MI_RA_Mangrove), the canopy (MI_RA_Canopy).
	 */
	namespace Mangrove
	{
		/**
		 * The bottom under the tree, from the water surface. Place 2 (the only place with mangroves): the bottom is
		 * 2.6–3.2 m under the eyes and the eyes 1.2 m under the surface, so the roots must reach 4.1 m down to meet it.
		 */
		constexpr double BottomZ = -4.1;
		constexpr double TopZ = 8.4;

		FLinearColor Paint(const FVector& Pt, double Canopy)
		{
			return Rgba(Sat((Pt.Z - (BottomZ - 0.3)) / (TopZ - (BottomZ - 0.3))), 1.0 - Ramp(-0.05, 0.05, Pt.Z), Canopy, Sat(1.0 - (Pt.Z - BottomZ) / 1.8));
		}

		void Make(FMesh& Wood, FMesh& Crown, int32 Seed)
		{
			FRandomStream Rng(Seed);
			const double Sd = Seed * 0.83;
			const double LeanX = Rng.FRandRange(-0.4, 0.4), LeanY = Rng.FRandRange(-0.3, 0.3);
			auto TrunkAt = [&](double Z) { return FVector(LeanX * FMath::Square(Z / 5.6), LeanY * (Z / 5.6), Z); };
			auto WoodPaint = [](double, double, const FVector& At) { return Paint(At, 0.0); };
			// The trunk.
			{
				TArray<FVector> Path;
				TArray<double> Rad;
				for (int32 k = 0; k <= 30; ++k)
				{
					const double Z = 0.3 + 5.5 * k / 30.0;
					Path.Add(TrunkAt(Z));
					Rad.Add(0.16 * (1.0 - 0.35 * (Z - 0.3) / 5.5) * (1.0 + 0.06 * Noise(Z * 1.3, Sd, 0.5)));
				}
				Tube(Wood, Path, Rad, 16, true, true, 0.0, WoodPaint);
			}
			// The prop roots.
			const int32 Roots = Rng.RandRange(28, 38);
			for (int32 r = 0; r < Roots; ++r)
			{
				const double Az = TwoPi * (r + Rng.FRandRange(-0.35, 0.35)) / Roots;
				const double Spring = Rng.FRandRange(0.5, 2.5);
				const double Reach = Rng.FRandRange(1.5, 3.5);
				const FVector Out(FMath::Cos(Az), FMath::Sin(Az), 0.0);
				const FVector Foot = TrunkAt(Spring);
				const FVector P0 = Foot + Out * 0.08;
				const FVector P3 = FVector(Foot.X, Foot.Y, 0.0) + Out * Reach + FVector(0, 0, BottomZ - 0.25);
				const FVector P1 = P0 + Out * (0.35 * Reach) + FVector(0, 0, 0.35 + 0.15 * Spring);
				const FVector P2 = P3 + FVector(0, 0, 1.3 + 0.5 * Spring) - Out * (0.1 * Reach);
				const double R0 = Rng.FRandRange(0.025, 0.05);
				TArray<FVector> Path;
				TArray<double> Rad;
				constexpr int32 Samples = 24;
				for (int32 k = 0; k <= Samples; ++k)
				{
					const double T = double(k) / Samples;
					const double Wob = FMath::Sin(Pi * T);
					const FVector B = Bezier(P0, P1, P2, P3, T);
					Path.Add(B + FVector(Noise(B.X * 1.1, B.Y * 1.1, B.Z * 1.1 + Sd), Noise(B.X * 1.1 + 7.0, B.Y * 1.1, B.Z * 1.1 + Sd), 0.0) * (0.08 * Wob));
					Rad.Add(R0 * (1.0 - 0.25 * T + 0.25 * Ramp(0.85, 1.0, T)));
				}
				Tube(Wood, Path, Rad, 8, false, true, 0.0, WoodPaint);
				// A fork: a second root leaves the first on its way down and plunges further out.
				if (Rng.FRand() < 0.4)
				{
					const int32 From = FMath::RoundToInt32(Rng.FRandRange(0.35, 0.6) * Samples);
					const FVector Pb = Path[From];
					const FVector Ob = FVector(Pb.X, Pb.Y, 0.0).GetSafeNormal();
					const FVector Side(-Ob.Y, Ob.X, 0.0);
					const FVector End2 = FVector(Pb.X, Pb.Y, 0.0) + Ob * Rng.FRandRange(0.4, 1.0) + Side * Rng.FRandRange(-0.5, 0.5) + FVector(0, 0, BottomZ - 0.25);
					const FVector C1 = Pb + Ob * 0.3 + FVector(0, 0, 0.1);
					const FVector C2 = End2 + FVector(0, 0, 0.8 + 0.3 * (Pb.Z - BottomZ));
					TArray<FVector> Fork;
					TArray<double> ForkRad;
					for (int32 k = 0; k <= 16; ++k)
					{
						const double T = k / 16.0;
						Fork.Add(Bezier(Pb, C1, C2, End2, T));
						ForkRad.Add(0.7 * R0 * (1.0 - 0.15 * T));
					}
					Tube(Wood, Fork, ForkRad, 8, false, true, 0.0, WoodPaint);
				}
			}
			// Boughs from the trunk's top up into the canopy.
			for (int32 b = 0; b < 5; ++b)
			{
				const double Az = Rng.FRandRange(0.0, TwoPi), Tilt = FMath::DegreesToRadians(Rng.FRandRange(40.0, 60.0));
				const FVector Dir(FMath::Sin(Tilt) * FMath::Cos(Az), FMath::Sin(Tilt) * FMath::Sin(Az), FMath::Cos(Tilt));
				const FVector Start = TrunkAt(Rng.FRandRange(4.0, 5.3));
				const double Len = Rng.FRandRange(1.5, 2.4);
				TArray<FVector> Path;
				TArray<double> Rad;
				for (int32 k = 0; k <= 10; ++k)
				{
					const double T = k / 10.0;
					Path.Add(Start + Dir * (Len * T) + FVector(0, 0, 0.3 * T * T));
					Rad.Add(0.07 - 0.035 * T);
				}
				Tube(Wood, Path, Rad, 10, false, true, 0.0, WoodPaint);
			}
			// The canopy: a lumpy mass of crowns (a cube-sphere, flattened below).
			const FVector Centre = TrunkAt(6.4);
			const FVector Radii(Rng.FRandRange(2.9, 3.4), Rng.FRandRange(2.6, 3.1), 1.5);
			const FVector Axes[6][3] = {{FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1)}, {FVector(-1, 0, 0), FVector(0, 0, 1), FVector(0, 1, 0)},
										{FVector(0, 1, 0), FVector(0, 0, 1), FVector(1, 0, 0)}, {FVector(0, -1, 0), FVector(1, 0, 0), FVector(0, 0, 1)},
										{FVector(0, 0, 1), FVector(1, 0, 0), FVector(0, 1, 0)}, {FVector(0, 0, -1), FVector(0, 1, 0), FVector(1, 0, 0)}};
			constexpr int32 N = 36;
			for (int32 f = 0; f < 6; ++f)
			{
				Sheet(Crown, N, N, [&](int32 I, int32 J, FVector& OutP, FVector& OutN, FLinearColor& OutC, FVector2D& OutUv)
				{
					const FVector Dir = (Axes[f][0] + Axes[f][1] * (-1.0 + 2.0 * I / N) + Axes[f][2] * (-1.0 + 2.0 * J / N)).GetSafeNormal();
					const double Rr = 1.0 + 0.16 * Fbm(Dir, 1.6, 3, Sd);
					FVector Pt = Centre + FVector(Dir.X * Radii.X, Dir.Y * Radii.Y, Dir.Z * (Dir.Z < 0.0 ? 0.7 : 1.0) * Radii.Z) * Rr;
					Pt += Dir * (0.45 * Sat(0.5 + Noise(Pt.X * 0.9, Pt.Y * 0.9, Pt.Z * 0.9 + Sd)));
					OutP = Pt;
					OutN = Dir;
					OutC = Paint(Pt, 1.0);
					OutUv = FVector2D(Pt.X, Pt.Y);
				});
			}
			Smooth(Wood, 0.01);
			Smooth(Crown, 0.01);
		}
	}

	// ================================================================================================ F. the karst islets

	/**
	 * A limestone islet of Raja Ampat (Piaynemo, Wayag): SizeX × SizeY across and H tall, steep-sided and mushroom-shaped
	 * — the sea has cut a notch 1–2.5 m deep into its foot at the tide line (from under the water to 2.2 m above it,
	 * its roof overhanging), above that sheer grey cliffs fluted by rain and bulging a little, and a rounded top under
	 * jungle whose crowns make the surface lumpy. Its origin at the water line under its centre; it reaches 3 m under the
	 * water. A lathe of the plan's outline (an irregular ellipse): rows up the cliff, then over the dome.
	 * Vertex colour: R = height fraction (-3 m … the top); G = vegetated (the upward-facing upper surfaces, fading on
	 * steep faces and near the water); B = 1 in the notch; A = 0. UV0 = (round the islet, up the surface) in metres.
	 */
	FMesh Karst(double SizeX, double SizeY, double H, int32 Seed)
	{
		FMesh M;
		const double Ax = 0.5 * SizeX, Ay = 0.5 * SizeY, Rm = 0.5 * (Ax + Ay);
		const double Hs = 0.62 * H;   // the cliff's top, where it rounds over into the dome
		const double NotchDepth = FMath::Clamp(0.9 + 0.013 * (Ax + Ay), 1.0, 2.5);
		const double Sd = Seed * 1.37;
		auto PlanR = [&](double Phi)
		{
			const double C = FMath::Cos(Phi), S = FMath::Sin(Phi);
			const double Ell = 1.0 / FMath::Sqrt(FMath::Square(C / Ax) + FMath::Square(S / Ay));
			return Ell * (1.0 + 0.12 * Noise(C * 1.5, S * 1.5, Sd) + 0.05 * Noise(C * 5.0, S * 5.0, Sd + 2.0));
		};
		// The notch's profile (0 … 1): from 0.8 m under the water up to 2.2 m above it, deepest a little above the
		// water line, its roof steeper than its floor.
		auto NotchShape = [](double Z)
		{
			const double Q = Sat((Z + 0.8) / 3.0);
			return FMath::Pow(FMath::Max(FMath::Sin(Pi * FMath::Pow(Q, 0.7)), 0.0), 0.8);
		};
		// The cliff's radius at angle Phi and height Z: the plan, a slight bulge, the flared foot, the notch, the flutes.
		auto CliffR = [&](double Phi, double Z, double& OutNotch)
		{
			const double C = FMath::Cos(Phi), S = FMath::Sin(Phi);
			const double Zf = Sat(Z / Hs);
			double Rho = PlanR(Phi) * (1.0 + 0.04 * FMath::Sin(Pi * Zf) - 0.03 * Zf);
			Rho += 0.6 * Ramp(-0.8, -3.0, Z);
			OutNotch = NotchShape(Z);
			Rho -= NotchDepth * (0.75 + 0.35 * Noise(C * 2.2, S * 2.2, Sd + 4.0)) * OutNotch;
			// Vertical flutes (about 0.3 a metre round, stretched up the face), buttresses and hollows, a finer grain.
			Rho += 0.9 * (Ridged(FVector(C * Rm * 0.3, S * Rm * 0.3, Z * 0.035), 1.0, 3, Sd) - 0.55)
				 + 2.0 * Fbm(FVector(C * Rm * 0.06, S * Rm * 0.06, Z * 0.03), 1.0, 2, Sd + 3.0)
				 + 0.25 * Fbm(FVector(C * Rm * 0.8, S * Rm * 0.8, Z * 0.8), 1.0, 2, Sd + 5.0);
			return Rho;
		};
		// The rows: (0, z) up the cliff, closer through the notch; then (1, psi) over the dome to the top.
		TArray<FVector2D> Rows;
		for (double Z = -3.0; Z < -1.0 - 1e-6; Z += 0.5) { Rows.Add(FVector2D(0.0, Z)); }
		for (double Z = -1.0; Z < 2.6 - 1e-6; Z += 0.15) { Rows.Add(FVector2D(0.0, Z)); }
		{
			double Z = 2.6, Step = 0.2;
			const double MaxStep = 0.012 * H + 0.2;
			while (Z < Hs - 0.5 * Step)
			{
				Rows.Add(FVector2D(0.0, Z));
				Z += Step;
				Step = FMath::Min(Step * 1.08, MaxStep);
			}
		}
		Rows.Add(FVector2D(0.0, Hs));
		const int32 NDome = 30 + FMath::RoundToInt32(0.4 * H);
		for (int32 k = 1; k <= NDome; ++k) { Rows.Add(FVector2D(1.0, 0.5 * Pi * k / NDome)); }
		const int32 NV = Rows.Num() - 1;
		const int32 NA = FMath::Clamp(FMath::RoundToInt32(TwoPi * Rm / 0.6), 192, 512);
		const int32 Stride = NA + 1;

		// The rock.
		TArray<FVector> Rock;
		TArray<double> NotchW;
		Rock.SetNum((NV + 1) * Stride);
		NotchW.SetNum((NV + 1) * Stride);
		for (int32 j = 0; j <= NV; ++j)
		{
			for (int32 i = 0; i < NA; ++i)
			{
				const double Phi = TwoPi * i / NA;
				const double C = FMath::Cos(Phi), S = FMath::Sin(Phi);
				double Nw = 0;
				FVector Pt = FVector::ZeroVector;
				if (Rows[j].X < 0.5)
				{
					const double Z = Rows[j].Y;
					const double Rho = CliffR(Phi, Z, Nw);
					Pt = FVector(Rho * C, Rho * S, Z);
				}
				else
				{
					const double Psi = Rows[j].Y;
					double Unused = 0;
					const double Cp = FMath::Max(FMath::Cos(Psi), 0.0);
					const double Rho = CliffR(Phi, Hs, Unused) * FMath::Pow(Cp, 0.9);
					const double Z = Hs + (H - Hs) * FMath::Pow(FMath::Sin(Psi), 0.7) + 0.03 * H * Cp * Fbm(FVector(C * Rm * 0.05, S * Rm * 0.05, Psi), 1.0, 2, Sd + 8.0);
					Pt = FVector(Rho * C, Rho * S, Z);
				}
				Rock[j * Stride + i] = Pt;
				NotchW[j * Stride + i] = Nw;
			}
			Rock[j * Stride + NA] = Rock[j * Stride];   // the seam: the same point exactly
			NotchW[j * Stride + NA] = NotchW[j * Stride];
		}
		// Facing normals (across the grid), the jungle where the surface faces up high enough, its crowns pushed out.
		TArray<FVector> Final, Facing;
		TArray<double> Green;
		Final.SetNum(Rock.Num());
		Facing.SetNum(Rock.Num());
		Green.SetNum(Rock.Num());
		for (int32 j = 0; j <= NV; ++j)
		{
			for (int32 i = 0; i <= NA; ++i)
			{
				const int32 Ic = i % NA, Il = (Ic + NA - 1) % NA, Ir = (Ic + 1) % NA;
				const FVector Du = Rock[j * Stride + Ir] - Rock[j * Stride + Il];
				const FVector Dv = Rock[FMath::Min(j + 1, NV) * Stride + Ic] - Rock[FMath::Max(j - 1, 0) * Stride + Ic];
				FVector Nrm = FVector::CrossProduct(Du, Dv).GetSafeNormal();
				if (Nrm.IsNearlyZero() || j == NV) { Nrm = FVector(0, 0, 1); }
				const FVector& Pt = Rock[j * Stride + Ic];
				// (the jungle clings to all but the sheerest faces, down to a few metres over the notch: Piaynemo's islets are green)
				const double G = Ramp(-0.45, 0.25, Nrm.Z) * Ramp(3.0, 6.5, Pt.Z);
				const double Crowns = G * (1.0 + 2.0 * Sat(0.55 + 0.8 * Noise(Pt.X * 0.16 + Sd, Pt.Y * 0.16, Pt.Z * 0.16)) + 0.6 * Noise(Pt.X * 0.4, Pt.Y * 0.4 + Sd, Pt.Z * 0.4));
				Final[j * Stride + i] = Pt + Nrm * Crowns;
				Facing[j * Stride + i] = Nrm;
				Green[j * Stride + i] = G;
			}
		}
		// The vertices: UV0's v the arc length up each column.
		TArray<double> Arc;
		Arc.SetNumZeroed(Stride);
		for (int32 j = 0; j <= NV; ++j)
		{
			for (int32 i = 0; i <= NA; ++i)
			{
				const int32 Id = j * Stride + i;
				if (j > 0) { Arc[i] += (Final[Id] - Final[Id - Stride]).Size(); }
				const FVector& Pt = Final[Id];
				M.V(Pt, Facing[Id], FVector2D(TwoPi * i / NA * Rm, Arc[i]), Rgba(Sat((Pt.Z + 3.0) / (H + 3.0)), Green[Id], NotchW[Id], 0.0));
			}
		}
		for (int32 j = 0; j < NV; ++j)
		{
			for (int32 i = 0; i < NA; ++i)
			{
				const int32 A = j * Stride + i;
				M.Quad(A, A + 1, A + Stride + 1, A + Stride);
			}
		}
		// The underside, 3 m under the water (closed, for shadows and the view from below).
		{
			const int32 Centre = M.V(FVector(0, 0, -3.0), FVector(0, 0, -1), FVector2D::ZeroVector, Rgba(0, 0, 0, 0));
			for (int32 i = 0; i < NA; ++i) { M.Tri(Centre, i, i + 1); }
		}
		Smooth(M, 0.01);
		return M;
	}

	// ================================================================================================ G. the seabeds

	/**
	 * The six places' bottoms, in each place's frame (the eyes at the origin, x east, y south, z up). BedHeight, WallX and
	 * BedMaterial (declared in the header) are the single source of truth: the game places things with them and the
	 * meshes are built from them. The material weights (sand, coral rubble, reef rock, algae/encrustation) sum to 1 and
	 * go into the vertex colours (R, G, B, A) for the material to blend its photographed sets with.
	 */
	namespace Beds
	{
		/** Coral heads: a field of rounded bumps (0 … 1) where the noise rises over a threshold. */
		double Heads(double X, double Y, double Freq, double Seed)
		{
			return FMath::Pow(Sat((Fbm(FVector(X, Y, 0.0), Freq, 3, Seed) - 0.12) * 3.2), 0.6);
		}

		/** Weights (sand, rubble, rock, algae), none negative, summing to 1. */
		FVector4 Mix(double Sand, double Rubble, double Rock, double Algae)
		{
			const double Sa = FMath::Max(Sand, 0.0), Ru = FMath::Max(Rubble, 0.0), Ro = FMath::Max(Rock, 0.0), Al = FMath::Max(Algae, 0.0);
			const double Sum = Sa + Ru + Ro + Al;
			if (Sum <= 1e-9) { return FVector4(1.0, 0.0, 0.0, 0.0); }
			return FVector4(Sa / Sum, Ru / Sum, Ro / Sum, Al / Sum);
		}

		/** Fine relief only near the eyes: beyond 15–30 m the bed's triangles grow too big to carry it. */
		double Near(double X, double Y) { return Ramp(30.0, 15.0, FMath::Sqrt(X * X + Y * Y)); }

		/**
		 * 1. Piaynemo, the surface (the eyes at the water line): a reef flat 3.5–5 m down round the car with coral heads
		 * on it; to the south it slopes away to 12 m at 60 m and on down; to the north it shoals to 1.5 m towards the islet
		 * 120 m away.
		 */
		double Piaynemo(double X, double Y)
		{
			const FVector At(X, Y, 0.0);
			double Z = -4.2 + 0.5 * Fbm(At, 0.04, 3, 1.0);
			const double South = Ramp(4.0, 60.0, Y);
			Z = FMath::Lerp(Z, -12.0 + 0.8 * Fbm(At, 0.03, 2, 2.0), South) - 0.05 * FMath::Max(Y - 60.0, 0.0);
			const double North = Ramp(-6.0, -100.0, Y);
			Z = FMath::Lerp(Z, -1.5 + 0.25 * Fbm(At, 0.05, 2, 3.0), North);
			const double Hd = Heads(X, Y, 0.11, 11.0);
			Z += Hd * (0.7 + 0.5 * Fbm(At, 0.3, 2, 4.0)) * (1.0 - 0.6 * Ramp(20.0, 70.0, Y)) * (1.0 - 0.5 * North);
			Z += 0.07 * Fbm(At, 0.9, 2, 5.0) * Near(X, Y);
			return FMath::Min(Z, -1.0);
		}

		FVector4 PiaynemoMat(double X, double Y)
		{
			const FVector At(X, Y, 0.0);
			const double Hd = Heads(X, Y, 0.11, 11.0);
			const double South = Ramp(4.0, 60.0, Y), North = Ramp(-6.0, -100.0, Y);
			const double Patchy = Fbm(At, 0.15, 2, 6.0);
			const double Rock = (0.35 + 0.45 * Ramp(-0.1, 0.25, Patchy)) * (1.0 - 0.6 * South) + 0.9 * Hd;
			const double Rubble = (0.3 + 0.3 * Ramp(0.0, 0.3, -Patchy)) * (1.0 - Hd);
			const double Sand = (0.15 + 0.7 * South + 0.3 * Ramp(0.05, 0.3, Fbm(At, 0.07, 2, 7.0))) * (1.0 - Hd);
			const double Algae = (0.1 + 0.35 * North) * (0.7 + 0.6 * Fbm(At, 0.2, 2, 8.0));
			return Mix(Sand, Rubble, Rock, Algae);
		}

		/** 2. The mangroves: silty sand with rubble 2.6–3.2 m under the eyes (which are 1.2 m under the surface). */
		double Mangroves(double X, double Y)
		{
			const FVector At(X, Y, 0.0);
			const double R = FMath::Sqrt(X * X + Y * Y);
			double Z = -2.9 + 0.22 * Fbm(At, 0.06, 3, 21.0) + 0.04 * Fbm(At, 0.7, 2, 22.0) * Near(X, Y);
			Z += 0.18 * Heads(X, Y, 0.35, 24.0);
			Z -= 1.5 * Ramp(40.0, 150.0, R);   // far off, the channel between the mangrove islands deepens a little
			return Z;
		}

		FVector4 MangroveMat(double X, double Y)
		{
			const FVector At(X, Y, 0.0);
			const double Hd = Heads(X, Y, 0.35, 24.0);
			return Mix(0.75 + 0.2 * Fbm(At, 0.1, 2, 25.0), 0.15 + 0.6 * Hd, 0.05 + 0.2 * Hd, 0.2 + 0.25 * Ramp(-0.1, 0.3, Fbm(At, 0.12, 2, 26.0)));
		}

		/**
		 * 3. Cape Kri: the eyes at 12 m on a reef slope that rises to 5 m depth 25 m to the north (its surface 6–8 m from
		 * the car on that side) and falls to 25–30 m depth 40 m to the south; spurs and grooves run down it, coral heads
		 * stand on it; mostly reef rock.
		 */
		double CapeKri(double X, double Y)
		{
			const FVector2D Slope[] = {{-300.0, 11.0}, {-150.0, 10.5}, {-60.0, 9.5}, {-25.0, 7.0}, {-7.0, 0.0}, {0.0, -2.5},
									   {20.0, -9.5}, {40.0, -15.5}, {80.0, -24.0}, {150.0, -36.0}, {300.0, -52.0}};
			const FVector At(X, Y, 0.0);
			double Z = Curve(Slope, int32(UE_ARRAY_COUNT(Slope)), Y);
			Z += 0.9 * (Ridged(FVector(X * 0.09, Y * 0.025, 0.0), 1.0, 2, 31.0) - 0.55);
			Z += 0.9 * Heads(X, Y, 0.14, 33.0);
			Z += 0.3 * Fbm(At, 0.05, 2, 34.0);
			Z += 0.1 * Fbm(At, 0.8, 2, 35.0) * Near(X, Y);
			return FMath::Min(Z, 11.5);   // the reef top, half a metre under the surface
		}

		FVector4 CapeKriMat(double X, double Y)
		{
			const double Groove = 1.0 - Ridged(FVector(X * 0.09, Y * 0.025, 0.0), 1.0, 2, 31.0);
			const double Hd = Heads(X, Y, 0.14, 33.0);
			const double Deep = Ramp(15.0, 60.0, Y);
			const double Rock = 0.55 + 0.4 * Hd - 0.3 * Deep;
			const double Rubble = 0.2 + 0.4 * Ramp(0.4, 0.8, Groove);
			const double Sand = 0.05 + 0.5 * Ramp(0.55, 0.9, Groove) * (0.4 + Deep);
			const double Algae = 0.12 + 0.2 * Ramp(-10.0, -40.0, Y);
			return Mix(Sand, Rubble, Rock, Algae);
		}

		/** 4. Manta Sandy's cleaning station: a coral mound 5 m across, 8 m north of the car. How much of it is here (0 … 1). */
		double BommieQ(double X, double Y)
		{
			const double Dx = X, Dy = Y + 8.0;
			const double D = FMath::Sqrt(Dx * Dx + Dy * Dy);
			const double A = FMath::Atan2(Dy, Dx);
			const double Rb = 2.5 * (1.0 + 0.15 * Noise(FMath::Cos(A) * 1.5, FMath::Sin(A) * 1.5, 44.0));
			return Sat(1.0 - FMath::Square(D / Rb));
		}

		/** 4. Manta Sandy: a sandy bottom 3 m under the eyes (18 m deep), gently rippled; the bommie rising to 1 m under the eyes. */
		double MantaSandy(double X, double Y)
		{
			const FVector At(X, Y, 0.0);
			const double R = FMath::Sqrt(X * X + Y * Y);
			double Z = -3.0 + 0.3 * Fbm(At, 0.015, 3, 41.0) - 0.01 * FMath::Max(R - 40.0, 0.0);
			// Ripples: crests across the current, 0.7 m apart and 3–4 cm high (near the car only: further out the grid is too coarse).
			const double Across = X * 0.34 + Y * 0.94;
			Z += 0.035 * FMath::Sin(TwoPi * Across / 0.7 + 2.0 * Noise(X * 0.08, Y * 0.08, 42.0)) * (0.7 + 0.3 * Noise(X * 0.05, Y * 0.05, 43.0)) * Ramp(18.0, 10.0, R);
			// A few coral heads far out on the sand.
			Z += 0.8 * Heads(X, Y, 0.04, 46.0) * Ramp(15.0, 30.0, R);
			// The bommie.
			const double Mound = -3.2 + (2.2 + 0.3 * Fbm(At, 0.9, 2, 45.0)) * FMath::Pow(BommieQ(X, Y), 0.55);
			return FMath::Max(Z, Mound);
		}

		FVector4 MantaSandyMat(double X, double Y)
		{
			const double R = FMath::Sqrt(X * X + Y * Y);
			const double On = Ramp(0.05, 0.3, BommieQ(X, Y));
			const double Apron = Ramp(4.5, 2.5, FMath::Sqrt(X * X + FMath::Square(Y + 8.0))) * (1.0 - On);
			const double Hd = Heads(X, Y, 0.04, 46.0) * Ramp(15.0, 30.0, R);
			return Mix(0.92 * (1.0 - On) * (1.0 - 0.6 * Apron) * (1.0 - Hd), 0.05 + 0.6 * Apron + 0.2 * On, 0.5 * On + 0.6 * Hd, 0.03 + 0.3 * On + 0.2 * Hd);
		}

		/** 6. Night, the surface (the eyes at the water line): a reef flat 1.5–2.5 m down, with coral heads; deeper far off. */
		double Night(double X, double Y)
		{
			const FVector At(X, Y, 0.0);
			const double R = FMath::Sqrt(X * X + Y * Y);
			double Z = -2.0 + 0.35 * Fbm(At, 0.05, 3, 61.0) - 3.0 * Ramp(60.0, 200.0, R);
			Z += 0.6 * Heads(X, Y, 0.16, 62.0);
			Z += 0.06 * Fbm(At, 0.9, 2, 63.0) * Near(X, Y);
			return FMath::Min(Z, -1.2);
		}

		FVector4 NightMat(double X, double Y)
		{
			const double Hd = Heads(X, Y, 0.16, 62.0);
			const double Patchy = Fbm(FVector(X, Y, 0.0), 0.12, 2, 64.0);
			return Mix(0.25 + 0.4 * Ramp(0.0, 0.3, Patchy), 0.35 + 0.2 * Ramp(0.0, 0.3, -Patchy), 0.35 + 0.8 * Hd, 0.2);
		}

		/** 5. The Wall's face: ledge tops (facing up) catch sand and rubble; the face is rock thick with encrusting life; cracks are bare rock. */
		FVector4 WallMat(double Y, double Z)
		{
			constexpr double E = 0.15;
			const double DxDy = (WallX(Y + E, Z) - WallX(Y - E, Z)) / (2.0 * E);
			const double DxDz = (WallX(Y, Z + E) - WallX(Y, Z - E)) / (2.0 * E);
			const double Up = FVector(1.0, -DxDy, -DxDz).GetSafeNormal().Z;
			const double Crack = Ramp(0.8, 0.98, Ridged(FVector(Y * 0.11, Z * 0.012, 0.0), 1.0, 2, 58.0));
			const double Life = 0.55 + 0.25 * Fbm(FVector(Y, Z, 3.0), 0.3, 2, 65.0);
			const double Shelf = Ramp(0.35, 0.75, Up);
			return Mix(0.8 * Shelf, 0.4 * Ramp(0.2, 0.5, Up), 0.35 + 0.4 * Crack, Life * (1.0 - 0.6 * Shelf) * (1.0 - 0.5 * Crack));
		}

		/**
		 * A bed's mesh: a height field on polar rings round the eyes, 0.2 m apart within 15 m (each ring with a power of
		 * two of points, up to 512, keeping them about 0.2 m apart; rings that double are stitched), then 512 round with
		 * the rings spaced to keep the cells near square, out to 250 m (cells about 3.8 × 3 m there).
		 */
		FMesh Bed(int32 Where)
		{
			FMesh M;
			auto Emit = [&M, Where](double X, double Y)
			{
				const double Z = BedHeight(Where, X, Y);
				const FVector4 W = BedMaterial(Where, X, Y);
				return M.V(FVector(X, Y, Z), FVector(0, 0, 1), FVector2D(X, Y), Rgba(W.X, W.Y, W.Z, W.W));
			};
			TArray<int32> Starts, Counts;
			auto AddRing = [&](double Radius, int32 Num)
			{
				Starts.Add(M.Positions.Num());
				Counts.Add(Num);
				for (int32 j = 0; j < Num; ++j)
				{
					const double A = TwoPi * j / Num;
					Emit(Radius * FMath::Cos(A), Radius * FMath::Sin(A));
				}
			};
			const int32 Centre = Emit(0.0, 0.0);
			for (int32 k = 1; k <= 75; ++k)
			{
				const double Ring = 0.2 * k;
				int32 Num = 8;
				while (Num < 512 && Num < TwoPi * Ring / 0.2) { Num *= 2; }
				AddRing(Ring, Num);
			}
			double Far = 15.0;
			while (Far < 250.0)
			{
				Far = FMath::Min(Far + 1.25 * TwoPi * Far / 512.0, 250.0);
				AddRing(Far, 512);
			}
			for (int32 j = 0; j < Counts[0]; ++j) { M.Tri(Centre, Starts[0] + j, Starts[0] + (j + 1) % Counts[0]); }
			for (int32 r = 0; r + 1 < Starts.Num(); ++r)
			{
				const int32 A = Starts[r], NA = Counts[r], B = Starts[r + 1], NB = Counts[r + 1];
				if (NB == NA)
				{
					for (int32 j = 0; j < NA; ++j) { M.Quad(A + j, A + (j + 1) % NA, B + (j + 1) % NB, B + j); }
				}
				else   // the outer ring has twice the points
				{
					for (int32 j = 0; j < NA; ++j)
					{
						const int32 O = 2 * j;
						M.Tri(A + j, B + O, B + O + 1);
						M.Tri(A + j, B + O + 1, A + (j + 1) % NA);
						M.Tri(A + (j + 1) % NA, B + O + 1, B + (O + 2) % NB);
					}
				}
			}
			Smooth(M, 0.0);
			return M;
		}

		/**
		 * 5. The Wall: a vertical height field (x as a function of y and z) on the car's west side, from 12 m over the eyes
		 * to 60 m under them and 250 m either way; 0.2 m cells near the car, growing away from it. Over its top the reef
		 * flat rolls back 30 m. UV0 = (y, z) metres.
		 */
		FMesh Wall()
		{
			FMesh M;
			TArray<double> Half;   // y from 0.2 m out to 250 m
			for (int32 k = 1; k <= 60; ++k) { Half.Add(0.2 * k); }
			while (Half.Last() < 250.0) { Half.Add(FMath::Min(Half.Last() * 1.02, 250.0)); }
			TArray<double> Ys;
			for (int32 k = Half.Num() - 1; k >= 0; --k) { Ys.Add(-Half[k]); }
			Ys.Add(0.0);
			Ys.Append(Half);
			TArray<double> Zs;
			{
				TArray<double> Deep;
				double Z = -12.0, Step = 0.2;
				while (Z > -60.0)
				{
					Z = FMath::Max(Z - Step, -60.0);
					Deep.Add(Z);
					Step = 0.2 + 0.02 * (-Z - 12.0);
				}
				for (int32 k = Deep.Num() - 1; k >= 0; --k) { Zs.Add(Deep[k]); }
				for (int32 k = 0; k <= 120; ++k) { Zs.Add(-12.0 + 0.2 * k); }
			}
			const double Lip[6] = {0.4, 1.2, 3.0, 7.0, 15.0, 30.0};
			const int32 NZ = Zs.Num();
			Sheet(M, Ys.Num() - 1, NZ - 1 + 6, [&](int32 I, int32 J, FVector& OutP, FVector& OutN, FLinearColor& OutC, FVector2D& OutUv)
			{
				const double Y = Ys[I];
				if (J < NZ)
				{
					const double Z = Zs[J];
					OutP = FVector(WallX(Y, Z), Y, Z);
					OutN = FVector(1, 0, 0);
					const FVector4 W = WallMat(Y, Z);
					OutC = Rgba(W.X, W.Y, W.Z, W.W);
					OutUv = FVector2D(Y, Z);
				}
				else
				{
					const double D = Lip[J - NZ];
					const double Roll = Ramp(0.0, 3.0, D);
					OutP = FVector(WallX(Y, 12.0) - D, Y, 12.0 + 0.35 * Roll + 0.3 * Roll * Fbm(FVector(Y, D, 0.0), 0.2, 2, 66.0));
					OutN = FVector(0, 0, 1);
					const FVector4 W = Mix(0.1, 0.3, 0.45, 0.35 + 0.2 * Fbm(FVector(Y, D, 1.0), 0.3, 2, 67.0));
					OutC = Rgba(W.X, W.Y, W.Z, W.W);
					OutUv = FVector2D(Y, 12.0 + D);
				}
			});
			Smooth(M, 0.0);
			return M;
		}
	}

	double WallX(double Y, double Z)
	{
		const double Ay = FMath::Abs(Y);
		const double Far = Ramp(6.0, 40.0, Ay);
		// The face 5 m from the car's axis, leaning back a little with depth; away from the car it wanders in and out,
		// and everywhere it bulges and hollows.
		double X = -5.0 - 0.015 * FMath::Max(-Z, 0.0)
				 + Far * (2.0 * Noise(Y * 0.018, 0.5, 51.0) + 0.8 * Noise(Y * 0.05, 1.5, 52.0))
				 + 0.8 * Fbm(FVector(Y, Z, 0.0), 0.06, 3, 53.0);
		// Ledges every ten metres or so: flat tops facing up, overhanging undersides; they come and go along the wall.
		for (int32 k = 0; k < 7; ++k)
		{
			const double Zk = 10.0 - 10.5 * k + 1.5 * Noise(Y * 0.02, k * 1.7, 54.0);
			const double There = Sat(0.45 + 0.9 * Noise(Y * 0.04, k * 3.1, 55.0));
			const double Proud = (0.6 + 0.9 * Sat(Noise(Y * 0.03, k * 2.3, 56.0) + 0.5)) * There;
			const double Dz = Z - Zk;
			X += Proud * (Dz > 0.0 ? Ramp(0.35, 0.0, Dz) : Ramp(-1.6, -0.2, Dz));
		}
		// A great overhang a few metres over the eyes: a roof 2 m proud, its underside sloping in.
		X += 2.0 * Ramp(3.0, 5.5, Z) * Ramp(8.5, 7.5, Z) * Sat(Noise(Y * 0.015, 7.7, 57.0) + 0.6);
		// Crevices: narrow, deep cracks running down the face.
		X -= 1.4 * Ramp(0.86, 0.98, Ridged(FVector(Y * 0.11, Z * 0.012, 0.0), 1.0, 2, 58.0));
		// Pocks and knobs (sponges, corals, their bases).
		X += 0.22 * Fbm(FVector(Y, Z, 1.0), 0.5, 3, 59.0) + 0.08 * Fbm(FVector(Y, Z, 2.0), 2.0, 2, 60.0) * Ramp(30.0, 15.0, Ay);
		// Beside the car the face keeps its distance: 4.3 m at the least.
		const double Keep = Ramp(12.0, 6.0, Ay) * Ramp(7.0, 3.0, FMath::Abs(Z));
		return FMath::Lerp(X, FMath::Min(X, -4.3), Keep);
	}

	double BedHeight(int32 Where, double X, double Y)
	{
		switch (Where)
		{
		case 1: return Beds::Piaynemo(X, Y);
		case 2: return Beds::Mangroves(X, Y);
		case 3: return Beds::CapeKri(X, Y);
		case 4: return Beds::MantaSandy(X, Y);
		case 5: return -200.0;   // the Wall: no bottom in sight
		case 6: return Beds::Night(X, Y);
		default: return -3.0;
		}
	}

	FVector4 BedMaterial(int32 Where, double X, double Y)
	{
		switch (Where)
		{
		case 1: return Beds::PiaynemoMat(X, Y);
		case 2: return Beds::MangroveMat(X, Y);
		case 3: return Beds::CapeKriMat(X, Y);
		case 4: return Beds::MantaSandyMat(X, Y);
		case 5: return Beds::WallMat(X, Y);   // (X, Y) are the wall's (y, z)
		case 6: return Beds::NightMat(X, Y);
		default: return FVector4(1.0, 0.0, 0.0, 0.0);
		}
	}

	// ================================================================================================ everything

	void Build(TArray<FJourneyThing>& Out)
	{
		auto Put = [&Out](const FString& Name, const FString& Material, FMesh& Mesh)
		{
			FJourneyThing T;
			T.Name = Name;
			T.Material = Material;
			T.Mesh = MoveTemp(Mesh);
			Out.Add(MoveTemp(T));
		};

		// A. The fishes.
		{
			TArray<Fish::FSpecies> Species;
			Fish::AllSpecies(Species);
			for (const Fish::FSpecies& Sp : Species)
			{
				FMesh M = Fish::Make(Sp);
				Put(TEXT("SM_RA_Fish_") + Sp.Suffix, TEXT("MI_RA_Fish_") + Sp.Suffix, M);
				Out.Last().bNanite = false;   // they swim by their material's offset: plain instanced meshes
			}
		}

		// B. The manta.
		{
			FMesh M = Manta::Make();
			Put(TEXT("SM_RA_Manta"), TEXT("MI_RA_Manta"), M);
			Out.Last().bNanite = false;
		}

		// C. Corals and sponges.
		{
			const double Tables[3] = {0.8, 1.4, 2.0};
			for (int32 k = 0; k < 3; ++k)
			{
				FMesh M = Coral::Table(Tables[k], 100 + k);
				Put(FString::Printf(TEXT("SM_RA_Coral_Table_%d"), k), TEXT("MI_RA_Coral_Table"), M);
			}
			const double StagW[3] = {1.0, 1.5, 2.0}, StagH[3] = {0.5, 0.7, 0.9};
			for (int32 k = 0; k < 3; ++k)
			{
				FMesh M = Coral::Staghorn(StagW[k], StagH[k], 200 + k);
				Put(FString::Printf(TEXT("SM_RA_Coral_Staghorn_%d"), k), TEXT("MI_RA_Coral_Staghorn"), M);
			}
			const double Domes[3] = {0.6, 1.2, 2.5};
			for (int32 k = 0; k < 3; ++k)
			{
				FMesh M = Coral::Massive(Domes[k], 300 + k);
				Put(FString::Printf(TEXT("SM_RA_Coral_Massive_%d"), k), TEXT("MI_RA_Coral_Massive"), M);
			}
			const double Softs[4] = {0.25, 0.35, 0.45, 0.6};
			for (int32 k = 0; k < 4; ++k)
			{
				FMesh M = Coral::SoftCoral(Softs[k], 400 + k);
				Put(FString::Printf(TEXT("SM_RA_SoftCoral_%d"), k), TEXT("MI_RA_SoftCoral"), M);
			}
			const double Fans[3] = {1.2, 2.0, 2.8};
			for (int32 k = 0; k < 3; ++k)
			{
				FMesh M = Coral::SeaFan(Fans[k], 500 + k);
				Put(FString::Printf(TEXT("SM_RA_SeaFan_%d"), k), TEXT("MI_RA_SeaFan"), M);
			}
			const double Barrels[2] = {0.8, 1.4};
			for (int32 k = 0; k < 2; ++k)
			{
				FMesh M = Coral::BarrelSponge(Barrels[k], 600 + k);
				Put(FString::Printf(TEXT("SM_RA_BarrelSponge_%d"), k), TEXT("MI_RA_Sponge"), M);
			}
			const double Leathers[2] = {0.4, 0.7};
			for (int32 k = 0; k < 2; ++k)
			{
				FMesh M = Coral::LeatherCoral(Leathers[k], 700 + k);
				Put(FString::Printf(TEXT("SM_RA_LeatherCoral_%d"), k), TEXT("MI_RA_SoftCoral"), M);
			}
			const double Rocks[4] = {1.0, 1.8, 2.8, 4.0};
			for (int32 k = 0; k < 4; ++k)
			{
				FMesh M = Coral::Rock(Rocks[k], 800 + k);
				Put(FString::Printf(TEXT("SM_RA_Rock_%d"), k), TEXT("MI_RA_ReefRock"), M);
			}
		}

		// D. The pygmy seahorse.
		{
			FMesh M = PygmySeahorse();
			Put(TEXT("SM_RA_PygmySeahorse"), TEXT("MI_RA_PygmySeahorse"), M);
		}

		// E. The mangroves: the wood and the canopy in two sections.
		for (int32 k = 0; k < 2; ++k)
		{
			FJourneyThing T;
			T.Name = FString::Printf(TEXT("SM_RA_Mangrove_%d"), k);
			T.Sections.SetNum(2);
			Mangrove::Make(T.Sections[0], T.Sections[1], 900 + k);
			T.Materials = {TEXT("MI_RA_Mangrove"), TEXT("MI_RA_Canopy")};
			Out.Add(MoveTemp(T));
		}

		// F. The karst islets (length × width × height, metres).
		{
			const FVector Isles[4] = {FVector(40, 30, 35), FVector(80, 60, 55), FVector(25, 20, 22), FVector(150, 90, 70)};
			for (int32 k = 0; k < 4; ++k)
			{
				FMesh M = Karst(Isles[k].X, Isles[k].Y, Isles[k].Z, 1000 + k);
				Put(FString::Printf(TEXT("SM_RA_Karst_%d"), k), TEXT("MI_RA_Karst"), M);
			}
		}

		// G. The six places' beds.
		for (int32 Where = 1; Where <= 6; ++Where)
		{
			FMesh M = Where == 5 ? Beds::Wall() : Beds::Bed(Where);
			Put(FString::Printf(TEXT("SM_RA_Bed_%d"), Where), TEXT("MI_RA_Seabed"), M);
		}
	}
}
