#include "Elan/ElanSphereTop.h"

#include "Elan/ElanKit.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Engine/Scene.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

namespace ElanTopKit
{
	/**
	 * A small warm lamp with no shadows (the neck's ring of light, the platform's rail). Diffuse only: as a
	 * sphere of light it would mirror in the glass and the polished stone as a blot.
	 */
	UPointLightComponent* Lamp(USceneComponent* Parent, const TCHAR* Name, const FVector& At, float Lumens, float Radius)
	{
		UPointLightComponent* L = GK::NewPart<UPointLightComponent>(Parent, Name);
		L->SetRelativeLocation(At * GK::Cm);
		L->SetIntensityUnits(ELightUnits::Lumens);
		L->SetIntensity(Lumens);
		L->SetUseTemperature(true);
		L->SetTemperature(3000.f);
		L->SetAttenuationRadius(Radius * GK::Cm);
		L->SetSourceRadius(8.f);
		L->SetCastShadows(false);
		L->SetSpecularScale(0.f);
		GK::Register(L);
		return L;
	}

}

namespace TK = ElanTopKit;

// ---------------------------------------------------------------------------------------------
// An iris

UElanIris::UElanIris()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UElanIris::Build(UMaterialInterface* TopMaterial, UMaterialInterface* UnderMaterial, UMaterialInterface* LineMaterial, bool bWalkable)
{
	if (bBuilt || !GetOwner()) { return; }
	bBuilt = true;
	const int32 N = GK::IrisBlades;
	const double Rm = GK::IrisMidR, HalfW = GK::IrisWidth / 2, T = GK::IrisBladeThickness;
	const double Arc = FMath::DegreesToRadians(GK::IrisArcDegrees);

	// One blade, open, in its pin's frame (the pin at (Rm, 0) on the middle circle; the blade runs round the
	// circle towards +Y). Its outline, counter-clockwise seen from above: the inner arc out, the far end's round
	// cap, the outer arc back, the pin end's round cap. Top face at 0, T deep.
	constexpr int32 ArcSteps = 40, CapSteps = 12;
	auto Mid = [Rm](double A) { return FVector(Rm * FMath::Cos(A), Rm * FMath::Sin(A), 0.0); };
	TArray<FVector> Outline;
	for (int32 i = 0; i <= ArcSteps; ++i) { Outline.Add(Mid(Arc * i / ArcSteps) * ((Rm - HalfW) / Rm)); }
	const int32 FarCap0 = Outline.Num();
	for (int32 i = 1; i < CapSteps; ++i)
	{
		// Centre Mid(Arc): from its inner point (direction Arc + pi) round through the forward tangent to its outer point (Arc).
		const double B = Arc + UE_DOUBLE_PI - UE_DOUBLE_PI * i / CapSteps;
		Outline.Add(Mid(Arc) + FVector(FMath::Cos(B), FMath::Sin(B), 0) * HalfW);
	}
	const int32 OuterStart = Outline.Num();
	for (int32 i = ArcSteps; i >= 0; --i) { Outline.Add(Mid(Arc * i / ArcSteps) * ((Rm + HalfW) / Rm)); }
	const int32 PinCap0 = Outline.Num();
	for (int32 i = 1; i < CapSteps; ++i)
	{
		// Centre Mid(0): from its outer point (direction 0) round through the backward tangent to its inner point (pi).
		const double B = -UE_DOUBLE_PI * i / CapSteps;
		Outline.Add(Mid(0) + FVector(FMath::Cos(B), FMath::Sin(B), 0) * HalfW);
	}
	const FVector Pin(Rm + GK::IrisPinOutset, 0.0, 0.0);
	for (FVector& P : Outline) { P -= Pin; }
	const FVector Up(0, 0, 1), Down(0, 0, -T);
	auto UvOf = [](const FVector& P) { return FVector2D(P.X, P.Y); };
	FMuseeMesh TopGeo, UnderGeo;
	// The band between the arcs in quads.
	for (int32 i = 0; i < ArcSteps; ++i)
	{
		const FVector I0 = Outline[i], I1 = Outline[i + 1];
		const FVector O0 = Outline[OuterStart + ArcSteps - i], O1 = Outline[OuterStart + ArcSteps - i - 1];
		TopGeo.Quad(I0, I1, O1, O0, Up, Up, Up, Up, UvOf(I0), UvOf(I1), UvOf(O1), UvOf(O0));
		UnderGeo.Quad(I0 + Down, I1 + Down, O1 + Down, O0 + Down, -Up, -Up, -Up, -Up);
	}
	// Each cap a fan from its centre, from the band's corner round to the other corner.
	auto Fan = [&](const FVector& Centre, const FVector& First, int32 From, const FVector& Last)
	{
		TArray<FVector> Rim;
		Rim.Add(First);
		for (int32 i = 0; i < CapSteps - 1; ++i) { Rim.Add(Outline[From + i]); }
		Rim.Add(Last);
		for (int32 i = 0; i + 1 < Rim.Num(); ++i)
		{
			TopGeo.Tri(Centre, Rim[i], Rim[i + 1], Up, Up, Up, UvOf(Centre), UvOf(Rim[i]), UvOf(Rim[i + 1]));
			UnderGeo.Tri(Centre + Down, Rim[i] + Down, Rim[i + 1] + Down, -Up, -Up, -Up);
		}
	};
	Fan(Mid(Arc) - Pin, Outline[ArcSteps], FarCap0, Outline[OuterStart]);
	Fan(Mid(0) - Pin, Outline[OuterStart + ArcSteps], PinCap0, Outline[0]);
	// The inlay: a 7 mm line just inside each long edge, a hair proud of the top.
	FMuseeMesh LineGeo;
	for (const double Edge : {Rm - HalfW + 0.004, Rm + HalfW - 0.011})
	{
		for (int32 i = 0; i < ArcSteps; ++i)
		{
			const double A0 = Arc * i / ArcSteps, A1 = Arc * (i + 1) / ArcSteps;
			const FVector P0 = Mid(A0) * (Edge / Rm) - Pin, P1 = Mid(A1) * (Edge / Rm) - Pin;
			const FVector Q0 = Mid(A0) * ((Edge + 0.007) / Rm) - Pin, Q1 = Mid(A1) * ((Edge + 0.007) / Rm) - Pin;
			const FVector Lift(0, 0, 0.0004);
			LineGeo.Quad(P0 + Lift, P1 + Lift, Q1 + Lift, Q0 + Lift, Up);
		}
	}
	// The edge all round, its normal out of the outline (the loop runs counter-clockwise).
	for (int32 i = 0; i < Outline.Num(); ++i)
	{
		const FVector A = Outline[i], B = Outline[(i + 1) % Outline.Num()];
		const FVector Out = FVector(B.Y - A.Y, -(B.X - A.X), 0).GetSafeNormal();
		UnderGeo.Quad(A + Down, B + Down, B, A, Out);
	}

	for (int32 k = 0; k < N; ++k)
	{
		const double Phi = GK::Turn * k / N;
		USceneComponent* Pivot = GK::NewPart<USceneComponent>(this, TEXT("IrisBladePin"));
		const double PinR = Rm + GK::IrisPinOutset;
		Pivot->SetRelativeLocation(FVector(PinR * FMath::Cos(Phi), PinR * FMath::Sin(Phi), -GK::IrisStep * k) * GK::Cm);
		GK::Register(Pivot);
		UProceduralMeshComponent* PM = GK::NewMesh(Pivot, TEXT("IrisBlade"), false);
		GK::Section(PM, 0, TopGeo, TopMaterial);
		GK::Section(PM, 1, UnderGeo, UnderMaterial);
		if (LineMaterial) { GK::Section(PM, 2, LineGeo, LineMaterial); }
		GK::Register(PM);
		Pivots.Add(Pivot);
	}

	if (bWalkable)
	{
		// Closed, a floor: one convex prism over the whole opening, as deep as the blades.
		TArray<FVector> Prism;
		constexpr int32 Sides = 48;
		const double Depth = GK::IrisStep * N + T;
		for (int32 k = 0; k < Sides; ++k)
		{
			const double A = GK::Turn * k / Sides;
			const FVector P(FMath::Cos(A) * (GK::IrisSeatR + 0.01), FMath::Sin(A) * (GK::IrisSeatR + 0.01), 0.0);
			Prism.Add((P + FVector(0, 0, -Depth)) * GK::Cm);
			Prism.Add(P * GK::Cm);
		}
		TArray<TArray<FVector>> Hulls;
		Hulls.Add(Prism);
		FloorBody = GK::NewMesh(this, TEXT("IrisFloor"), false);
		FloorBody->bUseComplexAsSimpleCollision = false;
		FloorBody->SetCollisionConvexMeshes(Hulls);
		GK::Solid(FloorBody, true);
		GK::Register(FloorBody);
	}
	AppliedOpen = -1.f;
	SetOpen(0.f);
}

