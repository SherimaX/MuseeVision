#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "ArtWork.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "LightImage.h"
#include "LightLuminaireComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionSubstrate.h"
#include "Make/PackageSave.h"
#include "Misc/FileHelper.h"
#include "UObject/Package.h"
#include "Stages/LightImageMake.h"
#include "Stages/StageMake.h"
#include "TimeMoment.h"

// Light's image, as Studio makes it: the star field drawn from the collection's catalogue, its light conserved, and the
// material that shows it; and, placed as the Sphere's schedule places its image, a surface of the sphere that shows the
// sky and gives its light.
namespace Musee::Studio::LightImageTests
{
	inline constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;

	/** Where the test makes the image apart from the museum's. */
	constexpr const TCHAR* SpecimenRoot = TEXT("/Game/Generated/Specimens");

	/** The Sphere's image (sphere.md, section 4): R 13.700, less the neck's opening r 2.980. */
	constexpr double SphereRadius = 13.7;
	constexpr double SphereOpening = 2.98;

	/** Rec. 709's luminance weights. */
	double LuminanceOf(const FLinearColor& Colour)
	{
		return 0.2126 * Colour.R + 0.7152 * Colour.G + 0.0722 * Colour.B;
	}

	int32 CountOf(const FMakeReport& Report, EMadeOutcome Outcome)
	{
		return Report.Packages.FilterByPredicate([Outcome](const FMadePackage& Made) { return Made.Outcome == Outcome; }).Num();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioLightImageTest, "Musee.Studio.LightImage", Musee::Studio::LightImageTests::Flags)

bool FStudioLightImageTest::RunTest(const FString& Parameters)
{
	using namespace Musee::Studio;
	using namespace Musee::Studio::LightImageTests;
	namespace Image = Musee::Light::Image;

	// The star field, drawn from the collection's catalogue.
	TArray<uint8> Bytes;
	if (!TestTrue(TEXT("The star catalogue reads"), FFileHelper::LoadFileToArray(Bytes, *Musee::Art::SourcePath(Image::CatalogueSource))))
	{
		return false;
	}
	const TValueOrError<Image::FStarField, FString> Field = Image::DrawStarField(Bytes, Image::StarFieldWidth, Image::StarFieldHeight);
	if (!TestTrue(TEXT("and draws"), Field.HasValue()))
	{
		AddError(Field.GetError());
		return false;
	}
	TestEqual(TEXT("Its 8,404 stars to V 6.5 are drawn"), Field.GetValue().Stars, 8404);
	TestTrue(*FString::Printf(TEXT("its mean luminance the one Light keeps (%.6g cd/m2)"), Field.GetValue().MeanLuminance),
		FMath::IsNearlyEqual(Field.GetValue().MeanLuminance, Image::StarFieldMeanLuminance, 1e-3 * Image::StarFieldMeanLuminance));
	double Integrated = 0.0;
	const double Texel = (2.0 * UE_DOUBLE_PI / Image::StarFieldWidth) * (UE_DOUBLE_PI / Image::StarFieldHeight);
	for (int32 Row = 0; Row < Image::StarFieldHeight; ++Row)
	{
		const double Steradians = Texel * FMath::Sin(UE_DOUBLE_PI * (Row + 0.5) / Image::StarFieldHeight);
		for (int32 Column = 0; Column < Image::StarFieldWidth; ++Column)
		{
			Integrated += LuminanceOf(Field.GetValue().Luminance[Row * Image::StarFieldWidth + Column]) * Steradians;
		}
	}
	TestTrue(*FString::Printf(TEXT("and its luminance over the sky gives back the stars' illuminance (%.6g lx)"), Integrated),
		FMath::IsNearlyEqual(Integrated, Field.GetValue().MeanLuminance * 4.0 * UE_DOUBLE_PI, 1e-3 * Integrated));

	// Made apart: the field and the material; a second make writes nothing.
	FStageMakeOptions Specimen;
	Specimen.Paths = FMakePaths{SpecimenRoot};
	FMakeReport First;
	MakeLightImage(Specimen, First);
	TestTrue(TEXT("The image is made"), First.Succeeded() && CountOf(First, EMadeOutcome::Failed) == 0 && First.Packages.Num() == 2);
	FMakeReport Again;
	MakeLightImage(Specimen, Again);
	TestEqual(TEXT("A second make writes nothing"), CountOf(Again, EMadeOutcome::Written), 0);
	// Nor does one in a fresh process, which finds the image on disk, not in memory (as every stage's make does).
	ReleasePackages({FindPackage(nullptr, *Specimen.Paths.LightImage(Image::MaterialName)), FindPackage(nullptr, *Specimen.Paths.LightImage(Image::StarFieldName))});
	FMakeReport FromDisk;
	MakeLightImage(Specimen, FromDisk);
	TestTrue(TEXT("nor one that finds it on disk"), CountOf(FromDisk, EMadeOutcome::Written) == 0 && CountOf(FromDisk, EMadeOutcome::Unchanged) == 2);
	const FString FieldPath = Specimen.Paths.LightImage(Image::StarFieldName) + TEXT(".") + Image::StarFieldName;
	const UTexture2D* Stars = LoadObject<UTexture2D>(nullptr, *FieldPath);
	if (TestNotNull(TEXT("The star field loads"), Stars))
	{
		TestTrue(TEXT("as half-float luminances over the whole sky, unmipped"), Stars->Source.GetSizeX() == Image::StarFieldWidth
			&& Stars->Source.GetSizeY() == Image::StarFieldHeight && Stars->Source.GetFormat() == TSF_RGBA16F && Stars->CompressionSettings == TC_HDR
			&& Stars->MipGenSettings == TMGS_NoMipmaps && !Stars->SRGB);
	}
	const FString MaterialPath = Specimen.Paths.LightImage(Image::MaterialName) + TEXT(".") + Image::MaterialName;
	const UMaterial* Material = LoadObject<UMaterial>(nullptr, *MaterialPath);
	if (TestNotNull(TEXT("The image material loads"), Material))
	{
		const UMaterialExpressionCustom* const* Emission = reinterpret_cast<const UMaterialExpressionCustom* const*>(
			Material->GetExpressionCollection().Expressions.FindByPredicate([](const UMaterialExpression* Each) { return Each && Each->IsA<UMaterialExpressionCustom>(); }));
		TestTrue(TEXT("its emission Light's"), Emission && (*Emission)->Code == Image::EmissionHlsl());
		TestTrue(TEXT("and it is two-sided and unlit"), Material->TwoSided && Material->GetExpressionCollection().Expressions.ContainsByPredicate(
			[](const UMaterialExpression* Each) { return Each && Each->IsA<UMaterialExpressionSubstrateUnlitBSDF>(); }));
	}

	// The museum's, where Light finds it; then the Sphere's image placed over its sphere.
	FMakeReport Museum;
	MakeLightImage(FStageMakeOptions(), Museum);
	if (!TestTrue(TEXT("The museum's image is made"), Museum.Succeeded() && Image::FindMaterial() != nullptr))
	{
		return false;
	}
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	AActor* Stage = World->SpawnActor<AActor>();
	USceneComponent* Root = NewObject<USceneComponent>(Stage, TEXT("Root"));
	Stage->SetRootComponent(Root);
	Root->RegisterComponent();
	ULightLuminaireComponent* Sky = NewObject<ULightLuminaireComponent>(Stage, TEXT("Image"));
	const double Equator = 2.0 * UE_DOUBLE_PI * SphereRadius;
	const double Area = Equator * (SphereRadius + FMath::Sqrt(SphereRadius * SphereRadius - SphereOpening * SphereOpening));
	Sky->Configure(FMuseeKey(TEXTVIEW("light-field-panel.d65")), ELightOperation::Always, FMuseeKey(), Musee::Meters(0.0),
		FVector2D(Equator, Area / Equator), FVector2D(SphereRadius, SphereOpening));
	Sky->SetupAttachment(Root);
	Sky->RegisterComponent();
	TestTrue(TEXT("The Sphere's image drives no engine light"), Sky->Emitters().IsEmpty());
	const UPrimitiveComponent* Surface = Sky->ImageSurface();
	if (TestNotNull(TEXT("and shows its picture on a surface of its sphere"), Surface))
	{
		const FBoxSphereBounds Bounds = Surface->CalcBounds(Surface->GetComponentTransform());
		TestTrue(*FString::Printf(TEXT("the sphere's inside, 13.7 m about its centre, down to its opening (%s)"), *Bounds.BoxExtent.ToString()),
			FMath::IsNearlyEqual(Bounds.BoxExtent.X, 1370.0, 1.0) && FMath::IsNearlyEqual(Bounds.GetBox().Max.Z, 1370.0, 1.0)
			&& FMath::IsNearlyEqual(Bounds.GetBox().Min.Z, -FMath::Sqrt(1370.0 * 1370.0 - 298.0 * 298.0), 1.0));
		TestTrue(TEXT("in Light's image material"), Surface->GetMaterial(0) && Surface->GetMaterial(0)->GetMaterial() == Image::FindMaterial()->GetMaterial());
	}
	// A night of full Moon: the sky's picture is the stars and the Moon, clipped at the panel's peak white.
	FTimeMoment Night;
	Night.MoonDirection = FVector(0.0, -0.6, 0.8);
	Night.MoonIlluminatedFraction = 1.0;
	Night.MoonPhaseAngleDegrees = 0.0;
	Night.MoonDistanceKilometers = 384400.0;
	Sky->FollowMoment(Night);
	const Image::FSkyPicture Picture = Image::SkyPicture(Night, Sky->GetComponentTransform(), 200.0);
	TestTrue(*FString::Printf(TEXT("It gives pi L A of its picture's mean luminance (%.4g lm)"), Sky->EmittedFlux().Get()),
		Sky->EmittedFlux().Get() > 0.0 && FMath::IsNearlyEqual(Sky->EmittedFlux().Get(), UE_DOUBLE_PI * Picture.MeanLuminance * Area, 1e-6 * Sky->EmittedFlux().Get()));
	World->DestroyWorld(false);
	return true;
}

#endif
