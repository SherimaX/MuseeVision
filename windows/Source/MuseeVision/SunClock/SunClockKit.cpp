#include "SunClock/SunClockBuild.h"

#include "SunClock/SunClockGlyphs.h"

namespace SunClockBuild
{
	// ---- The drum's door.

	double DoorHalfAngle() { return FMath::Asin((SC::DoorWidth / 2) / SC::DrumWallFace); }
	double DoorSpring() { return -SC::Lift + SC::DoorHeight - SC::DoorWidth / 2; }
	double DoorPhi(int32 J)
	{
		const double Psi = UE_DOUBLE_PI * J / ArchSegments;
		return UE_DOUBLE_PI - FMath::Asin(FMath::Sin(DoorHalfAngle()) * FMath::Cos(Psi));
	}
	double DoorZ(double R, int32 J)
	{
		const double Psi = UE_DOUBLE_PI * J / ArchSegments;
		return DoorSpring() + R * FMath::Sin(DoorHalfAngle()) * FMath::Sin(Psi);
	}

	const TArray<FStation>& DrumStations()
	{
		static const TArray<FStation> Stations = []
		{
			TArray<FStation> Out;
			const double Half = DoorHalfAngle(), Margin = 0.15 * Deg;
			for (int32 I = 0; I < Around; ++I)
			{
				const double Phi = GridAngle(I);
				if (FMath::Abs(Phi - UE_DOUBLE_PI) > Half + Margin) { Out.Add({Phi, INDEX_NONE}); }
			}
			for (int32 J = 0; J <= ArchSegments; ++J) { Out.Add({DoorPhi(J), J}); }
			Out.Sort([](const FStation& A, const FStation& B) { return A.Phi < B.Phi; });
			return Out;
		}();
		return Stations;
	}

	TArray<double> DrumRunAngles()
	{
		const TArray<FStation>& All = DrumStations();
		TArray<double> Out;
		const double North = DoorPhi(ArchSegments), South = DoorPhi(0);
		Out.Add(North);
		for (const FStation& S : All)
		{
			if (S.Arch == INDEX_NONE && S.Phi > North) { Out.Add(S.Phi); }
		}
		for (const FStation& S : All)
		{
			if (S.Arch == INDEX_NONE && S.Phi < South) { Out.Add(S.Phi + Turn); }
		}
		Out.Add(South + Turn);
		return Out;
	}

	// ---- The north port.

	double PortX(int32 J) { return SC::PortWidth / 2 * FMath::Cos(UE_DOUBLE_PI * J / ArchSegments); }
	double PortArchZ(int32 J) { return SC::PortSpring + SC::PortWidth / 2 * FMath::Sin(UE_DOUBLE_PI * J / ArchSegments); }
	double PortPhi(double R, int32 J)
	{
		const double X = PortX(J);
		return Wrap(FMath::Atan2(-FMath::Sqrt(R * R - X * X), X));
	}

	TArray<double> PortCircle(double R)
	{
		TArray<double> Out;
		const double Lo = PortPhi(R, ArchSegments), Hi = PortPhi(R, 0), Margin = 0.15 * Deg;
		for (int32 I = 0; I < Around; ++I)
		{
			const double Phi = GridAngle(I);
			if (Phi < Lo - Margin || Phi > Hi + Margin) { Out.Add(Phi); }
		}
		for (int32 J = 0; J <= ArchSegments; ++J) { Out.Add(PortPhi(R, J)); }
		Out.Sort();
		return Out;
	}

	// ---- The stair's layout.