void UElanIris::SetOpen(float Open)
{
	Open = FMath::Clamp(Open, 0.f, 1.f);
	if (FMath::Abs(Open - AppliedOpen) < 1e-4f) { return; }
	AppliedOpen = Open;
	// Each blade turns on its pin in the plane: closed, IrisSwingDegrees on from its open place.
	const float Eased = Open * Open * (3.f - 2.f * Open);
	for (int32 k = 0; k < Pivots.Num(); ++k)
	{
		if (USceneComponent* Pivot = Pivots[k])
		{
			Pivot->SetRelativeRotation(FRotator(0, 360.0 * k / Pivots.Num() + GK::IrisSwingDegrees * (1.0 - Eased), 0));
		}
	}
	if (FloorBody) { FloorBody->SetCollisionEnabled(Open <= 0.001f ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision); }
}

// ---------------------------------------------------------------------------------------------
// The neck

UElanNeck::UElanNeck()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UElanNeck::Build()
{
	if (bBuilt || !GetOwner()) { return; }
	bBuilt = true;
	const FVector Up(0, 0, 1);
	UMaterialInterface* Gilt = GK::LoadMaterial(GK::GiltPath);
	UMaterialInstanceDynamic* Dark = GK::MetalInstance(this, FLinearColor(0.085f, 0.062f, 0.04f), 1.f, 0.42f);
	UMaterialInterface* Pearl = GK::LoadMaterial(GK::PearlPath);
	// Daylight white (about 6500 K, the white balance's own): the Atrium's light carried on up. Warm light on
	// bronze and gilt, closed in by the irises, turned the whole throat orange.
	const FLinearColor Neutral(1.0f, 0.98f, 0.95f);
	UMaterialInstanceDynamic* Glow = GK::GlowInstance(this, Neutral, RingNits);

	// The throat: pearl, as the Atrium's collar and ribs, from the collar to the floor ring, coursed with
	// thin gilt rings and sixteen ribs a hair proud of it, and girdled by a ring of light at the eyes of a
	// waiting visitor.
	const double Z0 = GK::CollarTop(), Z1 = GK::NeckTop - GK::FlangeDepth;
	FMuseeMesh WallGeo, TrimGeo, RingGeo;
	GK::Band(WallGeo, GK::NeckInR, GK::NeckOutR, Z0, Z1, 0, GK::Turn, false);
	const int32 Courses = FMath::Max(2, FMath::RoundToInt32((Z1 - Z0) / 0.45));
	for (int32 i = 1; i < Courses; ++i)
	{
		const double Z = Z0 + (Z1 - Z0) * i / Courses;
		if (FMath::Abs(Z - GK::NeckLightZ) < 0.2) { continue; }
		GK::Band(TrimGeo, GK::NeckInR - 0.018, GK::NeckInR + 0.005, Z - 0.012, Z + 0.012, 0, GK::Turn, false);
	}
	for (int32 k = 0; k < GK::Petals; ++k)
	{
		const double A = GK::Turn * (k + 0.5) / GK::Petals;
		GK::Block(TrimGeo, GK::Polar(GK::NeckInR - 0.006, A, (Z0 + Z1) / 2), GK::Outward(A), GK::Along(A), Up, FVector(0.012, 0.011, (Z1 - Z0) / 2));
	}
	GK::Band(RingGeo, GK::NeckInR - 0.03, GK::NeckInR + 0.005, GK::NeckLightZ - 0.05, GK::NeckLightZ + 0.05, 0, GK::Turn, false);

	// The floor ring in the Sphere: the mouth's seat (the upper iris closes flush into it), dark bronze
	// with a gilt edge and underside.
	FMuseeMesh FlangeTop, FlangeSides;
	GK::PolygonRing(FlangeTop, FlangeSides, 96, GK::IrisSeatR, GK::FlangeOutR, GK::NeckTop - GK::FlangeDepth, GK::NeckTop);

	UProceduralMeshComponent* Throat = GK::NewMesh(this, TEXT("NeckThroat"), true);
	GK::Section(Throat, 0, WallGeo, Pearl);
	GK::Section(Throat, 1, TrimGeo, Gilt);
	GK::Section(Throat, 2, FlangeTop, Dark);
	GK::Section(Throat, 3, FlangeSides, Gilt);
	GK::Register(Throat);
	UProceduralMeshComponent* LightRing = GK::NewMesh(this, TEXT("NeckLightRing"), false);
	GK::NoRayTracing(LightRing);
	GK::Section(LightRing, 0, RingGeo, Glow);
	GK::Register(LightRing);
	for (int32 k = 0; k < 6; ++k)
	{
		UPointLightComponent* L = TK::Lamp(this, TEXT("NeckLamp"), GK::Polar(GK::NeckInR - 0.12, GK::Turn * (k + 0.5) / 6, GK::NeckLightZ), LightLumens, 3.8f);
		L->SetTemperature(6500.f);
	}

	// The irises: in the tube's gilt collar (gilt underneath, as the Atrium sees it; open, the blades lie in
	// the collar, 2 cm under its top) and in the floor ring at the mouth (gilt on top, as the Sphere sees it;
	// open, the blades lie in the ring just under its face).
	BottomIris = GK::NewPart<UElanIris>(this, TEXT("NeckIrisBelow"));
	BottomIris->SetRelativeLocation(FVector(0, 0, (GK::CollarTop() - 0.02) * GK::Cm));
	GK::Register(BottomIris);
	BottomIris->Build(Pearl, Gilt, nullptr, false);   // pearl on top, into the throat; gilt under, as the Atrium sees it
	TopIris = GK::NewPart<UElanIris>(this, TEXT("NeckIrisAbove"));
	TopIris->SetRelativeLocation(FVector(0, 0, (GK::NeckTop - 0.004) * GK::Cm));
	GK::Register(TopIris);
	TopIris->Build(Gilt, Pearl, Dark, false);   // pearl underneath, as the throat sees it
}

