#include "Tests/SurfaceWriter.h"

#include "Make/EngineUnits.h"
#include "StaticMeshAttributes.h"

namespace Musee::Studio
{
	FSurfaceWriter::FSurfaceWriter(FMeshDescription& InDescription)
		: Description(InDescription)
	{
		FStaticMeshAttributes Attributes(Description);
		Attributes.Register();
		Attributes.GetVertexInstanceUVs().SetNumChannels(1);
	}

	FPolygonGroupID FSurfaceWriter::AddGroup(FName SlotName)
	{
		const FPolygonGroupID Group = Description.CreatePolygonGroup();
		FStaticMeshAttributes(Description).GetPolygonGroupMaterialSlotNames()[Group] = SlotName;
		return Group;
	}

	FVertexInstanceID FSurfaceWriter::Instance(const FCorner& Corner)
	{
		FStaticMeshAttributes Attributes(Description);
		const FVector3f Position(EngineFromMeters(Corner.PositionMeters));
		FVertexID Vertex;
		if (const FVertexID* Found = Welded.Find(Position))
		{
			Vertex = *Found;
		}
		else
		{
			Vertex = Description.CreateVertex();
			Attributes.GetVertexPositions()[Vertex] = Position;
			Welded.Add(Position, Vertex);
		}
		const FVertexInstanceID Made = Description.CreateVertexInstance(Vertex);
		const FVector Normal = Corner.Normal.GetSafeNormal();
		const FVector Tangent = Corner.Tangent.GetSafeNormal();
		// The engine's bitangent is (normal x tangent) times the sign (FStaticMeshOperations, MikkTSpace convention).
		const double Handedness = FVector::DotProduct(FVector::CrossProduct(Normal, Tangent), Corner.Bitangent);
		Attributes.GetVertexInstanceNormals()[Made] = FVector3f(Normal);
		Attributes.GetVertexInstanceTangents()[Made] = FVector3f(Tangent);
		Attributes.GetVertexInstanceBinormalSigns()[Made] = Handedness >= 0.0 ? 1.0f : -1.0f;
		Attributes.GetVertexInstanceColors()[Made] = FVector4f(1.0f, 1.0f, 1.0f, 1.0f);
		Attributes.GetVertexInstanceUVs().Set(Made, 0, FVector2f(Corner.UVMeters));
		return Made;
	}

	void FSurfaceWriter::Triangle(FPolygonGroupID Group, const FCorner& A, const FCorner& B, const FCorner& C)
	{
		// The engine takes a triangle's front to face along (C - A) x (B - A) (FStaticMeshOperations).
		const FVector Front = FVector::CrossProduct(C.PositionMeters - A.PositionMeters, B.PositionMeters - A.PositionMeters);
		const bool bAsGiven = FVector::DotProduct(Front, A.Normal + B.Normal + C.Normal) >= 0.0;
		const FVertexInstanceID First = Instance(A);
		const FVertexInstanceID Second = Instance(bAsGiven ? B : C);
		const FVertexInstanceID Third = Instance(bAsGiven ? C : B);
		const FVertexInstanceID Corners[3] = {First, Second, Third};
		Description.CreateTriangle(Group, MakeArrayView(Corners));
	}

	void FSurfaceWriter::Quad(FPolygonGroupID Group, const FCorner& A, const FCorner& B, const FCorner& C, const FCorner& D)
	{
		Triangle(Group, A, B, C);
		Triangle(Group, A, C, D);
	}

	void FSurfaceWriter::Face(FPolygonGroupID Group, const FVector& OriginMeters, const FVector& AxisU, TConstArrayView<double> UCuts,
		const FVector& AxisV, TConstArrayView<double> VCuts, const FVector& Normal, TFunctionRef<bool(int32, int32)> Skip)
	{
		const auto Corner = [&](double U, double V)
		{
			FCorner Made;
			Made.PositionMeters = OriginMeters + AxisU * U + AxisV * V;
			Made.Normal = Normal;
			Made.Tangent = AxisU;
			Made.Bitangent = AxisV;
			Made.UVMeters = FVector2D(FVector::DotProduct(Made.PositionMeters, AxisU), FVector::DotProduct(Made.PositionMeters, AxisV));
			return Made;
		};
		for (int32 Column = 0; Column + 1 < UCuts.Num(); ++Column)
		{
			for (int32 Row = 0; Row + 1 < VCuts.Num(); ++Row)
			{
				if (Skip(Column, Row))
				{
					continue;
				}
				Quad(Group, Corner(UCuts[Column], VCuts[Row]), Corner(UCuts[Column + 1], VCuts[Row]),
					Corner(UCuts[Column + 1], VCuts[Row + 1]), Corner(UCuts[Column], VCuts[Row + 1]));
			}
		}
	}
}