	const FStairLayout& FStairLayout::Get()
	{
		static const FStairLayout Layout = []
		{
			FStairLayout L;
			// The top landing spans the door (164° … 194°); 18 treads, the mid landing (20°), 19 treads; the
			// last riser at −50°, so the foot faces the port (north, 270°) across the landing.
			L.Beta = 20 * Deg;
			L.Alpha = (214 * Deg - L.Beta) / 37;
			L.Top = 194 * Deg;
			L.Front[1] = 164 * Deg;
			for (int32 K = 2; K < SC::Risers; ++K) { L.Front[K] = L.Front[K - 1] - (K == MidLanding ? L.Beta : L.Alpha); }
			L.End = L.Front[SC::Risers - 1];
			// The split: the fixed balustrade's handrail there tops out at −0.23, under the lowered dial's ribs
			// (−0.13) with 10 cm to spare; the wall's rail (0.945 m over the ramp) starts a tread higher, at −0.18.
			L.Split = L.Front[8];
			L.WallRailStart = L.Front[7];
			const double HalfA = L.Alpha / 2;
			L.Ramp.Add(FVector2D(L.Top, L.TreadZ(1)));
			L.Ramp.Add(FVector2D(L.Front[1] + HalfA, L.TreadZ(1)));
			for (int32 K = 2; K < SC::Risers; ++K)
			{
				if (K == MidLanding)
				{
					L.Ramp.Add(FVector2D(L.Front[K - 1] - HalfA, L.TreadZ(K)));
					L.Ramp.Add(FVector2D(L.Front[K] + HalfA, L.TreadZ(K)));
				}
				else
				{
					L.Ramp.Add(FVector2D((L.Front[K] + L.Front[K - 1]) / 2, L.TreadZ(K)));
				}
			}
			L.Ramp.Add(FVector2D(L.End - HalfA, SC::Floor));
			L.Ramp.Add(FVector2D(L.End - 90 * Deg, SC::Floor));
			return L;
		}();
		return Layout;
	}

	double FStairLayout::RampZ(double Phi) const
	{
		if (Phi >= Ramp[0].X) { return Ramp[0].Y; }
		for (int32 I = 1; I < Ramp.Num(); ++I)
		{
			if (Phi >= Ramp[I].X)
			{
				const double T = (Phi - Ramp[I - 1].X) / (Ramp[I].X - Ramp[I - 1].X);
				return FMath::Lerp(Ramp[I - 1].Y, Ramp[I].Y, T);
			}
		}
		return Ramp.Last().Y;
	}

	double FStairLayout::CurbTop(double Phi) const { return FMath::Min(RampZ(Phi) + 0.12, -0.14); }
	double FStairLayout::SoffitZ(double Phi) const { return FMath::Max(RampZ(Phi) - 0.40, SC::Floor - 0.02); }

	TArray<double> FStairLayout::Kinks() const
	{
		TArray<double> Out;
		// Only where the pitch changes (the landings' ends): the flights' points are in line.
		for (int32 I = 1; I + 1 < Ramp.Num(); ++I)
		{
			const double In = (Ramp[I].Y - Ramp[I - 1].Y) / (Ramp[I].X - Ramp[I - 1].X);
			const double Out2 = (Ramp[I + 1].Y - Ramp[I].Y) / (Ramp[I + 1].X - Ramp[I].X);
			if (FMath::Abs(In - Out2) > 1e-6) { Out.Add(Ramp[I].X); }
		}
		// Where the string's top leaves −0.14, and where the underside reaches the floor.
		for (const double Level : {-0.26, SC::Floor + 0.38})
		{
			for (int32 I = 1; I < Ramp.Num(); ++I)
			{
				const double Z0 = Ramp[I - 1].Y, Z1 = Ramp[I].Y;
				if ((Z0 - Level) * (Z1 - Level) < 0)
				{
					Out.Add(FMath::Lerp(Ramp[I - 1].X, Ramp[I].X, (Level - Z0) / (Z1 - Z0)));
				}
			}
		}
		Out.Sort([](double A, double B) { return A > B; });
		return Out;
	}

	// ---- Mesh helpers.

	TArray<double> GridAngles()
	{
		TArray<double> Out;
		for (int32 I = 0; I < Around; ++I) { Out.Add(GridAngle(I)); }
		return Out;
	}

	void Revolve(FMeshData& M, const FProfile& Profile, const TArray<double>& Angles, bool bClosed)
	{
		double MeanR = 0;
		for (const SalonKit::FProfilePoint& P : Profile.Points) { MeanR += P.P.X; }
		MeanR /= FMath::Max(1, Profile.Points.Num());
		TArray<FFrame> Run;
		auto Add = [&Run, MeanR](double Phi)
		{
			FFrame F;
			F.Origin = FVector::ZeroVector;
			F.AxisA = F.NormA = RadialDir(Phi);
			F.AxisB = F.NormB = FVector::UpVector;
			F.S = Phi * MeanR;
			Run.Add(F);
		};
		for (const double Phi : Angles) { Add(Phi); }
		if (bClosed && Angles.Num() > 0) { Add(Angles[0] + Turn); }
		SalonKit::Sweep(M, Run, Profile);
	}

