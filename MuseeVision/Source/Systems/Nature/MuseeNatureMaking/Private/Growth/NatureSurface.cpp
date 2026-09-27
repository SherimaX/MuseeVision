#include "Growth/NatureSurface.h"

#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "DynamicMesh/MeshTangents.h"
#include "MuseeUnits.h"

namespace Musee::NatureMaking
{
	namespace
	{
		/** A triangle smaller than this in twice its area (square metres) carries no surface: a square tenth of a millimetre. */
		constexpr double LeastDoubleAreaSquareMeters = 1.0e-8;

		/** Every made mesh carries UV0 in metres and UV1 for Matter's piece layout (Form's kernel contract). */
		constexpr int32 UVLayers = 2;
	}

	int32 FNatureSurface::Vertex(const FVector& PositionMeters, const FVector& Normal, const FVector2D& UVMeters, const FLinearColor& Tint, double Occlusion)
	{
		Positions.Add(PositionMeters);
		Normals.Add(Normal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector));
		UVs.Add(UVMeters);
		FLinearColor Colour = Tint;
		Colour.A = static_cast<float>(FMath::Clamp(Occlusion, 0.0, 1.0));
		Tints.Add(Colour);
		return Positions.Num() - 1;
	}

	void FNatureSurface::Triangle(int32 A, int32 B, int32 C)
	{
		const FVector Cross = FVector::CrossProduct(Positions[C] - Positions[A], Positions[B] - Positions[A]);
		if (Cross.Size() < LeastDoubleAreaSquareMeters)
		{
			return;
		}
		// The engine's front face is the one whose normal is cross(c - a, b - a).
		const FVector Facing = Normals[A] + Normals[B] + Normals[C];
		Triangles.Add(FVector::DotProduct(Cross, Facing) >= 0.0 ? FIntVector(A, B, C) : FIntVector(A, C, B));
	}

	void FNatureSurface::Quad(int32 A, int32 B, int32 C, int32 D)
	{
		Triangle(A, B, C);
		Triangle(A, C, D);
	}

	void FNatureSurface::Append(const FNatureSurface& Other)
	{
		const int32 Base = Positions.Num();
		Positions.Append(Other.Positions);
		Normals.Append(Other.Normals);
		UVs.Append(Other.UVs);
		Tints.Append(Other.Tints);
		for (const FIntVector& T : Other.Triangles)
		{
			Triangles.Add(T + FIntVector(Base));
		}
	}

	void FNatureSurface::Translate(const FVector& OffsetMeters)
	{
		for (FVector& P : Positions)
		{
			P += OffsetMeters;
		}
	}

	namespace
	{
		/** The overlays a made mesh writes, with one element per drawn vertex shared by the triangles that share it. */
		struct FLayers
		{
			UE::Geometry::FDynamicMesh3& Mesh;
			UE::Geometry::FDynamicMeshNormalOverlay& Normals;
			UE::Geometry::FDynamicMeshUVOverlay& UVs;
			UE::Geometry::FDynamicMeshUVOverlay& Layout;
			UE::Geometry::FDynamicMeshColorOverlay& Tints;

			/** A new mesh vertex for a drawn one, with its elements: returns the vertex; Out receives the elements. */
			int32 Add(const FVector& Position, const FVector& Normal, const FVector2D& UV, const FLinearColor& Tint, FIntVector4& Out)
			{
				const int32 Id = Mesh.AppendVertex(ToEngine(Position));
				Out = FIntVector4(Normals.AppendElement(FVector3f(Normal)), UVs.AppendElement(FVector2f(UV)),
					Layout.AppendElement(FVector2f::ZeroVector), Tints.AppendElement(FVector4f(Tint)));
				return Id;
			}

			void Set(int32 Triangle, const FIntVector4 (&Elements)[3])
			{
				Normals.SetTriangle(Triangle, UE::Geometry::FIndex3i(Elements[0].X, Elements[1].X, Elements[2].X));
				UVs.SetTriangle(Triangle, UE::Geometry::FIndex3i(Elements[0].Y, Elements[1].Y, Elements[2].Y));
				Layout.SetTriangle(Triangle, UE::Geometry::FIndex3i(Elements[0].Z, Elements[1].Z, Elements[2].Z));
				Tints.SetTriangle(Triangle, UE::Geometry::FIndex3i(Elements[0].W, Elements[1].W, Elements[2].W));
				Mesh.Attributes()->GetMaterialID()->SetValue(Triangle, 0);
			}
		};
	}

	void FNatureSurface::Write(UE::Geometry::FDynamicMesh3& Out) const
	{
		using namespace UE::Geometry;
		Out = FDynamicMesh3();
		Out.EnableAttributes();
		FDynamicMeshAttributeSet& Attributes = *Out.Attributes();
		Attributes.SetNumUVLayers(UVLayers);
		Attributes.EnablePrimaryColors();
		Attributes.EnableMaterialID();
		FLayers Layers{ Out, *Attributes.PrimaryNormals(), *Attributes.GetUVLayer(0), *Attributes.GetUVLayer(1), *Attributes.PrimaryColors() };
		TArray<int32> MeshVertex;
		TArray<FIntVector4> Elements;
		MeshVertex.Init(INDEX_NONE, Positions.Num());
		Elements.SetNum(Positions.Num());
		for (const FIntVector& T : Triangles)
		{
			FIntVector4 Corner[3];
			FIndex3i Vertices;
			for (int32 K = 0; K < 3; ++K)
			{
				const int32 V = T[K];
				if (MeshVertex[V] == INDEX_NONE)
				{
					MeshVertex[V] = Layers.Add(Positions[V], Normals[V], UVs[V], Tints[V], Elements[V]);
				}
				Vertices[K] = MeshVertex[V];
				Corner[K] = Elements[V];
			}
			int32 Id = Out.AppendTriangle(Vertices);
			if (Id < 0)
			{
				// A sheet folded onto another (two petals meeting on one edge) would break the mesh's manifold rule:
				// that triangle takes corners of its own, which show the same drawn surface.
				for (int32 K = 0; K < 3; ++K)
				{
					Vertices[K] = Layers.Add(Positions[T[K]], Normals[T[K]], UVs[T[K]], Tints[T[K]], Corner[K]);
				}
				Id = Out.AppendTriangle(Vertices);
			}
			if (Id >= 0)
			{
				Layers.Set(Id, Corner);
			}
		}
		FMeshTangentsd::ComputeDefaultOverlayTangents(Out);
	}
}
