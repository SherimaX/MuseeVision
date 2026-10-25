#include "Chenghuai/ChenghuaiPaint.h"

namespace ChenghuaiPaint
{
	using ChenghuaiKit::FMeshData;

	void Box(FPart& Part, const FString& Name, const FLocal& L, double U0, double U1, double V0, double V1, double Z0, double Z1,
			 int32 Row, double B0, double B1)
	{
		Part.Begin(Name);
		FMeshData& M = Part.M;
		const double Len = FMath::Max(U1 - U0, 1e-3), Hgt = FMath::Max(Z1 - Z0, 1e-3);
		auto BandV = [&](double Z) { return RowV0(Row) + (B0 + (1.0 - (Z - Z0) / Hgt) * (B1 - B0)) / Rows; };
		auto Quad = [&](const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& N,
						const FVector2D& UA, const FVector2D& UB, const FVector2D& UC, const FVector2D& UD)
		{
			const int32 I0 = M.Vertex(A, N, UA), I1 = M.Vertex(B, N, UB), I2 = M.Vertex(C, N, UC), I3 = M.Vertex(D, N, UD);
			M.Quad(I0, I1, I2, I3);
		};
		// The long faces: the band, u across the row (the back face mirrored, so both read from the same end).
		for (const int32 Side : {0, 1})
		{
			const double V = Side == 0 ? V0 : V1;
			const FVector N = L.V3() * (Side == 0 ? -1.0 : 1.0);
			auto UVOf = [&](double U, double Z) { return FVector2D((U - U0) / Len, BandV(Z)); };
			Quad(L.P(U0, V, Z0), L.P(U1, V, Z0), L.P(U1, V, Z1), L.P(U0, V, Z1), N, UVOf(U0, Z0), UVOf(U1, Z0), UVOf(U1, Z1), UVOf(U0, Z1));
		}
		// Underside and top: the middle of the plain row, u along it.
		const double Mid0 = RowV0(4) + 0.40 / Rows, Mid1 = RowV0(4) + 0.60 / Rows;
		for (const int32 Side : {0, 1})
		{
			const double Z = Side == 0 ? Z0 : Z1;
			const FVector N(0, 0, Side == 0 ? -1.0 : 1.0);
			Quad(L.P(U0, V0, Z), L.P(U1, V0, Z), L.P(U1, V1, Z), L.P(U0, V1, Z), N,
				 FVector2D(0.0, Mid0), FVector2D(1.0, Mid0), FVector2D(1.0, Mid1), FVector2D(0.0, Mid1));
		}
		// The ends: the end band (箍头) of the row.
		for (const int32 Side : {0, 1})
		{
			const double U = Side == 0 ? U0 : U1;
			const FVector N = L.U3() * (Side == 0 ? -1.0 : 1.0);
			const double E0 = Side == 0 ? 0.0 : 0.97, E1 = Side == 0 ? 0.03 : 1.0;
			Quad(L.P(U, V0, Z0), L.P(U, V1, Z0), L.P(U, V1, Z1), L.P(U, V0, Z1), N,
				 FVector2D(E0, BandV(Z0)), FVector2D(E1, BandV(Z0)), FVector2D(E1, BandV(Z1)), FVector2D(E0, BandV(Z1)));
		}
	}

