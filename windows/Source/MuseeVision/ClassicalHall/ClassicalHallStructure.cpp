#include "ClassicalHall/ClassicalHallStructure.h"

#include "ClassicalHall/ClassicalHallGeometry.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/CollisionProfile.h"
#include "Geometry/MuseeBake.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Plan/MuseePlan.h"
#include "ProceduralMeshComponent.h"

/**
 * The actor's side of the classical hall: the meshes from ClassicalHallGeometry written into procedural components (with
 * tangents from the UVs), the materials (with stand-ins until Scripts/materials.py has made the hall's named marbles),
 * the lights and the places for works of art.
 */
namespace ClassicalHallActor
{
	using ClassicalHallGeometry::EPart;
	using ClassicalHallGeometry::FHall;
	using ClassicalHallKit::FMesh;

	/** Writes a mesh (metres) as a section (centimetres), with tangents from its UVs. */
	void WriteSection(UProceduralMeshComponent* Target, int32 Section, const FMesh& M, bool bCollision)
	{
		if (!Target) { return; }
		if (M.Indices.Num() == 0)
		{
			Target->ClearMeshSection(Section);
			return;
		}
		TArray<FVector> Positions;
		Positions.Reserve(M.Positions.Num());
		for (const FVector& P : M.Positions) { Positions.Add(P * MuseePlan::Cm); }
		TArray<FVector> SumX, SumY;
		SumX.Init(FVector::ZeroVector, M.Positions.Num());
		SumY.Init(FVector::ZeroVector, M.Positions.Num());
		for (int32 t = 0; t + 2 < M.Indices.Num(); t += 3)
		{
			const int32 I0 = M.Indices[t], I1 = M.Indices[t + 1], I2 = M.Indices[t + 2];
			const FVector E1 = M.Positions[I1] - M.Positions[I0], E2 = M.Positions[I2] - M.Positions[I0];
			const FVector2D D1 = M.UVs[I1] - M.UVs[I0], D2 = M.UVs[I2] - M.UVs[I0];
			const double Det = D1.X * D2.Y - D2.X * D1.Y;
			if (FMath::Abs(Det) < 1e-14) { continue; }
			const FVector TX = ClassicalHallKit::Unit((E1 * D2.Y - E2 * D1.Y) / Det);
			const FVector TY = ClassicalHallKit::Unit((E2 * D1.X - E1 * D2.X) / Det);
			SumX[I0] += TX; SumX[I1] += TX; SumX[I2] += TX;
			SumY[I0] += TY; SumY[I1] += TY; SumY[I2] += TY;
		}
		TArray<FProcMeshTangent> Tangents;
		Tangents.Reserve(M.Positions.Num());
		for (int32 i = 0; i < M.Positions.Num(); ++i)
		{
			const FVector& N = M.Normals[i];
			FVector TX = SumX[i] - N * FVector::DotProduct(N, SumX[i]);
			if (TX.SizeSquared() < 1e-20)
			{
				TX = FVector::CrossProduct(N, FMath::Abs(N.Z) < 0.9 ? FVector::UpVector : FVector::ForwardVector);
			}
			TX = ClassicalHallKit::Unit(TX);
			const bool bFlip = FVector::DotProduct(FVector::CrossProduct(N, TX), SumY[i]) < 0.0;
			Tangents.Add(FProcMeshTangent(TX, bFlip));
		}
		Target->CreateMeshSection(Section, Positions, M.Indices, M.Normals, M.UVs, TArray<FColor>(), Tangents, bCollision);
	}

	/** The light's component name: letters, digits and underscores. */
	FName ComponentName(const TCHAR* Prefix, const FString& Name)
	{
		FString S = FString(Prefix) + Name;
		for (TCHAR& C : S.GetCharArray())
		{
			if (C != 0 && !FChar::IsAlnum(C) && C != TEXT('_')) { C = TEXT('_'); }
		}
		return FName(*S);
	}

	/** A stand-in for a named marble that doesn't exist yet: M_Stone with its colour and polish. */
	struct FStandIn
	{
		uint32 Base = 0xEEECE7;
		uint32 Vein = 0xA9A49B;
		float VeinAmount = 0.f;
		float Roughness = 0.3f;
		float ClearCoat = 0.6f;
		float Joints = 0.f;
		float Pores = 0.f;
		float PoreDarkness = 0.72f;
	};

