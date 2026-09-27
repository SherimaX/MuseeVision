#include "TimeMuseumDial.h"

#include "Almanac/AstronomicalTime.h"
#include "Almanac/CelestialAngles.h"
#include "Design/TimeDesign.h"
#include "Sun/SunModel.h"

namespace Musee::Time
{
	namespace
	{
		/** A bearing from true north (clockwise, towards the east) is this far round from the stage's +x axis (east). */
		constexpr double EastBearingDegrees = 90.0;

		/** The sun whose shadow the dial reads. */
		FIdealisedSun SunOf(const FMuseumDial& Dial)
		{
			FIdealisedSun Sun;
			Sun.NodeHeightMeters = Dial.NodusHeightMeters();
			Sun.HourRingRadiusMeters = Dial.ShadowRadiusMeters();
			Sun.FirstHour = Dial.FirstHour();
			Sun.LastHour = Dial.LastHour();
			return Sun;
		}

		/** The Roman numerals of one to twelve, as a clock face spells them (IV, not IIII: the Rotunda's own glyphs). */
		const TCHAR* const Romans[] = {
			TEXT("I"), TEXT("II"), TEXT("III"), TEXT("IV"), TEXT("V"), TEXT("VI"), TEXT("VII"), TEXT("VIII"), TEXT("IX"), TEXT("X"), TEXT("XI"), TEXT("XII"),
		};
	}

	TValueOrError<FMuseumDial, FString> FMuseumDial::Read()
	{
		const TValueOrError<FTimeDesign, FString> Read = ReadDesign();
		if (Read.HasError())
		{
			return MakeError(Read.GetError());
		}
		const FIdealisedSun& Sun = Read.GetValue().IdealisedSun;
		return MakeValue(FMuseumDial(Sun.NodeHeightMeters, Sun.HourRingRadiusMeters, Sun.FirstHour, Sun.LastHour));
	}

	FMuseumDial::FMuseumDial(double InNodusHeightMeters, double InShadowRadiusMeters, double InFirstHour, double InLastHour)
		: NodusHeight(InNodusHeightMeters), ShadowRadius(InShadowRadiusMeters), First(InFirstHour), Last(InLastHour)
	{
	}

	double FMuseumDial::AngleDegrees(double CivilHour) const
	{
		// The shadow falls opposite the sun: at the sun's bearing turned half round, which is its azimuth plus a quarter
		// turn when measured from east towards south.
		return SunOf(*this).AzimuthDegrees(CivilHour) + EastBearingDegrees;
	}

	FVector FMuseumDial::ShadowPoint(double CivilHour) const
	{
		return SunOf(*this).ShadowPoint(CivilHour);
	}

	FVector FMuseumDial::SunDirection(double CivilHour) const
	{
		return SunOf(*this).Direction(CivilHour);
	}

	double FMuseumDial::ReadHour(const FVector& PointOnFace) const
	{
		return SunOf(*this).ReadHour(PointOnFace);
	}

	TArray<FDialMark> FMuseumDial::Marks(int32 EveryMinutes) const
	{
		TArray<FDialMark> Out;
		const int32 Step = FMath::Max(1, EveryMinutes);
		const int32 FirstMinute = FMath::CeilToInt32(First * MinutesPerHour);
		const int32 LastMinute = FMath::FloorToInt32(Last * MinutesPerHour);
		for (int32 Minute = FirstMinute; Minute <= LastMinute; Minute += Step)
		{
			FDialMark Mark;
			Mark.Minute = Minute;
			Mark.AngleDegrees = AngleDegrees(Mark.Hour());
			Mark.Direction = FVector2D(CosDegrees(Mark.AngleDegrees), SinDegrees(Mark.AngleDegrees));
			Out.Add(Mark);
		}
		return Out;
	}

	FString FMuseumDial::Numeral(int32 CivilHour)
	{
		const int32 OnTheFace = ((CivilHour % 12) + 11) % 12;
		return Romans[OnTheFace];
	}
}
