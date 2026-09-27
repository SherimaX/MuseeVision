#pragma once

#include "CoreMinimal.h"

class UStaticMesh;

namespace Musee::ArtMaking
{
	/**
	 * A scan of a work in the round as Art reads it: its points, a unit normal at each, and its triangles, in the frame the
	 * work stands in: x to the right of a viewer facing its front, y its front, z up. Its unit is its file's; a cast is
	 * scaled to its true height.
	 */
	struct FArtScanMesh
	{
		TArray<FVector3f> Points;
		TArray<FVector3f> Normals;
		TArray<FIntVector> Triangles;
	};

	/** The scan files the collection holds: glTF binaries (imported by Studio) and the old museum's `.mvm`. */
	inline const TCHAR* const GltfExtension = TEXT("glb");
	inline const TCHAR* const MvmExtension = TEXT("mvm");

	/**
	 * Reads an `.mvm`, the old museum's mesh (assets/sculptures/CREDITS.md): "MVM1", then the vertex and triangle counts
	 * (uint32), the points and the normals (float32 x, y, z each: y up, the front towards +z, x to the viewer's right, in
	 * metres), and the triangles (three uint32 each, anticlockwise seen from outside), little-endian. False, with why, for
	 * a file that isn't one.
	 */
	[[nodiscard]] bool ReadMvm(const FString& Path, FArtScanMesh& Out, FString& OutWhy);

	/**
	 * Reads a scan the engine's glTF import made, from the mesh description it keeps. The import turns glTF's axes into
	 * the engine's by taking glTF's (x, y, z) as (x, z, y), so the viewer's right stays x, the front (glTF +z) is y and up
	 * is z: the frame a work stands in. False, with why, when the mesh keeps no description or no triangles.
	 */
	[[nodiscard]] bool ReadImportedScan(UStaticMesh& Mesh, FArtScanMesh& Out, FString& OutWhy);
}
