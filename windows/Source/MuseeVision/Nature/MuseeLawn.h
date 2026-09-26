#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MuseeLawn.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/**
 * The grounds' mown lawn: real blades of grass over the site's ground (AMuseeGround, M_Lawn) wherever a visitor can
 * walk on grass, never on the paving, gravel, kerbs, basins, terraces, hedges, walls or floors.
 *
 * The turf is square patches of blades (FMuseeLawnPatch: tillers of two to four blades, 3–6 cm, fine, curved and
 * tapered, most with the mower's blunt cut tip, a few young pointed ones; darker and yellower down in the sward),
 * baked by the editor (UMuseeLawnLibrary, Scripts/lawn.py) into Nanite static meshes in /Game/Museum/Nature/Lawn:
 * 1 m patches for the open lawn, and 50, 25, 12.5 and 6.25 cm ones that fill in up to whatever stands on it; four
 * variants of each, and 1 m patches at half and a quarter of the density for the fringe. Each patch leans its blades
 * one way; the patches of alternate 2 m bands along the north–south axis are turned to lean the other way: the
 * mowing stripes.
 *
 * Where the lawn is was surveyed in the editor (Survey: box overlaps against everything the visitor collides with,
 * each 1 m cell split down to 6.25 cm near an obstacle, keeping Margin clear, HedgeMargin from the hedges' leaves)
 * and is stored as packed cells; the instances are placed from them at BeginPlay (and for the editor's viewport),
 * in transient instanced components, so the map holds only the cells. Beyond the Region the lawn thins and shortens
 * over Fringe metres to the bare ground, so it has no edge.
 *
 * Nothing moves and it is not baked (musee.nobake: the bake merges procedural geometry only; this is instanced).
 * The blades cast no shadows and are not in the ray-traced scene (they receive the sun's ray-traced shadows of the
 * trees, the building and the hedges; their own depth is in their colour and occlusion). musee.Lawn 0/1 hides and
 * shows them (for A/B timing).
 */
struct FMuseeLawnPatch
{
	/** Centimetres, about the patch's centre on the ground. */
	TArray<FVector3f> Positions;
	TArray<FVector3f> Normals;
	TArray<FVector3f> Tangents;
	/** u across the blade, v from the root (0) to the tip (1). */
	TArray<FVector2f> UVs;
	/** Linear colour (the material decodes the sRGB it is stored as); alpha = occlusion in the sward. */
	TArray<FLinearColor> Colours;
	TArray<int32> Indices;
	int32 Blades = 0;

	int32 NumTriangles() const { return Indices.Num() / 3; }
};

namespace MuseeLawn
{
	/** Patch sides: 1, 0.5, 0.25, 0.125, 0.0625 m. */
	constexpr int32 Levels = 5;
	constexpr int32 Variants = 4;
	/** 1 m patches at 1/2 and 1/4 of the density, for the fringe. */
	constexpr int32 SparseKinds = 2;
	constexpr int32 NumMeshes = Levels * Variants + SparseKinds * Variants;
	/** Blades per square metre of the full sward. */
	constexpr double BladesPerSquareMetre = 12000.0;

	inline double TileSize(int32 Level) { return 1.0 / double(1 << Level); }
	/** The index of a patch mesh: level 0…4 at full density, or (level 0) sparse kind 1…2. */
	inline int32 MeshIndex(int32 Level, int32 Sparse, int32 Variant) { return Sparse > 0 ? Levels * Variants + (Sparse - 1) * Variants + Variant : Level * Variants + Variant; }
	MUSEEVISION_API FString MeshName(int32 Index);
	MUSEEVISION_API FString MeshPath(int32 Index);
	MUSEEVISION_API void DecodeMeshIndex(int32 Index, int32& OutLevel, int32& OutSparse, int32& OutVariant);
	/** The blades of one patch. */
	MUSEEVISION_API void BuildPatch(int32 Level, int32 Sparse, int32 Variant, FMuseeLawnPatch& Out);

	inline const TCHAR* Folder() { return TEXT("/Game/Museum/Nature/Lawn"); }
	inline const TCHAR* MaterialPath() { return TEXT("/Game/Museum/Nature/MI_Lawn_Blade.MI_Lawn_Blade"); }
}

/** The surveyed patches of one size: packed cells, x in the low 16 bits and y in the high 16, from the grid's origin. */
USTRUCT()
struct FMuseeLawnCells
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<int32> Cells;
};

UCLASS()
class MUSEEVISION_API AMuseeLawn : public AActor
{
	GENERATED_BODY()

public:
	AMuseeLawn();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	/** Find the lawn: store the patches that fit (editor; the meshes must exist). Returns the number of patches. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Lawn")
	int32 Survey();

	/** Place the instances from the survey (done at BeginPlay). */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Lawn")
	void Plant();

	/** Show or hide the blades (musee.Lawn). */
	UFUNCTION(BlueprintCallable, Category = "Lawn")
	void SetShown(bool bShow);

	/** Instances, blades and triangles, per patch size. */
	UFUNCTION(BlueprintCallable, Category = "Lawn")
	FString Describe() const;

	/** Where the lawn grows fully (plan metres, x east, y south); it thins out over Fringe beyond. */
	UPROPERTY(EditAnywhere, Category = "Lawn")
	FVector2D RegionMin = FVector2D(-135.0, -75.0);

	UPROPERTY(EditAnywhere, Category = "Lawn")
	FVector2D RegionMax = FVector2D(125.0, 145.0);

	UPROPERTY(EditAnywhere, Category = "Lawn", meta = (ClampMin = "1"))
	float Fringe = 30.f;

	/** Height of the blades at the fringe's outer edge (fraction of the full). */
	UPROPERTY(EditAnywhere, Category = "Lawn", meta = (ClampMin = "0", ClampMax = "1"))
	float FringeHeight = 0.45f;

	/**
	 * Clear ground kept round whatever stands on the lawn (m): the blades lean up to about 4 cm off their patch, so a few
	 * lean over a kerb's foot, as a real lawn's do.
	 */
	UPROPERTY(EditAnywhere, Category = "Lawn", meta = (ClampMin = "0"))
	float Margin = 0.025f;

	/** Round the hedges' dark cores: their leaf shells stand 5.5–7.5 cm off them (MuseeHedge), the turf grows to the leaves. */
	UPROPERTY(EditAnywhere, Category = "Lawn", meta = (ClampMin = "0"))
	float HedgeMargin = 0.06f;

	/** The mowing stripes' width (m), along the north–south axis. */
	UPROPERTY(EditAnywhere, Category = "Lawn", meta = (ClampMin = "0.25"))
	float StripeWidth = 2.f;

	/** Slow drift of the sward's height (fraction), over metres. */
	UPROPERTY(EditAnywhere, Category = "Lawn", meta = (ClampMin = "0", ClampMax = "0.5"))
	float HeightVariation = 0.08f;

	/** No lawn in these boxes (plan metres: min x, min y, max x, max y): the Chinese court, the Hall of Light's gardens. */
	UPROPERTY(EditAnywhere, Category = "Lawn")
	TArray<FVector4> Exclusions;

	UPROPERTY(EditAnywhere, Category = "Lawn")
	bool bPreviewInEditor = true;

	UPROPERTY(EditAnywhere, Category = "Lawn")
	TArray<TSoftObjectPtr<UStaticMesh>> Meshes;

	/** The survey: cells per patch size, the grid's origin (plan metres) and the ground's height (cm). */
	UPROPERTY()
	TArray<FMuseeLawnCells> Tiles;

	UPROPERTY()
	FVector2D GridOrigin = FVector2D::ZeroVector;

	UPROPERTY()
	float GroundZ = -3.f;

private:
	double Weight(const FVector2D& At) const;
	void ClearPlanted();

	UPROPERTY(Transient, DuplicateTransient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Planted;

	TArray<int32> PlantedCounts;
};
