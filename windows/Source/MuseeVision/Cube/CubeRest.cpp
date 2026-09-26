#include "Cube/CubeRest.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Cube/CubePlan.h"
#include "Cube/SeaLight.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "MuseeVision.h"

namespace
{
	constexpr double Disc = 6.0;       // m: the tracks drawn pass within this of the car's axis at eye level
	constexpr float Life = 2.4f;       // s: a line is gone after this
	constexpr float Fade = 0.75f;      // s: its glow's time constant
	constexpr float Forming = 0.12f;   // s: the droplets forming along it

	/** Clip the line P + t D to the room (its faces 14 m from the eyes each way). */
	bool ClipToRoom(const FVector& P, const FVector& D, FVector& A, FVector& B)
	{
		double T0 = -1e9, T1 = 1e9;
		for (int32 k = 0; k < 3; ++k)
		{
			const double H = CubePlan::Half - 0.05;
			if (FMath::Abs(D[k]) < 1e-9) { if (FMath::Abs(P[k]) > H) { return false; } continue; }
			double Ta = (-H - P[k]) / D[k], Tb = (H - P[k]) / D[k];
			if (Ta > Tb) { Swap(Ta, Tb); }
			T0 = FMath::Max(T0, Ta);
			T1 = FMath::Min(T1, Tb);
		}
		if (T1 <= T0) { return false; }
		A = P + D * T0;
		B = P + D * T1;
		return true;
	}
}

ACubeRest::ACubeRest()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

ACubeRest* ACubeRest::Get(UWorld* World)
{
	if (!World) { return nullptr; }
	for (TActorIterator<ACubeRest> It(World); It; ++It) { return *It; }
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<ACubeRest>(ACubeRest::StaticClass(), FTransform(CubePlan::WorldCentre()), Params);
}

void ACubeRest::BeginPlay()
{
	Super::BeginPlay();
	SetActorLocation(CubePlan::WorldCentre());
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Museum/Journeys/Materials/M_CubeMuon.M_CubeMuon"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (!Mesh || !Material)
	{
		UE_LOG(LogMusee, Warning, TEXT("The Cube at rest: its tracks' mesh or material is missing (Scripts/journeys.py cube)."));
		SetActorTickEnabled(false);
		return;
	}
	LineMaterial = UMaterialInstanceDynamic::Create(Material, this);
	LineMaterial->SetScalarParameterValue(TEXT("Nits"), Nits);
	Lines = NewObject<UInstancedStaticMeshComponent>(this, TEXT("Tracks"));
	Lines->SetupAttachment(RootComponent);
	Lines->SetMobility(EComponentMobility::Movable);
	Lines->SetStaticMesh(Mesh);
	Lines->SetMaterial(0, LineMaterial);
	Lines->NumCustomDataFloats = 2;   // 0 the glow, 1 the line's radius (cm)
	Lines->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Lines->SetCastShadow(false);
	Lines->SetCanEverAffectNavigation(false);
	Lines->bAffectDistanceFieldLighting = false;
	Lines->SetVisibleInRayTracing(false);
	Lines->bVisibleInReflectionCaptures = false;
	Lines->SetVisibility(false);
	Lines->RegisterComponent();
}

void ACubeRest::AddTrack()
{
	// Where it crosses the eyes' level: uniform in the disc. Its zenith angle: the flux through a level surface goes as
	// cos³θ sinθ (cos² from the sky, cos from the surface), so cosθ = u^¼. Its azimuth uniform.
	const double Rr = Disc * FMath::Sqrt(Rng.FRand()), Az = Rng.FRandRange(0.0, 2.0 * PI);
	const FVector P(Rr * FMath::Cos(Az), Rr * FMath::Sin(Az), 0.0);
	const double CosT = FMath::Max(0.18, FMath::Pow(double(Rng.FRand()), 0.25)), SinT = FMath::Sqrt(1.0 - CosT * CosT);
	const double Phi = Rng.FRandRange(0.0, 2.0 * PI);
	const FVector D(SinT * FMath::Cos(Phi), SinT * FMath::Sin(Phi), -CosT);
	FVector A, B;
	if (!ClipToRoom(P, D, A, B)) { return; }
	FSegment S;
	S.A = A;
	S.B = B;
	S.Strength = Rng.FRandRange(0.7f, 1.f);
	Segments.Add(S);
	// Now and then (one in six) a delta electron is knocked out of an atom along it, within a few metres of the car.
	if (Rng.FRand() < 0.17f)
	{
		const double T = FMath::Clamp(FVector::DotProduct(-P, D) + Rng.FRandRange(-4.0, 4.0), FVector::DotProduct(A - P, D), FVector::DotProduct(B - P, D));
		AddCurl(P + D * T, D);
	}
}

