#include "Calendar/Season.h"

#include "Almanac/CelestialAngles.h"
#include "Calendar/SolarTerms.h"

namespace Musee::Time::SeasonReckoning
{
	constexpr int32 SeasonCount = 4;
	constexpr double SeasonSpanDegrees = 90.0;
	constexpr int32 MonthsPerSeason = 3;
	constexpr int32 MonthsPerYear = 12;
	/** Where each reckoning's spring begins in the Sun's apparent longitude: the March equinox, and Lichun. */
	constexpr double AstronomicalSpringDegrees = 0.0;
	constexpr double SolarTermsSpringDegrees = FirstSolarTermLongitudeDegrees;
	/** The meteorological spring begins on 1 March. */
	constexpr int32 MeteorologicalSpringMonth = 3;

	/** South of the equator the seasons fall half a year later. */
	ESeason ForHemisphere(int32 NorthernIndex, double LatitudeDegrees)
	{
		const int32 Index = LatitudeDegrees < 0.0 ? (NorthernIndex + SeasonCount / 2) % SeasonCount : NorthernIndex;
		return static_cast<ESeason>(Index);
	}

	FSeasonPlace FromLongitude(double LongitudeDegrees, double SpringDegrees, double LatitudeDegrees)
	{
		const double Quarters = ReduceDegrees(LongitudeDegrees - SpringDegrees) / SeasonSpanDegrees;
		const int32 NorthernIndex = FMath::Clamp(FMath::FloorToInt32(Quarters), 0, SeasonCount - 1);
		FSeasonPlace Place;
		Place.Season = ForHemisphere(NorthernIndex, LatitudeDegrees);
		Place.Progress = FMath::Clamp(Quarters - NorthernIndex, 0.0, 1.0);
		return Place;
	}

	FSeasonPlace FromMonths(const FDateTime& CivilDate, double LatitudeDegrees)
	{
		const int32 MonthsFromSpring = (CivilDate.GetMonth() - MeteorologicalSpringMonth + MonthsPerYear) % MonthsPerYear;
		const int32 NorthernIndex = MonthsFromSpring / MonthsPerSeason;
		const int32 StartMonthsFromSpring = NorthernIndex * MonthsPerSeason;
		const int32 StartMonth = (MeteorologicalSpringMonth - 1 + StartMonthsFromSpring) % MonthsPerYear + 1;
		const int32 StartYear = StartMonth > CivilDate.GetMonth() ? CivilDate.GetYear() - 1 : CivilDate.GetYear();
		const FDateTime Start(StartYear, StartMonth, 1);
		const int32 EndMonth = (StartMonth - 1 + MonthsPerSeason) % MonthsPerYear + 1;
		const FDateTime End(EndMonth < StartMonth ? StartYear + 1 : StartYear, EndMonth, 1);

		FSeasonPlace Place;
		Place.Season = ForHemisphere(NorthernIndex, LatitudeDegrees);
		Place.Progress = (CivilDate - Start).GetTotalDays() / (End - Start).GetTotalDays();
		return Place;
	}
}

namespace Musee::Time
{
	FSeasonPlace SeasonAt(ESeasonReckoning Reckoning, double ApparentSolarLongitudeDegrees, const FDateTime& CivilDate,
		double LatitudeDegrees)
	{
		using namespace SeasonReckoning;
		switch (Reckoning)
		{
		case ESeasonReckoning::Astronomical:
			return FromLongitude(ApparentSolarLongitudeDegrees, AstronomicalSpringDegrees, LatitudeDegrees);
		case ESeasonReckoning::SolarTerms:
			return FromLongitude(ApparentSolarLongitudeDegrees, SolarTermsSpringDegrees, LatitudeDegrees);
		case ESeasonReckoning::Meteorological:
			return FromMonths(CivilDate, LatitudeDegrees);
		}
		return {};
	}
}
