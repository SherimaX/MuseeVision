#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MuseeGraphics.generated.h"

/**
 * How the museum is rendered, as a visitor chooses it: the upscaler, the ray tracing, Lumen's quality
 * and the engine's scalability groups. Every field maps onto real console variables (UMuseeGraphics::Apply).
 */
struct FMuseeGraphicsState
{
	/** EMuseeUpscaler: DLAA, DLSS Ultra Quality / Quality / Balanced / Performance / Ultra Performance, or TSR. */
	int32 Upscaler = 2;
	/** TSR's resolution scale, in percent (DLSS sets its own from the mode). */
	int32 TsrScale = 67;
	/** DLSS Frame Generation: 0 off, 1 on, 2 auto (on when it helps). */
	int32 FrameGen = 2;
	bool bRayReconstruction = true;
	/** Lumen traces hardware ray-tracing geometry (else software distance fields). */
	bool bHardwareRT = true;
	/** 0: the surface cache; 1: hit lighting for reflections; 2: hit lighting for reflections and GI. */
	int32 Reflections = 1;
	bool bRTShadows = true;
	/** 0 off, 1 economy (half the samples), 2 full. */
	int32 MegaLights = 2;
	/** The scalability groups, 0 (Low) to 4 (Cinematic). */
	int32 GI = 3;
	int32 ViewDistance = 3;
	int32 Shadows = 3;
	int32 ReflectionDetail = 3;
	int32 Textures = 3;
	int32 Effects = 3;
	int32 Foliage = 3;
	int32 PostProcess = 3;
	// The display (no preset changes these).
	bool bVSync = true;
	/** Frames per second, 0: none. */
	int32 FrameCap = 0;
	/** The frame-rate readout in the corner. */
	bool bReadout = false;

	/** The same picture (the display fields aside). */
	bool SameRendering(const FMuseeGraphicsState& Other) const;
};

enum class EMuseeUpscaler : int32 { DLAA, DLSSUltraQuality, DLSSQuality, DLSSBalanced, DLSSPerformance, DLSSUltraPerformance, TSR, Count };

/** One line of the settings card: a label and a list of values, read from and written to the state. */
struct FMuseeGraphicsOption
{
	FText Section;
	FText Label;
	TArray<FText> Values;
	TFunction<int32(const FMuseeGraphicsState&)> Get;
	TFunction<void(FMuseeGraphicsState&, int32)> Set;
	/** Non-empty when another choice decides this one (shown quietly, not changeable). */
	TFunction<FText(const FMuseeGraphicsState&)> LockedBy;
};

/**
 * The graphics settings: five presets from Performance to Cinematic, every option on its own, kept in
 * GameUserSettings.ini ([MuseeGraphics]) so they survive a restart; Quality is the first launch's.
 * The keys (AMuseeCharacter): F10 or O (Start on a gamepad) opens the card, F6 and F7 step to a lighter
 * or heavier preset, F8 shows the frame-rate readout. Console: musee.Settings [0|1], musee.Preset <0-4>,
 * musee.Readout [0|1], musee.Graphics (log the state).
 *
 * A scripted or screenshot run (-MuseeDo, -MuseeShot) starts at Quality and saves nothing, so tours and
 * perf runs always see the museum as composed. The headset build keeps its own quality (MuseeVisionVR).
 */
UCLASS()
class MUSEEVISION_API UMuseeGraphics : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static constexpr int32 NumPresets = 5;
	static constexpr int32 DefaultPreset = 2;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	static UMuseeGraphics* Get(const UObject* WorldContext);

	/**
	 * Apply the settings again. DLSS and Streamline load after the game instance (PostEngineInit), so
	 * the HUD calls this when play begins, once their variables exist.
	 */
	void Reapply();

	const FMuseeGraphicsState& State() const { return Current; }
	/** Change the settings: applied at once, saved, and confirmed by the toast. */
	void SetState(const FMuseeGraphicsState& State, bool bToast = true);
	/** The preset the settings match, or INDEX_NONE (Custom). */
	int32 PresetIndex() const;
	void ApplyPreset(int32 Index, bool bToast = true);
	/** A lighter (-1) or heavier (+1) preset; from Custom, the nearest. */
	void StepPreset(int32 Delta);
	void ToggleReadout();

	static FMuseeGraphicsState Preset(int32 Index);
	static FText PresetName(int32 Index);
	/** One line on what the preset does. */
	static FText PresetNote(int32 Index);
	static const TArray<FMuseeGraphicsOption>& Options();
	/** "DLSS Quality · hit-lit reflections · ray-traced shadows", for the toast (the image on a line of its own). */
	static FText Summary(const FMuseeGraphicsState& State, bool bTwoLines = false);

	/** The settings card: the HUD shows it and takes the input while it is open. */
	bool IsPanelOpen() const { return bPanelOpen; }
	void SetPanelOpen(bool bOpen);
	void TogglePanel() { SetPanelOpen(!bPanelOpen); }
	/** Settings and keys don't apply in the headset build. */
	bool IsAvailable() const { return !bHeadset; }

	/** When the settings last changed (FPlatformTime::Seconds), for the toast. */
	double ChangedAt() const { return LastChange; }

	/** Called every frame by the HUD: the frame and GPU times, smoothed. */
	void SampleFrame(float DeltaSeconds);
	/** Frames shown per second (with frame generation, the generated ones too). */
	float Fps() const { return SmoothFps; }
	/** Frames rendered per second. */
	float RenderedFps() const { return SmoothFrameMs > 0.f ? 1000.f / SmoothFrameMs : 0.f; }
	float GpuMs() const { return SmoothGpuMs; }
	/** Whether frame generation is making frames right now. */
	bool IsGenerating() const { return bGenerating; }

	static bool IsDLSSAvailable();
	static bool IsFrameGenAvailable();

private:
	void Apply() const;
	void Load();
	void Save() const;
	void Log(const TCHAR* Why) const;

	FMuseeGraphicsState Current;
	bool bPanelOpen = false;
	/** A scripted run: nothing is loaded or saved. */
	bool bScripted = false;
	bool bHeadset = false;
	double LastChange = -1000.0;
	float SmoothFrameMs = 0.f;
	float SmoothGpuMs = 0.f;
	float SmoothFps = 0.f;
	bool bGenerating = false;
};
