#include "Keeper/TimeKeeper.h"

#include "Almanac/CelestialAngles.h"
#include "Calendar/Season.h"
#include "Calendar/SolarTerms.h"
#include "MuseeKey.h"

namespace Musee::Time
{
	namespace
	{
		/**
		 * Civil twilight ends when the sun's centre is 6 degrees below the horizon: the darkest sky by which outdoor
		 * objects can still be told apart without light (US Naval Observatory, "Rise, Set, and Twilight Definitions").
		 */
		constexpr double CivilTwilightDegrees = -6.0;

		/** The root of the weather generators' seeds: each city's generator draws from its own `weather.<city>`. */
		const TCHAR* const WeatherSeedRoot = TEXT("weather");

		FName SeasonName(ESeason Season)
		{
			switch (Season)
			{
			case ESeason::Spring: return TEXT("spring");
			case ESeason::Summer: return TEXT("summer");
			case ESeason::Autumn: return TEXT("autumn");
			default: return TEXT("winter");
			}
		}

		/** Daylight from the sun's elevation: full while it is up, falling through civil twilight to none. */
		double DaylightAt(double ElevationDegrees, bool bUp)
		{
			if (bUp) { return 1.0; }
			return FMath::Clamp((ElevationDegrees - CivilTwilightDegrees) / -CivilTwilightDegrees, 0.0, 1.0);
		}

		/**
		 * The idealised sun below the horizon: it keeps its bearing's pace of 15 degrees an hour and sinks at the same
		 * pace after its last hour (and rises so before its first), so twilight follows it as the design's day ends.
		 */
		double IdealisedDepressionDegrees(const FIdealisedSun& Sun, double CivilHour)
		{
			const double After = CivilHour > Sun.LastHour ? CivilHour - Sun.LastHour : CivilHour + HoursPerDay - Sun.LastHour;
			const double Before = CivilHour < Sun.FirstHour ? Sun.FirstHour - CivilHour : Sun.FirstHour + HoursPerDay - CivilHour;
			return -DegreesPerHour * FMath::Min(After, Before);
		}
	}

	FTimeKeeper::FTimeKeeper(const FTimeDesign& InDesign, FName InCity)
		: Design(InDesign)
		, CityKey(InDesign.MuseumCities.Contains(InCity) ? InCity : InDesign.DefaultCity)
		, MuseumClock(InDesign.Cities.FindChecked(CityKey).Zone)
		, Places(InDesign.Weather)
	{
		KeepPlaces();
	}

	const FCity& FTimeKeeper::MuseumCity() const
	{
		return Design.Cities.FindChecked(CityKey);
	}

	FWeatherPlace FTimeKeeper::PlaceIn(FName City) const
	{
		FWeatherPlace Place;
		Place.Observer = Design.Cities.FindChecked(City).Observer;
		Place.Normals = Design.Normals.FindChecked(Design.NormalsOfCity.FindChecked(City));
		Place.Seed = static_cast<uint32>(Musee::Seed(FMuseeKey(WeatherSeedRoot).Child(City.ToString())));
		return Place;
	}

	void FTimeKeeper::KeepPlaces()
	{
		for (const FWeatherPlaceDesign& Place : Design.WeatherPlaces)
		{
			Places.SetPlace(Place.Key, PlaceIn(Place.bFollowsMuseumCity ? CityKey : Place.City));
		}
	}

	bool FTimeKeeper::SetCity(FName InCity, const FDateTime& RealUtc)
	{
		if (!Design.MuseumCities.Contains(InCity)) { return false; }
		if (InCity == CityKey) { return true; }
		CityKey = InCity;
		MuseumClock.SetZone(MuseumCity().Zone, RealUtc);
		for (const FWeatherPlaceDesign& Place : Design.WeatherPlaces)
		{
			if (Place.bFollowsMuseumCity) { Places.SetPlace(Place.Key, PlaceIn(CityKey)); }
		}
		return true;
	}

	void FTimeKeeper::SetHour(TOptional<double> CivilHour, const FDateTime& RealUtc)
	{
		if (!CivilHour.IsSet())
		{
			MuseumClock.SetLive();
			return;
		}
		MuseumClock.SetHour(CivilHour.GetValue(), RealUtc);
		MuseumClock.SetRate(Design.ClockRate, RealUtc);
	}

	TOptional<double> FTimeKeeper::NextHourStop(TConstArrayView<double> Stops, TOptional<double> Chosen)
	{
		for (const double Stop : Stops)
		{
			if (!Chosen.IsSet() || Stop > Chosen.GetValue()) { return Stop; }
		}
		return {};
	}

	TArray<FName> FTimeKeeper::FeedPlaces() const
	{
		TArray<FName> Result;
		for (const FWeatherPlaceDesign& Place : Design.WeatherPlaces) { Result.Add(Place.Key); }
		return Result;
	}

	TArray<FObserver> FTimeKeeper::FeedObservers() const
	{
		TArray<FObserver> Result;
		for (const FWeatherPlaceDesign& Place : Design.WeatherPlaces)
		{
			Result.Add(Design.Cities.FindChecked(Place.bFollowsMuseumCity ? CityKey : Place.City).Observer);
		}
		return Result;
	}

	void FTimeKeeper::Observe(FName Place, const FWeatherObservation& Observation)
	{
		Places.Observe(Place, Observation);
	}

