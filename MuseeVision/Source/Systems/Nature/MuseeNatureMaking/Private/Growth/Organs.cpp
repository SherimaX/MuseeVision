#include "Growth/Organs.h"

namespace Musee::NatureMaking
{
	FLinearColor FromHex(const FString& Hex)
	{
		const FString Digits = Hex.StartsWith(TEXT("#")) ? Hex.RightChop(1) : Hex;
		return Digits.Len() == 6 ? FLinearColor::FromSRGBColor(FColor::FromHex(Digits)) : FLinearColor::Black;
	}

	FLinearColor Mix(const FLinearColor& A, const FLinearColor& B, double T)
	{
		const float F = static_cast<float>(FMath::Clamp(T, 0.0, 1.0));
		return A * (1.0f - F) + B * F;
	}

	FLinearColor Vary(const FLinearColor& Colour, FRandomStream& Random, double Hue, double Saturation, double Value)
	{
		FLinearColor HSV = Colour.LinearRGBToHSV();
		HSV.R = static_cast<float>(FMath::Fmod(double(HSV.R) + Random.FRandRange(-Hue, Hue) + 360.0, 360.0));
		HSV.G = static_cast<float>(FMath::Clamp(double(HSV.G) * (1.0 + Random.FRandRange(-Saturation, Saturation)), 0.0, 1.0));
		HSV.B = static_cast<float>(FMath::Max(0.0, double(HSV.B) * (1.0 + Random.FRandRange(-Value, Value))));
		FLinearColor Out = HSV.HSVToLinearRGB();
		Out.A = 1.0f;
		return Out;
	}

	FVector RotateAbout(const FVector& V, const FVector& Axis, double Angle)
	{
		const FVector K = Axis.GetSafeNormal();
		if (K.IsNearlyZero())
		{
			return V;
		}
		const double C = FMath::Cos(Angle);
		const double S = FMath::Sin(Angle);
		return V * C + FVector::CrossProduct(K, V) * S + K * FVector::DotProduct(K, V) * (1.0 - C);
	}

	void Basis(const FVector& Dir, FVector& OutU, FVector& OutW)
	{
		OutU = FVector::CrossProduct(Dir, FVector::UpVector);
		if (OutU.SizeSquared() < UE_KINDA_SMALL_NUMBER)
		{
			OutU = FVector::CrossProduct(Dir, FVector::ForwardVector);
		}
		OutU.Normalize();
		OutW = FVector::CrossProduct(Dir, OutU).GetSafeNormal();
	}

	void AddTube(FNatureSurface& Mesh, const TArray<FTubeRing>& Rings, int32 Sides, bool bCapEnd)
	{
		if (Rings.Num() < 2 || Sides < 3)
		{
			return;
		}
		FVector U, W;
		Basis(Rings[0].Dir, U, W);
		FVector PreviousDir = Rings[0].Dir;
		double V = 0.0;
		TArray<int32> Previous, Current;
		Previous.SetNum(Sides + 1);
		Current.SetNum(Sides + 1);
		for (int32 I = 0; I < Rings.Num(); ++I)
		{
			const FTubeRing& Ring = Rings[I];
			if (I > 0)
			{
				// Carry the frame along the tube (parallel transport), so it never twists.
				V += FVector::Dist(Rings[I - 1].Centre, Ring.Centre);
				U = FQuat::FindBetweenNormals(PreviousDir, Ring.Dir).RotateVector(U);
				U = (U - Ring.Dir * FVector::DotProduct(U, Ring.Dir)).GetSafeNormal();
				W = FVector::CrossProduct(Ring.Dir, U).GetSafeNormal();
				PreviousDir = Ring.Dir;
			}
			// The surface leans forward where the tube narrows.
			const FTubeRing& Before = Rings[FMath::Max(0, I - 1)];
			const FTubeRing& After = Rings[FMath::Min(Rings.Num() - 1, I + 1)];
			const double Run = FVector::Dist(Before.Centre, After.Centre);
			const double Slope = Run > UE_SMALL_NUMBER ? (Before.Radius - After.Radius) / Run : 0.0;
			const double Round = Tau * Ring.Radius;
			for (int32 K = 0; K <= Sides; ++K)
			{
				const double A = Tau * K / Sides;
				const FVector Radial = U * FMath::Cos(A) + W * FMath::Sin(A);
				Current[K] = Mesh.Vertex(Ring.Centre + Radial * Ring.Radius, Radial + Ring.Dir * Slope, FVector2D(Round * K / Sides, V), Ring.Tint);
			}
			if (I > 0)
			{
				for (int32 K = 0; K < Sides; ++K)
				{
					Mesh.Quad(Previous[K], Previous[K + 1], Current[K + 1], Current[K]);
				}
			}
			Swap(Previous, Current);
		}
		if (bCapEnd)
		{
			const FTubeRing& Last = Rings.Last();
			const int32 Tip = Mesh.Vertex(Last.Centre + Last.Dir * Last.Radius, Last.Dir, FVector2D(0.0, V + Last.Radius), Last.Tint);
			for (int32 K = 0; K < Sides; ++K)
			{
				Mesh.Triangle(Previous[K], Previous[K + 1], Tip);
			}
		}
	}

