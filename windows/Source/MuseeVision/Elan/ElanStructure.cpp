#include "Elan/ElanStructure.h"

#include "MuseeVision.h"
#include "Plan/MuseePlan.h"
#include "Components/LocalLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/Scene.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "Geometry/MuseeBake.h"
#include "Geometry/MuseeMesh.h"

/**
 * The roof's grid, the iris and the laylights. A named namespace (the module builds in unity files).
 * Positions in metres in the actor's frame: the Atrium's centre on the floor.
 */
namespace ElanRoofKit
{
	namespace E = MuseePlan::Elan;
	constexpr double Turn = 2.0 * UE_DOUBLE_PI;
	constexpr double DegToRad = UE_DOUBLE_PI / 180.0;
	constexpr int32 Round = 192;                        // segments round the drum and the Sphere

	// The grid: members 6 cm wide and 12 cm deep, 5 cm under the glass.
	constexpr double GridGap = 0.05, GridDepth = 0.12, GridWidth = 0.06;
	constexpr double GridR = E::SphereRadius + GridGap + GridDepth / 2;   // the members' centre line, from the Sphere's centre
	constexpr int32 OuterNodes = 72;                    // 1.22 m apart at the rim; a multiple of the 12 bays
	constexpr int32 InnerNodes = 36;                    // halved inside r 8 m, so the triangles stay 0.6–1.4 m
	constexpr double GridTransitionR = 8.0;
	constexpr double GridInnerR = 3.6;                  // the outer edge of the ring round the opening
	constexpr double GridOuterR = 13.88;                // where the underside meets the drum's glass

	// The laylights: two rings of twelve under the roof (r 3–8.5 m and 8.5–14 m), centred between the
	// ribs, 65 cm under the glass (below the ribs, so the steel reads against the lit glass).
	constexpr double LightDrop = 0.65;
	constexpr double LightFill = 0.85;                  // the glass's own glow still bounces a little
	constexpr double LightBayFraction = 0.9;            // each light's width: this much of its bay
	struct FLightRing { double RIn, ROut, RAt; };
	const FLightRing LightRings[2] = {{E::RingRadius, 8.5, 6.0}, {8.5, E::SphereRadius, 11.0}};
	const FColor LightColour(0xF4, 0xF2, 0xEE);


	const TCHAR* const MistyPath = TEXT("/Game/Museum/Materials/M_MistyGlass.M_MistyGlass");
	const TCHAR* const PearlPath = TEXT("/Game/Museum/Materials/M_RibPearl.M_RibPearl");
	const TCHAR* const GiltPath = TEXT("/Game/Museum/Materials/M_Gilt.M_Gilt");

	FVector SphereCentre() { return FVector(0, 0, E::SphereCentreHeight); }

	/** A point Rs from the Sphere's centre, Theta from the south pole, at plan angle Phi. */
	FVector OnSphere(double Theta, double Phi, double Rs)
	{
		return SphereCentre() + FVector(FMath::Sin(Theta) * FMath::Cos(Phi), FMath::Sin(Theta) * FMath::Sin(Phi), -FMath::Cos(Theta)) * Rs;
	}

	/** The conformal coordinate of the grid: equal steps of it keep the triangles' shape as they shrink. */
	double Mercator(double Theta) { return FMath::Loge(FMath::Tan(Theta / 2)); }
	double FromMercator(double U) { return 2 * FMath::Atan(FMath::Exp(U)); }

	/** A band of rectangular section round the axis, R0 to R1 (R0 = 0: a solid disc), Z0 to Z1. */
	void Band(FMuseeMesh& M, double R0, double R1, double Z0, double Z1)
	{
		const FVector Up(0, 0, 1);
		auto P = [](double R, double A, double Z) { return FVector(R * FMath::Cos(A), R * FMath::Sin(A), Z); };
		auto Out = [](double A) { return FVector(FMath::Cos(A), FMath::Sin(A), 0); };
		constexpr int32 N = 96;
		for (int32 i = 0; i < N; ++i)
		{
			const double T0 = Turn * i / N, T1 = Turn * (i + 1) / N;
			M.Quad(P(R1, T0, Z0), P(R1, T1, Z0), P(R1, T1, Z1), P(R1, T0, Z1), Out(T0), Out(T1), Out(T1), Out(T0));
			if (R0 > 0)
			{
				M.Quad(P(R0, T0, Z0), P(R0, T1, Z0), P(R0, T1, Z1), P(R0, T0, Z1), -Out(T0), -Out(T1), -Out(T1), -Out(T0));
				M.Quad(P(R0, T0, Z1), P(R0, T1, Z1), P(R1, T1, Z1), P(R1, T0, Z1), Up);
				M.Quad(P(R0, T0, Z0), P(R0, T1, Z0), P(R1, T1, Z0), P(R1, T0, Z0), -Up);
			}
			else
			{
				M.Tri(FVector(0, 0, Z1), P(R1, T0, Z1), P(R1, T1, Z1), Up, Up, Up);
				M.Tri(FVector(0, 0, Z0), P(R1, T0, Z0), P(R1, T1, Z0), -Up, -Up, -Up);
			}
		}
	}

