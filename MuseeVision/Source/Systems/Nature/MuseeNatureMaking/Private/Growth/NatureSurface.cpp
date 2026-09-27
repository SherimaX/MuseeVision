#include "Growth/NatureSurface.h"

namespace Musee::NatureMaking
{
	int32 FNatureSurface::Vertex(const FVector& PositionMeters, const FVector& Normal, const FVector2D& UVMeters, const FLinearColor& Tint, double Occlusion)
	{
		// A grower's normal may vanish where a sheet folds flat; the organ is then seen from above.
		const int32 Index = Mesh.AddVertex(PositionMeters, Normal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector), UVMeters);
		FLinearColor Colour = Tint;
		Colour.A = static_cast<float>(FMath::Clamp(Occlusion, 0.0, 1.0));
		Mesh.SetColor(Index, Colour);
		return Index;
	}

	void FNatureSurface::Triangle(int32 A, int32 B, int32 C)
	{
		Mesh.AddTriangle(A, B, C);
	}

	void FNatureSurface::Quad(int32 A, int32 B, int32 C, int32 D)
	{
		Mesh.AddTriangle(A, B, C);
		Mesh.AddTriangle(A, C, D);
	}

	void FNatureSurface::Append(const FNatureSurface& Other)
	{
		Mesh.Append(Other.Mesh);
	}

	void FNatureSurface::Translate(const FVector& OffsetMeters)
	{
		Mesh.Translate(OffsetMeters);
	}

	void FNatureSurface::Write(UE::Geometry::FDynamicMesh3& Out) const
	{
		Mesh.Write(Out);
	}
}
