#include "Elan/ElanCar.h"

#include "Elan/ElanKit.h"
#include "Components/BoxComponent.h"
#include "Components/RectLightComponent.h"
#include "Engine/Scene.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"


// ---------------------------------------------------------------------------------------------
// The car

UElanCar::UElanCar()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UElanCar::Build()
{
	if (bBuilt || !GetOwner()) { return; }
	bBuilt = true;
	const FVector Up(0, 0, 1);

	// Clear low-iron glass that always reads as glass: a stronger reflection than thin glass (Specular),
	// a little more body (Opacity), and its deep green edges at every joint, jamb and leaf.
	ClearGlass = GK::GlassInstance(this, GlassOpacity, 0.02f, GlassSpecular);
	// The floor lightly frosted: a pale satin, so it reads as a floor you can see down through.
	UMaterialInstanceDynamic* Frosted = GK::GlassInstance(this, 0.3f, 0.3f, 0.5f);
	FloorGlass = Frosted;
	if (Frosted)
	{
		Frosted->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.86f, 0.9f, 0.89f));
		Frosted->SetScalarParameterValue(TEXT("Diffuse"), 0.08f);
	}
	UMaterialInstanceDynamic* Rod = GK::GlassInstance(this, 0.4f, 0.03f, GlassSpecular);
	RailGlass = Rod;
	GK::FLook Look;
	Look.Clear = ClearGlass.Get();
	Look.Bronze = GK::LoadMaterial(GK::GiltPath);
	Look.Edge = GK::EdgeInstance(this);

	// The glass: four large panes, their joints clear of the landings'.
	GK::FixedGlass(this, TEXT("CarPane"), GK::CarR, GK::GlassBottom, GK::GlassTop, GK::CarSpans(), Look, FadingGlass);
	// The leaves, 5 cm inside the glass, standing on the base ring.
	for (const double Side : {-1.0, 1.0})
	{
		LeafPivots.Add(GK::Leaf(this, TEXT("CarLeaf"), GK::CarLeafR, GK::BaseRingTop, GK::GlassTop, Side, Look, FadingGlass, Colliders));
	}

	// Bronze: the base ring (it carries the floor's edge), the collar under the floor where the mast
	// meets it, the handrail's brackets; and the top ring, which goes with the glass at the top.
	FMuseeMesh Lower, Upper;
	GK::Band(Lower, GK::CarRingIn, GK::CarRingOut, GK::BaseRingBottom, GK::BaseRingTop, 0, GK::Turn, false);
	GK::Band(Lower, 0, GK::CollarR, GK::CollarBottom, GK::FloorBottom, 0, GK::Turn, false);
	for (const double Rel : {60.0, 120.0, 180.0, 240.0, 300.0})
	{
		const double A = GK::DoorTheta + Rel * GK::Deg;
		const double From = GK::RailR, To = GK::CarR - 0.004;
		GK::Block(Lower, GK::Polar((From + To) / 2, A, GK::RailZ), GK::Outward(A), GK::Along(A), Up, FVector((To - From) / 2, 0.006, 0.006));
	}
	GK::Band(Upper, GK::CarRingIn, GK::CarRingOut, GK::TopRingBottom, GK::TopRingTop, 0, GK::Turn, false);
	// The roller guides (the Cube shaft's rails, north and south): a shoe on each ring, its rollers either side of the
	// rail's blade.
	for (const double Bearing : {0.0, 180.0})
	{
		const double A = FMath::DegreesToRadians(Bearing - 90.0);
		const double From = GK::CarRingOut - 0.006, To = GK::ShoeOut;
		GK::Block(Lower, GK::Polar((From + To) / 2, A, (GK::BaseRingBottom + GK::BaseRingTop) / 2), GK::Outward(A), GK::Along(A), Up,
				  FVector((To - From) / 2, 0.045, (GK::BaseRingTop - GK::BaseRingBottom) / 2));
		GK::Block(Upper, GK::Polar((From + To) / 2, A, (GK::TopRingBottom + GK::TopRingTop) / 2), GK::Outward(A), GK::Along(A), Up,
				  FVector((To - From) / 2, 0.045, (GK::TopRingTop - GK::TopRingBottom) / 2));
	}
	UProceduralMeshComponent* LowerBronze = GK::NewMesh(this, TEXT("CarBronze"), true);
	GK::Section(LowerBronze, 0, Lower, Look.Bronze);
	GK::Register(LowerBronze);
	UProceduralMeshComponent* TopRing = GK::NewMesh(this, TEXT("CarTopRing"), true);
	GK::Section(TopRing, 0, Upper, Look.Bronze);
	GK::Register(TopRing);
	DimHidden.Add(TopRing);

	// The slim glass handrail, 95 cm over the floor, with rounded ends either side of the door.
	FMuseeMesh RailGeo;
	const double RailA0 = GK::DoorTheta + GK::RailFromDeg * GK::Deg, RailA1 = GK::DoorTheta + GK::RailToDeg * GK::Deg;
	GK::Tube(RailGeo, GK::RailR, GK::RailTube, GK::RailZ, RailA0, RailA1);
	GK::Ball(RailGeo, GK::Polar(GK::RailR, RailA0, GK::RailZ), GK::RailTube);
	GK::Ball(RailGeo, GK::Polar(GK::RailR, RailA1, GK::RailZ), GK::RailTube);
	UProceduralMeshComponent* Rail = GK::NewMesh(this, TEXT("CarRail"), false);
	GK::Section(Rail, 0, RailGeo, Rod);
	GK::Register(Rail);
	RailMesh = Rail;
	if (UMaterialInterface* Frost = GK::LoadMaterial(TEXT("/Game/Museum/Journeys/Materials/M_CubeFrost.M_CubeFrost")))
	{
		FrostFloor = UMaterialInstanceDynamic::Create(Frost, this);
		FrostFloor->SetScalarParameterValue(TEXT("Opacity"), 0.22f);
		FrostRail = UMaterialInstanceDynamic::Create(Frost, this);
		FrostRail->SetScalarParameterValue(TEXT("Opacity"), 0.45f);
		FrostRail->SetScalarParameterValue(TEXT("Roughness"), 0.2f);
	}

	// The floor: lightly frosted glass. Its collision is one convex prism to the glass (64 sides,
	// 5 cm deep): the visitor's moving base while riding. Drawn after the Square's glass floor,
	// which it stands 2 cm over at the bottom (see AElanElevator).
	FMuseeMesh FloorGeo;
	GK::Flat(FloorGeo, 0, GK::CarRingIn + 0.005, GK::FloorTop, Up);
	FloorMesh = GK::NewMesh(this, TEXT("CarFloor"), false);
	GK::Section(FloorMesh, 0, FloorGeo, Frosted);
	TArray<FVector> Prism;
	for (int32 i = 0; i < 64; ++i)
	{
		const double A = GK::Turn * i / 64;
		Prism.Add(GK::Polar(GK::CarR, A, GK::FloorBottom) * GK::Cm);
		Prism.Add(GK::Polar(GK::CarR, A, GK::FloorTop) * GK::Cm);
	}
	TArray<TArray<FVector>> Hulls;
	Hulls.Add(Prism);
	FloorMesh->bUseComplexAsSimpleCollision = false;
	FloorMesh->SetCollisionConvexMeshes(Hulls);
	GK::Solid(FloorMesh, true);
	GK::Register(FloorMesh);

	// The roof: a clear glass disc in the top ring (Apple-style: the view up stays open, and in the
	// Sphere, the sky), fading with the walls. The light is a slim luminous ring under its rim.
	FMuseeMesh RoofGeo, RingGeo;
	GK::Flat(RoofGeo, 0, GK::CarRingIn + 0.005, GK::CanopyZ, -Up);
	CanopyMesh = GK::NewMesh(this, TEXT("CarRoofGlass"), false);
	GK::Section(CanopyMesh, 0, RoofGeo, Look.Clear);
	GK::Register(CanopyMesh);
	FadingGlass.Add(CanopyMesh);
	RingGlow = GK::GlowInstance(this, FLinearColor(1.0f, 0.93f, 0.85f), RingNits);   // neutral white, as the car's light
	GK::Band(RingGeo, GK::LightRingIn, GK::LightRingOut, GK::LightRingBottom, GK::LightRingTop, 0, GK::Turn, false);
	UProceduralMeshComponent* LightRing = GK::NewMesh(this, TEXT("CarLightRing"), false);
	// Not in the ray-traced scene: its reflection in the glass all round came back as a flickering
	// dashed line (a thin bright emitter is noise to Lumen's reflections); the rect light lights the car.
	LightRing->SetVisibleInRayTracing(false);
	LightRing->bVisibleInReflectionCaptures = false;
	GK::Section(LightRing, 0, RingGeo, RingGlow.Get());
	GK::Register(LightRing);
	DimHidden.Add(LightRing);

	CarLight = GK::NewPart<URectLightComponent>(this, TEXT("CarLight"));
	CarLight->SetRelativeLocationAndRotation(FVector(0, 0, (GK::CanopyZ - 0.03) * GK::Cm), FRotator(-90, 0, 0));   // facing down
	CarLight->SetIntensityUnits(ELightUnits::Lumens);
	CarLight->SetSourceWidth(260.f);
	CarLight->SetSourceHeight(260.f);
	// Unshadowed, so no further than its own floor and landing (2.54 m below it) and a little more: 9 m reached from
	// the Atrium through its floor and the slab into the Square's galleries and dark rooms.
	CarLight->SetAttenuationRadius(380.f);
	CarLight->SetCastShadows(false);
	// Diffuse only: its 2.6 m panel would mirror in the curved glass as a wide white band (the ring of
	// light is what you see; this is what lights the floor).
	CarLight->SetSpecularScale(0.f);
	CarLight->SetUseTemperature(true);
	CarLight->SetTemperature(LightKelvin);
	GK::Register(CarLight);

	// The crown over the glass roof: the housing of the spiral-band mast the car hangs from in the Cube (its column is
	// formed here, from two interlocking steel bands, and fed out or taken back in). A gilt drum on four arms that bear
	// on the top ring (on the diagonals, clear of the door), a bead round its top; the mast's head rides on it. It stays
	// when the glass clears in the Cube: the car hangs from it.
	{
		FMuseeMesh CrownGeo;
		const double Base = GK::CanopyZ + 0.008;
		GK::Band(CrownGeo, 0, GK::CrownR, Base, GK::CrownTop - 0.012, 0, GK::Turn, false);
		GK::Tube(CrownGeo, GK::CrownR - 0.012, 0.012, GK::CrownTop - 0.012, 0, GK::Turn);
		GK::Band(CrownGeo, 0, GK::CrownR - 0.024, GK::CrownTop - 0.024, GK::CrownTop, 0, GK::Turn, false);
		GK::Band(CrownGeo, GK::CrownR - 0.004, GK::CrownR + 0.012, Base, Base + 0.03, 0, GK::Turn, false);
		for (const double Bearing : {45.0, 135.0, 225.0, 315.0})
		{
			const double A = FMath::DegreesToRadians(Bearing - 90.0);
			const double From = GK::CrownR - 0.03, To = GK::CarRingOut - 0.01;
			GK::Block(CrownGeo, GK::Polar((From + To) / 2, A, GK::TopRingTop + 0.016), GK::Outward(A), GK::Along(A), Up, FVector((To - From) / 2, 0.022, 0.016));
		}
		UProceduralMeshComponent* Crown = GK::NewMesh(this, TEXT("CarCrown"), true);
		GK::Section(Crown, 0, CrownGeo, Look.Bronze);
		GK::Register(Crown);
	}

	// The walls' collision, with the door gap; the leaves carry their own.
	GK::Wall(this, TEXT("CarWall"), GK::CarR, GK::DoorTheta + GK::DoorDeg * GK::Deg, GK::DoorTheta + (360.0 - GK::DoorDeg) * GK::Deg,
			 -0.05, MuseePlan::Elan::CarHeight, Colliders);

	SetDoors(0.f);
	SetDim(0.f);
	ApplyGlow();
}

