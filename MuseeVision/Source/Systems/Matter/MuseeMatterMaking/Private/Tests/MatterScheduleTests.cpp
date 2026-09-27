#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "Internationalization/Regex.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialParameterCollection.h"
#include "MatterInstances.h"
#include "MatterMake.h"
#include "MatterMasterParameters.h"
#include "MatterMasterParts.h"
#include "MatterMasters.h"
#include "MatterPaintLayer.h"
#include "MatterRecords.h"
#include "Misc/FileHelper.h"
#include "MuseeRecords.h"
#include "Tests/MatterTestMasters.h"

namespace Musee::Matter::ScheduleTests
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	/** The stages' schedules, under Data. */
	constexpr const TCHAR* StagesFolder = TEXT("Stages");

	/** A frame that is no frame (Hang.json). */
	constexpr const TCHAR* NoFrame = TEXT("none");

	/**
	 * The material specs a schedule other than the finish schedule names, found in its text: water, beds and cast
	 * specimens (Planting.json) give "material", "working" and "history" together in one object. Each field's value is a
	 * string, and nothing between them opens or closes an object. Matter reads other systems' schedules only here, to
	 * check its own keys, so it scans their text rather than reading their records.
	 */
	const TCHAR* const SpecPattern = TEXT("\"material\"\\s*:\\s*\"([^\"]*)\"[^{}]*?\"working\"\\s*:\\s*\"([^\"]*)\"[^{}]*?\"history\"\\s*:\\s*\"([^\"]*)\"");
	const TCHAR* const MaterialPattern = TEXT("\"material\"\\s*:");
	const TCHAR* const GlazingPattern = TEXT("\"glazing\"\\s*:\\s*\"([^\"]*)\"");
	const TCHAR* const FramePattern = TEXT("\"frame\"\\s*:\\s*\"([^\"]*)\"");

	/** One place a schedule names a material, and the spec it names, as written. */
	struct FNamedSpec
	{
		FString Where;
		FString Palette;
		FString Working;
		FString History;
	};

	TArray<FString> StageKeys()
	{
		TArray<FString> Stages;
		IFileManager::Get().FindFiles(Stages, *(Records::DataRoot() / StagesFolder / TEXT("*")), false, true);
		Stages.Sort();
		return Stages;
	}

	FString ScheduleText(const FString& Stage, const TCHAR* File)
	{
		FString Text;
		FFileHelper::LoadFileToString(Text, *(Records::DataRoot() / StagesFolder / Stage / File));
		return Text;
	}

	/** Every capture of Pattern's first group in Text. */
	TArray<FString> Captures(const TCHAR* Pattern, const FString& Text)
	{
		TArray<FString> Found;
		FRegexMatcher Matcher(FRegexPattern(Pattern), Text);
		while (Matcher.FindNext())
		{
			Found.Add(Matcher.GetCaptureGroup(1));
		}
		return Found;
	}

	/** The specs a planting schedule names; a "material" without its working and history is reported on Test. */
	TArray<FNamedSpec> PlantingSpecs(FAutomationTestBase& Test, const FString& Stage)
	{
		const FString Text = ScheduleText(Stage, TEXT("Planting.json"));
		TArray<FNamedSpec> Specs;
		FRegexMatcher Matcher(FRegexPattern(SpecPattern), Text);
		while (Matcher.FindNext())
		{
			Specs.Add({ FString::Printf(TEXT("%s/Planting.json"), *Stage), Matcher.GetCaptureGroup(1), Matcher.GetCaptureGroup(2), Matcher.GetCaptureGroup(3) });
		}
		int32 Materials = 0;
		FRegexMatcher Named(FRegexPattern(MaterialPattern), Text);
		while (Named.FindNext())
		{
			++Materials;
		}
		Test.TestEqual(FString::Printf(TEXT("%s/Planting.json: every material is named with its working and history"), *Stage), Specs.Num(), Materials);
		return Specs;
	}

	/** Every spec a stage's schedules name: its finishes, its water and beds, its cast specimens. */
	TArray<FNamedSpec> StageSpecs(FAutomationTestBase& Test, const FString& Stage, const FMatterFinishSchedule* Finishes)
	{
		TArray<FNamedSpec> Specs;
		for (const FMatterFinish& Finish : Finishes ? Finishes->Finishes : TArray<FMatterFinish>())
		{
			Specs.Add({ FString::Printf(TEXT("%s/Finishes.json %s"), *Stage, *Finish.Surface.ToString()), Finish.Material.ToString(),
				Finish.Working.ToString(), Finish.History.ToString() });
		}
		Specs.Append(PlantingSpecs(Test, Stage));
		return Specs;
	}

	/** Why a named spec does not resolve, or an empty string: its keys parse, the palette offers it as a finish (so
	 *  MuseeMatterMake makes its instance), its records read, and its instance composes over its master. */
	FString Unresolved(const FNamedSpec& Named, const TArray<FMatterSpec>& Offered, const TArray<FMadeMaster>& Masters)
	{
		const TOptional<FMuseeKey> Palette = FMuseeKey::Parse(Named.Palette);
		const TOptional<FMuseeKey> Working = FMuseeKey::Parse(Named.Working);
		const TOptional<FMuseeKey> History = FMuseeKey::Parse(Named.History);
		if (!Palette || !Working || !History)
		{
			return TEXT("a material, working or history is not a key");
		}
		const FMatterSpec Spec{ *Palette, *Working, *History };
		if (!Offered.ContainsByPredicate([&Spec](const FMatterSpec& O) { return O.ToString() == Spec.ToString(); }))
		{
			return TEXT("the palette offers no such finish, so no instance is made");
		}
		UMaterialInstanceConstant* Instance = TestMasters::TransientInstance();
		const FInstanceComposed Composed = ComposeInstance(Spec, *Instance, [&Masters](const FMasterKind& Kind) { return TestMasters::FindIn(Masters, Kind); });
		Instance->MarkAsGarbage();
		return FString::Join(Composed.Problems, TEXT(" "));
	}

	/** A glazing names a palette entry that offers finishes; a frame names a surface of the stage's finish schedule. */
	void CheckGlazingAndFrames(FAutomationTestBase& Test, const FString& Stage, const FMatterPalette& Palette, const FMatterFinishSchedule* Finishes)
	{
		for (const FString& Glazing : Captures(GlazingPattern, ScheduleText(Stage, TEXT("Lighting.json"))))
		{
			const TOptional<FMuseeKey> Key = FMuseeKey::Parse(Glazing);
			const FMatterPaletteEntry* Entry = Key ? Palette.Find(*Key) : nullptr;
			Test.TestTrue(FString::Printf(TEXT("%s/Lighting.json: the glazing %s is a palette entry with finishes"), *Stage, *Glazing),
				Entry && !Entry->Finishes.IsEmpty());
		}
		for (const FString& Frame : Captures(FramePattern, ScheduleText(Stage, TEXT("Hang.json"))))
		{
			Test.TestTrue(FString::Printf(TEXT("%s/Hang.json: the frame %s is none or a surface of the finish schedule"), *Stage, *Frame),
				Frame == NoFrame || (Finishes && Finishes->Find(FName(*Frame))));
		}
	}
}

