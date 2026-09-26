#include "Cube/SeaLight.h"

#include "Async/ParallelFor.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Cube/CubePlan.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "Misc/PackageName.h"
#include "MuseeVision.h"
#include "Visitor/MuseeCharacter.h"

#define LOCTEXT_NAMESPACE "MuseeSeaLight"

namespace SeaLightPlan
{
	constexpr int32 SampleRate = 48000;
	const TCHAR* const Folder = TEXT("/Game/Museum/Journeys/SeaLight");
	constexpr double CarR = 2.2;       // the car's glass (m)
	constexpr double PartR = 3.0;      // the shoal keeps outside this radius (1 m off the rail)
	constexpr double RailR = 2.0, RailZ = -0.65;   // the rail, from the eyes
	constexpr double Surface = 4.5;    // the sea's surface over the eyes (m)
	constexpr float Tau = 0.7f;        // the stirred water fades (s)
}

bool ACubeSeaLight::bForceHand = false;
bool ACubeSeaLight::bShowHand = false;
bool ACubeSeaLight::bProfile = false;
float ACubeSeaLight::TestFill = -1.f;

bool ACubeSeaLight::IsAvailable()
{
	static const bool bMade = FPackageName::DoesPackageExist(TEXT("/Game/Museum/Journeys/SeaLight/SM_SeaLight_Cards"))
		&& FPackageName::DoesPackageExist(TEXT("/Game/Museum/Journeys/SeaLight/M_SeaLight_Cells"))
		&& FPackageName::DoesPackageExist(TEXT("/Game/Museum/Journeys/SeaLight/SM_SeaLight_Fish"))
		&& FPackageName::DoesPackageExist(TEXT("/Game/Museum/Journeys/SeaLight/SM_SeaLight_Manta"));
	return bMade;
}

// ---------------------------------------------------------------------------------------------
// Sound

USeaLightWave::USeaLightWave(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetSampleRate(SeaLightPlan::SampleRate);
	NumChannels = 1;
	Duration = INDEFINITELY_LOOPING_DURATION;
	bLooping = false;
	SampleByteSize = 2;
}

int32 USeaLightWave::OnGeneratePCMAudio(TArray<uint8>& OutAudio, int32 NumSamples)
{
	OutAudio.SetNumUninitialized(NumSamples * 2);
	int16* Out = reinterpret_cast<int16*>(OutAudio.GetData());
	const float Want = FMath::Clamp(Level.load(std::memory_order_relaxed), 0.f, 1.f);
	const float Rate = float(SeaLightPlan::SampleRate);
	double SumSq = 0.0;
	for (int32 i = 0; i < NumSamples; ++i)
	{
		Held += (Want - Held) * 0.0002f;
		Clock += 1.0 / Rate;
		const float White = Rand01() * 2.f - 1.f;
		float S = 0.f;
		switch (Voice)
		{
		case 0:   // the hush: brown noise, low-passed again, breathing over ~9 s
		{
			Brown = FMath::Clamp(Brown * 0.998f + White * 0.02f, -1.f, 1.f);
			Band1 += (Brown - Band1) * 0.02f;
			Breath = 0.75f + 0.25f * FMath::Sin(float(Clock) * 0.7f);
			S = Band1 * 1.6f * Breath;
			break;
		}
		case 1:   // the fizz: fine random ticks, bright, their rate following the stirring
		case 3:   // the rain overhead: low soft ticks, muffled
		{
			const bool bRain = Voice == 3;
			const float PerSample = (bRain ? 40.f : 900.f) * Held / Rate;
			if (Rand01() < PerSample)
			{
				const float Freq = bRain ? 300.f + 500.f * Rand01() : 2000.f + 4000.f * Rand01();
				Tick = (bRain ? 0.25f : 0.12f) * (0.3f + 0.7f * Rand01());
				TickStep = 2.f * PI * Freq / Rate;
				TickPhase = 0.f;
				TickDecay = FMath::Exp(-1.f / (Rate * (bRain ? 0.012f : 0.0015f)));
			}
			if (Tick > 1e-4f)
			{
				S += Tick * FMath::Sin(TickPhase);
				TickPhase += TickStep;
				if (TickPhase > 2.f * PI) { TickPhase -= 2.f * PI; }
				Tick *= TickDecay;
			}
			// Under the ticks a faint wash of the same colour.
			Band1 += ((bRain ? Brown : White) - Band1) * (bRain ? 0.05f : 0.5f);
			Brown = FMath::Clamp(Brown * 0.998f + White * 0.02f, -1.f, 1.f);
			S += Band1 * (bRain ? 0.25f : 0.02f) * Held;
			break;
		}
		default:  // the shoal's hiss: band noise (two one-pole filters), rising and falling as it passes
		{
			Band1 += (White - Band1) * 0.35f;
			Band2 += (Band1 - Band2) * 0.08f;
			S = (Band1 - Band2) * 0.5f * Held;
			break;
		}
		}
		if (Voice == 0) { S *= Held; }
		Out[i] = int16(FMath::Clamp(S, -1.f, 1.f) * 30000.f);
		SumSq += double(Out[i]) * Out[i];
	}
	Rms.store(float(FMath::Sqrt(SumSq / FMath::Max(NumSamples, 1)) / 30000.0), std::memory_order_relaxed);
	Buffers.fetch_add(1, std::memory_order_relaxed);
	return NumSamples;
}

// ---------------------------------------------------------------------------------------------
// The piece

ACubeSeaLight::ACubeSeaLight()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Post = CreateDefaultSubobject<UPostProcessComponent>(TEXT("Post"));
	Post->SetupAttachment(RootComponent);
	Post->bUnbound = true;
	Post->Priority = 60.f;
	Post->bEnabled = false;
}

ACubeSeaLight* ACubeSeaLight::Get(UWorld* World)
{
	if (!World) { return nullptr; }
	for (TActorIterator<ACubeSeaLight> It(World); It; ++It) { return *It; }
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<ACubeSeaLight>(ACubeSeaLight::StaticClass(), FTransform(CubePlan::WorldCentre()), Params);
}

FVector ACubeSeaLight::Eyes() const
{
	return CubePlan::WorldCentre();
}

