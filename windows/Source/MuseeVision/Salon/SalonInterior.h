#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Nature/MuseeNatureActor.h"
#include "SalonInterior.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;

/**
 * The Salon's interior update (plan proposals/salon-interior: the Main, Plan, Light, Oval and Details boards). Built in
 * plan coordinates (x east, plan y south, height up; the Rotunda's centre is the origin): place at the origin.
 * Scripts/native.py places the benches; Scripts/salon_interior.py places the case, the four willow canvases and the
 * pond's kerb (they carry per-instance settings and the placards' work: tags).
 */

/**
 * The benches beside the line: a pair in each of bays 2–5, 2.6 m either side of the axis on the bay's centre, just behind
 * the viewing stones, the axis left clear. Backless (you face either wall), 2.4 × 0.6 m, 0.44 m high: an upholstered
 * body in velvet of the bay's own colour, its top quilted in four panels with piped seams and a piped edge, on a dark
 * fumed-oak plinth set back 40 mm. Each has a hidden, closed hit box from the floor to its seat (a plain block: nothing
 * to catch a visitor who jumps on it).
 */
UCLASS()
class MUSEEVISION_API ASalonBenches : public AActor
{
	GENERATED_BODY()

public:
	ASalonBenches();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** Sections: 0 the oak plinths, 1–4 velvet (bays 2–5), 5 the hit boxes (hidden, collision only). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Pieces;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> OakMaterial;

	/** Velvet, bays 2–5 (oxblood's bay has the case, not benches). */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TArray<TSoftObjectPtr<UMaterialInterface>> VelvetMaterials;

	/** The bench centres (plan metres), each along x. */
	UFUNCTION(BlueprintPure, Category = "Musee|Salon")
	static TArray<FVector2D> GetBenchCentres();

private:
	void Build();
	void ApplyMaterials();
};

/**
 * The 1874 case, opposite the stele in bay 1: a table vitrine 1.6 × 0.7 m, 1.0 m high. A fumed-oak carcase on a recessed
 * plinth, a moulded top rail, and on it a low-iron glass hood 13 cm high, its edges held in slim bronze angles; inside,
 * a felt-lined floor with a sloped reading mount. It holds the catalogue of the first exhibition (35 boulevard des
 * Capucines, April 1874) and Le Charivari of 25 April 1874, open at Louis Leroy's review (the article that named the
 * movement): public-domain scans (Scripts/salon_interior.py imports them; assets/salon/CREDITS.md).
 */
UCLASS()
class MUSEEVISION_API ASalonCase : public AActor
{
	GENERATED_BODY()

public:
	ASalonCase();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** Sections: 0 oak, 1 bronze, 2 felt, 3 glass, 4 the catalogue, 5 Le Charivari, 6 the hit box (hidden). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Pieces;

	/** The case's centre (plan metres) and its long axis's bearing (degrees from east towards south). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	FVector2D Centre = FVector2D(-20.1, -1.95);

	UPROPERTY(EditAnywhere, Category = "Musee")
	float Bearing = 0.f;

	/** The documents' sizes (m): the catalogue open at its title (two pages), Le Charivari's page. Their aspect is their scans'. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	FVector2D CatalogueSize = FVector2D(0.27, 0.18);

	UPROPERTY(EditAnywhere, Category = "Musee")
	FVector2D CharivariSize = FVector2D(0.32, 0.46);

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TArray<TSoftObjectPtr<UMaterialInterface>> Materials;

private:
	void Build();
	void ApplyMaterials();
};

/**
 * One of the Orangerie's second room's willow cycle, round the Nymphéas oval's wall at its true size (2 m tall, 0.7 m
 * off the floor): stretched canvases 4.25 m wide butted edge to edge (their joints are hairlines), each curved to the
 * oval wall, 35 mm proud of the plaster on its stretcher. StartAlong: where it begins, in metres round the wall from the
 * oval door's north jamb (north side first, then west and south); the image reads left to right from the room.
 */
UCLASS()
class MUSEEVISION_API ASalonCanvas : public AActor
{
	GENERATED_BODY()

public:
	ASalonCanvas();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** Section 0: the painted face. 1: the canvas's turned edges (bare linen). With collision (the placard's look trace). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Canvas;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float StartAlong = 1.f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float Length = 12.75f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	int32 Canvases = 3;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float Bottom = 0.7f;

	UPROPERTY(EditAnywhere, Category = "Musee")
	float Height = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> PaintingMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> LinenMaterial;

