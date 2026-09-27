#pragma once

#include "CoreMinimal.h"

#include "TimeRecords.generated.h"

// The records of Data/Time as the record reader reads them, field for field. They are read once into the core's own
// types (TimeDesign.h); nothing else sees them. Notes and sources are kept so the reader accepts the files whole.

USTRUCT()
struct FTimeSourceRecord
{
	GENERATED_BODY()

	UPROPERTY() FString Id;
	UPROPERTY() FString Values;
	UPROPERTY() FString Source;
	UPROPERTY() FString Title;
	UPROPERTY() FString Url;
	UPROPERTY() FString Accessed;
};

USTRUCT()
struct FTimeZoneRecord
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired)) double StandardOffsetHours = 0.0;
	/** `none`, `united-states` or `european` (ZoneRule.h). */
	UPROPERTY(meta = (MuseeRequired)) FString DaylightSaving;
};

USTRUCT()
struct FTimeCityRecord
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired)) FString Key;
	UPROPERTY() FString Name;
	UPROPERTY(meta = (MuseeRequired)) double LatitudeDegrees = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double LongitudeDegrees = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double ElevationMeters = 0.0;
	UPROPERTY(meta = (MuseeRequired)) FTimeZoneRecord Zone;
	UPROPERTY(meta = (MuseeRequired)) FString ClimateNormals;
	UPROPERTY() TArray<FTimeSourceRecord> Sources;
};

USTRUCT()
struct FTimeCitiesRecord
{
	GENERATED_BODY()

	UPROPERTY() FString Notes;
	UPROPERTY(meta = (MuseeRequired)) TArray<FTimeCityRecord> Cities;
};

USTRUCT()
struct FTimeDialRecord
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired)) double LatitudeDegrees = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double CityWithinDegrees = 0.0;
	UPROPERTY() FString Decision;
	UPROPERTY() FString Notes;
};

USTRUCT()
struct FTimeIdealisedSunRecord
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired)) double NodeHeightMeters = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double HourRingRadiusMeters = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double FirstHour = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double LastHour = 0.0;
	UPROPERTY() FString Notes;
	UPROPERTY() TArray<FTimeSourceRecord> Sources;
};

USTRUCT()
struct FTimeClockRecord
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired)) double Rate = 1.0;
	UPROPERTY(meta = (MuseeRequired)) TArray<double> HourKeyStops;
	UPROPERTY() FString Notes;
};

/** Data/Time/Museum.json: the museum's own decisions about time. */
USTRUCT()
struct FTimeMuseumRecord
{
	GENERATED_BODY()

	UPROPERTY() FString Notes;
	UPROPERTY(meta = (MuseeRequired)) FString DefaultCity;
	UPROPERTY() FString DefaultCityDecision;
	UPROPERTY(meta = (MuseeRequired)) FTimeDialRecord Dial;
	/** `idealised` or `ephemeris` (Decision D-003). */
	UPROPERTY(meta = (MuseeRequired)) FString SunModel;
	UPROPERTY() FString SunModelDecision;
	UPROPERTY(meta = (MuseeRequired)) FTimeIdealisedSunRecord IdealisedSun;
	/** `astronomical`, `solar-terms` or `meteorological` (Season.h). */
	UPROPERTY(meta = (MuseeRequired)) FString SeasonReckoning;
	UPROPERTY() FString SeasonNotes;
	UPROPERTY(meta = (MuseeRequired)) FTimeClockRecord Clock;
};

USTRUCT()
struct FTimeWeatherPlaceRecord
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired)) FString Key;
	UPROPERTY() FString City;
	UPROPERTY(meta = (MuseeRequired)) bool FollowsMuseumCity = false;
	UPROPERTY() FString Notes;
};

USTRUCT()
struct FTimeLiveRecord
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired)) double ValidMinutes = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double RefreshMinutes = 0.0;
	UPROPERTY(meta = (MuseeRequired)) FString Endpoint;
	UPROPERTY() FString Notes;
};

USTRUCT()
struct FTimeSmoothingRecord
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired)) double TemperatureSeconds = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double CloudCoverSeconds = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double PrecipitationSeconds = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double WindSeconds = 0.0;
	UPROPERTY() FString Notes;
};

USTRUCT()
struct FTimeHistoryRecord
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired)) TArray<double> RainMemoryHours;
	UPROPERTY(meta = (MuseeRequired)) double SnowMeltMillimetersPerDayCelsius = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double LongestStepSeconds = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double RespinAfterMinutes = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double SpinUpDays = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double SpinUpStepMinutes = 0.0;
	UPROPERTY() FString Notes;
};

