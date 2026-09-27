#pragma once

#include "CoreMinimal.h"

/**
 * The museum Moment as shaders see it: the material parameter collection MPC_Moment and its scalar parameters (the
 * stage contract). Matter's make step creates the collection with exactly these names; Time is its only writer and sets
 * every parameter at each new moment; Matter's shaders read them.
 */
namespace Musee::Time::MomentParameters
{
	/** The collection's object path. */
	inline constexpr const TCHAR* CollectionPath = TEXT("/Game/Generated/Time/MPC_Moment.MPC_Moment");

	/** The museum sun's elevation, in degrees (negative below the horizon). */
	inline constexpr const TCHAR* SunElevationDeg = TEXT("SunElevationDeg");
	/** The museum sun's bearing from true north, in degrees. */
	inline constexpr const TCHAR* SunAzimuthDeg = TEXT("SunAzimuthDeg");
	/** Daylight, 0 to 1 (FTimeMoment::Daylight). */
	inline constexpr const TCHAR* Daylight = TEXT("Daylight");
	/** The museum's rain, in millimetres an hour. */
	inline constexpr const TCHAR* RainRateMmH = TEXT("RainRateMmH");
	/** The museum's surface wetness, 0 to 1. */
	inline constexpr const TCHAR* Wetness = TEXT("Wetness");
	/** The museum's snow cover, 0 to 1. */
	inline constexpr const TCHAR* SnowCover = TEXT("SnowCover");
	/** The museum's air temperature, in degrees Celsius. */
	inline constexpr const TCHAR* TemperatureC = TEXT("TemperatureC");
	/** The museum's wind at 10 m, in metres a second. */
	inline constexpr const TCHAR* WindSpeedMS = TEXT("WindSpeedMS");
	/** The solar term in force, 0 to 23. */
	inline constexpr const TCHAR* SolarTerm = TEXT("SolarTerm");
	/** Surface wetness under Giverny's weather (the Salon's and the oval's glazing, Decision D-005). */
	inline constexpr const TCHAR* GivernyWetness = TEXT("GivernyWetness");
	/** Surface wetness under London's weather (Albion's glass). */
	inline constexpr const TCHAR* LondonWetness = TEXT("LondonWetness");

	/** Every parameter, in the order above. */
	inline constexpr const TCHAR* All[] = {
		SunElevationDeg, SunAzimuthDeg, Daylight, RainRateMmH, Wetness, SnowCover, TemperatureC, WindSpeedMS, SolarTerm,
		GivernyWetness, LondonWetness,
	};
}
