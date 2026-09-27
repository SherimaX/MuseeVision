#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Templates/PimplPtr.h"
#include "TimeMoment.h"

#include "TimeSubsystem.generated.h"

class UMaterialParameterCollection;

namespace Musee::Time
{
	class FTimeKeeper;
}

/**
 * The museum's clock, sky and weather in a world: Time's one public face at runtime. It keeps the moment (FTimeMoment)
 * and announces each new one through OnMoment, as the clock runs and whenever a setting changes; nobody polls. It
 * follows the settings `time.hour` (live, or a fixed civil hour) and `time.city` (the museum's city), fetches the live
 * weather for all of Time's places in one request and falls back on each place's seeded generator, and writes the
 * shader-visible moment into MPC_Moment (TimeMomentParameters.h).
 *
 * The settings and the commands `musee.time.advance-hour` (the hour key) and `musee.time.advance-city` (the city key)
 * are registered by the Time module.
 */
UCLASS()
class MUSEETIME_API UTimeSubsystem final : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	using FMomentEvent = TMulticastDelegate<void(const FTimeMoment&)>;

	/** The current moment. */
	[[nodiscard]] const FTimeMoment& Now() const { return Moment; }

	/** Announces each new moment. A listener that begins later reads Now() first. */
	FMomentEvent& OnMoment() { return MomentEvent; }

	/** Makes and announces the moment of this instant now, rather than at the next period. */
	void Refresh();

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableInEditor() const override { return true; }

private:
	void FollowSettings();
	void ApplyHour();
	void ApplyCity();
	void Announce();
	void WriteParameters() const;
	void RequestLiveWeather();
	void TakeLiveWeather(const FString& CachePath);

	TPimplPtr<Musee::Time::FTimeKeeper> Keeper;
	FTimeMoment Moment;
	FMomentEvent MomentEvent;
	FDelegateHandle HourHandle;
	FDelegateHandle CityHandle;
	double SecondsSinceMoment = 0.0;
	double SecondsSinceFetch = 0.0;
	bool bFetching = false;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollection> MomentCollection;
};
