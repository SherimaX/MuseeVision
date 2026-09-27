#pragma once

#include "CoreMinimal.h"
#include "FormDrawing.h"

namespace Musee::Form
{
	class FMeshBuilder;
}

/**
 * The drawing's words, each drawn into a mesh builder by the kernel in the shape's own frame, in metres. A shape of
 * several parts draws each in the builder slot named for its part (FFormWall::FacePart and the rest); a shape of one
 * part draws in the builder's first slot. Each returns false, with the reason, when the shape can't be drawn.
 */
namespace Musee::Form::Drawing
{
	bool DrawExtrusion(const FFormExtrusion& Shape, FMeshBuilder& Mesh, FString& OutWhy);
	bool DrawSweep(const FFormSweep& Shape, FMeshBuilder& Mesh, FString& OutWhy);
	bool DrawLathe(const FFormLathe& Shape, FMeshBuilder& Mesh, FString& OutWhy);
	bool DrawFace(const FFormFace& Shape, FMeshBuilder& Mesh, FString& OutWhy);
	bool DrawWall(const FFormWall& Shape, FMeshBuilder& Mesh, FString& OutWhy);
	bool DrawBasin(const FFormBasin& Shape, FMeshBuilder& Mesh, FString& OutWhy);
	bool DrawStair(const FFormStair& Shape, FMeshBuilder& Mesh, FString& OutWhy);
	bool DrawRelief(const FFormRelief& Shape, FMeshBuilder& Mesh, FString& OutWhy);

	/** The parts a dome is drawn in: its outer face and springing, its inner face (or its one sheet), its crown zone. */
	namespace DomePart
	{
		inline const FName Outer(TEXT("dome.outer"));
		inline const FName Inner(TEXT("dome.inner"));
		inline const FName Oculus(TEXT("dome.oculus"));
	}

	/** A dome's shell (or its one sheet), placed by its own frame within the drawing's, in the parts above. */
	bool DrawDome(const FFormDome& Shape, FMeshBuilder& Mesh, FString& OutWhy);

	/** The slab into Slab, and the pane (when the opening is glazed) into Pane. */
	bool DrawCeiling(const FFormCeiling& Shape, FMeshBuilder& Slab, FMeshBuilder& Pane, FString& OutWhy);

	/** A loft's side and ends. */
	bool DrawLoft(const FFormLoft& Shape, FMeshBuilder& Mesh, FString& OutWhy);

	/** How a laid surface's units stand against their stock (Form.WithinStock). */
	struct FLaidTally
	{
		int32 Units = 0;
		/** Units longer or wider than the stock gives. */
		int32 BeyondStock = 0;
		/** The largest unit's length and width, metres. */
		FVector2D LargestMeters = FVector2D::ZeroVector;
	};

	/** A laid surface: its units into Units and, when its joints are filled, the filling into Joints. */
	bool DrawLaid(const FFormLaid& Shape, FMeshBuilder& Units, FMeshBuilder& Joints, FLaidTally& OutTally, FString& OutWhy);

	/** Checks that a profile is well formed and, when bClosed, closed; false with the reason naming Role. */
	bool CheckProfile(const FFormProfile& Profile, const TCHAR* Role, bool bClosed, FString& OutWhy);
}
