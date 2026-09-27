#pragma once

#include "CoreMinimal.h"
#include "Calendar/Season.h"
#include "Clock/City.h"
#include "Sun/SunModel.h"
#include "Templates/ValueOrError.h"
#include "Weather/ClimateNormals.h"
#include "Weather/WeatherPlaces.h"

/**
 * Everything Time's records declare, read once from Data/Time into the core's own types: the cities and their climates,
 * the museum's sun and clock, and how its weather is kept.
 */
namespace Musee::Time
{
	/** A place whose weather Time keeps (Decision D-005). */
	struct FWeatherPlaceDesign
	{
		FName Key;
		/** The city it stands in; unset for the place that follows the museum's city. */
		FName City;
		bool bFollowsMuseumCity = false;
	};

	struct FTimeDesign
	{
		TMap<FName, FCity> Cities;
		/** Each city's name as the visitor reads it. */
		TMap<FName, FString> CityNames;
		/** Climate normals by the place their record names, and which normals each city takes. */
		TMap<FName, FClimateNormals> Normals;
		TMap<FName, FName> NormalsOfCity;
		/** The cities the museum may stand in: those within the dial's tolerance of its latitude (Decision D-002). */
		TArray<FName> MuseumCities;
		FName DefaultCity;

		ESunModel SunModel = ESunModel::Idealised;
		FIdealisedSun IdealisedSun;
		ESeasonReckoning SeasonReckoning = ESeasonReckoning::SolarTerms;

		double ClockRate = 1.0;
		TArray<double> HourKeyStops;

		FWeatherDesign Weather;
		TArray<FWeatherPlaceDesign> WeatherPlaces;
		FString LiveEndpoint;
		double LiveRefreshMinutes = 0.0;
		/** The depression storage of an impervious surface, and the snow depth that covers open ground. */
		double WetFilmMillimeters = 0.0;
		double FullSnowCoverMeters = 0.0;

		/** The solar terms' keys, by index. */
		TArray<FName> SolarTermKeys;
	};

	/** Reads Data/Time. The error names the file and the field at fault. */
	TValueOrError<FTimeDesign, FString> ReadDesign();

	/** The design, read on first use and kept; null (with the reason logged once) if the records can't be read. */
	const FTimeDesign* Design();
}