void ACubeSeaLight::BeginPlay()
{
	Super::BeginPlay();
	SetActorLocation(Eyes());
	CubeParameters = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Museum/Journeys/Materials/MPC_Cube.MPC_Cube"));
	Stir.SetNumZeroed(N * N * N);
	Flow.SetNumZeroed(N * N * N);
	SliceLive.SetNumZeroed(N);
	SliceDirty.SetNumZeroed(N);
	FieldOrigin = Eyes();
	BuildCells();
	BuildCreatures();
	// The voices: the hush round the car, the fizz at the hand, the shoal's hiss, the rain overhead.
	for (int32 v = 0; v < 4; ++v)
	{
		USeaLightWave* Wave = NewObject<USeaLightWave>(this);
		Wave->Voice = v;
		UAudioComponent* Audio = NewObject<UAudioComponent>(this);
		Audio->SetupAttachment(RootComponent);
		Audio->bAutoActivate = false;
		Audio->bAllowSpatialization = v != 0;
		Audio->bOverrideAttenuation = true;
		Audio->AttenuationOverrides.bAttenuate = true;
		Audio->AttenuationOverrides.bSpatialize = v != 0;
		Audio->AttenuationOverrides.AttenuationShape = EAttenuationShape::Sphere;
		Audio->AttenuationOverrides.AttenuationShapeExtents = FVector(v == 3 ? 600.f : 60.f, 0.f, 0.f);
		Audio->AttenuationOverrides.FalloffDistance = v == 2 ? 2500.f : 1500.f;
		Audio->SetSound(Wave);
		Audio->RegisterComponent();
		Voices.Add(Audio);
		Waves.Add(Wave);
	}
}

void ACubeSeaLight::EndPlay(const EEndPlayReason::Type Reason)
{
	for (UAudioComponent* A : Voices) { if (A) { A->Stop(); } }
	if (bPlaying) { SetRoomBlack(false); }
	Super::EndPlay(Reason);
}

void ACubeSeaLight::BuildCells()
{
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("%s/SM_SeaLight_Cards.SM_SeaLight_Cards"), SeaLightPlan::Folder));
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("%s/M_SeaLight_Cells.M_SeaLight_Cells"), SeaLightPlan::Folder));
	if (!Mesh || !Material)
	{
		UE_LOG(LogMusee, Warning, TEXT("Sea Light: its cells are missing (Scripts/journeys.py sealight)."));
		return;
	}
	FieldTexture = UTexture2D::CreateTransient(TexSize, TexSize, PF_B8G8R8A8);
	if (FieldTexture)
	{
		FieldTexture->SRGB = false;
		FieldTexture->Filter = TF_Bilinear;
		FieldTexture->AddressX = TA_Clamp;
		FieldTexture->AddressY = TA_Clamp;
		FieldTexture->UpdateResource();
	}
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Material, this);
	if (FieldTexture) { MID->SetTextureParameterValue(TEXT("Field"), FieldTexture); }
	MID->SetScalarParameterValue(TEXT("FieldSize"), float(Extent * 100.0));
	CellMaterials.Add(MID);
	// Four copies of the one mesh, turned and mirrored: 1 M cells.
	for (int32 k = 0; k < 4; ++k)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("Cells%d"), k + 1));
		C->SetupAttachment(RootComponent);
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(Mesh);
		C->SetMaterial(0, MID);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetCanEverAffectNavigation(false);
		C->bAffectDistanceFieldLighting = false;
		C->SetVisibleInRayTracing(false);
		C->BoundsScale = 1.25f;
		C->SetRelativeRotation(FRotator(0.0, 37.0 + 90.0 * k, 0.0));
		C->SetRelativeScale3D(FVector(1.0, 1.0, (k % 2) ? -1.0 : 1.0));
		C->SetVisibility(false);
		C->RegisterComponent();
		Cells.Add(C);
	}
}

namespace
{
	UInstancedStaticMeshComponent* MakeInstances(AActor* Owner, USceneComponent* Root, const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Material, int32 CustomFloats)
	{
		UInstancedStaticMeshComponent* I = NewObject<UInstancedStaticMeshComponent>(Owner, Name);
		I->SetupAttachment(Root);
		I->SetMobility(EComponentMobility::Movable);
		I->SetStaticMesh(Mesh);
		I->SetMaterial(0, Material);
		I->NumCustomDataFloats = CustomFloats;
		I->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		I->SetCastShadow(false);
		I->SetCanEverAffectNavigation(false);
		I->bAffectDistanceFieldLighting = false;
		I->SetVisibleInRayTracing(false);
		I->bVisibleInReflectionCaptures = false;
		I->SetVisibility(false);
		I->RegisterComponent();
		return I;
	}
}

void ACubeSeaLight::BuildCreatures()
{
	using namespace SeaLightPlan;
	auto Mesh = [](const TCHAR* Name) { return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("%s/%s.%s"), Folder, Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet); };
	auto Mat = [](const TCHAR* Name) { return LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("%s/%s.%s"), Folder, Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet); };
	if (UStaticMesh* M = Mesh(TEXT("SM_SeaLight_Fish")))
	{
		UMaterialInterface* FM = Mat(TEXT("M_SeaLight_Fish"));
		FishMaterial = FM ? UMaterialInstanceDynamic::Create(FM, this) : nullptr;
		FishMeshes = MakeInstances(this, RootComponent, TEXT("Shoal"), M, FishMaterial ? static_cast<UMaterialInterface*>(FishMaterial.Get()) : FM, 1);
	}
	if (UStaticMesh* M = Mesh(TEXT("SM_SeaLight_Manta")))
	{
		MantaMesh = NewObject<UStaticMeshComponent>(this, TEXT("Manta"));
		MantaMesh->SetupAttachment(RootComponent);
		MantaMesh->SetMobility(EComponentMobility::Movable);
		MantaMesh->SetStaticMesh(M);
		if (UMaterialInterface* MM = Mat(TEXT("M_SeaLight_Manta")))
		{
			MantaMaterial = UMaterialInstanceDynamic::Create(MM, this);
			MantaMesh->SetMaterial(0, MantaMaterial);
		}
		MantaMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MantaMesh->SetCastShadow(false);
		MantaMesh->SetCanEverAffectNavigation(false);
		MantaMesh->SetVisibleInRayTracing(false);
		MantaMesh->BoundsScale = 1.5f;
		MantaMesh->SetVisibility(false);
		MantaMesh->RegisterComponent();
	}
	if (UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")))
	{
		RingMeshes = MakeInstances(this, RootComponent, TEXT("RainRings"), Plane, Mat(TEXT("M_SeaLight_Ring")), 1);
	}
}

void ACubeSeaLight::SetRoomBlack(bool bBlack)
{
	// The panels to black, their grid off (the piece's light is the only light in the room).
	if (CubeParameters) { UKismetMaterialLibrary::SetScalarParameterValue(this, CubeParameters, TEXT("GridGlow"), bBlack ? 0.f : 1.f); }
	Post->bEnabled = bBlack;
	if (bBlack)
	{
		FPostProcessSettings& PP = Post->Settings;
		PP.bOverride_AutoExposureMinBrightness = true;
		PP.bOverride_AutoExposureMaxBrightness = true;
		PP.bOverride_AutoExposureBias = true;
		PP.bOverride_AutoExposureBiasCurve = true;
		PP.AutoExposureMinBrightness = ExposureEV;
		PP.AutoExposureMaxBrightness = ExposureEV;
		PP.AutoExposureBias = 0.f;
		PP.AutoExposureBiasCurve = nullptr;
		// Each flash carries a soft halo, as a bright point does in the dark-adapted eye.
		PP.bOverride_BloomIntensity = true;
		PP.BloomIntensity = 1.4f;
		PP.bOverride_BloomThreshold = true;
		PP.BloomThreshold = -1.f;
		PP.bOverride_BloomSizeScale = true;
		PP.BloomSizeScale = 2.5f;
	}
	for (UStaticMeshComponent* C : Cells) { if (C) { C->SetVisibility(bBlack); } }
	if (!bBlack)
	{
		if (FishMeshes) { FishMeshes->SetVisibility(false); }
		if (MantaMesh) { MantaMesh->SetVisibility(false); }
		if (RingMeshes) { RingMeshes->SetVisibility(false); }
	}
}

