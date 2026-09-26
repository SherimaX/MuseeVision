#pragma once

#include "CoreMinimal.h"
#include "Cube/CubeMesh.h"

/** One generated thing of a journey (a hut, a rock, a coral, a fish…), built into a Nanite mesh by the editor. */
struct FJourneyThing
{
	FString Name;               // the asset's name
	CubeMesh::FMesh Mesh;       // its geometry, in its own frame (cm)
	FString Material;           // the material instance's name (in the journey's material folder)
	TArray<FString> Materials;  // several sections: one mesh per section is not needed, Sections[i] uses Materials[i]
	TArray<CubeMesh::FMesh> Sections;
	bool bNanite = true;
	bool bCastShadow = true;
	bool bCollision = false;
};

/**
 * The journeys' things (Cube/JourneyMatterhorn, Cube/JourneySea), generated here and baked by the editor
 * (MuseeJourneyLibrary::BuildThings) into Nanite meshes. Sets: "matterhorn", "sea" (parked), "sealight" (the Cube's
 * first piece: its cells' mesh, not Nanite).
 */
namespace JourneyGen
{
	MUSEEVISION_API void Build(const FString& Set, TArray<FJourneyThing>& Out);
}
