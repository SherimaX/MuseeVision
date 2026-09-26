#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Sound/SoundWaveProcedural.h"
#include <atomic>
#include "SeaLight.generated.h"

class UAudioComponent;
class UMaterialInstanceDynamic;
class UMaterialParameterCollection;
class UPostProcessComponent;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UTexture2D;

/**
 * The sea's own sounds, made as they play (after Salon/SalonRain): 0 the hush of water round the car (low brown noise,
 * slowly breathing), 1 the fizz of sparks (fine random ticks, 2–6 kHz, at a rate that follows the stirring), 2 the hiss
 * of a shoal passing (band noise), 3 rain on the surface overhead, muffled (soft low ticks).
 */
UCLASS()
class MUSEEVISION_API USeaLightWave : public USoundWaveProcedural
{
	GENERATED_BODY()

public:
	USeaLightWave(const FObjectInitializer& ObjectInitializer);

	virtual int32 OnGeneratePCMAudio(TArray<uint8>& OutAudio, int32 NumSamples) override;
	virtual Audio::EAudioMixerStreamDataFormat::Type GetGeneratedPCMDataFormat() const override { return Audio::EAudioMixerStreamDataFormat::Int16; }

	/** Which sound (0 hush, 1 fizz, 2 hiss, 3 rain). Set before it plays. */
	int32 Voice = 0;
	/** 0 … 1, set from the game thread. */
	std::atomic<float> Level{0.f};
	/** What it made (RMS of the last buffer, 0 … 1) and how many buffers: the sound's check. */
	std::atomic<float> Rms{0.f};
	std::atomic<int32> Buffers{0};

private:
	uint32 Seed = 0x475u;
	float Brown = 0.f, Band1 = 0.f, Band2 = 0.f, Held = 0.f, Tick = 0.f, TickDecay = 0.f, TickPhase = 0.f, TickStep = 0.f, Breath = 0.f;
	double Clock = 0.0;

	float Rand01()
	{
		Seed = Seed * 1664525u + 1013904223u;
		return float(Seed >> 8) / float(1u << 24);
	}
};

/**
 * II · Sea Light, after Anna Atkins, *Photographs of British Algae* (1843): the Cube's first piece
 * (scratchpad cube_redesign/PROPOSAL.md § 1 and § 5). A sea at night, full of dinoflagellates that flash blue (475 nm)
 * when the water moves round them. Nothing lights up unless something moves, and the first thing that moves is you.
 *
 * - The cells: 1 M dormant points in the water round the car (SM_SeaLight_Cards, four times, turned). M_SeaLight_Cells
 *   lights each from the field where it sits: a flash drawn as a short streak along the water's motion there (longer the
 *   faster it moves), cyan-white while fresh and deep blue as it fades; kept a pixel wide far off, and near the eye
 *   large and soft (out of focus).
 * - The field: 96³ cells over 12 m round the eyes (12.5 cm): how stirred the water is and which way it moves, decaying
 *   in about 0.7 s, splatted on the CPU by what moves, uploaded each frame as a 960² BGRA texture (96 slices, 10 × 10).
 * - The hand: on the desktop a point 0.6 m along the look ray while the right mouse button is held (always, with
 *   `musee.SeaLightHand 1`); in the headset the right controller. It stirs a swirl round the fingers.
 * - The shoal (2:30–4:00): 700 scad, a boids school (separation, alignment, cohesion; the car an obstacle it parts
 *   round, 1 to 3 m off the rail), drawn only by the light on their skins (M_SeaLight_Fish: sparks along the outline)
 *   and by the wake the whole school leaves in the field.
 * - The manta (4:30–5:30): 4.5 m across at 1 m/s over the roof, a dark silhouette (M_SeaLight_Manta, its wings
 *   beating) outlined by the sparks its wing edges and wake stir.
 * - The rain (6:00–7:00): rings of light spreading on the surface 4.5 m overhead (M_SeaLight_Ring), sparks under each.
 * - The score (8 min), placed sound (USeaLightWave), the touch in the headset (the controller's buzz).
 *
 * Console: `musee.Piece 2 [minute] [time scale]` plays it from that minute (0 ends it); `musee.SeaLightHand 0|1|2`.
 */
UCLASS()
class MUSEEVISION_API ACubeSeaLight : public AActor
{
	GENERATED_BODY()

public:
	ACubeSeaLight();

	/** The piece in this world (spawned at the Cube's centre on first use). */
	static ACubeSeaLight* Get(UWorld* World);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

