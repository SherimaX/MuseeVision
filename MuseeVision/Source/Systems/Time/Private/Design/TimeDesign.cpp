#include "Design/TimeDesign.h"

#include "Calendar/SolarTerms.h"
#include "Design/TimeRecords.h"
#include "MuseeRecords.h"
#include "TimeLog.h"

namespace Musee::Time
{
	namespace
	{
		const TCHAR* const MuseumFile = TEXT("Time/Museum.json");
		const TCHAR* const CitiesFile = TEXT("Time/Cities.json");
		const TCHAR* const WeatherFile = TEXT("Time/Weather.json");
		const TCHAR* const NormalsFolder = TEXT("Time/ClimateNormals");
		const TCHAR* const SolarTermsFile = TEXT("Time/SolarTerms.json");

		template <typename T>
		bool Read(const TCHAR* Path, T& Out, FString& OutError)
		{
			TValueOrError<T, FString> Result = ReadRecord<T>(Path);
			if (Result.HasError())
			{
				OutError = Result.StealError();
				return false;
			}
			Out = Result.StealValue();
			return true;
		}

		bool ParseDaylightSaving(const FString& Text, EDaylightSaving& Out)
		{
			if (Text == TEXT("none")) { Out = EDaylightSaving::None; return true; }
			if (Text == TEXT("united-states")) { Out = EDaylightSaving::UnitedStates; return true; }
			if (Text == TEXT("european")) { Out = EDaylightSaving::European; return true; }
			return false;
		}

		bool ParseSeasonReckoning(const FString& Text, ESeasonReckoning& Out)
		{
			if (Text == TEXT("astronomical")) { Out = ESeasonReckoning::Astronomical; return true; }
			if (Text == TEXT("solar-terms")) { Out = ESeasonReckoning::SolarTerms; return true; }
			if (Text == TEXT("meteorological")) { Out = ESeasonReckoning::Meteorological; return true; }
			return false;
		}

		bool TakeCities(const FTimeCitiesRecord& Record, const FTimeDialRecord& Dial, FTimeDesign& Design, FString& OutError)
		{
			for (const FTimeCityRecord& Entry : Record.Cities)
			{
				FCity City;
				City.Key = FName(*Entry.Key);
				City.Observer = FObserver::At(Entry.LatitudeDegrees, Entry.LongitudeDegrees, Entry.ElevationMeters);
				City.Zone.StandardOffsetHours = Entry.Zone.StandardOffsetHours;
				if (!ParseDaylightSaving(Entry.Zone.DaylightSaving, City.Zone.DaylightSaving))
				{
					OutError = FString::Printf(TEXT("%s: city '%s' has an unknown daylight-saving law '%s'."), CitiesFile, *Entry.Key, *Entry.Zone.DaylightSaving);
					return false;
				}
				Design.Cities.Add(City.Key, City);
				Design.NormalsOfCity.Add(City.Key, FName(*Entry.ClimateNormals));
				Design.CityNames.Add(City.Key, Entry.Name.IsEmpty() ? Entry.Key : Entry.Name);
				// Decision D-002: the museum may stand only where the dial, cut for one latitude, reads true.
				if (FMath::Abs(Entry.LatitudeDegrees - Dial.LatitudeDegrees) <= Dial.CityWithinDegrees)
				{
					Design.MuseumCities.Add(City.Key);
				}
			}
			return true;
		}

