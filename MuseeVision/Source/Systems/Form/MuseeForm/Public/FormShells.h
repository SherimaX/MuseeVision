#pragma once

#include "CoreMinimal.h"
#include "FormEnclosure.h"

#include "FormShells.generated.h"

/** How one made piece's skin reads as a shell, edge by edge over its welded points (FormMaking's MeasureShell). */
USTRUCT()
struct MUSEEFORM_API FFormPieceShell
{
	GENERATED_BODY()

	UPROPERTY()
	FName Piece;

	/** Who drew it: where a fix belongs. */
	UPROPERTY()
	FName Drawer;

	UPROPERTY()
	int32 Faces = 0;

	/** Edges bounded by one triangle only: the skin is open along them. */
	UPROPERTY()
	int32 BoundaryEdges = 0;

	/** Edges bounded by more than two triangles. */
	UPROPERTY()
	int32 NonManifoldEdges = 0;

	/** Edges whose two triangles run them the same way: one face is turned inside out. */
	UPROPERTY()
	int32 MisorientedEdges = 0;

	/** True when the skin is one or more closed, consistently oriented shells. */
	[[nodiscard]] bool IsClosed() const { return Faces > 0 && BoundaryEdges == 0 && NonManifoldEdges == 0 && MisorientedEdges == 0; }
};

/**
 * A stage's enclosures with the shell reading of every made piece: the input of Form.Sealed, which Studio assembles
 * after the stage's pieces are made.
 */
USTRUCT()
struct MUSEEFORM_API FFormShells
{
	GENERATED_BODY()

	/** The stage the pieces belong to. */
	UPROPERTY()
	FMuseeKey Stage;

	UPROPERTY()
	TArray<FFormEnclosure> Enclosures;

	UPROPERTY()
	TArray<FFormPieceShell> Pieces;
};
