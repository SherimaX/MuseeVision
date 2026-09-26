#include "Albion/AlbionWork.h"

#include "Albion/AlbionKit.h"
#include "Engine/CollisionProfile.h"
#include "Geometry/MuseeBake.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

/** The work's geometry in its own frame (metres): X out of the wall, Y along the width, Z up; the origin on the wall. */
namespace AlbionWorkImpl
{
	using namespace AlbionKit;
	const FVector kX(1, 0, 0), kY(0, 1, 0), kZ(0, 0, 1);

	/** The canvas's face: the image upright to a viewer facing the wall (its left edge at +Y). */
	void Face(FMeshData& M, double W, double H, double X, int32 NY = 1, int32 NZ = 1, TFunctionRef<double(double, double)> Bulge = [](double, double) { return 0.0; })
	{
		const int32 Base = M.Positions.Num();
		for (int32 i = 0; i <= NZ; ++i)
		{
			for (int32 j = 0; j <= NY; ++j)
			{
				const double U = double(j) / NY, V = double(i) / NZ;
				const double Y = 0.5 * W - U * W, Z = 0.5 * H - V * H;
				M.Vertex(FVector(X + Bulge(Y, Z), Y, Z), kX, FVector2D(U, V));
			}
		}
		for (int32 i = 0; i < NZ; ++i)
		{
			for (int32 j = 0; j < NY; ++j)
			{
				const int32 A = Base + i * (NY + 1) + j;
				M.Quad(A, A + 1, A + NY + 2, A + NY + 1);
			}
		}
		// Normals from the surface when it bulges.
		if (NY > 1 || NZ > 1)
		{
			for (int32 i = 0; i <= NZ; ++i)
			{
				for (int32 j = 0; j <= NY; ++j)
				{
					const int32 K = Base + i * (NY + 1) + j;
					const FVector DY = M.Positions[Base + i * (NY + 1) + FMath::Min(j + 1, NY)] - M.Positions[Base + i * (NY + 1) + FMath::Max(j - 1, 0)];
					const FVector DZ = M.Positions[Base + FMath::Min(i + 1, NZ) * (NY + 1) + j] - M.Positions[Base + FMath::Max(i - 1, 0) * (NY + 1) + j];
					FVector N = FVector::CrossProduct(DZ, DY).GetSafeNormal();
					if (FVector::DotProduct(N, kX) < 0) { N = -N; }
					M.Normals[K] = N;
				}
			}
		}
	}

	/** The four tacking edges from the face back to the wall, and the back. */
	void Box(FMeshData& M, double W, double H, double X0, double X1)
	{
		M.Box(FVector(X0, -0.5 * W, -0.5 * H), FVector(X1, 0.5 * W, 0.5 * H), FMeshData::NegX | FMeshData::PosY | FMeshData::NegY | FMeshData::PosZ | FMeshData::NegZ);
	}

	/** The sight's outline (a closed loop, anticlockwise seen from the room), N points. */
	TArray<FVector2D> SightLoop(EAlbionSight S, double W, double H, int32 N)
	{
		TArray<FVector2D> Out;   // (y, z)
		if (S == EAlbionSight::Oval)
		{
			for (int32 i = 0; i < N; ++i)
			{
				const double A = 2.0 * UE_DOUBLE_PI * i / N;
				Out.Add(FVector2D(0.5 * W * FMath::Cos(A), 0.5 * H * FMath::Sin(A)));
			}
			return Out;
		}
		// Arched: the sides straight up to a semicircular head of the full width.
		const double R = 0.5 * W, Spring = 0.5 * H - R;
		const int32 NA = N / 2;
		Out.Add(FVector2D(R, -0.5 * H));
		Out.Add(FVector2D(R, Spring));
		for (int32 i = 1; i < NA; ++i)
		{
			const double A = UE_DOUBLE_PI * i / NA;
			Out.Add(FVector2D(R * FMath::Cos(A), Spring + R * FMath::Sin(A)));
		}
		Out.Add(FVector2D(-R, Spring));
		Out.Add(FVector2D(-R, -0.5 * H));
		return Out;
	}