		bool TakeNormals(FTimeDesign& Design, FString& OutError)
		{
			TValueOrError<TArray<FTimeClimateNormalsRecord>, FString> Result = ReadRecords<FTimeClimateNormalsRecord>(NormalsFolder);
			if (Result.HasError())
			{
				OutError = Result.StealError();
				return false;
			}
			for (const FTimeClimateNormalsRecord& Record : Result.GetValue())
			{
				if (Record.Months.Num() != NormalsMonthCount)
				{
					OutError = FString::Printf(TEXT("%s: '%s' gives %d months, not 12."), NormalsFolder, *Record.Place, Record.Months.Num());
					return false;
				}
				FClimateNormals Normals;
				Normals.WetDayThresholdMillimeters = Record.WetDayThresholdMillimeters;
				for (const FTimeClimateMonthRecord& Month : Record.Months)
				{
					if (Month.Month < 1 || Month.Month > NormalsMonthCount)
					{
						OutError = FString::Printf(TEXT("%s: '%s' has a month numbered %d."), NormalsFolder, *Record.Place, Month.Month);
						return false;
					}
					FClimateMonth& Into = Normals.Months[Month.Month - 1];
					Into.MeanMaximumCelsius = Month.MeanMaximumCelsius;
					Into.MeanMinimumCelsius = Month.MeanMinimumCelsius;
					Into.PrecipitationMillimeters = Month.PrecipitationMillimeters;
					Into.WetDays = Month.WetDays;
					Into.SunshineHours = Month.SunshineHours;
					Into.WindSpeedMetersPerSecond = Month.WindSpeedMetersPerSecond;
					Into.WindFromDegrees = Month.WindFromDegrees;
					if (Month.SnowDays >= 0.0) { Into.SnowDays = Month.SnowDays; }
					if (Month.FrostDays >= 0.0) { Into.FrostDays = Month.FrostDays; }
				}
				Design.Normals.Add(FName(*Record.Place), Normals);
			}
			return true;
		}

		bool TakeMuseum(const FTimeMuseumRecord& Record, FTimeDesign& Design, FString& OutError)
		{
			Design.DefaultCity = FName(*Record.DefaultCity);
			if (!Design.MuseumCities.Contains(Design.DefaultCity))
			{
				OutError = FString::Printf(TEXT("%s: the default city '%s' is not a city the dial reads true for."), MuseumFile, *Record.DefaultCity);
				return false;
			}
			if (Record.SunModel == TEXT("idealised")) { Design.SunModel = ESunModel::Idealised; }
			else if (Record.SunModel == TEXT("ephemeris")) { Design.SunModel = ESunModel::Ephemeris; }
			else
			{
				OutError = FString::Printf(TEXT("%s: unknown sun model '%s'."), MuseumFile, *Record.SunModel);
				return false;
			}
			if (!ParseSeasonReckoning(Record.SeasonReckoning, Design.SeasonReckoning))
			{
				OutError = FString::Printf(TEXT("%s: unknown season reckoning '%s'."), MuseumFile, *Record.SeasonReckoning);
				return false;
			}
			Design.IdealisedSun.NodeHeightMeters = Record.IdealisedSun.NodeHeightMeters;
			Design.IdealisedSun.HourRingRadiusMeters = Record.IdealisedSun.HourRingRadiusMeters;
			Design.IdealisedSun.FirstHour = Record.IdealisedSun.FirstHour;
			Design.IdealisedSun.LastHour = Record.IdealisedSun.LastHour;
			Design.ClockRate = Record.Clock.Rate;
			Design.HourKeyStops = Record.Clock.HourKeyStops;
			return true;
		}

		void TakeGenerator(const FTimeGeneratorRecord& Record, FWeatherGeneratorDesign& Generator)
		{
			for (int32 Month = 0; Month < NormalsMonthCount; ++Month)
			{
				Generator.MaximumSpreadCelsius[Month] = Record.MaximumSpreadCelsius[Month];
				Generator.MinimumSpreadCelsius[Month] = Record.MinimumSpreadCelsius[Month];
			}
			Generator.AnomalyMemoryDays = Record.AnomalyMemoryDays;
			Generator.WetDayCloudCover = Record.WetDayCloudCover;
			Generator.CloudCoverPrecision = Record.CloudCoverPrecision;
			Generator.RainingCloudCover = Record.RainingCloudCover;
			Generator.RainCloudLeadHours = Record.RainCloudLeadHours;
			Generator.RainMeanHours = Record.RainMeanHours;
			Generator.RainShortestHours = Record.RainShortestHours;
			Generator.RainLongestHours = Record.RainLongestHours;
			Generator.SnowToRainBandCelsius = Record.SnowToRainBandCelsius;
			Generator.WindDirectionSpreadDegrees = Record.WindDirectionSpreadDegrees;
		}

