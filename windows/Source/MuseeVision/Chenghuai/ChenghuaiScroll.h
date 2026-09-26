#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ChenghuaiScroll.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;

UENUM(BlueprintType)
enum class EChenghuaiScrollKind : uint8
{
	/** A hanging scroll (立轴): the painting in its silk mount, a top stick with its cord and two 'swallow-scarers' (惊燕),
	 * a roller at the foot whose knobs stand out past the mount. */
	Hanging,
	/** A handscroll (手卷) opened on a case's bed: the work flat, its beginning (the right end) with the frontispiece's
	 * mount and the wrapper's end stick, the rest of the mount rolled up round the roller at the left end. */
	Hand
};

/**
 * One of Chenghuai's works on paper or silk, at its true size in its mount (装裱), for the Hang board's cases.
 *
 * Local frame: the work lies in the actor's YZ plane facing +X, Z up the painting; seen from the front, the actor's −Y is
 * the viewer's right (Unreal's frame is left-handed). A hanging scroll's origin is the middle of its top stick (where
 * the cord hangs from its hook); a handscroll's is the middle of the image's right-hand edge (where it begins), and it
 * runs to the viewer's left (read right to left), its lead and wrapper to the right. Scripts/chenghuai_works.py
 * places each (a handscroll pitched onto its case's slanted bed) and tags it work:<id> for the placards.
 *
 * The image is ImageMaterials' tiles, left to right, each an equal share of the width. The mount is silk: pale for its
 * main fields (天头, 地头, 引首), dark brocade for the narrow bands (隔水) and borders. Meshes are rebuilt on construction
 * and at play; the image catches the look trace (visibility), nothing else collides.
 */
UCLASS()
class MUSEEVISION_API AChenghuaiScroll : public AActor
{
	GENERATED_BODY()

public:
	AChenghuaiScroll();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Scroll")
	EChenghuaiScrollKind Kind = EChenghuaiScrollKind::Hanging;

	/** The painting's width and height, metres (the catalogue's). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Scroll")
	float ImageWidth = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Scroll")
	float ImageHeight = 1.6f;

	/** Hanging: the mount above the painting (天头) and below it (地头); the side borders (镶边); the bands (隔水). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Scroll")
	float Heaven = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Scroll")
	float Earth = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Scroll")
	float Border = 0.035f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Scroll")
	float Band = 0.06f;

	/** Handscroll: the mount shown flat before the image (隔水 and 引首, the frontispiece's field), metres. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Scroll")
	float Lead = 0.45f;

	/** Handscroll: the wrapper (包首) shown open at the right end, with its stick and ribbon; 0 shows it rolled up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Scroll")
	float Wrapper = 0.22f;

	/** Handscroll: the rolled remainder's radius at the left end. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Scroll")
	float RollRadius = 0.035f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Scroll")
	TArray<TSoftObjectPtr<UMaterialInterface>> ImageMaterials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Scroll")
	TSoftObjectPtr<UMaterialInterface> MountMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Scroll")
	TSoftObjectPtr<UMaterialInterface> BandMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Scroll")
	TSoftObjectPtr<UMaterialInterface> WoodMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Scroll")
	TSoftObjectPtr<UMaterialInterface> KnobMaterial;

	/** Rebuild the meshes. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Musee|Scroll")
	void Rebuild();

	/** The scroll's full length (hanging: its height from the stick to the roller's foot; handscroll: the shown length). */
	UFUNCTION(BlueprintPure, Category = "Musee|Scroll")
	float GetMountedLength() const;

	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Mesh;

private:
	void Build();
};