	void RoundBox(FPart& Part, const FString& Name, const FLocal& L, double U0, double U1, double V0, double V1, double Z0, double Z1,
				  int32 Row, double B0, double B1, double Arris, bool bPaintedUnder)
	{
		Part.Begin(Name);
		FMeshData& M = Part.M;
		const double Len = FMath::Max(U1 - U0, 1e-3), Hgt = FMath::Max(Z1 - Z0, 1e-3), Wid = FMath::Max(V1 - V0, 1e-3);
		const double R = FMath::Clamp(Arris, 0.0, 0.45 * FMath::Min(Hgt, Wid));
		auto BandV = [&](double Z) { return RowV0(Row) + (B0 + (1.0 - (Z - Z0) / Hgt) * (B1 - B0)) / Rows; };
		const double Mid0 = RowV0(4) + 0.40 / Rows, Mid1 = RowV0(4) + 0.60 / Rows;
		// The section (v, z) round the box with its four arrises rounded (radius R, 4 steps each), with each point's
		// normal and its atlas v: a long face's band by height, the underside the band across its width (a painted
		// soffit, round 9) or the plain row's middle, the top the plain row's middle.
		struct FPt { FVector2D P; FVector2D N; double TV; };
		TArray<FPt> Sec;
		const int32 Steps = R > 1e-4 ? 4 : 0;
		const FVector2D Corners[4] = {FVector2D(V0 + R, Z0 + R), FVector2D(V1 - R, Z0 + R), FVector2D(V1 - R, Z1 - R), FVector2D(V0 + R, Z1 - R)};
		const double A0[4] = {180.0, 270.0, 0.0, 90.0};
		auto TexV = [&](const FVector2D& P, const FVector2D& N)
		{
			const double Side = FMath::Abs(N.X), Down = FMath::Max(0.0, -N.Y), Up = FMath::Max(0.0, N.Y);
			const double VSide = BandV(FMath::Clamp(P.Y, Z0, Z1));
			const double Across = (P.X - V0) / Wid;
			const double VUnder = bPaintedUnder ? RowV0(Row) + (B0 + Across * (B1 - B0)) / Rows : FMath::Lerp(Mid0, Mid1, Across);
			const double VTop = FMath::Lerp(Mid0, Mid1, Across);
			const double W = Side + Down + Up + 1e-6;
			return (VSide * Side + VUnder * Down + VTop * Up) / W;
		};
		for (int32 c = 0; c < 4; ++c)
		{
			for (int32 k = 0; k <= Steps; ++k)
			{
				const double An = FMath::DegreesToRadians(A0[c] + 90.0 * (Steps ? double(k) / Steps : 0.5));
				const FVector2D N(FMath::Cos(An), FMath::Sin(An));
				const FVector2D P = Corners[c] + N * R;
				Sec.Add({P, N, TexV(P, N)});
			}
		}
		const int32 NS = Sec.Num();
		auto W3 = [&](double U, const FVector2D& P) { return L.P(U, P.X, P.Y); };
		auto N3 = [&](const FVector2D& N) { return L.V3() * N.X + FVector(0, 0, N.Y); };
		for (int32 i = 0; i < NS; ++i)
		{
			const FPt& A = Sec[i];
			const FPt& B = Sec[(i + 1) % NS];
			// A flat face between corners keeps its own normal; within a corner the normals are smooth.
			const bool bFlat = (i % (Steps + 1)) == Steps;
			const FVector2D NA = bFlat ? (A.N + B.N).GetSafeNormal() : A.N, NB = bFlat ? (A.N + B.N).GetSafeNormal() : B.N;
			const double TA = bFlat ? TexV(0.5 * (A.P + B.P), NA) : A.TV, TB = bFlat ? TexV(0.5 * (A.P + B.P), NB) : B.TV;
			const double TA2 = bFlat ? TexV(A.P, NA) : TA, TB2 = bFlat ? TexV(B.P, NB) : TB;
			const int32 I0 = M.Vertex(W3(U0, A.P), N3(NA), FVector2D(0.0, TA2)), I1 = M.Vertex(W3(U1, A.P), N3(NA), FVector2D(1.0, TA2));
			const int32 I2 = M.Vertex(W3(U1, B.P), N3(NB), FVector2D(1.0, TB2)), I3 = M.Vertex(W3(U0, B.P), N3(NB), FVector2D(0.0, TB2));
			M.Quad(I0, I1, I2, I3);
		}
		// The ends: the end band (箍头), fanned from the section's centre.
		const FVector2D C(0.5 * (V0 + V1), 0.5 * (Z0 + Z1));
		for (const int32 Side : {0, 1})
		{
			const double U = Side == 0 ? U0 : U1;
			const FVector N = L.U3() * (Side == 0 ? -1.0 : 1.0);
			const double E = Side == 0 ? 0.015 : 0.985;
			const int32 IC = M.Vertex(W3(U, C), N, FVector2D(E, BandV(C.Y)));
			TArray<int32> Ring;
			for (const FPt& P : Sec) { Ring.Add(M.Vertex(W3(U, P.P), N, FVector2D(E, BandV(FMath::Clamp(P.P.Y, Z0, Z1))))); }
			for (int32 i = 0; i < NS; ++i) { M.Tri(IC, Ring[i], Ring[(i + 1) % NS]); }
		}
	}

