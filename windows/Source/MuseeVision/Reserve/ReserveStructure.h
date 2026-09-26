#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ReserveStructure.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

/**
 * The Reserve's architecture, built natively from MuseePlan::Reserve to look target 04: a grand brick undercroft at
 * h −5.8 under the whole Salon and the lawn either side of it. Place it at the world origin: the geometry is in plan
 * coordinates.
 *
 * - A nave 10 m between the pier lines (9.3 m clear) and an aisle either side (6.65 m clear), 24 m wide and 60 m long
 *   inside (x −74 … −14, y ±12), in eight bays of 7.5 m. Brick piers 0.7 m square with chamfered arrises on the arcade
 *   lines (y ±5), x −66.5 … −21.5, on moulded travertine bases, under travertine imposts whose tops (4.0 m) are the
 *   springing of every arch; on the walls, where bands spring from them, brick responds with travertine bases and imposts.
 * - Brick groin vaults over the 24 bays (8 along × north aisle, nave, south aisle): each the union of two
 *   semi-elliptical barrels with level crowns, so the groins are the bays' diagonals. The nave's rise 1.55 m (crowns
 *   5.61 m), the aisles' 1.25 m (5.31 m). Brick arch bands 6 cm proud of the webs run across the nave and the aisles
 *   over the piers, and along the arcades (at the aisles' rise); over each arcade band a brick lunette rises to the
 *   nave's higher vault.
 * - Brick walls 0.6 m thick, the arched west door (2.4 m, springing 2.0 m) where the long stair arrives, and a closed
 *   box: outer faces, and a top 5 cm under the Salon's floor inside its outer faces (5 cm in), 0.3 m under the lawn
 *   (h −0.33) beyond them.
 * - A dark polished concrete floor at h −5.8 with a bronze-lined guide slot flush in it under each picture screen
 *   (AReserveRacks: its fins run in it, from near the side wall to near the axis), and a notch where the stair's bottom landing reaches 0.3 m into the room
 *   (x −74 … −73.7, y ±1.2), at the same height.
 * - Opal globe pendants from the vaults' crowns: a large one in each bay of the nave on a chain with its cable threaded
 *   through the links, a smaller one in each bay of the aisles on a rod (the cable inside it); each hangs from a
 *   bronze canopy, and the globe's neck sits in a spun bronze gallery held by three thumb screws. The globe is cased
 *   opal: M_GlobeLamp is the lit opal under a glossy skin that reflects the room (one opaque shell: a separate clear
 *   translucent shell cost 2 ms at 4K in front-layer translucency passes for reflections that are the same).
 *   GetPendantPositions() gives the globes' centres for their lamps.
 * - Small bronze uplights on the piers' impost ledges, one up each groin (GetUplightTransforms()).
 * - The void between the vault's back and the box's top is closed by a ceiling over the vault, so nothing behind the
 *   vault is ever lit or seen.
 *
 * Brick relief: the brick displaces (Nanite tessellation) only where the surface is continuous. Vertex colour R is 1
 * inside a smooth patch and 0 on a crease (the groins, the bands' arrises, the piers' arrises) and on the vault's
 * open edges, and the brick master (MI_Brick_Vault, Scripts/reserve_materials.py) scales its displacement by it, so
 * the two sides of a crease never part.
 * - The plan chests: in each aisle's last two bays (round the easel at the east end) an island of two walnut chests
 *   back to back, ten drawers a side with brass pulls; on the south islands a felt mat where one of the Degas pastels
 *   lies (imported, placed by AReserveRacks::ArrangeImported).
 * - The viewing easel on the axis at the east end: an oak H-frame with a ledge on brass brackets (MuseePlan::Reserve
 *   EaselFrontX, EaselLedge); the painting on it is AReserveRacks'.
 *
 * Kept clear for the picture screens and their tracks (AReserveRacks: tracks 3.58–3.70 m, hung from the vault; the
 * lowest arch over any track is the arcade band, 5.0 m; the aisles' globes' bottoms 4.25 m), the easel and the stair.
 *
 * UV0 is in metres. On brick, U runs along the courses and V across them: up the walls, piers and lunettes (from the
 * floor), and along the arc on the vault (from 4.0 m at the springing), so the courses follow each barrel and meet at
 * the groins.
 */