void ACubeSeaLight::Begin(double FromSeconds)
{
	T = FMath::Clamp(FromSeconds, 0.0, Length - 1.0);
	bPlaying = true;
	bClosing = false;
	bFinished = false;
	bToldReach = T > 60.0;
	Fade = FromSeconds > 1.0 ? 1.f : 0.f;
	FMemory::Memzero(Stir.GetData(), Stir.Num() * sizeof(float));
	FMemory::Memzero(Flow.GetData(), Flow.Num() * sizeof(FVector3f));
	for (int32 z = 0; z < N; ++z) { SliceLive[z] = 0; SliceDirty[z] = 1; }
	Fish.Reset();
	ShoalShown = 0.f;
	if (FishMeshes) { FishMeshes->ClearInstances(); FishMeshes->SetVisibility(false); }
	Rings.Reset();
	bHandValid = false;
	SetRoomBlack(true);
	for (int32 v = 0; v < Voices.Num(); ++v)
	{
		if (Voices[v]) { Voices[v]->SetVolumeMultiplier(v == 0 ? 0.35f : 0.6f); Voices[v]->Play(); }
	}
	SetActorTickEnabled(true);
	UE_LOG(LogMusee, Log, TEXT("Sea Light: begins at %.0f s."), T);
}

void ACubeSeaLight::End()
{
	if (!bPlaying || bClosing) { return; }
	bClosing = true;
	UE_LOG(LogMusee, Log, TEXT("Sea Light: ends at %.0f s."), T);
}

void ACubeSeaLight::Splat(const FVector& P, double Radius, double Amount, const FVector& Velocity)
{
	// P in metres from the eyes (Unreal's axes). Cells within ~2 radii get a Gaussian share of the stirring, and of the
	// water's motion (momentum: the shader divides it back by the stirring).
	const double H = Extent / N;
	const double R = FMath::Max(Radius, 0.6 * H);
	const FVector G = P / H + FVector(N * 0.5 - 0.5);
	const int32 Reach = FMath::CeilToInt32(2.0 * R / H);
	const int32 X0 = FMath::Max(0, FMath::FloorToInt32(G.X) - Reach + 1), X1 = FMath::Min(N - 1, FMath::FloorToInt32(G.X) + Reach);
	const int32 Y0 = FMath::Max(0, FMath::FloorToInt32(G.Y) - Reach + 1), Y1 = FMath::Min(N - 1, FMath::FloorToInt32(G.Y) + Reach);
	const int32 Z0 = FMath::Max(0, FMath::FloorToInt32(G.Z) - Reach + 1), Z1 = FMath::Min(N - 1, FMath::FloorToInt32(G.Z) + Reach);
	const double InvR2 = 1.0 / (R * R);
	const FVector3f V(Velocity);
	for (int32 z = Z0; z <= Z1; ++z)
	{
		SliceLive[z] = 1;
		for (int32 y = Y0; y <= Y1; ++y)
		{
			for (int32 x = X0; x <= X1; ++x)
			{
				const FVector C = (FVector(x, y, z) - FVector(N * 0.5 - 0.5)) * H;
				const double W = FMath::Exp(-(C - P).SizeSquared() * InvR2);
				if (W < 0.02) { continue; }
				const int32 I = (z * N + y) * N + x;
				const float Add = float(Amount * W);
				if (Stir[I] + Add > 2.f) { continue; }
				Stir[I] += Add;
				Flow[I] += V * Add;
			}
		}
	}
}

void ACubeSeaLight::SplatLine(const FVector& A, const FVector& B, double Radius, double Amount, const FVector& Velocity)
{
	const double Len = (B - A).Size();
	const int32 Steps = FMath::Clamp(FMath::CeilToInt32(Len / FMath::Max(Radius, 0.06)), 1, 32);
	for (int32 s = 0; s <= Steps; ++s) { Splat(FMath::Lerp(A, B, double(s) / Steps), Radius, Amount / (Steps + 1), Velocity); }
}

void ACubeSeaLight::Decay(float DeltaSeconds)
{
	// Only the slices something has stirred (the rest are still zero), a slice to a worker.
	const float K = FMath::Exp(-DeltaSeconds / SeaLightPlan::Tau);
	ParallelFor(N, [this, K](int32 z)
	{
		if (!SliceLive[z]) { return; }
		float* S = &Stir[z * N * N];
		FVector3f* F = &Flow[z * N * N];
		bool bAny = false;
		for (int32 i = 0; i < N * N; ++i)
		{
			if (S[i] < 2e-3f) { S[i] = 0.f; F[i] = FVector3f::ZeroVector; continue; }
			S[i] *= K;
			F[i] *= K;
			bAny = true;
		}
		// Gone quiet: written once more (as zero) at the next upload, then left alone.
		if (!bAny) { SliceLive[z] = 0; SliceDirty[z] = 1; }
	});
}

void ACubeSeaLight::Upload()
{
	if (!FieldTexture || !FieldTexture->GetResource()) { return; }
	// Only the slices that changed go up: each encoded (a worker each) into its own 96² tile of one compact buffer, one
	// region per tile. BGRA: R the stirring (0 … 2), G B A the water's motion x y z (±2 m/s).
	TArray<int32> Changed;
	for (int32 z = 0; z < N; ++z) { if (SliceLive[z] || SliceDirty[z]) { Changed.Add(z); SliceDirty[z] = 0; } }
	if (Changed.Num() == 0) { return; }
	const int32 TileBytes = N * N * 4;
	uint8* Data = static_cast<uint8*>(FMemory::Malloc(TileBytes * Changed.Num()));
	FUpdateTextureRegion2D* Regions = new FUpdateTextureRegion2D[Changed.Num()];
	for (int32 k = 0; k < Changed.Num(); ++k)
	{
		const int32 z = Changed[k];
		Regions[k] = FUpdateTextureRegion2D((z % Tiles) * N, (z / Tiles) * N, 0, k * N, N, N);
	}
	ParallelFor(Changed.Num(), [this, &Changed, Data, TileBytes](int32 k)
	{
		auto Enc = [](float V) { return uint8(FMath::Clamp(V * 0.25f + 0.5f, 0.f, 1.f) * 255.f + 0.5f); };
		const int32 z = Changed[k];
		uint8* Dst = Data + k * TileBytes;
		const float* S = &Stir[z * N * N];
		const FVector3f* F = &Flow[z * N * N];
		for (int32 i = 0; i < N * N; ++i, Dst += 4)
		{
			if (S[i] <= 0.f) { Dst[0] = 128; Dst[1] = 128; Dst[2] = 0; Dst[3] = 128; continue; }
			const FVector3f V = F[i] / S[i];
			Dst[0] = Enc(V.Y);
			Dst[1] = Enc(V.X);
			Dst[2] = uint8(FMath::Clamp(S[i] * 0.5f, 0.f, 1.f) * 255.f + 0.5f);
			Dst[3] = Enc(V.Z);
		}
	});
	FieldTexture->UpdateTextureRegions(0, uint32(Changed.Num()), Regions, uint32(N * 4), 4, Data,
		[](uint8* SrcData, const FUpdateTextureRegion2D* R) { FMemory::Free(SrcData); delete[] R; });
}