	FLinearColor Srgb(uint32 Hex) { return FLinearColor(FColor((Hex >> 16) & 0xFF, (Hex >> 8) & 0xFF, Hex & 0xFF)); }

	TSoftObjectPtr<UMaterialInterface> Path(const TCHAR* Name)
	{
		return TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(FString::Printf(TEXT("/Game/Museum/Materials/%s.%s"), Name, Name)));
	}
}

AClassicalHallStructure::AClassicalHallStructure()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	auto Make = [this](const TCHAR* ComponentName, bool bCollision)
	{
		UProceduralMeshComponent* Component = CreateDefaultSubobject<UProceduralMeshComponent>(ComponentName);
		Component->SetupAttachment(RootComponent);
		Component->bUseAsyncCooking = true;
		if (bCollision)
		{
			Component->bUseComplexAsSimpleCollision = true;
			Component->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
			Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		}
		else
		{
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		return Component;
	};
	Masonry = Make(TEXT("Masonry"), true);
	Order = Make(TEXT("Order"), false);
	Vaults = Make(TEXT("Vaults"), false);
	Laylights = Make(TEXT("Laylights"), false);
	Laylights->SetCastShadow(false);
	MuseeBake::NoBake(Laylights);   // its material is made at run time

	for (const ClassicalHallGeometry::FLight& L : ClassicalHallGeometry::Lights())
	{
		if (L.Type == ClassicalHallGeometry::FLight::EType::Rect)
		{
			URectLightComponent* C = CreateDefaultSubobject<URectLightComponent>(ClassicalHallActor::ComponentName(TEXT("Area_"), L.Name));
			C->SetupAttachment(RootComponent);
			C->SetMobility(EComponentMobility::Movable);
			AreaLights.Add(C);
		}
		else
		{
			USpotLightComponent* C = CreateDefaultSubobject<USpotLightComponent>(ClassicalHallActor::ComponentName(TEXT("Spot_"), L.Name));
			C->SetupAttachment(RootComponent);
			C->SetMobility(EComponentMobility::Movable);
			AccentLights.Add(C);
		}
	}

	using ClassicalHallActor::Path;
	WallMaterial = Path(TEXT("M_Marble_Wall"));
	DadoMaterial = Path(TEXT("M_Marble_RossoAntico"));
	NicheMaterial = Path(TEXT("M_Stucco_Pompeian"));
	FloorMaterial = Path(TEXT("M_Marble_Polished"));
	GialloMaterial = Path(TEXT("M_Marble_GialloAntico"));
	PorphyryMaterial = Path(TEXT("M_Porphyry"));
	VerdeMaterial = Path(TEXT("M_Marble_VerdeAntico"));
	RossoMaterial = Path(TEXT("M_Marble_RossoAntico"));
	CarvedMaterial = Path(TEXT("M_Marble_Carved"));
	PavonazzettoMaterial = Path(TEXT("M_Marble_Pavonazzetto"));
	VaultMaterial = Path(TEXT("M_Plaster_Coffer"));
	GiltMaterial = Path(TEXT("M_Gilt_Aged"));
	LaylightMaterial = Path(TEXT("MI_light_grid_FFFFFF"));
	ShellMaterial = Path(TEXT("M_Travertine_Honed"));
	StoneMaster = Path(TEXT("M_Stone"));

	PlaceLights();
	AddTags();
}

void AClassicalHallStructure::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
	ApplyMaterials(false);
	PlaceLights();
	AddTags();
}

void AClassicalHallStructure::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	// Before anyone's BeginPlay: MuseeSky gathers laylights (and their full intensity) in its own, if they follow the sky.
	PlaceLights();
	AddTags();
}

void AClassicalHallStructure::BeginPlay()
{
	Super::BeginPlay();
	// A placed actor doesn't rerun its construction when the map loads: rebuild, so the map never shows an older build.
	Build();
	ApplyMaterials(true);
}

void AClassicalHallStructure::AddTags()
{
	Tags.AddUnique(FName(TEXT("musee.building")));
	Tags.AddUnique(FName(TEXT("musee.wing:ClassicalHall")));
	Tags.RemoveAll([](const FName& Tag) { return Tag.ToString().StartsWith(TEXT("laylight.night:")) || Tag == FName(TEXT("musee.laylight")); });
	if (bLaylightsFollowDaylight)
	{
		// MuseeSky scales musee.laylight actors' lights by max(Daylight, the night floor), as M_Daylit.
		Tags.AddUnique(FName(TEXT("musee.laylight")));
		Tags.Add(FName(TEXT("laylight.night:0.6")));
	}
}