	FTimeMoment FTimeKeeper::MomentAt(const FDateTime& RealUtc)
	{
		FTimeMoment Moment;
		Moment.Utc = MuseumClock.MuseumUtc(RealUtc);
		Moment.Civil = MuseumClock.MuseumCivil(RealUtc);
		Moment.CivilHour = Moment.Civil.GetTimeOfDay().GetTotalHours();
		Moment.bLive = MuseumClock.IsLive();
		Moment.City = CityKey;
		FillSky(Moment, FInstant::FromUtc(Moment.Utc));
		Places.Advance(Moment.Utc);
		FillWeather(Moment);
		return Moment;
	}

	void FTimeKeeper::FillSky(FTimeMoment& Moment, const FInstant& Instant) const
	{
		const FObserver& Observer = MuseumCity().Observer;
		const FSunPosition RealSun = Sun(Instant, Observer);
		const FMuseumSun Shown = MuseumSun(Design.SunModel, Design.IdealisedSun, Moment.CivilHour, RealSun);
		const bool bIdealised = Design.SunModel == ESunModel::Idealised;
		Moment.SunModel = bIdealised ? TEXT("idealised") : TEXT("ephemeris");
		Moment.bSunUp = Shown.bUp;
		Moment.SunAzimuthDegrees = Shown.AzimuthDegrees;
		Moment.SunElevationDegrees = bIdealised && !Shown.bUp
			? IdealisedDepressionDegrees(Design.IdealisedSun, Moment.CivilHour) : Shown.AltitudeDegrees;
		FHorizontalPlace SunPlace;
		SunPlace.AzimuthDegrees = Moment.SunAzimuthDegrees;
		SunPlace.AltitudeDegrees = Moment.SunElevationDegrees;
		Moment.SunDirection = DirectionFromHorizontal(SunPlace);
		Moment.SunDistanceAstronomicalUnits = RealSun.Coordinates.DistanceAu();
		Moment.Daylight = DaylightAt(Moment.SunElevationDegrees, Moment.bSunUp);

		const FMoonPosition RealMoon = Moon(Instant, Observer);
		Moment.MoonDirection = RealMoon.Place.Direction;
		Moment.MoonAzimuthDegrees = RealMoon.Place.Apparent.AzimuthDegrees;
		Moment.MoonElevationDegrees = RealMoon.Place.Apparent.AltitudeDegrees;
		Moment.MoonIlluminatedFraction = RealMoon.Phase.IlluminatedFraction;
		Moment.MoonPhaseAngleDegrees = RealMoon.Phase.PhaseAngleDegrees;
		Moment.MoonBrightLimbDegrees = RealMoon.Phase.BrightLimbAngleDegrees;
		Moment.MoonAgeDays = RealMoon.AgeDays;
		Moment.MoonDistanceKilometers = RealMoon.Coordinates.DistanceKilometers;
		Moment.StarFieldOrientation = StarFieldOrientation(Instant, Observer);

		const double SolarLongitude = RealSun.Coordinates.ApparentLongitudeDegrees;
		const FSolarTermPlace Term = SolarTermAt(SolarLongitude);
		Moment.SolarTerm = Term.Index;
		Moment.SolarTermKey = Design.SolarTermKeys.IsValidIndex(Term.Index) ? Design.SolarTermKeys[Term.Index] : NAME_None;
		Moment.SolarTermProgress = Term.Progress;
		Moment.Season = SeasonName(SeasonAt(Design.SeasonReckoning, SolarLongitude, Moment.Civil, Observer.LatitudeDegrees).Season);
	}

	void FTimeKeeper::FillWeather(FTimeMoment& Moment) const
	{
		for (const FWeatherPlaceDesign& Place : Design.WeatherPlaces)
		{
			const FWeatherState* State = Places.Weather(Place.Key);
			const FPrecipitationHistory* History = Places.History(Place.Key);
			if (!State || !History) { continue; }
			FTimeWeather& Weather = Moment.Weather.AddDefaulted_GetRef();
			Weather.Place = Place.Key;
			Weather.bLive = Places.IsLive(Place.Key);
			Weather.AirTemperatureCelsius = State->AirTemperatureCelsius;
			Weather.CloudCover = State->CloudCover;
			Weather.RainMillimetersPerHour = State->RainMillimetersPerHour;
			Weather.SnowfallMillimetersPerHour = State->SnowfallMillimetersPerHour;
			Weather.WindSpeedMetersPerSecond = State->WindSpeedMetersPerSecond;
			Weather.WindFromDegrees = State->WindFromDegrees;
			// A wet surface is read from the record with the shortest memory: water that fell within the hour or so.
			int32 Shortest = 0;
			for (int32 Record = 1; Record < History->RainRecordCount(); ++Record)
			{
				if (History->RainMemoryHours(Record) < History->RainMemoryHours(Shortest)) { Shortest = Record; }
			}
			const double Film = History->RainRecordCount() > 0 ? History->RainRecordMillimeters(Shortest) : 0.0;
			Weather.Wetness = FMath::Clamp(Film / Design.WetFilmMillimeters, 0.0, 1.0);
			Weather.SnowDepthMeters = History->SnowDepthMeters();
			Weather.SnowCover = FMath::Clamp(Weather.SnowDepthMeters / Design.FullSnowCoverMeters, 0.0, 1.0);
		}
	}
}
