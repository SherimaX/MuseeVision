#include "ArtCare.h"

#include "Care/ArtCareRecords.h"
#include "MuseeRecords.h"

namespace Musee::Art
{
	namespace
	{
		/** The record, under Data. */
		const TCHAR* const CareRecord = TEXT("Art/Care.json");

		const FArtCareReproduction* FindReproduction(const FArtCare& Care, FName Key)
		{
			return Care.Reproductions.FindByPredicate([Key](const FArtCareReproduction& R) { return R.Key == Key; });
		}

		/** The clauses by which a dose exceeds a class's limits; an unset limit (zero, written null) binds nothing. */
		TArray<FString> Exceeded(const FArtCareClass& Class, const FArtLightDose& Dose)
		{
			TArray<FString> Out;
			const double Lux = Dose.Illuminance.Get();
			if (Class.IlluminanceLuxMax > 0.0f && Lux > Class.IlluminanceLuxMax)
			{
				Out.Add(FString::Printf(TEXT("%.0f lx is over %s's %.0f lx"), Lux, *Class.Key.ToString(), Class.IlluminanceLuxMax));
			}
			if (Class.AnnualExposureLuxHoursMax > 0.0 && Dose.AnnualLuxHours > Class.AnnualExposureLuxHoursMax)
			{
				Out.Add(FString::Printf(TEXT("%.0f lx h a year is over %s's %.0f lx h"), Dose.AnnualLuxHours, *Class.Key.ToString(),
					Class.AnnualExposureLuxHoursMax));
			}
			if (Class.UvMicrowattsPerLumenMax > 0.0f && Dose.UvMicrowattsPerLumen > Class.UvMicrowattsPerLumenMax)
			{
				Out.Add(FString::Printf(TEXT("%.0f uW/lm of ultraviolet is over %s's %.0f uW/lm"), Dose.UvMicrowattsPerLumen, *Class.Key.ToString(),
					Class.UvMicrowattsPerLumenMax));
			}
			return Out;
		}
	}

	TValueOrError<TArray<FName>, FString> CareClasses()
	{
		const TValueOrError<FArtCare, FString> Care = ReadRecord<FArtCare>(CareRecord);
		if (Care.HasError())
		{
			return MakeError(Care.GetError());
		}
		TArray<FName> Keys;
		for (const FArtCareClass& Class : Care.GetValue().Classes)
		{
			Keys.Add(Class.Key);
		}
		return MakeValue(MoveTemp(Keys));
	}

	TValueOrError<TArray<FName>, FString> Reproductions()
	{
		const TValueOrError<FArtCare, FString> Care = ReadRecord<FArtCare>(CareRecord);
		if (Care.HasError())
		{
			return MakeError(Care.GetError());
		}
		TArray<FName> Keys;
		for (const FArtCareReproduction& Reproduction : Care.GetValue().Reproductions)
		{
			Keys.Add(Reproduction.Key);
		}
		return MakeValue(MoveTemp(Keys));
	}

	TValueOrError<FString, FString> ReproductionCredit(FName Reproduction)
	{
		const TValueOrError<FArtCare, FString> Care = ReadRecord<FArtCare>(CareRecord);
		if (Care.HasError())
		{
			return MakeError(Care.GetError());
		}
		const FArtCareReproduction* Found = FindReproduction(Care.GetValue(), Reproduction);
		if (!Found)
		{
			return MakeError(FString::Printf(TEXT("Care.json lists no reproduction '%s'"), *Reproduction.ToString()));
		}
		return MakeValue(Found->Credit);
	}

	TValueOrError<TArray<FString>, FString> CareFaults(FName CareClass, FName Reproduction, const FArtLightDose& Dose)
	{
		const TValueOrError<FArtCare, FString> Care = ReadRecord<FArtCare>(CareRecord);
		if (Care.HasError())
		{
			return MakeError(Care.GetError());
		}
		const FArtCareClass* Class = Care.GetValue().Classes.FindByPredicate([CareClass](const FArtCareClass& C) { return C.Key == CareClass; });
		if (!Class)
		{
			return MakeError(FString::Printf(TEXT("Care.json has no care class '%s'"), *CareClass.ToString()));
		}
		if (Reproduction.IsNone())
		{
			return MakeValue(Exceeded(*Class, Dose));
		}
		const FArtCareReproduction* Found = FindReproduction(Care.GetValue(), Reproduction);
		if (!Found)
		{
			return MakeError(FString::Printf(TEXT("Care.json lists no reproduction '%s'"), *Reproduction.ToString()));
		}
		return MakeValue(Found->HeldToCare ? Exceeded(*Class, Dose) : TArray<FString>());
	}
}