void AClassicalHallStructure::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }   // baked into static meshes (Geometry/MuseeBake.h)
	using ClassicalHallActor::WriteSection;
	using ClassicalHallGeometry::EPart;
	using ClassicalHallGeometry::FHall;
	const FHall Hall = ClassicalHallGeometry::Build();
	for (UProceduralMeshComponent* Component : {Masonry.Get(), Order.Get(), Vaults.Get(), Laylights.Get()})
	{
		if (Component) { Component->ClearAllMeshSections(); }
	}
	WriteSection(Masonry, 0, Hall[EPart::Wall], true);
	WriteSection(Masonry, 1, Hall[EPart::Dado], true);
	WriteSection(Masonry, 2, Hall[EPart::Niche], true);
	WriteSection(Masonry, 3, Hall[EPart::FloorWhite], true);
	WriteSection(Masonry, 4, Hall[EPart::FloorGiallo], true);
	WriteSection(Masonry, 5, Hall[EPart::FloorPorphyry], true);
	WriteSection(Masonry, 6, Hall[EPart::FloorVerde], true);
	WriteSection(Masonry, 7, Hall[EPart::FloorRosso], true);
	WriteSection(Masonry, 8, Hall[EPart::Shell], false);
	WriteSection(Masonry, 9, Hall[EPart::Collision], true);
	if (Masonry) { Masonry->SetMeshSectionVisible(9, false); }
	WriteSection(Order, 0, Hall[EPart::Carved], false);
	WriteSection(Order, 1, Hall[EPart::ShaftGiallo], false);
	WriteSection(Order, 2, Hall[EPart::ShaftPavonazzetto], false);
	WriteSection(Order, 3, Hall[EPart::Panel], false);
	WriteSection(Vaults, 0, Hall[EPart::Vault], false);
	WriteSection(Vaults, 1, Hall[EPart::Gilt], false);
	WriteSection(Laylights, 0, Hall[EPart::Laylight], false);
}

