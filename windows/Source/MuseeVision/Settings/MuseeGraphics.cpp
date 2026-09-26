#include "Settings/MuseeGraphics.h"

#include "MuseeVision.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Parse.h"
#include "RHI.h"
#include "Scalability.h"
#if MUSEE_WITH_STREAMLINE
#include "StreamlineDLSSG.h"
#endif

#define LOCTEXT_NAMESPACE "MuseeGraphics"

namespace
{
	const TCHAR* Section = TEXT("MuseeGraphics");

	/** The screen percentage of each upscaler mode (DLSS picks its mode from it). */
	float ScreenPercent(const FMuseeGraphicsState& S)
	{
		switch (static_cast<EMuseeUpscaler>(S.Upscaler))
		{
		case EMuseeUpscaler::DLAA: return 100.f;
		case EMuseeUpscaler::DLSSUltraQuality: return 77.f;
		case EMuseeUpscaler::DLSSQuality: return 67.f;
		case EMuseeUpscaler::DLSSBalanced: return 58.f;
		case EMuseeUpscaler::DLSSPerformance: return 50.f;
		case EMuseeUpscaler::DLSSUltraPerformance: return 33.3f;
		default: return float(FMath::Clamp(S.TsrScale, 50, 100));
		}
	}

	bool UsesDLSS(const FMuseeGraphicsState& S)
	{
		return S.Upscaler != int32(EMuseeUpscaler::TSR) && UMuseeGraphics::IsDLSSAvailable();
	}

