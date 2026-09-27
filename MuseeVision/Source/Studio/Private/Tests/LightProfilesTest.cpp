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
#include "Serialization/MemoryReader.h"
#include "Stages/LightProfiles.h"
#include "Stages/StageMake.h"
#include "UObject/Package.h"
#include "UObject/PackageFileSummary.h"

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

	/** Blanks every copy of Bytes' pattern in Data. */
	void Blank(TArray<uint8>& Data, TConstArrayView<uint8> Pattern)
	{
		for (int32 At = 0; Pattern.Num() > 0 && At + Pattern.Num() <= Data.Num(); ++At)
		{
			if (FMemory::Memcmp(&Data[At], Pattern.GetData(), Pattern.Num()) == 0) { FMemory::Memzero(&Data[At], Pattern.Num()); }
		}
	}

	/**
	 * A made package's bytes with its texture source's payload identity (the FGuid the engine gives a new bulk payload)
	 * and the package's saved hash (FPackageFileSummary, over all its bytes) blanked: what makes one make of the same
	 * inputs differ from another.
	 */
	TArray<uint8> Masked(const TArray<uint8>& Package, const FGuid& PayloadId)
	{
		TArray<uint8> Data = Package;
		const uint32 Words[4] = {PayloadId.A, PayloadId.B, PayloadId.C, PayloadId.D};
		Blank(Data, TConstArrayView<uint8>(reinterpret_cast<const uint8*>(Words), sizeof(Words)));
		FMemoryReader Reader(Package);
		FPackageFileSummary Summary;
		Reader << Summary;
		const FIoHash Saved = Summary.GetSavedHash();
		Blank(Data, TConstArrayView<uint8>(Saved.GetBytes(), sizeof(FIoHash::ByteArray)));
		return Data;
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

		/**
		 * Calls Visit with every direction of a grid over the sphere about the light (Polar by Azimuth cells, about its x
		 * axis) and the solid angle of its cell, in steradians.
		 */
		void ForEachDirection(int32 Polar, int32 Azimuth, TFunctionRef<void(const FVector&, double)> Visit) const
		{
			const FVector X = Light.GetUnitAxis(EAxis::X);
			const FVector Y = Light.GetUnitAxis(EAxis::Y);
			const FVector Z = Light.GetUnitAxis(EAxis::Z);
			const double Cell = (UE_DOUBLE_PI / Polar) * (2.0 * UE_DOUBLE_PI / Azimuth);
			for (int32 I = 0; I < Polar; ++I)
			{
				const double Theta = (I + 0.5) * UE_DOUBLE_PI / Polar;
				for (int32 J = 0; J < Azimuth; ++J)
				{
					const double Phi = (J + 0.5) * 2.0 * UE_DOUBLE_PI / Azimuth;
					Visit(X * FMath::Cos(Theta) + (Y * FMath::Cos(Phi) + Z * FMath::Sin(Phi)) * FMath::Sin(Theta), FMath::Sin(Theta) * Cell);
				}
			}
		}

		/** The flux the placed light gives, in lumens: its intensity integrated over the sphere about it. */
		double FluxLumens() const
		{
			double Lumens = 0.0;
			ForEachDirection(720, 720, [&](const FVector& Direction, double SolidAngle) { Lumens += CandelaTowards(Direction) * SolidAngle; });
			return Lumens;
		}

		/** The direction the placed light is brightest in, found on a grid of a quarter of a degree. */
		FVector PeakDirection() const
		{
			FVector Peak = Light.GetUnitAxis(EAxis::X);
			double Greatest = -1.0;
			ForEachDirection(720, 1440, [&](const FVector& Direction, double)
			{
				const double Intensity = CandelaTowards(Direction);
				if (Intensity > Greatest) { Greatest = Intensity; Peak = Direction; }
			});
			return Peak;
		}
	};

	/**
	 * A washer as a stage's schedule places it, in stage metres: its seat, its aim on the wall it washes, and that wall's
	 * top above the floor (z = 0) at the aim, taken from the stage's drawing.
	 */
	struct FWasher
	{
		const TCHAR* Id;
		FVector Seat;
		FVector Aim;
		double WallTop;
	};

	/** The wash a placed wallwasher lays: on the wall it is aimed at, on the floor at that wall's foot, and the other way. */
	struct FWash
	{
		double WallLux = 0.0;
		double FloorLux = 0.0;
		double OppositeLux = 0.0;
		/** The greatest and least illuminance up the wall's plumb line through the aim, from 0.15 m to its top. */
		double WallBrightestLux = 0.0;
		double WallDimmestLux = 0.0;
	};

	/** Where the wall's plumb line is read from: the height of a skirting, below which the floor's own light dominates. */
	constexpr double PlumbLineFromMeters = 0.15;

	/** The wall's plane: through the aim, square to the plan's line from the seat to it. */
	FVector TowardsWallOf(const FWasher& Washer)
	{
		return FVector(Washer.Aim.X - Washer.Seat.X, Washer.Aim.Y - Washer.Seat.Y, 0.0).GetSafeNormal();
	}

	/**
	 * Measures the wash of a washer over a floor at z = 0: the mean illuminance on the wall's face up the plumb line through
	 * the aim, from the floor to the wall's top, with its brightest and dimmest; on the floor at the wall's foot; and on a
	 * wall as far the other way, facing it.
	 */
	FWash MeasureWash(const FPlacedProfile& Placed, const FWasher& Washer)
	{
		const FVector TowardsWall = TowardsWallOf(Washer);
		const FVector Across = FVector(Washer.Aim.X - Washer.Seat.X, Washer.Aim.Y - Washer.Seat.Y, 0.0);
		const FVector Opposite = FVector(Washer.Seat.X, Washer.Seat.Y, 0.0) - Across;
		constexpr int32 Steps = 64;
		FWash Wash;
		Wash.WallDimmestLux = TNumericLimits<double>::Max();
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			const double Height = (Step + 0.5) * Washer.WallTop / Steps;
			const double Lux = Placed.LuxAt(FVector(Washer.Aim.X, Washer.Aim.Y, Height), -TowardsWall);
			Wash.WallLux += Lux / Steps;
			Wash.OppositeLux += Placed.LuxAt(FVector(Opposite.X, Opposite.Y, Height), TowardsWall) / Steps;
			if (Height >= PlumbLineFromMeters)
			{
				Wash.WallBrightestLux = FMath::Max(Wash.WallBrightestLux, Lux);
				Wash.WallDimmestLux = FMath::Min(Wash.WallDimmestLux, Lux);
			}
		}
		Wash.FloorLux = Placed.LuxAt(FVector(Washer.Aim.X, Washer.Aim.Y, 0.0), FVector::UpVector);
		return Wash;
	}

	/** Where a ray from the washer's seat along Direction meets the wall's plane: its height there, or unset if it runs away from the wall. */
	TOptional<double> HeightOnWall(const FWasher& Washer, const FVector& Direction)
	{
		const FVector TowardsWall = TowardsWallOf(Washer);
		const double Closing = Direction | TowardsWall;
		if (Closing <= 0.0) { return {}; }
		const double Distance = (Washer.Aim - Washer.Seat) | TowardsWall;
		return Washer.Seat.Z + Direction.Z * Distance / Closing;
	}

	/** How a placed washer's flux divides: onto the wall below its top, past the wall's top, and onto the floor before the wall. */
	struct FShares
	{
		double Wall = 0.0;
		double AboveWall = 0.0;
		double FloorFirst = 0.0;
	};

	/**
	 * The shares of the placed washer's flux that meet the wall's plane between the floor and the wall's top, above the top
	 * (onto the ceiling or the roof, past the wall), and the floor before the wall. The wall is taken as long as the room
	 * along it, so a share runs a little high for a short wall.
	 */
	FShares SharesOf(const FPlacedProfile& Placed, const FWasher& Washer)
	{
		FShares Lumens;
		double Total = 0.0;
		Placed.ForEachDirection(360, 720, [&](const FVector& Direction, double SolidAngle)
		{
			const double Flux = Placed.CandelaTowards(Direction) * SolidAngle;
			Total += Flux;
			const TOptional<double> Height = HeightOnWall(Washer, Direction);
			if (!Height.IsSet()) { return; }
			if (Height.GetValue() < 0.0) { Lumens.FloorFirst += Flux; }
			else if (Height.GetValue() <= Washer.WallTop) { Lumens.Wall += Flux; }
			else { Lumens.AboveWall += Flux; }
		});
		return Total > 0.0 ? FShares{Lumens.Wall / Total, Lumens.AboveWall / Total, Lumens.FloorFirst / Total} : FShares();
	}

	/**
	 * What a wallwasher is for (Docs/Light/Luminaires.md: an even wash down a whole wall): most of its light, over half its
	 * flux, on the wall below the wall's top, and at most a tenth past the top, onto a ceiling or roof it isn't meant for.
	 */
	constexpr double LeastWallShare = 0.5;
	constexpr double MostShareAboveWall = 0.1;

	/**
	 * How close the placed light's brightest direction lies to its aim, in degrees: the profile's texels are 0.7° apart
	 * in the vertical angle and 1.4° in the horizontal (256 over 180° and 360°), so the brightest texel lies within half
	 * a texel diagonal, under 1°, of the file's peak.
	 */
	constexpr double ThrowOnAimDegrees = 1.0;
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

	// Made afresh from nothing, its package is the same file, byte for byte, but for the identity the engine gives every
	// newly made bulk payload (FEditorBulkData's random id, which the bulk-data registry keeps unique) and the package's
	// hash, which covers it.
	const FString Filename = FileOf(Package);
	const TArray<uint8> Before = FileBytes(Filename);
	const FGuid PayloadBefore = Profile->Source.GetPersistentId();
	const FGuid LightingBefore = Profile->GetLightingGuid();
	const FGuid SourceBefore = Profile->Source.GetId();
	ReleasePackages({FindPackage(nullptr, *Package)});
	IFileManager::Get().Delete(*Filename);
	FMakeReport Afresh;
	MakeLightProfiles(Options, Afresh);
	TestEqual(TEXT("Made afresh, it is written"), CountOf(Afresh, Package, EMadeOutcome::Written), 1);
	const UTextureLightProfile* Remade = LoadObject<UTextureLightProfile>(nullptr, *(Package + TEXT(".") + WallwasherProfile));
	if (!TestNotNull(TEXT("and loads"), Remade)) { return false; }
	TestTrue(TEXT("with the same texels and the same lighting GUID, derived from its inputs"),
		Remade->Source.GetId() == SourceBefore && Remade->GetLightingGuid() == LightingBefore);
	const TArray<uint8> After = FileBytes(Filename);
	TestTrue(FString::Printf(TEXT("its file the same, byte for byte but for its payload's identity (%lld bytes)"), static_cast<int64>(Before.Num())),
		!Before.IsEmpty() && Masked(Before, PayloadBefore) == Masked(After, Remade->Source.GetPersistentId()));
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

	// Washers as the stages' schedules place them, each with the top of the wall it washes:
	// - the Classical Hall's washer.000, 2.82 m from the tribune wall and aimed 1.83 m down it; the wall rises to the
	//   entablature's ledge at 3.70 (Docs/Stages/classical-hall.md, 3.1 and the luminaires);
	// - Chenghuai's wash.linchi.s.01, 4.44 m across the front row and 1.12 m down its south wall, and wash.shujuan.01,
	//   4.29 m across the rear row and 1.02 m down its north wall. Both rows' back walls stand under a sealed eave: the
	//   wall's inner face (v 4.80 and 4.70) meets the rafters' underside 0.25 in front of the back purlin, whose seat is
	//   3.154, on the last step's rise of 0.5, at 3.279 (Docs/Stages/chenghuai.md, 3.1: the derived heights and the halls).
	const FWasher Washers[] = {
		{TEXT("hall"), FVector(3.78, 0.0, 3.86), FVector(6.6, 0.0, 2.029), 3.70},
		{TEXT("linchi"), FVector(-19.004, -4.95, 3.02), FVector(-19.004, -0.515, 1.9), 3.279},
		{TEXT("shujuan"), FVector(-18.988, -40.10, 3.02), FVector(-18.988, -44.385, 2.0), 3.279},
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
	TestEqual(TEXT("Light places every washer lit, with nothing to find"),
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
		// Its light, not its housing, is aimed: the brightest direction of the profile as placed runs to the aim, and meets
		// the wall between the floor and the wall's top. Turned about its axis (a roll) or left on its nadir, it would not.
		const FVector Peak = Profile.PeakDirection();
		const FVector ToAim = (Washer.Aim - Washer.Seat).GetSafeNormal();
		const double OffAim = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Peak | ToAim, -1.0, 1.0)));
		const TOptional<double> PeakHeight = HeightOnWall(Washer, Peak);
		AddInfo(FString::Printf(TEXT("%s: its peak runs %.2f deg from its aim and meets the wall's plane at z %.3f (the wall's top %.3f)."),
			Washer.Id, OffAim, PeakHeight.Get(TNumericLimits<double>::Max()), Washer.WallTop));
		TestTrue(FString::Printf(TEXT("%s throws its peak on its aim, within %.1f deg (%.2f deg)"), Washer.Id, ThrowOnAimDegrees, OffAim),
			OffAim <= ThrowOnAimDegrees);
		TestTrue(FString::Printf(TEXT("%s: its peak meets the wall below the wall's top"), Washer.Id),
			PeakHeight.IsSet() && PeakHeight.GetValue() >= 0.0 && PeakHeight.GetValue() <= Washer.WallTop);
		const FShares Shares = SharesOf(Profile, Washer);
		AddInfo(FString::Printf(TEXT("%s: %.1f %% of its flux on the wall, %.1f %% past its top, %.1f %% on the floor first."), Washer.Id,
			100.0 * Shares.Wall, 100.0 * Shares.AboveWall, 100.0 * Shares.FloorFirst));
		TestTrue(FString::Printf(TEXT("%s lays most of its light on the wall below its top (%.1f %%)"), Washer.Id, 100.0 * Shares.Wall),
			Shares.Wall >= LeastWallShare);
		TestTrue(FString::Printf(TEXT("%s sends at most a tenth past the wall's top (%.1f %%)"), Washer.Id, 100.0 * Shares.AboveWall),
			Shares.AboveWall <= MostShareAboveWall);
		const FWash Wash = MeasureWash(Profile, Washer);
		AddInfo(FString::Printf(TEXT("%s: %.1f lx on the wall (%.1f to %.1f up its plumb line), %.1f lx on the floor at its foot, %.2f lx the other way."),
			Washer.Id, Wash.WallLux, Wash.WallDimmestLux, Wash.WallBrightestLux, Wash.FloorLux, Wash.OppositeLux));
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