void AClassicalHallStructure::ApplyMaterials(bool bRuntime)
{
	using ClassicalHallActor::FStandIn;
	using ClassicalHallActor::Srgb;
	if (bRuntime) { Fallbacks.Reset(); }
	// A named material, or (at run time, while it doesn't exist yet) a stand-in made from the stone master, or in the editor
	// an existing marble (the map must only refer to assets).
	auto Pick = [this, bRuntime](const TSoftObjectPtr<UMaterialInterface>& Named, const FStandIn* StandIn) -> UMaterialInterface*
	{
		if (UMaterialInterface* M = Named.LoadSynchronous()) { return M; }
		if (!StandIn) { return nullptr; }
		UMaterialInterface* Master = StoneMaster.LoadSynchronous();
		if (!bRuntime || !Master) { return ClassicalHallActor::Path(TEXT("M_Marble_Polished")).LoadSynchronous(); }
		UMaterialInstanceDynamic* D = UMaterialInstanceDynamic::Create(Master, this);
		D->SetFlags(RF_Transient);
		D->SetVectorParameterValue(TEXT("BaseColor"), Srgb(StandIn->Base));
		D->SetVectorParameterValue(TEXT("VeinColor"), Srgb(StandIn->Vein));
		D->SetScalarParameterValue(TEXT("VeinAmount"), StandIn->VeinAmount);
		D->SetScalarParameterValue(TEXT("VeinSharpness"), 14.f);
		D->SetScalarParameterValue(TEXT("StrataAmount"), 0.04f);
		D->SetScalarParameterValue(TEXT("StrataAlong"), 0.6f);
		D->SetScalarParameterValue(TEXT("StrataAcross"), 0.6f);
		D->SetScalarParameterValue(TEXT("FloorVeinScale"), 0.5f);
		D->SetScalarParameterValue(TEXT("Roughness"), StandIn->Roughness);
		D->SetScalarParameterValue(TEXT("ClearCoat"), StandIn->ClearCoat);
		D->SetScalarParameterValue(TEXT("ClearCoatRoughness"), 0.15f);
		D->SetScalarParameterValue(TEXT("WallJoints"), StandIn->Joints);
		D->SetScalarParameterValue(TEXT("FloorJoints"), StandIn->Joints);
		D->SetScalarParameterValue(TEXT("PoreAmount"), StandIn->Pores);
		D->SetScalarParameterValue(TEXT("PoreSize"), 0.002f);
		D->SetScalarParameterValue(TEXT("PoreStretch"), 1.0f);
		D->SetScalarParameterValue(TEXT("PoreThreshold"), 0.35f);
		D->SetScalarParameterValue(TEXT("PoreDarkness"), StandIn->PoreDarkness);
		D->SetScalarParameterValue(TEXT("BlockTone"), 0.03f);
		D->SetScalarParameterValue(TEXT("MacroVariation"), 0.05f);
		Fallbacks.Add(D);
		return D;
	};
	auto Set = [](UProceduralMeshComponent* Component, int32 Section, UMaterialInterface* M)
	{
		if (Component && M) { Component->SetMaterial(Section, M); }
	};
	const FStandIn Rosso{0x6E2A22, 0x2E0F0C, 0.25f, 0.25f, 0.6f, 1.f, 0.f, 0.72f};
	const FStandIn Giallo{0xD9B571, 0x9C5F32, 0.35f, 0.22f, 0.65f, 0.f, 0.f, 0.72f};
	const FStandIn Porphyry{0x5A1C20, 0x3A1014, 0.0f, 0.2f, 0.7f, 0.f, 0.6f, 1.45f};
	const FStandIn Verde{0x2C4436, 0xD9DDD2, 0.4f, 0.22f, 0.65f, 0.f, 0.f, 0.72f};
	const FStandIn Carved{0xECE8E0, 0xB9B3A8, 0.12f, 0.36f, 0.25f, 0.f, 0.f, 0.72f};
	const FStandIn Pavonazzetto{0xEDE8E3, 0x6D4B5F, 0.5f, 0.24f, 0.6f, 0.f, 0.f, 0.72f};
	const FStandIn Pompeian{0x8E4B3D, 0x6A3328, 0.0f, 0.88f, 0.0f, 0.f, 0.f, 0.72f};

	Set(Masonry, 0, Pick(WallMaterial, nullptr));
	Set(Masonry, 1, Pick(DadoMaterial, &Rosso));
	Set(Masonry, 2, Pick(NicheMaterial, &Pompeian));
	Set(Masonry, 3, Pick(FloorMaterial, nullptr));
	Set(Masonry, 4, Pick(GialloMaterial, &Giallo));
	Set(Masonry, 5, Pick(PorphyryMaterial, &Porphyry));
	Set(Masonry, 6, Pick(VerdeMaterial, &Verde));
	Set(Masonry, 7, Pick(RossoMaterial, &Rosso));
	Set(Masonry, 8, Pick(ShellMaterial, nullptr));
	Set(Masonry, 9, Pick(ShellMaterial, nullptr));
	Set(Order, 0, Pick(CarvedMaterial, &Carved));
	Set(Order, 1, Pick(GialloMaterial, &Giallo));
	Set(Order, 2, Pick(PavonazzettoMaterial, &Pavonazzetto));
	Set(Order, 3, Pick(PavonazzettoMaterial, &Pavonazzetto));
	Set(Vaults, 0, Pick(VaultMaterial, nullptr));
	Set(Vaults, 1, Pick(GiltMaterial, nullptr));

	// The diffusers: M_Daylit (MI_light_grid) in the editor; at run time its luminance matched to the rect lights behind,
	// in the lamps' colour, and held steady unless they follow the sky.
	UMaterialInterface* Grid = LaylightMaterial.LoadSynchronous();
	UMaterialInterface* Diffuser = Grid;
	if (bRuntime && Grid)
	{
		LaylightGlow = UMaterialInstanceDynamic::Create(Grid, this);
		LaylightGlow->SetFlags(RF_Transient);
		const FLinearColor Lamp = FLinearColor::MakeFromColorTemperature(3800.f);
		const float Luma = FMath::Max(Lamp.GetLuminance(), 1e-3f);
		LaylightGlow->SetVectorParameterValue(TEXT("Tint"), FLinearColor(Lamp.R / Luma, Lamp.G / Luma, Lamp.B / Luma, 1.f));
		LaylightGlow->SetScalarParameterValue(TEXT("Luminance"), static_cast<float>(ClassicalHallGeometry::LaylightNits()) * LightScale);
		LaylightGlow->SetScalarParameterValue(TEXT("NightFloor"), bLaylightsFollowDaylight ? 0.6f : 1.f);
		Diffuser = LaylightGlow;
	}
	Set(Laylights, 0, Diffuser);
}

