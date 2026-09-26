#include "Nature/MuseeNatureEditorLibrary.h"

#include "Materials/MaterialExpression.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
#include "Materials/MaterialExpressionCustom.h"
PRAGMA_ENABLE_DEPRECATION_WARNINGS

bool UMuseeNatureEditorLibrary::ConfigureCustomExpression(UMaterialExpression* Expression, const FString& Code, int32 OutputComponents,
														  const TArray<FString>& InputNames, const FString& Description)
{
#if WITH_EDITOR
	UMaterialExpressionCustom* Custom = Cast<UMaterialExpressionCustom>(Expression);
	if (!Custom) { return false; }
	Custom->Modify();
	Custom->Code = Code;
	Custom->Description = Description;
	switch (FMath::Clamp(OutputComponents, 1, 4))
	{
	case 1: Custom->OutputType = ECustomMaterialOutputType::CMOT_Float1; break;
	case 2: Custom->OutputType = ECustomMaterialOutputType::CMOT_Float2; break;
	case 3: Custom->OutputType = ECustomMaterialOutputType::CMOT_Float3; break;
	default: Custom->OutputType = ECustomMaterialOutputType::CMOT_Float4; break;
	}
	Custom->Inputs.Reset();
	for (const FString& Name : InputNames)
	{
		FCustomInput& Input = Custom->Inputs.AddDefaulted_GetRef();
		Input.InputName = FName(*Name);
	}
	Custom->PostEditChange();
	return true;
#else
	return false;
#endif
}
