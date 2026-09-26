#include "SunClock/SunClock.h"

#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/CollisionProfile.h"
#include "Geometry/MuseeBake.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "MuseeVision.h"
#include "Plan/MuseePlan.h"
#include "ProceduralMeshComponent.h"
#include "SunClock/SunClockBuild.h"
#include "Visitor/MuseeCharacter.h"

#define LOCTEXT_NAMESPACE "SunClock"

/** The actor's own numbers (a named namespace: the module builds in unity files). */
namespace SunClockActor
{
	namespace SC = MuseePlan::SunClock;
	constexpr double RingFrom = 7.35;        // "on the ring": the capsule (r 0.3) clear of the drum
	constexpr double RingTo = 10.0;
	constexpr double InsideRadius = 7.2;     // never lower with anyone inside this
	constexpr double LightsFrom = 0.3;       // m of lift before the shaft's lights come on
}

namespace SCA = SunClockActor;

const FName ASunClock::RaisePromptId(TEXT("sunclock.raise"));
const FName ASunClock::LowerPromptId(TEXT("sunclock.lower"));

ASunClock::ASunClock()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	auto Make = [this](const TCHAR* ComponentName, USceneComponent* Parent, bool bCollision)
	{
		UProceduralMeshComponent* C = CreateDefaultSubobject<UProceduralMeshComponent>(ComponentName);
		C->SetupAttachment(Parent);
		C->SetMobility(EComponentMobility::Movable);
		C->bUseAsyncCooking = true;
		if (bCollision)
		{
			C->bUseComplexAsSimpleCollision = true;
			C->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			C->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		else
		{
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		return C;
	};
	Drum = Make(TEXT("Drum"), RootComponent, true);
	DrumBronze = Make(TEXT("DrumBronze"), Drum, false);
	Ring = Make(TEXT("Ring"), RootComponent, true);
	RingBronze = Make(TEXT("RingBronze"), RootComponent, false);
	Shaft = Make(TEXT("Shaft"), RootComponent, true);
	Stair = Make(TEXT("Stair"), RootComponent, true);
	Rails = Make(TEXT("Rails"), RootComponent, false);
	TopRails = Make(TEXT("TopRails"), RootComponent, true);
	Walkway = Make(TEXT("Walkway"), RootComponent, true);
	Walkway->SetVisibility(false);
	Walkway->SetHiddenInGame(true);
	Walkway->SetCastShadow(false);
	// What moves with the drum, the retracting balustrade and the hidden walkway stay procedural; the fixed
	// ring, shaft, stair, fixed rails and the balustrade's housings may be baked (Geometry/MuseeBake.h).
	for (UActorComponent* C : {static_cast<UActorComponent*>(Drum), static_cast<UActorComponent*>(DrumBronze),
							   static_cast<UActorComponent*>(TopRails), static_cast<UActorComponent*>(Walkway)})
	{
		MuseeBake::NoBake(C);
	}

	for (int32 I = 0; I < SunClockBuild::CoveCount; ++I)
	{
		USpotLightComponent* L = CreateDefaultSubobject<USpotLightComponent>(*FString::Printf(TEXT("CoveLight%02d"), I + 1));
		L->SetupAttachment(Drum);
		L->SetMobility(EComponentMobility::Movable);
		CoveLights.Add(L);
	}
	WellLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("WellLight"));
	WellLight->SetupAttachment(Drum);
	WellLight->SetMobility(EComponentMobility::Movable);
	FillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FillLight"));
	FillLight->SetupAttachment(Drum);
	FillLight->SetMobility(EComponentMobility::Movable);

	auto Path = [](const TCHAR* Folder, const TCHAR* Name)
	{
		return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(FString::Printf(TEXT("%s/%s.%s"), Folder, Name, Name)));
	};
	const TCHAR* Materials = TEXT("/Game/Museum/Materials");
	const TCHAR* Imported = TEXT("/Game/Museum/Materials/USD");
	MarbleMaterial = Path(Materials, TEXT("M_SunClock_Marble"));
	MarbleAltMaterial = Path(Materials, TEXT("M_SunClock_MarbleAlt"));
	MarbleFallbackMaterial = Path(Materials, TEXT("M_Marble_Polished"));
	RossoMaterial = Path(Materials, TEXT("M_Marble_Rosso"));
	NeroMaterial = Path(Materials, TEXT("M_Marble_Nero"));
	DarkFallbackMaterial = Path(Imported, TEXT("MI_floor_dark"));
	DrumMaterial = Path(Materials, TEXT("M_SunClock_Drum"));
	WallMaterial = Path(Materials, TEXT("M_Marble_Wall"));
	SoffitMaterial = Path(Materials, TEXT("M_Plaster_Coffer"));
	BronzeMaterial = Path(Imported, TEXT("MI_bronze_gold_r40"));
	GiltMaterial = Path(Materials, TEXT("M_Gilt_Aged"));
	GiltFallbackMaterial = Path(Materials, TEXT("M_Gilt"));

	AddTags();
	PlaceLights();
}

