#include "Growth/Plant.h"

#include "Growth/Organs.h"

namespace Musee::NatureMaking
{
	namespace
	{
		/** Whether a plan point lies inside a closed polygon: an odd count of its edges crossed by the ray along +x. */
		bool InsidePolygon(const TArray<FVector2D>& Corners, const FVector2D& Point)
		{
			bool bInside = false;
			for (int32 Index = 0, Previous = Corners.Num() - 1; Index < Corners.Num(); Previous = Index++)
			{
				const FVector2D& A = Corners[Index];
				const FVector2D& B = Corners[Previous];
				if ((A.Y > Point.Y) != (B.Y > Point.Y) && Point.X < A.X + (B.X - A.X) * (Point.Y - A.Y) / (B.Y - A.Y))
				{
					bInside = !bInside;
				}
			}
			return bInside;
		}

		const FForm* const* Forms()
		{
			static const FForm* const All[] = { &WaterLilyForm(), &IrisForm(), &SedgeForm(), &ForgetMeNotForm(), &TreeForm(), &BambooForm(), &LotusForm(),
				&MossForm(), &GrassForm(), &HerbForm(), &HedgeForm(), &VineForm(), nullptr };
			return All;
		}
	}

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

	bool FPlantSite::InRegion(const FVector2D& Point) const
	{
		return InsidePolygon(Region, Point) && !Holes.ContainsByPredicate([&Point](const TArray<FVector2D>& Hole) { return InsidePolygon(Hole, Point); });
	}

	const FForm* FindForm(FName Name)
	{
		for (const FForm* const* Form = Forms(); *Form; ++Form)
		{
			if ((*Form)->Name == Name)
			{
				return *Form;
			}
		}
		return nullptr;
	}

	FString FormNames()
	{
		TArray<FString> Names;
		for (const FForm* const* Form = Forms(); *Form; ++Form)
		{
			Names.Add((*Form)->Name.ToString());
		}
		return FString::Join(Names, TEXT(", "));
	}

	double Draw(const TArray<float>& Range, FRandomStream& Random)
	{
		return Range.Num() == 2 ? Random.FRandRange(Range[0], Range[1]) : 0.0;
	}

	int32 Draw(const TArray<int32>& Range, FRandomStream& Random)
	{
		return Range.Num() == 2 ? Random.RandRange(Range[0], Range[1]) : 0;
	}

	double Middle(const TArray<float>& Range)
	{
		return Range.Num() == 2 ? 0.5 * (Range[0] + Range[1]) : 0.0;
	}

	FLinearColor Colour(const TArray<FString>& List, int32 Index)
	{
		return List.IsValidIndex(Index) ? FromHex(List[Index]) : FLinearColor::Black;
	}

	FLinearColor AnyColour(const TArray<FString>& List, FRandomStream& Random)
	{
		return List.IsEmpty() ? FLinearColor::Black : FromHex(List[Random.RandRange(0, List.Num() - 1)]);
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
