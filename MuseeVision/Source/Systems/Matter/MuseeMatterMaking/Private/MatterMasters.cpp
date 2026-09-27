#include "MatterMasters.h"

#include "MatterNames.h"

namespace Musee::Matter
{
TArray<FMasterKind> AllMasters()
{
	const FName Gilt(Masters::Gilt);
	return {
		{ FName(Masters::Stone), NAME_None },
		{ FName(Masters::Marble), NAME_None },
		{ FName(Masters::White), NAME_None },
		{ FName(Masters::White), Gilt },
		{ FName(Masters::Metal), NAME_None },
		{ FName(Masters::Metal), Gilt },
		{ FName(Masters::Glass), NAME_None },
		{ FName(Masters::Stained), NAME_None },
		{ FName(Masters::Water), NAME_None },
		{ FName(Masters::Paint), NAME_None },
		{ FName(Masters::Plate), NAME_None },
		{ FName(Masters::Wood), NAME_None },
		{ FName(Masters::Wood), Gilt },
		{ FName(Masters::Cloth), NAME_None },
		{ FName(Masters::Sheet), NAME_None },
		{ FName(Masters::Living), NAME_None },
	};
}

TValueOrError<FMasterRecipe, FString> MasterRecipe(const FMasterKind& Kind, UMaterialParameterCollection& Moment)
{
	const bool bGilt = Kind.Layer == FName(Masters::Gilt);
	if (!Kind.Layer.IsNone() && !bGilt)
	{
		return MakeError(FString::Printf(TEXT("No master carries the layer working '%s'; gilt is the one layer."), *Kind.Layer.ToString()));
	}
	const FName Master = Kind.Master;
	if (Master == FName(Masters::White))
	{
		return MakeValue(WhiteRecipe(Moment, bGilt));
	}
	if (Master == FName(Masters::Metal))
	{
		return MakeValue(MetalRecipe(Moment, bGilt));
	}
	if (Master == FName(Masters::Wood))
	{
		return MakeValue(WoodRecipe(Moment, bGilt));
	}
	if (bGilt)
	{
		return MakeError(FString::Printf(TEXT("The %s master does not host gilt; the gilt record lays leaf over plaster, bronze and wood."), *Master.ToString()));
	}
	if (Master == FName(Masters::Stone))
	{
		return MakeValue(StoneRecipe(Moment));
	}
	if (Master == FName(Masters::Marble))
	{
		return MakeValue(MarbleRecipe(Moment));
	}
	if (Master == FName(Masters::Glass))
	{
		return MakeValue(GlassRecipe(Moment));
	}
	if (Master == FName(Masters::Stained))
	{
		return MakeValue(StainedRecipe(Moment));
	}
	if (Master == FName(Masters::Water))
	{
		return MakeValue(WaterRecipe(Moment));
	}
	if (Master == FName(Masters::Paint))
	{
		return MakeValue(PaintRecipe(Moment));
	}
	if (Master == FName(Masters::Plate))
	{
		return MakeValue(PlateRecipe(Moment));
	}
	if (Master == FName(Masters::Cloth))
	{
		return MakeValue(ClothRecipe(Moment));
	}
	if (Master == FName(Masters::Sheet))
	{
		return MakeValue(SheetRecipe(Moment));
	}
	if (Master == FName(Masters::Living))
	{
		return MakeValue(LivingRecipe(Moment));
	}
	return MakeError(FString::Printf(TEXT("There is no master '%s'."), *Master.ToString()));
}
}