void AClassicalHallStructure::PlaceLights()
{
	int32 Rect = 0, Spot = 0;
	for (const ClassicalHallGeometry::FLight& L : ClassicalHallGeometry::Lights())
	{
		const FVector At = L.Location * MuseePlan::Cm;
		if (L.Type == ClassicalHallGeometry::FLight::EType::Rect)
		{
			URectLightComponent* C = AreaLights.IsValidIndex(Rect) ? AreaLights[Rect].Get() : nullptr;
			++Rect;
			if (!C) { continue; }
			C->SetRelativeLocationAndRotation(At, FRotationMatrix::MakeFromXY(L.Direction, L.Across).Rotator());
			C->SetSourceWidth(static_cast<float>(L.Width * MuseePlan::Cm));
			C->SetSourceHeight(static_cast<float>(L.Height * MuseePlan::Cm));
			C->SetBarnDoorAngle(static_cast<float>(L.BarnDoorAngle));
			C->SetBarnDoorLength(static_cast<float>(L.BarnDoorLength * MuseePlan::Cm));
			C->SetIntensityUnits(ELightUnits::Candelas);
			C->SetIntensity(static_cast<float>(L.Candela) * LightScale);
			C->SetUseTemperature(true);
			C->SetTemperature(static_cast<float>(L.Kelvin));
			C->SetLightColor(FLinearColor::White);
			C->SetAttenuationRadius(static_cast<float>(L.Range * MuseePlan::Cm));
			C->SetCastShadows(L.bShadows);
		}
		else
		{
			USpotLightComponent* C = AccentLights.IsValidIndex(Spot) ? AccentLights[Spot].Get() : nullptr;
			++Spot;
			if (!C) { continue; }
			C->SetRelativeLocationAndRotation(At, FRotationMatrix::MakeFromX(L.Direction).Rotator());
			C->SetInnerConeAngle(static_cast<float>(L.InnerCone));
			C->SetOuterConeAngle(static_cast<float>(L.OuterCone));
			C->SetSourceRadius(static_cast<float>(L.SourceRadius * MuseePlan::Cm));
			C->SetSoftSourceRadius(static_cast<float>(L.SourceRadius * MuseePlan::Cm));
			C->SetIntensityUnits(ELightUnits::Candelas);
			C->SetIntensity(static_cast<float>(L.Candela) * LightScale);
			C->SetUseTemperature(true);
			C->SetTemperature(static_cast<float>(L.Kelvin));
			C->SetLightColor(FLinearColor::White);
			C->SetAttenuationRadius(static_cast<float>(L.Range * MuseePlan::Cm));
			C->SetCastShadows(L.bShadows);
			C->SetVisibility(bAccentLights);
		}
	}
}

namespace ClassicalHallActor
{
	FClassicalStatueSpot ToWorld(const ClassicalHallGeometry::FSpot& S, const FTransform& Actor)
	{
		FClassicalStatueSpot Out;
		Out.Name = FName(*S.Name);
		Out.Kind = FName(*S.Kind);
		const FRotator Facing = FRotationMatrix::MakeFromXZ(S.Facing, FVector::UpVector).Rotator();
		Out.Transform = FTransform(Facing, S.Location * MuseePlan::Cm) * Actor;
		Out.SuggestedHeight = static_cast<float>(S.Height * MuseePlan::Cm);
		Out.MaxWidth = static_cast<float>(S.MaxWidth * MuseePlan::Cm);
		Out.SuggestedWork = S.Suggested;
		return Out;
	}
}

TArray<FClassicalStatueSpot> AClassicalHallStructure::GetStatueSpots() const
{
	TArray<FClassicalStatueSpot> Out;
	for (const ClassicalHallGeometry::FSpot& S : ClassicalHallGeometry::StatueSpots()) { Out.Add(ClassicalHallActor::ToWorld(S, GetActorTransform())); }
	return Out;
}

FClassicalStatueSpot AClassicalHallStructure::GetCentrepieceSpot() const
{
	return ClassicalHallActor::ToWorld(ClassicalHallGeometry::Centrepiece(), GetActorTransform());
}