		bool TakeWeather(const FTimeWeatherRecord& Record, FTimeDesign& Design, FString& OutError)
		{
			if (Record.Generator.MaximumSpreadCelsius.Num() != NormalsMonthCount || Record.Generator.MinimumSpreadCelsius.Num() != NormalsMonthCount)
			{
				OutError = FString::Printf(TEXT("%s: the generator's spreads need one value for each of 12 months."), WeatherFile);
				return false;
			}
			FWeatherDesign& Weather = Design.Weather;
			Weather.Smoothing = { Record.Smoothing.TemperatureSeconds, Record.Smoothing.CloudCoverSeconds,
				Record.Smoothing.PrecipitationSeconds, Record.Smoothing.WindSeconds };
			Weather.History.RainMemoryHours = Record.History.RainMemoryHours;
			Weather.History.SnowMeltMillimetersPerDayCelsius = Record.History.SnowMeltMillimetersPerDayCelsius;
			TakeGenerator(Record.Generator, Weather.Generator);
			Weather.LiveValidMinutes = Record.Live.ValidMinutes;
			Weather.LongestStepSeconds = Record.History.LongestStepSeconds;
			Weather.RespinAfterMinutes = Record.History.RespinAfterMinutes;
			Weather.SpinUpDays = Record.History.SpinUpDays;
			Weather.SpinUpStepMinutes = Record.History.SpinUpStepMinutes;
			Design.LiveEndpoint = Record.Live.Endpoint;
			Design.LiveRefreshMinutes = Record.Live.RefreshMinutes;
			Design.WetFilmMillimeters = Record.Surface.WetFilmMillimeters;
			Design.FullSnowCoverMeters = Record.Surface.FullSnowCoverMeters;
			for (const FTimeWeatherPlaceRecord& Place : Record.Places)
			{
				const FName City = Place.FollowsMuseumCity ? NAME_None : FName(*Place.City);
				if (!Place.FollowsMuseumCity && !Design.Cities.Contains(City))
				{
					OutError = FString::Printf(TEXT("%s: weather place '%s' stands in an unknown city '%s'."), WeatherFile, *Place.Key, *Place.City);
					return false;
				}
				Design.WeatherPlaces.Add({ FName(*Place.Key), City, Place.FollowsMuseumCity });
			}
			return true;
		}

		bool TakeSolarTerms(const FTimeSolarTermsRecord& Record, FTimeDesign& Design, FString& OutError)
		{
			Design.SolarTermKeys.SetNum(SolarTermCount);
			for (const FTimeSolarTermRecord& Term : Record.Terms)
			{
				if (Term.Index < 0 || Term.Index >= SolarTermCount)
				{
					OutError = FString::Printf(TEXT("%s: term '%s' has index %d."), SolarTermsFile, *Term.Key, Term.Index);
					return false;
				}
				Design.SolarTermKeys[Term.Index] = FName(*Term.Key);
			}
			return true;
		}
	}

	TValueOrError<FTimeDesign, FString> ReadDesign()
	{
		FTimeDesign Design;
		FTimeMuseumRecord Museum;
		FTimeCitiesRecord Cities;
		FTimeWeatherRecord Weather;
		FTimeSolarTermsRecord Terms;
		FString Error;
		const bool bRead = Read(MuseumFile, Museum, Error) && Read(CitiesFile, Cities, Error)
			&& Read(WeatherFile, Weather, Error) && Read(SolarTermsFile, Terms, Error)
			&& TakeCities(Cities, Museum.Dial, Design, Error) && TakeNormals(Design, Error)
			&& TakeMuseum(Museum, Design, Error) && TakeWeather(Weather, Design, Error)
			&& TakeSolarTerms(Terms, Design, Error);
		if (!bRead)
		{
			return MakeError(MoveTemp(Error));
		}
		for (const TPair<FName, FName>& City : Design.NormalsOfCity)
		{
			if (!Design.Normals.Contains(City.Value))
			{
				return MakeError(FString::Printf(TEXT("%s: no climate normals '%s' for city '%s'."), NormalsFolder, *City.Value.ToString(), *City.Key.ToString()));
			}
		}
		return MakeValue(MoveTemp(Design));
	}

	const FTimeDesign* Design()
	{
		static const TOptional<FTimeDesign> Kept = []() -> TOptional<FTimeDesign>
		{
			TValueOrError<FTimeDesign, FString> Result = ReadDesign();
			if (Result.HasError())
			{
				UE_LOG(LogMuseeTime, Error, TEXT("Time's records can't be read: %s"), *Result.GetError());
				return {};
			}
			return Result.StealValue();
		}();
		return Kept.GetPtrOrNull();
	}
}
