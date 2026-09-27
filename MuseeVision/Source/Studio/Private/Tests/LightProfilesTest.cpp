#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/PointLightComponent.h"
#include "Engine/TextureLightProfile.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "LightLuminaire.h"
#include "LightLuminaireComponent.h"
#include "Make/PackageSave.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "MuseeStageDresser.h"
#include "Stages/LightProfiles.h"
#include "Stages/StageMake.h"
#include "UObject/Package.h"

// The light profiles Studio imports from the makers' photometric files: made the same, byte for byte, from the same
// file, and, placed as a stage's schedule places a wallwasher, throwing its wash onto the wall it is aimed at.
namespace Musee::Studio::LightProfileTests
{
	inline constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	/** The 362G wallwasher's type, and the profile its file makes. */
	const FMuseeKey WallwasherType(TEXTVIEW("track-wallwasher.3000k"));
	constexpr const TCHAR* WallwasherProfile = TEXT("IES_iguzzini_362G_O25F");

	/** Where the determinism test makes its profiles, apart from the museum's. */
	constexpr const TCHAR* SpecimenRoot = TEXT("/Game/Generated/Specimens");

	/** The tag Light gives each luminaire it places (Light's dresser). */
	constexpr const TCHAR* LuminaireTag = TEXT("musee.luminaire:");

	/** Centimetres in a metre, the engine's unit. */
	constexpr double CentimetersPerMeter = 100.0;

	/** The 362G's rated flux, and its file's greatest intensity. */
	constexpr double RatedFluxLumens = 999.0;
	constexpr double PeakCandela = 1566.3;

	/** The file's bytes on disk, or empty. */
	TArray<uint8> FileBytes(const FString& Filename)
	{
		TArray<uint8> Bytes;
		FFileHelper::LoadFileToArray(Bytes, *Filename, FILEREAD_Silent);
		return Bytes;
	}

	/** A made package's file. */
	FString FileOf(const FString& Package)
	{
		return FPackageName::LongPackageNameToFilename(Package, FPackageName::GetAssetPackageExtension());
	}

	int32 CountOf(const FMakeReport& Report, const FString& Package, EMadeOutcome Outcome)
	{
		return Report.Packages.FilterByPredicate([&](const FMadePackage& Made) { return Made.PackageName == Package && Made.Outcome == Outcome; }).Num();
	}

	/**
	 * A profile's intensity towards Direction, in candelas, as the renderer reads it for a light placed where Light is
	 * (Engine/Shaders/Private/IESLightProfilesCommon.ush): the vertical angle from the light's x axis, the horizontal from
	 * its -y axis turning towards -z, looked up bilinearly between the profile's texel centres, times the profile's own
	 * intensity, its multiplier and the light's dimming level.
	 */
	struct FPlacedProfile
	{
		FTransform Light;
		UTextureLightProfile* Profile = nullptr;
		float Level = 1.0f;
		TArray64<uint8> Texels;

		explicit FPlacedProfile(const UPointLightComponent& Lamp)
			: Light(Lamp.GetComponentTransform()), Profile(Lamp.IESTexture), Level(Lamp.IESBrightnessScale)
		{
			Profile->Source.GetMipData(Texels, 0);
		}

		double Texel(int64 X, int64 Y) const
		{
			const int64 Width = Profile->Source.GetSizeX();
			const FFloat16Color* Colours = reinterpret_cast<const FFloat16Color*>(Texels.GetData());
			return Colours[Y * Width + X].R.GetFloat();
		}

		double Sample(double U, double V) const
		{
			const int64 Width = Profile->Source.GetSizeX();
			const int64 Height = Profile->Source.GetSizeY();
			const double X = FMath::Clamp(U * Width - 0.5, 0.0, Width - 1.0);
			const double Y = FMath::Clamp(V * Height - 0.5, 0.0, Height - 1.0);
			const int64 X0 = FMath::FloorToInt64(X);
			const int64 Y0 = FMath::FloorToInt64(Y);
			const int64 X1 = FMath::Min(X0 + 1, Width - 1);
			const int64 Y1 = FMath::Min(Y0 + 1, Height - 1);
			const double A = X - X0;
			const double B = Y - Y0;
			return FMath::Lerp(FMath::Lerp(Texel(X0, Y0), Texel(X1, Y0), A), FMath::Lerp(Texel(X0, Y1), Texel(X1, Y1), A), B);
		}