UPrimitiveComponent* UElanCar::FloorBody() const
{
	return FloorMesh.Get();
}

void UElanCar::Place(double FloorHeight, double Speed)
{
	// Élan Cube: under the Atrium floor the glass reflects only the dark shaft and room; the sky's cube map (all that
	// translucent glass sees behind its front layer) would light it white, so its reflections are turned down there.
	const bool bUnder = FloorHeight < -0.6;
	if (bUnder != bUnderground)
	{
		bUnderground = bUnder;
		// (The floor keeps its sheen: underground the sun is kept off the glass (AElanElevator), and the car's own glimmer is
		// what it shows in the Cube.)
		const float K = bUnder ? 0.12f : 1.f;
		if (ClearGlass) { ClearGlass->SetScalarParameterValue(TEXT("Specular"), GlassSpecular * K); }
		if (RailGlass) { RailGlass->SetScalarParameterValue(TEXT("Specular"), GlassSpecular * FMath::Max(K, 0.35f)); }
	}
	const FVector Where(0, 0, FloorHeight * MuseePlan::Cm);
	if (!GetRelativeLocation().Equals(Where, 0.01)) { SetRelativeLocation(Where); }
	// The floor's velocity, for the visitor stepping off a moving base.
	if (FloorMesh) { FloorMesh->ComponentVelocity = FVector(0, 0, Speed * MuseePlan::Cm); }
}

