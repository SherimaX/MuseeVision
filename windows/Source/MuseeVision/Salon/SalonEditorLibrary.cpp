#include "Salon/SalonEditorLibrary.h"

#include "Materials/MaterialExpressionCustom.h"

namespace
{
	ECustomMaterialOutputType OutputTypeFor(int32 Components)
	{
		switch (FMath::Clamp(Components, 1, 4))
		{
		case 1: return ECustomMaterialOutputType::CMOT_Float1;
		case 2: return ECustomMaterialOutputType::CMOT_Float2;
		case 3: return ECustomMaterialOutputType::CMOT_Float3;
		default: return ECustomMaterialOutputType::CMOT_Float4;
		}
	}
}

bool USalonEditorLibrary::ConfigureCustom(UMaterialExpression* Expression, const FString& Code, int32 OutputComponents,
										  const TArray<FString>& InputNames, const TArray<FString>& ExtraOutputNames,
										  const TArray<int32>& ExtraOutputComponents, const FString& Description)
{
#if WITH_EDITOR
	UMaterialExpressionCustom* Custom = Cast<UMaterialExpressionCustom>(Expression);
	if (!Custom) { return false; }
	Custom->Modify();
	Custom->Code = Code;
	Custom->Description = Description;
	Custom->OutputType = OutputTypeFor(OutputComponents);
	Custom->Inputs.Reset();
	for (const FString& Name : InputNames)
	{
		FCustomInput& Input = Custom->Inputs.AddDefaulted_GetRef();
		Input.InputName = FName(*Name);
	}
	Custom->AdditionalOutputs.Reset();
	for (int32 i = 0; i < ExtraOutputNames.Num(); ++i)
	{
		FCustomOutput& Output = Custom->AdditionalOutputs.AddDefaulted_GetRef();
		Output.OutputName = FName(*ExtraOutputNames[i]);
		Output.OutputType = OutputTypeFor(ExtraOutputComponents.IsValidIndex(i) ? ExtraOutputComponents[i] : 4);
	}
	Custom->RebuildOutputs();
	Custom->PostEditChange();
	return true;
#else
	return false;
#endif
}
