#include "Sky/MuseeSky.h"

#include "MuseeVision.h"
#include "Catalog/MuseeFiles.h"
#include "Plan/MuseePlan.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "ProceduralMeshComponent.h"
#include "HAL/IConsoleManager.h"
#include "Components/LocalLightComponent.h"
#include "EngineUtils.h"

static TAutoConsoleVariable<float> CVarMuseeHour(
	TEXT("musee.Hour"), -1.f,
	TEXT("Show the sky at this local hour (0-24) instead of the live clock; -1 for live."),
	ECVF_Default);

AMuseeSky::AMuseeSky()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(RootComponent);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetAtmosphereSunLight(true);
	Sun->SetAtmosphereSunLightIndex(0);
	Sun->SetIntensity(SunLux);
	Sun->SetLightColor(FLinearColor(FColor(0xFF, 0xF6, 0xEC)));   // a warm white, not amber
	Sun->SetLightSourceAngle(0.5357f);
	Sun->SetCastShadows(true);
	Sun->ForwardShadingPriority = 1;   // the sun, not the moon, lights translucency, water and fog

	MoonLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon"));
	MoonLight->SetupAttachment(RootComponent);
	MoonLight->SetMobility(EComponentMobility::Movable);
	MoonLight->SetAtmosphereSunLight(true);
	MoonLight->SetAtmosphereSunLightIndex(1);
	MoonLight->SetIntensity(MoonLux);
	MoonLight->SetLightColor(FLinearColor(FColor(0xC8, 0xD4, 0xE8)));
	MoonLight->SetLightSourceAngle(0.52f);
	MoonLight->SetCastShadows(true);

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(RootComponent);

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(RootComponent);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->SetRealTimeCapture(true);
	// Indoors the sky arrives through laylights, velaria and skylight glass that scatter it: a
	// little warmer than the open sky's blue, so the stone under the lanterns reads neutral, not slate
	// (no warmer: the rooms are to read in neutral light).
	SkyLight->SetLightColor(FLinearColor(1.0f, 0.965f, 0.91f));

	Stars = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Stars"));
	Stars->SetupAttachment(RootComponent);
	Stars->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Stars->SetCastShadow(false);
	Stars->bAffectDistanceFieldLighting = false;
	Stars->SetAbsolute(true, true, true);

	StarMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Museum/Sky/M_Stars.M_Stars")));
	MilkyWayMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Museum/Sky/M_MilkyWay.M_MilkyWay")));
	Parameters = TSoftObjectPtr<UMaterialParameterCollection>(FSoftObjectPath(TEXT("/Game/Museum/Materials/MPC_Musee.MPC_Musee")));
}

void AMuseeSky::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Sun->SetIntensity(SunLux);
	MoonLight->SetIntensity(MoonLux);
	Refresh();
}

void AMuseeSky::BeginPlay()
{
	Super::BeginPlay();
	Observer = FMuseeObserver::FromTimeZone();
	BuildStars();
	Refresh();
}

void AMuseeSky::SetObserver(double Latitude, double Longitude)
{
	Observer.Latitude = FMath::Clamp(Latitude, -90.0, 90.0);
	Observer.Longitude = FMath::Clamp(Longitude, -180.0, 180.0);
	Refresh();
}

FDateTime AMuseeSky::LocalNow() const
{
	return MuseeClock::LocalNow();   // the museum's city (musee.City), or musee.Hour
}

FDateTime AMuseeSky::UtcNow() const
{
	return MuseeClock::UtcNow();
}

FVector AMuseeSky::SunClockDirection(double LocalHour)
{
	// Angle from west (VI) through north (XII) to east (VI); the point of shadow sits 6.78 m out.
	const double Phi = FMath::DegreesToRadians((LocalHour - 6.0) * 15.0);
	const FVector Point = MuseePlan::At(-FMath::Cos(Phi) * MuseePlan::Rotunda::ShadowRadius, -FMath::Sin(Phi) * MuseePlan::Rotunda::ShadowRadius, 0);
	const FVector Node = MuseePlan::At(0, 0, MuseePlan::Rotunda::LatticeHeight);
	return (Point - Node).GetSafeNormal();
}