	/** One straight steel member between two nodes of the grid: its depth along the Sphere's radius. */
	void Member(FMuseeMesh& M, const FVector& A, const FVector& B)
	{
		const FVector Axis = (B - A).GetSafeNormal();
		const FVector Radial = ((A + B) / 2 - SphereCentre()).GetSafeNormal();
		const FVector Side = FVector::CrossProduct(Axis, Radial).GetSafeNormal();
		const FVector Deep = FVector::CrossProduct(Side, Axis).GetSafeNormal();
		const FVector W = Side * (GridWidth / 2), D = Deep * (GridDepth / 2);
		// A little past each node, so the members meeting there close up.
		const FVector A1 = A - Axis * (GridWidth / 2), B1 = B + Axis * (GridWidth / 2);
		M.Quad(A1 + W - D, B1 + W - D, B1 + W + D, A1 + W + D, Side);
		M.Quad(A1 - W - D, B1 - W - D, B1 - W + D, A1 - W + D, -Side);
		M.Quad(A1 - W + D, B1 - W + D, B1 + W + D, A1 + W + D, Deep);
		M.Quad(A1 - W - D, B1 - W - D, B1 + W - D, A1 + W - D, -Deep);
		M.Quad(A1 - W - D, A1 + W - D, A1 + W + D, A1 - W + D, -Axis);
		M.Quad(B1 - W - D, B1 + W - D, B1 + W + D, B1 - W + D, Axis);
	}

	struct FGridRing { double Theta; int32 Nodes; double Offset; };   // Offset: in node steps

	FVector Node(const FGridRing& Ring, int32 j)
	{
		return OnSphere(Ring.Theta, (j + Ring.Offset) * Turn / Ring.Nodes, GridR);
	}

	/**
	 * The roof's grid, like the Great Court's: rings, and two families of diagonals between them
	 * (loxodromes, near enough), in triangles. 72 nodes round from the rim to r 8 m, 36 inside it;
	 * where they halve, each inner node fans to three outer ones.
	 */
	void Grid(FMuseeMesh& M)
	{
		const double R = E::SphereRadius;
		const double ThetaOuter = FMath::Asin(GridOuterR / GridR);
		const double ThetaT = FMath::Asin(GridTransitionR / R), Theta0 = FMath::Asin(GridInnerR / R);
		// Near-equilateral triangles: rings a triangle's height apart in the conformal coordinate.
		const double StepOuter = Turn / OuterNodes * FMath::Sqrt(3.0) / 2, StepInner = Turn / InnerNodes * FMath::Sqrt(3.0) / 2;
		const int32 NOuter = FMath::Max(1, FMath::RoundToInt32((Mercator(ThetaOuter) - Mercator(ThetaT)) / StepOuter));
		const int32 NInner = FMath::Max(1, FMath::RoundToInt32((Mercator(ThetaT) - Mercator(Theta0)) / StepInner));
		TArray<FGridRing> GridRings;   // from the rim inwards
		for (int32 s = 0; s <= NOuter; ++s)
		{
			const double U = Mercator(ThetaOuter) + (Mercator(ThetaT) - Mercator(ThetaOuter)) * s / NOuter;
			GridRings.Add({FromMercator(U), OuterNodes, (s % 2) * 0.5});
		}
		const double TransitionOffset = GridRings.Last().Offset;
		for (int32 s = 1; s <= NInner; ++s)
		{
			const double U = Mercator(ThetaT) + (Mercator(Theta0) - Mercator(ThetaT)) * s / NInner;
			// The first inner ring stands over every other transition node; then they alternate.
			GridRings.Add({FromMercator(U), InnerNodes, TransitionOffset / 2 + ((s - 1) % 2) * 0.5});
		}
		auto Wrap = [](int32 j, int32 N) { return ((j % N) + N) % N; };
		for (int32 r = 0; r < GridRings.Num(); ++r)
		{
			const FGridRing& Ring = GridRings[r];
			for (int32 j = 0; j < Ring.Nodes; ++j) { Member(M, Node(Ring, j), Node(Ring, j + 1)); }
			if (r == 0) { continue; }
			// The diagonals to the ring outside: each node to the two outer nodes either side of it, or,
			// standing over one (where the nodes halve), to it and its neighbours.
			const FGridRing& Outer = GridRings[r - 1];
			for (int32 j = 0; j < Ring.Nodes; ++j)
			{
				const double T = (j + Ring.Offset) * Outer.Nodes / double(Ring.Nodes) - Outer.Offset;
				const int32 Near = FMath::RoundToInt32(T);
				const FVector Here = Node(Ring, j);
				if (FMath::Abs(T - Near) < 1e-4)
				{
					for (int32 d = -1; d <= 1; ++d) { Member(M, Here, Node(Outer, Wrap(Near + d, Outer.Nodes))); }
				}
				else
				{
					const int32 Below = FMath::FloorToInt32(T);
					Member(M, Here, Node(Outer, Wrap(Below, Outer.Nodes)));
					Member(M, Here, Node(Outer, Wrap(Below + 1, Outer.Nodes)));
				}
			}
		}
	}

}