	void AddStalk(FNatureSurface& Mesh, const FVector& From, const FVector& To, double Radius, const FVector& Bow, const FLinearColor& Foot,
		const FLinearColor& Top)
	{
		constexpr int32 Segments = 8;
		constexpr int32 Sides = 6;
		TArray<FTubeRing> Rings;
		for (int32 S = 0; S <= Segments; ++S)
		{
			const double T = double(S) / Segments;
			Rings.Add({ FMath::Lerp(From, To, T) + Bow * FMath::Sin(UE_DOUBLE_PI * T), FVector::UpVector, Radius * (1.0 - 0.25 * T), Mix(Foot, Top, T) });
		}
		for (int32 S = 0; S <= Segments; ++S)
		{
			Rings[S].Dir = (Rings[FMath::Min(Segments, S + 1)].Centre - Rings[FMath::Max(0, S - 1)].Centre).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
		}
		AddTube(Mesh, Rings, Sides, true);
	}

	namespace
	{
		/** A grid of points (Across + 1 by Along + 1) as a smooth sheet, its normals facing the side each row's Up names. */
		void AddSheet(FNatureSurface& Mesh, const TArray<FVector>& Points, const TArray<FVector>& RowUp, int32 Across, int32 Along,
			TFunctionRef<FVector2D(int32 I, int32 J)> UV, TFunctionRef<FLinearColor(int32 I, int32 J)> Tint, TFunctionRef<double(int32 J)> Occlusion)
		{
			const int32 Row = Across + 1;
			TArray<int32> Index;
			Index.SetNum(Points.Num());
			for (int32 J = 0; J <= Along; ++J)
			{
				for (int32 I = 0; I <= Across; ++I)
				{
					const FVector DX = Points[J * Row + FMath::Min(I + 1, Across)] - Points[J * Row + FMath::Max(I - 1, 0)];
					const FVector DY = Points[FMath::Min(J + 1, Along) * Row + I] - Points[FMath::Max(J - 1, 0) * Row + I];
					FVector N = FVector::CrossProduct(DX, DY).GetSafeNormal(UE_SMALL_NUMBER, RowUp[J]);
					N = FVector::DotProduct(N, RowUp[J]) < 0.0 ? -N : N;
					Index[J * Row + I] = Mesh.Vertex(Points[J * Row + I], N, UV(I, J), Tint(I, J), Occlusion(J));
				}
			}
			for (int32 J = 0; J < Along; ++J)
			{
				for (int32 I = 0; I < Across; ++I)
				{
					Mesh.Quad(Index[J * Row + I], Index[J * Row + I + 1], Index[(J + 1) * Row + I + 1], Index[(J + 1) * Row + I]);
				}
			}
		}

		/** The half width of a blade or petal at share S of its length. */
		double HalfWidth(double Width, double S, double TipShape, double BaseWidth)
		{
			return 0.5 * Width * (FMath::Pow(FMath::Sin(UE_DOUBLE_PI * FMath::Pow(S, TipShape)), 0.6) + BaseWidth * FMath::Square(1.0 - S));
		}
	}