void AMuseeSky::SetSphereMode(bool bInSphere)
{
	if (bSphereMode == bInSphere) { return; }
	bSphereMode = bInSphere;
	Refresh();
}

void AMuseeSky::Refresh()
{
	if (bJourneySky) { UpdateJourneySky(); return; }
	Observer = MuseeClock::Observer();   // follows the city (C switches it)
	UpdateSun(LocalNow());
	UpdateMoonAndStars(UtcNow());
}

void AMuseeSky::UpdateSun(const FDateTime& Local)
{
	const double Hour = Local.GetHour() + Local.GetMinute() / 60.0 + Local.GetSecond() / 3600.0;
	const bool bDay = Hour >= 6.0 && Hour <= 18.0 && !bSphereMode;
	Sun->SetVisibility(bDay);
	Sun->SetWorldRotation(SunClockDirection(FMath::Clamp(Hour, 6.0, 18.0)).Rotation());
	// How much daylight the skylights and laylights pass: the idealised sun's height, softened.
	const double Daylight = bDay ? FMath::Sqrt(FMath::Max(0.0, FMath::Sin(UE_DOUBLE_PI * (Hour - 6.0) / 12.0))) : 0.0;
	if (UMaterialParameterCollection* Collection = Parameters.LoadSynchronous())
	{
		UKismetMaterialLibrary::SetScalarParameterValue(this, Collection, TEXT("Daylight"), static_cast<float>(Daylight));
	}
	UpdateLaylights(Daylight);
	// Inside the Sphere the image is the sky all round: no atmosphere, no horizon. The sky light's
	// real-time capture needs the atmosphere to capture (else the engine warns on screen), and the
	// building it lights is hidden in the Sphere, so it rests there too.
	Atmosphere->SetVisibility(!bSphereMode);
	SkyLight->SetRealTimeCapture(!bSphereMode);
	SkyLight->SetVisibility(!bSphereMode);
}

void AMuseeSky::UpdateLaylights(double Daylight)
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld()) { return; }   // the editor keeps the lights as placed
	if (!bLaylightsGathered)
	{
		bLaylightsGathered = true;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (!It->ActorHasTag(TEXT("musee.laylight"))) { continue; }
			float Night = 0.25f;
			for (const FName& Tag : It->Tags)
			{
				const FString T = Tag.ToString();
				if (T.StartsWith(TEXT("laylight.night:"))) { Night = FCString::Atof(*T.Mid(15)); }
			}
			TArray<ULocalLightComponent*> Lights;
			It->GetComponents(Lights);
			for (ULocalLightComponent* L : Lights) { Laylights.Add({L, L->Intensity, Night}); }
		}
	}
	if (FMath::Abs(Daylight - LastLaylightDaylight) < 0.01) { return; }
	LastLaylightDaylight = Daylight;
	for (const FLaylight& L : Laylights)
	{
		if (ULocalLightComponent* Light = L.Light.Get())
		{
			// As M_Daylit: full × max(Daylight, the night floor), so the panel and its light agree.
			Light->SetIntensity(L.Full * static_cast<float>(FMath::Max(Daylight, static_cast<double>(L.NightFloor))));
		}
	}
}

void AMuseeSky::UpdateMoonAndStars(const FDateTime& Utc)
{
	const MuseeEphemeris::FMoon Moon = MuseeEphemeris::Moon(Utc, Observer);
	MoonLight->SetWorldRotation((-Moon.Direction).Rotation());
	MoonLight->SetIntensity(MoonLux * FMath::Max(0.05, Moon.Illuminated));
	MoonLight->SetVisibility(Moon.Altitude > -0.02 || bSphereMode);

	// The stars show at night, and always inside the Sphere.
	const MuseeEphemeris::FSun RealSun = MuseeEphemeris::Sun(Utc, Observer);
	const bool bNight = FMath::RadiansToDegrees(RealSun.Altitude) < -6.0;
	Stars->SetVisibility(bNight || bSphereMode);

	// The star mesh is built in the equatorial frame with y and z swapped (Unreal is
	// left-handed), so the sky's rotation becomes a proper rotation: its columns are the
	// images of the equatorial x, z and y axes.
	const FMatrix M = MuseeEphemeris::SkyRotation(Utc, Observer);
	const FMatrix R(M.GetScaledAxis(EAxis::X), M.GetScaledAxis(EAxis::Z), M.GetScaledAxis(EAxis::Y), FVector::ZeroVector);
	Stars->SetWorldRotation(FQuat(R));
}