void ACubeRest::AddCurl(const FVector& From, const FVector& Along)
{
	// A slow electron has no field to curl it here: it scatters at every step, more as it slows, and stops within a
	// hand's length: a short kinked track leaving the line at a steep angle.
	FVector Dir = (Along + Rng.GetUnitVector() * 1.6).GetSafeNormal();
	FVector At = From;
	const int32 Steps = Rng.RandRange(8, 13);
	for (int32 k = 0; k < Steps; ++k)
	{
		const double Len = 0.018 * (1.0 - 0.5 * k / Steps);
		const FVector Next = At + Dir * Len;
		FSegment S;
		S.A = At;
		S.B = Next;
		S.Delay = 0.03f;
		S.Strength = 1.25f;
		Segments.Add(S);
		At = Next;
		const double Kink = FMath::DegreesToRadians(20.0 + 50.0 * k / Steps);
		Dir = (Dir + FVector::CrossProduct(Dir, Rng.GetUnitVector()).GetSafeNormal() * FMath::Tan(Kink)).GetSafeNormal();
	}
}

void ACubeRest::Draw()
{
	if (!Lines) { return; }
	const bool bShow = Segments.Num() > 0;
	if (Lines->IsVisible() != bShow) { Lines->SetVisibility(bShow); }
	if (!bShow && Drawn == 0) { return; }
	Lines->ClearInstances();
	for (const FSegment& S : Segments)
	{
		const float Age = S.Age - S.Delay;
		const FVector Axis = S.B - S.A;
		const double Len = Axis.Size();
		if (Len < 1e-4) { continue; }
		// The line widens as the droplets grow and drift (2.5 mm to about 6 mm), and fades.
		const float RadiusCm = 0.25f + 0.18f * FMath::Max(Age, 0.f);
		const float Glow = Age <= 0.f ? 0.f : FMath::SmoothStep(0.f, Forming, Age) * FMath::Exp(-Age / Fade) * S.Strength * Presence;
		const FTransform T(FRotationMatrix::MakeFromZ(Axis / Len).ToQuat(), (S.A + S.B) * 50.0,
						   FVector(RadiusCm / 50.0, RadiusCm / 50.0, Len));
		const int32 I = Lines->AddInstance(T, false);
		Lines->SetCustomDataValue(I, 0, Glow, false);
		Lines->SetCustomDataValue(I, 1, RadiusCm, false);
	}
	Drawn = Segments.Num();
	Lines->MarkRenderStateDirty();
}

void ACubeRest::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Lines) { return; }
	// Only while the eyes are in the room and no piece plays.
	bool bIn = false;
	float PixelAngle = 0.0012f;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (PC->PlayerCameraManager)
		{
			const FVector E = (PC->PlayerCameraManager->GetCameraLocation() - GetActorLocation()) / 100.0;
			bIn = E.GetAbsMax() < CubePlan::Half;
			int32 SX = 0, SY = 0;
			PC->GetViewportSize(SX, SY);
			if (SX > 0) { PixelAngle = 2.f * FMath::Tan(FMath::DegreesToRadians(PC->PlayerCameraManager->GetFOVAngle()) * 0.5f) / float(SX); }
		}
	}
	bool bPiece = false;
	for (TActorIterator<ACubeSeaLight> It(GetWorld()); It; ++It) { bPiece |= It->IsPlaying(); }
	Presence = FMath::FInterpConstantTo(Presence, bIn && !bPiece ? 1.f : 0.f, DeltaSeconds, 0.5f);
	if (Presence > 0.f)
	{
		// Poisson: the gaps between tracks are exponential.
		Carry -= DeltaSeconds * Presence;
		while (Carry <= 0.0)
		{
			if (Segments.Num() < 400) { AddTrack(); }
			Carry += -FMath::Loge(FMath::Max(double(Rng.FRand()), 1e-6)) / FMath::Max(Rate, 0.01f);
		}
	}
	for (int32 i = Segments.Num() - 1; i >= 0; --i)
	{
		Segments[i].Age += DeltaSeconds;
		if (Segments[i].Age > Life + Segments[i].Delay) { Segments.RemoveAtSwap(i); }
	}
	if (LineMaterial) { LineMaterial->SetScalarParameterValue(TEXT("PixelAngle"), PixelAngle); }
	Draw();
}