	void AddBlade(FNatureSurface& Mesh, const FVector& Base, const FVector& Dir, const FVector& Normal, const FBlade& Blade)
	{
		const int32 Across = FMath::Max(1, Blade.Across);
		const int32 Along = FMath::Max(1, Blade.Along);
		const double Step = Blade.Length / Along;
		FVector D = Dir.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
		FVector N = (Normal - D * FVector::DotProduct(Normal, D)).GetSafeNormal();
		if (N.IsNearlyZero())
		{
			FVector U;
			Basis(D, U, N);
		}
		TArray<FVector> Line, Ups, Sides;
		FVector P = Base;
		for (int32 J = 0; J <= Along; ++J)
		{
			const double S = double(J) / Along;
			Line.Add(P);
			Ups.Add(N);
			Sides.Add(FVector::CrossProduct(N, D).GetSafeNormal());
			// Curl towards the upper side, droop towards the ground, twist about the blade.
			const FVector CurlAxis = FVector::CrossProduct(D, N);
			const double CurlStep = FMath::DegreesToRadians(Blade.Curl / Along);
			D = RotateAbout(D, CurlAxis, CurlStep);
			N = RotateAbout(N, CurlAxis, CurlStep);
			if (Blade.Droop != 0.0)
			{
				const FVector Old = D;
				D = (D - FVector::UpVector * (Blade.Droop * Step * (0.3 + S))).GetSafeNormal(UE_SMALL_NUMBER, Old);
				N = FQuat::FindBetweenNormals(Old, D).RotateVector(N);
			}
			N = RotateAbout(N, D, FMath::DegreesToRadians(Blade.Twist / Along));
			N = (N - D * FVector::DotProduct(N, D)).GetSafeNormal(UE_SMALL_NUMBER, Ups.Last());
			P += D * Step;
		}
		const int32 Row = Across + 1;
		TArray<FVector> Points;
		TArray<double> Halves;
		Points.SetNum(Row * (Along + 1));
		for (int32 J = 0; J <= Along; ++J)
		{
			Halves.Add(HalfWidth(Blade.Width, double(J) / Along, Blade.TipShape, Blade.BaseWidth));
			for (int32 I = 0; I <= Across; ++I)
			{
				const double X = -1.0 + 2.0 * I / Across;
				Points[J * Row + I] = Line[J] + Sides[J] * (X * Halves[J]) + Ups[J] * (Blade.Cup * Blade.Width * X * X);
			}
		}
		AddSheet(Mesh, Points, Ups, Across, Along,
			[&](int32 I, int32 J) { return FVector2D((-1.0 + 2.0 * I / Across) * Halves[J], Blade.Length * J / Along); },
			[&](int32, int32 J) { return Mix(Blade.Root, Blade.Tip, FMath::Pow(double(J) / Along, 1.3)); },
			[&](int32 J) { return 0.7 + 0.3 * J / Along; });
	}

