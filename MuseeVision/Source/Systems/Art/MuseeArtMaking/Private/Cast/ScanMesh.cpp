#include "Cast/ScanMesh.h"

#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "Misc/FileHelper.h"
#include "StaticMeshAttributes.h"

namespace Musee::ArtMaking
{
	namespace
	{
		/** The `.mvm` header: its four-byte mark and its two counts. */
		constexpr uint32 MvmMark = 0x314D564D; // "MVM1", little-endian
		constexpr int64 MvmHeaderBytes = 12;
		constexpr int64 BytesPerPoint = 3 * sizeof(float);
		constexpr int64 BytesPerTriangle = 3 * sizeof(uint32);

		/** The frame a work stands in from y-up, front +z axes (glTF's and the .mvm's): (x, y, z) is (x, z, y). */
		FVector3f StandUp(const FVector3f& YUp)
		{
			return FVector3f(YUp.X, YUp.Z, YUp.Y);
		}

		uint32 ReadWord(const TArray<uint8>& Bytes, int64 At)
		{
			uint32 Word = 0;
			FMemory::Memcpy(&Word, Bytes.GetData() + At, sizeof(Word));
			return Word;
		}

		FVector3f ReadVector(const TArray<uint8>& Bytes, int64 At)
		{
			float XYZ[3];
			FMemory::Memcpy(XYZ, Bytes.GetData() + At, sizeof(XYZ));
			return FVector3f(XYZ[0], XYZ[1], XYZ[2]);
		}
	}

	bool ReadMvm(const FString& Path, FArtScanMesh& Out, FString& OutWhy)
	{
		TArray<uint8> Bytes;
		if (!FFileHelper::LoadFileToArray(Bytes, *Path, FILEREAD_Silent))
		{
			OutWhy = FString::Printf(TEXT("%s can't be read"), *Path);
			return false;
		}
		if (Bytes.Num() < MvmHeaderBytes || ReadWord(Bytes, 0) != MvmMark)
		{
			OutWhy = FString::Printf(TEXT("%s is not an MVM1 mesh"), *Path);
			return false;
		}
		const int64 Points = ReadWord(Bytes, 4);
		const int64 Triangles = ReadWord(Bytes, 8);
		if (Bytes.Num() != MvmHeaderBytes + 2 * Points * BytesPerPoint + Triangles * BytesPerTriangle || Points == 0 || Triangles == 0)
		{
			OutWhy = FString::Printf(TEXT("%s holds %d bytes, not the %lld points and %lld triangles its header counts"), *Path, Bytes.Num(), Points, Triangles);
			return false;
		}
		Out = FArtScanMesh();
		Out.Points.Reserve(Points);
		Out.Normals.Reserve(Points);
		Out.Triangles.Reserve(Triangles);
		const int64 NormalsAt = MvmHeaderBytes + Points * BytesPerPoint;
		const int64 TrianglesAt = NormalsAt + Points * BytesPerPoint;
		for (int64 Point = 0; Point < Points; ++Point)
		{
			Out.Points.Add(StandUp(ReadVector(Bytes, MvmHeaderBytes + Point * BytesPerPoint)));
			Out.Normals.Add(StandUp(ReadVector(Bytes, NormalsAt + Point * BytesPerPoint)));
		}
		for (int64 Triangle = 0; Triangle < Triangles; ++Triangle)
		{
			const int64 At = TrianglesAt + Triangle * BytesPerTriangle;
			const FIntVector Corners(ReadWord(Bytes, At), ReadWord(Bytes, At + 4), ReadWord(Bytes, At + 8));
			if (Corners.X >= Points || Corners.Y >= Points || Corners.Z >= Points)
			{
				OutWhy = FString::Printf(TEXT("%s: triangle %lld names a point past its %lld"), *Path, Triangle, Points);
				return false;
			}
			Out.Triangles.Add(Corners);
		}
		return true;
	}

	bool ReadImportedScan(UStaticMesh& Mesh, FArtScanMesh& Out, FString& OutWhy)
	{
		const FMeshDescription* Description = Mesh.GetMeshDescription(0);
		if (!Description || Description->Triangles().Num() == 0)
		{
			OutWhy = FString::Printf(TEXT("%s keeps no mesh description with triangles"), *Mesh.GetPathName());
			return false;
		}
		const FStaticMeshConstAttributes Attributes(*Description);
		const TVertexAttributesConstRef<FVector3f> Positions = Attributes.GetVertexPositions();
		const TVertexInstanceAttributesConstRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
		const int32 Points = Description->Vertices().GetArraySize();
		Out = FArtScanMesh();
		Out.Points.SetNumZeroed(Points);
		Out.Normals.SetNumZeroed(Points);
		for (const FVertexID Vertex : Description->Vertices().GetElementIDs())
		{
			Out.Points[Vertex.GetValue()] = Positions[Vertex];
		}
		Out.Triangles.Reserve(Description->Triangles().Num());
		for (const FTriangleID Triangle : Description->Triangles().GetElementIDs())
		{
			const TArrayView<const FVertexInstanceID> Corners = Description->GetTriangleVertexInstances(Triangle);
			FIntVector Points3;
			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				const FVertexID Vertex = Description->GetVertexInstanceVertex(Corners[Corner]);
				// A scan's normals are smooth, one to a point: every corner at a point carries the same.
				Out.Normals[Vertex.GetValue()] = Normals[Corners[Corner]];
				Points3[Corner] = Vertex.GetValue();
			}
			Out.Triangles.Add(Points3);
		}
		return true;
	}
}
