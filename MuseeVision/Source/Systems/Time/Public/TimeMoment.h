#pragma once

#include "CoreMinimal.h"
#include "TimeWeather.h"

#include "TimeMoment.generated.h"

/**
 * One moment of the museum: its hour and date in its city, the sun it shows (the idealised sun of the approved design
 * by default, Decision D-003), the real Moon and stars, the solar term and season, and the weather at each of Time's
 * places. Time makes a new moment as the clock runs or a setting changes and announces it through
 * UTimeSubsystem::OnMoment; nobody polls. Directions are unit vectors in the museum's horizon frame: x east, y south,
 * z up, which a stage laid out on its plan shares.
 */
USTRUCT()
struct MUSEETIME_API FTimeMoment
{
	GENERATED_BODY()

	/** The museum's instant. */
	UPROPERTY()
	FDateTime Utc;

	/** The civil (wall-clock) time in the museum's city at that instant. */
	UPROPERTY()
	FDateTime Civil;

	/** The civil time of day, in hours from midnight, [0, 24). */
	UPROPERTY()
	double CivilHour = 0.0;

	/** Whether the clock is live (the real time) rather than set to an hour. */
	UPROPERTY()
	bool bLive = true;

	/** The museum's city (`beijing`, `new-york`). */
	UPROPERTY()
	FName City;

	/** Which sun shows: `idealised` or `ephemeris`. */
	UPROPERTY()
	FName SunModel;

	/** The unit vector towards the sun the museum shows. Below the horizon while the sun is down. */
	UPROPERTY()
	FVector SunDirection = FVector::UpVector;

	/** The sun's bearing, clockwise from true north. */
	UPROPERTY(meta = (Units = "Degrees"))
	double SunAzimuthDegrees = 0.0;

	/**
	 * The sun's elevation above the horizon: its apparent altitude for the real sun; for the idealised sun its constant
	 * altitude while up and, outside its hours, a depression that deepens 15 degrees an hour, so twilight falls after it.
	 */
	UPROPERTY(meta = (Units = "Degrees"))
	double SunElevationDegrees = 0.0;

	/** Whether the sun is up. */
	UPROPERTY()
	bool bSunUp = false;

	/** The Earth's distance from the real Sun, in astronomical units: the sun's light falls off as its square. */
	UPROPERTY()
	double SunDistanceAstronomicalUnits = 1.0;

	/**
	 * How much daylight there is, 0 to 1: 1 while the sun is up, falling through civil twilight to 0 when the sun's
	 * centre is 6 degrees below the horizon.
	 */
	UPROPERTY()
	double Daylight = 0.0;

	/** The unit vector towards the real Moon as seen. */
	UPROPERTY()
	FVector MoonDirection = FVector::UpVector;

	UPROPERTY(meta = (Units = "Degrees"))
	double MoonAzimuthDegrees = 0.0;

	UPROPERTY(meta = (Units = "Degrees"))
	double MoonElevationDegrees = 0.0;

	/** The lit fraction of the Moon's disc. */
	UPROPERTY()
	double MoonIlluminatedFraction = 0.0;

	/** The Sun-Moon-Earth angle: 0 at full Moon, 180 at new. */
	UPROPERTY(meta = (Units = "Degrees"))
	double MoonPhaseAngleDegrees = 0.0;

	/** The position angle of the lit limb's midpoint, from celestial north through east. */
	UPROPERTY(meta = (Units = "Degrees"))
	double MoonBrightLimbDegrees = 0.0;

	/** Days since the last new Moon. */
	UPROPERTY()
	double MoonAgeDays = 0.0;

	/** The Moon's distance from the Earth's centre. */
	UPROPERTY()
	double MoonDistanceKilometers = 0.0;

	/**
	 * Maps a star catalogue's frame (the equator and equinox of J2000.0, right-handed) into the museum's horizon frame
	 * (x east, y south, z up, left-handed like the engine), so it is orthonormal with a determinant of -1.
	 */
	UPROPERTY()
	FMatrix StarFieldOrientation = FMatrix::Identity;

	/** The solar term in force, 0 (Lichun) to 23 (Dahan), its key (`qiufen`) and how far through it, [0, 1). */
	UPROPERTY()
	int32 SolarTerm = 0;

	UPROPERTY()
	FName SolarTermKey;

	UPROPERTY()
	double SolarTermProgress = 0.0;

	/** The season by the museum's reckoning: `spring`, `summer`, `autumn` or `winter`. */
	UPROPERTY()
	FName Season;

	/** The weather at each of Time's places. */
	UPROPERTY()
	TArray<FTimeWeather> Weather;

	/** The weather at a place, or null if Time keeps no weather there. */
	[[nodiscard]] const FTimeWeather* WeatherAt(FName Place) const
	{
		return Weather.FindByPredicate([Place](const FTimeWeather& Entry) { return Entry.Place == Place; });
	}
};