void UElanNeck::SetIrises(float Bottom, float Top)
{
	if (BottomIris) { BottomIris->SetOpen(Bottom); }
	if (TopIris) { TopIris->SetOpen(Top); }
}

// ---------------------------------------------------------------------------------------------
// The platform

UElanPlatform::UElanPlatform()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UElanPlatform::Build()
{
	if (bBuilt || !GetOwner()) { return; }
	bBuilt = true;
	const FVector Up(0, 0, 1);
	UMaterialInterface* Gilt = GK::LoadMaterial(GK::GiltPath);
	UMaterialInterface* Stone = GK::LoadMaterial(GK::TravertinePath);
	UMaterialInterface* Pearl = GK::LoadMaterial(GK::PearlPath);
	ClearGlass = GK::GlassInstance(this, 0.09f, 0.02f, 0.9f);
	GK::FLook Look;
	Look.Clear = ClearGlass.Get();
	Look.Bronze = Gilt;
	Look.Edge = GK::EdgeInstance(this);
	const double H = GK::BalustradeHeight, R = GK::BalustradeR;

	// The floor: pale stone round the car's way (its seat for the iris), pearl underneath, a gilt nosing.
	FMuseeMesh SlabTop, SlabSides, Metal, RingGeo;
	GK::PolygonRing(SlabTop, SlabSides, 96, GK::IrisSeatR, GK::PlatformOutR, -GK::PlatformDepth, 0.0);
	GK::Band(Metal, GK::PlatformOutR - 0.002, GK::PlatformOutR + 0.012, -0.06, 0.003, 0, GK::Turn, false);

	// The balustrade: a gilt shoe and rail on slim posts, clear glass between (its edges green), a warm
	// line of light under the rail.
	GK::Band(Metal, R - 0.03, R + 0.03, 0.0, 0.06, 0, GK::Turn, false);
	GK::Tube(Metal, R, 0.028, H, 0, GK::Turn);
	constexpr int32 Posts = 32;
	for (int32 k = 0; k < Posts; ++k)
	{
		const double A = GK::Turn * (k + 0.5) / Posts;
		GK::Block(Metal, GK::Polar(R, A, (0.06 + H) / 2), GK::Outward(A), GK::Along(A), Up, FVector(0.0125, 0.0125, (H - 0.06) / 2));
	}
	TArray<FVector2D> Spans;
	for (int32 k = 0; k < 8; ++k) { Spans.Add(FVector2D(45.0 * k, 45.0 * (k + 1))); }
	GK::FixedGlass(this, TEXT("BalustradeGlass"), R, 0.05, H - 0.02, Spans, Look, Glass);
	GK::Band(RingGeo, R - 0.05, R - 0.028, H - 0.045, H - 0.03, 0, GK::Turn, false);

	// Eight slender struts carry it from the neck's floor ring, far below.
	for (int32 k = 0; k < 8; ++k)
	{
		const double A = GK::Turn * (k + 0.5) / 8;
		GK::Rod(Metal, GK::Polar(4.4, A, -GK::PlatformDepth + 0.02), GK::Polar(3.35, A, GK::NeckTop - GK::PlatformTop), 0.04);
	}

	UProceduralMeshComponent* Slab = GK::NewMesh(this, TEXT("PlatformSlab"), true);
	GK::Section(Slab, 0, SlabTop, Stone);
	GK::Section(Slab, 1, SlabSides, Pearl);
	GK::Section(Slab, 2, Metal, Gilt);
	GK::Register(Slab);
	UMaterialInstanceDynamic* Glow = GK::GlowInstance(this, GK::WarmWhite, RailGlowNits);
	UProceduralMeshComponent* RailLight = GK::NewMesh(this, TEXT("PlatformRailLight"), false);
	GK::NoRayTracing(RailLight);
	GK::Section(RailLight, 0, RingGeo, Glow);
	GK::Register(RailLight);
	for (int32 k = 0; k < 8; ++k)
	{
		Lamps.Add(TK::Lamp(this, TEXT("PlatformLamp"), GK::Polar(R - 0.35, GK::Turn * (k + 0.5) / 8, H - 0.1), LightLumens, 5.0f));
	}
	// And a soft fill from high above, like moonlight, so the whole floor reads (the closed iris too) and
	// the car's glass catches it. An emitter no one sees; diffuse only, no shadows.
	Fill = GK::NewPart<URectLightComponent>(this, TEXT("PlatformFill"));
	Fill->SetRelativeLocationAndRotation(FVector(0, 0, 900.0), FRotator(-90, 0, 0));
	Fill->SetIntensityUnits(ELightUnits::Lumens);
	Fill->SetIntensity(FillLumens);
	Fill->SetSourceWidth(800.f);
	Fill->SetSourceHeight(800.f);
	Fill->SetAttenuationRadius(2500.f);
	Fill->SetCastShadows(false);
	Fill->SetSpecularScale(0.f);
	Fill->SetUseTemperature(true);
	Fill->SetTemperature(4600.f);
	GK::Register(Fill);
	SetLit(false);

	// The iris over the car's way: stone on top, 4 mm under the floor when closed (a spiral in the stone);
	// open, its blades lie in the slab.
	Iris = GK::NewPart<UElanIris>(this, TEXT("PlatformIris"));
	Iris->SetRelativeLocation(FVector(0, 0, -0.4));
	GK::Register(Iris);
	Iris->Build(Stone, Gilt, Pearl, true);   // pearl lines: gilt mirrored the black sky and read as cracks

	// Collision: the floor in 24 boxes (none reaching into the car's way), the balustrade, and the guard.
	const double RIn = GK::IrisSeatR;
	for (int32 k = 0; k < 24; ++k)
	{
		const double A = GK::Turn * k / 24;
		UBoxComponent* Box = GK::NewPart<UBoxComponent>(this, TEXT("PlatformFloor"));
		Box->InitBoxExtent(FVector((GK::PlatformOutR - RIn) / 2, GK::PlatformOutR * FMath::Tan(GK::Turn / 48) + 0.01, GK::PlatformDepth / 2) * GK::Cm);
		Box->SetRelativeLocationAndRotation(GK::Polar((GK::PlatformOutR + RIn) / 2, A, -GK::PlatformDepth / 2) * GK::Cm, FRotator(0, FMath::RadiansToDegrees(A), 0));
		Box->SetHiddenInGame(true);
		GK::Solid(Box, true);
		GK::Register(Box);
		Colliders.Add(Box);
	}
	GK::Wall(this, TEXT("PlatformBalustrade"), R, 0, GK::Turn, 0.0, 1.15, Colliders);
	GK::Wall(this, TEXT("PlatformGuard"), GK::PlatformIrisR, 0, GK::Turn, 0.0, 1.2, Guard);
	bGuardOn = true;
	SetGuard(false);
	SetIris(0.f);
}

void UElanPlatform::SetIris(float Open)
{
	if (Iris) { Iris->SetOpen(Open); }
}

void UElanPlatform::SetGuard(bool bOn)
{
	if (bOn == bGuardOn) { return; }
	bGuardOn = bOn;
	for (UBoxComponent* Box : Guard)
	{
		if (Box) { Box->SetCollisionEnabled(bOn ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision); }
	}
}

void UElanPlatform::SetLit(bool bOn)
{
	// Lit only while the visitor is in the Sphere (inside its shell no one else can see it).
	if (bOn == bLitNow) { return; }
	bLitNow = bOn;
	if (Fill) { Fill->SetVisibility(bOn); }
	for (UPointLightComponent* L : Lamps) { if (L) { L->SetVisibility(bOn); } }
}
