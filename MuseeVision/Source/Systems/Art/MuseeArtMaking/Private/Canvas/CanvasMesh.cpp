#include "Canvas/CanvasMesh.h"

#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "DynamicMesh/MeshTangents.h"
#include "MuseeUnits.h"

namespace Musee::ArtMaking
{
	namespace
	{
		/**
		 * A canvas surface being drawn: its corners (metres, in the piece frame, with the normal of the face they belong
		 * to and UVs in metres) and its quads, written as a made piece's mesh: normals, tangents from UV0, UV0 and the
		 * piece layout, white colours, slot 0.
		 */
		class FSheet
		{
		public:
			int32 Corner(const FVector& Point, const FVector& Normal, const FVector2D& UV)
			{
				Points.Add(Point);
				Normals.Add(Normal.GetSafeNormal());
				UVs.Add(UV);
				return Points.Num() - 1;
			}

			/** A quad A, B, C, D round its edge, wound to face along its corners' normal. */
			void Quad(int32 A, int32 B, int32 C, int32 D)
			{
				// The engine's front face is the one whose normal is cross(c - a, b - a).
				const bool bFacing = FVector::DotProduct(FVector::CrossProduct(Points[C] - Points[A], Points[B] - Points[A]), Normals[A]) >= 0.0;
				Triangles.Add(bFacing ? FIntVector(A, B, C) : FIntVector(A, C, B));
				Triangles.Add(bFacing ? FIntVector(A, C, D) : FIntVector(A, D, C));
			}

			void Write(UE::Geometry::FDynamicMesh3& Out) const
			{
				using namespace UE::Geometry;
				Out = FDynamicMesh3();
				Out.EnableAttributes();
				FDynamicMeshAttributeSet& Attributes = *Out.Attributes();
				Attributes.SetNumUVLayers(2);
				Attributes.EnablePrimaryColors();
				Attributes.EnableMaterialID();
				for (int32 V = 0; V < Points.Num(); ++V)
				{
					Out.AppendVertex(ToEngine(Points[V]));
					Attributes.PrimaryNormals()->AppendElement(FVector3f(Normals[V]));
					Attributes.GetUVLayer(0)->AppendElement(FVector2f(UVs[V]));
					Attributes.GetUVLayer(1)->AppendElement(FVector2f::ZeroVector);
					Attributes.PrimaryColors()->AppendElement(FVector4f(1.0f, 1.0f, 1.0f, 1.0f));
				}
				for (const FIntVector& T : Triangles)
				{
					const FIndex3i Corners(T.X, T.Y, T.Z);
					const int32 Id = Out.AppendTriangle(Corners);
					Attributes.PrimaryNormals()->SetTriangle(Id, Corners);
					Attributes.GetUVLayer(0)->SetTriangle(Id, Corners);
					Attributes.GetUVLayer(1)->SetTriangle(Id, Corners);
					Attributes.PrimaryColors()->SetTriangle(Id, Corners);
					Attributes.GetMaterialID()->SetValue(Id, 0);
				}
				FMeshTangentsd::ComputeDefaultOverlayTangents(Out);
			}

		private:
			TArray<FVector> Points;
			TArray<FVector> Normals;
			TArray<FVector2D> UVs;
			TArray<FIntVector> Triangles;
		};

		/** The wall's stations across the canvas, in the piece frame. */
		struct FStations
		{
			TArray<double> Along;
			TArray<FWallPath::FStation> At;
		};

		FStations Stations(const FWallPath& Wall, const FCanvasSpec& Canvas, const FMuseeFrame& Piece)
		{
			FStations Out;
			const int32 Count = FMath::Max(1, FMath::CeilToInt(Canvas.WidthMeters / Wall.StationSpacing(Canvas.WidthMeters)));
			for (int32 I = 0; I <= Count; ++I)
			{
				const double S = Canvas.StartMeters + Canvas.WidthMeters * I / Count;
				FWallPath::FStation Station = Wall.At(S);
				Station.Point = Piece.PointFromParent(Station.Point);
				Station.Along = Piece.DirectionFromParent(Station.Along);
				Station.Out = Piece.DirectionFromParent(Station.Out);
				Out.Along.Add(S - Canvas.StartMeters);
				Out.At.Add(Station);
			}
			return Out;
		}