void UElanCar::SetDoors(float Open)
{
	if (FMath::Abs(Open - AppliedOpen) < 1e-4f) { return; }
	AppliedOpen = Open;
	GK::TurnLeaves(LeafPivots, Open);
}

void UElanCar::SetDim(float Amount)
{
	Amount = FMath::Clamp(Amount, 0.f, 1.f);
	const bool bEnds = (Amount <= 0.f) != (AppliedDim <= 0.f) || (Amount >= 1.f) != (AppliedDim >= 1.f);
	if (!bEnds && FMath::Abs(Amount - AppliedDim) < 0.004f) { return; }
	AppliedDim = Amount;
	// The glass fades by its own opacity; what can't fade (bronze, the ring of light) goes at the end.
	const float Keep = 1.f - Amount;
	if (ClearGlass) { ClearGlass->SetScalarParameterValue(TEXT("Opacity"), GlassOpacity * Keep); }
	// Élan Cube: the frosted floor stays (it hides the floor panels below); cleared, the car's light is all but out, and
	// the frost's own diffuse goes with it (a translucent pane would otherwise catch light that never reaches the room).
	if (FloorGlass) { FloorGlass->SetScalarParameterValue(TEXT("Diffuse"), 0.08f * Keep); }
	const bool bShow = Amount < 0.98f;
	for (UPrimitiveComponent* Part : FadingGlass) { if (Part) { Part->SetVisibility(bShow); } }
	for (UPrimitiveComponent* Part : DimHidden) { if (Part) { Part->SetVisibility(bShow); } }
	// The light stays, low: cleared, the car keeps a glimmer on its floor and rail (the Cube is kept black).
	if (CarLight)
	{
		CarLight->SetVisibility(true);
	}
	// Cleared (the Cube's centre), the floor and the rail show by the glimmer their frost scatters.
	if (FrostFloor && bFrostShown == bShow)
	{
		bFrostShown = !bShow;
		if (FloorMesh) { FloorMesh->SetMaterial(0, bFrostShown ? static_cast<UMaterialInterface*>(FrostFloor.Get()) : FloorGlass.Get()); }
		if (RailMesh) { RailMesh->SetMaterial(0, bFrostShown ? static_cast<UMaterialInterface*>(FrostRail.Get()) : RailGlass.Get()); }
	}
	ApplyGlow();
}

