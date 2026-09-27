#include "Feed/LiveFeed.h"

#include "Almanac/AstronomicalTime.h"
#include "Feed/FeedRecords.h"
#include "Misc/Paths.h"
#include "MuseeRecords.h"

namespace Musee::Time::LiveFeed
{
	namespace
	{
		/** The quantities asked for, in Open-Meteo's names. */
		const TCHAR* const Quantities = TEXT("temperature_2m,cloud_cover,rain,showers,snowfall,wind_speed_10m,wind_direction_10m,snow_depth");

		/** Cloud cover comes in percent of the sky. */
		constexpr double PercentPerFraction = 100.0;

		/**
		 * Open-Meteo gives snowfall as a depth of fresh snow in centimetres, made from the water equivalent by a factor
		 * of 7: "for the water equivalent in millimetre, divide by 7; 7 cm snow = 10 mm precipitation water equivalent"
		 * (https://open-meteo.com/en/docs, hourly variables, snowfall).
		 */
		constexpr double WaterMillimetersPerSnowCentimeter = 10.0 / 7.0;

		/** The units every reading must be in: the feed's own spelling of what the request asked for. */
		bool UnitsAreAsAsked(const FTimeFeedUnits& Units, FString& OutWhich)
		{
			const TPair<const FString*, const TCHAR*> Expected[] = {
				{ &Units.Time, TEXT("unixtime") }, { &Units.Interval, TEXT("seconds") },
				{ &Units.Temperature_2m, TEXT("°C") }, { &Units.Cloud_Cover, TEXT("%") },
				{ &Units.Rain, TEXT("mm") }, { &Units.Showers, TEXT("mm") }, { &Units.Snowfall, TEXT("cm") },
				{ &Units.Wind_Speed_10m, TEXT("m/s") }, { &Units.Wind_Direction_10m, TEXT("°") }, { &Units.Snow_Depth, TEXT("m") },
			};
			for (const TPair<const FString*, const TCHAR*>& Unit : Expected)
			{
				if (!Unit.Key->Equals(Unit.Value, ESearchCase::CaseSensitive))
				{
					OutWhich = FString::Printf(TEXT("'%s' where '%s' was asked for"), **Unit.Key, Unit.Value);
					return false;
				}
			}
			return true;
		}

		/** The feed's current conditions as an observation: precipitation sums over the interval become rates. */
		FWeatherObservation Observation(const FTimeFeedCurrent& Current)
		{
			const double HoursPerInterval = static_cast<double>(Current.Interval) / SecondsPerHour;
			FWeatherObservation Result;
			Result.Utc = FDateTime::FromUnixTimestamp(Current.Time);
			Result.State.AirTemperatureCelsius = Current.Temperature_2m;
			Result.State.CloudCover = FMath::Clamp(Current.Cloud_Cover / PercentPerFraction, 0.0, 1.0);
			Result.State.RainMillimetersPerHour = (Current.Rain + Current.Showers) / HoursPerInterval;
			Result.State.SnowfallMillimetersPerHour = Current.Snowfall * WaterMillimetersPerSnowCentimeter / HoursPerInterval;
			Result.State.WindSpeedMetersPerSecond = Current.Wind_Speed_10m;
			Result.State.WindFromDegrees = Current.Wind_Direction_10m;
			if (Current.Snow_Depth >= 0.0) { Result.SnowDepthMeters = Current.Snow_Depth; }
			return Result;
		}
	}

	FString RequestUrl(const FString& Endpoint, TConstArrayView<FObserver> Places)
	{
		TArray<FString> Latitudes, Longitudes, Elevations;
		for (const FObserver& Place : Places)
		{
			Latitudes.Add(FString::Printf(TEXT("%.5f"), Place.LatitudeDegrees));
			Longitudes.Add(FString::Printf(TEXT("%.5f"), Place.LongitudeDegrees));
			Elevations.Add(FString::Printf(TEXT("%.0f"), Place.ElevationMeters));
		}
		return FString::Printf(TEXT("%s?latitude=%s&longitude=%s&elevation=%s&current=%s&wind_speed_unit=ms&timeformat=unixtime&timezone=GMT"),
			*Endpoint, *FString::Join(Latitudes, TEXT(",")), *FString::Join(Longitudes, TEXT(",")),
			*FString::Join(Elevations, TEXT(",")), Quantities);
	}

	FString CachePath(FName City)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Musee/Time") / FString::Printf(TEXT("LiveWeather.%s.json"), *City.ToString()));
	}

	FString AsRecordFile(const FString& Answer)
	{
		return FString::Printf(TEXT("{ \"$records\": %s }"), *Answer.TrimStartAndEnd());
	}

	bool Read(const FString& Path, int32 PlaceCount, TArray<FWeatherObservation>& OutObservations, FString& OutError)
	{
		TValueOrError<TArray<FTimeFeedPlace>, FString> Result = ReadRecords<FTimeFeedPlace>(Path);
		if (Result.HasError())
		{
			OutError = Result.StealError();
			return false;
		}
		const TArray<FTimeFeedPlace>& Places = Result.GetValue();
		if (Places.Num() != PlaceCount)
		{
			OutError = FString::Printf(TEXT("%s: the feed answered for %d places, not %d."), *Path, Places.Num(), PlaceCount);
			return false;
		}
		OutObservations.Reset();
		for (const FTimeFeedPlace& Place : Places)
		{
			FString Which;
			if (!UnitsAreAsAsked(Place.Current_Units, Which) || Place.Current.Interval <= 0)
			{
				OutError = FString::Printf(TEXT("%s: the feed answered in %s."), *Path, Which.IsEmpty() ? TEXT("no interval") : *Which);
				return false;
			}
			OutObservations.Add(Observation(Place.Current));
		}
		return true;
	}
}
