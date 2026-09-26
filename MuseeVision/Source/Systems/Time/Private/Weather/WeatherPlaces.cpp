#include "Weather/WeatherPlaces.h"

#include "Almanac/AstronomicalTime.h"

namespace Musee::Time
{
	FWeatherPlaces::FWeatherPlaces(const FWeatherDesign& InDesign)
		: Design(InDesign)
	{
	}

	void FWeatherPlaces::SetPlace(FName Key, const FWeatherPlace& Place)
	{
		FKeptPlace Kept;
		Kept.Generator = MakeUnique<FWeatherGenerator>(Place, Design.Generator);
		Kept.History = MakeUnique<FPrecipitationHistory>(Design.History);
		Places.Add(Key, MoveTemp(Kept));
	}

	void FWeatherPlaces::RemovePlace(FName Key)
	{
		Places.Remove(Key);
	}

	bool FWeatherPlaces::IsCurrent(const FWeatherObservation& Observation, const FDateTime& Utc) const
	{
		return FMath::Abs((Utc - Observation.Utc).GetTotalMinutes()) <= Design.LiveValidMinutes;
	}

	void FWeatherPlaces::Observe(FName Key, const FWeatherObservation& Observation)
	{
		FKeptPlace* Place = Places.Find(Key);
		if (!Place) { return; }
		Place->Live = Observation;
		if (Observation.SnowDepthMeters.IsSet() && Place->LastUtc.IsSet() && IsCurrent(Observation, Place->LastUtc.GetValue()))
		{
			Place->History->ObserveSnowDepth(Observation.SnowDepthMeters.GetValue());
		}
	}

	FWeatherState FWeatherPlaces::TargetAt(const FKeptPlace& Place, const FDateTime& Utc, bool& bOutLive) const
	{
		bOutLive = Place.Live.IsSet() && IsCurrent(Place.Live.GetValue(), Utc);
		return bOutLive ? Place.Live->State : Place.Generator->StateAt(Utc);
	}

	void FWeatherPlaces::Respin(FKeptPlace& Place, const FDateTime& Utc) const
	{
		// The ground's history is rebuilt from what the weather was over the preceding weeks, each step taking the
		// weather at its middle.
		Place.History->Reset();
		const FDateTime From = Utc - FTimespan::FromDays(Design.SpinUpDays);
		const FTimespan StepSpan = Design.SpinUpStepMinutes > 0.0 ? FTimespan::FromMinutes(Design.SpinUpStepMinutes) : Utc - From;
		bool bLive = false;
		for (FDateTime StepStart = From; StepStart < Utc; StepStart += StepSpan)
		{
			const double Seconds = FMath::Min(StepSpan, Utc - StepStart).GetTotalSeconds();
			Place.History->Advance(TargetAt(Place, StepStart + FTimespan::FromSeconds(Seconds / 2.0), bLive), Seconds);
		}
		Place.Smoother.Reset(TargetAt(Place, Utc, Place.bLive));
		if (Place.Live.IsSet() && Place.Live->SnowDepthMeters.IsSet() && IsCurrent(Place.Live.GetValue(), Utc))
		{
			Place.History->ObserveSnowDepth(Place.Live->SnowDepthMeters.GetValue());
		}
	}

	void FWeatherPlaces::Step(FKeptPlace& Place, const FDateTime& Utc) const
	{
		FDateTime Reached = Place.LastUtc.GetValue();
		const FTimespan LongestStep = Design.LongestStepSeconds > 0.0 ? FTimespan::FromSeconds(Design.LongestStepSeconds) : Utc - Reached;
		while (Reached < Utc)
		{
			const FDateTime Next = FMath::Min(Reached + LongestStep, Utc);
			const double Seconds = (Next - Reached).GetTotalSeconds();
			const FWeatherState& Shown = Place.Smoother.Advance(TargetAt(Place, Next, Place.bLive), Seconds, Design.Smoothing);
			Place.History->Advance(Shown, Seconds);
			Reached = Next;
		}
	}

	void FWeatherPlaces::Advance(const FDateTime& Utc)
	{
		const FTimespan RespinAfter = FTimespan::FromMinutes(Design.RespinAfterMinutes);
		for (TPair<FName, FKeptPlace>& Entry : Places)
		{
			FKeptPlace& Place = Entry.Value;
			const bool bJumped = !Place.LastUtc.IsSet() || Utc < Place.LastUtc.GetValue()
				|| Utc - Place.LastUtc.GetValue() > RespinAfter;
			if (bJumped)
			{
				Respin(Place, Utc);
			}
			else
			{
				Step(Place, Utc);
			}
			Place.LastUtc = Utc;
		}
	}

	const FWeatherState* FWeatherPlaces::Weather(FName Key) const
	{
		const FKeptPlace* Place = Places.Find(Key);
		return Place && Place->Smoother.IsStarted() ? &Place->Smoother.State() : nullptr;
	}

	const FPrecipitationHistory* FWeatherPlaces::History(FName Key) const
	{
		const FKeptPlace* Place = Places.Find(Key);
		return Place ? Place->History.Get() : nullptr;
	}

	bool FWeatherPlaces::IsLive(FName Key) const
	{
		const FKeptPlace* Place = Places.Find(Key);
		return Place && Place->bLive;
	}

	TArray<FName> FWeatherPlaces::Keys() const
	{
		TArray<FName> Result;
		Places.GetKeys(Result);
		return Result;
	}
}
