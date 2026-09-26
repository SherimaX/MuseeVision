#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SalonEditorLibrary.generated.h"

class UMaterialExpression;

/**
 * Editor helpers for the Salon's material scripts (Scripts/salon_interior.py): Python can't fill a Custom
 * expression's inputs or its extra outputs, so the script hands the code and the names here.
 */
UCLASS()
class MUSEEVISION_API USalonEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Sets up a Custom expression: its HLSL, its main output's component count (1–4), its inputs by name, and extra
	 * outputs (by name, each with its component count, 1–4) that the code writes as out parameters. False outside the
	 * editor or if Expression isn't a Custom expression.
	 */
	UFUNCTION(BlueprintCallable, Category = "Musee|Salon")
	static bool ConfigureCustom(UMaterialExpression* Expression, const FString& Code, int32 OutputComponents,
								const TArray<FString>& InputNames, const TArray<FString>& ExtraOutputNames,
								const TArray<int32>& ExtraOutputComponents, const FString& Description);
};