void ACubeSeaLight::MoveHand(float DeltaSeconds)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	FVector Hand = FVector::ZeroVector, Pointing = FVector::ForwardVector;
	bool bActive = false;
	bHandVR = false;
	// In the headset: the right controller (the VR visitor's "RightHand").
	if (Pawn)
	{
		TArray<USceneComponent*> Parts;
		Pawn->GetComponents<USceneComponent>(Parts);
		for (USceneComponent* S : Parts)
		{
			if (S && S->GetName().Contains(TEXT("RightHand")))
			{
				Hand = S->GetComponentLocation() + S->GetForwardVector() * 6.0;
				Pointing = S->GetForwardVector();
				bActive = !S->GetRelativeLocation().IsNearlyZero(0.01);   // (an untracked controller sits at its origin)
				bHandVR = bActive;
				break;
			}
		}
	}
	// On the desktop: 0.6 m along the look ray while the right mouse button is held.
	if (!bActive && PC && PC->PlayerCameraManager && (bForceHand || PC->IsInputKeyDown(EKeys::RightMouseButton)))
	{
		Pointing = PC->PlayerCameraManager->GetCameraRotation().Vector();
		Hand = PC->PlayerCameraManager->GetCameraLocation() + Pointing * 60.0;
		bActive = true;
	}
	if (!bActive) { bHandValid = false; HandSpeed = FMath::FInterpTo(HandSpeed, 0.f, DeltaSeconds, 6.f); return; }
#if ENABLE_DRAW_DEBUG
	if (bShowHand) { DrawDebugSphere(GetWorld(), Hand, 2.5f, 10, FColor(90, 170, 255), false, -1.f, SDPG_World, 0.2f); }
#endif
	const FVector P = (Hand - Eyes()) / 100.0;
	if (!bHandValid) { HandLast = P; bHandValid = true; }
	const FVector Move = P - HandLast;
	const float Speed = float(Move.Size() / FMath::Max(DeltaSeconds, 1e-3f));
	HandSpeed = FMath::FInterpTo(HandSpeed, Speed, DeltaSeconds, 12.f);
	const FVector HandV = Move / FMath::Max(double(DeltaSeconds), 1e-3);
	// The hand's wake: the water it pushes lights along its path (per metre of travel, so a sweep lights its whole path
	// whatever the frame rate), moving with it.
	const double Travel = FMath::Min(Move.Size(), 0.5);
	SplatLine(HandLast, P, 0.07, 10.0 * Travel, HandV * 0.8);
	// Round the fingers the water curls into eddies (a swirl about the hand's line of motion, or about the arm when it is
	// still): sparks that turn round the hand and fade.
	const FVector Axis = (Travel > 0.002 ? Move.GetSafeNormal() : Pointing).GetSafeNormal();
	const FVector U = FVector::CrossProduct(Axis, FMath::Abs(Axis.Z) < 0.9 ? FVector::UpVector : FVector::ForwardVector).GetSafeNormal();
	const FVector W = FVector::CrossProduct(Axis, U);
	const double Swirl = 0.35 + 0.9 * FMath::Min(Speed, 2.f);
	const double Spin = T * 5.0;
	for (int32 k = 0; k < 6; ++k)
	{
		const double A = Spin + 2.0 * PI * k / 6.0;
		const FVector Off = (U * FMath::Cos(A) + W * FMath::Sin(A)) * 0.11;
		const FVector Tangent = (-U * FMath::Sin(A) + W * FMath::Cos(A)) * Swirl;
		Splat(P + Off - Axis * 0.05, 0.06, (1.2 + 0.15 * Travel / FMath::Max(double(DeltaSeconds), 1e-3)) * DeltaSeconds, Tangent + HandV * 0.4);
	}
	HandPos = P;
	HandLast = P;
}