USTRUCT()
struct FTimeGeneratorRecord
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired)) TArray<double> MaximumSpreadCelsius;
	UPROPERTY(meta = (MuseeRequired)) TArray<double> MinimumSpreadCelsius;
	UPROPERTY() FString SpreadNotes;
	UPROPERTY() FTimeSourceRecord SpreadSource;
	UPROPERTY(meta = (MuseeRequired)) double AnomalyMemoryDays = 0.0;
	UPROPERTY() FString AnomalyNotes;
	UPROPERTY(meta = (MuseeRequired)) double WetDayCloudCover = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double CloudCoverPrecision = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double RainingCloudCover = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double RainCloudLeadHours = 0.0;
	UPROPERTY() FString CloudNotes;
	UPROPERTY(meta = (MuseeRequired)) double RainMeanHours = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double RainShortestHours = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double RainLongestHours = 0.0;
	UPROPERTY() FString RainNotes;
	UPROPERTY(meta = (MuseeRequired)) double SnowToRainBandCelsius = 0.0;
	UPROPERTY() FString SnowNotes;
	UPROPERTY(meta = (MuseeRequired)) double WindDirectionSpreadDegrees = 0.0;
	UPROPERTY() FString WindNotes;
};

USTRUCT()
struct FTimeSurfaceRecord
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired)) double WetFilmMillimeters = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double FullSnowCoverMeters = 0.0;
	UPROPERTY() FString Notes;
	UPROPERTY() TArray<FTimeSourceRecord> Sources;
};

/** Data/Time/Weather.json: how the museum keeps its weather. */
USTRUCT()
struct FTimeWeatherRecord
{
	GENERATED_BODY()

	UPROPERTY() FString Notes;
	UPROPERTY(meta = (MuseeRequired)) TArray<FTimeWeatherPlaceRecord> Places;
	UPROPERTY(meta = (MuseeRequired)) FTimeLiveRecord Live;
	UPROPERTY(meta = (MuseeRequired)) FTimeSmoothingRecord Smoothing;
	UPROPERTY(meta = (MuseeRequired)) FTimeHistoryRecord History;
	UPROPERTY(meta = (MuseeRequired)) FTimeGeneratorRecord Generator;
	UPROPERTY(meta = (MuseeRequired)) FTimeSurfaceRecord Surface;
};

USTRUCT()
struct FTimeClimateMonthRecord
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired)) int32 Month = 0;
	UPROPERTY(meta = (MuseeRequired)) double MeanMaximumCelsius = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double MeanMinimumCelsius = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double PrecipitationMillimeters = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double WetDays = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double SunshineHours = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double WindSpeedMetersPerSecond = 0.0;
	UPROPERTY(meta = (MuseeRequired)) double WindFromDegrees = 0.0;
	/** Counts the source publishes for some places only; a count is never negative, so -1 marks one not published. */
	UPROPERTY() double SnowDays = -1.0;
	UPROPERTY() double FrostDays = -1.0;
};

/** Data/Time/ClimateNormals/<place>.json. */
USTRUCT()
struct FTimeClimateNormalsRecord
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired)) FString Place;
	UPROPERTY() FString Station;
	UPROPERTY() FString Period;
	UPROPERTY() FString Notes;
	UPROPERTY(meta = (MuseeRequired)) double WetDayThresholdMillimeters = 0.0;
	UPROPERTY(meta = (MuseeRequired)) TArray<FTimeClimateMonthRecord> Months;
	UPROPERTY() TArray<FTimeSourceRecord> Sources;
};

USTRUCT()
struct FTimeSolarTermRecord
{
	GENERATED_BODY()

	UPROPERTY(meta = (MuseeRequired)) FString Key;
	UPROPERTY(meta = (MuseeRequired)) int32 Index = 0;
	UPROPERTY(meta = (MuseeRequired)) double StartLongitudeDegrees = 0.0;
	UPROPERTY() FString Hanzi;
	UPROPERTY() FString Pinyin;
	UPROPERTY() FString English;
};

/** Data/Time/SolarTerms.json. */
USTRUCT()
struct FTimeSolarTermsRecord
{
	GENERATED_BODY()

	UPROPERTY() FString Notes;
	UPROPERTY(meta = (MuseeRequired)) TArray<FTimeSolarTermRecord> Terms;
};
