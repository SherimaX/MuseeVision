#include "Chenghuai/ChenghuaiVessel.h"

#include "Geometry/MuseeBake.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "Salon/SalonKit.h"

/** Turning a profile, in a named namespace (unity builds). */
namespace ChenghuaiVesselImpl
{
	using SalonKit::FMeshData;

	/**
	 * Turns a profile (r, z) about z: smooth normals along it except where it turns sharply (a foot ring's edge, the
	 * lip), where the point is split. UV: U round (0 … 1), V along the profile (0 … 1 by length).
	 */
	void Turn(FMeshData& M, const TArray<FVector2D>& P, double Z0, int32 Segs)
	{
		const int32 N = P.Num();
		if (N < 2) { return; }
		TArray<double> S;
		S.Add(0.0);
		for (int32 i = 1; i < N; ++i) { S.Add(S.Last() + FVector2D::Distance(P[i - 1], P[i])); }
		const double Len = FMath::Max(S.Last(), 1e-6);
		// The segments' normals in (r, z): the outer wall's point outward, the inner wall's inward, by the walk's turn.
		TArray<FVector2D> SegN;
		for (int32 i = 0; i + 1 < N; ++i)
		{
			const FVector2D D = (P[i + 1] - P[i]).GetSafeNormal();
			SegN.Add(FVector2D(D.Y, -D.X));   // right of the walk: out for the outer wall walked upward
		}
		for (int32 i = 0; i + 1 < N; ++i)
		{
			// Each segment's two rings: normals shared with a neighbour when the turn between them is gentle.
			FVector2D NA = SegN[i], NB = SegN[i];
			if (i > 0 && FVector2D::DotProduct(SegN[i - 1], SegN[i]) > 0.8) { NA = (SegN[i - 1] + SegN[i]).GetSafeNormal(); }
			if (i + 2 < N && FVector2D::DotProduct(SegN[i], SegN[i + 1]) > 0.8) { NB = (SegN[i] + SegN[i + 1]).GetSafeNormal(); }
			const int32 Base = M.Positions.Num();
			for (int32 k = 0; k <= Segs; ++k)
			{
				const double A = 2.0 * PI * k / Segs, C = FMath::Cos(A), Sn = FMath::Sin(A);
				M.Vertex(FVector(P[i].X * C, P[i].X * Sn, Z0 + P[i].Y), FVector(NA.X * C, NA.X * Sn, NA.Y), FVector2D(double(k) / Segs, S[i] / Len));
				M.Vertex(FVector(P[i + 1].X * C, P[i + 1].X * Sn, Z0 + P[i + 1].Y), FVector(NB.X * C, NB.X * Sn, NB.Y),
						 FVector2D(double(k) / Segs, S[i + 1] / Len));
			}
			for (int32 k = 0; k < Segs; ++k) { M.Quad(Base + 2 * k, Base + 2 * k + 2, Base + 2 * k + 3, Base + 2 * k + 1); }
		}
	}

	/** A carved stand (座): a moulded top, a waist, a foot ring on little feet; top radius R, height H. */
	TArray<FVector2D> StandProfile(double R, double H)
	{
		return {FVector2D(0.0, 0.0), FVector2D(R * 1.02, 0.0), FVector2D(R * 1.08, H * 0.12), FVector2D(R * 1.08, H * 0.28),
				FVector2D(R * 0.94, H * 0.42), FVector2D(R * 0.9, H * 0.62), FVector2D(R * 1.04, H * 0.74), FVector2D(R * 1.06, H * 0.88),
				FVector2D(R, H), FVector2D(0.0, H)};
	}
}

AChenghuaiVessel::AChenghuaiVessel()
{
	PrimaryActorTick.bCanEverTick = false;
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->bUseComplexAsSimpleCollision = true;
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	Tags.AddUnique(MuseeBake::BakeableTag());
	Tags.AddUnique(FName(TEXT("musee.wing:Chenghuai")));
}

void AChenghuaiVessel::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
}

void AChenghuaiVessel::BeginPlay()
{
	Super::BeginPlay();
	if (Mesh && Mesh->GetNumSections() == 0) { Build(); }
}

void AChenghuaiVessel::Rebuild() { Build(); }

void AChenghuaiVessel::Build()
{
	using namespace ChenghuaiVesselImpl;
	if (MuseeBake::IsBaked(this) || !Mesh) { return; }
	Mesh->ClearAllMeshSections();
	const int32 Segs = FMath::Clamp(Segments, 12, 256);
	const double H = StandRadius > 0.f ? StandHeight : 0.0;
	FMeshData Body, Stand;
	Turn(Body, Profile, H, Segs);
	Turn(Body, LidProfile, H, Segs);
	if (StandRadius > 0.f)
	{
		// The stand is walked the other way (centre, out, up, back to the axis): its normals face out the same.
		TArray<FVector2D> SP = StandProfile(StandRadius, StandHeight);
		Turn(Stand, SP, 0.0, FMath::Max(24, Segs / 2));
	}
	Body.Write(Mesh, 0, true);
	Stand.Write(Mesh, 1, true);
	const TCHAR* Default = TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial");
	UMaterialInterface* M0 = Material.IsNull() ? nullptr : Material.LoadSynchronous();
	UMaterialInterface* M1 = StandMaterial.IsNull() ? nullptr : StandMaterial.LoadSynchronous();
	Mesh->SetMaterial(0, M0 ? M0 : LoadObject<UMaterialInterface>(nullptr, Default));
	Mesh->SetMaterial(1, M1 ? M1 : LoadObject<UMaterialInterface>(nullptr, Default));
}
