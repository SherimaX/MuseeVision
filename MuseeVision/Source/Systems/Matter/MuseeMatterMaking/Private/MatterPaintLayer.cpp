#include "MatterPaintLayer.h"

#include "Engine/Texture.h"
#include "MatterInstances.h"
#include "MatterMasterParameters.h"
#include "MatterMasters.h"
#include "MatterNames.h"
#include "Materials/MaterialInstanceConstant.h"

namespace Musee::Matter
{
namespace
{
	TOptional<FMuseeKey> KeyOf(FName Name)
	{
		return Name.IsNone() ? TOptional<FMuseeKey>() : FMuseeKey::Parse(Name.ToString());
	}
}

FString PaintMasterPackage()
{
	return MasterPackage(FName(Masters::Paint));
}

TArray<FString> ComposePaintLayer(const FMatterPaintLayer& Layer, UMaterialInterface& PaintMaster, UMaterialInstanceConstant& Instance)
{
	const TOptional<FMuseeKey> Work = KeyOf(Layer.Work);
	const TOptional<FMuseeKey> Palette = KeyOf(Layer.Palette);
	const TOptional<FMuseeKey> Working = KeyOf(Layer.Working);
	const TOptional<FMuseeKey> History = KeyOf(Layer.History);
	if (!Work || !Palette || !Working || !History)
	{
		return { FString::Printf(TEXT("A paint layer is named by its work's key and a palette finish; this one by '%s' and '%s/%s/%s'."),
			*Layer.Work.ToString(), *Layer.Palette.ToString(), *Layer.Working.ToString(), *Layer.History.ToString()) };
	}
	TArray<FString> Problems;
	if (!Layer.Image)
	{
		Problems.Add(FString::Printf(TEXT("%s: the work has no image."), *Layer.Work.ToString()));
	}
	if (!(Layer.WidthMeters > 0.0) || !(Layer.HeightMeters > 0.0))
	{
		Problems.Add(FString::Printf(TEXT("%s: the work's size (%g x %g m) is not a size."), *Layer.Work.ToString(), Layer.WidthMeters, Layer.HeightMeters));
	}
	if (!Problems.IsEmpty())
	{
		return Problems;
	}

	const FMatterSpec Paint{ *Palette, *Working, *History };
	const FMasterKind PaintKind{ FName(Masters::Paint), NAME_None };
	FInstanceComposed Composed = ComposeInstance(Paint, Instance, [&PaintMaster, &PaintKind](const FMasterKind& Kind) -> UMaterialInterface*
	{
		return Kind == PaintKind ? &PaintMaster : nullptr;
	});
	if (!Composed.Succeeded())
	{
		return MoveTemp(Composed.Problems);
	}

	using namespace MasterParameter;
	const float Width = static_cast<float>(Layer.WidthMeters);
	const float Height = static_cast<float>(Layer.HeightMeters);
	Instance.SetTextureParameterValueEditorOnly(FMaterialParameterInfo(Image), Layer.Image);
	Instance.SetScalarParameterValueEditorOnly(FMaterialParameterInfo(ImageWidthMeters), Width);
	Instance.SetScalarParameterValueEditorOnly(FMaterialParameterInfo(ImageHeightMeters), Height);
	Composed.Written += FString::Printf(TEXT(";%s=%s;%s=%.9g;%s=%.9g"), Image, *Layer.Image->GetPathName(), ImageWidthMeters, Width,
		ImageHeightMeters, Height);
	// The instance is the work's: its identity carries the work's key as well as the paint's records.
	StampInstance(Composed, FMatterSpec{ *Work, *Working, *History }, Instance);
	return {};
}
}