void AMuseeSky::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// The stars are at infinity: keep the field centred on the eye.
	if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		Stars->SetWorldLocation(Camera->GetCameraLocation());
	}
	if (bJourneySky)
	{
		UpdateJourneySky();
		return;
	}
	UpdateUnderground(DeltaSeconds);
	Timer += DeltaSeconds;
	const int32 City = static_cast<int32>(MuseeClock::City());
	if (Timer > 20.f || CVarMuseeHour.GetValueOnGameThread() >= 0.f || City != LastCity)
	{
		Timer = 0.f;
		LastCity = City;
		Refresh();
	}
}

void AMuseeSky::UpdateUnderground(float DeltaSeconds)
{
	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Camera || !SkyLight) { return; }
	namespace E = MuseePlan::Elan;
	const FVector Eye = Camera->GetCameraLocation() / MuseePlan::Cm;
	const double Reach = E::SquareHalf + E::SquareWall;
	const bool bUnder = Eye.Z < -1.0 && FMath::Abs(Eye.X - E::CentreX) < Reach && FMath::Abs(Eye.Y - E::CentreY) < Reach;
	const float Target = bUnder ? 0.f : 1.f;
	if (SkyLightFull < 0.f) { SkyLightFull = SkyLight->Intensity; }
	if (SkyLightScale == Target) { return; }
	SkyLightScale = FMath::FInterpConstantTo(SkyLightScale, Target, DeltaSeconds, 0.8f);   // over about a second
	SkyLight->SetIntensity(SkyLightFull * SkyLightScale);
}

