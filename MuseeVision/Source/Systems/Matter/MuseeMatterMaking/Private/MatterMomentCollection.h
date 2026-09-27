#pragma once

#include "CoreMinimal.h"

class UMaterialParameterCollection;

/**
 * The museum Moment as the shaders see it: Time's material parameter collection, /Game/Generated/Time/MPC_Moment.
 *
 * Time owns what the values mean and sets them at runtime; Matter's make step creates the collection (the stage
 * contract), because Matter's masters read it. Its scalars are the contract's, in its order: the sun's elevation and
 * azimuth in degrees and the daylight fraction; the rain rate in mm per hour, the wetness and snow cover of a level,
 * exposed reference surface (0 to 1), the air temperature in degrees Celsius, the wind speed in metres per second and
 * the solar term (0 to 23); and, per weather place of D-005, Giverny's and London's wetness. Every value starts at
 * zero until Time sets it.
 *
 * The collection's identifiers (its StateId, which the shaders of every master reading it are keyed by, and each
 * parameter's id) are derived from the parameter names, so a make writes the same collection every time.
 */
namespace Musee::Matter
{
	namespace MomentParameter
	{
		inline constexpr const TCHAR* SunElevationDeg = TEXT("SunElevationDeg");
		inline constexpr const TCHAR* SunAzimuthDeg = TEXT("SunAzimuthDeg");
		inline constexpr const TCHAR* Daylight = TEXT("Daylight");
		inline constexpr const TCHAR* RainRateMmH = TEXT("RainRateMmH");
		inline constexpr const TCHAR* Wetness = TEXT("Wetness");
		inline constexpr const TCHAR* SnowCover = TEXT("SnowCover");
		inline constexpr const TCHAR* TemperatureC = TEXT("TemperatureC");
		inline constexpr const TCHAR* WindSpeedMS = TEXT("WindSpeedMS");
		inline constexpr const TCHAR* SolarTerm = TEXT("SolarTerm");
		inline constexpr const TCHAR* GivernyWetness = TEXT("GivernyWetness");
		inline constexpr const TCHAR* LondonWetness = TEXT("LondonWetness");
	}

	/** Every parameter of the Moment, in the contract's order. */
	TArray<FName> MomentParameterNames();

	/** Fills a new collection with the Moment's parameters and derives its identifiers from them. */
	void BuildMomentCollection(UMaterialParameterCollection& Collection);

	/** The collection's StateId for these names. */
	FGuid MomentStateId(const TArray<FName>& Names);
}
