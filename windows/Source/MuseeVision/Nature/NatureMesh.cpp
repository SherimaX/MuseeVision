#include "Nature/NatureMesh.h"

#include "MuseeVision.h"
#include "Materials/MaterialInterface.h"
#include "UObject/SoftObjectPath.h"

UMaterialInterface* MuseeNature::LoadMaterial(const TCHAR* Name)
{
	const FString Path = FString::Printf(TEXT("/Game/Museum/Nature/%s.%s"), Name, Name);
	UMaterialInterface* Material = Cast<UMaterialInterface>(FSoftObjectPath(Path).TryLoad());
	if (!Material)
	{
		static TSet<FString> Warned;
		if (!Warned.Contains(Path))
		{
			Warned.Add(Path);
			UE_LOG(LogMusee, Warning, TEXT("Nature: %s is missing; run Scripts/nature.py."), *Path);
		}
	}
	return Material;
}

void MuseeNature::AddTube(FNatureMesh& Mesh, const TArray<FNatureTubeRing>& Rings, int32 Sides, double VStart, bool bCapEnd,
						  const FNatureTubeShape& Shape)
{
	const int32 NumRings = Rings.Num();
	if (NumRings < 2 || Sides < 3) { return; }

	// Parallel-transported frame, so the tube doesn't twist.
	FVector U, W;
	Basis(Rings[0].Dir, U, W);
	FVector PrevDir = Rings[0].Dir;
	// u runs round the tube in metres, rounded so the seam falls on whole 5 cm.
	const double Round = FMath::Max(1.0, FMath::RoundToDouble(Tau * Rings[0].Radius / 0.05)) * 0.05;
	double V = VStart;

	TArray<int32> Prev, Cur;
	Prev.SetNum(Sides + 1);
	Cur.SetNum(Sides + 1);
	for (int32 i = 0; i < NumRings; ++i)
	{
		const FNatureTubeRing& Ring = Rings[i];
		if (i > 0)
		{
			V += FVector::Dist(Rings[i - 1].Centre, Ring.Centre);
			U = FQuat::FindBetweenNormals(PrevDir, Ring.Dir).RotateVector(U);
			U = (U - Ring.Dir * FVector::DotProduct(U, Ring.Dir)).GetSafeNormal();
			if (U.IsNearlyZero()) { Basis(Ring.Dir, U, W); }
			W = FVector::CrossProduct(Ring.Dir, U).GetSafeNormal();
			PrevDir = Ring.Dir;
		}
		// The normals lean forward where the tube narrows.
		const int32 I0 = FMath::Max(0, i - 1), I1 = FMath::Min(NumRings - 1, i + 1);
		const double Run = FVector::Dist(Rings[I0].Centre, Rings[I1].Centre);
		const double Slope = Run > 1e-6 ? (Rings[I0].Radius - Rings[I1].Radius) / Run : 0.0;
		for (int32 k = 0; k <= Sides; ++k)
		{
			const double A = Tau * k / Sides;
			const FVector Radial = U * FMath::Cos(A) + W * FMath::Sin(A);
			double Scale = 1.0;
			if (Shape.Flare > 0)
			{
				const double Lobe = Shape.Lobes > 0 ? 0.55 + 0.45 * FMath::Pow(FMath::Abs(FMath::Cos(0.5 * Shape.Lobes * A + Shape.Seed)), 3.0) : 1.0;
				Scale += Shape.Flare * FMath::Exp(-FMath::Max(0.0, Ring.Centre.Z) / Shape.FlareHeight) * Lobe;
			}
			if (Shape.Gnarl > 0)
			{
				Scale += Shape.Gnarl * (Noise(FVector(FMath::Cos(A) * 1.4, FMath::Sin(A) * 1.4, V * 2.3), Shape.Seed) - 0.5) * 2.0;
			}
			const FVector P = Ring.Centre + Radial * (Ring.Radius * Scale);
			const FVector N = (Radial + Ring.Dir * Slope).GetSafeNormal();
			const FVector T = FVector::CrossProduct(Ring.Dir, Radial);
			Cur[k] = Mesh.Vertex(P, N, T, FVector2D(Round * k / Sides, V), Ring.Wind, Ring.Colour, Ring.Occlusion);
		}
		if (i > 0)
		{
			for (int32 k = 0; k < Sides; ++k)
			{
				Mesh.Triangle(Prev[k], Prev[k + 1], Cur[k + 1]);
				Mesh.Triangle(Prev[k], Cur[k + 1], Cur[k]);
			}
		}
		Swap(Prev, Cur);
	}
	if (bCapEnd)
	{
		const FNatureTubeRing& Last = Rings.Last();
		const int32 Tip = Mesh.Vertex(Last.Centre + Last.Dir * Last.Radius * 0.9, Last.Dir, U, FVector2D(0, V + Last.Radius),
									  Last.Wind, Last.Colour, Last.Occlusion);
		for (int32 k = 0; k < Sides; ++k) { Mesh.Triangle(Prev[k], Prev[k + 1], Tip); }
	}
}