	void Rod(FPart& Part, const FString& Name, const FVector& A, const FVector& B, double R, int32 Segments, int32 Row, double Repeat)
	{
		const double Len = FVector::Distance(A, B);
		if (Len < 1e-4) { return; }
		Part.Begin(Name);
		FMeshData& M = Part.M;
		const FVector W = (B - A) / Len;
		const FVector Ref = FMath::Abs(W.Z) < 0.9 ? FVector::UpVector : FVector::ForwardVector;
		const FVector E1 = FVector::CrossProduct(W, Ref).GetSafeNormal();
		const FVector E2 = FVector::CrossProduct(W, E1).GetSafeNormal();
		const double VA = RowV0(Row) + 0.05 / Rows, VB = RowV1(Row) - 0.05 / Rows;
		const int32 Base = M.Positions.Num();
		for (int32 k = 0; k <= Segments; ++k)
		{
			const double T = 2.0 * UE_DOUBLE_PI * (k % Segments) / Segments;
			const FVector N = E1 * FMath::Cos(T) + E2 * FMath::Sin(T);
			const double V = FMath::Lerp(VA, VB, double(k) / Segments);
			M.Vertex(A + N * R, N, FVector2D(0.0, V));
			M.Vertex(B + N * R, N, FVector2D(Len / Repeat, V));
		}
		for (int32 k = 0; k < Segments; ++k) { M.Quad(Base + 2 * k, Base + 2 * k + 1, Base + 2 * k + 3, Base + 2 * k + 2); }
		for (const int32 End : {0, 1})
		{
			const FVector C = End == 0 ? A : B;
			const FVector N = End == 0 ? -W : W;
			const int32 Centre = M.Vertex(C, N, FVector2D(0.0, 0.5 * (VA + VB)));
			const int32 First = M.Positions.Num();
			for (int32 k = 0; k < Segments; ++k)
			{
				const double T = 2.0 * UE_DOUBLE_PI * k / Segments;
				M.Vertex(C + (E1 * FMath::Cos(T) + E2 * FMath::Sin(T)) * R, N, FVector2D(0.0, 0.5 * (VA + VB)));
			}
			for (int32 k = 0; k < Segments; ++k) { M.Tri(Centre, First + k, First + (k + 1) % Segments); }
		}
	}