	/**
	 * Set a console variable at the priority it already has: the project's [SystemSettings] (DLSS, frame
	 * generation, vertical sync) keep theirs, and the scalability groups' own variables stay theirs, so the
	 * next change of a group doesn't fight these. Apply sets the groups first and these after.
	 */
	void SetCVar(const TCHAR* Name, const FString& Value)
	{
		if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(Name))
		{
			if (!CVar->GetString().Equals(Value)) { CVar->SetWithCurrentPriority(*Value); }
		}
	}
	void SetCVar(const TCHAR* Name, int32 Value) { SetCVar(Name, FString::FromInt(Value)); }

	FText QualityName(int32 Level)
	{
		switch (Level)
		{
		case 0: return LOCTEXT("Low", "Low");
		case 1: return LOCTEXT("Medium", "Medium");
		case 2: return LOCTEXT("High", "High");
		case 3: return LOCTEXT("Epic", "Epic");
		default: return LOCTEXT("Cinematic", "Cinematic");
		}
	}

	TArray<FText> QualityNames(int32 From = 0)
	{
		TArray<FText> Names;
		for (int32 Level = From; Level <= 4; ++Level) { Names.Add(QualityName(Level)); }
		return Names;
	}

	/** A scalability group's line: Low to Cinematic. */
	FMuseeGraphicsOption Group(const FText& InSection, const FText& Label, int32 FMuseeGraphicsState::* Field)
	{
		FMuseeGraphicsOption Option;
		Option.Section = InSection;
		Option.Label = Label;
		Option.Values = QualityNames();
		Option.Get = [Field](const FMuseeGraphicsState& S) { return S.*Field; };
		Option.Set = [Field](FMuseeGraphicsState& S, int32 Value) { S.*Field = Value; };
		return Option;
	}

	FMuseeGraphicsOption Toggle(const FText& InSection, const FText& Label, bool FMuseeGraphicsState::* Field, const FText& Off, const FText& On)
	{
		FMuseeGraphicsOption Option;
		Option.Section = InSection;
		Option.Label = Label;
		Option.Values = {Off, On};
		Option.Get = [Field](const FMuseeGraphicsState& S) { return S.*Field ? 1 : 0; };
		Option.Set = [Field](FMuseeGraphicsState& S, int32 Value) { S.*Field = Value != 0; };
		return Option;
	}

	TArray<FMuseeGraphicsOption> MakeOptions()
	{
		const FText Image = LOCTEXT("Image", "Image");
		const FText Light = LOCTEXT("Light", "Light");
		const FText Detail = LOCTEXT("Detail", "Detail");
		const FText Display = LOCTEXT("Display", "Display");
		const FText Off = LOCTEXT("Off", "Off");
		const FText On = LOCTEXT("On", "On");
		TArray<FMuseeGraphicsOption> Options;

		// The image: how many pixels are rendered and how they are rebuilt.
		{
			FMuseeGraphicsOption O;
			O.Section = Image;
			O.Label = LOCTEXT("Upscaler", "Upscaler");
			O.Values = {LOCTEXT("DLAA", "DLAA"), LOCTEXT("DLSSUltraQuality", "DLSS Ultra Quality"), LOCTEXT("DLSSQuality", "DLSS Quality"), LOCTEXT("DLSSBalanced", "DLSS Balanced"),
						LOCTEXT("DLSSPerformance", "DLSS Performance"), LOCTEXT("DLSSUltra", "DLSS Ultra Performance"), LOCTEXT("TSR", "TSR")};
			O.Get = [](const FMuseeGraphicsState& S) { return S.Upscaler; };
			O.Set = [](FMuseeGraphicsState& S, int32 V) { S.Upscaler = V; };
			O.LockedBy = [](const FMuseeGraphicsState&) { return UMuseeGraphics::IsDLSSAvailable() ? FText::GetEmpty() : LOCTEXT("TSROnly", "TSR"); };
			Options.Add(O);
		}
		{
			FMuseeGraphicsOption O;
			O.Section = Image;
			O.Label = LOCTEXT("Resolution", "Resolution scale");
			// DLSS's own fractions, and two between Quality and native.
			static const int32 Scales[] = {50, 58, 67, 75, 83, 100};
			for (const int32 Percent : Scales) { O.Values.Add(FText::Format(LOCTEXT("Percent", "{0} %"), FText::AsNumber(Percent))); }
			O.Get = [](const FMuseeGraphicsState& S)
			{
				int32 Best = 0;
				for (int32 i = 1; i < UE_ARRAY_COUNT(Scales); ++i) { if (FMath::Abs(Scales[i] - S.TsrScale) < FMath::Abs(Scales[Best] - S.TsrScale)) { Best = i; } }
				return Best;
			};
			O.Set = [](FMuseeGraphicsState& S, int32 V) { S.TsrScale = Scales[FMath::Clamp(V, 0, int32(UE_ARRAY_COUNT(Scales)) - 1)]; };
			O.LockedBy = [](const FMuseeGraphicsState& S)
			{
				return UsesDLSS(S) ? FText::Format(LOCTEXT("ByDLSS", "{0} %, set by DLSS"), FText::AsNumber(FMath::RoundToInt(ScreenPercent(S)))) : FText::GetEmpty();
			};
			Options.Add(O);
		}
		{
			FMuseeGraphicsOption O;
			O.Section = Image;
			O.Label = LOCTEXT("FrameGen", "Frame generation");
			O.Values = {Off, On, LOCTEXT("Auto", "Auto")};
			O.Get = [](const FMuseeGraphicsState& S) { return S.FrameGen; };
			O.Set = [](FMuseeGraphicsState& S, int32 V) { S.FrameGen = V; };
			O.LockedBy = [](const FMuseeGraphicsState&) { return UMuseeGraphics::IsFrameGenAvailable() ? FText::GetEmpty() : LOCTEXT("NotAvailable", "Not available"); };
			Options.Add(O);
		}
		{
			FMuseeGraphicsOption O = Toggle(Image, LOCTEXT("RR", "Ray reconstruction"), &FMuseeGraphicsState::bRayReconstruction, Off, On);
			O.LockedBy = [](const FMuseeGraphicsState& S) { return UsesDLSS(S) ? FText::GetEmpty() : LOCTEXT("DLSSOnly", "With DLSS only"); };
			Options.Add(O);
		}

		// The light: Lumen, the reflections, the shadows.
		Options.Add(Toggle(Light, LOCTEXT("Tracing", "Ray tracing"), &FMuseeGraphicsState::bHardwareRT, LOCTEXT("Software", "Software"), LOCTEXT("Hardware", "Hardware")));
		{
			FMuseeGraphicsOption O;
			O.Section = Light;
			O.Label = LOCTEXT("Reflections", "Reflections");
			O.Values = {LOCTEXT("SurfaceCache", "Surface cache"), LOCTEXT("HitLighting", "Hit lighting"), LOCTEXT("HitLightingAll", "Hit lighting, all light")};
			O.Get = [](const FMuseeGraphicsState& S) { return S.Reflections; };
			O.Set = [](FMuseeGraphicsState& S, int32 V) { S.Reflections = V; };
			O.LockedBy = [](const FMuseeGraphicsState& S) { return S.bHardwareRT ? FText::GetEmpty() : LOCTEXT("SoftwareCache", "Surface cache"); };
			Options.Add(O);
		}
		Options.Add(Toggle(Light, LOCTEXT("Shadows", "Shadows"), &FMuseeGraphicsState::bRTShadows, LOCTEXT("ShadowMaps", "Shadow maps"), LOCTEXT("RayTraced", "Ray traced")));
		{
			FMuseeGraphicsOption O;
			O.Section = Light;
			O.Label = LOCTEXT("MegaLights", "MegaLights");
			O.Values = {Off, LOCTEXT("Economy", "Economy"), LOCTEXT("Full", "Full")};
			O.Get = [](const FMuseeGraphicsState& S) { return S.MegaLights; };
			O.Set = [](FMuseeGraphicsState& S, int32 V) { S.MegaLights = V; };
			Options.Add(O);
		}
		{
			// Low turns Lumen off altogether (a dark museum): Medium is the least.
			FMuseeGraphicsOption O;
			O.Section = Light;
			O.Label = LOCTEXT("GI", "Global illumination");
			O.Values = QualityNames(1);
			O.Get = [](const FMuseeGraphicsState& S) { return FMath::Clamp(S.GI, 1, 4) - 1; };
			O.Set = [](FMuseeGraphicsState& S, int32 V) { S.GI = V + 1; };
			Options.Add(O);
		}

		// The engine's scalability groups.
		Options.Add(Group(Detail, LOCTEXT("ViewDistance", "View distance"), &FMuseeGraphicsState::ViewDistance));
		Options.Add(Group(Detail, LOCTEXT("ShadowDetail", "Shadow detail"), &FMuseeGraphicsState::Shadows));
		Options.Add(Group(Detail, LOCTEXT("ReflectionDetail", "Reflection detail"), &FMuseeGraphicsState::ReflectionDetail));
		Options.Add(Group(Detail, LOCTEXT("Textures", "Textures"), &FMuseeGraphicsState::Textures));
		Options.Add(Group(Detail, LOCTEXT("Effects", "Effects"), &FMuseeGraphicsState::Effects));
		Options.Add(Group(Detail, LOCTEXT("Foliage", "Foliage"), &FMuseeGraphicsState::Foliage));
		Options.Add(Group(Detail, LOCTEXT("PostProcess", "Post-processing"), &FMuseeGraphicsState::PostProcess));

		// The display.
		Options.Add(Toggle(Display, LOCTEXT("VSync", "Vertical sync"), &FMuseeGraphicsState::bVSync, Off, On));
		{
			static const int32 Caps[] = {0, 30, 60, 90, 120};
			FMuseeGraphicsOption O;
			O.Section = Display;
			O.Label = LOCTEXT("FrameCap", "Frame-rate cap");
			O.Values = {LOCTEXT("NoCap", "None")};
			for (int32 i = 1; i < UE_ARRAY_COUNT(Caps); ++i) { O.Values.Add(FText::Format(LOCTEXT("Fps", "{0} fps"), FText::AsNumber(Caps[i]))); }
			O.Get = [](const FMuseeGraphicsState& S)
			{
				for (int32 i = 0; i < UE_ARRAY_COUNT(Caps); ++i) { if (Caps[i] == S.FrameCap) { return i; } }
				return 0;
			};
			O.Set = [](FMuseeGraphicsState& S, int32 V) { S.FrameCap = Caps[FMath::Clamp(V, 0, int32(UE_ARRAY_COUNT(Caps)) - 1)]; };
			Options.Add(O);
		}
		Options.Add(Toggle(Display, LOCTEXT("Readout", "Frame-rate readout"), &FMuseeGraphicsState::bReadout, Off, On));
		return Options;
	}

	UMuseeGraphics* FromWorld(UWorld* World)
	{
		return World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UMuseeGraphics>() : nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs GSettingsCommand(
		TEXT("musee.Settings"), TEXT("musee.Settings [0|1]: hide or show the graphics settings card (no argument: toggle)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UMuseeGraphics* Graphics = FromWorld(World))
			{
				Graphics->SetPanelOpen(Args.Num() ? FCString::Atoi(*Args[0]) != 0 : !Graphics->IsPanelOpen());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GPresetCommand(
		TEXT("musee.Preset"), TEXT("musee.Preset <0-4 | performance, balanced, quality, ultra, cinematic>: apply a graphics preset (+ and - step)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UMuseeGraphics* Graphics = FromWorld(World);
			if (!Graphics) { return; }
			if (Args.Num() == 0)
			{
				UE_LOG(LogMusee, Log, TEXT("Graphics: preset %d (%s)."), Graphics->PresetIndex(), *UMuseeGraphics::PresetName(Graphics->PresetIndex()).ToString());
				return;
			}
			if (Args[0] == TEXT("+") || Args[0] == TEXT("-")) { Graphics->StepPreset(Args[0] == TEXT("+") ? 1 : -1); return; }
			int32 Index = Args[0].IsNumeric() ? FCString::Atoi(*Args[0]) : INDEX_NONE;
			for (int32 i = 0; i < UMuseeGraphics::NumPresets && Index == INDEX_NONE; ++i)
			{
				if (UMuseeGraphics::PresetName(i).ToString().Equals(Args[0], ESearchCase::IgnoreCase)) { Index = i; }
			}
			if (Index >= 0 && Index < UMuseeGraphics::NumPresets) { Graphics->ApplyPreset(Index); }
		}));

	FAutoConsoleCommandWithWorldAndArgs GReadoutCommand(
		TEXT("musee.Readout"), TEXT("musee.Readout [0|1]: the frame-rate readout in the corner (no argument: toggle)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (UMuseeGraphics* Graphics = FromWorld(World))
			{
				FMuseeGraphicsState S = Graphics->State();
				S.bReadout = Args.Num() ? FCString::Atoi(*Args[0]) != 0 : !S.bReadout;
				Graphics->SetState(S, false);
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GGraphicsCommand(
		TEXT("musee.Graphics"), TEXT("musee.Graphics: log the graphics settings; musee.Graphics <line> <value>: set one line of the card (0-based)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UMuseeGraphics* Graphics = FromWorld(World);
			if (!Graphics) { return; }
			const TArray<FMuseeGraphicsOption>& Options = UMuseeGraphics::Options();
			if (Args.Num() >= 2)
			{
				const int32 Line = FCString::Atoi(*Args[0]);
				if (Options.IsValidIndex(Line))
				{
					FMuseeGraphicsState S = Graphics->State();
					Options[Line].Set(S, FMath::Clamp(FCString::Atoi(*Args[1]), 0, Options[Line].Values.Num() - 1));
					Graphics->SetState(S);
				}
				return;
			}
			for (int32 Line = 0; Line < Options.Num(); ++Line)
			{
				const FMuseeGraphicsOption& O = Options[Line];
				const int32 Value = O.Get(Graphics->State());
				UE_LOG(LogMusee, Log, TEXT("Graphics %2d  %-20s %s"), Line, *O.Label.ToString(), O.Values.IsValidIndex(Value) ? *O.Values[Value].ToString() : TEXT("?"));
			}
		}));
}

bool FMuseeGraphicsState::SameRendering(const FMuseeGraphicsState& O) const
{
	return Upscaler == O.Upscaler && (Upscaler != int32(EMuseeUpscaler::TSR) || TsrScale == O.TsrScale)
		&& FrameGen == O.FrameGen && bRayReconstruction == O.bRayReconstruction && bHardwareRT == O.bHardwareRT
		&& Reflections == O.Reflections && bRTShadows == O.bRTShadows && MegaLights == O.MegaLights && GI == O.GI
		&& ViewDistance == O.ViewDistance && Shadows == O.Shadows && ReflectionDetail == O.ReflectionDetail
		&& Textures == O.Textures && Effects == O.Effects && Foliage == O.Foliage && PostProcess == O.PostProcess;
}

void UMuseeGraphics::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// The headset build sets its own quality (MuseeVisionVR); nothing here touches it.
	bHeadset = FindObject<UClass>(nullptr, TEXT("/Script/MuseeVisionVR.MuseeVRCharacter")) != nullptr;
	if (bHeadset) { return; }
	const TCHAR* Line = FCommandLine::Get();
	FString Unused;
	bScripted = FParse::Value(Line, TEXT("-MuseeDo="), Unused, false) || FParse::Value(Line, TEXT("-MuseeShot="), Unused, false);
	Current = Preset(DefaultPreset);
	if (!bScripted) { Load(); }
	Apply();
	Log(bScripted ? TEXT("scripted run, Quality") : TEXT("start"));
}

void UMuseeGraphics::Reapply()
{
	if (bHeadset) { return; }
	Apply();
	Log(TEXT("play"));
}

UMuseeGraphics* UMuseeGraphics::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UMuseeGraphics>() : nullptr;
}

FMuseeGraphicsState UMuseeGraphics::Preset(int32 Index)
{
	FMuseeGraphicsState S;   // Quality: the museum as composed (DefaultEngine.ini, DefaultGameUserSettings.ini)
	switch (FMath::Clamp(Index, 0, NumPresets - 1))
	{
	case 0:   // Performance: a quarter of the pixels, reflections from the surface cache, shadow maps, MegaLights at half its samples.
		S.Upscaler = int32(EMuseeUpscaler::DLSSPerformance);
		S.bRayReconstruction = false;
		// Lumen keeps the hardware rays: in software (distance fields) the Atrium's upper walls went dark.
		S.Reflections = 0;
		S.bRTShadows = false;
		S.MegaLights = 1;
		S.GI = 2;
		S.ViewDistance = 2;
		S.Shadows = 2;
		S.Effects = 2;
		S.Foliage = 2;
		S.PostProcess = 2;
		// Reflection detail stays Epic: High drops the front-layer reflections, and the glass all but vanishes.
		break;
	case 1:   // Balanced: ray traced, but reflections from Lumen's surface cache.
		S.Upscaler = int32(EMuseeUpscaler::DLSSBalanced);
		S.Reflections = 0;
		break;
	case 3:   // Ultra: more pixels (DLSS at 77 %), Lumen and MegaLights at their finest.
		S.Upscaler = int32(EMuseeUpscaler::DLSSUltraQuality);
		S.GI = 4;
		S.ViewDistance = 4;
		S.Shadows = 4;
		S.ReflectionDetail = 4;
		S.Effects = 4;
		S.Foliage = 4;
		S.PostProcess = 4;
		break;
	case 4:   // Cinematic: every pixel rendered (DLAA), hit lighting for all the light, reflections in reflections.
		S = Preset(3);
		S.Upscaler = int32(EMuseeUpscaler::DLAA);
		S.Reflections = 2;
		S.Textures = 4;
		break;
	default:
		break;
	}
	return S;
}

FText UMuseeGraphics::PresetName(int32 Index)
{
	switch (Index)
	{
	case 0: return LOCTEXT("Performance", "Performance");
	case 1: return LOCTEXT("Balanced", "Balanced");
	case 2: return LOCTEXT("Quality", "Quality");
	case 3: return LOCTEXT("Ultra", "Ultra");
	case 4: return LOCTEXT("CinematicPreset", "Cinematic");
	default: return LOCTEXT("Custom", "Custom");
	}
}

FText UMuseeGraphics::PresetNote(int32 Index)
{
	switch (Index)
	{
	case 0: return LOCTEXT("PerformanceNote", "Fluid above all: DLSS Performance, surface-cache light, shadow maps.");
	case 1: return LOCTEXT("BalancedNote", "Ray-traced light and shadow; reflections from Lumen's surface cache.");
	case 2: return LOCTEXT("QualityNote", "The museum as composed: DLSS Quality, hit-lit reflections.");
	case 3: return LOCTEXT("UltraNote", "More pixels, DLSS at 77 %; Lumen and MegaLights at their finest.");
	case 4: return LOCTEXT("CinematicNote", "Every pixel rendered (DLAA), hit lighting for all the light, reflections in reflections.");
	default: return LOCTEXT("CustomNote", "An arrangement of your own.");
	}
}

const TArray<FMuseeGraphicsOption>& UMuseeGraphics::Options()
{
	static const TArray<FMuseeGraphicsOption> All = MakeOptions();
	return All;
}

FText UMuseeGraphics::Summary(const FMuseeGraphicsState& S, bool bTwoLines)
{
	const FMuseeGraphicsOption& Upscaler = Options()[0];
	FText Image = UsesDLSS(S) ? Upscaler.Values[S.Upscaler]
		: FText::Format(LOCTEXT("TSRAt", "TSR at {0} %"), FText::AsNumber(FMath::RoundToInt(ScreenPercent(S))));
	FText Light;
	if (!S.bHardwareRT) { Light = LOCTEXT("SumSoftware", "software Lumen"); }
	else if (S.Reflections == 0) { Light = LOCTEXT("SumCache", "surface-cache reflections"); }
	else if (S.Reflections == 1) { Light = LOCTEXT("SumHit", "hit-lit reflections"); }
	else { Light = LOCTEXT("SumHitAll", "hit lighting throughout"); }
	const FText Shadow = S.bRTShadows ? LOCTEXT("SumRTShadows", "ray-traced shadows") : LOCTEXT("SumShadowMaps", "shadow maps");
	return FText::Format(bTwoLines ? LOCTEXT("Summary2", "{0}\n{1} · {2}") : LOCTEXT("Summary", "{0} · {1} · {2}"), Image, Light, Shadow);
}

int32 UMuseeGraphics::PresetIndex() const
{
	for (int32 i = 0; i < NumPresets; ++i)
	{
		if (Current.SameRendering(Preset(i))) { return i; }
	}
	return INDEX_NONE;
}

void UMuseeGraphics::SetState(const FMuseeGraphicsState& State, bool bToast)
{
	if (bHeadset) { return; }
	Current = State;
	Apply();
	Save();
	if (bToast) { LastChange = FPlatformTime::Seconds(); }
	Log(TEXT("set"));
}

void UMuseeGraphics::ApplyPreset(int32 Index, bool bToast)
{
	// A preset sets the picture; the display (sync, cap, readout) stays as chosen.
	FMuseeGraphicsState S = Preset(Index);
	S.bVSync = Current.bVSync;
	S.FrameCap = Current.FrameCap;
	S.bReadout = Current.bReadout;
	SetState(S, bToast);
}

void UMuseeGraphics::StepPreset(int32 Delta)
{
	int32 Index = PresetIndex();
	if (Index == INDEX_NONE)
	{
		// From an arrangement of one's own: the preset of the same upscaler, then a step from there.
		Index = DefaultPreset;
		for (int32 i = 0; i < NumPresets; ++i) { if (Preset(i).Upscaler == Current.Upscaler) { Index = i; break; } }
	}
	ApplyPreset(FMath::Clamp(Index + Delta, 0, NumPresets - 1));
}

void UMuseeGraphics::ToggleReadout()
{
	FMuseeGraphicsState S = Current;
	S.bReadout = !S.bReadout;
	SetState(S, false);
}

void UMuseeGraphics::SetPanelOpen(bool bOpen)
{
	bPanelOpen = bOpen && !bHeadset;
}

bool UMuseeGraphics::IsDLSSAvailable()
{
	return IConsoleManager::Get().FindConsoleVariable(TEXT("r.NGX.DLSS.Enable")) != nullptr;
}

bool UMuseeGraphics::IsFrameGenAvailable()
{
#if MUSEE_WITH_STREAMLINE
	return IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.DLSSG.Enable")) != nullptr && IsStreamlineDLSSGSupported();
#else
	return false;
#endif
}

void UMuseeGraphics::Apply() const
{
	const FMuseeGraphicsState& S = Current;
	const bool bDLSS = UsesDLSS(S);

	// The scalability groups first: each sets its own family of variables, which the lines below refine.
	Scalability::FQualityLevels Levels = Scalability::GetQualityLevels();
	Levels.ResolutionQuality = ScreenPercent(S);
	Levels.ViewDistanceQuality = S.ViewDistance;
	Levels.ShadowQuality = S.Shadows;
	Levels.GlobalIlluminationQuality = S.GI;
	Levels.ReflectionQuality = S.ReflectionDetail;
	Levels.TextureQuality = S.Textures;
	Levels.EffectsQuality = S.Effects;
	Levels.FoliageQuality = S.Foliage;
	Levels.PostProcessQuality = S.PostProcess;
	Scalability::SetQualityLevels(Levels);

	// The image: DLSS picks its mode from the screen percentage (67 Quality, 58 Balanced, 50 Performance,
	// 33 Ultra Performance, 100 DLAA); without it, TSR upscales (r.AntiAliasingMethod 4).
	SetCVar(TEXT("r.ScreenPercentage"), FString::SanitizeFloat(ScreenPercent(S)));
	SetCVar(TEXT("r.NGX.DLSS.Enable"), bDLSS ? 1 : 0);
	SetCVar(TEXT("r.NGX.DLSS.DenoiserMode"), bDLSS && S.bRayReconstruction ? 1 : 0);
	SetCVar(TEXT("r.Streamline.DLSSG.Enable"), IsFrameGenAvailable() ? FMath::Clamp(S.FrameGen, 0, 2) : 0);

	// The light. Hit lighting evaluates the full material and lights where a reflection ray lands (2), or
	// every Lumen ray (1); the surface cache (0) reuses Lumen's cached lighting. Scalability's High GI
	// disallows hit lighting: allowed here whenever it's chosen, so the lines stay independent.
	SetCVar(TEXT("r.Lumen.HardwareRayTracing"), S.bHardwareRT ? 1 : 0);
	const int32 LightingMode = !S.bHardwareRT ? 0 : (S.Reflections == 1 ? 2 : (S.Reflections >= 2 ? 1 : 0));
	SetCVar(TEXT("r.Lumen.HardwareRayTracing.LightingMode"), LightingMode);
	SetCVar(TEXT("r.Lumen.HardwareRayTracing.HitLighting.Allowed"), LightingMode != 0 ? 1 : 0);
	// Reflections within reflections (the polished floors in the glass, the mirror-gilt): hit lighting only.
	SetCVar(TEXT("r.Lumen.Reflections.MaxBounces"), LightingMode == 1 ? 3 : 0);
	SetCVar(TEXT("r.RayTracing.Shadows"), S.bRTShadows ? 1 : 0);
	SetCVar(TEXT("r.MegaLights.Allowed"), S.MegaLights > 0 ? 1 : 0);
	SetCVar(TEXT("r.MegaLights.NumSamplesPerPixel"), S.MegaLights == 1 || S.Shadows < 2 ? 2 : 4);

	// The display.
	SetCVar(TEXT("r.VSync"), S.bVSync ? 1 : 0);
	SetCVar(TEXT("t.MaxFPS"), S.FrameCap);

	// GameUserSettings mirrors the groups, sync and cap, so its own ApplySettings (the game mode's full
	// screen) keeps them.
	if (UGameUserSettings* User = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		User->SetViewDistanceQuality(S.ViewDistance);
		User->SetShadowQuality(S.Shadows);
		User->SetGlobalIlluminationQuality(S.GI);
		User->SetReflectionQuality(S.ReflectionDetail);
		User->SetTextureQuality(S.Textures);
		User->SetVisualEffectQuality(S.Effects);
		User->SetFoliageQuality(S.Foliage);
		User->SetPostProcessingQuality(S.PostProcess);
		User->SetResolutionScaleValueEx(ScreenPercent(S));
		User->SetVSyncEnabled(S.bVSync);
		User->SetFrameRateLimit(float(S.FrameCap));
	}
}

void UMuseeGraphics::Load()
{
	const FString& Ini = GGameUserSettingsIni;
	int32 Version = 0;
	if (!GConfig->GetInt(Section, TEXT("Version"), Version, Ini) || Version != 2) { return; }   // first launch: Quality
	auto Int = [&Ini](const TCHAR* Key, int32& Value, int32 Min, int32 Max)
	{
		int32 Read = Value;
		if (GConfig->GetInt(Section, Key, Read, Ini)) { Value = FMath::Clamp(Read, Min, Max); }
	};
	auto Bool = [&Ini](const TCHAR* Key, bool& Value) { GConfig->GetBool(Section, Key, Value, Ini); };
	FMuseeGraphicsState& S = Current;
	Int(TEXT("Upscaler"), S.Upscaler, 0, int32(EMuseeUpscaler::Count) - 1);
	Int(TEXT("TsrScale"), S.TsrScale, 50, 100);
	Int(TEXT("FrameGen"), S.FrameGen, 0, 2);
	Bool(TEXT("RayReconstruction"), S.bRayReconstruction);
	Bool(TEXT("HardwareRT"), S.bHardwareRT);
	Int(TEXT("Reflections"), S.Reflections, 0, 2);
	Bool(TEXT("RTShadows"), S.bRTShadows);
	Int(TEXT("MegaLights"), S.MegaLights, 0, 2);
	Int(TEXT("GI"), S.GI, 1, 4);
	Int(TEXT("ViewDistance"), S.ViewDistance, 0, 4);
	Int(TEXT("Shadows"), S.Shadows, 0, 4);
	Int(TEXT("ReflectionDetail"), S.ReflectionDetail, 0, 4);
	Int(TEXT("Textures"), S.Textures, 0, 4);
	Int(TEXT("Effects"), S.Effects, 0, 4);
	Int(TEXT("Foliage"), S.Foliage, 0, 4);
	Int(TEXT("PostProcess"), S.PostProcess, 0, 4);
	Bool(TEXT("VSync"), S.bVSync);
	Int(TEXT("FrameCap"), S.FrameCap, 0, 1000);
	Bool(TEXT("Readout"), S.bReadout);
}

void UMuseeGraphics::Save() const
{
	if (bScripted || bHeadset) { return; }
	const FString& Ini = GGameUserSettingsIni;
	const FMuseeGraphicsState& S = Current;
	GConfig->SetInt(Section, TEXT("Version"), 2, Ini);
	GConfig->SetInt(Section, TEXT("Upscaler"), S.Upscaler, Ini);
	GConfig->SetInt(Section, TEXT("TsrScale"), S.TsrScale, Ini);
	GConfig->SetInt(Section, TEXT("FrameGen"), S.FrameGen, Ini);
	GConfig->SetBool(Section, TEXT("RayReconstruction"), S.bRayReconstruction, Ini);
	GConfig->SetBool(Section, TEXT("HardwareRT"), S.bHardwareRT, Ini);
	GConfig->SetInt(Section, TEXT("Reflections"), S.Reflections, Ini);
	GConfig->SetBool(Section, TEXT("RTShadows"), S.bRTShadows, Ini);
	GConfig->SetInt(Section, TEXT("MegaLights"), S.MegaLights, Ini);
	GConfig->SetInt(Section, TEXT("GI"), S.GI, Ini);
	GConfig->SetInt(Section, TEXT("ViewDistance"), S.ViewDistance, Ini);
	GConfig->SetInt(Section, TEXT("Shadows"), S.Shadows, Ini);
	GConfig->SetInt(Section, TEXT("ReflectionDetail"), S.ReflectionDetail, Ini);
	GConfig->SetInt(Section, TEXT("Textures"), S.Textures, Ini);
	GConfig->SetInt(Section, TEXT("Effects"), S.Effects, Ini);
	GConfig->SetInt(Section, TEXT("Foliage"), S.Foliage, Ini);
	GConfig->SetInt(Section, TEXT("PostProcess"), S.PostProcess, Ini);
	GConfig->SetBool(Section, TEXT("VSync"), S.bVSync, Ini);
	GConfig->SetInt(Section, TEXT("FrameCap"), S.FrameCap, Ini);
	GConfig->SetBool(Section, TEXT("Readout"), S.bReadout, Ini);
	if (UGameUserSettings* User = GEngine ? GEngine->GetGameUserSettings() : nullptr) { User->SaveSettings(); }
	GConfig->Flush(false, Ini);
}

void UMuseeGraphics::Log(const TCHAR* Why) const
{
	UE_LOG(LogMusee, Log, TEXT("Graphics (%s): %s, %s; screen %.0f%%, DLSS %s, RR %d, FG %d, Lumen %s, reflections %d, RT shadows %d, MegaLights %d, sg GI %d VD %d SH %d RE %d TX %d FX %d FO %d PP %d, vsync %d, cap %d."),
		Why, *PresetName(PresetIndex()).ToString(), *Summary(Current).ToString(), ScreenPercent(Current), UsesDLSS(Current) ? TEXT("on") : TEXT("off"),
		Current.bRayReconstruction ? 1 : 0, Current.FrameGen, Current.bHardwareRT ? TEXT("HW") : TEXT("SW"), Current.Reflections, Current.bRTShadows ? 1 : 0,
		Current.MegaLights, Current.GI, Current.ViewDistance, Current.Shadows, Current.ReflectionDetail, Current.Textures, Current.Effects, Current.Foliage,
		Current.PostProcess, Current.bVSync ? 1 : 0, Current.FrameCap);
}

void UMuseeGraphics::SampleFrame(float DeltaSeconds)
{
	if (DeltaSeconds <= 0.f) { return; }
	const float FrameMs = FMath::Min(DeltaSeconds * 1000.f, 250.f);
	const float GpuMs = FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles(0));
	// Smoothed over about half a second, so the numbers can be read.
	const float Blend = 1.f - FMath::Exp(-DeltaSeconds / 0.5f);
	SmoothFrameMs = SmoothFrameMs > 0.f ? FMath::Lerp(SmoothFrameMs, FrameMs, Blend) : FrameMs;
	if (GpuMs > 0.f) { SmoothGpuMs = SmoothGpuMs > 0.f ? FMath::Lerp(SmoothGpuMs, GpuMs, Blend) : GpuMs; }
	int32 Presented = 1;
#if MUSEE_WITH_STREAMLINE
	if (IsFrameGenAvailable() && Current.FrameGen > 0)
	{
		float Hertz = 0.f;
		GetStreamlineDLSSGFrameTiming(Hertz, Presented);
		Presented = FMath::Max(1, Presented);
	}
#endif
	bGenerating = Presented > 1;
	SmoothFps = RenderedFps() * Presented;
}

#undef LOCTEXT_NAMESPACE
