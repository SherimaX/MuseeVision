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
#include "MatterNames.h"
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
	 * The fields that name a material spec in a schedule other than the finish schedule: water, beds' ground, cast
	 * specimens (Planting.json) and the blind doors give "material", "working" and "history" together in one object, in
	 * any order. Matter reads other systems' schedules only here, to check its own keys, so it scans their text rather
	 * than reading their records, whose other fields are theirs.
	 */
	const TCHAR* const MaterialField = TEXT("material");
	const TCHAR* const WorkingField = TEXT("working");
	const TCHAR* const HistoryField = TEXT("history");
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

	/** One JSON object in a schedule's text: the keys it names itself, and those of them whose values are strings. */
	struct FTextObject
	{
		TMap<FString, FString> Fields;
		TSet<FString> Keys;
	};

	/**
	 * The string at Text[Start] (a quote), unescaped only as far as a key or a key's value needs; Next is left on the
	 * character after its closing quote.
	 */
	FString StringAt(const FString& Text, int32 Start, int32& Next)
	{
		FString Value;
		int32 Index = Start + 1;
		for (; Index < Text.Len() && Text[Index] != TEXT('"'); ++Index)
		{
			if (Text[Index] == TEXT('\\') && Index + 1 < Text.Len())
			{
				++Index;
			}
			Value.AppendChar(Text[Index]);
		}
		Next = Index + 1;
		return Value;
	}

	int32 SkipSpace(const FString& Text, int32 Index)
	{
		while (Index < Text.Len() && FChar::IsWhitespace(Text[Index]))
		{
			++Index;
		}
		return Index;
	}

	/** Every object in a schedule's text, innermost first, with the keys it names and its own string-valued fields. */
	TArray<FTextObject> TextObjects(const FString& Text)
	{
		TArray<FTextObject> Done;
		TArray<FTextObject> Open;
		for (int32 Index = 0; Index < Text.Len();)
		{
			const TCHAR Char = Text[Index];
			if (Char == TEXT('{'))
			{
				Open.AddDefaulted();
				++Index;
			}
			else if (Char == TEXT('}'))
			{
				if (!Open.IsEmpty())
				{
					Done.Add(Open.Pop());
				}
				++Index;
			}
			else if (Char == TEXT('"'))
			{
				int32 Next = Index;
				const FString Token = StringAt(Text, Index, Next);
				const int32 After = SkipSpace(Text, Next);
				Index = Next;
				if (After < Text.Len() && Text[After] == TEXT(':') && !Open.IsEmpty())
				{
					Open.Last().Keys.Add(Token);
					const int32 ValueAt = SkipSpace(Text, After + 1);
					if (ValueAt < Text.Len() && Text[ValueAt] == TEXT('"'))
					{
						Open.Last().Fields.Add(Token, StringAt(Text, ValueAt, Index));
					}
				}
			}
			else
			{
				++Index;
			}
		}
		return Done;
	}

	/**
	 * The specs a record's text names: every object that names a "material" gives its working and history beside it; one
	 * that doesn't is reported on Test.
	 */
	TArray<FNamedSpec> TextSpecs(FAutomationTestBase& Test, const FString& Where, const FString& Text)
	{
		TArray<FNamedSpec> Specs;
		for (const FTextObject& Object : TextObjects(Text))
		{
			if (!Object.Keys.Contains(MaterialField))
			{
				continue;
			}
			const FString* Material = Object.Fields.Find(MaterialField);
			const FString* Working = Object.Fields.Find(WorkingField);
			const FString* History = Object.Fields.Find(HistoryField);
			if (!Material || !Working || !History)
			{
				Test.AddError(FString::Printf(TEXT("%s: the material %s is not named with its working and history."), *Where,
					Material ? **Material : TEXT("(not a string)")));
				continue;
			}
			Specs.Add({ Where, *Material, *Working, *History });
		}
		return Specs;
	}

	/** The specs a planting schedule names: its water, beds and cast specimens. */
	TArray<FNamedSpec> PlantingSpecs(FAutomationTestBase& Test, const FString& Stage)
	{
		return TextSpecs(Test, FString::Printf(TEXT("%s/Planting.json"), *Stage), ScheduleText(Stage, TEXT("Planting.json")));
	}

	/** The Site's blind doors, which carry their own bronze rather than a stage's finish (Site/Blinds.json). */
	const TCHAR* const BlindsRecord = TEXT("Site/Blinds.json");

	TArray<FNamedSpec> BlindSpecs(FAutomationTestBase& Test)
	{
		FString Text;
		FFileHelper::LoadFileToString(Text, *(Records::DataRoot() / BlindsRecord));
		const TArray<FNamedSpec> Specs = TextSpecs(Test, BlindsRecord, Text);
		Test.TestEqual(TEXT("The blind doors name their bronze"), Specs.Num(), 1);
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
	const auto Check = [this, &Offered, &Masters, &Checked](const TArray<FNamedSpec>& Specs)
	{
		for (const FNamedSpec& Named : Specs)
		{
			const FString Why = Unresolved(Named, Offered.GetValue(), Masters);
			if (!Why.IsEmpty())
			{
				AddError(FString::Printf(TEXT("%s: %s/%s/%s does not resolve: %s"), *Named.Where, *Named.Palette, *Named.Working, *Named.History, *Why));
			}
			++Checked;
		}
	};
	for (const FString& Stage : Stages)
	{
		const TValueOrError<FMatterFinishSchedule, FString> Finishes = ReadFinishSchedule(FString(StagesFolder) / Stage);
		if (Finishes.HasError())
		{
			AddError(FString::Printf(TEXT("%s: %s"), *Stage, *Finishes.GetError()));
		}
		const FMatterFinishSchedule* Schedule = Finishes.HasValue() ? &Finishes.GetValue() : nullptr;
		Check(StageSpecs(*this, Stage, Schedule));
		CheckGlazingAndFrames(*this, Stage, Palette.GetValue(), Schedule);
	}
	Check(BlindSpecs(*this));
	TestTrue(TEXT("The schedules name materials"), Checked > 0);
	AddInfo(FString::Printf(TEXT("%d material specs checked across %d stages."), Checked, Stages.Num()));
	TestMasters::Retire(Masters);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatterWorkingLays, "Musee.Matter.WorkingLays", TestFlags)