	void AddPetal(FNatureSurface& Mesh, const FVector& Base, const FVector& Radial, const FPetal& Petal, TFunctionRef<FLinearColor(double X, double S)> Tint)
	{
		const FVector Up = FVector::UpVector;
		const FVector Side = FVector::CrossProduct(Up, Radial).GetSafeNormal(UE_SMALL_NUMBER, FVector::RightVector);
		const int32 Across = FMath::Max(1, Petal.Across);
		const int32 Along = FMath::Max(1, Petal.Along);
		TArray<FVector> Line, Planes;
		FVector P = Base;
		for (int32 J = 0; J <= Along; ++J)
		{
			const double S = double(J) / Along;
			const double E = FMath::DegreesToRadians(Petal.Elevation + Petal.Curl * FMath::Pow(S, Petal.CurlPower));
			Line.Add(P);
			Planes.Add(-Radial * FMath::Sin(E) + Up * FMath::Cos(E));
			P += (Radial * FMath::Cos(E) + Up * FMath::Sin(E)) * (Petal.Length / Along);
		}
		const int32 Row = Across + 1;
		TArray<FVector> Points;
		TArray<double> Halves;
		Points.SetNum(Row * (Along + 1));
		for (int32 J = 0; J <= Along; ++J)
		{
			const double S = double(J) / Along;
			Halves.Add(HalfWidth(Petal.Width, S, Petal.TipShape, Petal.BaseWidth));
			for (int32 I = 0; I <= Across; ++I)
			{
				const double X = -1.0 + 2.0 * I / Across;
				const double Wave = Petal.Ruffle * Petal.Width * FMath::Sin(9.0 * X + 5.0 * S) * X * X * S;
				Points[J * Row + I] = Line[J] + Side * (X * Halves[J]) + Planes[J] * (Petal.Cup * Petal.Width * X * X + Wave);
			}
		}
		AddSheet(Mesh, Points, Planes, Across, Along,
			[&](int32 I, int32 J) { return FVector2D((-1.0 + 2.0 * I / Across) * Halves[J], Petal.Length * J / Along); },
			[&](int32 I, int32 J) { return Tint(-1.0 + 2.0 * I / Across, double(J) / Along); },
			[&](int32 J) { return 0.75 + 0.25 * J / Along; });
	}

	void AddCard(FNatureSurface& Mesh, const FVector& Base, const FVector& Dir, const FVector& Normal, double Length, double Width,
		double Fold, double Curl, int32 Rows, const FLinearColor& Tint, double Occlusion)
	{
		const FVector Side = FVector::CrossProduct(Normal, Dir).GetSafeNormal();
		const int32 NumRows = FMath::Max(1, Rows);
		int32 PreviousL = INDEX_NONE;
		int32 PreviousR = INDEX_NONE;
		for (int32 R = 0; R <= NumRows; ++R)
		{
			const double S = double(R) / NumRows;
			// The midrib arches: the tip drops below the card's plane by Curl times its length.
			const FVector Centre = Base + Dir * (Length * S) - Normal * (Curl * Length * S * S);
			const FVector N = (Normal + Dir * (2.0 * Curl * S)).GetSafeNormal(UE_SMALL_NUMBER, Normal);
			const int32 L = Mesh.Vertex(Centre - Side * (0.5 * Width), (N + Side * Fold).GetSafeNormal(), FVector2D(-0.5 * Width, Length * S), Tint, Occlusion);
			const int32 Rt = Mesh.Vertex(Centre + Side * (0.5 * Width), (N - Side * Fold).GetSafeNormal(), FVector2D(0.5 * Width, Length * S), Tint, Occlusion);
			if (R > 0)
			{
				Mesh.Quad(PreviousL, PreviousR, Rt, L);
			}
			PreviousL = L;
			PreviousR = Rt;
		}
	}

	void AddFlowerCard(FNatureSurface& Mesh, const FVector& Centre, const FVector& Normal, double Spin, double Size, double Cup,
		const FLinearColor& Tint, double Occlusion)
	{
		FVector U, W;
		Basis(Normal, U, W);
		const FVector A = U * FMath::Cos(Spin) + W * FMath::Sin(Spin);
		const FVector B = FVector::CrossProduct(Normal, A).GetSafeNormal();
		const double H = 0.5 * Size;
		const int32 Middle = Mesh.Vertex(Centre - Normal * (Cup * H), Normal, FVector2D::ZeroVector, Tint, Occlusion);
		const double Signs[4][2] = { { -1, -1 }, { 1, -1 }, { 1, 1 }, { -1, 1 } };
		int32 Corner[4];
		for (int32 K = 0; K < 4; ++K)
		{
			const FVector Offset = A * (Signs[K][0] * H) + B * (Signs[K][1] * H);
			Corner[K] = Mesh.Vertex(Centre + Offset, (Normal + Offset.GetSafeNormal() * Cup).GetSafeNormal(), FVector2D(Signs[K][0] * H, Signs[K][1] * H), Tint, Occlusion);
		}
		for (int32 K = 0; K < 4; ++K)
		{
			Mesh.Triangle(Middle, Corner[K], Corner[(K + 1) % 4]);
		}
	}

