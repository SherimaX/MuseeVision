#include "Growth/Plant.h"
#include "MuseeRecords.h"
#include "NaturePieces.h"

namespace Musee::NatureMaking
{
	namespace
	{
		/** The species records' folder, under Data. */
		const TCHAR* const SpeciesFolder = TEXT("Nature/Species");
	}

	TValueOrError<TArray<FNatureSpecies>, FString> ReadSpecies()
	{
		return ReadRecords<FNatureSpecies>(SpeciesFolder);
	}

	TArray<FString> SpeciesFaults(const FNatureSpecies& Species)
	{
		const FForm* Form = FindForm(Species.Form);
		if (!Form)
		{
			return { FString::Printf(TEXT("its form '%s' is not water-lily, iris, sedge or forget-me-not"), *Species.Form.ToString()) };
		}
		TArray<FString> Out = Form->Faults(Species);
		NeedRange(Species.ReachMeters, TEXT("reachMeters"), Out);
		if (Species.FloweringShare < 0.0f || Species.FloweringShare > 1.0f)
		{
			Out.Add(TEXT("its floweringShare is not a share from 0 to 1"));
		}
		for (const FName Organ : Form->Organs)
		{
			if (Species.MaterialOf(Organ).IsNone())
			{
				Out.Add(FString::Printf(TEXT("its %s has no tissue (a palette key in organs)"), *Organ.ToString()));
			}
		}
		return Out;
	}
}
