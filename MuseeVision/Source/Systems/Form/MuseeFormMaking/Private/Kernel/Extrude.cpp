#include "Kernel/Extrude.h"

#include "Kernel/MeshBuilder.h"
#include "Kernel/PlanarFace.h"
#include "Kernel/Profile.h"
#include "Kernel/Sweep.h"
#include "Kernel/SweepFrame.h"
#include "Kernel/Triangulation.h"

namespace Musee::Form
{
	namespace
	{
		bool IsLoop(const FProfileStations& Loop)
		{
			return Loop.bClosed && Loop.Vertices.Num() >= 3;
		}

		FSweepFrame ExtrusionFrame(const FPlaneFrame& Plane, const FVector& Along, const FVector& Offset, double U)
		{
			FSweepFrame F;
			F.Origin = Plane.Origin + Offset;
			F.AxisA = Plane.AxisU;
			F.AxisB = Plane.AxisV;
			F.NormalA = Plane.AxisU;
			F.NormalB = Plane.AxisV;
			F.Along = Along;
			F.U = U;
			return F;
		}
	}

	bool Extrude(FMeshBuilder& Mesh, const FPlaneFrame& Plane, const FProfileStations& Outline,
		TConstArrayView<FProfileStations> Holes, double DepthMeters, bool bFaces)
	{
		if (!IsLoop(Outline) || DepthMeters == 0.0)
		{
			return false;
		}
		const FProfileStations Outer = Outline.SignedAreaSquareMeters() >= 0.0 ? Outline : Outline.Reversed();
		TArray<FProfileStations> Inner;
		TArray<TArray<FVector2D>> HolePoints;
		for (const FProfileStations& Hole : Holes)
		{
			if (!IsLoop(Hole))
			{
				return false;
			}
			Inner.Add(Hole.SignedAreaSquareMeters() <= 0.0 ? Hole : Hole.Reversed());
			HolePoints.Add(Hole.Points());
		}
		const TArray<FVector2D> OuterPoints = Outer.Points();
		TArray<FIntVector> Triangles;
		if (!TriangulatePolygon(OuterPoints, HolePoints, Triangles))
		{
			return false;
		}

		const FVector Push = Plane.Normal * DepthMeters;
		const FVector Along = Push.GetSafeNormal();
		const FSweepFrame Run[2] = {ExtrusionFrame(Plane, Along, FVector::ZeroVector, 0.0),
			ExtrusionFrame(Plane, Along, Push, FMath::Abs(DepthMeters))};
		Sweep(Mesh, Run, Outer);
		for (const FProfileStations& Hole : Inner)
		{
			Sweep(Mesh, Run, Hole);
		}
		if (bFaces)
		{
			auto UVOf = [](const FVector2D& Point) { return Point; };
			auto Near = [&Plane](const FVector2D& Point) { return Plane.At(Point); };
			auto Far = [&Plane, &Push](const FVector2D& Point) { return Plane.At(Point) + Push; };
			AddFlatPolygon(Mesh, OuterPoints, HolePoints, Near, -Along, UVOf);
			AddFlatPolygon(Mesh, OuterPoints, HolePoints, Far, Along, UVOf);
		}
		return true;
	}
}
