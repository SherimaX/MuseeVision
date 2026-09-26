#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlbionWork.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;

/** What a work in Albion is, as an object. */
UENUM(BlueprintType)
enum class EAlbionWorkKind : uint8
{
	/** A stretched canvas (or a panel, or paper on a stretcher): its face, tacking edges and linen back. */
	Canvas,
	/** A woven tapestry hung from a batten by rings: the cloth in shallow vertical folds, its lining behind. */
	Tapestry,
	/** Lengths of printed wallpaper on a panel. */
	Wallpaper,
	/** A lustre dish or charger (the image its face seen from above), on a plate stand. */
	Dish,
	/** A vase, turned (the image round its side). */
	Vase,
};

/** The sight's shape: the frame's slip covers what lies outside it. */
UENUM(BlueprintType)
enum class EAlbionSight : uint8
{
	Rect,
	/** A round-headed top (Ophelia, The Awakening Conscience, The Long Engagement). */
	Arched,
	/** An oval (An English Autumn Afternoon) or a tondo (The Last of England). */
	Oval,
};

/**
 * One work in Albion's hang (Scripts/albion_hang.py places them, each tagged work:<id> for its placard, and puts an
 * AMuseeFrame round the framed ones). Its origin is the middle of its back, on the wall; +X faces the room, +Y along
 * its width, +Z up (as AMuseeFrame's). Sizes in metres, the catalogue's.
 *
 * - Canvas: the painted face at CanvasOffset in front of the wall, UVs 0 … 1 across it (the image upright), the tacking
 *   margins round its sides and an unbleached linen back on a pine stretcher. An arched or oval sight gets a gilt slip
 *   in front of the canvas (as its frame-maker's), its opening chamfered.
 * - Tapestry: the cloth (the image) hangs from an oak batten at its top, lined, in shallow vertical folds; a hem at the
 *   foot; the batten on two iron brackets.
 * - Wallpaper: the paper on a panel, the pattern repeated (PatternRepeat, m: one repeat of the image's height).
 * - Dish: a shallow lustre dish on a stand; Vase: a turned body.
 * Hanging rods (bronze, Ø 6 mm) run from the frame's top to the picture rail at RailHeight when that is above it.
 */
UCLASS()
class MUSEEVISION_API AAlbionWork : public AActor
{
	GENERATED_BODY()

public:
	AAlbionWork();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Albion")
	EAlbionWorkKind Kind = EAlbionWorkKind::Canvas;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Albion")
	EAlbionSight Sight = EAlbionSight::Rect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Albion")
	float Width = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Albion")
	float Height = 1.f;

	/** The painted face in front of the wall (m). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Albion")
	float CanvasOffset = 0.045f;

	/** The frame's outside half-width and top (m, from the work's middle), for the hanging rods; 0: none. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Albion")
	float FrameOuterHalfWidth = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Albion")
	float FrameOuterTop = 0.f;

	/** The picture rail's height above the work's middle (m); 0: no rods. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Albion")
	float RailAbove = 0.f;

	/** Wallpaper: one repeat's height (m) and width (m). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Albion")
	FVector2D PatternRepeat = FVector2D(0.53, 0.53);

	/** The object's box in its photograph (Dish, Vase): U0 (its left), U1, V0 (its top), V1, as fractions of the image. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Albion")
	FVector4 ImageWindow = FVector4(0.0, 1.0, 0.0, 1.0);

	/** Vase: its outline from its photograph (radius / the widest, height / the height), from the foot up (assets/albion/vessels.json). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Albion")
	TArray<FVector2D> Profile;

	/** Vase: a handle's centre-line in the photograph's plane (across / the widest radius, height / the height), and its
	 *  thickness (/ the widest radius); mirrored for the other handle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Albion")
	TArray<FVector2D> HandlePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Albion")
	float HandleThickness = 0.f;

	/** The image (a material instance with its texture). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Albion")
	TSoftObjectPtr<UMaterialInterface> FaceMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Albion")
	TSoftObjectPtr<UMaterialInterface> LinenMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Albion")
	TSoftObjectPtr<UMaterialInterface> GiltMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Albion")
	TSoftObjectPtr<UMaterialInterface> OakMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Albion")
	TSoftObjectPtr<UMaterialInterface> BronzeMaterial;

	/** Section 0 the face; 1 the linen (edges, back, lining); 2 gilt (slips); 3 oak (batten, panel, stand); 4 bronze. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Mesh;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Musee|Albion")
	void Rebuild();

private:
	void Build();
};