bool FMatterWorkingLays::RunTest(const FString& Parameters)
{
	const auto Read = [this](const TCHAR* Palette, const TCHAR* Working, const TCHAR* History)
	{
		const FMatterSpec Spec{ FMuseeKey(Palette), FMuseeKey(Working), FMuseeKey(History) };
		TValueOrError<FSpecRecords, FString> Records = ReadSpec(Spec);
		if (Records.HasError())
		{
			AddError(FString::Printf(TEXT("%s: %s"), *Spec.ToString(), *Records.GetError()));
		}
		return Records;
	};
	const TValueOrError<FSpecRecords, FString> Painted = Read(TEXT("accent.qing-pine-lacquered"), TEXT("dizhang.tiehong"), TEXT("weathered"));
	if (Painted.HasValue())
	{
		TestEqual(TEXT("A painted pine shows its paint, not its pine"), Painted.GetValue().Substance.Key.ToString(), FString(TEXT("paint.tiehong")));
		TestTrue(TEXT("and the paint's master draws it"), Painted.GetValue().Master == (FMasterKind{ FName(Masters::White), NAME_None }));
		TestEqual(TEXT("with the paint master's history"), Painted.GetValue().History.Key.ToString(), FString(TEXT("white.weathered")));
	}
	const TValueOrError<FSpecRecords, FString> Lacquered = Read(TEXT("accent.chinese-fir-guangqi"), TEXT("guangqi.chestnut"), TEXT("weathered"));
	if (Lacquered.HasValue())
	{
		TestEqual(TEXT("A translucent lacquer lays no coat: the fir shows through"), Lacquered.GetValue().Substance.Key.ToString(), FString(TEXT("chinese-fir")));
	}
	const TValueOrError<FSpecRecords, FString> Gilt = Read(TEXT("oak.sessile-french"), TEXT("gilt.gold-leaf.oil"), TEXT("aged.patina"));
	if (Gilt.HasValue())
	{
		TestTrue(TEXT("Leaf oil-gilt on oak is carried by the wood master"), Gilt.GetValue().Master == (FMasterKind{ FName(Masters::Wood), FName(Masters::Gilt) }));
		TestTrue(TEXT("over the sealed, sized oak"), Gilt.GetValue().Host.IsSet() && Gilt.GetValue().Host.GetValue().Key.ToString() == TEXT("oil-sized"));
	}
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
	TestEqual(TEXT("The paint master is made where Studio finds it"), PaintMasterPackage(), MasterPackage(FName(Masters::Paint)));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatterStainedLayerComposes, "Musee.Matter.StainedLayer", TestFlags)
