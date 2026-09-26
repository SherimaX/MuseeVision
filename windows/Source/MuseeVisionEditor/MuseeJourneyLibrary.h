#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MuseeJourneyLibrary.generated.h"

/**
 * The Élan Cube's journeys (Source/MuseeVision/Cube): their land and their things as Nanite static meshes, built in the
 * editor (a game can't build Nanite) by Scripts/journeys.py.
 *
 * BuildTerrain reads Scripts/journey_terrain.py's output (terrain.json and its tiles: height grids in LV95 metres) and
 * makes one mesh per tile in Folder (SM_<tile>__<west>_<north>): vertices on the grid, relative to the tile's north-west corner (x east, y south, z the height
 * above the sea, curvature taken off), smooth normals from the heights, UV0 metres from the tile's north-west corner,
 * UV1 the tile's place in its orthophoto block (the block's size 8 km). Each tile's material is the instance its
 * image needs (MaterialFolder/MI_MH_…, made by journeys.py first). The far tiles leave out the near box.
 */
UCLASS()
class MUSEEVISIONEDITOR_API UMuseeJourneyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Build and save the terrain's meshes. Only (a comma list of) tile names if Only is set. Returns how many were saved. */
	UFUNCTION(BlueprintCallable, Category = "Musee|Journeys")
	static int32 BuildTerrain(const FString& TerrainJson, const FString& Folder, const FString& MaterialFolder, const FString& Only);

	/** The journeys' generated things (the huts, rocks, corals, fishes…: Cube/JourneyGen), as Nanite meshes in Folder. */
	UFUNCTION(BlueprintCallable, Category = "Musee|Journeys")
	static int32 BuildThings(const FString& Set, const FString& Folder, const FString& MaterialFolder);
};
