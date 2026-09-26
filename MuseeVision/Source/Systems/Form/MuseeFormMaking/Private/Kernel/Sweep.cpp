#include "Kernel/Sweep.h"

#include "Kernel/MeshBuilder.h"
#include "Kernel/PlanarFace.h"
#include "Kernel/Profile.h"
#include "Kernel/SweepFrame.h"

namespace Musee::Form
{
	namespace
	{
		/**
		 * The vertices of a sweep, per frame and profile station: the one that starts the segment leaving the
		 * station and the one that ends the segment arriving at it. They are one vertex where the profile is smooth.
		 */
		struct FSweepVertices
		{
			int32 Stations = 0;
			TArray<int32> Leaving;
			TArray<int32> Arriving;

			int32 Slot(int32 Frame, int32 Station) const { return Frame * Stations + Station; }
		};

		FSweepVertices AddSweepVertices(FMeshBuilder& Mesh, TConstArrayView<FSweepFrame> Run,
			const FProfileStations& Profile)
		{
			FSweepVertices Vertices;
			Vertices.Stations = Profile.Vertices.Num();
			Vertices.Leaving.Init(INDEX_NONE, Run.Num() * Vertices.Stations);
			Vertices.Arriving.Init(INDEX_NONE, Run.Num() * Vertices.Stations);
			for (int32 Frame = 0; Frame < Run.Num(); ++Frame)
			{
				const FSweepFrame& F = Run[Frame];
				for (int32 Station = 0; Station < Vertices.Stations; ++Station)
				{
					const FProfileVertex& S = Profile.Vertices[Station];
					const FVector P = F.At(S.Point);
					const double U = F.U;
					const bool bLeaves = Profile.bClosed || Station + 1 < Vertices.Stations;
					const bool bArrives = Profile.bClosed || Station > 0;
					// A closed profile's V runs from zero at its first station to its whole length on returning there.
					const bool bSeam = Profile.bClosed && Station == 0;
					const int32 Slot = Vertices.Slot(Frame, Station);
					if (bLeaves)
					{
						Vertices.Leaving[Slot] = Mesh.AddVertex(P, F.NormalAt(S.NormalOut), FVector2D(U, S.LengthMeters));
					}
					if (bArrives)
					{
						const bool bShared = bLeaves && !S.bCrisp && !bSeam;
						const double V = bSeam ? Profile.ClosedLengthMeters : S.LengthMeters;
						Vertices.Arriving[Slot] = bShared ? Vertices.Leaving[Slot]
							: Mesh.AddVertex(P, F.NormalAt(S.NormalIn), FVector2D(U, V));
					}
				}
			}
			return Vertices;
		}
	}

	void Sweep(FMeshBuilder& Mesh, TConstArrayView<FSweepFrame> Run, const FProfileStations& Profile)
	{
		const int32 Segments = Profile.NumSegments();
		if (Run.Num() < 2 || Segments < 1)
		{
			return;
		}
		const FSweepVertices Vertices = AddSweepVertices(Mesh, Run, Profile);
		for (int32 Segment = 0; Segment < Segments; ++Segment)
		{
			const int32 From = Segment;
			const int32 To = (Segment + 1) % Vertices.Stations;
			for (int32 Frame = 0; Frame + 1 < Run.Num(); ++Frame)
			{
				Mesh.AddQuad(Vertices.Leaving[Vertices.Slot(Frame, From)], Vertices.Leaving[Vertices.Slot(Frame + 1, From)],
					Vertices.Arriving[Vertices.Slot(Frame + 1, To)], Vertices.Arriving[Vertices.Slot(Frame, To)]);
			}
		}
	}

	bool Cap(FMeshBuilder& Mesh, const FSweepFrame& Frame, const FProfileStations& Profile, bool bRunEnd)
	{
		if (!Profile.bClosed || Profile.Vertices.Num() < 3)
		{
			return false;
		}
		// A counter-clockwise profile faces out of the solid it bounds, so its cap faces away from the run; a clockwise
		// one faces into the void it bounds, so its cap faces into the run.
		const bool bOutward = Profile.SignedAreaSquareMeters() >= 0.0;
		const FVector Facing = (bRunEnd == bOutward) ? Frame.Along : -Frame.Along;
		const FVector AxisU = Frame.AxisA.GetSafeNormal();
		const FVector AxisV = (Frame.AxisB - AxisU * FVector::DotProduct(Frame.AxisB, AxisU)).GetSafeNormal();
		auto Place = [&Frame](const FVector2D& Point) { return Frame.At(Point); };
		auto UVOf = [&Frame, &AxisU, &AxisV](const FVector2D& Point)
		{
			const FVector Offset = Frame.At(Point) - Frame.Origin;
			return FVector2D(FVector::DotProduct(Offset, AxisU), FVector::DotProduct(Offset, AxisV));
		};
		return AddFlatPolygon(Mesh, Profile.Points(), {}, Place, Facing, UVOf);
	}
}
