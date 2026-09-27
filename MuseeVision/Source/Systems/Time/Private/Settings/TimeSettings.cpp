#include "Settings/TimeSettings.h"

#include "Design/TimeDesign.h"
#include "Keeper/TimeKeeper.h"
#include "MuseeCommand.h"
#include "MuseeSetting.h"

namespace Musee::Time::Settings
{
	namespace
	{
		const FName AdvanceHourCommand(TEXT("musee.time.advance-hour"));
		const FName AdvanceCityCommand(TEXT("musee.time.advance-city"));

		/** A civil hour as a clock face reads it. */
		FString ClockReading(double Hour)
		{
			const int32 Minutes = FMath::RoundToInt(Hour * 60.0);
			return FString::Printf(TEXT("%02d:%02d"), Minutes / 60, Minutes % 60);
		}

		bool AdvanceHour(const TArray<FString>& Args, UWorld*, FOutputDevice& Output)
		{
			FMuseeSetting* Hour = FMuseeSetting::Find(HourKey());
			const FTimeDesign* TimeDesign = Design();
			if (!Hour || !TimeDesign) { return false; }
			const TOptional<double> Next = FTimeKeeper::NextHourStop(TimeDesign->HourKeyStops, ChosenHour());
			Hour->SetNumber(Next.Get(LiveHour));
			Output.Log(Next.IsSet()
				? FString::Printf(TEXT("The museum's clock stands at %s."), *ClockReading(Next.GetValue()))
				: FString(TEXT("The museum's clock runs live.")));
			return true;
		}

		bool AdvanceCity(const TArray<FString>& Args, UWorld*, FOutputDevice& Output)
		{
			FMuseeSetting* City = FMuseeSetting::Find(CityKey());
			const FTimeDesign* TimeDesign = Design();
			if (!City || !TimeDesign || City->Choices().IsEmpty()) { return false; }
			const TArray<FName>& Choices = City->Choices();
			const FName Next = Choices[(Choices.IndexOfByKey(City->Choice()) + 1) % Choices.Num()];
			City->SetChoice(Next);
			const FString* Name = TimeDesign->CityNames.Find(Next);
			Output.Log(FString::Printf(TEXT("The museum stands in %s."), Name ? **Name : *Next.ToString()));
			return true;
		}
	}

	FMuseeKey HourKey()
	{
		return FMuseeKey(TEXTVIEW("time.hour"));
	}

	FMuseeKey CityKey()
	{
		return FMuseeKey(TEXTVIEW("time.city"));
	}

	TOptional<double> ChosenHour()
	{
		const FMuseeSetting* Hour = FMuseeSetting::Find(HourKey());
		if (!Hour || Hour->Number() < 0.0) { return {}; }
		return Hour->Number();
	}

	FName ChosenCity()
	{
		const FMuseeSetting* City = FMuseeSetting::Find(CityKey());
		return City ? City->Choice() : NAME_None;
	}

	bool Register()
	{
		const FTimeDesign* TimeDesign = Design();
		if (!TimeDesign) { return false; }
		FMuseeSetting::RegisterNumber(HourKey(), LiveHour, LiveHour, LastHour,
			TEXT("The civil hour the museum's clock is set to, 0 to 24, from which it runs on; -1 for the live clock."));
		FMuseeSetting::RegisterChoice(CityKey(), TimeDesign->DefaultCity, TimeDesign->MuseumCities,
			TEXT("The city the museum stands in: its clock, sun, Moon and weather."));
		FMuseeCommand::Register({ AdvanceHourCommand,
			TEXT("The hour key: sets the museum's clock to the next of the design's hours, and after the last back to live."), &AdvanceHour });
		FMuseeCommand::Register({ AdvanceCityCommand,
			TEXT("The city key: moves the museum to the next city it may stand in."), &AdvanceCity });
		return true;
	}

	void Unregister()
	{
		FMuseeCommand::Unregister(AdvanceCityCommand);
		FMuseeCommand::Unregister(AdvanceHourCommand);
		FMuseeSetting::Unregister(CityKey());
		FMuseeSetting::Unregister(HourKey());
	}
}