	/** The length of the oval's wall from the door's north jamb round to its south jamb (m). */
	UFUNCTION(BlueprintPure, Category = "Musee|Salon")
	static double WallLength();

private:
	void Build();
	void ApplyMaterials();
};

/**
 * The Nymphéas pond's edge (the pond-edge redesign, 2026-09-26: the user's "A + B, and a touch of D"). The water's edge is
 * the plan's oval (8.4 × 4.6 m over the coping's first design: the water 7.8 × 4.0 m at h 0.38) and round it:
 * - A · a moulded coping of honed travertine (S1) at sitting height, 0.42 m, 0.44 m wide: a torus on top, a fillet and a
 *   cavetto down the outer face, a plain die, a small base on a plinth with a toe recess; cut in sixteen blocks
 *   (1.2–1.36 m, the joints radial, 3 mm wide, their arrises chamfered so they read in the specular), each block its own
 *   slice of the stone, its own tone and polish; a thin gilt-bronze lip round the inside, the water brimming 2.5 cm under it;
 * - B · a carved frieze of water lilies on the die, a band 0.2 m tall framed by the cavetto and a ledge (Art Nouveau, after
 *   Gallé and Majorelle): a whiplash rhizome running on through the band, pads 21–25 cm across overlapping, one lifted on
 *   its stalk, open lilies 13–15 cm and buds on curving stems; a lily from the front 19 cm across at each end of the long
 *   axis, a keystone. Real geometry (a 2.5 mm height field, 40–55 mm proud, the edges cut square and undercut 3 mm), its
 *   normals welded, in a close-grained block (MI_Salon_AshlarCarved: the figure quieter, the voids filled);
 * - D · two submerged planting shelves inside the rim at the ends of the long axis (ASalonPondPlants stands on them).
 * Underneath, a bronze tray (seen from the stair when the pond is lifted). A line of light in the lip (32 lengths, lit in
 * order by APondLift). Attached to the imported pond (part:pond), so all of it rises with it; replaces the pale basin
 * (retired by Scripts/salon_interior.py). Collision: a hidden hit block to sit or stand on (no ledge to trap anyone) and
 * a hidden guard round the water for the visitor only (the look trace passes to the golden lily).
 */
UCLASS()
class MUSEEVISION_API ASalonPond : public AActor
{
	GENERATED_BODY()

public:
	ASalonPond();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** Sections: 0 the coping (travertine), 1 the lining, bed and shelves (dark stone), 2 the tray (bronze), 3 the lip (gilt),
	 *  4 the frieze (travertine), 5 the hit block (hidden, the only collision). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Basin;

	/** The line of light in the lip: 32 sections, shown in order by SetRimReveal (never baked). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> RimLight;

	/** The guard round the water's edge: the visitor only (never baked, never drawn). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Guard;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TArray<TSoftObjectPtr<UMaterialInterface>> Materials;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> RimLightMaterial;

	/** Lights the first Fraction of the rim's 32 lengths (0 dark). */
	void SetRimReveal(float Fraction);

	/** True if the plan point (metres) lies within the pond's outline grown by Margin (the coping's outer face at 0). */
	static bool IsOver(const FVector2D& Plan, double Margin);

	/** The point on the outline grown by Margin, along the ray from the pond's centre through Plan (metres). */
	static FVector2D PushOut(const FVector2D& Plan, double Margin);

private:
	void Build();
	void ApplyMaterials();
};

/**
 * The planting at the pond's two ends (the pond-edge redesign, "a touch of D"): Monet's banks at Giverny, on the submerged
 * shelves inside the coping at the ends of the long axis, framing the axis without hiding the lilies. Japanese irises
 * (Iris laevigata: fans of sword leaves, blue-violet flowers with a white signal on each fall, buds in their spathes),
 * tufts of sedge (Carex, arching, a few brown spikes) and water forget-me-nots (Myosotis scorpioides: low stems, small
 * oblong leaves, sky-blue flowers with a yellow eye in coiled sprays). The actor stands at the water's surface at the
 * pond's centre, attached to the lifting pond (placed by Scripts/salon_interior.py).
 */
UCLASS()
class MUSEEVISION_API ASalonPondPlants : public AMuseeNatureActor
{
	GENERATED_BODY()

public:
	ASalonPondPlants();

	/** The shelves: from |x| = ShelfX (metres from the centre) out to the water's edge, ShelfDepth under the water. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	float ShelfX = 3.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	float ShelfDepth = 0.12f;

protected:
	virtual void BuildPlant() override;
	virtual bool IsMovablePlant() const override { return true; }
};
