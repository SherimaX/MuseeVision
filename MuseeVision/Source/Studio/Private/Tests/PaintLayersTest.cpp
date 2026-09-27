#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceConstant.h"
#include "MatterNames.h"
#include "Misc/Paths.h"
#include "Stages/PaintLayers.h"
#include "Stages/StageMake.h"

/**
 * Each hung work's paint layer is made before the stage is cast: its image as an sRGB texture under Art, its paint
 * instance over the paint's parent carrying that texture, and nothing rewritten by a second make.
 */
namespace Musee::Studio::Tests
{
	namespace
	{
		/** The specimens are made under their own root, so they never touch the museum's assets. */
		const TCHAR* const PaintRoot = TEXT("/Game/Generated/Specimens");

		/** The engine's stock opaque surface, standing for the paint's palette instance. */
		const TCHAR* const StockParent = TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial");

		/** Art's framed test stage: a framed canvas and a work on two canvases, on a flat wall. */
		FMuseeStageContext FramedStage()
		{
			FMuseeStageContext Stage;
			Stage.Stage = FMuseeKey(TEXT("framed"));
			Stage.ScheduleDir = FPaths::ConvertRelativePathToFull(FPaths::GameSourceDir() / TEXT("Systems/Art/MuseeArtMaking/Private/Tests/Data/Stages/framed"));
			FMuseeStageAnchor& Wall = Stage.Anchors.AddDefaulted_GetRef();
			Wall.Key = FMuseeKey(TEXT("oval.wall.flat"));
			Wall.Kind = TEXT("Wall");
			Wall.Frame = FMuseeFrame(FVector(0.0, -5.0, 0.0));
			return Stage;
		}

		int32 Count(const FMakeReport& Report, EMadeOutcome Outcome)
		{
			return Report.Packages.FilterByPredicate([Outcome](const FMadePackage& Made) { return Made.Outcome == Outcome; }).Num();
		}
	}

	IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStudioPaintLayersTest, "Musee.Studio.PaintLayers", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
	bool FStudioPaintLayersTest::RunTest(const FString& Parameters)
	{
		FStageMakeOptions Options;
		Options.Paths = FMakePaths{PaintRoot};
		Options.Resolve = [](FName, FName, FName) { return LoadObject<UMaterialInterface>(nullptr, StockParent); };
		FMakeReport First;
		MakePaintLayers(FramedStage(), Options, First);
		for (const FMadePackage& Made : First.Packages)
		{
			TestTrue(FString::Printf(TEXT("%s is made (%s)"), *Made.PackageName, *Made.Why), Made.Outcome != EMadeOutcome::Failed);
		}
		TestEqual(TEXT("two works: two images and two paint instances, and nothing found wrong"), First.Packages.Num() * 10 + First.Findings.Num(), 40);

		const FString Image = Options.Paths.WorkImage(TEXT("MonetWillowsTwoWillows"));
		const UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Musee::Matter::ObjectPathOf(Image));
		TestTrue(TEXT("the image is an sRGB texture under Art"), Texture && Texture->SRGB);
		const FString Paint = Options.Paths.MatterInstance(Musee::Matter::InstanceName(TEXT("paint.monet-willows-two-willows"), TEXT("stretched"), TEXT("clean")));
		const UMaterialInstanceConstant* Instance = LoadObject<UMaterialInstanceConstant>(nullptr, *Musee::Matter::ObjectPathOf(Paint));
		if (TestNotNull(TEXT("the work's paint instance is made"), Instance))
		{
			TestTrue(TEXT("over the paint's parent"), Instance->Parent.Get() == LoadObject<UMaterialInterface>(nullptr, StockParent));
			TestTrue(TEXT("carrying the work's image"), Instance->TextureParameterValues.ContainsByPredicate([Texture](const FTextureParameterValue& Value)
			{
				return Value.ParameterValue == Texture;
			}));
			TestTrue(TEXT("and the image's size across its two canvases"), Instance->VectorParameterValues.ContainsByPredicate([](const FVectorParameterValue& Value)
			{
				return FMath::IsNearlyEqual(Value.ParameterValue.R, 17.0f) && FMath::IsNearlyEqual(Value.ParameterValue.G, 2.0f);
			}));
		}

		FMakeReport Second;
		MakePaintLayers(FramedStage(), Options, Second);
		TestEqual(TEXT("a second make rewrites nothing"), Count(Second, EMadeOutcome::Unchanged), 4);
		return true;
	}
}

#endif
