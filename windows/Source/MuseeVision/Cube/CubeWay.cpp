#include "Cube/CubeWay.h"

#include "Cube/CubePlan.h"
#include "Components/SpotLightComponent.h"
#include "Elan/ElanKit.h"
#include "Elan/ElanSphereTop.h"
#include "Geometry/MuseeMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

UElanCubeWay::UElanCubeWay()
{
	PrimaryComponentTick.bCanEverTick = false;
}

double UElanCubeWay::ClampLevel()
{
	// The blades' stack runs 26 mm down from IrisTop: its middle in the middle of the head's groove.
	return (CubePlan::IrisTop - 0.013) - (GK::CrownTop + 0.5 * (GK::HeadGrooveBottom + GK::HeadGrooveTop));
}

void UElanCubeWay::Build()
{
	if (bBuilt || !GetOwner()) { return; }
	bBuilt = true;
	UMaterialInterface* Gilt = GK::LoadMaterial(GK::GiltPath);
	UMaterialInterface* Clean = GK::LoadMaterial(TEXT("/Game/Museum/Materials/M_Gilt_Clean.M_Gilt_Clean"));
	UMaterialInstanceDynamic* Dark = GK::MetalInstance(this, FLinearColor(0.085f, 0.062f, 0.04f), 1.f, 0.42f);

	// The iris: its blades just over the gilt soffit ring (ACubeStructure), gilt below (the room sees them) and above
	// (the shaft does), a dark hairline along each blade so the shut iris reads as a spiral.
	Iris = GK::NewPart<UElanIris>(this, TEXT("CeilingIris"));
	Iris->SetRelativeLocation(FVector(0, 0, CubePlan::IrisTop * GK::Cm));
	GK::Register(Iris);
	Iris->Build(Gilt, Clean ? Clean : Gilt, Dark, false);

	// The mast's head: a collar, the groove the blades close into, a cap with a low dome.
	{
		const double G0 = GK::HeadGrooveBottom, G1 = GK::HeadGrooveTop, Top = GK::HeadTop - GK::CrownTop;
		const double R = CubePlan::HeadR, Rg = CubePlan::GrooveR;
		TArray<FVector2D> Profile = {
			FVector2D(0.0, 0.0), FVector2D(R - 0.012, 0.0), FVector2D(R, 0.012), FVector2D(R, G0 - 0.006), FVector2D(R - 0.006, G0),
			FVector2D(Rg, G0), FVector2D(Rg, G1), FVector2D(R - 0.006, G1), FVector2D(R, G1 + 0.006), FVector2D(R, Top - 0.03)};
		for (int32 i = 1; i <= 8; ++i)
		{
			const double A = 0.5 * UE_DOUBLE_PI * i / 8;
			Profile.Add(FVector2D(R * FMath::Cos(A), Top - 0.03 + 0.03 * FMath::Sin(A)));
		}
		FMuseeMesh HeadGeo;
		HeadGeo.Lathe(Profile, 96, false, FVector2D(1.6, 0.2));
		Head = GK::NewMesh(this, TEXT("MastHead"), true);
		GK::Section(Head, 0, HeadGeo, Clean ? Clean : Gilt);
		GK::Register(Head);
	}

	// The column: a unit length, scaled to the mast's length (its material's spiral seam is in world space, so it
	// stands still as the column is fed out from the crown below).
	{
		FMuseeMesh ColumnGeo;
		const double R = CubePlan::MastR;
		constexpr int32 N = 64;
		for (int32 i = 0; i < N; ++i)
		{
			const double A0 = 2 * UE_DOUBLE_PI * i / N, A1 = 2 * UE_DOUBLE_PI * (i + 1) / N;
			const FVector D0(FMath::Cos(A0), FMath::Sin(A0), 0), D1(FMath::Cos(A1), FMath::Sin(A1), 0);
			ColumnGeo.Quad(D0 * R, D1 * R, D1 * R + FVector(0, 0, 1), D0 * R + FVector(0, 0, 1), D0, D1, D1, D0,
						   FVector2D(A0 * R, 0), FVector2D(A1 * R, 0), FVector2D(A1 * R, 1), FVector2D(A0 * R, 1));
		}
		Column = GK::NewMesh(this, TEXT("MastColumn"), true);
		UMaterialInterface* Mast = GK::LoadMaterial(TEXT("/Game/Museum/Journeys/Materials/M_CubeMast.M_CubeMast"));
		GK::Section(Column, 0, ColumnGeo, Mast ? Mast : Gilt);
		GK::Register(Column);
		Column->SetVisibility(false);
	}
	// The mast's lights: four narrow spots at the soffit ring's inner edge (r 2.5 m, on the diagonals), grazing the mast
	// down to the crown; their beams end on the car (nothing lights the room's faces). A glimmer: the gilt catches it.
	for (int32 k = 0; k < 4; ++k)
	{
		const double A = FMath::DegreesToRadians(45.0 + 90.0 * k);
		const FVector From(2.5 * FMath::Cos(A), 2.5 * FMath::Sin(A), CubePlan::Ceiling - 0.012);
		const FVector To(0.0, 0.0, CubePlan::CarStop + GK::CrownTop + 0.6);
		USpotLightComponent* L = GK::NewPart<USpotLightComponent>(this, *FString::Printf(TEXT("MastLight%d"), k + 1));
		L->SetRelativeLocationAndRotation(From * GK::Cm, (To - From).Rotation());
		L->SetIntensityUnits(ELightUnits::Lumens);
		L->SetIntensity(25.f);
		L->SetUseTemperature(true);
		L->SetTemperature(3500.f);
		L->SetInnerConeAngle(2.f);
		L->SetOuterConeAngle(5.f);
		L->SetSourceRadius(1.5f);
		L->SetAttenuationRadius(2600.f);
		L->SetCastShadows(true);
		GK::Register(L);
		MastLights.Add(L);
	}
	AppliedY = 1e9;
	Apply(0.0, 0.f);
}

