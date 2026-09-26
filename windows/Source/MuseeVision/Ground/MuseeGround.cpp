#include "Ground/MuseeGround.h"

#include "Chenghuai/ChenghuaiPlan.h"

#include "Geometry/MuseeBake.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "ConstrainedDelaunay2.h"

AMuseeGround::AMuseeGround()
{
	PrimaryActorTick.bCanEverTick = false;
	Ground = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Ground"));
	RootComponent = Ground;
	Ground->SetCollisionProfileName(TEXT("BlockAll"));
	Ground->bUseComplexAsSimpleCollision = true;
	GroundMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Museum/HallOfLight/HallOfLight/Materials/grass_8A9E6A.grass_8A9E6A")));
	Tags.Add(FName(TEXT("musee.building")));
	Tags.Add(FName(TEXT("musee.wing:HallOfLight")));
}

TArray<FString> AMuseeGround::GetReplacedImportPrims()
{
	return {TEXT("/Museum/HallOfLight/Ground")};
}

void AMuseeGround::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
}

void AMuseeGround::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }   // baked into static meshes (Geometry/MuseeBake.h)
	using namespace UE::Geometry;
	TConstrainedDelaunay2<double> CDT;
	CDT.FillRule = TConstrainedDelaunay2<double>::EFillRule::Odd;
	CDT.bOrientedEdges = false;
	auto Loop = [&CDT](const TArray<FVector2d>& Points, bool bHole)
	{
		const int32 Start = CDT.Vertices.Num();
		CDT.Vertices.Append(Points);
		TArray<FIndex2i>& Edges = bHole ? CDT.HoleEdges : CDT.Edges;
		for (int32 k = 0; k < Points.Num(); ++k) { Edges.Add(FIndex2i(Start + k, Start + (k + 1) % Points.Num())); }
	};
	auto Circle = [](const FVector2d& C, double R, int32 N)
	{
		TArray<FVector2d> P;
		for (int32 k = 0; k < N; ++k) { const double A = 2.0 * UE_DOUBLE_PI * k / N; P.Add(C + FVector2d(FMath::Cos(A), FMath::Sin(A)) * R); }
		return P;
	};
	// The square's edge, a vertex every 20 m.
	const double H = HalfSize, Step = 2000.0;
	const int32 Per = FMath::Max(1, FMath::RoundToInt(2 * H / Step));
	TArray<FVector2d> Outer;
	for (int32 k = 0; k < Per; ++k) { Outer.Add(FVector2d(-H + 2 * H * k / Per, -H)); }
	for (int32 k = 0; k < Per; ++k) { Outer.Add(FVector2d(H, -H + 2 * H * k / Per)); }
	for (int32 k = 0; k < Per; ++k) { Outer.Add(FVector2d(H - 2 * H * k / Per, H)); }
	for (int32 k = 0; k < Per; ++k) { Outer.Add(FVector2d(-H, H - 2 * H * k / Per)); }
	Loop(Outer, false);
	// The holes.
	const FVector2d ElanCentre(5400.0, 0.0);
	const FBox2d Pond(FVector2d(-9045.0, -125.0), FVector2d(-8285.0, 125.0));
	Loop(Circle(FVector2d::ZeroVector, HoleRadius, 192), true);
	Loop(Circle(ElanCentre, ElanHoleRadius, 64), true);
	Loop({Pond.Min, FVector2d(Pond.Max.X, Pond.Min.Y), Pond.Max, FVector2d(Pond.Min.X, Pond.Max.Y)}, true);
	// Chenghuai's pond (Chenghuai/ChenghuaiGardenPlan.h): the garden's own ground covers the rest of this box.
	// (Only once Chenghuai has the north door: Chenghuai::bNorthDoorOpen.)
	const FBox2d GardenPond(FVector2d(480.0, -5360.0), FVector2d(1450.0, -3930.0));
	if (Chenghuai::bNorthDoorOpen)
	{
		Loop({GardenPond.Min, FVector2d(GardenPond.Max.X, GardenPond.Min.Y), GardenPond.Max, FVector2d(GardenPond.Min.X, GardenPond.Max.Y)}, true);
	}
	// A lattice of free points inside, clear of the holes and the edge.
	for (double X = -H + Step; X < H - 1.0; X += Step)
	{
		for (double Y = -H + Step; Y < H - 1.0; Y += Step)
		{
			const FVector2d P(X, Y);
			if (P.Size() < HoleRadius + 300.0 || (P - ElanCentre).Size() < ElanHoleRadius + 300.0 || Pond.ExpandBy(300.0).IsInside(P) ||
				(Chenghuai::bNorthDoorOpen && GardenPond.ExpandBy(300.0).IsInside(P))) { continue; }
			CDT.Vertices.Add(P);
		}
	}
	if (!CDT.Triangulate())
	{
		UE_LOG(LogTemp, Warning, TEXT("MuseeGround: the triangulation failed; no ground."));
		Ground->ClearAllMeshSections();
		return;
	}
	TArray<FVector> V;
	TArray<FVector> N;
	TArray<FVector2D> UV;
	TArray<FProcMeshTangent> Tan;
	for (const FVector2d& P : CDT.Vertices)
	{
		V.Add(FVector(P.X, P.Y, Height));
		N.Add(FVector::UpVector);
		UV.Add(FVector2D(P.X, P.Y) / 100.0);
		Tan.Add(FProcMeshTangent(1, 0, 0));
	}
	TArray<int32> T;
	for (const FIndex3i& Tri : CDT.Triangles)
	{
		// Wound so cross(b − a, c − a) points down, away from a viewer above (the project's front-face rule).
		const FVector2d A = CDT.Vertices[Tri.A], B = CDT.Vertices[Tri.B], C = CDT.Vertices[Tri.C];
		const double Z = (B - A).X * (C - A).Y - (B - A).Y * (C - A).X;
		if (Z < 0) { T.Append({Tri.A, Tri.B, Tri.C}); }
		else { T.Append({Tri.A, Tri.C, Tri.B}); }
	}
	Ground->ClearAllMeshSections();
	Ground->CreateMeshSection(0, V, T, N, UV, TArray<FColor>(), Tan, true);
	if (UMaterialInterface* M = GroundMaterial.LoadSynchronous()) { Ground->SetMaterial(0, M); }
}