	void EndCap(FPart& Part, const FVector& Centre, const FVector& N, const FVector& Up, double Half, bool bRound, int32 Cell)
	{
		Part.Begin(TEXT("Painted end"), false);
		FMeshData& M = Part.M;
		const FVector Nn = N.GetSafeNormal();
		const FVector Side = FVector::CrossProduct(Nn, Up).GetSafeNormal();   // the viewer's right, facing the face
		const FVector Upn = FVector::CrossProduct(Side, Nn).GetSafeNormal();
		const int32 Row = 6 + Cell / 8, Col = Cell % 8;
		const double CU = (Col + 0.5) / 8.0, CV = RowV0(Row) + 0.5 / Rows;
		const double SU = 0.5 / 8.0 * 0.96, SV = 0.5 / Rows * 0.96;
		auto UVOf = [&](double X, double Y) { return FVector2D(CU + X * SU, CV - Y * SV); };
		if (bRound)
		{
			constexpr int32 Segs = 16;
			const int32 C = M.Vertex(Centre, Nn, UVOf(0, 0));
			const int32 First = M.Positions.Num();
			for (int32 k = 0; k < Segs; ++k)
			{
				const double T = 2.0 * UE_DOUBLE_PI * k / Segs;
				const double X = FMath::Cos(T), Y = FMath::Sin(T);
				M.Vertex(Centre + (Side * X + Upn * Y) * Half, Nn, UVOf(X, Y));
			}
			for (int32 k = 0; k < Segs; ++k) { M.Tri(C, First + k, First + (k + 1) % Segs); }
		}
		else
		{
			const int32 A = M.Vertex(Centre + (-Side - Upn) * Half, Nn, UVOf(-1, -1));
			const int32 B = M.Vertex(Centre + (Side - Upn) * Half, Nn, UVOf(1, -1));
			const int32 C = M.Vertex(Centre + (Side + Upn) * Half, Nn, UVOf(1, 1));
			const int32 D = M.Vertex(Centre + (-Side + Upn) * Half, Nn, UVOf(-1, 1));
			M.Quad(A, B, C, D);
		}
	}

	void AtlasQuad(FPart& Part, const FVector& Centre, const FVector& N, const FVector& Up, double HalfW, double HalfH, const FBox2D& UV)
	{
		Part.Begin(TEXT("Atlas face"), false);
		FMeshData& M = Part.M;
		const FVector Nn = N.GetSafeNormal();
		// Side: to the right as one faces the face (looking along −N).
		const FVector Side = FVector::CrossProduct(Nn, Up).GetSafeNormal();   // the viewer's right, facing the face
		const FVector Upn = FVector::CrossProduct(Side, Nn).GetSafeNormal();
		auto At = [&](double X, double Y) { return Centre + Side * (X * HalfW) + Upn * (Y * HalfH); };
		auto UVOf = [&](double X, double Y) { return FVector2D(FMath::Lerp(UV.Min.X, UV.Max.X, 0.5 * (X + 1.0)), FMath::Lerp(UV.Max.Y, UV.Min.Y, 0.5 * (Y + 1.0))); };
		const int32 A = M.Vertex(At(-1, -1), Nn, UVOf(-1, -1)), B = M.Vertex(At(1, -1), Nn, UVOf(1, -1));
		const int32 C = M.Vertex(At(1, 1), Nn, UVOf(1, 1)), D = M.Vertex(At(-1, 1), Nn, UVOf(-1, 1));
		M.Quad(A, B, C, D);
	}

	void AtlasDisc(FPart& Part, const FVector& Centre, const FVector& N, const FVector& Up, double R, const FBox2D& UV)
	{
		Part.Begin(TEXT("Atlas disc"), false);
		FMeshData& M = Part.M;
		const FVector Nn = N.GetSafeNormal();
		const FVector Side = FVector::CrossProduct(Nn, Up).GetSafeNormal();   // the viewer's right, facing the face
		const FVector Upn = FVector::CrossProduct(Side, Nn).GetSafeNormal();
		auto UVOf = [&](double X, double Y) { return FVector2D(FMath::Lerp(UV.Min.X, UV.Max.X, 0.5 * (X + 1.0)), FMath::Lerp(UV.Max.Y, UV.Min.Y, 0.5 * (Y + 1.0))); };
		constexpr int32 Segs = 32;
		const int32 C = M.Vertex(Centre, Nn, UVOf(0, 0));
		const int32 First = M.Positions.Num();
		for (int32 k = 0; k < Segs; ++k)
		{
			const double T = 2.0 * UE_DOUBLE_PI * k / Segs;
			const double X = FMath::Cos(T), Y = FMath::Sin(T);
			M.Vertex(Centre + (Side * X + Upn * Y) * R, Nn, UVOf(X, Y));
		}
		for (int32 k = 0; k < Segs; ++k) { M.Tri(C, First + k, First + (k + 1) % Segs); }
	}
}