	void Zip(FMeshData& M, const TArray<int32>& A, const TArray<double>& ParamA, const TArray<int32>& B, const TArray<double>& ParamB)
	{
		int32 I = 0, J = 0;
		const int32 NA = A.Num(), NB = B.Num();
		if (NA == 0 || NB == 0) { return; }
		while (I < NA - 1 || J < NB - 1)
		{
			if (I == NA - 1 || (J < NB - 1 && ParamB[J + 1] <= ParamA[I + 1]))
			{
				M.Tri(A[I], B[J], B[J + 1]);
				++J;
			}
			else
			{
				M.Tri(A[I], B[J], A[I + 1]);
				++I;
			}
		}
	}

	static void RingRow(FMeshData& M, double R, const TArray<double>& Angles, double Z, const FVector& N, bool bClosed,
						TArray<int32>& OutIdx, TArray<double>& OutParam)
	{
		for (const double Phi : Angles)
		{
			const FVector P = Polar(R, Phi, Z);
			OutIdx.Add(M.Vertex(P, N, FVector2D(P.X, P.Y)));
			OutParam.Add(Phi);
		}
		if (bClosed && Angles.Num() > 0)
		{
			const int32 First = OutIdx[0];
			OutIdx.Add(First);
			OutParam.Add(Angles[0] + Turn);
		}
	}

	void Annulus(FMeshData& M, double R0, const TArray<double>& Inner, double R1, const TArray<double>& Outer, double Z, bool bUp)
	{
		const FVector N = bUp ? FVector::UpVector : -FVector::UpVector;
		TArray<int32> IA, IB;
		TArray<double> PA, PB;
		if (R0 <= 1e-9)
		{
			// A disc: a fan from the centre.
			const int32 C = M.Vertex(FVector(0, 0, Z), N, FVector2D::ZeroVector);
			RingRow(M, R1, Outer, Z, N, true, IB, PB);
			for (int32 I = 0; I + 1 < IB.Num(); ++I) { M.Tri(C, IB[I], IB[I + 1]); }
			return;
		}
		RingRow(M, R0, Inner, Z, N, true, IA, PA);
		RingRow(M, R1, Outer, Z, N, true, IB, PB);
		Zip(M, IA, PA, IB, PB);
	}

	void Sector(FMeshData& M, double R0, const TArray<double>& Inner, double R1, const TArray<double>& Outer, double Z, bool bUp)
	{
		const FVector N = bUp ? FVector::UpVector : -FVector::UpVector;
		TArray<int32> IA, IB;
		TArray<double> PA, PB;
		RingRow(M, R0, Inner, Z, N, false, IA, PA);
		RingRow(M, R1, Outer, Z, N, false, IB, PB);
		Zip(M, IA, PA, IB, PB);
	}

	void CylinderColumns(FMeshData& M, double R, const TArray<FColumn>& Columns, bool bOutward, bool bClosed)
	{
		TArray<TArray<TArray<int32>>> Idx;
		for (const FColumn& C : Columns)
		{
			const FVector N = RadialDir(C.Phi) * (bOutward ? 1.0 : -1.0);
			TArray<TArray<int32>>& Spans = Idx.AddDefaulted_GetRef();
			for (const TArray<double>& Span : C.Spans)
			{
				TArray<int32>& Row = Spans.AddDefaulted_GetRef();
				for (const double Z : Span) { Row.Add(M.Vertex(Polar(R, C.Phi, Z), N, FVector2D(R * C.Phi, -Z))); }
			}
		}
		const int32 Count = Columns.Num();
		for (int32 I = 0; I < (bClosed ? Count : Count - 1); ++I)
		{
			const int32 J = (I + 1) % Count;
			double DPhi = Columns[J].Phi - Columns[I].Phi;
			if (J == 0) { DPhi += Turn; }
			if (FMath::Abs(DPhi) < 1e-9) { continue; }
			const int32 Spans = FMath::Min(Columns[I].Spans.Num(), Columns[J].Spans.Num());
			for (int32 S = 0; S < Spans; ++S)
			{
				Zip(M, Idx[I][S], Columns[I].Spans[S], Idx[J][S], Columns[J].Spans[S]);
			}
		}
	}