		/** A strip along the canvas between two offsets from the wall at one height, or two heights at one offset. */
		void Strip(FSheet& Sheet, const FStations& Stations, TFunctionRef<FVector(const FWallPath::FStation&, int32 Side)> Point,
			TFunctionRef<FVector(const FWallPath::FStation&)> Normal, TFunctionRef<FVector2D(double Along, int32 Side)> UV)
		{
			int32 Previous[2] = { INDEX_NONE, INDEX_NONE };
			for (int32 I = 0; I < Stations.At.Num(); ++I)
			{
				const FWallPath::FStation& At = Stations.At[I];
				const int32 Current[2] = { Sheet.Corner(Point(At, 0), Normal(At), UV(Stations.Along[I], 0)), Sheet.Corner(Point(At, 1), Normal(At), UV(Stations.Along[I], 1)) };
				if (I > 0)
				{
					Sheet.Quad(Previous[0], Current[0], Current[1], Previous[1]);
				}
				Previous[0] = Current[0];
				Previous[1] = Current[1];
			}
		}
	}

	void BuildCanvas(const FWallPath& Wall, const FCanvasSpec& Canvas, const FMuseeFrame& PieceFrame, UE::Geometry::FDynamicMesh3& OutFace,
		UE::Geometry::FDynamicMesh3& OutLinen)
	{
		const FStations Across = Stations(Wall, Canvas, PieceFrame);
		const FVector Up = PieceFrame.DirectionFromParent(FVector::UpVector);
		const double Bottom = Canvas.BottomMeters;
		const double Top = Canvas.BottomMeters + Canvas.HeightMeters;
		auto Height = [&Up](const FWallPath::FStation& At, double Offset, double Z) { return At.Point + At.Out * Offset + Up * Z; };

		FSheet Face;
		Strip(Face, Across, [&](const FWallPath::FStation& At, int32 Side) { return Height(At, StretcherDepthMeters, Side == 0 ? Bottom : Top); },
			[](const FWallPath::FStation& At) { return At.Out; },
			[&](double Along, int32 Side) { return FVector2D(Canvas.bReadsWithPath ? Along : Canvas.WidthMeters - Along, Side == 0 ? Canvas.HeightMeters : 0.0); });
		Face.Write(OutFace);

		FSheet Linen;
		const double Depth = StretcherDepthMeters - HangingGapMeters;
		for (const double Z : { Bottom, Top })
		{
			Strip(Linen, Across, [&](const FWallPath::FStation& At, int32 Side) { return Height(At, Side == 0 ? HangingGapMeters : StretcherDepthMeters, Z); },
				[&](const FWallPath::FStation&) { return Z == Bottom ? -Up : Up; },
				[&](double Along, int32 Side) { return FVector2D(Along, Side * Depth); });
		}
		Strip(Linen, Across, [&](const FWallPath::FStation& At, int32 Side) { return Height(At, HangingGapMeters, Side == 0 ? Bottom : Top); },
			[](const FWallPath::FStation& At) { return -At.Out; },
			[&](double Along, int32 Side) { return FVector2D(Along, Side * Canvas.HeightMeters); });
		for (int32 End = 0; End < 2; ++End)
		{
			const FWallPath::FStation& At = End == 0 ? Across.At[0] : Across.At.Last();
			const FVector Normal = End == 0 ? -At.Along : At.Along;
			const int32 A = Linen.Corner(Height(At, HangingGapMeters, Bottom), Normal, FVector2D(0.0, 0.0));
			const int32 B = Linen.Corner(Height(At, StretcherDepthMeters, Bottom), Normal, FVector2D(Depth, 0.0));
			const int32 C = Linen.Corner(Height(At, StretcherDepthMeters, Top), Normal, FVector2D(Depth, Canvas.HeightMeters));
			const int32 D = Linen.Corner(Height(At, HangingGapMeters, Top), Normal, FVector2D(0.0, Canvas.HeightMeters));
			Linen.Quad(A, B, C, D);
		}
		Linen.Write(OutLinen);
	}
}
