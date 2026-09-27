#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "LightLuminaire.h"
#include "LightLuminaireComponent.h"
#include "LightSky.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Schedule/LightDresser.h"
#include "Sky/SkyPhotometry.h"
#include "TimeMoment.h"

// Light's connectors: the catalogue, the sky's photometry, the luminaire, the stage dresser and the sky.
namespace Musee::Light::Tests
{
	inline constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	/** A game world for one test, destroyed with the scope. */
	class FLightTestWorld
	{
	public:
		FLightTestWorld() : World(UWorld::CreateWorld(EWorldType::Game, false)) {}
		~FLightTestWorld() { World->DestroyWorld(false); }
		UWorld* operator->() const { return World; }
		UWorld* Get() const { return World; }

	private:
		UWorld* World;
	};

	/** An actor with a root, as a stage actor is made. */
	AActor* SpawnRootedActor(UWorld& World)
	{
		AActor* Actor = World.SpawnActor<AActor>();
		USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"));
		Actor->SetRootComponent(Root);
		Actor->AddInstanceComponent(Root);
		Root->RegisterComponent();
		return Actor;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLightCatalogueTest, "Musee.Light.Catalogue", Flags)
	bool FLightCatalogueTest::RunTest(const FString& Parameters)
	{
		TestEqual(TEXT("Seventeen luminaires in the catalogue"), LuminaireKeys().Num(), 17);
		const FLightLuminaire* Spot = FindLuminaire(FMuseeKey(TEXTVIEW("track-spot.3000k.20deg")));
		if (TestNotNull(TEXT("The 20 degree spot"), Spot))
		{
			TestTrue(TEXT("is lit, as a spot"), Spot->bLit && Spot->Emitter == FLightLuminaire::EEmitter::Spot);
			TestEqual(TEXT("with its published peak intensity"), Spot->PeakIntensity.Get(), 10963.0);
			TestEqual(TEXT("and beam angle"), Spot->BeamAngle.Get(), 20.0);
		}
		const FLightLuminaire* Strip = FindLuminaire(FMuseeKey(TEXTVIEW("linear-strip.2700k.slim")));
		if (TestNotNull(TEXT("The slim strip for a gilt lip"), Strip))
		{
			TestTrue(TEXT("is lit, as a line"), Strip->bLit && Strip->Emitter == FLightLuminaire::EEmitter::Linear);
			TestEqual(TEXT("at 1950 lm a metre"), Strip->FluxPerMeterLumens, 1950.0);
			TestTrue(TEXT("5 mm wide, so it fits the lip's 6 mm slot"), FMath::IsNearlyEqual(Strip->EmittingWidth.Get(), 0.005));
			TestEqual(TEXT("at 2700 K"), Strip->ColourTemperature.Get(), 2700.0);
		}
		const FLightLuminaire* Slot = FindLuminaire(FMuseeKey(TEXTVIEW("linear-slot.3000k")));
		TestTrue(TEXT("A record without its flux stands unlit"), Slot && !Slot->bLit && !Slot->WhyUnlit.IsEmpty());
		const FLightLuminaire* Washer = FindLuminaire(FMuseeKey(TEXTVIEW("track-wallwasher.3000k")));
		TestTrue(TEXT("A wallwasher waits for its photometric file"), Washer && !Washer->bLit);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLightPhotometryTest, "Musee.Light.Photometry", Flags)
	bool FLightPhotometryTest::RunTest(const FString& Parameters)
	{
		using namespace SkyPhotometry;
		TestEqual(TEXT("The sun at 1 AU gives the luminous solar constant"), SunIlluminance(1.0).Get(), 133800.0);
		TestTrue(TEXT("and less at aphelion, by the square of the distance"), FMath::IsNearlyEqual(SunIlluminance(1.0167).Get(), 133800.0 / (1.0167 * 1.0167)));
		TestTrue(TEXT("The sun's disc is about 0.53 degrees across"), FMath::IsNearlyEqual(SunAngularDiameter(1.0).Get(), 0.5331, 1e-3));
		TestTrue(TEXT("The full Moon gives about a third of a lux"), FMath::IsNearlyEqual(FullMoonIlluminance().Get(), 0.333, 0.01));
		TestTrue(TEXT("A quarter Moon gives much less"), MoonIlluminance(Degrees(90.0), 384400.0).Get() < FullMoonIlluminance().Get() / 5.0);
		TestTrue(TEXT("The Moon's disc is about 0.52 degrees across"), FMath::IsNearlyEqual(MoonAngularDiameter(384400.0).Get(), 0.518, 1e-3));
		TestEqual(TEXT("Clear air adds no haze"), PrecipitationExtinctionPerMeter(0.0, 0.0), 0.0);
		const double Rain = PrecipitationExtinctionPerMeter(5.0, 0.0);
		TestTrue(TEXT("5 mm/h of rain limits sight to a few kilometres (Koschmieder, 3.912 / extinction)"), 3.912 / Rain > 3000.0 && 3.912 / Rain < 8000.0);
		TestTrue(TEXT("Snow hides more than rain of the same water"), PrecipitationExtinctionPerMeter(0.0, 5.0) > Rain);
		TestTrue(TEXT("The engine's fog density is ten times the extinction per metre"), FMath::IsNearlyEqual(EngineFogDensity(Rain), static_cast<float>(10.0 * Rain)));
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLightLuminaireTest, "Musee.Light.Luminaire", Flags)
	bool FLightLuminaireTest::RunTest(const FString& Parameters)
	{
		const FLightTestWorld World;
		AActor* Actor = SpawnRootedActor(*World.Get());
		ULightLuminaireComponent* Rim = NewObject<ULightLuminaireComponent>(Actor, TEXT("Rim"));
		Rim->Configure(FMuseeKey(TEXTVIEW("linear-strip.2700k.slim")), ELightOperation::Always, FMuseeKey(TEXTVIEW("test.pond.rim")), Meters(0.594));
		Rim->SetupAttachment(Actor->GetRootComponent());
		Rim->RegisterComponent();
		const URectLightComponent* Line = Cast<URectLightComponent>(Rim->Emitter());
		if (!TestNotNull(TEXT("A linear luminaire lights as a rect light"), Line)) { return false; }
		TestTrue(TEXT("as long as the length laid"), FMath::IsNearlyEqual(Line->SourceWidth, 59.4f, 1e-3f));
		TestTrue(TEXT("in lumens, at its catalogue output"), Line->IntensityUnits == ELightUnits::Lumens && FMath::IsNearlyEqual(Line->Intensity, 1950.0f * 0.594f, 0.01f));
		TestTrue(TEXT("at its colour temperature"), Line->bUseTemperature && FMath::IsNearlyEqual(Line->Temperature, 2700.0f));
		Rim->FollowChannel(0.5f);
		TestTrue(TEXT("The channel it follows dims it"), FMath::IsNearlyEqual(Rim->EmittedFlux().Get(), 1950.0 * 0.594 * 0.5, 1e-6));
		Rim->FollowChannel(0.0f);
		TestFalse(TEXT("and at 0 it is dark"), Line->IsVisible());

		ULightLuminaireComponent* Lamp = NewObject<ULightLuminaireComponent>(Actor, TEXT("Lamp"));
		Lamp->Configure(FMuseeKey(TEXTVIEW("track-spot.3000k.20deg")), ELightOperation::Dusk, FMuseeKey(), Meters(0.0));
		Lamp->SetupAttachment(Actor->GetRootComponent());
		Lamp->RegisterComponent();
		const USpotLightComponent* Spot = Cast<USpotLightComponent>(Lamp->Emitter());
		if (!TestNotNull(TEXT("A spot lights as a spot light"), Spot)) { return false; }
		TestTrue(TEXT("at its peak intensity in candelas"), Spot->IntensityUnits == ELightUnits::Candelas && FMath::IsNearlyEqual(Spot->Intensity, 10963.0f));
		TestTrue(TEXT("its beam's half-angle inside"), FMath::IsNearlyEqual(Spot->InnerConeAngle, 10.0f));
		TestTrue(TEXT("its field's outside"), FMath::IsNearlyEqual(Spot->OuterConeAngle, 18.226f, 1e-2f));
		FTimeMoment Day;
		Day.bSunUp = true;
		Lamp->FollowMoment(Day);
		TestEqual(TEXT("A dusk luminaire is off while the sun is up"), Lamp->Level(), 0.0);
		FTimeMoment Night;
		Lamp->FollowMoment(Night);
		TestEqual(TEXT("and on once it is down"), Lamp->Level(), 1.0);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLightDresserTest, "Musee.Light.Dresser", Flags)
	bool FLightDresserTest::RunTest(const FString& Parameters)
	{
		const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::AutomationTransientDir() / TEXT("MuseeLight/Stage"));
		const FString Schedule = TEXT(R"({ "stage": "test", "daylight": [ { "opening": "roof", "glazing": "low-iron-glass", "weather": "paris", "control": "none" } ],
			"luminaires": [
				{ "id": "rim.1", "type": "linear-strip.2700k.slim", "mount": "test.rim.1", "aim": [0, 0, 1], "operation": "always", "follows": "test.pond.rim", "body": "test.basin", "length": 0.594 },
				{ "id": "lost", "type": "no-such.lamp", "mount": "test.rim.1", "operation": "always" },
				{ "id": "short", "type": "linear-strip.2700k.slim", "mount": "test.rim.1", "operation": "always" } ] })");
		FFileHelper::SaveStringToFile(Schedule, *(Folder / TEXT("Lighting.json")));

		const FLightTestWorld World;
		AActor* Stage = SpawnRootedActor(*World.Get());
		USceneComponent* Basin = NewObject<USceneComponent>(Stage, TEXT("Basin"));
		Basin->ComponentTags.Add(TEXT("test.basin"));
		Basin->SetupAttachment(Stage->GetRootComponent());
		Basin->SetRelativeLocation(FVector(0.0, 0.0, 100.0));
		Basin->RegisterComponent();

		FMuseeStageContext Context;
		Context.Stage = FMuseeKey(TEXTVIEW("test"));
		Context.ScheduleDir = Folder;
		Context.StageActor = Stage;
		Context.Anchors.Add({ FMuseeKey(TEXTVIEW("test.rim.1")), TEXT("Mount"), FMuseeFrame(FVector(1.0, 0.0, 1.39)) });
		TArray<FMuseeFinding> Findings;
		DressStage(Context, Findings);

		TestEqual(TEXT("An unknown weather place, an unknown type and a line without length are findings"), Findings.Num(), 3);
		TArray<ULightLuminaireComponent*> Placed;
		Stage->GetComponents(Placed);
		if (TestEqual(TEXT("One luminaire is placed"), Placed.Num(), 1))
		{
			ULightLuminaireComponent* Rim = Placed[0];
			TestTrue(TEXT("on the moving body"), Rim->GetAttachParent() == Basin);
			TestTrue(TEXT("at its mount in the stage"), Rim->GetComponentLocation().Equals(FVector(100.0, 0.0, 139.0), 0.01));
			TestTrue(TEXT("its light leaving along the aim"), Rim->GetForwardVector().Equals(FVector::UpVector, 1e-4));
			TestTrue(TEXT("tagged with its id"), Rim->ComponentHasTag(TEXT("musee.luminaire:rim.1")));
			TestTrue(TEXT("following its channel"), Rim->Follows() == FMuseeKey(TEXTVIEW("test.pond.rim")));
			TestTrue(TEXT("kept with the stage"), Stage->GetInstanceComponents().Contains(Rim));
		}
		IFileManager::Get().DeleteDirectory(*FPaths::GetPath(Folder), false, true);
		return true;
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLightSkyTest, "Musee.Light.Sky", Flags)
	bool FLightSkyTest::RunTest(const FString& Parameters)
	{
		const FLightTestWorld World;
		ALightSky* Sky = World->SpawnActor<ALightSky>();
		if (!TestNotNull(TEXT("The sky"), Sky)) { return false; }
		FTimeMoment Moment;
		Moment.SunDirection = FVector(0.0, 0.3, 0.95).GetSafeNormal();
		Moment.SunElevationDegrees = 71.5;
		Moment.bSunUp = true;
		Moment.SunDistanceAstronomicalUnits = 1.0;
		Moment.MoonDirection = FVector(0.0, 0.0, -1.0);
		Moment.MoonElevationDegrees = -90.0;
		Moment.MoonDistanceKilometers = 384400.0;
		FTimeWeather& Museum = Moment.Weather.AddDefaulted_GetRef();
		Museum.Place = TEXT("museum");
		Museum.RainMillimetersPerHour = 5.0;
		Sky->Follow(Moment);
		TestTrue(TEXT("The sun shines from its direction"), Sky->Sun()->GetForwardVector().Equals(-Moment.SunDirection, 1e-4));
		TestTrue(TEXT("at the luminous solar constant, in lux"), FMath::IsNearlyEqual(Sky->Sun()->Intensity, 133800.0f));
		TestTrue(TEXT("and is lit while up"), Sky->Sun()->IsVisible());
		TestFalse(TEXT("The Moon below the horizon doesn't light"), Sky->Moon()->IsVisible());
		TestTrue(TEXT("Rain hazes the air"), Sky->Fog()->FogDensity > 0.0f);
		Moment.SunElevationDegrees = -12.0;
		Sky->Follow(Moment);
		TestFalse(TEXT("After civil twilight the sun is out"), Sky->Sun()->IsVisible());
		return true;
	}
}

#endif