void AMuseeSky::BuildStars()
{
	TArray<uint8> Data;
	if (!MuseeFiles::Load(TEXT("assets/sky/stars.bin"), Data) || Data.Num() < 8)
	{
		UE_LOG(LogMusee, Warning, TEXT("Sky: stars.bin not found; no star field."));
		return;
	}
	UMaterialInterface* Material = StarMaterial.LoadSynchronous();
	if (!Material)
	{
		UE_LOG(LogMusee, Warning, TEXT("Sky: %s is missing (run Scripts/setup_project.py)."), *StarMaterial.ToString());
	}

	auto ReadFloat = [&Data](int32 Offset) { float V; FMemory::Memcpy(&V, Data.GetData() + Offset, 4); return V; };
	uint32 Count = 0;
	FMemory::Memcpy(&Count, Data.GetData() + 4, 4);

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> Colours;
	TArray<FProcMeshTangent> Tangents;
	const double R = StarRadius;

	auto Colour = [](float BV) -> FColor
	{
		if (BV < 0.0f) { return FColor(0xAF, 0xC8, 0xFF); }
		if (BV < 0.3f) { return FColor(0xDC, 0xE6, 0xFF); }
		if (BV < 0.6f) { return FColor(0xFF, 0xF8, 0xEC); }
		if (BV < 1.0f) { return FColor(0xFF, 0xE7, 0xC2); }
		if (BV < 1.4f) { return FColor(0xFF, 0xD2, 0x9A); }
		return FColor(0xFF, 0xBC, 0x7A);
	};

	for (uint32 i = 0; i < Count; ++i)
	{
		const int32 O = 8 + static_cast<int32>(i) * 16;
		if (O + 16 > Data.Num()) { break; }
		const float Ra = ReadFloat(O), Dec = ReadFloat(O + 4), Mag = ReadFloat(O + 8), BV = ReadFloat(O + 12);
		if (Mag >= 6.5f) { continue; }
		// Equatorial with y and z swapped (see UpdateMoonAndStars).
		const FVector Eq(FMath::Cos(Dec) * FMath::Cos(Ra), FMath::Cos(Dec) * FMath::Sin(Ra), FMath::Sin(Dec));
		const FVector V(Eq.X, Eq.Z, Eq.Y);
		// Angular size: bright stars larger (a sprite ~0.07°–0.4° across, its round falloff a point of a few pixels;
		// at 0.2°–1.1° they read as squares on a 1600 px, 84° view).
		const double Size = R * (0.0012 + 0.0062 * FMath::Pow(FMath::Max(0.0, 6.5 - Mag) / 8.0, 1.6));
		const FVector Up = FMath::Abs(V.Z) > 0.95 ? FVector(1, 0, 0) : FVector(0, 0, 1);
		const FVector T1 = FVector::CrossProduct(Up, V).GetSafeNormal() * Size / 2;
		const FVector T2 = FVector::CrossProduct(V, T1).GetSafeNormal() * Size / 2;
		const FVector C = V * R;
		// Fainter stars dimmer: about 2.5× per magnitude, softened.
		const float Brightness = FMath::Clamp(FMath::Pow(2.512f, (1.5f - Mag) * 0.6f), 0.08f, 3.0f);
		const FLinearColor Col = FLinearColor(Colour(BV)) * Brightness;
		const int32 Base = Vertices.Num();
		Vertices.Append({C - T1 - T2, C + T1 - T2, C + T1 + T2, C - T1 + T2});
		UV0.Append({FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1), FVector2D(0, 1)});
		for (int32 k = 0; k < 4; ++k) { Normals.Add(-V); Colours.Add(Col); Tangents.Add(FProcMeshTangent(T1.GetSafeNormal(), false)); }
		// Facing the centre (Unreal's front faces: cross(b − a, c − a) points away from the viewer).
		Triangles.Append({Base, Base + 1, Base + 2, Base, Base + 2, Base + 3});
	}
	Stars->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV0, Colours, Tangents, false);
	if (Material) { Stars->SetMaterial(0, Material); }
	UE_LOG(LogMusee, Log, TEXT("Sky: %d stars."), Vertices.Num() / 4);

	// The Milky Way behind them: a sphere in the same frame, facing in, its UVs the map's plate carrée (NASA SVS: RA 0h
	// at the centre, increasing to the left; Dec +90° at the top).
	if (UMaterialInterface* MilkyWay = MilkyWayMaterial.LoadSynchronous())
	{
		TArray<FVector> MV, MN;
		TArray<FVector2D> MUV;
		TArray<int32> MT;
		constexpr int32 Around = 128, Up = 64;
		const double RM = R * 1.02;
		for (int32 j = 0; j <= Up; ++j)
		{
			const double Dec = UE_DOUBLE_HALF_PI - UE_DOUBLE_PI * j / Up;
			for (int32 i = 0; i <= Around; ++i)
			{
				const double Ra = -UE_DOUBLE_PI + UE_DOUBLE_TWO_PI * i / Around;
				const FVector V(FMath::Cos(Dec) * FMath::Cos(Ra), FMath::Sin(Dec), FMath::Cos(Dec) * FMath::Sin(Ra));
				MV.Add(V * RM);
				MN.Add(-V);
				MUV.Add(FVector2D(0.5 - Ra / UE_DOUBLE_TWO_PI, static_cast<double>(j) / Up));
			}
		}
		for (int32 j = 0; j < Up; ++j)
		{
			for (int32 i = 0; i < Around; ++i)
			{
				const int32 A = j * (Around + 1) + i, B = A + 1, C = A + Around + 1, D = C + 1;
				MT.Append({A, C, B, B, C, D});
			}
		}
		Stars->CreateMeshSection_LinearColor(1, MV, MT, MN, MUV, TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
		Stars->SetMaterial(1, MilkyWay);
	}
}

// ---------------------------------------------------------------------------------------------
// Élan Cube journeys (Cube/ElanJourney): the place's own sky.

