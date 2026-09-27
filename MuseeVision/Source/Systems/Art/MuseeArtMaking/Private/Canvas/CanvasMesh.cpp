#include "Canvas/CanvasMesh.h"

#include "FormMesh.h"

namespace Musee::ArtMaking
{
	namespace
	{
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
		void Strip(FFormMesh& Sheet, const FStations& Stations, TFunctionRef<FVector(const FWallPath::FStation&, int32 Side)> Point,
			TFunctionRef<FVector(const FWallPath::FStation&)> Normal, TFunctionRef<FVector2D(double Along, int32 Side)> UV)
		{
			int32 Previous[2] = { INDEX_NONE, INDEX_NONE };
			for (int32 I = 0; I < Stations.At.Num(); ++I)
			{
				const FWallPath::FStation& At = Stations.At[I];
				const int32 Current[2] = { Sheet.AddVertex(Point(At, 0), Normal(At), UV(Stations.Along[I], 0)), Sheet.AddVertex(Point(At, 1), Normal(At), UV(Stations.Along[I], 1)) };
				if (I > 0)
				{
					Sheet.AddQuad(Previous[0], Current[0], Current[1], Previous[1]);
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

		FFormMesh Face;
		Strip(Face, Across, [&](const FWallPath::FStation& At, int32 Side) { return Height(At, StretcherDepthMeters, Side == 0 ? Bottom : Top); },
			[](const FWallPath::FStation& At) { return At.Out; },
			[&](double Along, int32 Side) { return FVector2D(Canvas.bReadsWithPath ? Along : Canvas.WidthMeters - Along, Side == 0 ? Canvas.HeightMeters : 0.0); });
		Face.Write(OutFace);

		FFormMesh Linen;
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
			const int32 A = Linen.AddVertex(Height(At, HangingGapMeters, Bottom), Normal, FVector2D(0.0, 0.0));
			const int32 B = Linen.AddVertex(Height(At, StretcherDepthMeters, Bottom), Normal, FVector2D(Depth, 0.0));
			const int32 C = Linen.AddVertex(Height(At, StretcherDepthMeters, Top), Normal, FVector2D(Depth, Canvas.HeightMeters));
			const int32 D = Linen.AddVertex(Height(At, HangingGapMeters, Top), Normal, FVector2D(0.0, Canvas.HeightMeters));
			Linen.AddQuad(A, B, C, D);
		}
		Linen.Write(OutLinen);
	}
}
