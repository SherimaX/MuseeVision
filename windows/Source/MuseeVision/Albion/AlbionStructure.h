#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlbionStructure.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;

/**
 * Albion's architecture (Albion/AlbionPlan.h; plan/proposals/albion): the court of iron and glass on the Rotunda's south
 * door, its porch, and its outside, built natively. Place it at the world origin with no rotation or scale: it is built
 * in plan coordinates (x east, plan y south, height up; the Rotunda's centre is the origin).
 *
 * Masonry: buff ashlar walls banded with red stone every 1.2 m over a slate skirting, dressed-stone pilasters, strings,
 * copings and surrounds; six lancets a side with chamfered reveals; the court door (pointed) with Ruskin's words over it;
 * ALBION carved on the west aisle's north wall; the porch's walls, its lining against the drum round the Rotunda's door,
 * its oak ceiling and lead flat; outside, the plinth, buttresses with set-offs, parapets and copings.
 *
 * The ten columns: moulded bases, polished shafts (each its own British stone, AlbionPlan's order A … J), capitals carved
 * with the plants the Details board names; half-columns against the end walls.
 *
 * Iron: clustered shafts from the capitals to the girders that carry the valley gutters, pointed arcade arches with
 * wrought-iron rings in their spandrels, the three vaults' ribs (main ribs on the column lines with a pierced inner arch,
 * lighter ribs between), purlins and glazing bars, bolted shoes; the end screens' mullions and transoms; lead gutters,
 * ridges, flashings, rainwater heads and downpipes.
 *
 * Glass: the three vaults and the end screens (clear), the south screen's Tristram and Isolde band, the lancets (birds
 * and flowers west, Chaucer's Good Women east). The glass casts no shadow of its own: its iron does; the stained glass's
 * colour reaches the floor through AAlbionSkyEffects' light function on the sun.
 *
 * Floors: encaustic tiles on the diagonal in the nave, York stone flags in the aisles, encaustic tiles in a black border in
 * the porch; honed stone in the Rotunda door's reveal, from the sun clock's edge; a bronze strip at the court door.
 *
 * Furniture: two oak benches in the nave, four oak table cases in the aisles (lines 3 and 5).
 *
 * UV0 is in metres (walls along the face and down, floors plan x, y; shafts round and up). The geometry is rebuilt at
 * BeginPlay as well as on construction, unless baked (Geometry/MuseeBake.h).
 *
 * Development builds: -MuseeAlbion previews it in a game run before it is placed (spawned at the origin with its
 * companions, the Chinese Wing it replaces hidden).
 */
UCLASS()
class MUSEEVISION_API AAlbionStructure : public AActor
{
	GENERATED_BODY()

public:
	AAlbionStructure();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** Imported prims this actor replaces: the Chinese Wing's, all of them (native.py retires them). */
	UFUNCTION(BlueprintPure, Category = "Musee|Albion")
	static TArray<FString> GetReplacedImportPrims();

	/** The nave's and aisles' boxes (world cm), for the weather's fog and the light function. */
	UFUNCTION(BlueprintPure, Category = "Musee|Albion")
	static FBox GetCourtBox();

	/** Walls, pilasters, dressings, capitals, columns, inscriptions. With collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Masonry;

	/** The outside: outer faces, plinth, buttresses, parapets, copings, the porch's. With collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Exterior;

	/** The floors. With collision (walkable). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Floor;

	/** Iron, lead and bronze: no collision (out of reach), shadows. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Iron;

	/** The glass: vaults, screens, the stained band and lancets. No collision, no shadow of its own. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Glass;

	/** Oak benches and table cases (and the cases' glass). With collision. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Furniture;

	/** The materials, by slot (AlbionBuild::ESlot). */
	UPROPERTY(EditAnywhere, Category = "Musee|Materials")
	TArray<TSoftObjectPtr<UMaterialInterface>> SlotMaterials;

private:
	void Build();
	void ApplyMaterials();
};
