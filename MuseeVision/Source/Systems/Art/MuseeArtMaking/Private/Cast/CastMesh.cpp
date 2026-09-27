#include "Cast/CastMesh.h"

#include "FormMesh.h"

namespace Musee::ArtMaking
{
	namespace
	{
		/** The points the triangles use: a mesh may keep points no triangle names, which are not the work. */
		TBitArray<> UsedPoints(const FArtScanMesh& Scan)
		{
			TBitArray<> Used(false, Scan.Points.Num());
			for (const FIntVector& Triangle : Scan.Triangles)
			{
				Used[Triangle.X] = true;
				Used[Triangle.Y] = true;
				Used[Triangle.Z] = true;
			}
			return Used;
		}

		/** The axis a face turns most towards: 0 for x, 1 for y, 2 for z. */
		int32 DominantAxis(const FVector& Face)
		{
			const FVector Size = Face.GetAbs();
			return Size.X >= Size.Y && Size.X >= Size.Z ? 0 : (Size.Y >= Size.Z ? 1 : 2);
		}

		/** A point's UV0 projected along an axis, metres, read as a viewer facing that side reads it (up is -v). */
		FVector2D Projected(const FVector& Point, int32 Axis)
		{
			switch (Axis)
			{
			case 0: return FVector2D(Point.Y, -Point.Z);
			case 1: return FVector2D(Point.X, -Point.Z);
			default: return FVector2D(Point.X, Point.Y);
			}
		}

		/** The part a triangle belongs to: the first band its centre's height lies in (1 up), or the body (0). */
		int32 PartOf(double Height, TConstArrayView<FCastBand> Bands)
		{
			for (int32 Band = 0; Band < Bands.Num(); ++Band)
			{
				if (Height >= Bands[Band].FromMeters && Height < Bands[Band].ToMeters)
				{
					return Band + 1;
				}
			}
			return 0;
		}
	}

	FCastFit FitCast(const FArtScanMesh& Scan, double HeightMeters, double BaseSliceMeters)
	{
		const TBitArray<> Used = UsedPoints(Scan);
		FBox Bounds(ForceInit);
		for (TConstSetBitIterator<> Point(Used); Point; ++Point)
		{
			Bounds += FVector(Scan.Points[Point.GetIndex()]);
		}
		FCastFit Fit;
		const double Height = Bounds.IsValid ? Bounds.Max.Z - Bounds.Min.Z : 0.0;
		if (Height <= 0.0 || HeightMeters <= 0.0)
		{
			return Fit;
		}
		Fit.Scale = HeightMeters / Height;
		const double SliceTop = Bounds.Min.Z + BaseSliceMeters / Fit.Scale;
		FBox Slice(ForceInit);
		for (TConstSetBitIterator<> Point(Used); Point; ++Point)
		{
			const FVector At(Scan.Points[Point.GetIndex()]);
			if (At.Z <= SliceTop)
			{
				Slice += At;
			}
		}
		const FVector SliceCentre = Slice.GetCenter();
		Fit.Foot = FVector(SliceCentre.X, SliceCentre.Y, Bounds.Min.Z);
		Fit.WidthMeters = (Bounds.Max.X - Bounds.Min.X) * Fit.Scale;
		Fit.DepthMeters = (Bounds.Max.Y - Bounds.Min.Y) * Fit.Scale;
		Fit.BaseMeters = FVector2D(Slice.Max.X - Slice.Min.X, Slice.Max.Y - Slice.Min.Y) * Fit.Scale;
		return Fit;
	}

	void BuildCast(const FArtScanMesh& Scan, const FCastFit& Fit, TConstArrayView<FCastBand> Bands, TArray<FFormMesh>& OutParts)
	{
		OutParts.Reset();
		OutParts.SetNum(1 + Bands.Num());
		// Each part's vertex for a point projected along an axis: a point on a projection's seam is two vertices.
		TArray<TArray<int32>> Made;
		Made.SetNum(OutParts.Num());
		for (TArray<int32>& Vertices : Made)
		{
			Vertices.Init(INDEX_NONE, Scan.Points.Num() * 3);
		}
		for (const FIntVector& Triangle : Scan.Triangles)
		{
			const FVector Corners[3] = { Fit.Place(Scan.Points[Triangle.X]), Fit.Place(Scan.Points[Triangle.Y]), Fit.Place(Scan.Points[Triangle.Z]) };
			const FVector Face = FVector::CrossProduct(Corners[1] - Corners[0], Corners[2] - Corners[0]);
			if (Face.IsNearlyZero(0.0))
			{
				continue;
			}
			const int32 Axis = DominantAxis(Face);
			const int32 Part = PartOf((Corners[0].Z + Corners[1].Z + Corners[2].Z) / 3.0, Bands);
			FFormMesh& Mesh = OutParts[Part];
			int32 Vertices[3];
			for (int32 Corner = 0; Corner < 3; ++Corner)
			{
				const int32 Point = Triangle[Corner];
				int32& Vertex = Made[Part][Point * 3 + Axis];
				if (Vertex == INDEX_NONE)
				{
					const FVector Normal(Scan.Normals[Point]);
					Vertex = Mesh.AddVertex(Corners[Corner], Normal.IsNearlyZero() ? Face.GetSafeNormal(0.0) : Normal, Projected(Corners[Corner], Axis));
				}
				Vertices[Corner] = Vertex;
			}
			Mesh.AddTriangle(Vertices[0], Vertices[1], Vertices[2]);
		}
	}
}