	TArray<int32> EarClip(const TArray<FVector2D>& Poly)
	{
		TArray<int32> Out;
		const int32 N = Poly.Num();
		if (N < 3) { return Out; }
		double Area = 0;
		for (int32 I = 0; I < N; ++I) { Area += SalonKit::Cross2(Poly[I], Poly[(I + 1) % N]); }
		TArray<int32> V;
		for (int32 I = 0; I < N; ++I) { V.Add(Area >= 0 ? I : N - 1 - I); }
		auto Cross = [&Poly](int32 O, int32 A, int32 B) { return SalonKit::Cross2(Poly[A] - Poly[O], Poly[B] - Poly[O]); };
		auto Inside = [&Poly, &Cross](int32 P, int32 A, int32 B, int32 C)
		{
			const FVector2D Q = Poly[P];
			if (Q.Equals(Poly[A], 1e-12) || Q.Equals(Poly[B], 1e-12) || Q.Equals(Poly[C], 1e-12)) { return false; }
			return Cross(A, B, P) >= -1e-14 && Cross(B, C, P) >= -1e-14 && Cross(C, A, P) >= -1e-14;
		};
		int32 Guard = 0;
		while (V.Num() > 3 && Guard++ < 100000)
		{
			bool bCut = false;
			const int32 Count = V.Num();
			for (int32 K = 0; K < Count; ++K)
			{
				const int32 A = V[(K + Count - 1) % Count], B = V[K], C = V[(K + 1) % Count];
				if (Cross(A, B, C) <= 1e-14) { continue; }
				bool bBlocked = false;
				for (const int32 P : V)
				{
					if (P != A && P != B && P != C && Inside(P, A, B, C)) { bBlocked = true; break; }
				}
				if (bBlocked) { continue; }
				Out.Append({A, B, C});
				V.RemoveAt(K);
				bCut = true;
				break;
			}
			if (!bCut)
			{
				// Only collinear leftovers: drop the flattest.
				int32 Best = 0;
				double BestArea = TNumericLimits<double>::Max();
				for (int32 K = 0; K < Count; ++K)
				{
					const double A = FMath::Abs(Cross(V[(K + Count - 1) % Count], V[K], V[(K + 1) % Count]));
					if (A < BestArea) { BestArea = A; Best = K; }
				}
				V.RemoveAt(Best);
			}
		}
		if (V.Num() == 3 && Cross(V[0], V[1], V[2]) > 1e-14) { Out.Append({V[0], V[1], V[2]}); }
		return Out;
	}

	void FlatPolygon(FMeshData& M, const TArray<FVector2D>& Outline, TFunctionRef<FVector(const FVector2D&)> Place, const FVector& Normal)
	{
		const TArray<int32> Tris = EarClip(Outline);
		const int32 Base = M.Positions.Num();
		for (const FVector2D& Q : Outline)
		{
			const FVector P = Place(Q);
			M.Vertex(P, Normal, SalonKit::FaceUV(P, Normal));
		}
		for (int32 T = 0; T + 2 < Tris.Num(); T += 3) { M.Tri(Base + Tris[T], Base + Tris[T + 1], Base + Tris[T + 2]); }
	}

	FProfile InlaySection(double HalfWidth, double Height, double Bevel, double Sink)
	{
		FProfile P;
		P.Add(HalfWidth, -Sink).Add(HalfWidth, Height - Bevel).Add(HalfWidth - Bevel, Height)
			.Add(-HalfWidth + Bevel, Height).Add(-HalfWidth, Height - Bevel).Add(-HalfWidth, -Sink);
		return P;
	}

	void InlayBar(FMeshData& M, const FVector2D& A, const FVector2D& B, double Z0, double Width, double Height, double Bevel, double Sink)
	{
		const FVector2D Dir = (B - A).GetSafeNormal();
		const FVector Lateral(-Dir.Y, Dir.X, 0.0);
		const FProfile Section = InlaySection(Width / 2, Height, Bevel, Sink);
		TArray<FFrame> Run;
		for (const FVector2D& P : {A, B})
		{
			FFrame F;
			F.Origin = FVector(P.X, P.Y, Z0);
			F.AxisA = F.NormA = Lateral;
			F.AxisB = F.NormB = FVector::UpVector;
			F.S = FVector2D::Distance(A, P);
			Run.Add(F);
		}
		SalonKit::Sweep(M, Run, Section);
		SalonKit::CapConvex(M, Run[0], Section, -FVector(Dir.X, Dir.Y, 0));
		SalonKit::CapConvex(M, Run[1], Section, FVector(Dir.X, Dir.Y, 0));
	}

