#include "Kernel/MeshBuilder.h"

#include "Kernel/Tolerances.h"

namespace Musee::Form
{
	namespace
	{
		/**
		 * A quad takes its B-D diagonal only when that is shorter than A-C by more than a part in a million, so a
		 * regular grid, whose diagonals differ only by rounding, splits the same way throughout.
		 */
		constexpr double DiagonalPreference = 1.0e-6;
	}

	FMeshBuilder::FMeshBuilder(int32 UVChannelCount)
	{
		ensureMsgf(UVChannelCount >= MinUVChannels && UVChannelCount <= MaxUVChannels,
			TEXT("A Form mesh carries %d to %d UV channels, not %d."), MinUVChannels, MaxUVChannels, UVChannelCount);
		UVs.SetNum(FMath::Clamp(UVChannelCount, MinUVChannels, MaxUVChannels));
		SlotNames.Add(NAME_None);
	}

	int32 FMeshBuilder::UseSlot(FName SlotName)
	{
		Slot = SlotNames.AddUnique(SlotName);
		return Slot;
	}

	int32 FMeshBuilder::AddVertex(const FVector& PositionMeters, const FVector& Normal, const FVector2D& UVMeters)
	{
		FVector Unit = Normal;
		const bool bHasDirection = Unit.Normalize(UE_DOUBLE_SMALL_NUMBER);
		ensureMsgf(bHasDirection, TEXT("A Form vertex needs a normal: the side its surface is seen from."));
		const int32 Index = Positions.Add(PositionMeters);
		Normals.Add(Unit);
		UVs[0].Add(UVMeters);
		for (int32 Channel = 1; Channel < UVs.Num(); ++Channel)
		{
			UVs[Channel].Add(FVector2D::ZeroVector);
		}
		if (HasColors())
		{
			Colors.Add(FLinearColor::White);
		}
		return Index;
	}

	void FMeshBuilder::SetUV(int32 Vertex, int32 Channel, const FVector2D& UV)
	{
		UVs[Channel][Vertex] = UV;
	}

	void FMeshBuilder::SetColor(int32 Vertex, const FLinearColor& Color)
	{
		if (!HasColors())
		{
			Colors.Init(FLinearColor::White, Positions.Num());
		}
		Colors[Vertex] = Color;
	}

	bool FMeshBuilder::AddTriangle(int32 A, int32 B, int32 C)
	{
		const FVector& PA = Positions[A];
		const FVector& PB = Positions[B];
		const FVector& PC = Positions[C];
		const double AB = FVector::Dist(PA, PB);
		const double BC = FVector::Dist(PB, PC);
		const double CA = FVector::Dist(PC, PA);
		const FVector Cross = FVector::CrossProduct(PB - PA, PC - PA);
		// |cross| is twice the area: the longest edge times the height over it.
		const bool bWeldsShut = FMath::Min3(AB, BC, CA) < 2.0 * WeldToleranceMeters;
		const bool bFlat = Cross.Size() < WeldToleranceMeters * FMath::Max3(AB, BC, CA);
		if (bWeldsShut || bFlat)
		{
			++DroppedTriangles;
			return false;
		}
		// Unreal's front face has normal cross(c - a, b - a): (A, B, C) faces along the normals when cross(b - a, c - a)
		// points against them.
		const FVector Facing = Normals[A] + Normals[B] + Normals[C];
		if (FVector::DotProduct(Cross, Facing) < 0.0)
		{
			Triangles.Add(FIntVector(A, B, C));
		}
		else
		{
			Triangles.Add(FIntVector(A, C, B));
		}
		TriangleSlots.Add(Slot);
		return true;
	}

	void FMeshBuilder::AddQuad(int32 A, int32 B, int32 C, int32 D)
	{
		const double AC = FVector::DistSquared(Positions[A], Positions[C]);
		const double BD = FVector::DistSquared(Positions[B], Positions[D]);
		if (BD < AC * (1.0 - DiagonalPreference))
		{
			AddTriangle(A, B, D);
			AddTriangle(B, C, D);
		}
		else
		{
			AddTriangle(A, B, C);
			AddTriangle(A, C, D);
		}
	}

	void FMeshBuilder::Append(const FMeshBuilder& Other)
	{
		ensureMsgf(Other.NumUVChannels() == NumUVChannels(),
			TEXT("Form meshes join only with the same UV channels (%d and %d)."), NumUVChannels(), Other.NumUVChannels());
		const int32 Base = Positions.Num();
		Positions.Append(Other.Positions);
		Normals.Append(Other.Normals);
		for (int32 Channel = 0; Channel < UVs.Num(); ++Channel)
		{
			if (Channel < Other.UVs.Num())
			{
				UVs[Channel].Append(Other.UVs[Channel]);
			}
			else
			{
				UVs[Channel].AddZeroed(Other.NumVertices());
			}
		}
		if (HasColors() || Other.HasColors())
		{
			if (!HasColors())
			{
				Colors.Init(FLinearColor::White, Base);
			}
			for (int32 Vertex = 0; Vertex < Other.NumVertices(); ++Vertex)
			{
				Colors.Add(Other.HasColors() ? Other.Colors[Vertex] : FLinearColor::White);
			}
		}
		TArray<int32> SlotOf;
		for (const FName& Name : Other.SlotNames)
		{
			SlotOf.Add(SlotNames.AddUnique(Name));
		}
		for (int32 Index = 0; Index < Other.NumTriangles(); ++Index)
		{
			const FIntVector& T = Other.Triangles[Index];
			Triangles.Add(FIntVector(T.X + Base, T.Y + Base, T.Z + Base));
			TriangleSlots.Add(SlotOf[Other.TriangleSlots[Index]]);
		}
		DroppedTriangles += Other.DroppedTriangles;
	}

	void FMeshBuilder::Translate(const FVector& OffsetMeters)
	{
		for (FVector& Position : Positions)
		{
			Position += OffsetMeters;
		}
	}
}