void ACubeSeaLight::MoveShoal(float DeltaSeconds, float Presence)
{
	using namespace SeaLightPlan;
	ShoalShown = FMath::FInterpConstantTo(ShoalShown, Presence, DeltaSeconds, 0.5f);
	if (Presence <= 0.f && Fish.Num() == 0)
	{
		ShoalHiss = FMath::FInterpTo(ShoalHiss, 0.f, DeltaSeconds, 1.f);
		if (FishMeshes && FishMeshes->IsVisible()) { FishMeshes->SetVisibility(false); }
		return;
	}
	if (Fish.Num() == 0 && Presence > 0.f)
	{
		// A school of scad from the west: a long loose band, 12 m by 5 by 3, denser at its heart, all heading east.
		for (int32 i = 0; i < FishCount; ++i)
		{
			FFish F;
			const FVector R = Rng.GetUnitVector() * FMath::Pow(double(Rng.FRand()), 0.55);
			F.P = FVector(-13.0 + R.X * 6.0, 0.3 + R.Y * 2.6, -0.3 + R.Z * 1.5);
			F.Cruise = Rng.FRandRange(1.1f, 1.4f);
			F.V = FVector(F.Cruise, 0, 0);
			F.Size = Rng.FRandRange(0.85f, 1.15f);
			Fish.Add(F);
		}
		if (FishMeshes)
		{
			FishMeshes->ClearInstances();
			TArray<FTransform> Ts;
			for (const FFish& F : Fish) { Ts.Add(FTransform(FQuat::Identity, F.P * 100.0, FVector(F.Size))); }
			FishMeshes->AddInstances(Ts, false);
			// Each fish its own brightness (how much its skin catches), set once; the school's fade is the material's.
			for (int32 i = 0; i < Fish.Num(); ++i) { FishMeshes->SetCustomDataValue(i, 0, Rng.FRandRange(0.6f, 1.f), false); }
		}
	}
	// Neighbours by a hashed grid (0.8 m cells).
	constexpr double Cell = 0.8;
	constexpr int32 Buckets = 4096;
	GridHead.Init(-1, Buckets);
	GridNext.SetNumUninitialized(Fish.Num());
	auto Key = [](int32 X, int32 Y, int32 Z) { return int32((uint32(X) * 73856093u ^ uint32(Y) * 19349663u ^ uint32(Z) * 83492791u) % Buckets); };
	for (int32 i = 0; i < Fish.Num(); ++i)
	{
		const FVector& P = Fish[i].P;
		const int32 K = Key(FMath::FloorToInt32(P.X / Cell), FMath::FloorToInt32(P.Y / Cell), FMath::FloorToInt32(P.Z / Cell));
		GridNext[i] = GridHead[K];
		GridHead[K] = i;
	}
	// The school's way: east, drifting a little south as it goes, then leaving (after 4:00 it keeps going east and out).
	const FVector Goal = FVector(1.0, -0.12, 0.0).GetSafeNormal();
	// The school's heart and heading (last frame's): the wake is thrown back mostly by the fish at its rear, so the water
	// between the fish stays dark and one wake trails the whole school.
	FVector Heart = FVector::ZeroVector, Heading = FVector::ZeroVector;
	for (const FFish& F : Fish) { Heart += F.P; Heading += F.V; }
	Heart /= FMath::Max(Fish.Num(), 1);
	Heading = Heading.GetSafeNormal();
	FVector Sum = FVector::ZeroVector;
	int32 Near = 0;
	TArray<FVector> NewV;
	NewV.SetNumUninitialized(Fish.Num());
	// The steering of each fish reads the others only (last frame's): a batch of fish to a worker.
	ParallelFor(Fish.Num(), [&](int32 i)
	{
		const FFish& F = Fish[i];
		FVector Sep = FVector::ZeroVector, Ali = FVector::ZeroVector, Coh = FVector::ZeroVector;
		int32 Count = 0;
		const int32 CX = FMath::FloorToInt32(F.P.X / Cell), CY = FMath::FloorToInt32(F.P.Y / Cell), CZ = FMath::FloorToInt32(F.P.Z / Cell);
		for (int32 dz = -1; dz <= 1; ++dz) for (int32 dy = -1; dy <= 1; ++dy) for (int32 dx = -1; dx <= 1; ++dx)
		{
			for (int32 j = GridHead[Key(CX + dx, CY + dy, CZ + dz)]; j >= 0; j = GridNext[j])
			{
				if (j == i) { continue; }
				const FVector D = Fish[j].P - F.P;
				const double D2 = D.SizeSquared();
				if (D2 > Cell * Cell) { continue; }
				if (D2 < 0.3 * 0.3) { Sep -= D / FMath::Max(D2, 0.004); }
				Ali += Fish[j].V;
				Coh += D;
				++Count;
			}
		}
		FVector Acc = FVector::ZeroVector;
		if (Count > 0)
		{
			Acc += (Ali / Count - F.V) * 2.2;        // alignment: a polarised school
			Acc += (Coh / Count) * 0.9;              // cohesion
		}
		Acc += Sep * 0.06;                            // separation (about a body length apart)
		Acc += (Goal * F.Cruise - F.V) * 0.6;         // the way
		// The car: a cylinder about the vertical axis (and the mast above it). Steer round it early, as one body.
		const FVector2D Flat(F.P.X, F.P.Y);
		const double R = Flat.Size();
		if (R < PartR + 2.5)
		{
			const FVector2D Out = Flat / FMath::Max(R, 0.01);
			const double Push = FMath::Square(FMath::Clamp((PartR + 2.5 - R) / 2.5, 0.0, 1.0)) * 6.0;
			// Aside, not back: the push turns into a slide round the car along the school's way.
			const FVector2D Side = FVector2D(-Out.Y, Out.X) * (FVector2D::DotProduct(FVector2D(-Out.Y, Out.X), FVector2D(Goal.X, Goal.Y)) >= 0.0 ? 1.0 : -1.0);
			Acc += FVector(Out.X + 0.6 * Side.X, Out.Y + 0.6 * Side.Y, 0.0) * Push;
		}
		// The water's depth for them: 2.6 m above the eyes to 2.8 m below.
		if (F.P.Z > 2.6) { Acc.Z -= (F.P.Z - 2.6) * 4.0; }
		if (F.P.Z < -2.8) { Acc.Z += (-2.8 - F.P.Z) * 4.0; }
		FVector V = F.V + Acc * DeltaSeconds;
		const double S = V.Size();
		V = V / FMath::Max(S, 1e-3) * FMath::Clamp(S, 0.7, 1.9);
		NewV[i] = V;
	}, EParallelForFlags::None);
	TArray<FTransform> Ts;
	Ts.SetNumUninitialized(Fish.Num());
	for (int32 i = 0; i < Fish.Num(); ++i)
	{
		FFish& F = Fish[i];
		const FVector Before = F.V.GetSafeNormal();
		F.V = NewV[i];
		F.P += F.V * DeltaSeconds;
		// Never inside the part radius (a fish that swam through a person would break the spell).
		const FVector2D Flat(F.P.X, F.P.Y);
		if (Flat.Size() < PartR)
		{
			const FVector2D Out = Flat.GetSafeNormal() * PartR;
			F.P.X = Out.X;
			F.P.Y = Out.Y;
		}
		const FVector Dir = F.V.GetSafeNormal();
		F.Turn = FMath::FInterpTo(F.Turn, float(FMath::Acos(FMath::Clamp(FVector::DotProduct(Before, Dir), -1.0, 1.0)) / FMath::Max(DeltaSeconds, 1e-3f)), DeltaSeconds, 4.f);
		// Its wake: the water it throws back lights behind its tail, and the school's wakes join into one.
		if (ShoalShown > 0.f && FMath::Abs(F.P.X) < Extent * 0.5 && FMath::Abs(F.P.Y) < Extent * 0.5)
		{
			const FVector Tail = F.P - Dir * 0.16 * F.Size;
			const double Rear = FMath::Clamp(-FVector::DotProduct(F.P - Heart, Heading) / 3.0, 0.0, 1.0);
			Splat(Tail, 0.06, (0.2 + 1.6 * Rear + 0.5 * FMath::Min(F.Turn, 2.f)) * DeltaSeconds * ShoalShown, -F.V * 0.35 + FVector(0, 0, 0.05));
		}
		Ts[i] = FTransform(FRotationMatrix::MakeFromXZ(Dir, FVector::UpVector).ToQuat(), F.P * 100.0, FVector(F.Size));
		if (FMath::Abs(F.P.X) < 10.0) { Sum += F.P; ++Near; }
	}
	if (FishMeshes)
	{
		FishMeshes->BatchUpdateInstancesTransforms(0, Ts, false, true, true);
		if (FishMaterial) { FishMaterial->SetScalarParameterValue(TEXT("Fade"), ShoalShown * Fade); }
		if (!FishMeshes->IsVisible()) { FishMeshes->SetVisibility(true); }
	}
	// Gone east and out of the room, or faded: the school is let go.
	if (ShoalShown <= 0.f && Presence <= 0.f)
	{
		Fish.Reset();
		if (FishMeshes) { FishMeshes->ClearInstances(); FishMeshes->SetVisibility(false); }
	}
	ShoalCentre = Near > 0 ? Sum / Near : FVector(-12, 0, 0);
	ShoalHiss = FMath::FInterpTo(ShoalHiss, FMath::Clamp(Near / 250.f, 0.f, 1.f) * ShoalShown, DeltaSeconds, 1.5f);
}