using namespace Musee::Matter;
using namespace Musee::Matter::ScheduleTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatterEveryScheduleResolves, "Musee.Matter.EveryScheduleResolves", TestFlags)
bool FMatterEveryScheduleResolves::RunTest(const FString& Parameters)
{
	const TValueOrError<FMatterPalette, FString> Palette = ReadPalette();
	TValueOrError<TArray<FMatterSpec>, FString> Offered = PaletteSpecs();
	if (!TestTrue(TEXT("The palette reads"), Palette.HasValue() && Offered.HasValue()))
	{
		AddError(Palette.HasError() ? Palette.GetError() : Offered.GetError());
		return false;
	}
	UMaterialParameterCollection* Moment = TestMasters::TransientMoment();
	const TArray<FMadeMaster> Masters = TestMasters::TransientMasters(*this, *Moment);
	const TArray<FString> Stages = StageKeys();
	TestTrue(TEXT("There are stages to check"), !Stages.IsEmpty());
	int32 Checked = 0;
	for (const FString& Stage : Stages)
	{
		const TValueOrError<FMatterFinishSchedule, FString> Finishes = ReadFinishSchedule(FString(StagesFolder) / Stage);
		if (Finishes.HasError())
		{
			AddError(FString::Printf(TEXT("%s: %s"), *Stage, *Finishes.GetError()));
		}
		const FMatterFinishSchedule* Schedule = Finishes.HasValue() ? &Finishes.GetValue() : nullptr;
		for (const FNamedSpec& Named : StageSpecs(*this, Stage, Schedule))
		{
			const FString Why = Unresolved(Named, Offered.GetValue(), Masters);
			if (!Why.IsEmpty())
			{
				AddError(FString::Printf(TEXT("%s: %s/%s/%s does not resolve: %s"), *Named.Where, *Named.Palette, *Named.Working, *Named.History, *Why));
			}
			++Checked;
		}
		CheckGlazingAndFrames(*this, Stage, Palette.GetValue(), Schedule);
	}
	TestTrue(TEXT("The schedules name materials"), Checked > 0);
	AddInfo(FString::Printf(TEXT("%d material specs checked across %d stages."), Checked, Stages.Num()));
	TestMasters::Retire(Masters);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatterPaintLayerComposes, "Musee.Matter.PaintLayer", TestFlags)