	/** Where the ray from the rectangle's middle through P meets its edge. */
	FVector2D ToRect(const FVector2D& P, double HW, double HH)
	{
		const double SX = FMath::Abs(P.X) > 1e-9 ? HW / FMath::Abs(P.X) : 1e9;
		const double SZ = FMath::Abs(P.Y) > 1e-9 ? HH / FMath::Abs(P.Y) : 1e9;
		return P * FMath::Min(SX, SZ);
	}

	/** The gilt slip over an arched or oval sight: a plate from the sight to the rectangle's edge, its opening chamfered. */
	void Slip(FMeshData& M, EAlbionSight S, double W, double H, double X)
	{
		const TArray<FVector2D> Loop = SightLoop(S, W, H, 96);
		const int32 N = Loop.Num();
		const double T = 0.004, Ch = 0.006;
		for (int32 i = 0; i < N; ++i)
		{
			const FVector2D P0 = Loop[i], P1 = Loop[(i + 1) % N];
			const FVector2D Q0 = ToRect(P0, 0.5 * W, 0.5 * H), Q1 = ToRect(P1, 0.5 * W, 0.5 * H);
			if (FVector2D::Distance(P0, Q0) < 1e-5 && FVector2D::Distance(P1, Q1) < 1e-5) { continue; }
			// The face (a chamfer's width in from the opening) and the chamfer down to the canvas.
			const FVector2D I0 = P0 + (Q0 - P0).GetSafeNormal() * FMath::Min(Ch, FVector2D::Distance(P0, Q0)), I1 = P1 + (Q1 - P1).GetSafeNormal() * FMath::Min(Ch, FVector2D::Distance(P1, Q1));
			M.Poly({FVector(X + T, I0.X, I0.Y), FVector(X + T, I1.X, I1.Y), FVector(X + T, Q1.X, Q1.Y), FVector(X + T, Q0.X, Q0.Y)}, kX);
			const FVector2D Edge = (P1 - P0).GetSafeNormal();
			const FVector InPlane = FVector(0.0, -Edge.Y, Edge.X);   // towards the sight's middle
			M.Poly({FVector(X, P0.X, P0.Y), FVector(X, P1.X, P1.Y), FVector(X + T, I1.X, I1.Y), FVector(X + T, I0.X, I0.Y)}, (kX + InPlane).GetSafeNormal());
		}
	}

	/** A lathe about Z from a profile of (r, z) points, the image mapped as a view from the side (Vase) or above (Dish). */
	void Turned(FMeshData& M, const TArray<FVector2D>& Profile, bool bTopView, double R, double H, int32 Seg, const FVector4& Win = FVector4(0.0, 1.0, 0.0, 1.0))
	{
		const int32 NP = Profile.Num();
		const int32 Base = M.Positions.Num();
		for (int32 i = 0; i < NP; ++i)
		{
			for (int32 k = 0; k <= Seg; ++k)
			{
				const double A = 2.0 * UE_DOUBLE_PI * k / Seg;
				const FVector P(Profile[i].X * FMath::Cos(A), Profile[i].X * FMath::Sin(A), Profile[i].Y);
				// Normal from the profile's slope.
				const FVector2D D = Profile[FMath::Min(i + 1, NP - 1)] - Profile[FMath::Max(i - 1, 0)];
				const FVector2D N2 = FVector2D(D.Y, -D.X).GetSafeNormal();
				const FVector N(N2.X * FMath::Cos(A), N2.X * FMath::Sin(A), N2.Y);
				FVector2D UV;
				if (bTopView) { UV = FVector2D(0.5 - 0.5 * P.Y / R, 0.5 - 0.5 * P.X / R); }
				else { UV = FVector2D(0.5 - 0.5 * P.Y / FMath::Max(R, 1e-4), 1.0 - P.Z / H); }
				UV = FVector2D(FMath::Lerp(Win.X, Win.Y, UV.X), FMath::Lerp(Win.Z, Win.W, UV.Y));   // the object's box in its photograph
				M.Vertex(P, N, UV);
			}
		}
		for (int32 i = 0; i + 1 < NP; ++i)
		{
			for (int32 k = 0; k < Seg; ++k)
			{
				const int32 A = Base + i * (Seg + 1) + k;
				M.Quad(A, A + 1, A + Seg + 2, A + Seg + 1);
			}
		}
	}
}