void UElanCar::SetGlimmer(float Fraction)
{
	Fraction = FMath::Clamp(Fraction, 0.f, 1.f);
	if (FMath::IsNearlyEqual(Fraction, Glimmer, 1e-4f)) { return; }
	Glimmer = Fraction;
	ApplyGlow();
}

void UElanCar::SetGlow(float Amount)
{
	Amount = FMath::Clamp(Amount, 0.f, 1.f);
	if (FMath::Abs(Amount - AppliedGlow) < 0.01f && (Amount > 0.f) == (AppliedGlow > 0.f)) { return; }
	AppliedGlow = Amount;
	ApplyGlow();
}

void UElanCar::ApplyGlow()
{
	// A quarter brighter while it runs (it glows down through the misty glass); out at the top.
	const float Keep = 1.f - FMath::Clamp(AppliedDim, 0.f, 1.f);
	const float Boost = 1.f + 0.25f * AppliedGlow;
	if (RingGlow) { RingGlow->SetScalarParameterValue(TEXT("Luminance"), RingNits * Boost * Keep); }
	if (CarLight) { CarLight->SetIntensity(LightLumens * Boost * FMath::Max(Keep, Glimmer)); }   // cleared: a glimmer (3 %), the Cube kept dark
}

// ---------------------------------------------------------------------------------------------
// A landing