void MuseeNature::AddCard(FNatureMesh& Mesh, const FVector& Base, const FVector& Dir, const FVector& Normal, double Length, double Width,
						  double Fold, double Curl, int32 Rows, const FNatureWind& WindBase, const FNatureWind& WindTip,
						  const FLinearColor& Colour, double Occlusion, const FVector& ShadeBias)
{
	const FVector Side = FVector::CrossProduct(Normal, Dir).GetSafeNormal();
	const int32 NumRows = FMath::Clamp(Rows, 1, 4);
	int32 PrevL = INDEX_NONE, PrevR = INDEX_NONE;
	for (int32 r = 0; r <= NumRows; ++r)
	{
		const double S = double(r) / NumRows;
		// The centreline arches: the tip drops below the card's plane by Curl × Length.
		const FVector Centre = Base + Dir * (Length * S) - Normal * (Curl * Length * S * S);
		const FVector N = (Normal + Dir * (2.0 * Curl * S) + ShadeBias).GetSafeNormal(UE_SMALL_NUMBER, Normal);
		const FNatureWind Wind = FNatureWind::Lerp(WindBase, WindTip, S);
		const FVector NL = (N + Side * Fold).GetSafeNormal();
		const FVector NR = (N - Side * Fold).GetSafeNormal();
		const int32 L = Mesh.Vertex(Centre - Side * (0.5 * Width), NL, Side, FVector2D(0, S), Wind, Colour, Occlusion);
		const int32 R = Mesh.Vertex(Centre + Side * (0.5 * Width), NR, Side, FVector2D(1, S), Wind, Colour, Occlusion);
		if (r > 0) { Mesh.Quad(PrevL, PrevR, R, L); }
		PrevL = L;
		PrevR = R;
	}
}