namespace RK = ElanRoofKit;

AElanStructure::AElanStructure()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	auto Make = [this](const TCHAR* Name)
	{
		UProceduralMeshComponent* M = CreateDefaultSubobject<UProceduralMeshComponent>(Name);
		M->SetupAttachment(RootComponent);
		M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		M->bUseAsyncCooking = true;
		return M;
	};
	Glass = Make(TEXT("MistyGlass"));
	Ribs = Make(TEXT("Ribs"));
	Lattice = Make(TEXT("Lattice"));
	Lattice->SetCastShadow(false);   // behind the laylights: nothing to shade
	Iris = Make(TEXT("Iris"));
	Iris->SetCastShadow(false);
	MuseeBake::NoBake(Glass);   // its luminance is set at BeginPlay (ApplyMaterials): stays procedural
	MuseeBake::NoBake(Iris);    // opened and closed by the elevator (SetIrisOpen): stays procedural

	for (int32 i = 0; i < 24; ++i)
	{
		URectLightComponent* L = CreateDefaultSubobject<URectLightComponent>(*FString::Printf(TEXT("RoofLight%02d"), i + 1));
		L->SetupAttachment(RootComponent);
		L->SetMobility(EComponentMobility::Movable);
		RoofLights.Add(L);
	}
	for (int32 i = 0; i < 12; ++i)
	{
		USpotLightComponent* L = CreateDefaultSubobject<USpotLightComponent>(*FString::Printf(TEXT("RibUplight%02d"), i + 1));
		L->SetupAttachment(RootComponent);
		L->SetMobility(EComponentMobility::Movable);
		RibUplights.Add(L);
	}

	GlassMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(RK::MistyPath));
	RoofMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(RK::MistyPath));
	RibMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(RK::PearlPath));
	IrisMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(RK::PearlPath));
	BronzeMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(RK::GiltPath));
	PlaceRoofLights();
	PlaceRibUplights();
}

void AElanStructure::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
	ApplyMaterials(false);
	PlaceRoofLights();
	PlaceRibUplights();
	AddLaylightTags();
}

void AElanStructure::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	// Before anyone's BeginPlay: MuseeSky gathers the laylights (and their full intensity) in its own.
	PlaceRoofLights();
	AddLaylightTags();
}

void AElanStructure::BeginPlay()
{
	Super::BeginPlay();
	// A placed actor doesn't rerun its construction when the map loads: rebuild, so the map never
	// shows an older build of the roof; then the roof's and the drum's own luminance.
	Build();
	ApplyMaterials(true);
	RetireDrumLights();
	// The opening's old iris: the elevator's neck seals it now (its irises, below and above).
	Iris->SetVisibility(false);
}

void AElanStructure::SetIrisOpen(bool bOpen)
{
	// Retired: the elevator's neck (Elan/ElanSphereTop) seals the opening with its own irises now.
	Iris->SetVisibility(false);
}

