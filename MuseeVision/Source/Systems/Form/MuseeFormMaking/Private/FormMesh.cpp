#include "FormMesh.h"

#include "DynamicMesh/DynamicMesh3.h"
#include "Kernel/DynamicMeshOut.h"
#include "Kernel/MeshBuilder.h"
#include "MuseeUnits.h"

FFormMesh::FFormMesh() : Builder(MakeUnique<Musee::Form::FMeshBuilder>()) {}
FFormMesh::~FFormMesh() = default;
FFormMesh::FFormMesh(const FFormMesh& Other) : Builder(MakeUnique<Musee::Form::FMeshBuilder>(*Other.Builder)) {}
FFormMesh::FFormMesh(FFormMesh&& Other) : Builder(MoveTemp(Other.Builder)) { Other.Builder = MakeUnique<Musee::Form::FMeshBuilder>(); }

FFormMesh& FFormMesh::operator=(const FFormMesh& Other)
{
	*Builder = *Other.Builder;
	return *this;
}

FFormMesh& FFormMesh::operator=(FFormMesh&& Other)
{
	Swap(Builder, Other.Builder);
	return *this;
}

int32 FFormMesh::UseSlot(FName Surface) { return Builder->UseSlot(Surface); }
int32 FFormMesh::AddVertex(const FVector& PositionMeters, const FVector& Normal, const FVector2D& UVMeters) { return Builder->AddVertex(PositionMeters, Normal, UVMeters); }
void FFormMesh::SetColor(int32 Vertex, const FLinearColor& Color) { Builder->SetColor(Vertex, Color); }
bool FFormMesh::AddTriangle(int32 A, int32 B, int32 C) { return Builder->AddTriangle(A, B, C); }
void FFormMesh::AddQuad(int32 A, int32 B, int32 C, int32 D) { Builder->AddQuad(A, B, C, D); }
void FFormMesh::Append(const FFormMesh& Other) { Builder->Append(*Other.Builder); }
void FFormMesh::Translate(const FVector& OffsetMeters) { Builder->Translate(OffsetMeters); }
int32 FFormMesh::NumVertices() const { return Builder->NumVertices(); }
int32 FFormMesh::NumTriangles() const { return Builder->NumTriangles(); }
const FVector& FFormMesh::Position(int32 Vertex) const { return Builder->Position(Vertex); }
TConstArrayView<FVector> FFormMesh::AllPositions() const { return Builder->AllPositions(); }
TConstArrayView<FName> FFormMesh::Slots() const { return Builder->Slots(); }

void FFormMesh::Write(UE::Geometry::FDynamicMesh3& Out) const
{
	Musee::Form::WriteDynamicMesh(*Builder, Musee::ToEngine(Musee::Meters(1.0)), Out);
}