void MuseeNature::AddFlowerCard(FNatureMesh& Mesh, const FVector& Centre, const FVector& Normal, double Spin, double Size, double Cup,
								const FNatureWind& Wind, const FLinearColor& Colour, double Occlusion)
{
	FVector U, W;
	Basis(Normal, U, W);
	const FVector A = U * FMath::Cos(Spin) + W * FMath::Sin(Spin);
	const FVector B = FVector::CrossProduct(Normal, A).GetSafeNormal();
	const double H = 0.5 * Size;
	// Five vertices: the centre a little lower than the corners, so the flower is cupped.
	FNatureWind CentreWind = Wind;
	CentreWind.Flutter *= 0.3;
	const int32 C = Mesh.Vertex(Centre - Normal * (Cup * H), Normal, A, FVector2D(0.5, 0.5), CentreWind, Colour, Occlusion);
	int32 Corner[4];
	const double Signs[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
	for (int32 k = 0; k < 4; ++k)
	{
		const FVector Offset = A * (Signs[k][0] * H) + B * (Signs[k][1] * H);
		const FVector N = (Normal + Offset.GetSafeNormal() * Cup).GetSafeNormal();
		Corner[k] = Mesh.Vertex(Centre + Offset, N, A, FVector2D(0.5 + 0.5 * Signs[k][0], 0.5 + 0.5 * Signs[k][1]), Wind, Colour, Occlusion);
	}
	for (int32 k = 0; k < 4; ++k) { Mesh.Triangle(C, Corner[k], Corner[(k + 1) % 4]); }
}

void MuseeNature::AddBlade(FNatureMesh& Mesh, const FVector& Base, const FVector& Dir, const FVector& Normal, const FNatureBlade& Blade,
						   const FNatureWind& Wind0)
{
	const int32 NX = FMath::Max(1, Blade.NX), NY = FMath::Max(1, Blade.NY);
	const double Step = Blade.Length / NY;
	FVector D = Dir.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	FVector N = (Normal - D * FVector::DotProduct(Normal, D)).GetSafeNormal();
	if (N.IsNearlyZero()) { FVector U, W; Basis(D, U, W); N = W; }
	TArray<FVector> Line, Norms, Sides;
	FVector P = Base;
	for (int32 j = 0; j <= NY; ++j)
	{
		const double S = double(j) / NY;
		Line.Add(P);
		Norms.Add(N);
		Sides.Add(FVector::CrossProduct(N, D).GetSafeNormal());
		// Curl towards the upper side, droop towards the ground, twist about the blade.
		const FVector CurlAxis = FVector::CrossProduct(D, N);
		const double CurlStep = FMath::DegreesToRadians(Blade.Curl / NY);
		D = RotateAbout(D, CurlAxis, CurlStep);
		N = RotateAbout(N, CurlAxis, CurlStep);
		if (Blade.Droop != 0)
		{
			const FVector Old = D;
			D = (D - FVector::UpVector * (Blade.Droop * Step * (0.3 + S))).GetSafeNormal(UE_SMALL_NUMBER, Old);
			N = FQuat::FindBetweenNormals(Old, D).RotateVector(N);
		}
		if (Blade.Twist != 0) { N = RotateAbout(N, D, FMath::DegreesToRadians(Blade.Twist / NY)); }
		N = (N - D * FVector::DotProduct(N, D)).GetSafeNormal(UE_SMALL_NUMBER, Norms.Last());
		P += D * Step;
	}
	const int32 Row = NX + 1;
	TArray<FVector> Pts;
	Pts.SetNum(Row * (NY + 1));
	for (int32 j = 0; j <= NY; ++j)
	{
		const double S = double(j) / NY;
		const double Half = 0.5 * Blade.Width * (FMath::Pow(FMath::Sin(UE_DOUBLE_PI * FMath::Pow(S, Blade.TipShape)), 0.6) + Blade.BaseWidth * FMath::Square(1.0 - S));
		for (int32 i = 0; i <= NX; ++i)
		{
			const double X = -1.0 + 2.0 * i / NX;
			Pts[j * Row + i] = Line[j] + Sides[j] * (X * Half) + Norms[j] * (Blade.Cup * Blade.Width * X * X);
		}
	}
	TArray<int32> Index;
	Index.SetNum(Pts.Num());
	for (int32 j = 0; j <= NY; ++j)
	{
		const double S = double(j) / NY;
		FNatureWind Wind = Wind0;
		Wind.Bend = Wind0.Bend + Blade.BendGain * S * S;
		Wind.Flutter = Blade.FlutterGain * S;
		const FLinearColor Colour = Mix(Blade.Root, Blade.Tip, FMath::Pow(S, 1.3));
		for (int32 i = 0; i <= NX; ++i)
		{
			const FVector DX = Pts[j * Row + FMath::Min(i + 1, NX)] - Pts[j * Row + FMath::Max(i - 1, 0)];
			const FVector DY = Pts[FMath::Min(j + 1, NY) * Row + i] - Pts[FMath::Max(j - 1, 0) * Row + i];
			FVector VN = FVector::CrossProduct(DX, DY).GetSafeNormal(UE_SMALL_NUMBER, Norms[j]);
			if (FVector::DotProduct(VN, Norms[j]) < 0) { VN = -VN; }
			Wind.Height = Pts[j * Row + i].Z;
			Index[j * Row + i] = Mesh.Vertex(Pts[j * Row + i], VN, Sides[j], FVector2D(double(i) / NX, S), Wind, Colour, 0.7 + 0.3 * S);
		}
	}
	for (int32 j = 0; j < NY; ++j)
	{
		for (int32 i = 0; i < NX; ++i)
		{
			Mesh.Quad(Index[j * Row + i], Index[j * Row + i + 1], Index[(j + 1) * Row + i + 1], Index[(j + 1) * Row + i]);
		}
	}
}

void MuseeNature::AddEllipsoid(FNatureMesh& Mesh, const FVector& Centre, const FVector& Axis, double Radius, double HalfLength,
							   int32 Segments, int32 Rings, const FNatureWind& Wind, const FLinearColor& Colour, double Occlusion)
{
	FVector U, W;
	const FVector Z = Axis.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	Basis(Z, U, W);
	TArray<int32> Grid;
	Grid.SetNum((Segments + 1) * (Rings + 1));
	for (int32 j = 0; j <= Rings; ++j)
	{
		const double Phi = UE_DOUBLE_PI * j / Rings;   // 0 at the top
		for (int32 i = 0; i <= Segments; ++i)
		{
			const double Theta = Tau * i / Segments;
			const FVector Radial = U * FMath::Cos(Theta) + W * FMath::Sin(Theta);
			const FVector P = Centre + Radial * (Radius * FMath::Sin(Phi)) + Z * (HalfLength * FMath::Cos(Phi));
			const FVector N = (Radial * (FMath::Sin(Phi) / Radius) + Z * (FMath::Cos(Phi) / HalfLength)).GetSafeNormal(UE_SMALL_NUMBER, Z);
			const FVector T = FVector::CrossProduct(Z, Radial);
			Grid[j * (Segments + 1) + i] = Mesh.Vertex(P, N, T, FVector2D(double(i) / Segments, double(j) / Rings), Wind, Colour, Occlusion);
		}
	}
	for (int32 j = 0; j < Rings; ++j)
	{
		for (int32 i = 0; i < Segments; ++i)
		{
			const int32 A = Grid[j * (Segments + 1) + i], B = Grid[j * (Segments + 1) + i + 1];
			const int32 C = Grid[(j + 1) * (Segments + 1) + i + 1], D = Grid[(j + 1) * (Segments + 1) + i];
			if (j > 0) { Mesh.Triangle(A, B, C); }
			if (j + 1 < Rings) { Mesh.Triangle(A, C, D); }
		}
	}
}

void MuseeNature::AddLathe(FNatureMesh& Mesh, const FVector& Origin, const FVector& Axis, const TArray<FVector2D>& Profile, int32 Segments,
						   const FNatureWind& Wind, const TArray<FLinearColor>& Colours, double Occlusion, double USpan)
{
	const int32 NumPoints = Profile.Num();
	if (NumPoints < 2 || Segments < 3) { return; }
	FVector U, W;
	const FVector Z = Axis.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
	Basis(Z, U, W);
	// Each profile span gets its own vertices, so creases (a pot's rim) stay sharp.
	double Along = 0;
	for (int32 k = 0; k + 1 < NumPoints; ++k)
	{
		const FVector2D PA = Profile[k], PB = Profile[k + 1];
		const FVector2D D = PB - PA;
		const double SpanLength = D.Size();
		if (SpanLength < 1e-7) { continue; }
		const FVector2D N2 = FVector2D(D.Y, -D.X) / SpanLength;   // outward in (r, h) for a profile going up the outside
		const FLinearColor CA = Colours.Num() == NumPoints ? Colours[k] : (Colours.Num() > 0 ? Colours[0] : FLinearColor::White);
		const FLinearColor CB = Colours.Num() == NumPoints ? Colours[k + 1] : CA;
		int32 PrevA = INDEX_NONE, PrevB = INDEX_NONE;
		for (int32 i = 0; i <= Segments; ++i)
		{
			const double Theta = Tau * i / Segments;
			const FVector Radial = U * FMath::Cos(Theta) + W * FMath::Sin(Theta);
			const FVector N = Radial * N2.X + Z * N2.Y;
			const FVector T = FVector::CrossProduct(Z, Radial);
			const double UCoord = USpan * i / Segments;
			const int32 VA = Mesh.Vertex(Origin + Radial * PA.X + Z * PA.Y, N, T, FVector2D(UCoord, Along), Wind, CA, Occlusion);
			const int32 VB = Mesh.Vertex(Origin + Radial * PB.X + Z * PB.Y, N, T, FVector2D(UCoord, Along + SpanLength), Wind, CB, Occlusion);
			if (i > 0) { Mesh.Quad(PrevA, VA, VB, PrevB); }
			PrevA = VA;
			PrevB = VB;
		}
		Along += SpanLength;
	}
}