void AElanStructure::AddLaylightTags()
{
	// MuseeSky scales musee.laylight actors' lights by max(Daylight, the night floor), as M_MistyGlass.
	Tags.RemoveAll([](const FName& Tag) { return Tag.ToString().StartsWith(TEXT("laylight.night:")); });
	Tags.AddUnique(FName(TEXT("musee.laylight")));
	Tags.Add(FName(*(TEXT("laylight.night:") + FString::SanitizeFloat(NightFloor))));
	Tags.AddUnique(FName(TEXT("musee.building")));
	Tags.AddUnique(FName(TEXT("musee.wing:Elan")));
}

void AElanStructure::RetireDrumLights()
{
	// Scripts/relight.py used to light the Atrium from twelve rect lights inside the drum; the roof's
	// lights replace them. Off for good (their visibility, which the Sphere's hiding never restores).
	int32 Off = 0;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (*It == this || !It->ActorHasTag(TEXT("musee.laylight")) || !It->ActorHasTag(TEXT("musee.wing:Elan"))) { continue; }
		TArray<ULocalLightComponent*> Lights;
		It->GetComponents(Lights);
		for (ULocalLightComponent* L : Lights) { L->SetVisibility(false); ++Off; }
	}
	if (Off > 0) { UE_LOG(LogMusee, Log, TEXT("Atrium: %d older drum laylights turned off (the roof's lights replace them)."), Off); }
}

void AElanStructure::PlaceRoofLights()
{
	namespace E = MuseePlan::Elan;
	const double R = E::SphereRadius;
	int32 Index = 0;
	for (const RK::FLightRing& Ring : RK::LightRings)
	{
		const double ThetaIn = FMath::Asin(Ring.RIn / R), ThetaOut = FMath::Asin(FMath::Min(Ring.ROut, R) / R);
		const double ThetaAt = FMath::Asin(Ring.RAt / R);
		// Each light's share of its zone of the roof, candela = luminance × area × fill.
		const double Area = RK::Turn * R * R * (FMath::Cos(ThetaIn) - FMath::Cos(ThetaOut)) / 12.0;
		const double Width = RK::Turn * Ring.RAt / 12.0 * RK::LightBayFraction;
		// Flat panels on a curved roof: no longer than the zone's chord either side of the centre.
		const double Height = FMath::Min(Area / Width, 2 * R * FMath::Sin(FMath::Min(ThetaAt - ThetaIn, ThetaOut - ThetaAt)));
		const float Candela = static_cast<float>(RoofNits * Area * RK::LightFill);
		for (int32 k = 0; k < 12 && Index < RoofLights.Num(); ++k, ++Index)
		{
			URectLightComponent* L = RoofLights[Index];
			if (!L) { continue; }
			// Centred between the ribs (at 15° + 30°k from north), facing down along the roof's normal.
			const double Phi = (30.0 * k - 60.0) * RK::DegToRad;
			const FVector Normal(FMath::Sin(ThetaAt) * FMath::Cos(Phi), FMath::Sin(ThetaAt) * FMath::Sin(Phi), -FMath::Cos(ThetaAt));
			const FVector Around(-FMath::Sin(Phi), FMath::Cos(Phi), 0);
			const FVector At = RK::SphereCentre() + Normal * (R + RK::LightDrop);
			L->SetRelativeLocationAndRotation(At * MuseePlan::Cm, FRotationMatrix::MakeFromXY(Normal, Around).Rotator());
			L->SetSourceWidth(static_cast<float>(Width * MuseePlan::Cm));
			L->SetSourceHeight(static_cast<float>(Height * MuseePlan::Cm));
			L->SetBarnDoorAngle(88.f);
			L->SetBarnDoorLength(0.f);
			L->SetIntensityUnits(ELightUnits::Candelas);
			L->SetIntensity(Candela);
			L->SetLightColor(FLinearColor(RK::LightColour));
			L->SetAttenuationRadius(4000.f);
			L->SetCastShadows(bRoofLightShadows);
		}
	}
}