	void InlayRing(FMeshData& M, double R0, double R1, double Z0, double Height, double Bevel, double Sink, const TArray<double>& Angles)
	{
		FProfile P;
		P.Add(R1, Z0 - Sink).Add(R1, Z0 + Height - Bevel).Add(R1 - Bevel, Z0 + Height)
			.Add(R0 + Bevel, Z0 + Height).Add(R0, Z0 + Height - Bevel).Add(R0, Z0 - Sink);
		Revolve(M, P, Angles, true);
	}

	// ---- Text.

	double TextWidth(const FString& Text, const FTextStyle& Style)
	{
		double Pen = 0, Last = 0;
		for (const TCHAR C : Text)
		{
			if (C == TEXT(' ')) { Pen += Style.WordGap; Last = 0; continue; }
			if (C == TEXT('.')) { Pen += Style.StopGap; Last = 0; continue; }
			if (const SunClockGlyphs::FGlyph* G = SunClockGlyphs::Find(C))
			{
				Pen += G->Advance + Style.Tracking;
				Last = Style.Tracking;
			}
		}
		return Pen - Last;
	}

	static void AddGlyph(FMeshData& M, const SunClockGlyphs::FGlyph& G, double PenX, TFunctionRef<FVector2D(double, double)> Place,
						 double CapHeight, double Z0, double Height, double Bevel, double Sink)
	{
		using namespace SunClockGlyphs;
		const double B = Bevel / CapHeight;           // the bevel in cap heights
		const double ZTop = Z0 + Height, ZEdge = Z0 + Height - Bevel, ZFoot = Z0 - Sink;
		const FVector Up = FVector::UpVector;
		const int32 First = ContourStart[G.FirstContour];
		const int32 Last = ContourStart[G.FirstContour + G.NumContours];
		const int32 Count = Last - First;
		TArray<FVector2D> Base, Inset, InDir;
		Base.SetNum(Count);
		Inset.SetNum(Count);
		InDir.SetNum(Count);
		for (int32 I = 0; I < Count; ++I)
		{
			const int32 P = First + I;
			const double X = PenX + Point[2 * P], Y = Point[2 * P + 1];
			const double MX = Mitre[2 * P], MY = Mitre[2 * P + 1];
			Base[I] = Place(X, Y);
			Inset[I] = Place(X + B * MX, Y + B * MY);
			// Into the solid, in plan (from the mitre, or the glyph's own x/y if the bevel is nil there).
			FVector2D D = Place(X + 0.01 * MX, Y + 0.01 * MY) - Base[I];
			InDir[I] = D;
		}
		// Each edge's outward normal in plan.
		TArray<FVector> EdgeN;
		EdgeN.SetNum(Count);
		TArray<int32> Next;
		Next.SetNum(Count);
		TArray<int32> Prev;
		Prev.SetNum(Count);
		for (int32 C = 0; C < G.NumContours; ++C)
		{
			const int32 S = ContourStart[G.FirstContour + C] - First, E = ContourStart[G.FirstContour + C + 1] - First;
			for (int32 I = S; I < E; ++I)
			{
				const int32 J = I + 1 < E ? I + 1 : S;
				Next[I] = J;
				Prev[J] = I;
				const FVector2D Edge = Base[J] - Base[I];
				FVector2D N(Edge.Y, -Edge.X);
				if (FVector2D::DotProduct(N, InDir[I] + InDir[J]) > 0) { N = -N; }
				N.Normalize();
				EdgeN[I] = FVector(N.X, N.Y, 0);
			}
		}
		auto VertexN = [&](int32 I, int32 Edge) -> FVector
		{
			// A smooth point takes the mean of its two edges.
			if (Smooth[First + I]) { return (EdgeN[Prev[I]] + EdgeN[I]).GetSafeNormal(); }
			return EdgeN[Edge];
		};
		auto P3 = [](const FVector2D& Q, double Z) { return FVector(Q.X, Q.Y, Z); };
		auto V = [&M](const FVector& P, const FVector& N) { return M.Vertex(P, N, SalonKit::FaceUV(P, N)); };
		for (int32 I = 0; I < Count; ++I)
		{
			const int32 J = Next[I];
			const FVector NI = VertexN(I, I), NJ = VertexN(J, I);
			// The side.
			M.Quad(V(P3(Base[I], ZFoot), NI), V(P3(Base[J], ZFoot), NJ), V(P3(Base[J], ZEdge), NJ), V(P3(Base[I], ZEdge), NI));
			// The bevel.
			const FVector CI = (NI + Up).GetSafeNormal(), CJ = (NJ + Up).GetSafeNormal();
			M.Quad(V(P3(Base[I], ZEdge), CI), V(P3(Base[J], ZEdge), CJ), V(P3(Inset[J], ZTop), CJ), V(P3(Inset[I], ZTop), CI));
		}
		// The top, on the glyph's own triangles.
		const int32 TopBase = M.Positions.Num();
		for (int32 I = 0; I < Count; ++I) { V(P3(Inset[I], ZTop), Up); }
		for (int32 T = 0; T < G.NumTris; ++T)
		{
			const int32 A = Tri[3 * (G.FirstTri + T)] - First, BB = Tri[3 * (G.FirstTri + T) + 1] - First, C = Tri[3 * (G.FirstTri + T) + 2] - First;
			M.Tri(TopBase + A, TopBase + BB, TopBase + C);
		}
	}

