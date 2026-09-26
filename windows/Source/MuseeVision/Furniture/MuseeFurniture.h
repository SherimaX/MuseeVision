#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MuseeFurniture.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;

/**
 * The museum's seats and its thresholds, as one set of pieces in the material language (the materials spec, Part 4
 * and section 1.4): built in plan coordinates (x east, plan y south, height up; the Rotunda's centre is the origin), so
 * place it at the world origin with no rotation or scale. Geometry: Furniture/FurnitureKit.h (eased edges throughout).
 *
 * Seats (every one 0.45 m high; each has a hidden, closed hit box from the floor to its seat, a plain block with no
 * overhang, so a visitor walks round it or stands on it, steps off it anywhere, and can never be caught between its
 * legs or under its slab):
 * - (None since the Salon interior update, which keeps the axis clear: its benches are ASalonBenches, Salon/SalonInterior.h.
 *   The detail stays for reference.) The Salon's gallery banquettes (bays 2–4, on the axis): 1.80 × 0.60 m,
 *   an 80 mm cognac leather cushion (F3) with a slight crown and piped (welted) top and bottom edges, on a fumed oak
 *   frame (W1): 60 mm aprons set 3 mm back from four legs that taper from 50 to 35 mm on their inner faces, an
 *   H-stretcher, patinated bronze shoes (M1) 40 mm high. The cushion overhangs the frame by 40 mm all round.
 * - The Nymphéas oval's four pond benches (Plan.Oval.benches): a natural oak seat slab (0.55 × 0.10 m, 10 mm radius on
 *   every edge, grain along the arc) on two honed travertine plinths set back 60 mm, a 20 mm shadow gap under the
 *   slab and a toe recess at the floor.
 * - The Sculpture Hall's bench opposite The Dance: the same detail all in honed travertine (0.5 × 2.4 m).
 * - The Chinese Wing's two terrace benches: a Suzhou granite slab on two waisted (束腰) legs with a hoof foot.
 *
 * Thresholds: at every doorway between rooms, a flush brushed bronze strip (M2) 60 mm wide and 12 mm deep on the
 * threshold line, its top 0.5 mm proud of the stone, its ends 15 mm into the jambs: the Rotunda's four doors on the sun
 * clock's edge (r 10.3), the Salon's door from the Rotunda's passage, the Manet cabinet's door, the Nymphéas oval's
 * arch, the Atrium's west door (on its floor's edge, r 14.3), the Reserve's arch (where the stair's landing meets the
 * concrete), the portico's bronze door; and a 10 mm bronze angle on the nosing of the Classical Hall's porphyry sill.
 */
UCLASS()
class MUSEEVISION_API AMuseeFurniture : public AActor
{
	GENERATED_BODY()

public:
	AMuseeFurniture();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** The Salon banquettes' centres (plan metres), each along x. */
	UFUNCTION(BlueprintPure, Category = "Musee|Furniture")
	static TArray<FVector2D> GetBanquetteCentres();

	/**
	 * Sections: 0 fumed oak, 1 leather, 2 patinated bronze, 3 natural oak, 4 honed travertine, 5 granite (no
	 * collision); 6 the seats' hit boxes (hidden, collision only).
	 */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Pieces;

	/** The bronze thresholds (no collision: the floors carry the visitor). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Thresholds;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> FumedOakMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> LeatherMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> PatinaBronzeMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> NaturalOakMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> TravertineMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> GraniteMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TSoftObjectPtr<UMaterialInterface> ThresholdMaterial;

	/** If a material above is missing (materials.py not yet run): these. */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TArray<TSoftObjectPtr<UMaterialInterface>> FallbackMaterials;

private:
	void Build();
	void ApplyMaterials();
};
