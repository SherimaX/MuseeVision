#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "LightOperation.h"
#include "MuseeKey.h"
#include "MuseeUnits.h"

#include "LightLuminaireComponent.generated.h"

class ULocalLightComponent;
struct FMuseeChannel;
struct FTimeMoment;

/**
 * One luminaire in the museum: a catalogue type (Data/Light/Luminaires) mounted where a stage's lighting schedule puts
 * it, with its light leaving along the component's x axis (a linear luminaire lies along its y axis, centred on the
 * component; the lamps of an electrolier stand on a ring about its x axis). It lights at its catalogue output, switched by its operation (always, at dusk, on presence) and dimmed by
 * the channel it follows (0 to 1), and nothing else sets how much light it gives. Light's stage dresser places it.
 */
UCLASS(ClassGroup = (Musee))
class MUSEELIGHT_API ULightLuminaireComponent final : public USceneComponent
{
	GENERATED_BODY()

public:
	ULightLuminaireComponent();

	/**
	 * Sets what the luminaire is and how it is run: its catalogue type, its operation, the channel it follows (the none
	 * key for none) and, for a linear type, the length laid.
	 */
	void Configure(const FMuseeKey& InType, ELightOperation InOperation, const FMuseeKey& InFollows, Musee::Meters InLength);

	[[nodiscard]] const FMuseeKey& Type() const { return LuminaireType; }
	[[nodiscard]] ELightOperation Operation() const { return LuminaireOperation; }
	[[nodiscard]] const FMuseeKey& Follows() const { return FollowedChannel; }
	[[nodiscard]] Musee::Meters Length() const { return Musee::Meters(LengthMeters); }

	/** The fraction of its catalogue output it gives now, 0 to 1: its switch times the channel it follows. */
	[[nodiscard]] double Level() const;

	/** The luminous flux it emits now. */
	[[nodiscard]] Musee::Lumens EmittedFlux() const;

	/** The engine light it drives (its first lamp's, for a luminaire of several lamps); null for a type Light doesn't light. */
	[[nodiscard]] ULocalLightComponent* Emitter() const { return EmitterLights.IsEmpty() ? nullptr : EmitterLights[0].Get(); }

	/** Every engine light it drives: one, or one per lamp of a luminaire of several lamps. */
	[[nodiscard]] const TArray<TObjectPtr<ULocalLightComponent>>& Emitters() const { return EmitterLights; }

	/** Takes a moment: switches a dusk luminaire, and looks for the visitor for a presence luminaire. */
	void FollowMoment(const FTimeMoment& Moment);

	/** Takes a value of the channel it follows. */
	void FollowChannel(float Value);

protected:
	virtual void OnRegister() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type Reason) override;

private:
	void MakeEmitters();
	void DestroyEmitters();
	void Apply();
	void HearChannel(const FMuseeChannel& Channel);
	bool SeesVisitor() const;

	/** The catalogue type. */
	UPROPERTY(EditAnywhere, Category = "Luminaire")
	FMuseeKey LuminaireType;

	UPROPERTY(EditAnywhere, Category = "Luminaire")
	ELightOperation LuminaireOperation = ELightOperation::Always;

	/** The channel whose value (0 to 1) dims it; the none key for none. */
	UPROPERTY(EditAnywhere, Category = "Luminaire")
	FMuseeKey FollowedChannel;

	/** The length laid, for a linear type. */
	UPROPERTY(EditAnywhere, Category = "Luminaire", meta = (Units = "Meters"))
	double LengthMeters = 0.0;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULocalLightComponent>> EmitterLights;

	bool bSwitchedOn = true;
	double ChannelLevel = 1.0;
	/** The world time, in seconds, at which the presence detector last saw the visitor. */
	TOptional<double> LastSeenSeconds;
	FDelegateHandle MomentHandle;
	FDelegateHandle ChannelHandle;
};