namespace AlbionWorkImpl
{
	/** A vase's handle: a round strap (T thick, 0.8 T deep) swept along Path in the photograph's plane (Y, Z; metres), its
	 *  face mapped from the photograph as the body is (so the handle wears its own pixels). Side −1 mirrors it. */
	void Handle(FMeshData& M, const TArray<FVector2D>& Path, double T, double R, double H, double Side, const FVector4& Win)
	{
		const int32 N = Path.Num(), Seg = 12;
		if (N < 2) { return; }
		const int32 Base = M.Positions.Num();
		for (int32 i = 0; i < N; ++i)
		{
			const FVector2D D = (Path[FMath::Min(i + 1, N - 1)] - Path[FMath::Max(i - 1, 0)]).GetSafeNormal();
			const FVector Tan(0.0, Side * D.X, D.Y);
			const FVector In(0.0, Side * D.Y, -D.X);                  // in the plane, across the strap
			const FVector C(0.0, Side * Path[i].X, Path[i].Y);
			for (int32 k = 0; k <= Seg; ++k)
			{
				const double A = 2.0 * UE_DOUBLE_PI * (k % Seg) / Seg;
				const FVector N3 = (kX * FMath::Cos(A) + In * FMath::Sin(A)).GetSafeNormal();
				const FVector P = C + kX * (0.4 * T * FMath::Cos(A)) + In * (0.5 * T * FMath::Sin(A));
				const FVector2D UV(FMath::Lerp(Win.X, Win.Y, 0.5 - 0.5 * P.Y / R), FMath::Lerp(Win.Z, Win.W, 1.0 - P.Z / H));
				M.Vertex(P, N3, UV);
			}
			(void)Tan;
		}
		for (int32 i = 0; i + 1 < N; ++i)
		{
			for (int32 k = 0; k < Seg; ++k)
			{
				const int32 A = Base + i * (Seg + 1) + k;
				M.Quad(A, A + 1, A + Seg + 2, A + Seg + 1);
			}
		}
	}
}

AAlbionWork::AAlbionWork()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetMobility(EComponentMobility::Static);
	Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	auto Soft = [](const TCHAR* P) { return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(P)); };
	LinenMaterial = Soft(TEXT("/Game/Museum/Materials/M_Frame_Linen.M_Frame_Linen"));
	GiltMaterial = Soft(TEXT("/Game/Museum/Materials/M_Gilt_Aged.M_Gilt_Aged"));
	OakMaterial = Soft(TEXT("/Game/Museum/Materials/Albion/MI_Albion_Oak.MI_Albion_Oak"));   // the museum's oak, Victorian finish
	BronzeMaterial = Soft(TEXT("/Game/Museum/Materials/M_Bronze_Brushed.M_Bronze_Brushed"));
	Tags.AddUnique(MuseeBake::BakeableTag());
}

void AAlbionWork::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
}

void AAlbionWork::BeginPlay()
{
	Super::BeginPlay();
	Build();
}

void AAlbionWork::Rebuild()
{
	Build();
}

