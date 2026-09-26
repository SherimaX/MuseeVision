#pragma once

#include "CoreMinimal.h"
#include "Nature/MuseeNatureActor.h"
#include "MuseeTaihuRock.generated.h"

class UCapsuleComponent;

/**
 * The Taihu rock on the garden's axis (Shared/Wings/ChineseWing.swift: "tall, waisted, twin-peaked,
 * pitted"; ChinesePlan.rock and rockSilhouette). A water-worn limestone: its body follows the plan's
 * section (the N–S silhouette, 4.1 m high with two peaks, waisted at 2.5 m), eroded by the four
 * hollows the Swift marks as through-holes (漏, 透), more holes and blind pits, and vertical
 * wrinkles (皺). It is a signed-distance field carved with smooth subtractions and meshed with
 * surface nets (Voxel metres), with smooth normals from the field's gradient and occlusion from
 * how open each point is. At the snowy terms (19-23) snow lies on its upward faces.
 *
 * The actor stands at the rock's centre on the plan, ChinesePlan.rock (0.08, 25.26).
 */
UCLASS()
class MUSEEVISION_API AMuseeTaihuRock : public AMuseeNatureActor
{
	GENERATED_BODY()

public:
	AMuseeTaihuRock();

	/** Voxel size, metres (smaller is finer and slower: 0.04 m gives about 40k triangles). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature", meta = (ClampMin = "0.02", ClampMax = "0.1"))
	float Voxel = 0.04f;

	/** How deeply the water has worn it (holes, pits and wrinkles). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature", meta = (ClampMin = "0", ClampMax = "2"))
	float Erosion = 1.f;

	/** (Chenghuai) The rock's material: a name in /Game/Museum/Nature, or a full object path (Chenghuai's 石兄 uses its
	 * photographed MI_Ch_Taihu). The default keeps the procedural limestone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nature")
	FString RockMaterial = TEXT("MI_Rock_Taihu");

	UPROPERTY(VisibleAnywhere, Category = "Nature")
	TObjectPtr<UCapsuleComponent> RockCollision;

protected:
	virtual void BuildPlant() override;
};