bool FMatterPaintLayerComposes::RunTest(const FString& Parameters)
{
	UMaterialParameterCollection* Moment = TestMasters::TransientMoment();
	const TArray<FMadeMaster> Made = TestMasters::TransientMasters(*this, *Moment);
	UMaterialInterface* Paint = TestMasters::FindIn(Made, FMasterKind{ FName(Masters::Paint), NAME_None });
	UTexture* Image = LoadObject<UTexture>(nullptr, MasterParts::EngineDefaultTexture);
	if (!TestNotNull(TEXT("The paint master builds"), Paint) || !TestNotNull(TEXT("The engine's default texture loads"), Image))
	{
		return false;
	}
	FMatterPaintLayer Layer;
	Layer.Work = TEXT("paint.matter-test");
	Layer.Image = Image;
	Layer.WidthMeters = 12.75;
	Layer.HeightMeters = 2.0;

	UMaterialInstanceConstant* Instance = TestMasters::TransientInstance();
	for (const FString& Problem : ComposePaintLayer(Layer, *Paint, *Instance))
	{
		AddError(Problem);
	}
	TestTrue(TEXT("The layer is parented to the paint master"), Instance->Parent == Paint);
	float Width = 0.0f;
	float Height = 0.0f;
	UTexture* Carried = nullptr;
	Instance->GetScalarParameterValue(FHashedMaterialParameterInfo(MasterParameter::ImageWidthMeters), Width);
	Instance->GetScalarParameterValue(FHashedMaterialParameterInfo(MasterParameter::ImageHeightMeters), Height);
	Instance->GetTextureParameterValue(FHashedMaterialParameterInfo(MasterParameter::Image), Carried);
	TestEqual(TEXT("It carries the work's width"), Width, 12.75f);
	TestEqual(TEXT("It carries the work's height"), Height, 2.0f);
	TestTrue(TEXT("It carries the work's image"), Carried == Image);

	UMaterialInstanceConstant* Again = TestMasters::TransientInstance();
	TestTrue(TEXT("The same work composes again"), ComposePaintLayer(Layer, *Paint, *Again).IsEmpty());
	TestEqual(TEXT("The same work has the same identity"), Again->ParameterStateId, Instance->ParameterStateId);

	FMatterPaintLayer Wider = Layer;
	Wider.WidthMeters = 6.0;
	UMaterialInstanceConstant* Other = TestMasters::TransientInstance();
	TestTrue(TEXT("Another size composes"), ComposePaintLayer(Wider, *Paint, *Other).IsEmpty());
	TestNotEqual(TEXT("Another size is another identity"), Other->ParameterStateId, Instance->ParameterStateId);

	FMatterPaintLayer Blank = Layer;
	Blank.Image = nullptr;
	UMaterialInstanceConstant* Refused = TestMasters::TransientInstance();
	TestFalse(TEXT("A work with no image is refused"), ComposePaintLayer(Blank, *Paint, *Refused).IsEmpty());
	for (UMaterialInstanceConstant* Composed : { Instance, Again, Other, Refused })
	{
		Composed->MarkAsGarbage();
	}
	TestMasters::Retire(Made);
	return true;
}

#endif
