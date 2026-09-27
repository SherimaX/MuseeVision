#pragma once

#include "ArtHangEntry.h"
#include "ArtMesh.h"
#include "ArtWall.h"
#include "CoreMinimal.h"
#include "MuseeKey.h"

#include "ArtHang.generated.h"

/** A stage's hanging plan, `Data/Stages/<stage>/Hang.json`: its curved walls, the meshes works hang on, and its works. */
USTRUCT()
struct MUSEEART_API FArtHang
{
	GENERATED_BODY()

	/** The file every stage keeps its hanging plan in, inside its schedule folder. */
	static constexpr const TCHAR* FileName = TEXT("Hang.json");

	UPROPERTY(meta = (MuseeRequired))
	FMuseeKey Stage;

	UPROPERTY()
	TArray<FArtWall> Walls;

	UPROPERTY()
	TArray<FArtMesh> Meshes;

	UPROPERTY()
	TArray<FArtHangEntry> Works;
};
