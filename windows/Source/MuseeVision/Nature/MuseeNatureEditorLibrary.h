#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MuseeNatureEditorLibrary.generated.h"

class UMaterialExpression;

/**
 * Helpers for Scripts/nature.py. Unreal's Python can't make a Custom material expression's inputs
 * (FCustomInput is not exposed to scripting), so this sets them, with the HLSL and the output size.
 * Editor only; in a game build it does nothing.
 */
UCLASS()
class MUSEEVISION_API UMuseeNatureEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Sets a Custom expression's HLSL (Code), output size (1-4 floats), named inputs (connect them
	 * afterwards by these names) and caption. Returns false if Expression is not a Custom expression.
	 */
	UFUNCTION(BlueprintCallable, Category = "Musee|Nature")
	static bool ConfigureCustomExpression(UMaterialExpression* Expression, const FString& Code, int32 OutputComponents,
										  const TArray<FString>& InputNames, const FString& Description);
};