		double CandelaTowards(const FVector& Direction) const
		{
			const FVector Forward = Light.GetUnitAxis(EAxis::X);
			const FVector Tangent = Light.GetUnitAxis(EAxis::Z);
			const FVector Bitangent = (Tangent ^ Forward).GetSafeNormal();
			const FVector ToLight = -Direction.GetSafeNormal();
			const FVector Local(ToLight | Forward, ToLight | Bitangent, ToLight | Tangent);
			const double U = FMath::Asin(FMath::Clamp(Local.X, -1.0, 1.0)) / UE_DOUBLE_PI + 0.5;
			const double V = FMath::Atan2(-Local.Z, -Local.Y) / (2.0 * UE_DOUBLE_PI) + 0.5;
			return Profile->Brightness * Profile->TextureMultiplier * Level * Sample(U, V);
		}

		/** The illuminance the light lays on a surface at Point (stage metres) facing Normal, in lux. */
		double LuxAt(const FVector& PointMeters, const FVector& Normal) const
		{
			const FVector Ray = (PointMeters * CentimetersPerMeter - Light.GetLocation()) / CentimetersPerMeter;
			const double Cosine = FMath::Max(0.0, -(Ray.GetSafeNormal() | Normal));
			return CandelaTowards(Ray) * Cosine / Ray.SizeSquared();
		}

		/** The flux the placed light gives, in lumens: its intensity integrated over the sphere about it. */
		double FluxLumens() const
		{
			constexpr int32 Polar = 720;
			constexpr int32 Azimuth = 720;
			const FVector X = Light.GetUnitAxis(EAxis::X);
			const FVector Y = Light.GetUnitAxis(EAxis::Y);
			const FVector Z = Light.GetUnitAxis(EAxis::Z);
			double Lumens = 0.0;
			for (int32 I = 0; I < Polar; ++I)
			{
				const double Theta = (I + 0.5) * UE_DOUBLE_PI / Polar;
				for (int32 J = 0; J < Azimuth; ++J)
				{
					const double Phi = (J + 0.5) * 2.0 * UE_DOUBLE_PI / Azimuth;
					const FVector Direction = X * FMath::Cos(Theta) + (Y * FMath::Cos(Phi) + Z * FMath::Sin(Phi)) * FMath::Sin(Theta);
					Lumens += CandelaTowards(Direction) * FMath::Sin(Theta);
				}
			}
			return Lumens * (UE_DOUBLE_PI / Polar) * (2.0 * UE_DOUBLE_PI / Azimuth);
		}
	};

	/** The wash a placed wallwasher lays: on the wall it is aimed at, on the floor at that wall's foot, and the other way. */
	struct FWash
	{
		double WallLux = 0.0;
		double FloorLux = 0.0;
		double OppositeLux = 0.0;
	};

	/**
	 * Measures the wash of a wallwasher at Seat aimed at Aim, a point on a vertical wall square to the plan's line between
	 * them, over a floor at z = 0: the mean illuminance on the wall's face up the plumb line through the aim, from the floor
	 * to the luminaire's height; on the floor at the wall's foot; and on a wall as far the other way, facing it.
	 */
	FWash MeasureWash(const FPlacedProfile& Placed, const FVector& Seat, const FVector& Aim)
	{
		const FVector Across = FVector(Aim.X - Seat.X, Aim.Y - Seat.Y, 0.0);
		const FVector TowardsWall = Across.GetSafeNormal();
		const FVector Opposite = FVector(Seat.X, Seat.Y, 0.0) - Across;
		constexpr int32 Steps = 64;
		FWash Wash;
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			const double Height = (Step + 0.5) * Seat.Z / Steps;
			Wash.WallLux += Placed.LuxAt(FVector(Aim.X, Aim.Y, Height), -TowardsWall) / Steps;
			Wash.OppositeLux += Placed.LuxAt(FVector(Opposite.X, Opposite.Y, Height), TowardsWall) / Steps;
		}
		Wash.FloorLux = Placed.LuxAt(FVector(Aim.X, Aim.Y, 0.0), FVector::UpVector);
		return Wash;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioLightProfilesTest, "Musee.Studio.LightProfiles", Musee::Studio::LightProfileTests::Flags)

