#pragma once

#include "CoreMinimal.h"
#include "Clock/MuseumClock.h"
#include "Design/TimeDesign.h"
#include "TimeMoment.h"
#include "Weather/WeatherPlaces.h"

namespace Musee::Time
{
	/**
	 * Keeps the museum's time: its city, its clock and the weather at each of its places, and makes the moment of any
	 * real instant from them. It reads nothing but what it is given, so it runs the same in a world, a test or a check.
	 */
	class FTimeKeeper
	{
	public:
		/** Keeps time in a city the museum may stand in (the design's default if the city isn't one). */
		FTimeKeeper(const FTimeDesign& InDesign, FName InCity);

		FName City() const { return CityKey; }
		const FMuseumClock& Clock() const { return MuseumClock; }

		/** Moves the museum to another of its cities; false, changing nothing, for a city it may not stand in. */
		bool SetCity(FName InCity, const FDateTime& RealUtc);

		/** Stands the clock at a civil hour, or sets it live. */
		void SetHour(TOptional<double> CivilHour, const FDateTime& RealUtc);

		/** The places the live feed is asked about, and where each stands, in one order. */
		TArray<FName> FeedPlaces() const;
		TArray<FObserver> FeedObservers() const;

		/** A live reading for a place. */
		void Observe(FName Place, const FWeatherObservation& Observation);

		/** The moment at a real instant. The weather is brought to the museum's instant, so instants go forward. */
		FTimeMoment MomentAt(const FDateTime& RealUtc);

		/** The hour the hour key goes to from a clock set to Chosen (unset while live): the next later stop, or live. */
		static TOptional<double> NextHourStop(TConstArrayView<double> Stops, TOptional<double> Chosen);

	private:
		const FCity& MuseumCity() const;
		FWeatherPlace PlaceIn(FName City) const;
		void KeepPlaces();
		void FillSky(FTimeMoment& Moment, const FInstant& Instant) const;
		void FillWeather(FTimeMoment& Moment) const;

		const FTimeDesign& Design;
		FName CityKey;
		FMuseumClock MuseumClock;
		FWeatherPlaces Places;
	};
}