	/** Play from the given second of the score (0: from the start). */
	void Begin(double FromSeconds = 0.0);
	/** Fade to black and stop. */
	void End();
	bool IsPlaying() const { return bPlaying; }
	bool IsFinished() const { return bFinished; }

	/** Test: the hand always on (the desktop's reach without the mouse button). */
	static bool bForceHand;
	/** Test: a small marker where the hand is (the hand's debug view). */
	static bool bShowHand;
	/** Test: log the game-thread costs. */
	static bool bProfile;
	/** Test: the whole field held at this value (below 0: off). */
	static float TestFill;

	/** Its assets are made (Scripts/journeys.py sealight): the car offers it only then. */
	static bool IsAvailable();

	/** Test: the score's speed. */
	float TimeScale = 1.f;

	static constexpr double Length = 480.0;   // 8 min

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UPostProcessComponent> Post;

	/** Exposure while it plays (EV100): a dark-adapted eye (the proposal: about 2; one stop more open reads truer). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float ExposureEV = 1.f;

	/** A flash's peak luminance (cd/m²) at its own size (a few millimetres). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float PeakNits = 12.f;

private:
	static constexpr int32 N = 96;          // the field's cells a side (12.5 cm)
	static constexpr double Extent = 12.0;  // m, the field's size
	static constexpr int32 Tiles = 10;      // slices a row in the uploaded texture
	static constexpr int32 TexSize = N * Tiles;
	static constexpr int32 FishCount = 700;

	struct FFish
	{
		FVector P = FVector::ZeroVector, V = FVector::ZeroVector;   // m, m/s, the eyes' frame
		float Cruise = 1.2f, Size = 1.f, Turn = 0.f;
	};

	void BuildCells();
	void BuildCreatures();
	/** Stir the water at P (m from the eyes) over Radius, moving it with Velocity (m/s). */
	void Splat(const FVector& P, double Radius, double Amount, const FVector& Velocity = FVector::ZeroVector);
	void SplatLine(const FVector& A, const FVector& B, double Radius, double Amount, const FVector& Velocity = FVector::ZeroVector);
	void Decay(float DeltaSeconds);
	void Upload();
	void Score(float DeltaSeconds);
	void MoveHand(float DeltaSeconds);
	void MoveShoal(float DeltaSeconds, float Presence);
	void MoveManta(float DeltaSeconds, double T);
	void Rain(float DeltaSeconds, float Rate);
	void SetRoomBlack(bool bBlack);
	void SetSound(int32 Voice, float Level, const FVector& Where);
	FVector Eyes() const;

	UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Cells;
	UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> CellMaterials;
	UPROPERTY(Transient) TObjectPtr<UInstancedStaticMeshComponent> FishMeshes;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> FishMaterial;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> MantaMesh;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> MantaMaterial;
	UPROPERTY(Transient) TObjectPtr<UInstancedStaticMeshComponent> RingMeshes;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> FieldTexture;
	UPROPERTY(Transient) TObjectPtr<UMaterialParameterCollection> CubeParameters;
	UPROPERTY(Transient) TArray<TObjectPtr<UAudioComponent>> Voices;
	UPROPERTY(Transient) TArray<TObjectPtr<USeaLightWave>> Waves;

	TArray<float> Stir;              // N³, x fastest: how stirred
	TArray<FVector3f> Flow;          // N³: the stirring's momentum (m/s × stir)
	TArray<uint8> SliceLive, SliceDirty;   // per z slice: something stirred there; to be rewritten
	FVector FieldOrigin = FVector::ZeroVector;   // world cm, the field's centre

	bool bPlaying = false, bClosing = false, bFinished = false;
	double T = 0.0;              // score seconds
	float Fade = 0.f;            // 0 … 1: the piece's light (fades in and out at the ends)
	bool bToldReach = false;
	float LogClock = 0.f;
	float ProfClock = 0.f;
	float ProfMax[4] = {0.f, 0.f, 0.f, 0.f};

	// The hand.
	FVector HandPos = FVector::ZeroVector, HandLast = FVector::ZeroVector;
	float HandSpeed = 0.f, HandFizz = 0.f;
	bool bHandValid = false;
	bool bHandVR = false;

	// The shoal.
	TArray<FFish> Fish;
	TArray<int32> GridHead, GridNext;
	float ShoalHiss = 0.f, ShoalShown = 0.f;
	FVector ShoalCentre = FVector::ZeroVector;

	// The manta.
	bool bMantaShown = false;

	// The rain's rings: centre (m, eyes' frame, z the surface), age (s).
	TArray<FVector4> Rings;
	double RainCarry = 0.0, AmbientCarry = 0.0;
	FRandomStream Rng{475};
};