void ACubeSeaLight::MoveManta(float DeltaSeconds, double Tm)
{
	// A reef manta 4.5 m across, 1 m/s: from the south-west over the car's roof (1.5 m over it), then banking away
	// north-east. Its body is dark: it is seen by what it hides and by the sparks its wing edges and wake stir.
	constexpr double Speed = 1.0, Span = 4.5;
	const FVector From(-14.0, 7.0, 2.8), Dir = FVector(1.0, -0.5, -0.03).GetSafeNormal();
	const double Dist = Tm * Speed;
	FVector Pos = From + Dir * Dist;
	const double Bank = FMath::DegreesToRadians(22.0 * FMath::SmoothStep(20.0, 30.0, Dist));
	FVector Fwd = Dir.RotateAngleAxis(-35.0 * FMath::SmoothStep(20.0, 32.0, Dist), FVector::UpVector);
	Pos += (Fwd - Dir) * 2.0 * FMath::SmoothStep(20.0, 32.0, Dist);
	const FVector Side = FVector::CrossProduct(FVector::UpVector, Fwd).GetSafeNormal();
	const FVector Up = FVector::UpVector * FMath::Cos(Bank) + Side * FMath::Sin(Bank);
	// The wingbeat: a slow flight stroke every 5 s.
	const double Phase = 2.0 * PI * Tm / 5.0;
	const double Stroke = FMath::Sin(Phase);
	if (MantaMesh)
	{
		const bool bShow = Tm >= 0.0 && Dist < 34.0;
		if (MantaMesh->IsVisible() != bShow) { MantaMesh->SetVisibility(bShow); }
		MantaMesh->SetRelativeTransform(FTransform(FRotationMatrix::MakeFromXZ(Fwd, Up).ToQuat(), Pos * 100.0, FVector(Span / 4.5)));
		if (MantaMaterial) { MantaMaterial->SetScalarParameterValue(TEXT("WingPhase"), float(Phase)); }
	}
	// The outline (the mesh's own, m, x forward, y to the right wing): the edges and tips stir the water as they move,
	// most on the downstroke.
	static const FVector2D Outline[] = {
		{1.0, 0.3}, {1.1, 0.14}, {0.95, 0.0}, {1.1, -0.14}, {1.0, -0.3},
		{0.62, -0.95}, {0.18, -1.85}, {-0.08, -2.25}, {-0.35, -1.6}, {-0.7, -0.6},
		{-0.95, -0.18}, {-2.6, 0.0}, {-0.95, 0.18},
		{-0.7, 0.6}, {-0.35, 1.6}, {-0.08, 2.25}, {0.18, 1.85}, {0.62, 0.95}};
	constexpr int32 NumOutline = UE_ARRAY_COUNT(Outline);
	// Which edges stir: the leading edges and the tips (1), the head's lobes a little (0.4), the trailing edges not.
	static const double Lead[] = {0.4, 0.4, 0.4, 0.4, 0.4, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
	auto Lift = [&](const FVector2D& Q) { return 0.45 * Stroke * FMath::Square(FMath::Abs(Q.Y) / 2.25); };
	auto World = [&](const FVector2D& Q) { return Pos + Fwd * Q.X + Side * Q.Y + Up * Lift(Q); };
	const double Down = FMath::Max(0.0, -FMath::Cos(Phase));   // the downstroke's speed
	for (int32 i = 0; i < NumOutline; ++i)
	{
		const FVector2D& A2 = Outline[i];
		const FVector2D& B2 = Outline[(i + 1) % NumOutline];
		const double Tip = FMath::Max(FMath::Abs(A2.Y), FMath::Abs(B2.Y)) / 2.25;
		const FVector WingV = Up * (-0.45 * 2.0 * PI / 5.0 * FMath::Cos(Phase) * Tip * Tip);
		const double Stirs = FMath::Min(Lead[i], Lead[(i + 1) % NumOutline]);
		if (Stirs <= 0.0) { continue; }
		const double Amount = (0.3 + 3.0 * Down * Tip) * DeltaSeconds * Stirs;
		SplatLine(World(A2), World(B2), 0.07, Amount * 4.0, Fwd * Speed * 0.3 + WingV);
	}
	// The wake: two swirls shed from the wing tips, and the water pushed down behind the body.
	for (const double S : {-1.0, 1.0})
	{
		const FVector TipP = World(FVector2D(-0.1, 2.2 * S)) - Fwd * 0.4;
		Splat(TipP, 0.08, (0.4 + 1.5 * Down) * DeltaSeconds, -Fwd * 0.3 + Side * S * 0.3 - Up * 0.2);
	}
	Splat(Pos - Fwd * 1.2 - Up * 0.3, 0.14, 0.5 * DeltaSeconds, -Up * 0.3 - Fwd * 0.2);
}

void ACubeSeaLight::Rain(float DeltaSeconds, float RatePerSecond)
{
	using namespace SeaLightPlan;
	RainCarry += RatePerSecond * DeltaSeconds;
	while (RainCarry >= 1.0)
	{
		RainCarry -= 1.0;
		const double A = Rng.FRandRange(0.0, 2.0 * PI), R = 6.0 * FMath::Sqrt(Rng.FRand());
		Rings.Add(FVector4(R * FMath::Cos(A), R * FMath::Sin(A), Surface, 0.0));
	}
	TArray<FTransform> Ts;
	TArray<float> Glow;
	for (int32 i = Rings.Num() - 1; i >= 0; --i)
	{
		FVector4& Ring = Rings[i];
		Ring.W += DeltaSeconds;
		if (Ring.W > 1.4) { Rings.RemoveAtSwap(i); continue; }
		// A ring spreading on the underside of the surface (about 0.3 m/s, slowing), bright at once, fading.
		const double Radius = 0.03 + 0.42 * (1.0 - FMath::Exp(-Ring.W / 0.6));
		const double G = FMath::Exp(-Ring.W / 0.45) * FMath::SmoothStep(0.0, 0.04, Ring.W);
		Ts.Add(FTransform(FQuat::Identity, FVector(Ring.X, Ring.Y, Ring.Z) * 100.0, FVector(Radius * 2.0 / 0.8, Radius * 2.0 / 0.8, 1.0)));
		Glow.Add(float(G));
		// Just under it the drop's splash stirs the water: a few sparks falling away.
		if (Ring.W < 0.15) { Splat(FVector(Ring.X, Ring.Y, Ring.Z - 0.15), 0.07, 6.0 * DeltaSeconds, FVector(0, 0, -0.12)); }
	}
	if (RingMeshes)
	{
		RingMeshes->ClearInstances();
		if (Ts.Num())
		{
			RingMeshes->AddInstances(Ts, false);
			for (int32 i = 0; i < Glow.Num(); ++i) { RingMeshes->SetCustomDataValue(i, 0, Glow[i] * Fade, false); }
			RingMeshes->MarkRenderStateDirty();
		}
		const bool bShow = Ts.Num() > 0;
		if (RingMeshes->IsVisible() != bShow) { RingMeshes->SetVisibility(bShow); }
	}
}

void ACubeSeaLight::SetSound(int32 Voice, float Value, const FVector& Where)
{
	if (!Waves.IsValidIndex(Voice) || !Waves[Voice] || !Voices[Voice]) { return; }
	Waves[Voice]->Level.store(FMath::Clamp(Value, 0.f, 1.f), std::memory_order_relaxed);
	Voices[Voice]->SetWorldLocation(Eyes() + Where * 100.0);
}

void ACubeSeaLight::Score(float DeltaSeconds)
{
	using namespace SeaLightPlan;
	// The score (PROPOSAL § 1, II): the minute marks.
	auto Ramp = [](double X, double A, double B) { return FMath::Clamp((X - A) / (B - A), 0.0, 1.0); };
	// Far sparks: something small moved; later the bloom's own life all round, and now and then close by the rail.
	const double Ambient = T < 45.0 ? 0.0 : T < 60.0 ? 2.0 : T < 150.0 ? 9.0 : T < 420.0 ? 16.0 : 16.0 * (1.0 - Ramp(T, 420.0, 470.0));
	AmbientCarry += Ambient * DeltaSeconds;
	while (AmbientCarry >= 1.0)
	{
		AmbientCarry -= 1.0;
		if (Rng.FRand() < 0.2f)
		{
			// A small fish or a copepod darting past just beyond the rail: a short streak within 30 cm of it.
			const double A = Rng.FRandRange(0.0, 2.0 * PI), R = RailR + Rng.FRandRange(0.1, 0.3);
			const FVector P(R * FMath::Cos(A), R * FMath::Sin(A), RailZ + Rng.FRandRange(-0.4, 0.6));
			const FVector V = FVector(-FMath::Sin(A), FMath::Cos(A), Rng.FRandRange(-0.2, 0.2)) * Rng.FRandRange(0.6, 1.4) * (Rng.FRand() < 0.5f ? -1.0 : 1.0);
			SplatLine(P, P + V * 0.25, 0.06, 2.0, V);
			continue;
		}
		const FVector D = Rng.GetUnitVector();
		const double R = Rng.FRandRange(3.0, 5.8);
		const FVector V = Rng.GetUnitVector() * Rng.FRandRange(0.15, 0.6);
		const FVector P(D.X * R, D.Y * R, FMath::Clamp(D.Z * R, -5.8, 4.3));
		SplatLine(P, P + V * 0.3, Rng.FRandRange(0.07f, 0.16f), Rng.FRandRange(1.8f, 3.2f), V);
	}
	if (!bToldReach && T >= 60.0)
	{
		bToldReach = true;
		if (AMuseeCharacter* Visitor = Cast<AMuseeCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
		{
			Visitor->SetMessage(LOCTEXT("Reach", "Reach out and stir the water (hold the right mouse button, and move)."));
		}
	}
	// The shoal of scad, 2:30 to 4:00, from the west.
	MoveShoal(DeltaSeconds, float(T >= 150.0 && T < 240.0 ? 1.0 : 0.0));
	// The manta, 4:30 to 5:30 (it crosses in about 34 s).
	const bool bManta = T >= 270.0 && T < 330.0;
	if (bManta) { MoveManta(DeltaSeconds, T - 272.0); }
	else if (MantaMesh && MantaMesh->IsVisible()) { MantaMesh->SetVisibility(false); }
	// Rain on the surface overhead, 6:00 to 7:00.
	const float RainRate = float(T >= 360.0 && T < 420.0 ? 18.0 * FMath::Min(1.0, Ramp(T, 360.0, 368.0)) * (1.0 - Ramp(T, 412.0, 420.0)) : 0.0);
	Rain(DeltaSeconds, RainRate);
	// The bloom thins, 7:00 to 8:00: the last sparks follow only the hand.
	const float Thin = float(Ramp(T, 420.0, 478.0));
	for (UMaterialInstanceDynamic* M : CellMaterials) { if (M) { M->SetScalarParameterValue(TEXT("Thin"), Thin); } }
	// Sound.
	SetSound(0, Fade * (0.6f + 0.4f * float(1.0 - Ramp(T, 440.0, 480.0))), FVector::ZeroVector);
	HandFizz = FMath::FInterpTo(HandFizz, FMath::Clamp(HandSpeed / 1.5f, 0.f, 1.f) * (bHandValid ? 1.f : 0.f), DeltaSeconds, 10.f);
	SetSound(1, HandFizz * Fade, HandPos);
	SetSound(2, ShoalHiss * Fade, ShoalCentre);
	SetSound(3, RainRate / 18.f * Fade, FVector(0, 0, Surface));
	// The touch (the ultrasound's stand-in): in the headset the right controller buzzes finely where the sparks are, and
	// the shoal's pressure wave passes through it; on the desktop the fizz is the placed tick at the hand.
	if (bHandVR)
	{
		if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		{
			const float ShoalNear = ShoalHiss * FMath::Clamp(1.f - float((ShoalCentre - HandPos).Size() - 2.0) / 4.f, 0.f, 1.f);
			const float Amp = FMath::Clamp(0.35f * HandFizz + 0.3f * ShoalNear, 0.f, 0.6f) * Fade;
			PC->SetHapticsByValue(Amp > 0.02f ? 0.8f : 0.f, Amp, EControllerHand::Right);
		}
	}
	if (T >= Length && !bClosing) { bFinished = true; End(); }
}

void ACubeSeaLight::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bPlaying) { SetActorTickEnabled(false); return; }
	const float Dt = DeltaSeconds * TimeScale;
	// The visitor gone from the Cube: the piece ends.
	if (APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		const FVector E = (Camera->GetCameraLocation() - Eyes()) / 100.0;
		if (E.GetAbsMax() > CubePlan::Half && !bClosing) { End(); }
	}
	Fade = bClosing ? FMath::Max(0.f, Fade - DeltaSeconds / 3.f) : FMath::Min(1.f, Fade + DeltaSeconds / 2.f);
	if (bClosing && Fade <= 0.f)
	{
		bPlaying = false;
		bClosing = false;
		for (UAudioComponent* A : Voices) { if (A) { A->Stop(); } }
		if (bHandVR) { if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0)) { PC->SetHapticsByValue(0.f, 0.f, EControllerHand::Right); } }
		Fish.Reset();
		Rings.Reset();
		SetRoomBlack(false);
		SetActorTickEnabled(false);
		UE_LOG(LogMusee, Log, TEXT("Sea Light: the room at rest."));
		return;
	}
	if (!bClosing) { T += Dt; }
	const double C0 = FPlatformTime::Seconds();
	Decay(Dt);
	const double C1 = FPlatformTime::Seconds();
	MoveHand(DeltaSeconds);
	if (!bClosing) { Score(Dt); }
	const double C2 = FPlatformTime::Seconds();
	if (TestFill >= 0.f)
	{
		for (int32 i = 0; i < Stir.Num(); ++i) { Stir[i] = TestFill; Flow[i] = FVector3f(0.8f, 0.f, 0.f) * TestFill; }
		for (int32 z = 0; z < N; ++z) { SliceLive[z] = 1; }
	}
	LogClock += DeltaSeconds;
	if (LogClock > 2.f)
	{
		LogClock = 0.f;
		float Max = 0.f; int32 Lit = 0;
		for (const float F : Stir) { Max = FMath::Max(Max, F); Lit += F > 0.3f; }
		UE_LOG(LogMusee, Verbose, TEXT("Sea Light: %.0f s, fade %.2f, field max %.2f, %d cells over 0.3, hand %s, %d fish."),
			T, Fade, Max, Lit, bHandValid ? TEXT("on") : TEXT("off"), Fish.Num());
	}
	Upload();
	const double C3 = FPlatformTime::Seconds();
	if (bProfile)
	{
		ProfMax[0] = FMath::Max(ProfMax[0], float(C1 - C0) * 1000.f);
		ProfMax[1] = FMath::Max(ProfMax[1], float(C2 - C1) * 1000.f);
		ProfMax[2] = FMath::Max(ProfMax[2], float(C3 - C2) * 1000.f);
		ProfMax[3] = FMath::Max(ProfMax[3], DeltaSeconds * 1000.f);
		ProfClock += DeltaSeconds;
		if (ProfClock > 2.f)
		{
			int32 Live = 0;
			for (const uint8 L : SliceLive) { Live += L; }
			FString Sound;
			for (int32 v = 0; v < Waves.Num(); ++v)
			{
				if (Waves[v]) { Sound += FString::Printf(TEXT(" %d:%.3f/%d%s"), v, Waves[v]->Rms.load(), Waves[v]->Buffers.load(), Voices[v] && Voices[v]->IsPlaying() ? TEXT("") : TEXT("(stopped)")); }
			}
			UE_LOG(LogMusee, Log, TEXT("Sea Light profile (max over 2 s, ms): decay %.2f, movers %.2f, upload %.2f, frame %.1f; %d live slices, %d fish; sound%s."),
				ProfMax[0], ProfMax[1], ProfMax[2], ProfMax[3], Live, Fish.Num(), *Sound);
			ProfClock = 0.f;
			for (float& P : ProfMax) { P = 0.f; }
		}
	}
	// The cells' parameters: where the field is, how big a pixel is (sparks keep at least a pixel and a half), the light.
	float PixelAngle = 0.0012f;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		int32 SX = 0, SY = 0;
		PC->GetViewportSize(SX, SY);
		const float Fov = PC->PlayerCameraManager ? PC->PlayerCameraManager->GetFOVAngle() : 90.f;
		if (SX > 0) { PixelAngle = 2.f * FMath::Tan(FMath::DegreesToRadians(Fov) * 0.5f) / float(SX); }
	}
	for (UMaterialInstanceDynamic* M : CellMaterials)
	{
		if (!M) { continue; }
		M->SetVectorParameterValue(TEXT("FieldOrigin"), FLinearColor(FieldOrigin.X, FieldOrigin.Y, FieldOrigin.Z, 0.f));
		M->SetScalarParameterValue(TEXT("PixelAngle"), PixelAngle);
		M->SetScalarParameterValue(TEXT("Fade"), Fade);
		M->SetScalarParameterValue(TEXT("PeakNits"), PeakNits);
	}
}