void UElanCubeWay::Apply(double CarY, float IrisOpen)
{
	if (!bBuilt) { return; }
	if (FMath::Abs(IrisOpen - AppliedIris) > 1e-4f)
	{
		AppliedIris = IrisOpen;
		Iris->SetOpen(IrisOpen);
	}
	if (FMath::Abs(CarY - AppliedY) < 1e-5) { return; }
	AppliedY = CarY;
	const double Clamp = ClampLevel();
	// The mast's lights: only while the car is down in the room.
	const bool bLights = CarY < Clamp + 0.5 && bShown;
	if (bLights != bLightsOn)
	{
		bLightsOn = bLights;
		for (USpotLightComponent* L : MastLights) { if (L) { L->SetVisibility(bLights && !bDark); } }
	}
	// Riding on the crown above the clamp; held in the iris below it.
	const double HeadZ = (CarY >= Clamp ? CarY : Clamp) + GK::CrownTop;
	Head->SetRelativeLocation(FVector(0, 0, HeadZ * GK::Cm));
	const double Bottom = CarY + GK::CrownTop - 0.01, Length = HeadZ - Bottom;
	const bool bColumn = Length > 0.02;
	Column->SetVisibility(bColumn);
	if (bColumn)
	{
		Column->SetRelativeLocation(FVector(0, 0, Bottom * GK::Cm));
		Column->SetRelativeScale3D(FVector(1, 1, Length));
	}
}

void UElanCubeWay::SetShown(bool bShow)
{
	if (bShown == bShow) { return; }
	bShown = bShow;
	if (Iris) { Iris->SetVisibility(bShown && !bMastOnly, true); }
	bLightsOn = bShown && AppliedY < ClampLevel() + 0.5;
	for (USpotLightComponent* L : MastLights) { if (L) { L->SetVisibility(bLightsOn && !bDark); } }
}

void UElanCubeWay::SetDark(bool bNow)
{
	if (bDark == bNow) { return; }
	bDark = bNow;
	for (USpotLightComponent* L : MastLights) { if (L) { L->SetVisibility(bLightsOn && !bDark); } }
}

void UElanCubeWay::SetMastOnly(bool bOnly)
{
	if (bMastOnly == bOnly) { return; }
	bMastOnly = bOnly;
	if (Iris) { Iris->SetVisibility(bShown && !bMastOnly, true); }
	if (Head) { Head->SetVisibility(!bMastOnly); }
}
