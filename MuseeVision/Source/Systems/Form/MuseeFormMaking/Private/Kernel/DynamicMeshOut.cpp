#include "Kernel/DynamicMeshOut.h"

#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "DynamicMesh/DynamicMeshOverlay.h"
#include "Kernel/MeshBuilder.h"
#include "Kernel/Tangents.h"
#include "Kernel/Welding.h"

namespace Musee::Form
{
	namespace
	{
		using UE::Geometry::FDynamicMesh3;
		using UE::Geometry::FIndex3i;

		/** The tangent space is three normal layers: the normal, the tangent and the bitangent. */
		constexpr int32 TangentSpaceLayers = 3;

		/**
		 * The overlays a Form mesh writes. Every overlay takes one element per corner in the same order, so one
		 * element index names a corner in all of them.
		 */
		struct FOverlays
		{
			UE::Geometry::FDynamicMeshNormalOverlay* Normals = nullptr;
			UE::Geometry::FDynamicMeshNormalOverlay* Tangents = nullptr;
			UE::Geometry::FDynamicMeshNormalOverlay* Bitangents = nullptr;
			UE::Geometry::FDynamicMeshColorOverlay* Colors = nullptr;
			UE::Geometry::FDynamicMeshMaterialAttribute* Materials = nullptr;
			TArray<UE::Geometry::FDynamicMeshUVOverlay*> UVs;

			FOverlays(FDynamicMesh3& Out, int32 UVLayers)
			{
				Out.EnableAttributes();
				UE::Geometry::FDynamicMeshAttributeSet& Attributes = *Out.Attributes();
				Attributes.SetNumUVLayers(UVLayers);
				Attributes.SetNumNormalLayers(TangentSpaceLayers);
				Attributes.EnablePrimaryColors();
				Attributes.EnableMaterialID();
				Normals = Attributes.PrimaryNormals();
				Tangents = Attributes.PrimaryTangents();
				Bitangents = Attributes.PrimaryBiTangents();
				Colors = Attributes.PrimaryColors();
				Materials = Attributes.GetMaterialID();
				for (int32 Layer = 0; Layer < UVLayers; ++Layer)
				{
					UVs.Add(Attributes.GetUVLayer(Layer));
				}
			}

			/** Appends one builder vertex's corner to every overlay and returns its element. */
			int32 AppendCorner(const FMeshBuilder& Mesh, const FVertexTangent& Frame, int32 Vertex)
			{
				const FVector& Normal = Mesh.Normal(Vertex);
				const FVector Bitangent = FVector::CrossProduct(Normal, Frame.TangentX) * Frame.BinormalSign;
				const FLinearColor Color = Mesh.HasColors() ? Mesh.Color(Vertex) : FLinearColor::White;
				const int32 Element = Normals->AppendElement(FVector3f(Normal));
				verify(Tangents->AppendElement(FVector3f(Frame.TangentX)) == Element);
				verify(Bitangents->AppendElement(FVector3f(Bitangent.GetSafeNormal())) == Element);
				verify(Colors->AppendElement(FVector4f(Color)) == Element);
				for (int32 Layer = 0; Layer < UVs.Num(); ++Layer)
				{
					verify(UVs[Layer]->AppendElement(FVector2f(Mesh.UV(Vertex, Layer))) == Element);
				}
				return Element;
			}

			void SetTriangle(int32 Triangle, const FIndex3i& Elements, int32 Slot)
			{
				Normals->SetTriangle(Triangle, Elements);
				Tangents->SetTriangle(Triangle, Elements);
				Bitangents->SetTriangle(Triangle, Elements);
				Colors->SetTriangle(Triangle, Elements);
				for (UE::Geometry::FDynamicMeshUVOverlay* Layer : UVs)
				{
					Layer->SetTriangle(Triangle, Elements);
				}
				Materials->SetValue(Triangle, Slot);
			}
		};
	}

	FDynamicMeshWriteReport WriteDynamicMesh(const FMeshBuilder& Mesh, double PositionUnitsPerMeter, FDynamicMesh3& Out)
	{
		FDynamicMeshWriteReport Report;
		Out.Clear();
		FOverlays Overlays(Out, Mesh.NumUVChannels());
		const FWeld Weld = WeldVertices(Mesh);
		const TArray<FVertexTangent> Frames = DeriveTangents(Mesh);

		// Points are written when a triangle first stands on them, so the mesh holds no loose vertices.
		TArray<int32> VertexOfPoint;
		VertexOfPoint.Init(FDynamicMesh3::InvalidID, Weld.Points.Num());
		TArray<int32> ElementOfCorner;
		ElementOfCorner.Init(INDEX_NONE, Mesh.NumVertices());
		auto VertexFor = [&](int32 Point)
		{
			if (VertexOfPoint[Point] == FDynamicMesh3::InvalidID)
			{
				VertexOfPoint[Point] = Out.AppendVertex(FVector3d(Weld.Points[Point] * PositionUnitsPerMeter));
			}
			return VertexOfPoint[Point];
		};

		for (int32 Triangle = 0; Triangle < Mesh.NumTriangles(); ++Triangle)
		{
			const FIntVector& T = Mesh.Triangle(Triangle);
			const int32 Corners[3] = {T.X, T.Y, T.Z};
			const int32 Points[3] = {Weld.PointOfVertex[T.X], Weld.PointOfVertex[T.Y], Weld.PointOfVertex[T.Z]};
			if (Points[0] == Points[1] || Points[1] == Points[2] || Points[2] == Points[0])
			{
				++Report.Collapsed;
				continue;
			}
			FIndex3i Vertices(VertexFor(Points[0]), VertexFor(Points[1]), VertexFor(Points[2]));
			int32 Written = Out.AppendTriangle(Vertices);
			FIndex3i Elements;
			if (Written < 0)
			{
				++Report.Separated;
				for (int32 Corner = 0; Corner < 3; ++Corner)
				{
					Vertices[Corner] = Out.AppendVertex(Out.GetVertex(Vertices[Corner]));
					Elements[Corner] = Overlays.AppendCorner(Mesh, Frames[Corners[Corner]], Corners[Corner]);
				}
				Written = Out.AppendTriangle(Vertices);
				check(Written >= 0);
			}
			else
			{
				for (int32 Corner = 0; Corner < 3; ++Corner)
				{
					int32& Element = ElementOfCorner[Corners[Corner]];
					if (Element == INDEX_NONE)
					{
						Element = Overlays.AppendCorner(Mesh, Frames[Corners[Corner]], Corners[Corner]);
					}
					Elements[Corner] = Element;
				}
			}
			Overlays.SetTriangle(Written, Elements, Mesh.TriangleSlot(Triangle));
		}
		return Report;
	}
}
