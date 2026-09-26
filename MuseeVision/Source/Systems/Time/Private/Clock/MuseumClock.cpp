#include "Clock/MuseumClock.h"

namespace Musee::Time
{
	FMuseumClock::FMuseumClock(const FZoneRule& InZone)
		: CityZone(InZone)
	{
	}

	FDateTime FMuseumClock::MuseumUtc(const FDateTime& RealUtc) const
	{
		if (IsLive()) { return RealUtc; }
		const double ElapsedSeconds = (RealUtc - AnchorRealUtc).GetTotalSeconds();
		return AnchorMuseumUtc + FTimespan::FromSeconds(ClockRate * ElapsedSeconds);
	}

	FDateTime FMuseumClock::MuseumCivil(const FDateTime& RealUtc) const
	{
		return CityZone.CivilFromUtc(MuseumUtc(RealUtc));
	}

	void FMuseumClock::SetLive()
	{
		ChosenCivilHour.Reset();
	}

	void FMuseumClock::SetHour(double CivilHour, const FDateTime& RealUtc)
	{
		const FDateTime Today = CityZone.CivilFromUtc(RealUtc).GetDate();
		ChosenCivilHour = CivilHour;
		AnchorRealUtc = RealUtc;
		AnchorMuseumUtc = CityZone.UtcFromCivil(Today + FTimespan::FromHours(CivilHour));
	}

	void FMuseumClock::SetRate(double InClockRate, const FDateTime& RealUtc)
	{
		AnchorMuseumUtc = MuseumUtc(RealUtc);
		AnchorRealUtc = RealUtc;
		ClockRate = InClockRate;
	}

	void FMuseumClock::SetZone(const FZoneRule& InZone, const FDateTime& RealUtc)
	{
		if (!IsLive())
		{
			const FDateTime Reading = CityZone.CivilFromUtc(MuseumUtc(RealUtc));
			AnchorMuseumUtc = InZone.UtcFromCivil(Reading);
			AnchorRealUtc = RealUtc;
		}
		CityZone = InZone;
	}

	void FMuseumClock::AdvanceHour(TConstArrayView<double> HourStops, const FDateTime& RealUtc)
	{
		for (const double Stop : HourStops)
		{
			if (IsLive() || Stop > ChosenCivilHour.GetValue())
			{
				SetHour(Stop, RealUtc);
				return;
			}
		}
		SetLive();
	}
}
