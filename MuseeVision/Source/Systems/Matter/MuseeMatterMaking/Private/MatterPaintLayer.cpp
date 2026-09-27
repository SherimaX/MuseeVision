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

	/** The master a finish's layer is composed over: the paint master for a paint, the plate master for a daguerreotype's
	 *  plate, the stained-glass master for a glass. */
	TOptional<FName> LayerMasterOf(const FMasterKind& Finish)
	{
		if (!Finish.Layer.IsNone())
		{
			return {};
		}
		if (Finish.Master == FName(Masters::Paint) || Finish.Master == FName(Masters::Plate))
		{
			return Finish.Master;
		}
		return Finish.Master == FName(Masters::Glass) ? TOptional<FName>(FName(Masters::Stained)) : TOptional<FName>();
	}

	/** The spec whose records give a layer's physics, or why it is no spec. */
	TValueOrError<FMatterSpec, FString> FinishOf(const FMatterPaintLayer& Layer)
	{
		const TOptional<FMuseeKey> Palette = KeyOf(Layer.Palette);
		const TOptional<FMuseeKey> Working = KeyOf(Layer.Working);
		const TOptional<FMuseeKey> History = KeyOf(Layer.History);
		if (!Palette || !Working || !History)
		{
			return MakeError(FString::Printf(TEXT("A layer derives from a palette finish; this one names '%s/%s/%s'."),
				*Layer.Palette.ToString(), *Layer.Working.ToString(), *Layer.History.ToString()));
		}
		return MakeValue(FMatterSpec{ *Palette, *Working, *History });
	}
}

FString PaintMasterPackage()
{
	return MasterPackage(FName(Masters::Paint));
}

TValueOrError<FString, FString> LayerMasterPackage(const FMatterPaintLayer& Layer)
{
	const TValueOrError<FMatterSpec, FString> Finish = FinishOf(Layer);
	if (Finish.HasError())
	{
		return MakeError(Finish.GetError());
	}
	const TValueOrError<FSpecRecords, FString> Records = ReadSpec(Finish.GetValue());
	if (Records.HasError())
	{
		return MakeError(FString::Printf(TEXT("%s: %s"), *Finish.GetValue().ToString(), *Records.GetError()));
	}
	const TOptional<FName> Master = LayerMasterOf(Records.GetValue().Master);
	if (!Master)
	{
		return MakeError(FString::Printf(TEXT("%s is shaded by the %s master, which carries no work's image; a paint, a plate or a glass does."),
			*Finish.GetValue().ToString(), *MasterName(Records.GetValue().Master.Master, Records.GetValue().Master.Layer)));
	}
	return MakeValue(MasterPackage(*Master));
}

TArray<FString> ComposePaintLayer(const FMatterPaintLayer& Layer, UMaterialInterface& LayerMaster, UMaterialInstanceConstant& Instance)
{
	const TOptional<FMuseeKey> Work = KeyOf(Layer.Work);
	const TValueOrError<FMatterSpec, FString> Finish = FinishOf(Layer);
	if (!Work || Finish.HasError())
	{
		return { FString::Printf(TEXT("A layer is named by its work's key and a palette finish; this one by '%s' and '%s/%s/%s'."),
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

	// The finish's own master is the plain paint or glass; its layer is composed over the master that carries the image.
	FInstanceComposed Composed = ComposeInstance(Finish.GetValue(), Instance, [&LayerMaster](const FMasterKind& Kind) -> UMaterialInterface*
	{
		return LayerMasterOf(Kind).IsSet() ? &LayerMaster : nullptr;
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
	// The instance is the work's: its identity carries the work's key as well as the finish's records.
	StampInstance(Composed, FMatterSpec{ *Work, Finish.GetValue().Working, Finish.GetValue().History }, Instance);
	return {};
}
}
