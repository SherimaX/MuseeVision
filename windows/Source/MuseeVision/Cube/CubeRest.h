#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CubeRest.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * The Cube at rest: what reaches the earth (scratchpad cube_redesign/PROPOSAL.md § 1, "At rest"). Above, the Sphere
 * shows the sky now; below, 22 m down, one thing from that sky still arrives: cosmic-ray muons, about one a minute
 * through each square centimetre at the surface, most of them passing on through the chalk and the concrete. When no
 * piece plays, the black room shows their tracks: fine white condensation lines, as in a cloud chamber, each drawn
 * along the whole line where a muon passed and fading in about two seconds; now and then a delta electron curls off one.
 *
 * A Poisson stream (a few a second through a disc 6 m round the car), the zenith angle drawn from cos², the azimuth
 * uniform. Each line is an instance of one thin cylinder (M_CubeMuon: additive, kept at least a pixel wide at any
 * distance, its energy spread to match), in one instanced component: < 0.1 ms.
 *
 * Spawned at the Cube's centre on first use (ACubeRest::Get); it runs while the eyes are in the room and no piece plays.
 */
UCLASS()
class MUSEEVISION_API ACubeRest : public AActor
{
	GENERATED_BODY()

public:
	ACubeRest();

	static ACubeRest* Get(UWorld* World);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Tracks a second through the disc round the car. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float Rate = 2.5f;

	/** A fresh track's luminance (cd/m², at its own width). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	float Nits = 6.f;

private:
	struct FSegment
	{
		FVector A = FVector::ZeroVector, B = FVector::ZeroVector;   // m, from the eyes
		float Age = 0.f;
		float Delay = 0.f;       // a delta's curl forms a moment after the line
		float Strength = 1.f;
	};

	void AddTrack();
	void AddCurl(const FVector& From, const FVector& Along);
	void Draw();

	UPROPERTY(Transient) TObjectPtr<UInstancedStaticMeshComponent> Lines;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> LineMaterial;

	TArray<FSegment> Segments;
	int32 Drawn = 0;
	double Carry = 0.0;
	float Presence = 0.f;
	FRandomStream Rng{1912};   // (Hess's balloon flights, 1912)
};
