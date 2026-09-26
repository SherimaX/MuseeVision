#include "Salon/SalonRain.h"

namespace
{
	constexpr int32 RainSampleRate = 48000;
}

USalonRainWave::USalonRainWave(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetSampleRate(RainSampleRate);
	NumChannels = 1;
	Duration = INDEFINITELY_LOOPING_DURATION;
	bLooping = false;
	SampleByteSize = 2;
}

int32 USalonRainWave::OnGeneratePCMAudio(TArray<uint8>& OutAudio, int32 NumSamples)
{
	OutAudio.SetNumUninitialized(NumSamples * 2);
	int16* Out = reinterpret_cast<int16*>(OutAudio.GetData());
	const float Want = FMath::Clamp(Intensity.load(std::memory_order_relaxed), 0.f, 1.f);
	// Drops a second: a drizzle a few dozen, a downpour a few hundred (the near ones: the far ones are the hiss).
	const float DropsPerSample = (20.f + 380.f * Want * Want) / RainSampleRate;
	for (int32 i = 0; i < NumSamples; ++i)
	{
		Level += (Want - Level) * 0.00005f;   // ease over a fraction of a second
		// The hiss: pink noise (Voss–McCartney, three poles), high-passed so it sits on the glass, not in the room.
		const float White = Rand01() * 2.f - 1.f;
		Pink[0] = 0.99765f * Pink[0] + White * 0.0990460f;
		Pink[1] = 0.96300f * Pink[1] + White * 0.2965164f;
		Pink[2] = 0.57000f * Pink[2] + White * 1.0526913f;
		const float P = (Pink[0] + Pink[1] + Pink[2] + White * 0.1848f) * 0.25f;
		HighPass = 0.93f * (HighPass + P - Last);
		Last = P;
		float S = HighPass * 0.22f * Level;
		// A drop lands: a free voice rings.
		if (Rand01() < DropsPerSample * FMath::Max(Level, 0.02f) * 3.f)
		{
			for (FDrop& D : Drops)
			{
				if (D.Amp < 1e-3f)
				{
					const float Freq = 2500.f + 4500.f * Rand01();
					D.Phase = 0.f;
					D.Step = 2.f * PI * Freq / RainSampleRate;
					D.Amp = (0.04f + 0.22f * Rand01() * Rand01()) * Level;
					D.Decay = FMath::Exp(-1.f / (RainSampleRate * (0.002f + 0.006f * Rand01())));
					break;
				}
			}
		}
		for (FDrop& D : Drops)
		{
			if (D.Amp < 1e-4f) { D.Amp = 0.f; continue; }
			S += D.Amp * FMath::Sin(D.Phase);
			D.Phase += D.Step;
			if (D.Phase > 2.f * PI) { D.Phase -= 2.f * PI; }
			D.Amp *= D.Decay;
		}
		Out[i] = int16(FMath::Clamp(S, -1.f, 1.f) * 32000.f);
	}
	return NumSamples;
}