	void AddText(FMeshData& M, const FString& Text, const FTextStyle& Style, TFunctionRef<FVector2D(double, double)> Place,
				 double CapHeight, double Z0, double Height, double Bevel, double Sink, TFunctionRef<void(double)> Stop)
	{
		double Pen = 0;
		for (const TCHAR C : Text)
		{
			if (C == TEXT(' ')) { Pen += Style.WordGap; continue; }
			if (C == TEXT('.'))
			{
				Stop(Pen + Style.StopGap / 2);
				Pen += Style.StopGap;
				continue;
			}
			if (const SunClockGlyphs::FGlyph* G = SunClockGlyphs::Find(C))
			{
				AddGlyph(M, *G, Pen, Place, CapHeight, Z0, Height, Bevel, Sink);
				Pen += G->Advance + Style.Tracking;
			}
		}
	}

	void FrameBox(FMeshData& M, const FVector& Origin, const FVector& AxisA, const FVector& AxisB, const FVector& AxisC,
				  const FVector& Lo, const FVector& Hi, int32 Faces)
	{
		auto C = [&](double A, double B, double Cc) { return Origin + AxisA * A + AxisB * B + AxisC * Cc; };
		const double X0 = Lo.X, X1 = Hi.X, Y0 = Lo.Y, Y1 = Hi.Y, Z0 = Lo.Z, Z1 = Hi.Z;
		if (Faces & FMeshData::NegX) { M.Rect(C(X0, Y0, Z0), C(X0, Y1, Z0), C(X0, Y1, Z1), C(X0, Y0, Z1), -AxisA); }
		if (Faces & FMeshData::PosX) { M.Rect(C(X1, Y0, Z0), C(X1, Y1, Z0), C(X1, Y1, Z1), C(X1, Y0, Z1), AxisA); }
		if (Faces & FMeshData::NegY) { M.Rect(C(X0, Y0, Z0), C(X1, Y0, Z0), C(X1, Y0, Z1), C(X0, Y0, Z1), -AxisB); }
		if (Faces & FMeshData::PosY) { M.Rect(C(X0, Y1, Z0), C(X1, Y1, Z0), C(X1, Y1, Z1), C(X0, Y1, Z1), AxisB); }
		if (Faces & FMeshData::NegZ) { M.Rect(C(X0, Y0, Z0), C(X1, Y0, Z0), C(X1, Y1, Z0), C(X0, Y1, Z0), -AxisC); }
		if (Faces & FMeshData::PosZ) { M.Rect(C(X0, Y0, Z1), C(X1, Y0, Z1), C(X1, Y1, Z1), C(X0, Y1, Z1), AxisC); }
	}

	void BuildAll(FSunClockMeshes& M)
	{
		BuildDial(M);
		BuildDrum(M);
		BuildRing(M);
		BuildShaft(M);
		BuildStair(M);
		BuildRails(M);
	}
}
