#include "Growth/Plant.h"

#include "Growth/Organs.h"

namespace Musee::NatureMaking
{
	bool FPlantSite::IsClearOf(const FNatureSurface& Surface, double MarginMeters) const
	{
		if (!IsClear)
		{
			return true;
		}
		for (const FVector& Point : Surface.AllPositions())
		{
			if (!IsClear(Point, MarginMeters))
			{
				return false;
			}
		}
		return true;
	}

	const FForm* FindForm(FName Name)
	{
		const FForm* const Forms[] = { &WaterLilyForm(), &IrisForm(), &SedgeForm(), &ForgetMeNotForm() };
		for (const FForm* Form : Forms)
		{
			if (Form->Name == Name)
			{
				return Form;
			}
		}
		return nullptr;
	}

	double Draw(const TArray<float>& Range, FRandomStream& Random)
	{
		return Range.Num() == 2 ? Random.FRandRange(Range[0], Range[1]) : 0.0;
	}

	int32 Draw(const TArray<int32>& Range, FRandomStream& Random)
	{
		return Range.Num() == 2 ? Random.RandRange(Range[0], Range[1]) : 0;
	}

	FLinearColor Colour(const TArray<FString>& List, int32 Index)
	{
		return List.IsValidIndex(Index) ? FromHex(List[Index]) : FLinearColor::Black;
	}

	void NeedRange(const TArray<float>& Range, const TCHAR* Name, TArray<FString>& Out)
	{
		if (Range.Num() != 2 || Range[0] < 0.0f || Range[0] > Range[1])
		{
			Out.Add(FString::Printf(TEXT("%s needs a range [least, most] of positive numbers"), Name));
		}
	}

	void NeedRange(const TArray<int32>& Range, const TCHAR* Name, TArray<FString>& Out)
	{
		if (Range.Num() != 2 || Range[0] < 0 || Range[0] > Range[1])
		{
			Out.Add(FString::Printf(TEXT("%s needs a range [least, most] of whole numbers"), Name));
		}
	}

	void NeedColours(const TArray<FString>& List, int32 Count, const TCHAR* Name, TArray<FString>& Out)
	{
		const bool bHex = List.Num() >= Count && !List.ContainsByPredicate([](const FString& Hex)
		{
			const FString Digits = Hex.StartsWith(TEXT("#")) ? Hex.RightChop(1) : Hex;
			bool bDigits = Digits.Len() == 6;
			for (const TCHAR Digit : Digits)
			{
				bDigits &= FChar::IsHexDigit(Digit);
			}
			return !bDigits;
		});
		if (!bHex)
		{
			Out.Add(FString::Printf(TEXT("%s needs %d colours as sRGB hex"), Name, Count));
		}
	}
}