// ---------------------------------------------------------------------------------------------
// Console: musee.Piece <2 Sea Light | 0 end> [minute] [time scale]; musee.SeaLightHand <0|1|2>; musee.SeaLightTest <v>

namespace
{
	FAutoConsoleCommandWithWorldAndArgs GPieceCommand(
		TEXT("musee.Piece"),
		TEXT("musee.Piece <2 Sea Light | 0 end> [minute] [time scale]: play the Cube's piece from that minute, the visitor in the car at the centre."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (!World) { return; }
			const int32 Which = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 2;
			ACubeSeaLight* Piece = ACubeSeaLight::Get(World);
			if (!Piece) { return; }
			if (Which != 2) { Piece->End(); return; }
			Piece->TimeScale = Args.Num() > 2 ? FMath::Max(0.f, FCString::Atof(*Args[2])) : 1.f;
			Piece->Begin(Args.Num() > 1 ? 60.0 * FCString::Atod(*Args[1]) : 0.0);
		}));

	FAutoConsoleCommandWithWorldAndArgs GProfileCommand(
		TEXT("musee.SeaLightProfile"),
		TEXT("musee.SeaLightProfile <0|1>: log Sea Light's game-thread costs every 2 s."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld*)
		{
			ACubeSeaLight::bProfile = Args.Num() > 0 && FCString::Atoi(*Args[0]) != 0;
		}));

	FAutoConsoleCommandWithWorldAndArgs GTestCommand(
		TEXT("musee.SeaLightTest"),
		TEXT("musee.SeaLightTest <v>: Sea Light's field held at v everywhere (tests of the cells; below 0 off)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld*)
		{
			ACubeSeaLight::TestFill = Args.Num() > 0 ? FCString::Atof(*Args[0]) : -1.f;
		}));

	FAutoConsoleCommandWithWorldAndArgs GHandCommand(
		TEXT("musee.SeaLightHand"),
		TEXT("musee.SeaLightHand <0|1|2>: Sea Light's hand always on (tests; on the desktop it is the right mouse button); 2 also marks where it is."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld*)
		{
			const int32 Mode = Args.Num() > 0 ? FCString::Atoi(*Args[0]) : 0;
			ACubeSeaLight::bForceHand = Mode != 0;
			ACubeSeaLight::bShowHand = Mode == 2;
		}));
}

#undef LOCTEXT_NAMESPACE
