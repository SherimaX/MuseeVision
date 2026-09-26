#pragma once

#include "CoreMinimal.h"
#include "Cube/JourneyGen.h"

/**
 * The sea journey's generated things (Raja Ampat): fishes, a reef manta, corals, sponges, sea fans, a pygmy seahorse,
 * red mangroves, karst islets and the six places' seabeds (Cube/JourneySeaGen.cpp). The seabeds' shapes and their
 * material weights are exposed so that the game can place things on them at run time: the meshes are built from these
 * same functions, so the two always agree.
 */
namespace SeaGen
{
	/** Every sea thing, for the editor's bake (MuseeJourneyLibrary::BuildThings). */
	MUSEEVISION_API void Build(TArray<FJourneyThing>& Out);

	/**
	 * The bottom's height (metres, relative to the eyes; z up) at (X, Y) in place 1…6's frame (x east, y south, the eyes
	 * at the origin). Place 5 (the Wall) has no bottom in sight: -200.
	 */
	double BedHeight(int32 Place, double X, double Y);

	/** Place 5: the wall face's x (negative, about -5 m beside the car) at (Y, Z); the wall faces +x, towards the car. */
	double WallX(double Y, double Z);

	/**
	 * The bottom's material weights at (X, Y) as the beds' vertex colours carry them: (sand, coral rubble, reef rock,
	 * algae/encrustation), summing to 1. For place 5 the two coordinates are the wall's (Y, Z).
	 */
	FVector4 BedMaterial(int32 Place, double X, double Y);
}