bool FStudioLightProfilesTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Studio;
	using namespace Musee::Studio::LightProfileTests;

	FStageMakeOptions Options;
	Options.Paths = FMakePaths{SpecimenRoot};
	const FString Package = Options.Paths.LightProfile(WallwasherProfile);
	FMakeReport First;
	MakeLightProfiles(Options, First);
	TestTrue(TEXT("The 362G's profile is made"), First.Succeeded() && CountOf(First, Package, EMadeOutcome::Failed) == 0
		&& CountOf(First, Package, EMadeOutcome::Written) + CountOf(First, Package, EMadeOutcome::Unchanged) == 1);
	FMakeReport Again;
	MakeLightProfiles(Options, Again);
	TestEqual(TEXT("A second make writes nothing"), Again.Count(EMadeOutcome::Written), 0);

	const UTextureLightProfile* Profile = LoadObject<UTextureLightProfile>(nullptr, *(Package + TEXT(".") + WallwasherProfile));
	if (!TestNotNull(TEXT("The profile loads"), Profile)) { return false; }
	TestTrue(TEXT("as the engine's IES import makes one: 256 by 256 half-float texels, HDR, clamped, unmipped, in the IES group"),
		Profile->Source.GetSizeX() == 256 && Profile->Source.GetSizeY() == 256 && Profile->Source.GetFormat() == TSF_RGBA16F
		&& Profile->CompressionSettings == TC_HDR && Profile->AddressX == TA_Clamp && Profile->AddressY == TA_Clamp
		&& Profile->MipGenSettings == TMGS_NoMipmaps && Profile->LODGroup == TEXTUREGROUP_IESLightProfile && !Profile->SRGB);
	TestTrue(TEXT("its own intensity the file's greatest, 1566.3 cd, over a table normalised to it"),
		FMath::IsNearlyEqual(Profile->Brightness, static_cast<float>(PeakCandela), 0.01f) && Profile->TextureMultiplier == 1.0f);

	// Imported again from the same bytes, it is the same profile: the same source, whose id is the hash of its texels.
	const FLightLuminaire* Type = Musee::Light::FindLuminaire(WallwasherType);
	if (!TestNotNull(TEXT("The 362G is in the catalogue"), Type)) { return false; }
	UTextureLightProfile* Aside = NewObject<UTextureLightProfile>(GetTransientPackage());
	FString Why;
	TestTrue(TEXT("The file imports aside"), ImportLightProfile(FileBytes(Type->PhotometricFile), *Aside, Why));
	TestTrue(TEXT("to the same texels"), Aside->Source.GetId() == Profile->Source.GetId() && Aside->Brightness == Profile->Brightness);
	Aside->MarkAsGarbage();

	// Made afresh from nothing, its package is the same file, byte for byte.
	const FString Filename = FileOf(Package);
	const TArray<uint8> Before = FileBytes(Filename);
	ReleasePackages({FindPackage(nullptr, *Package)});
	IFileManager::Get().Delete(*Filename);
	FMakeReport Afresh;
	MakeLightProfiles(Options, Afresh);
	TestEqual(TEXT("Made afresh, it is written"), CountOf(Afresh, Package, EMadeOutcome::Written), 1);
	const TArray<uint8> After = FileBytes(Filename);
	TestTrue(FString::Printf(TEXT("byte for byte as before (%lld bytes)"), static_cast<int64>(Before.Num())), !Before.IsEmpty() && Before == After);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioWallwasherWashesWallTest, "Musee.Studio.WallwasherWashesWall", Musee::Studio::LightProfileTests::Flags)

bool FStudioWallwasherWashesWallTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Studio;
	using namespace Musee::Studio::LightProfileTests;

	FMakeReport Made;
	MakeLightProfiles(FStageMakeOptions(), Made);
	TestTrue(TEXT("The museum's profiles are made"), Made.Succeeded());

	// Two washers as the stages' schedules place them: the Classical Hall's washer.000, 2.82 m from its wall and aimed
	// 1.83 m down it, and Chenghuai's wash.linchi.s.01, 4.44 m across the room and 1.12 m down.
	struct FWasher { const TCHAR* Id; FVector Seat; FVector Aim; };
	const FWasher Washers[] = {
		{TEXT("hall"), FVector(3.78, 0.0, 3.86), FVector(6.6, 0.0, 2.029)},
		{TEXT("scrolls"), FVector(-19.004, -4.95, 3.02), FVector(-19.004, -0.515, 1.9)},
	};
	const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::AutomationTransientDir() / TEXT("MuseeStudio/Wallwashers"));
	FString Entries;
	for (const FWasher& Washer : Washers)
	{
		Entries += FString::Printf(TEXT("%s{ \"id\": \"%s\", \"type\": \"%s\", \"point\": [%f, %f, %f], \"aim\": [%f, %f, %f], \"operation\": \"always\" }"),
			Entries.IsEmpty() ? TEXT("") : TEXT(", "), Washer.Id, *WallwasherType.ToString(), Washer.Seat.X, Washer.Seat.Y, Washer.Seat.Z,
			Washer.Aim.X, Washer.Aim.Y, Washer.Aim.Z);
	}
	FFileHelper::SaveStringToFile(FString::Printf(TEXT("{ \"stage\": \"test\", \"luminaires\": [ %s ] }"), *Entries), *(Folder / TEXT("Lighting.json")));

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	AActor* Stage = World->SpawnActor<AActor>();
	USceneComponent* Root = NewObject<USceneComponent>(Stage, TEXT("Root"));
	Stage->SetRootComponent(Root);
	Stage->AddInstanceComponent(Root);
	Root->RegisterComponent();
	FMuseeStageContext Context;
	Context.Stage = FMuseeKey(TEXTVIEW("test"));
	Context.ScheduleDir = Folder;
	Context.StageActor = Stage;
	TArray<FMuseeFinding> Findings;
	Musee::DressStage(Context, Findings);
	TestEqual(TEXT("Light places both washers lit, with nothing to find"),
		Findings.FilterByPredicate([](const FMuseeFinding& Finding) { return Finding.Owner == FName(TEXT("Light")); }).Num(), 0);

	const FLightLuminaire* Type = Musee::Light::FindLuminaire(WallwasherType);
	TArray<ULightLuminaireComponent*> Placed;
	Stage->GetComponents(Placed);
	for (const FWasher& Washer : Washers)
	{
		const FName Tag(FString(LuminaireTag) + Washer.Id);
		ULightLuminaireComponent* const* Found = Placed.FindByPredicate([Tag](const ULightLuminaireComponent* C) { return C->ComponentHasTag(Tag); });
		const UPointLightComponent* Lamp = Found ? Cast<UPointLightComponent>((*Found)->Emitter()) : nullptr;
		if (!TestNotNull(FString::Printf(TEXT("%s lights as a source carrying its profile"), Washer.Id), Lamp)) { continue; }
		TestTrue(FString::Printf(TEXT("%s: the profile's own intensity, in candelas, at full output"), Washer.Id),
			Lamp->IESTexture == (*Found)->LightProfile() && Lamp->IESTexture != nullptr && Lamp->bUseIESBrightness
			&& Lamp->IESBrightnessScale == 1.0f && Lamp->IntensityUnits == ELightUnits::Candelas);
		const FPlacedProfile Profile(*Lamp);
		const double Flux = Profile.FluxLumens();
		TestTrue(FString::Printf(TEXT("%s gives the file's 999 lm as the renderer reads its profile, to within 2 %% (%.1f lm)"), Washer.Id, Flux),
			FMath::Abs(Flux - RatedFluxLumens) <= 0.02 * RatedFluxLumens);
		const FWash Wash = MeasureWash(Profile, Washer.Seat, Washer.Aim);
		AddInfo(FString::Printf(TEXT("%s: %.1f lx on the wall, %.1f lx on the floor at its foot, %.2f lx the other way."), Washer.Id,
			Wash.WallLux, Wash.FloorLux, Wash.OppositeLux));
		TestTrue(FString::Printf(TEXT("%s washes its wall: over twice the floor at its foot"), Washer.Id), Wash.WallLux > 2.0 * Wash.FloorLux);
		TestTrue(FString::Printf(TEXT("%s throws nothing the other way"), Washer.Id), Wash.WallLux > 0.0 && Wash.OppositeLux < 0.01 * Wash.WallLux);
		(*Found)->FollowChannel(0.5f);
		TestTrue(FString::Printf(TEXT("%s dims by its profile's level, to half the file's flux"), Washer.Id), Type && Lamp->IESBrightnessScale == 0.5f
			&& FMath::IsNearlyEqual((*Found)->EmittedFlux().Get(), 0.5 * Type->Flux.Get(), 1e-9));
	}
	World->DestroyWorld(false);
	IFileManager::Get().DeleteDirectory(*FPaths::GetPath(Folder), false, true);
	return true;
}

#endif