TArray<FString> ASunClock::GetReplacedImportPrims()
{
	return {TEXT("/Museum/Rotunda/Sun_clock")};
}

float ASunClock::GetLiftHeight() const
{
	const double T = FMath::Clamp(Progress, 0.0, 1.0);
	return float(T * T * (3 - 2 * T) * SCA::SC::Lift);
}

bool ASunClock::IsRaised() const { return State == EState::Up; }

FVector ASunClock::GetLandingPoint() const
{
	return GetActorTransform().TransformPosition(MuseePlan::At(0, -5.2, SCA::SC::Floor));
}

void ASunClock::SetLiftFraction(float Fraction)
{
	Progress = FMath::Clamp(double(Fraction), 0.0, 1.0);
	State = Progress >= 1.0 ? EState::Up : (Progress <= 0.0 ? EState::Down : EState::Held);
	Apply();
}

void ASunClock::AddTags()
{
	Tags.AddUnique(FName(TEXT("musee.building")));
	Tags.AddUnique(FName(TEXT("musee.wing:Rotunda")));
	// It ticks and offers prompts, but only what is tagged musee.nobake moves.
	Tags.AddUnique(MuseeBake::BakeableTag());
}

void ASunClock::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	AddTags();   // a placing script may have set the actor's tags
	Build();
	ApplyMaterials();
	PlaceLights();
	Apply();
}

void ASunClock::BeginPlay()
{
	Super::BeginPlay();
	AddTags();
	// A placed actor doesn't rerun its construction when the map loads: rebuild, so the map never
	// shows an older build.
	Build();
	ApplyMaterials();
	PlaceLights();
	Apply();
}

void ASunClock::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }   // baked into static meshes (Geometry/MuseeBake.h); the rest keeps its saved sections
	SunClockBuild::FSunClockMeshes M;
	SunClockBuild::BuildAll(M);
	for (UProceduralMeshComponent* C : {Drum.Get(), DrumBronze.Get(), Ring.Get(), RingBronze.Get(), Shaft.Get(), Stair.Get(), Rails.Get(), TopRails.Get(), Walkway.Get()})
	{
		if (C) { C->ClearAllMeshSections(); }
	}
	M.DialA.Write(Drum, 0, true);
	M.DialB.Write(Drum, 1, true);
	M.DialRosso.Write(Drum, 2, true);
	M.DialNero.Write(Drum, 3, true);
	M.DrumStone.Write(Drum, 4, true);
	M.DrumSoffit.Write(Drum, 5, false);
	M.DrumBronze.Write(DrumBronze, 0, false);
	M.DrumGilt.Write(DrumBronze, 1, false);
	M.RingA.Write(Ring, 0, true);
	M.RingB.Write(Ring, 1, true);
	M.RingBronze.Write(RingBronze, 0, false);
	M.RingGilt.Write(RingBronze, 1, false);
	M.Masonry.Write(Shaft, 0, true);
	M.FloorA.Write(Shaft, 1, true);
	M.FloorRosso.Write(Shaft, 2, true);
	M.FloorNero.Write(Shaft, 3, true);
	M.PortStone.Write(Shaft, 4, true);
	M.Treads.Write(Stair, 0, false);
	M.Curb.Write(Stair, 1, true);
	M.StairSoffit.Write(Stair, 2, true);
	M.RailBronze.Write(Rails, 0, false);
	M.Housing.Write(Rails, 1, false);
	M.HousingStone.Write(Rails, 2, false);
	M.TopRailBronze.Write(TopRails, 0, false);
	M.TopRailFence.Write(TopRails, 1, true);
	M.Ramp.Write(Walkway, 0, true);
	M.RailFence.Write(Walkway, 1, true);
	// What the visitor walks on and the fences are felt, not seen.
	TopRails->SetMeshSectionVisible(1, false);
}

