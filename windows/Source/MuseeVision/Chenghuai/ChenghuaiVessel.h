#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChenghuaiVessel.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;

/**
 * A thrown vessel for Chenghuai's ceramics cases (the pieces the Rooms board marks "loan to select": the Ding bowl, the
 * Jun flowerpot, the Guan vase, the Jian bowl, the Longquan vase, the Xuande jar, the Jiajing wucai jar, the Yongzheng
 * falangcai bowl), turned from its profile as a potter's wheel would: the outer wall from the foot's centre up to the
 * lip, then the inner wall back down to the centre (metres, r and z). An optional lid from its own profile. UV: U round
 * the vessel, V along the profile (0 … 1 by length), as Scripts/chenghuai_vessels.py paints its glaze and decoration.
 *
 * Its carved nanmu stand (座) under it, if StandRadius > 0; the vessel then stands on the stand's top. A mesh placed
 * from the museum's scans (the horse, the Ru basin …) can have a stand alone: an empty profile and a radius.
 */
UCLASS()
class MUSEEVISION_API AChenghuaiVessel : public AActor
{
	GENERATED_BODY()

public:
	AChenghuaiVessel();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** The profile (r, z) in metres: outer foot centre → lip → inner centre. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Vessel")
	TArray<FVector2D> Profile;

	/** The lid's profile (r, z), closed the same way (its outer from the rim to the knob's top, then its underside). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Vessel")
	TArray<FVector2D> LidProfile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Vessel")
	int32 Segments = 72;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Vessel")
	TSoftObjectPtr<UMaterialInterface> Material;

	/** The stand's top radius (0: none) and height, metres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Vessel")
	float StandRadius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Vessel")
	float StandHeight = 0.03f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Vessel")
	TSoftObjectPtr<UMaterialInterface> StandMaterial;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Musee|Vessel")
	void Rebuild();

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Mesh;

private:
	void Build();
};
