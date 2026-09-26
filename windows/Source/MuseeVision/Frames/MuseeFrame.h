#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MuseeFrame.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

/** How a work is framed. */
UENUM(BlueprintType)
enum class EMuseeFrameStyle : uint8
{
	/**
	 * A carved and gilded Barbizon / Louis XV frame, as on the Impressionists at the Musée d'Orsay:
	 * sight-edge bead, sanded frieze, astragal, deep cove, carved ogee with leaf tips, a raised
	 * bead-and-reel ribbon and a back edge; a shell cartouche with acanthus at each corner and at
	 * the middle of each long enough side. About 10–16 cm wide with the canvas.
	 */
	SalonGilt,
	/** A narrower gilt frame for small works: sight bead, frieze, cove, a leaf-tip torus, corner rosettes. */
	CabinetGilt,
	/** A thin dark oak frame round a white mat (the Hall of Light's photographs). */
	Photograph,
	/** No frame (the scrolls, the Water Lilies). */
	None,
	/** Albion: Rossetti's and Madox Brown's reeded frame, flat gilded oak: a sight bead, a flat, a band of reeds, a broad
	 * flat carrying roundels at the corners, two outer reeds. About 10–16 cm. */
	ReededGilt,
	/** Albion: a Watts frame, a gilt cassetta: a small sight moulding, a broad sanded flat, an outer ogee carved with
	 * leaves and a bead at the back edge. About 12–18 cm. */
	WattsGilt,
};

/**
 * A picture frame, built procedurally round a canvas: a moulding profile swept round the sight
 * rectangle with mitred corners (smooth normals along the profile's curves, hard edges at its
 * steps), and carved ornament made as geometry where it is cheap (leaf tips along the ogee, beads
 * and reels on the ribbon, shells and acanthus at the corners and centres). Finer carving and wear
 * are left to the material, which gets:
 *
 *   UV0     metres: U along the moulding (from the middle of each side), V across the profile;
 *   colour  R exposure (0 deep in the cove and under the ornament … 1 on crests, beads and shells),
 *           G the sanded frieze, B carved ornament.
 *
 * The actor's origin is the middle of the frame's back (on the wall), its X axis faces the room,
 * Y is along the width and Z up. Sizes are in metres. Scripts/frames.py places one on every
 * framed painting (FitToCanvas) and attaches it to the painting.
 *
 * The meshes are rebuilt when the actor is constructed (the editor) or begins play; they are not
 * saved with the level (they would be several MB a frame).
 */
UCLASS()
class MUSEEVISION_API AMuseeFrame : public AActor
{
	GENERATED_BODY()

public:
	AMuseeFrame();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Frame")
	EMuseeFrameStyle Style = EMuseeFrameStyle::SalonGilt;

	/** The canvas's width (m): the sight size. The frame's lip overlaps it by SightLip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Frame", meta = (ClampMin = "0.02"))
	float SightWidth = 1.0f;

	/** The canvas's height (m). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Frame", meta = (ClampMin = "0.02"))
	float SightHeight = 0.75f;

	/** The canvas's face in front of the frame's back (the wall), m. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Frame", meta = (ClampMin = "0.0"))
	float CanvasOffset = 0.045f;

	/** Moulding width from the sight edge to the outside (m); 0 = by style and canvas size. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Frame", meta = (ClampMin = "0.0"))
	float FrameWidth = 0.f;

	/** How far the frame's sight edge overlaps the canvas (m). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Frame", meta = (ClampMin = "0.0"))
	float SightLip = 0.006f;

	/**
	 * Mat (Photograph) or linen liner (gilt styles) all round between the canvas and the frame (m).
	 * Negative: the style's default (7 cm for Photograph, none for the gilt styles).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Frame")
	float MatWidth = -1.f;

	/** A fixed outer size for the mat (m), e.g. the Hall of Light's 1.0 × 0.9 m mounts; 0 = MatWidth all round. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Frame")
	FVector2D MatOuterSize = FVector2D::ZeroVector;

	/** 0 = plain mouldings, 1 = the full carving; above 1 tessellates the ornament more finely. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Frame", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float OrnamentDetail = 1.f;

	/** Close the back as the work's own back: a painting's unbleached linen on its pine stretcher (keys in the
	 * corners, a cross-brace on a large canvas), a photograph's backing board. Seen from behind (a glass stele,
	 * the Reserve's racks) the frame is no longer a window. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Frame")
	bool bBackPanel = true;

	/** Collision round the moulding (blocks the look trace, so a glance at the frame finds the work). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Frame")
	bool bCollision = true;

	/** Aged gilding (preferred). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Frame|Materials")
	TSoftObjectPtr<UMaterialInterface> GiltMaterial;

	/** Used when GiltMaterial isn't there (Scripts/frames.py makes it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Frame|Materials")
	TSoftObjectPtr<UMaterialInterface> GiltFallbackMaterial;

	/** Dark oak: the Photograph frames, and the backs and rebates of every frame. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Frame|Materials")
	TSoftObjectPtr<UMaterialInterface> WoodMaterial;

	/** Museum board: the mats and liners. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Frame|Materials")
	TSoftObjectPtr<UMaterialInterface> MatMaterial;

	/** The back of a canvas: unbleached linen (Scripts/frames.py makes it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Frame|Materials")
	TSoftObjectPtr<UMaterialInterface> LinenMaterial;

	/** The stretcher bars and keys: pale pine (Scripts/frames.py makes it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Musee|Frame|Materials")
	TSoftObjectPtr<UMaterialInterface> StretcherMaterial;

	/** The moulding: face (section 0), back and rebate (section 1), hidden collision (section 2). */
	UPROPERTY(VisibleInstanceOnly, Transient, BlueprintReadOnly, Category = "Musee|Frame")
	TObjectPtr<UProceduralMeshComponent> FrameMesh;

