#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundWaveProcedural.h"
#include <atomic>
#include "SalonRain.generated.h"

/**
 * The sound of rain on a glass roof, made as it plays (no recording): a soft hiss of many far drops, and near ones that
 * tick on the pane, each a short ring of the glass (2.5–7 kHz, dying in a few milliseconds), landing at random at a rate
 * that follows the rain. Intensity (0 … 1) is set from the game thread (ASalonSky, from Giverny's rain).
 */
UCLASS()
class MUSEEVISION_API USalonRainWave : public USoundWaveProcedural
{
	GENERATED_BODY()

public:
	USalonRainWave(const FObjectInitializer& ObjectInitializer);

	virtual int32 OnGeneratePCMAudio(TArray<uint8>& OutAudio, int32 NumSamples) override;
	virtual Audio::EAudioMixerStreamDataFormat::Type GetGeneratedPCMDataFormat() const override { return Audio::EAudioMixerStreamDataFormat::Int16; }

	/** 0 … 1: how hard it rains (read on the audio thread). */
	std::atomic<float> Intensity{0.f};

private:
	struct FDrop { float Phase = 0.f, Step = 0.f, Amp = 0.f, Decay = 0.f; };
	FDrop Drops[24];
	uint32 Seed = 0x1874u;
	float Pink[3] = {0.f, 0.f, 0.f};
	float HighPass = 0.f, Last = 0.f, Level = 0.f;

	float Rand01()
	{
		Seed = Seed * 1664525u + 1013904223u;
		return float(Seed >> 8) / float(1u << 24);
	}
};