void AElanStructure::PlaceRibUplights()
{
	namespace E = MuseePlan::Elan;
	for (int32 k = 0; k < RibUplights.Num(); ++k)
	{
		USpotLightComponent* L = RibUplights[k];
		if (!L) { continue; }
		// On the floor, a metre in front of the rib's foot, aimed up its front along the wall to the arch.
		const double Th = FMath::DegreesToRadians(15.0 + 30.0 * k - 90.0);
		const FVector Radial(FMath::Cos(Th), FMath::Sin(Th), 0);
		const FVector At = Radial * (E::Radius - 1.3) + FVector(0, 0, 0.05);
		const FVector Aim = Radial * (E::Radius - 1.6) + FVector(0, 0, 10.0);
		L->SetRelativeLocationAndRotation(At * MuseePlan::Cm, (Aim - At).Rotation());
		L->SetInnerConeAngle(5.f);
		L->SetOuterConeAngle(15.f);   // the rib's full width up to the equator, and a little of the drum each side
		L->SetSourceRadius(4.f);
		L->SetIntensityUnits(ELightUnits::Candelas);
		L->SetIntensity(RibUplightCandela);
		L->SetLightColor(FLinearColor(RK::LightColour));
		L->SetAttenuationRadius(2600.f);
		L->SetCastShadows(false);   // grazing its own rib: nothing to shade
	}
}

void AElanStructure::ApplyMaterials(bool bInstances)
{
	UMaterialInterface* DrumBase = GlassMaterial.LoadSynchronous();
	UMaterialInterface* RoofBase = RoofMaterial.LoadSynchronous();
	UMaterialInterface* Drum = DrumBase;
	UMaterialInterface* Roof = RoofBase;
	if (bInstances)
	{
		// Play only (never saved with the map): the roof bright, the drum dimmer, both × max(Daylight,
		// NightFloor) through M_MistyGlass's own parameters.
		auto Glow = [this](UMaterialInterface* Base, float Nits) -> UMaterialInstanceDynamic*
		{
			if (!Base) { return nullptr; }
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, this);
			MID->SetFlags(RF_Transient);
			MID->SetScalarParameterValue(TEXT("Luminance"), Nits);
			MID->SetScalarParameterValue(TEXT("NightFloor"), NightFloor);
			return MID;
		};
		DrumGlow = Glow(DrumBase, DrumNits);
		RoofGlow = Glow(RoofBase, RoofNits);
		if (DrumGlow) { Drum = DrumGlow; }
		if (RoofGlow) { Roof = RoofGlow; }
	}
	UMaterialInterface* Pearl = RibMaterial.LoadSynchronous();
	UMaterialInterface* Leaf = IrisMaterial.LoadSynchronous();
	UMaterialInterface* Bronze = BronzeMaterial.LoadSynchronous();
	if (Drum) { Glass->SetMaterial(0, Drum); Glass->SetMaterial(2, Drum); }
	if (Roof) { Glass->SetMaterial(1, Roof); }
	if (Pearl) { Ribs->SetMaterial(0, Pearl); Lattice->SetMaterial(0, Pearl); }
	if (Bronze) { Ribs->SetMaterial(1, Bronze); Iris->SetMaterial(1, Bronze); }
	if (Leaf) { Iris->SetMaterial(0, Leaf); }
}