	void AddEllipsoid(FNatureSurface& Mesh, const FVector& Centre, const FVector& Axis, double Radius, double HalfLength, int32 Segments,
		int32 Rings, const FLinearColor& Tint, double Occlusion)
	{
		FVector U, W;
		const FVector Z = Axis.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
		Basis(Z, U, W);
		TArray<int32> Grid;
		Grid.SetNum((Segments + 1) * (Rings + 1));
		for (int32 J = 0; J <= Rings; ++J)
		{
			const double Phi = UE_DOUBLE_PI * J / Rings;
			for (int32 I = 0; I <= Segments; ++I)
			{
				const double Theta = Tau * I / Segments;
				const FVector Radial = U * FMath::Cos(Theta) + W * FMath::Sin(Theta);
				const FVector P = Centre + Radial * (Radius * FMath::Sin(Phi)) + Z * (HalfLength * FMath::Cos(Phi));
				const FVector N = Radial * (FMath::Sin(Phi) / Radius) + Z * (FMath::Cos(Phi) / HalfLength);
				Grid[J * (Segments + 1) + I] = Mesh.Vertex(P, N, FVector2D(Theta * Radius, Phi * HalfLength), Tint, Occlusion);
			}
		}
		for (int32 J = 0; J < Rings; ++J)
		{
			for (int32 I = 0; I < Segments; ++I)
			{
				const int32 A = Grid[J * (Segments + 1) + I];
				const int32 B = Grid[J * (Segments + 1) + I + 1];
				const int32 C = Grid[(J + 1) * (Segments + 1) + I + 1];
				const int32 D = Grid[(J + 1) * (Segments + 1) + I];
				Mesh.Triangle(A, B, C);
				Mesh.Triangle(A, C, D);
			}
		}
	}

	void AddLathe(FNatureSurface& Mesh, const FVector& Origin, const FVector& Axis, const TArray<FVector2D>& Profile, int32 Segments,
		const TArray<FLinearColor>& Tints, double Occlusion)
	{
		if (Profile.Num() < 2 || Segments < 3)
		{
			return;
		}
		FVector U, W;
		const FVector Z = Axis.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
		Basis(Z, U, W);
		double Along = 0.0;
		// Each span of the profile has its own vertices, so its corners stay crisp.
		for (int32 K = 0; K + 1 < Profile.Num(); ++K)
		{
			const FVector2D PA = Profile[K];
			const FVector2D PB = Profile[K + 1];
			const FVector2D D = PB - PA;
			const double Span = D.Size();
			if (Span < UE_KINDA_SMALL_NUMBER)
			{
				continue;
			}
			const FVector2D Out(D.Y / Span, -D.X / Span);
			const FLinearColor TA = Tints.Num() == Profile.Num() ? Tints[K] : (Tints.Num() > 0 ? Tints[0] : FLinearColor::White);
			const FLinearColor TB = Tints.Num() == Profile.Num() ? Tints[K + 1] : TA;
			const double Round = Tau * FMath::Max(PA.X, PB.X);
			int32 PreviousA = INDEX_NONE;
			int32 PreviousB = INDEX_NONE;
			for (int32 I = 0; I <= Segments; ++I)
			{
				const double Theta = Tau * I / Segments;
				const FVector Radial = U * FMath::Cos(Theta) + W * FMath::Sin(Theta);
				const FVector N = Radial * Out.X + Z * Out.Y;
				const double UCoord = Round * I / Segments;
				const int32 VA = Mesh.Vertex(Origin + Radial * PA.X + Z * PA.Y, N, FVector2D(UCoord, Along), TA, Occlusion);
				const int32 VB = Mesh.Vertex(Origin + Radial * PB.X + Z * PB.Y, N, FVector2D(UCoord, Along + Span), TB, Occlusion);
				if (I > 0)
				{
					Mesh.Quad(PreviousA, VA, VB, PreviousB);
				}
				PreviousA = VA;
				PreviousB = VB;
			}
			Along += Span;
		}
	}
}