void AMuseeSky::SetJourneySky(const FMuseeSkyOverride& In)
{
	if (!bJourneySky)
	{
		SkyLightColourAsPlaced = SkyLight->GetLightColor();
		if (SkyLightFull < 0.f) { SkyLightFull = SkyLight->Intensity; }
	}
	bJourneySky = true;
	JourneySky = In;
	UpdateJourneySky();
}

void AMuseeSky::ClearJourneySky()
{
	if (!bJourneySky) { return; }
	bJourneySky = false;
	Atmosphere->TransformMode = ESkyAtmosphereTransformMode::PlanetTopAtAbsoluteWorldOrigin;
	Atmosphere->SetRelativeLocation(FVector::ZeroVector);
	Atmosphere->MarkRenderStateDirty();
	Sun->SetIntensity(SunLux);
	Sun->SetLightColor(FLinearColor(FColor(0xFF, 0xF6, 0xEC)));
	Sun->SetLightFunctionMaterial(nullptr);
	SkyLight->SetLightColor(SkyLightColourAsPlaced);
	if (SkyLightFull >= 0.f) { SkyLight->SetIntensity(SkyLightFull * SkyLightScale); }
	Refresh();
}

void AMuseeSky::UpdateJourneySky()
{
	const FMuseeSkyOverride& J = JourneySky;
	const FQuat Turn(FVector::UpVector, FMath::DegreesToRadians(J.YawDegrees));
	// The sun: its real place for the moment and the spot, turned with the scene. Kept on a little below the horizon
	// (the atmosphere's own shadow of the Earth ends its light) so the dawn and the dusk glow come from it.
	const MuseeEphemeris::FSun RealSun = MuseeEphemeris::Sun(J.Utc, J.Observer);
	const FVector ToSun = Turn.RotateVector(RealSun.Direction);
	Sun->SetVisibility(RealSun.Altitude > FMath::DegreesToRadians(-6.0));
	Sun->SetWorldRotation((-ToSun).Rotation());
	Sun->SetIntensity(J.SunLux);
	Sun->SetLightColor(J.SunTint);
	if (Sun->LightFunctionMaterial != J.LightFunction) { Sun->SetLightFunctionMaterial(J.LightFunction); }
	const MuseeEphemeris::FMoon Moon = MuseeEphemeris::Moon(J.Utc, J.Observer);
	MoonLight->SetWorldRotation((-Turn.RotateVector(Moon.Direction)).Rotation());
	MoonLight->SetIntensity(MoonLux * FMath::Max(0.05, Moon.Illuminated) * J.SunTint.G);
	MoonLight->SetVisibility(Moon.Altitude > -0.02);
	// The stars once the sun is 6° down; the sky turned with the scene.
	Stars->SetVisibility(FMath::RadiansToDegrees(RealSun.Altitude) < -6.0 && J.bAtmosphere);
	const FMatrix M = MuseeEphemeris::SkyRotation(J.Utc, J.Observer);
	const FMatrix R(M.GetScaledAxis(EAxis::X), M.GetScaledAxis(EAxis::Z), M.GetScaledAxis(EAxis::Y), FVector::ZeroVector);
	Stars->SetWorldRotation(Turn * FQuat(R));
	// The atmosphere's ground (sea level) under the visitor.
	if (Atmosphere->TransformMode != ESkyAtmosphereTransformMode::PlanetTopAtComponentTransform)
	{
		Atmosphere->TransformMode = ESkyAtmosphereTransformMode::PlanetTopAtComponentTransform;
		Atmosphere->MarkRenderStateDirty();
	}
	if (!Atmosphere->GetComponentLocation().Equals(J.PlanetTop, 1.0))
	{
		Atmosphere->SetWorldLocation(J.PlanetTop);
		Atmosphere->MarkRenderStateDirty();
	}
	Atmosphere->SetVisibility(J.bAtmosphere);
	SkyLight->SetRealTimeCapture(J.bAtmosphere);
	SkyLight->SetVisibility(true);
	SkyLight->SetLightColor(J.SkyTint);
	if (SkyLightFull < 0.f) { SkyLightFull = SkyLight->Intensity; }
	SkyLight->SetIntensity(SkyLightFull * J.SkyLightScale);
	SkyLightScale = 1.f;
}