UCLASS()
class MUSEEVISION_API AReserveStructure : public AActor
{
	GENERATED_BODY()

public:
	AReserveStructure();

	virtual void OnConstruction(const FTransform& Transform) override;

	/**
	 * USD paths of the imported prims this actor replaces: the export's walls, floor, aisle, vault and columns, its
	 * plan chests (with their glass) and its easel. The racks' are AReserveRacks'.
	 */
	UFUNCTION(BlueprintPure, Category = "Musee|Reserve")
	static TArray<FString> GetReplacedImportPrims();

	/** The centres of the pendants' globes in world space (cm): the nave's (|y| < 1 m) west to east, then the aisles'. */
	UFUNCTION(BlueprintPure, Category = "Musee|Reserve")
	TArray<FVector> GetPendantPositions() const;

	/** The same in the actor's frame (cm): the plan's own coordinates when the actor stands at the origin. */
	static TArray<FVector> PendantLocalPositions();

	/** The piers' centres at their impost tops (the springing), in the actor's frame (cm). */
	UFUNCTION(BlueprintPure, Category = "Musee|Reserve")
	static TArray<FVector> GetPierImposts();

	/**
	 * The vault uplights, in the actor's frame (cm): each at the lens of its fitting on an impost's corner ledge, its
	 * X axis the beam's direction (up a groin). native.py puts a shadowing spot light at each.
	 */
	UFUNCTION(BlueprintPure, Category = "Musee|Reserve")
	static TArray<FTransform> GetUplightTransforms();

	/** The globes' radii (cm), for the lights' source radius: the nave's and the aisles'. */
	static double NaveGlobeRadius() { return 22.0; }
	static double AisleGlobeRadius() { return 17.0; }

	/** The floor's height (cm): the stair's bottom landing is at the same height. */
	static double FloorHeight() { return -580.0; }

	/**
	 * With complex collision. Section 0: brick walls, the door's reveal and the outside of the box.
	 * 1: brick piers. 2: travertine bases and imposts. 3: the floor. 4: the screens' bronze-lined guide slots.
	 * 5: hidden, collision only: the floor under the stair's landing and the door.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Structure;

	/** The brick vault: webs, arch bands, lunettes and their steps. No collision (it is 4 m up and more). */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Vault;

	/** Section 0: the pendants' bronze (canopies, chains, rods, galleries, thumb screws) and the uplights' fittings. 1: the cables. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Pendants;

	/**
	 * The globes: cased opal glass, lit (UV0 = (the globe's radius in metres, 1 on the neck)). They cast no shadow, so
	 * a lamp can sit inside each (the gallery above does shadow it).
	 */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Globes;

	/** The plan chests and the easel, with collision. Section 0: walnut and oak. 1: brass. 2: the chests' dark plinths and felt mats. */
	UPROPERTY(VisibleAnywhere, Category = "Musee")
	TObjectPtr<UProceduralMeshComponent> Furniture;

	/** Brick laid by UV0 (U along the course, V across, metres): walls, piers, vault; relief scaled by vertex colour R. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> BrickMaterial;

	/** Used for the brick while BrickMaterial doesn't exist yet. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> BrickFallbackMaterial;

	/** The piers' bases and the imposts. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> StoneMaterial;

	/** Dark polished concrete. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> FloorMaterial;

	/** The racks' tracks in the floor. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> TrackMaterial;

	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> BronzeMaterial;

	/** The lit cased opal: core-to-rim luminance, cloudiness, a darker neck, the lamp's colour, a glossy skin (M_GlobeLamp). */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> GlobeMaterial;

	/** Used for the lit opal while GlobeMaterial doesn't exist yet. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> GlobeFallbackMaterial;

	/** The pendants' cloth-covered cables. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> CableMaterial;

	/** The plan chests' wood. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> TimberMaterial;

	/** The chests' pulls. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> BrassMaterial;

	/** The chests' plinths and the felt under the pastels. */
	UPROPERTY(EditAnywhere, Category = "Musee")
	TSoftObjectPtr<UMaterialInterface> FeltMaterial;

private:
	void Build();
};