void AAlbionWork::Build()
{
	using namespace AlbionWorkImpl;
	if (MuseeBake::IsBaked(this)) { return; }
	FMeshData Face_, Linen, Gilt, Oak, Bronze;
	const double W = Width, H = Height;
	switch (Kind)
	{
	case EAlbionWorkKind::Canvas:
	{
		Face(Face_, W, H, CanvasOffset);
		Box(Linen, W, H, 0.004, CanvasOffset - 0.0005);
		if (Sight != EAlbionSight::Rect) { Slip(Gilt, Sight, W, H, CanvasOffset); }
		break;
	}
	case EAlbionWorkKind::Tapestry:
	{
		// Wool and silk on a cotton warp hangs nearly flat: shallow folds from the batten, a little fuller at the foot.
		const double X = 0.07;
		auto Folds = [H](double Y, double Z)
		{
			const double Low = 0.35 + 0.65 * (0.5 - Z / H);
			return Low * (0.012 * FMath::Sin(Y / 0.47 * 2.0 * UE_DOUBLE_PI) + 0.006 * FMath::Sin(Y / 0.19 * 2.0 * UE_DOUBLE_PI + 1.3));
		};
		Face(Face_, W, H, X, FMath::Max(8, int32(W / 0.04)), FMath::Max(4, int32(H / 0.12)), Folds);
		// The lining behind, a hem at the foot, the batten and its brackets.
		FMeshData Back;
		Face(Back, W, H, X - 0.004, FMath::Max(8, int32(W / 0.04)), FMath::Max(4, int32(H / 0.12)), Folds);
		for (FVector& N : Back.Normals) { N = -N; }
		Linen.Positions.Append(Back.Positions);
		const int32 Off = Linen.Normals.Num();
		Linen.Normals.Append(Back.Normals);
		Linen.UVs.Append(Back.UVs);
		for (int32 i = 0; i < Back.Indices.Num(); i += 3) { Linen.Tri(Off + Back.Indices[i], Off + Back.Indices[i + 2], Off + Back.Indices[i + 1]); }
		FurnitureKit::Bar(Oak, FurnitureKit::FPath::Line(FVector(X - 0.02, 0.5 * W + 0.06, 0.5 * H + 0.03), FVector(X - 0.02, -0.5 * W - 0.06, 0.5 * H + 0.03), kZ),
						  FurnitureKit::RectSection(0.0, 0.0, 0.035, 0.03, 0.006));
		for (int32 s = -1; s <= 1; s += 2)
		{
			const double YB = s * (0.5 * W - 0.3);
			Bronze.Box(FVector(0.0, YB - 0.02, 0.5 * H + 0.0), FVector(X - 0.04, YB + 0.02, 0.5 * H + 0.012), FMeshData::AllFaces);
			Bronze.Box(FVector(0.0, YB - 0.03, 0.5 * H - 0.06), FVector(0.01, YB + 0.03, 0.5 * H + 0.06), FMeshData::AllFaces);
		}
		break;
	}
	case EAlbionWorkKind::Wallpaper:
	{
		// The paper pasted on a panel: the image tiled at its repeat.
		const int32 Base = Face_.Positions.Num();
		Face(Face_, W, H, 0.024);
		for (int32 i = Base; i < Face_.UVs.Num(); ++i)
		{
			Face_.UVs[i] = FVector2D(Face_.UVs[i].X * W / PatternRepeat.Y, Face_.UVs[i].Y * H / PatternRepeat.X);
		}
		// (the board 6 mm behind the paper: nearer, a Nanite bake quantises the two into one another)
		Oak.Box(FVector(0.004, -0.5 * W - 0.004, -0.5 * H - 0.004), FVector(0.018, 0.5 * W + 0.004, 0.5 * H + 0.004), FMeshData::AllFaces & ~FMeshData::NegX);
		break;
	}
	case EAlbionWorkKind::Dish:
	{
		// A shallow dish lying on the felt: a foot ring, the well, a broad rim (R the rim's radius, H its height).
		const double R = 0.5 * W, D = FMath::Max(0.02, double(H));
		// (Turned's normal is the profile's right: the face runs from the rim in to the centre, so it faces up; the
		// underside from the centre out to the rim, so it faces down.)
		const TArray<FVector2D> Top = {FVector2D(R, 0.96 * D), FVector2D(0.97 * R, D), FVector2D(0.80 * R, 0.85 * D), FVector2D(0.72 * R, 0.55 * D),
									   FVector2D(0.55 * R, 0.24 * D), FVector2D(0.0, 0.18 * D)};
		Turned(Face_, Top, true, R, D, 96, ImageWindow);
		const TArray<FVector2D> Under = {FVector2D(0.0, 0.12 * D), FVector2D(0.40 * R, 0.1 * D), FVector2D(0.42 * R, 0.0), FVector2D(0.50 * R, 0.0),
										 FVector2D(0.52 * R, 0.1 * D), FVector2D(0.78 * R, 0.62 * D), FVector2D(0.98 * R, 0.88 * D), FVector2D(R, 0.96 * D)};
		Turned(Linen, Under, true, R, D, 96);
		break;
	}
	case EAlbionWorkKind::Vase:
	{
		const double R = 0.5 * W;
		TArray<FVector2D> P;
		if (Profile.Num() >= 3)
		{
			// Its outline from its photograph (albion_hang.py sets it from assets/albion/vessels.json), from a closed foot up;
			// an open mouth turns in over its lip and down its neck to a floor inside.
			P.Add(FVector2D(0.0, 0.0));
			for (const FVector2D& Q : Profile) { P.Add(FVector2D(FMath::Max(0.001, R * Q.X), H * Q.Y)); }
			const FVector2D Lip = P.Last();
			if (Lip.X > 0.02)
			{
				const double Wall = FMath::Min(0.006, 0.3 * Lip.X);
				P.Add(FVector2D(Lip.X - Wall, Lip.Y - 0.002));
				P.Add(FVector2D(Lip.X - Wall - 0.004, Lip.Y - FMath::Min(0.12 * H, 0.05)));
				P.Add(FVector2D(0.0, Lip.Y - FMath::Min(0.12 * H, 0.05)));
			}
			Turned(Face_, P, false, R, H, 64, ImageWindow);
			if (HandlePath.Num() >= 2 && HandleThickness > 0.f)
			{
				TArray<FVector2D> HP;
				for (const FVector2D& Q : HandlePath) { HP.Add(FVector2D(R * Q.X, H * Q.Y)); }
				for (const double Side : {1.0, -1.0}) { Handle(Face_, HP, R * HandleThickness, R, H, Side, ImageWindow); }
			}
			break;
		}
		// A generic De Morgan ovoid vase (when no outline is given).
		for (int32 i = 0; i <= 40; ++i)
		{
			const double T = double(i) / 40.0;
			const double Body = FMath::Sin(UE_DOUBLE_PI * FMath::Min(T / 0.8, 1.0));
			const double Neck = T > 0.8 ? 0.42 + 0.35 * FMath::Square((T - 0.8) / 0.2) : 0.0;
			P.Add(FVector2D(FMath::Max(0.001, R * FMath::Max(Body * (0.55 + 0.45 * FMath::Sin(UE_DOUBLE_PI * T * 0.6)), Neck)), H * T));
		}
		P.Insert(FVector2D(0.0, 0.0), 0);
		Turned(Face_, P, false, R, H, 64);
		break;
	}
	}
	// Hanging rods from the frame's top to the picture rail.
	if (RailAbove > FrameOuterTop + 0.05 && FrameOuterHalfWidth > 0.f)
	{
		for (int32 s = -1; s <= 1; s += 2)
		{
			const double Y = s * FrameOuterHalfWidth * 0.62;
			TArray<FVector2D> Round;
			for (int32 k = 0; k < 8; ++k) { const double A = 2.0 * UE_DOUBLE_PI * k / 8.0; Round.Add(FVector2D(0.003 * FMath::Cos(A), 0.003 * FMath::Sin(A))); }
			// A path up the wall: the section in the X–Y plane.
			TArray<FStation> Up = {{FVector(0.012, Y, FrameOuterTop - 0.03), kX, kY}, {FVector(0.012, Y, RailAbove), kX, kY}};
			SweepStations(Bronze, Up, Round);
			Ball(Bronze, FVector(0.016, Y, FrameOuterTop - 0.03), 0.009, 10);
		}
	}
	Mesh->ClearAllMeshSections();
	Face_.Write(Mesh, 0, true);
	Linen.Write(Mesh, 1, true);
	Gilt.Write(Mesh, 2, false);
	Oak.Write(Mesh, 3, true);
	Bronze.Write(Mesh, 4, false);
	const TSoftObjectPtr<UMaterialInterface>* Mats[5] = {&FaceMaterial, &LinenMaterial, &GiltMaterial, &OakMaterial, &BronzeMaterial};
	for (int32 s = 0; s < 5; ++s)
	{
		if (UMaterialInterface* M = Mats[s]->LoadSynchronous()) { Mesh->SetMaterial(s, M); }
	}
}