	/** The mat or liner. */
	UPROPERTY(VisibleInstanceOnly, Transient, BlueprintReadOnly, Category = "Musee|Frame")
	TObjectPtr<UProceduralMeshComponent> MatMesh;

	/** Rebuild the meshes from the properties. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Musee|Frame")
	void Rebuild();

	/**
	 * Size and place the frame on a canvas (a thin quad): its facing from the quad's normals, its
	 * sight size and face from SightActor's geometry (the canvas itself if null; the Hall of Light
	 * passes a print's mount), and the back plane from the rearmost point of BackActor (the imported
	 * box frame, or the mount; if null, 4.5 cm behind the canvas). Sets SightWidth, SightHeight,
	 * CanvasOffset and the actor's world transform, then rebuilds. False if the canvas has no geometry.
	 */
	UFUNCTION(BlueprintCallable, Category = "Musee|Frame")
	bool FitToCanvas(AActor* CanvasActor, AActor* BackActor = nullptr, AActor* SightActor = nullptr);

	/**
	 * Set the style by name ("SalonGilt", "CabinetGilt", "Photograph", "None") and the options the
	 * scripts use, in one go (one rebuild). Width 0 = the style's default; MatWidth < 0 = the style's
	 * default; MatOuterSize 0 = none. False if the style name is unknown (the style is left as it was).
	 */
	UFUNCTION(BlueprintCallable, Category = "Musee|Frame")
	bool Configure(const FString& StyleName, float NewFrameWidth, float NewMatWidth, FVector2D NewMatOuterSize, bool bNewBackPanel);

	/** The moulding width in use (m): FrameWidth, or the style's default for this canvas. */
	UFUNCTION(BlueprintPure, Category = "Musee|Frame")
	float GetResolvedFrameWidth() const;

	/** The mat widths in use (m) along the width (X) and height (Y). */
	UFUNCTION(BlueprintPure, Category = "Musee|Frame")
	FVector2D GetResolvedMatWidths() const;

	/** The frame's outside size (m): sight + mats + mouldings. */
	UFUNCTION(BlueprintPure, Category = "Musee|Frame")
	FVector2D GetOuterSize() const;

	/**
	 * World position of the middle of the frame's top outside edge, at its front: where a picture
	 * light's arm can start (keep about 10 cm clear above it). Valid after a build.
	 */
	UFUNCTION(BlueprintPure, Category = "Musee|Frame")
	FVector GetTopEdgeWorld() const;

	/** A style's default moulding width (m) for a canvas whose longer side is LongSide (m). */
	UFUNCTION(BlueprintPure, Category = "Musee|Frame")
	static float DefaultFrameWidth(EMuseeFrameStyle ForStyle, float LongSide);

private:
	void EnsureComponents();
	void Build();
	void ResolveMaterials();
	static UMaterialInterface* LoadIfPresent(const TSoftObjectPtr<UMaterialInterface>& Soft);

	/** The materials found in the editor, kept as hard references so a cooked game has them. */
	UPROPERTY()
	TObjectPtr<UMaterialInterface> ResolvedGilt;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> ResolvedWood;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> ResolvedMat;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> ResolvedLinen;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> ResolvedStretcher;

	bool bBuilt = false;

	/** From the last build (m): the frame's front (deepest point from the back) and its top outside edge. */
	double BuiltFront = 0.0;
	double BuiltTop = 0.0;
};
