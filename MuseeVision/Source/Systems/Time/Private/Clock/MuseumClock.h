#pragma once

#include "CoreMinimal.h"
#include "Clock/ZoneRule.h"

namespace Musee::Time
{
	/**
	 * The museum's one clock, behind the settings time.hour and time.rate. Live, it reads the real clock. Set to an
	 * hour, it stands at that civil hour of the current civil date in the museum's city and runs on from there at
	 * ClockRate times real time (a rate of 0 holds it). The rate applies only while the clock is set; live is always
	 * the real time. The hour key steps the clock through the design's hours and back to live.
	 */
	class FMuseumClock
	{
	public:
		explicit FMuseumClock(const FZoneRule& InZone);

		/** The museum's instant for a real instant. */
		FDateTime MuseumUtc(const FDateTime& RealUtc) const;
		/** The museum's civil (wall-clock) time in its city. */
		FDateTime MuseumCivil(const FDateTime& RealUtc) const;

		bool IsLive() const { return !ChosenCivilHour.IsSet(); }
		/** The civil hour the clock was last set to; unset while live. */
		TOptional<double> ChosenHour() const { return ChosenCivilHour; }
		double Rate() const { return ClockRate; }
		const FZoneRule& Zone() const { return CityZone; }

		void SetLive();
		/** Stands the clock at a civil hour [0, 24) of the current civil date. */
		void SetHour(double CivilHour, const FDateTime& RealUtc);
		/** How fast the set clock runs, from the museum time it shows at the moment of the change. */
		void SetRate(double InClockRate, const FDateTime& RealUtc);
		/** Moves the museum to another city: a set clock keeps its civil reading, so the new city's sky shows the
		 *  same hour. */
		void SetZone(const FZoneRule& InZone, const FDateTime& RealUtc);
		/** The hour key: from live to the first stop, from a stop to the next later one, after the last back to live. */
		void AdvanceHour(TConstArrayView<double> HourStops, const FDateTime& RealUtc);

	private:
		FZoneRule CityZone;
		TOptional<double> ChosenCivilHour;
		double ClockRate = 1.0;
		/** While set, the museum's time is AnchorMuseumUtc + ClockRate x (real time - AnchorRealUtc). */
		FDateTime AnchorRealUtc;
		FDateTime AnchorMuseumUtc;
	};
}
