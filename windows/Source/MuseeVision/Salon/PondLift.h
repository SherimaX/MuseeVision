#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Visitor/MuseeInteractable.h"
#include "PondLift.generated.h"

class AMuseeCharacter;
class ASalonPond;

/**
 * The lily pond that lifts (Shared/Wings/Reserve.swift, PondLift and updatePond). Use the golden
 * lily: a line of warm light runs round the rim. Use it again: the whole pond (basin, water and
 * lilies) rises 2.6 m in 5 s on four bronze posts, and under it a 36-step stair goes down east to
 * the Reserve. It settles again when used from the oval floor, or once you leave the oval.
 *
 * It drives the imported parts by their tags: part:pond (the pond), part:pond_post (4),
 * part:pond_rim_light (32), part:pond_halo. With the pond's new edge (ASalonPond, attached to the pond) the line of
 * light is the lip's own (ASalonPond::SetRimReveal) and the imported rim lights stay hidden. As the pond settles, a
 * visitor under its edge is moved out from under it (never caught).
 *
 * Development (not in shipping): musee.PondCheck (anything in the pond's swept volume, the lift's whole travel) and
 * musee.EyeHeight <m> (the camera's height above the feet, e.g. 0.8 seated; 1.6 again after).
 */
UCLASS()
class MUSEEVISION_API APondLift : public AActor, public IMuseeInteractable, public IMuseePromptProvider
{
	GENERATED_BODY()

public:
	APondLift();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// IMuseeInteractable: the golden lily.
	virtual bool CanInteract(const AMuseeCharacter* Visitor, const FHitResult& Hit) const override;
	virtual void Interact(AMuseeCharacter* Visitor, const FHitResult& Hit) override;
	virtual FText InteractHint(const AMuseeCharacter* Visitor, const FHitResult& Hit) const override;

	// IMuseePromptProvider: the same, as a button when you stand by the pond.
	virtual void GetPrompts(const AMuseeCharacter* Visitor, TArray<FMuseeActionPrompt>& Out) const override;
	virtual void RunPrompt(AMuseeCharacter* Visitor, FName Id) override;

private:
	enum class EState : uint8 { Down, Armed, Rising, Up, Lowering };

	void FindParts();
	void Use(AMuseeCharacter* Visitor);
	bool CanUse(const AMuseeCharacter* Visitor) const;
	bool Near(const AMuseeCharacter* Visitor) const;
	bool OverOpening(const FVector& Feet) const;
	FText UseTitle() const;
	void Apply();

	EState State = EState::Down;
	double Height = 0;       // m, 0 … 2.6
	float ArmedTimer = 0.f;
	float RimReveal = 0.f;   // 0 … 1

	TWeakObjectPtr<AActor> Pond;
	FVector PondHome = FVector::ZeroVector;
	struct FPost { TWeakObjectPtr<AActor> Actor; FVector Home; };
	TArray<FPost> Posts;
	TArray<TWeakObjectPtr<AActor>> RimLights;
	TWeakObjectPtr<AActor> Halo;
	TArray<TWeakObjectPtr<AActor>> PondActors;   // the pond and everything attached to it
	TWeakObjectPtr<ASalonPond> Edge;             // the pond's edge (the coping, its lip and its light)

public:
	/** Development: logs anything (visible) inside the pond's swept volume over its whole lift, and the posts' contact. */
	void CheckClearance() const;
};