bool FMatterStainedLayerComposes::RunTest(const FString& Parameters)
{
	UMaterialParameterCollection* Moment = TestMasters::TransientMoment();
	const TArray<FMadeMaster> Made = TestMasters::TransientMasters(*this, *Moment);
	UMaterialInterface* Stained = TestMasters::FindIn(Made, FMasterKind{ FName(Masters::Stained), NAME_None });
	UTexture* Image = LoadObject<UTexture>(nullptr, MasterParts::EngineDefaultTexture);
	if (!TestNotNull(TEXT("The stained-glass master builds"), Stained) || !TestNotNull(TEXT("The engine's default texture loads"), Image))
	{
		return false;
	}
	// A panel of Albion's Tristram and Isolde band, fired in its antique glass (albion/Finishes.json albion.band.glass).
	FMatterPaintLayer Panel;
	Panel.Work = TEXT("paint.matter-test.1");
	Panel.Palette = TEXT("glass.mouth-blown.antique");
	Panel.Working = TEXT("leaded");
	Panel.History = TEXT("aged.patina");
	Panel.Image = Image;
	Panel.WidthMeters = 1.6;
	Panel.HeightMeters = 1.7;
	const TValueOrError<FString, FString> Master = LayerMasterPackage(Panel);
	TestTrue(TEXT("A glass's layer is composed over the stained-glass master"), Master.HasValue() && Master.GetValue() == MasterPackage(FName(Masters::Stained)));
	FMatterPaintLayer Canvas = Panel;
	Canvas.Palette = PaintFinish::Palette;
	Canvas.Working = PaintFinish::Working;
	Canvas.History = PaintFinish::History;
	const TValueOrError<FString, FString> CanvasMaster = LayerMasterPackage(Canvas);
	TestTrue(TEXT("a canvas's over the paint master"), CanvasMaster.HasValue() && CanvasMaster.GetValue() == PaintMasterPackage());
	FMatterPaintLayer Stone = Panel;
	Stone.Palette = TEXT("stone.travertine-romano");
	Stone.Working = TEXT("honed");
	Stone.History = TEXT("new");
	TestTrue(TEXT("and a stone carries no image"), LayerMasterPackage(Stone).HasError());

	UMaterialInstanceConstant* Instance = TestMasters::TransientInstance();
	for (const FString& Problem : ComposePaintLayer(Panel, *Stained, *Instance))
	{
		AddError(Problem);
	}
	TestTrue(TEXT("The panel's glass is parented to the stained-glass master"), Instance->Parent == Stained);
	float Width = 0.0f;
	float Thickness = 0.0f;
	UTexture* Carried = nullptr;
	Instance->GetScalarParameterValue(FHashedMaterialParameterInfo(MasterParameter::ImageWidthMeters), Width);
	Instance->GetScalarParameterValue(FHashedMaterialParameterInfo(MasterParameter::PaneThicknessMillimeters), Thickness);
	Instance->GetTextureParameterValue(FHashedMaterialParameterInfo(MasterParameter::Image), Carried);
	TestTrue(TEXT("It carries the panel's image and width"), Carried == Image && Width == 1.6f);
	TestTrue(TEXT("and its glass's thickness, from the glass's working"), Thickness > 0.0f);
	Instance->MarkAsGarbage();
	TestMasters::Retire(Made);
	return true;
}

#endif