UElanLanding::UElanLanding()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UElanLanding::Build(double Height, double TubeTop, double CollarRadius)
{
	if (bBuilt || !GetOwner()) { return; }
	bBuilt = true;

	ClearGlass = GK::GlassInstance(this, GlassOpacity, 0.02f, GlassSpecular);
	GK::FLook Look;
	Look.Clear = ClearGlass.Get();
	Look.Bronze = GK::LoadMaterial(GK::GiltPath);
	Look.Edge = GK::EdgeInstance(this);

	// The glass, from the shoe to the head ring; the leaves run on the sill.
	const double PaneBottom = GK::ShoeTop - 0.01, PaneTop = Height - 0.06;
	GK::FixedGlass(this, TEXT("ShaftPane"), GK::ShaftR, PaneBottom, PaneTop, GK::DoorSpans(), Look, Panes);
	for (const double Side : {-1.0, 1.0})
	{
		LeafPivots.Add(GK::Leaf(this, TEXT("ShaftLeaf"), GK::ShaftLeafR, GK::SillTop, PaneTop, Side, Look, Panes, Colliders));
	}

	// Bronze: the shoe (open at the door), the head ring over the door, the curved sill across it
	// (8 mm proud of the floor, from the car's base ring to outside the glass).
	FMuseeMesh Metal, Pearl;
	const double Jamb0 = GK::DoorTheta + GK::DoorDeg * GK::Deg, Jamb1 = GK::DoorTheta + (360.0 - GK::DoorDeg) * GK::Deg;
	GK::Band(Metal, GK::ShaftRingIn, GK::ShaftRingOut, -0.01, GK::ShoeTop, Jamb0, Jamb1, true);
	GK::Band(Metal, GK::ShaftRingIn, GK::ShaftRingOut, Height - 0.07, Height, 0, GK::Turn, false);
	GK::Band(Metal, GK::SillIn, GK::SillOut, GK::SillBottom, GK::SillTop, GK::DoorTheta - GK::SillDeg * GK::Deg, GK::DoorTheta + GK::SillDeg * GK::Deg, true);
	// Over the head ring the glass tube carries the car's way on up, in tiers of about 2.6 m with a
	// thin bronze ring between them, its panes over the enclosure's. Each tier's panes are their own
	// primitives, so they sort against the car (25 cm inside them) by distance.
	if (TubeTop > Height + 0.5)
	{
		const int32 Tiers = FMath::Max(1, FMath::RoundToInt32((TubeTop - Height) / GK::TubeRingEvery));
		const double Tier = (TubeTop - Height) / Tiers;
		for (int32 t = 0; t < Tiers; ++t)
		{
			const double Z0 = Height + Tier * t, Z1 = Z0 + Tier;
			const bool bTop = t == Tiers - 1;
			// Into the rings at both ends (the top one: into the collar, or into the ceiling).
			const double GlassTo = bTop ? (CollarRadius > 0 ? Z1 - 0.03 : Z1 + 0.05) : Z1 + 0.012;
			GK::FixedGlass(this, TEXT("TubePane"), GK::ShaftR, Z0 - 0.012 - (t == 0 ? 0.02 : 0.0), GlassTo, GK::TubeSpans(), Look, Panes);
			if (!bTop) { GK::Band(Metal, GK::TubeRingIn, GK::TubeRingOut, Z1 - 0.02, Z1 + 0.02, 0, GK::Turn, false); }
		}
		if (CollarRadius > 0)
		{
			// At the Atrium: a collar from the tube out under the ring round the opening, holding the iris (its open
			// blades lie 2 cm under the top, r 2.46 … 4.01, inside the collar's solid). Pearl like the ribs and the ring;
			// a flat pearl plate 8 m across read as a big grey disc, so its underside is modelled to break the scale
			// down, as a coffered soffit is: a deep gilt hub round the tube; an inner ring; a coffered band of 24
			// radial spokes (every other one under an Atrium rib's bearing, 15° + 30°k from north) with a fine gilt
			// line along each spoke; an outer ring; and a moulded edge: a gilt bead on the soffit's arris, an ovolo
			// rising to a plain fascia, and a gilt lip at the top. Every ring's arris carries a fine gilt line.
			// Heights from the collar's top (TubeTop); nothing rises above it (the compression ring is 3 cm over).
			const double T = TubeTop, RIn = GK::ShaftRingIn + 0.105, ROut = CollarRadius;
			constexpr double Ring = -0.26, CofferBack = -0.17, SpokeDepth = 0.05;
			const double RStep0 = 2.80, RStep1 = 3.62, REdge = ROut - 0.14;   // the inner ring, the coffers, the outer ring
			TArray<FVector2D> Body;   // (r, h), closed: soffit, edge, top, back down the hub
			Body.Add(FVector2D(RIn, T + Ring));
			Body.Add(FVector2D(RStep0, T + Ring));
			Body.Add(FVector2D(RStep0, T + CofferBack));
			Body.Add(FVector2D(RStep1, T + CofferBack));
			Body.Add(FVector2D(RStep1, T + Ring));
			Body.Add(FVector2D(REdge + 0.04, T + Ring));
			// The ovolo: a quarter round (R 0.08) from the soffit up to the fascia.
			constexpr int32 OvoloSteps = 10;
			const FVector2D OvoloC(ROut - 0.08, T + Ring + 0.01 + 0.08);
			Body.Add(FVector2D(OvoloC.X, T + Ring + 0.01));
			for (int32 i = 1; i <= OvoloSteps; ++i)
			{
				const double A = -0.5 * UE_DOUBLE_PI + 0.5 * UE_DOUBLE_PI * i / OvoloSteps;
				Body.Add(OvoloC + FVector2D(FMath::Cos(A), FMath::Sin(A)) * 0.08);
			}
			Body.Add(FVector2D(ROut, T - 0.03));
			Body.Add(FVector2D(ROut, T));
			Body.Add(FVector2D(RIn, T));
			Body.Add(FVector2D(RIn, T + Ring));
			Pearl.Lathe(Body, 192, false, FVector2D(26, 1));
			// The spokes: 24, hanging 5 cm from the coffers' backs to the rings' level less 4 cm, into both risers.
			FMuseeMesh Lines;
			const FVector Up(0, 0, 1);
			const double SpokeMid = 0.5 * (RStep0 + RStep1), SpokeHalf = 0.5 * (RStep1 - RStep0) + 0.004;
			const double SpokeZ = T + CofferBack - SpokeDepth / 2 + 0.0025;
			for (int32 k = 0; k < 24; ++k)
			{
				const double A = FMath::DegreesToRadians(15.0 + 15.0 * k - 90.0);   // compass bearings 15° + 15°k
				GK::Block(Pearl, GK::Polar(SpokeMid, A, SpokeZ), GK::Outward(A), GK::Along(A), Up, FVector(SpokeHalf, 0.022, SpokeDepth / 2 + 0.0025));
				GK::Block(Lines, GK::Polar(SpokeMid, A, T + CofferBack - SpokeDepth - 0.001), GK::Outward(A), GK::Along(A), Up,
						  FVector(SpokeHalf - 0.004, 0.006, 0.0018));
			}
			// Fine gilt lines on the arrises: the hub's foot, both rings' edges, the bead at the soffit's edge, the lip.
			GK::Band(Metal, GK::ShaftRingIn, RIn + 0.004, T + Ring - 0.04, T, 0, GK::Turn, false);   // the hub, 4 cm deeper
			GK::Tube(Lines, RIn + 0.01, 0.008, T + Ring - 0.002, 0, GK::Turn);
			GK::Tube(Lines, RStep0 - 0.004, 0.007, T + Ring + 0.002, 0, GK::Turn);
			GK::Tube(Lines, RStep1 + 0.004, 0.007, T + Ring + 0.002, 0, GK::Turn);
			GK::Tube(Lines, REdge, 0.016, T + Ring + 0.004, 0, GK::Turn);
			GK::Band(Lines, ROut - 0.01, ROut + 0.02, T - 0.03, T + 0.0, 0, GK::Turn, false);
			UMaterialInterface* Clean = GK::LoadMaterial(TEXT("/Game/Museum/Materials/M_Gilt_Clean.M_Gilt_Clean"));
			UProceduralMeshComponent* CollarLines = GK::NewMesh(this, TEXT("CollarLines"), true);
			GK::Section(CollarLines, 0, Lines, Clean ? Clean : Look.Bronze);
			GK::Register(CollarLines);
		}
		else
		{
			// At the Square: a ring into the ceiling.
			GK::Band(Metal, GK::ShaftRingIn, GK::ShaftRingOut, TubeTop - 0.07, TubeTop + 0.05, 0, GK::Turn, false);
		}
	}
	UProceduralMeshComponent* Bronze = GK::NewMesh(this, TEXT("LandingBronze"), true);
	GK::Section(Bronze, 0, Metal, Look.Bronze);
	if (Pearl.Vertices.Num() > 0) { GK::Section(Bronze, 1, Pearl, GK::LoadMaterial(GK::PearlPath)); }
	GK::Register(Bronze);

	// Collision: the wall with the door gap (the leaves carry their own), and the sill to walk over.
	GK::Wall(this, TEXT("ShaftWall"), GK::ShaftR, Jamb0, Jamb1, -0.05, Height, Colliders);
	UBoxComponent* Sill = GK::NewPart<UBoxComponent>(this, TEXT("SillFloor"));
	const double SillMid = (GK::SillIn + GK::SillOut) / 2;
	Sill->InitBoxExtent(FVector((GK::SillOut - GK::SillIn) / 2, GK::SillOut * FMath::Sin(GK::SillDeg * GK::Deg), (GK::SillTop - GK::SillBottom) / 2) * GK::Cm);
	Sill->SetRelativeLocationAndRotation(GK::Polar(SillMid, GK::DoorTheta, (GK::SillTop + GK::SillBottom) / 2) * GK::Cm, FRotator(0, 180, 0));
	Sill->SetHiddenInGame(true);
	GK::Solid(Sill, true);
	GK::Register(Sill);
	Colliders.Add(Sill);

	SetDoors(0.f);
}

void UElanLanding::SetDoors(float Open)
{
	if (FMath::Abs(Open - AppliedOpen) < 1e-4f) { return; }
	AppliedOpen = Open;
	GK::TurnLeaves(LeafPivots, Open);
}

void UElanLanding::SetCallable(bool bCall)
{
	if (bCall == bCallableNow) { return; }
	bCallableNow = bCall;
	for (UBoxComponent* Box : Colliders)
	{
		if (Box) { Box->SetCollisionResponseToChannel(ECC_Visibility, bCall ? ECR_Block : ECR_Ignore); }
	}
}
