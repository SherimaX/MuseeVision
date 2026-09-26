#include "Chenghuai/ChenghuaiWater.h"

#include "Chenghuai/ChenghuaiGardenPlan.h"
#include "Chenghuai/ChenghuaiKit.h"
#include "Chenghuai/ChenghuaiPlan.h"

namespace ChenghuaiWater
{
	TArray<FVector2D> PondOutline()
	{
		TArray<FVector2D> Out;
		for (int32 i = 0; i < ChenghuaiGardenPlan::PondEdgeCount; ++i)
		{
			FVector2D P(ChenghuaiGardenPlan::PondEdge[i][0], ChenghuaiGardenPlan::PondEdge[i][1]);
			// Under the water pavilion (x 13 … 16.4, y −50.6 … −46.1) the pond reaches in to x 14.3: its west half stands in
			// the water on stone piers.
			if (P.Y < -46.0 && P.Y > -50.7 && P.X > 12.5)
			{
				const double T = FMath::Clamp(FMath::Min(P.Y + 50.7, -46.0 - P.Y) / 0.6, 0.0, 1.0);
				P.X = FMath::Lerp(P.X, 14.3, T);
			}
			Out.Add(P);
		}
		return Out;
	}

	SalonKit::FMeshData BuildWater()
	{
		SalonKit::FMeshData M;
		const TArray<FVector2D> Edge = PondOutline();
		// 4 cm out past the edge: the bank's lip covers the join.
		TArray<FVector2D> Wider;
		const int32 N = Edge.Num();
		FVector2D C = FVector2D::ZeroVector;
		for (const FVector2D& P : Edge) { C += P; }
		C /= N;
		for (int32 i = 0; i < N; ++i)
		{
			const FVector2D A = Edge[(i + N - 1) % N], B = Edge[(i + 1) % N];
			FVector2D T = (B - A).GetSafeNormal();
			FVector2D Nrm(T.Y, -T.X);
			if (ChenghuaiKit::Area2(Edge) < 0.0) { Nrm = -Nrm; }
			Wider.Add(Edge[i] + Nrm * 0.04);
		}
		ChenghuaiKit::Level(M, Wider, {}, Chenghuai::WaterLevel, true);
		return M;
	}
}