void ASunClock::ApplyMaterials()
{
	auto Pick = [](const TSoftObjectPtr<UMaterialInterface>& Wanted, const TSoftObjectPtr<UMaterialInterface>& Fallback) -> UMaterialInterface*
	{
		if (UMaterialInterface* Loaded = Wanted.LoadSynchronous()) { return Loaded; }
		return Fallback.IsNull() ? nullptr : Fallback.LoadSynchronous();
	};
	auto Set = [](UProceduralMeshComponent* C, int32 Section, UMaterialInterface* Material)
	{
		if (C && Material) { C->SetMaterial(Section, Material); }
	};
	UMaterialInterface* Marble = Pick(MarbleMaterial, MarbleFallbackMaterial);
	UMaterialInterface* MarbleAlt = Pick(MarbleAltMaterial, MarbleFallbackMaterial);
	UMaterialInterface* Rosso = Pick(RossoMaterial, DarkFallbackMaterial);
	UMaterialInterface* Nero = Pick(NeroMaterial, DarkFallbackMaterial);
	UMaterialInterface* Wall = Pick(WallMaterial, MarbleFallbackMaterial);
	UMaterialInterface* DrumStone = Pick(DrumMaterial, WallMaterial);
	UMaterialInterface* Soffit = Pick(SoffitMaterial, WallMaterial);
	UMaterialInterface* Bronze = Pick(BronzeMaterial, GiltMaterial);
	UMaterialInterface* Gilt = Pick(GiltMaterial, GiltFallbackMaterial);
	Set(Drum, 0, Marble);
	Set(Drum, 1, MarbleAlt);
	Set(Drum, 2, Rosso);
	Set(Drum, 3, Nero);
	Set(Drum, 4, DrumStone);
	Set(Drum, 5, Soffit);
	Set(DrumBronze, 0, Bronze);
	Set(DrumBronze, 1, Gilt);
	Set(Ring, 0, Marble);
	Set(Ring, 1, MarbleAlt);
	Set(RingBronze, 0, Bronze);
	Set(RingBronze, 1, Gilt);
	Set(Shaft, 0, Wall);
	Set(Shaft, 1, Marble);
	Set(Shaft, 2, Rosso);
	Set(Shaft, 3, Nero);
	Set(Shaft, 4, Marble);
	Set(Stair, 0, Marble);
	Set(Stair, 1, Marble);
	Set(Stair, 2, Wall);
	Set(Rails, 0, Bronze);
	Set(Rails, 1, Bronze);
	Set(Rails, 2, Marble);
	Set(TopRails, 0, Bronze);
	Set(TopRails, 1, Bronze);
	Set(Walkway, 0, Wall);
	Set(Walkway, 1, Wall);
}

void ASunClock::PlaceLights()
{
	auto Setup = [this](USpotLightComponent* L, const FVector& AtMetres, float Candela, float Inner, float Outer, float RangeCm, float SourceCm)
	{
		if (!L) { return; }
		L->SetRelativeLocationAndRotation(AtMetres * MuseePlan::Cm, FRotator(-90, 0, 0));
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(Candela);
		L->SetUseTemperature(true);
		L->SetTemperature(LightKelvin);
		L->SetLightColor(FLinearColor::White);
		L->SetInnerConeAngle(Inner);
		L->SetOuterConeAngle(Outer);
		L->SetAttenuationRadius(RangeCm);
		L->SetSourceRadius(SourceCm);
		L->SetSoftSourceRadius(0.f);
		L->SetCastShadows(true);
	};
	for (int32 I = 0; I < CoveLights.Num(); ++I)
	{
		// In every other coffer of the fourth ring (24 to the round), just under its panel.
		Setup(CoveLights[I], SunClockBuild::Polar(SunClockBuild::CoveRadius, SunClockBuild::CovePhi(I), SunClockBuild::CoveZ), CoveCandela, 24.f, 60.f, 1500.f, 6.f);
	}
	Setup(WellLight, FVector(0, 0, SunClockBuild::WellLightZ), WellCandela, 8.f, 16.f, 1400.f, 4.f);
	if (FillLight)
	{
		// 2.4 m under the coffers, on the well's axis (raised: 1.1 m over the floor, in the open well).
		FillLight->SetRelativeLocation(FVector(0, 0, SunClockBuild::FillLightZ) * MuseePlan::Cm);
		FillLight->SetIntensityUnits(ELightUnits::Candelas);
		FillLight->SetIntensity(FillCandela);
		FillLight->SetUseTemperature(true);
		FillLight->SetTemperature(LightKelvin);
		FillLight->SetLightColor(FLinearColor::White);
		FillLight->SetAttenuationRadius(1500.f);
		FillLight->SetSourceRadius(40.f);
		FillLight->SetSoftSourceRadius(40.f);
		FillLight->SetCastShadows(true);
	}
}