void AElanStructure::Build()
{
	if (MuseeBake::IsBaked(this)) { return; }   // baked into static meshes (Geometry/MuseeBake.h)
	namespace E = MuseePlan::Elan;
	const double R = E::SphereRadius, C = E::SphereCentreHeight;
	const double PoleAngle = FMath::Asin((C - E::RingHeight()) / R);

	// Misty glass (one layer; the material is two-sided): the drum from the stone base to the
	// equator (section 0), the Sphere's underside from the opening to the equator, the roof
	// (section 1), and its top, seen from outside (section 2).
	auto ShellProfile = [R, C](double From, double To, int32 Steps)
	{
		TArray<FVector2D> Profile;
		for (int32 i = 0; i <= Steps; ++i)
		{
			const double A = From + (To - From) * i / Steps;
			Profile.Add(FVector2D(FMath::Max(0.0, R * FMath::Cos(A)), C + R * FMath::Sin(A)));
		}
		return Profile;
	};
	FMuseeMesh DrumMesh, RoofMesh, TopMesh;
	DrumMesh.Lathe({FVector2D(E::Radius, E::BaseHeight), FVector2D(E::Radius, C)}, RK::Round, false, FVector2D(72, 12));
	RoofMesh.Lathe(ShellProfile(-PoleAngle, 0.0, 64), RK::Round, false, FVector2D(72, 13));
	TopMesh.Lathe(ShellProfile(0.0, UE_DOUBLE_PI / 2, 32), RK::Round, false, FVector2D(72, 15));
	DrumMesh.Write(Glass, 0);
	RoofMesh.Write(Glass, 1);
	TopMesh.Write(Glass, 2);

	// Ribs: twelve, each one member of constant section from the floor to the ring, as a structure is drawn
	// (the bone-like lens that swelled and tapered read as roots under a bulb). Each rises plumb up the drum
	// (where the stone base's pilasters stood), turns in a 3 m arc at about 9 m and runs on under the Sphere,
	// a meridian of the globe, to the ring round the opening. Its section is a pearl rectangle with chamfered
	// arrises, 0.42 m deep and 0.30 m wide, its back tucked 3 cm into the stone and the glass; a bronze shoe
	// takes it at the floor, and a fine bronze line runs up its face. A sweep: the back line B(s) with the
	// room-side normal N(s).
	TArray<FVector2D> Back, Normal;   // (r, z) in the rib's plane
	{
		const double Tuck = 0.03, XWall = E::Radius + Tuck, Rb = R - Tuck, Rho = 3.0;
		const FVector2D Sc(0.0, C);
		const FVector2D Fc(XWall - Rho, C - FMath::Sqrt(FMath::Square(Rb + Rho) - FMath::Square(XWall - Rho)));
		const FVector2D U = (Sc - Fc).GetSafeNormal();   // from the arch's centre to the Sphere's
		for (double Z = 0.0; Z < Fc.Y - 1e-6; Z += 0.25) { Back.Add(FVector2D(XWall, Z)); Normal.Add(FVector2D(-1, 0)); }
		const double A1 = FMath::Atan2(U.Y, U.X);
		constexpr int32 ArchSteps = 24;
		for (int32 i = 0; i <= ArchSteps; ++i)
		{
			const double A = A1 * i / ArchSteps;
			const FVector2D Dir(FMath::Cos(A), FMath::Sin(A));
			Back.Add(Fc + Dir * Rho);
			Normal.Add(-Dir);
		}
		const FVector2D T2 = Fc + U * Rho;
		const double B0 = FMath::Atan2(T2.Y - C, T2.X), B1 = -FMath::Acos((E::RingRadius + 0.6) / Rb);
		constexpr int32 UnderSteps = 40;
		for (int32 i = 1; i <= UnderSteps; ++i)
		{
			const double B = B0 + (B1 - B0) * i / UnderSteps;
			const FVector2D Dir(FMath::Cos(B), FMath::Sin(B));
			Back.Add(Sc + Dir * Rb);
			Normal.Add(Dir);
		}
	}
	TArray<double> Along;
	Along.Add(0.0);
	for (int32 i = 1; i < Back.Num(); ++i) { Along.Add(Along.Last() + FVector2D::Distance(Back[i], Back[i - 1])); }
	const int32 NI = Back.Num();

	// The section, (x out from the back along N, y along the tangent), round from the back's one corner.
	constexpr double RibD = 0.42, RibW = 0.15, Chamfer = 0.03;
	const FVector2D Section[] = {{0.0, -RibW}, {RibD - Chamfer, -RibW}, {RibD, -RibW + Chamfer}, {RibD, RibW - Chamfer},
								 {RibD - Chamfer, RibW}, {0.0, RibW}};
	constexpr double ShoeH = 0.36, ShoeOut = 0.015, LineW = 0.012, LineProud = 0.002;
	FMuseeMesh RibMesh, TrimMesh;
	for (int32 k = 0; k < 12; ++k)
	{
		const double Th = FMath::DegreesToRadians(15.0 + 30.0 * k - 90.0);   // bay boundaries, from the compass bearing
		const FVector Radial(FMath::Cos(Th), FMath::Sin(Th), 0), Tangent(-FMath::Sin(Th), FMath::Cos(Th), 0);
		auto At = [&](int32 i, const FVector2D& S)
		{
			const FVector2D Q = Back[i] + Normal[i] * S.X;
			return Radial * Q.X + FVector(0, 0, Q.Y) + Tangent * S.Y;
		};
		// Each face a flat strip along the spine: its normal the face's outward direction in the section.
		auto Strip = [&](FMuseeMesh& M, const FVector2D& Sa, const FVector2D& Sb, int32 From)
		{
			const FVector2D Edge = Sb - Sa;
			const FVector2D Out2 = FVector2D(Edge.Y, -Edge.X).GetSafeNormal();   // outward: the section runs counter-clockwise in (x, y)
			for (int32 i = From; i + 1 < NI; ++i)
			{
				auto Nrm = [&](int32 n) { const FVector2D Nn = Normal[n] * Out2.X; return (Radial * Nn.X + FVector(0, 0, Nn.Y) + Tangent * Out2.Y).GetSafeNormal(); };
				const FVector2D UA(0.0, Along[i]), UB(0.0, Along[i + 1]), UC(Edge.Size(), Along[i + 1]), UD(Edge.Size(), Along[i]);
				M.Quad(At(i, Sa), At(i + 1, Sa), At(i + 1, Sb), At(i, Sb), Nrm(i), Nrm(i + 1), Nrm(i + 1), Nrm(i), UA, UB, UC, UD);
			}
		};
		for (int32 e = 0; e + 1 < UE_ARRAY_COUNT(Section); ++e) { Strip(RibMesh, Section[e], Section[e + 1], 0); }
		// The end in the ring, capped.
		{
			const FVector Out = (At(NI - 1, FVector2D(0, 0)) - At(NI - 2, FVector2D(0, 0))).GetSafeNormal();
			const FVector Centre = At(NI - 1, FVector2D(RibD / 2, 0));
			for (int32 e = 0; e < UE_ARRAY_COUNT(Section); ++e)
			{
				RibMesh.Tri(Centre, At(NI - 1, Section[e]), At(NI - 1, Section[(e + 1) % UE_ARRAY_COUNT(Section)]), Out, Out, Out);
			}
		}
		// The bronze shoe: a sleeve 1.5 cm proud of the rib, 36 cm high, with a flat top.
		{
			const FVector2D Shoe[] = {{0.0, -RibW - ShoeOut}, {RibD + ShoeOut, -RibW - ShoeOut}, {RibD + ShoeOut, RibW + ShoeOut}, {0.0, RibW + ShoeOut}};
			auto P = [&](const FVector2D& S, double Z) { return Radial * (Back[0].X - S.X) + FVector(0, 0, Z) + Tangent * S.Y; };
			for (int32 e = 0; e + 1 < UE_ARRAY_COUNT(Shoe); ++e)
			{
				const FVector2D Ed = Shoe[e + 1] - Shoe[e];
				const FVector2D O = FVector2D(Ed.Y, -Ed.X).GetSafeNormal();
				const FVector N = (-Radial * O.X + Tangent * O.Y).GetSafeNormal();
				TrimMesh.Quad(P(Shoe[e], 0.0), P(Shoe[e], ShoeH), P(Shoe[e + 1], ShoeH), P(Shoe[e + 1], 0.0), N);
			}
			TrimMesh.Quad(P(Shoe[0], ShoeH), P(Shoe[1], ShoeH), P(Shoe[2], ShoeH), P(Shoe[3], ShoeH), FVector(0, 0, 1));
		}
		// The line up the face, from the shoe to the ring.
		{
			int32 From = 0;
			while (From + 1 < NI && Back[From].Y < ShoeH) { ++From; }
			Strip(TrimMesh, FVector2D(RibD + LineProud, -LineW), FVector2D(RibD + LineProud, LineW), From);
		}
	}
	const double Depth = RingDepth;
	// The compression ring round the opening, under the Sphere.
	const double R0 = E::RingRadius, R1 = E::RingRadius + 0.6;
	RibMesh.Lathe({FVector2D(R0, E::Underside(R0) - Depth), FVector2D(R1, E::Underside(R1) - Depth), FVector2D(R1, E::Underside(R1)),
				   FVector2D(R0, E::Underside(R0)), FVector2D(R0, E::Underside(R0) - Depth)}, 96, false);   // one layer: the material is two-sided
	RibMesh.Write(Ribs, 0);
	TrimMesh.Write(Ribs, 1);   // the shoes and the lines up the ribs' faces (bronze)

	// The roof's grid, 5 cm under the glass.
	FMuseeMesh GridMesh;
	RK::Grid(GridMesh);
	GridMesh.Write(Lattice, 0);

	// The opening's old iris is retired: the elevator's neck (Elan/ElanSphereTop) seals it now.
	Iris->ClearAllMeshSections();
	Iris->SetVisibility(false);
}
