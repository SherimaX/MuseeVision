#include "GroundsSettingOut.h"

#include "GroundsParts.h"

namespace Musee::Grounds
{
	FVector2D RingWestAt(double Y, double Radius)
	{
		const FVector2D Centre = At(Plan::RondPoint);
		return FVector2D(Centre.X - FMath::Sqrt(FMath::Square(Radius) - FMath::Square(Y - Centre.Y)), Y);
	}

	FVector2D RingNorthAt(double X, double Radius)
	{
		const FVector2D Centre = At(Plan::RondPoint);
		return FVector2D(X, Centre.Y - FMath::Sqrt(FMath::Square(Radius) - FMath::Square(X - Centre.X)));
	}

	FVector2D TerraceSouthAt(double X, double Radius)
	{
		const FVector2D Axis = At(Plan::ElanAxis);
		return FVector2D(X, Axis.Y + FMath::Sqrt(FMath::Square(Radius) - FMath::Square(X - Axis.X)));
	}

	TArray<TPair<FString, FRun>> KerbRuns()
	{
		const FVector2D Ring = At(Plan::RondPoint);
		const FVector2D Elan = At(Plan::ElanAxis);
		const double EndInner = Plan::TerraceEndKerbFrom;
		const double Face = Plan::KerbHalf;
		TArray<TPair<FString, FRun>> Runs;

		// The lawn panels' loops, from the corner nearest the canal.
		for (const double Side : {-1.0, 1.0})
		{
			const double Inner = Plan::AxisX + Side * Plan::PanelInner;
			const double Outer = Plan::AxisX + Side * Plan::PanelOuter;
			const FVector2D Loop[] = {{Inner, Plan::PanelNorthY}, {Outer, Plan::PanelNorthY}, {Outer, Plan::PanelSouthY}, {Inner, Plan::PanelSouthY}};
			Runs.Emplace(Side < 0.0 ? TEXT("panel.west") : TEXT("panel.east"), FRun::Through(Loop, true));
		}

		// The parvis's kerbs stop on the cour's kerbs' faces (x -64, x -24) and on parvis.south's (y 23).
		Runs.Emplace(TEXT("parvis.west"), FRun::Through({FVector2D(Plan::ParvisWestX, Plan::SalonStripEdgeY), FVector2D(Plan::ParvisWestX, Plan::ParvisSouthY - Face)}, false));
		Runs.Emplace(TEXT("parvis.east"), FRun::Through({FVector2D(Plan::ParvisEastX, Plan::SalonStripEdgeY), FVector2D(Plan::ParvisEastX, Plan::ParvisSouthY - Face)}, false));
		Runs.Emplace(TEXT("parvis.south"), FRun::Through({FVector2D(Plan::CourWestX + Face, Plan::ParvisSouthY), FVector2D(Plan::CourEastX - Face, Plan::ParvisSouthY)}, false));

		// cour.west, turning into avenue.west.
		Runs.Emplace(TEXT("cour.west"), FRun::Through({FVector2D(Plan::CourWestX, Plan::SalonStripEdgeY), FVector2D(Plan::CourWestX, Plan::CourSouthY),
			FVector2D(Plan::AvenueWestX, Plan::CourSouthY), FVector2D(Plan::AvenueWestX, Plan::AvenueSouthY)}, false));

		// avenue.east turning into cour.east-south, which turns into walk-east.outer, the ring's long arc round by the south
		// and east, path-north.east, and the terrace's outer arc back round by the east to its north end.
		FRun Outer = FRun::Through({FVector2D(Plan::AvenueEastX, Plan::AvenueSouthY), FVector2D(Plan::AvenueEastX, Plan::CourSouthY),
			FVector2D(Plan::CourEastX, Plan::CourSouthY), FVector2D(Plan::CourEastX, Plan::WalkOuterY), FVector2D(-Plan::WalkBendOuterX, Plan::WalkOuterY),
			FVector2D(-Plan::WalkBendOuterX, Plan::WalkSouthOuterY), FVector2D(Plan::WalkBendOuterX, Plan::WalkSouthOuterY),
			FVector2D(Plan::WalkBendOuterX, Plan::WalkOuterY), RingWestAt(Plan::WalkOuterY, Plan::RingRadius)}, false);
		Outer.ArcTo(Ring, AngleOf(Ring, RingNorthAt(Plan::PathEastX, Plan::RingRadius)), false);
		Outer.LineTo(TerraceSouthAt(Plan::PathEastX, Plan::TerraceOuterRadius));
		Outer.ArcTo(Elan, -Plan::TerraceEndDeg, false);
		Outer.LineTo(OnCircle(Elan, EndInner, -Plan::TerraceEndDeg));
		Runs.Emplace(TEXT("walk-east.outer"), MoveTemp(Outer));

		// cour.east-north turning into walk-east.inner, the ring's short arc by the north-west, path-north.west, and the
		// terrace's outer arc to its south end.
		FRun Inner = FRun::Through({FVector2D(Plan::CourEastX, Plan::SalonStripEdgeY), FVector2D(Plan::CourEastX, Plan::WalkInnerY),
			FVector2D(-Plan::WalkBendInnerX, Plan::WalkInnerY), FVector2D(-Plan::WalkBendInnerX, Plan::WalkSouthInnerY),
			FVector2D(Plan::WalkBendInnerX, Plan::WalkSouthInnerY), FVector2D(Plan::WalkBendInnerX, Plan::WalkInnerY), RingWestAt(Plan::WalkInnerY, Plan::RingRadius)}, false);
		Inner.ArcTo(Ring, AngleOf(Ring, RingNorthAt(Plan::PathWestX, Plan::RingRadius)) + 360.0, true);
		Inner.LineTo(TerraceSouthAt(Plan::PathWestX, Plan::TerraceOuterRadius));
		Inner.ArcTo(Elan, Plan::TerraceEndDeg, true);
		Inner.LineTo(OnCircle(Elan, EndInner, Plan::TerraceEndDeg));
		Runs.Emplace(TEXT("walk-east.inner"), MoveTemp(Inner));
		return Runs;
	}
}
