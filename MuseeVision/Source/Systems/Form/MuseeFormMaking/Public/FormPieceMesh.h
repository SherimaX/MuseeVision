#pragma once

#include "CoreMinimal.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Templates/UniquePtr.h"

/**
 * A made piece's mesh, kept at one address for its whole life. An FDynamicMesh3's attribute set points back at its
 * mesh, and Unreal's arrays move their elements by copying their bytes, so a mesh held by value inside an array of
 * pieces would leave its attributes pointing at the place it used to be. Held here, the mesh itself never moves; only
 * this handle does.
 *
 * It reads as the mesh it holds: it converts to FDynamicMesh3& wherever one is asked for, and -> and * reach it.
 */
class FFormPieceMesh
{
public:
	FFormPieceMesh() : Mesh(MakeUnique<UE::Geometry::FDynamicMesh3>()) {}
	FFormPieceMesh(const FFormPieceMesh& Other) : Mesh(MakeUnique<UE::Geometry::FDynamicMesh3>(*Other.Mesh)) {}
	FFormPieceMesh(FFormPieceMesh&& Other) : Mesh(MoveTemp(Other.Mesh)) { Other.Mesh = MakeUnique<UE::Geometry::FDynamicMesh3>(); }

	FFormPieceMesh& operator=(const FFormPieceMesh& Other)
	{
		*Mesh = *Other.Mesh;
		return *this;
	}

	FFormPieceMesh& operator=(FFormPieceMesh&& Other)
	{
		Swap(Mesh, Other.Mesh);
		return *this;
	}

	operator UE::Geometry::FDynamicMesh3&() { return *Mesh; }
	operator const UE::Geometry::FDynamicMesh3&() const { return *Mesh; }

	UE::Geometry::FDynamicMesh3& operator*() { return *Mesh; }
	const UE::Geometry::FDynamicMesh3& operator*() const { return *Mesh; }
	UE::Geometry::FDynamicMesh3* operator->() { return Mesh.Get(); }
	const UE::Geometry::FDynamicMesh3* operator->() const { return Mesh.Get(); }

private:
	TUniquePtr<UE::Geometry::FDynamicMesh3> Mesh;
};