void ASunClock::Apply()
{
	const double Height = GetLiftHeight();
	if (Drum) { Drum->SetRelativeLocation(FVector(0, 0, Height * MuseePlan::Cm)); }
	// The retracting balustrade is driven from the lift through a cam (SunClockBuild::PanelOffset): down in its housings
	// until the drum is well clear, then up with it, then standing.
	if (TopRails) { TopRails->SetRelativeLocation(FVector(0, 0, SunClockBuild::PanelOffset(Height) * MuseePlan::Cm)); }
	const bool bLit = Height > SCA::LightsFrom;
	for (USpotLightComponent* L : CoveLights)
	{
		if (L) { L->SetVisibility(bLit); }
	}
	if (WellLight) { WellLight->SetVisibility(bLit); }
	if (FillLight) { FillLight->SetVisibility(bLit); }
}

FVector ASunClock::LocalFeet(const AMuseeCharacter* Visitor) const
{
	return GetActorTransform().InverseTransformPosition(Visitor->FeetLocation()) / MuseePlan::Cm;
}

bool ASunClock::OnRing(const AMuseeCharacter* Visitor) const
{
	if (!Visitor) { return false; }
	const FVector F = LocalFeet(Visitor);
	const double R = FVector2D(F.X, F.Y).Size();
	return R > SCA::RingFrom && R < SCA::RingTo && FMath::Abs(F.Z) < 0.4;
}

bool ASunClock::OnDial(const AMuseeCharacter* Visitor) const
{
	if (!Visitor) { return false; }
	const FVector F = LocalFeet(Visitor);
	return FVector2D(F.X, F.Y).Size() <= SCA::RingFrom && F.Z > -0.3 && F.Z < 0.6;
}

bool ASunClock::InsideOrBelow(const AMuseeCharacter* Visitor) const
{
	if (!Visitor) { return false; }
	const FVector F = LocalFeet(Visitor);
	return FVector2D(F.X, F.Y).Size() < SCA::InsideRadius || F.Z < -0.3;
}

void ASunClock::GetPrompts(const AMuseeCharacter* Visitor, TArray<FMuseeActionPrompt>& Out) const
{
	if (!OnRing(Visitor)) { return; }
	if (State == EState::Down) { Out.Add({RaisePromptId, LOCTEXT("Raise", "Raise the sun clock")}); }
	else if (State == EState::Up) { Out.Add({LowerPromptId, LOCTEXT("Lower", "Lower the sun clock")}); }
}

void ASunClock::RunPrompt(AMuseeCharacter* Visitor, FName Id)
{
	if (!Visitor) { return; }
	if (Id == RaisePromptId && State == EState::Down)
	{
		if (OnDial(Visitor))
		{
			Visitor->SetMessage(LOCTEXT("StepOff", "Step off the sun clock to raise it."));
			return;
		}
		if (!OnRing(Visitor)) { return; }
		State = EState::Rising;
		Visitor->SetMessage(LOCTEXT("Rising", "The sun clock rises…"));
	}
	else if (Id == LowerPromptId && State == EState::Up)
	{
		if (!OnRing(Visitor) || InsideOrBelow(Visitor)) { return; }
		State = EState::Lowering;
		Visitor->SetMessage(FText::GetEmpty());
	}
}

void ASunClock::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AMuseeCharacter* Visitor = Cast<AMuseeCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	const double Step = DeltaSeconds / SCA::SC::LiftSeconds;
	switch (State)
	{
	case EState::Rising:
		Progress = FMath::Min(1.0, Progress + Step);
		if (Progress >= 1.0)
		{
			State = EState::Up;
			if (Visitor) { Visitor->SetMessage(LOCTEXT("Up", "Under the sun clock, a stair winds down into the earth.")); }
		}
		break;
	case EState::Lowering:
		// Never onto anyone: back up if the visitor comes in under it or is below.
		if (Visitor && InsideOrBelow(Visitor))
		{
			State = EState::Rising;
			break;
		}
		Progress = FMath::Max(0.0, Progress - Step);
		if (Progress <= 0.0) { State = EState::Down; }
		break;
	case EState::Up:
		if (Visitor && LocalFeet(Visitor).Z < -1.0 && Visitor->Message().ToString().StartsWith(TEXT("Under the sun clock")))
		{
			Visitor->SetMessage(FText::GetEmpty());
		}
		break;
	case EState::Down:
	case EState::Held:
		break;
	}
	Apply();
}

#undef LOCTEXT_NAMESPACE
