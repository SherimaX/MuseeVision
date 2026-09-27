#include "GroundsParts.h"
#include "GroundsSettingOut.h"

/**
 * The gravel walks (§3.4): the museum's own travertine offcuts crushed and bound as a stabilisé, top +0.018, bottom
 * -0.120, laid between the kerbs' faces (60 mm either side of their centre lines) and against the copings' outer faces.
 * The regions meet without overlaps or staggered tops: the side walks, the allées, the cross walks north and south of
 * the canal, the canal walks (named apart, as they carry the exterior-walkway target), the avenue, the walk east round
 * Albion's south end, the rond-point's ring, the path north and the Élan's terrace. The walk east and the path north
 * end on the ring's circle, and the path starts on the terrace's, each at its kerb's face. The canal walks leave a round
 * hole for each lamp standard's base.
 */
namespace Musee::Grounds
{
	namespace
	{
		const Musee::Collision::FClass* const Walkable = &Musee::Collision::Walkable;

		constexpr double K = Plan::KerbHalf;

		void Walk(TArray<FFormDrawing>& Out, const TCHAR* Name, FFormProfile Outline, TArray<FFormProfile> Holes = {},
			const TCHAR* LocalSurface = TEXT("gravel"))
		{
			Out.Add(Laid(FString::Printf(TEXT("gravel.%s"), Name), LocalSurface, MoveTemp(Outline), MoveTemp(Holes), Plan::GravelBottom,
				Plan::GravelTop, Walkable));
		}

		/** The front: the side walks, the allées, the cross walks, the canal walks and the avenue. */
		void Front(TArray<FFormDrawing>& Out)
		{
			const double Axis = Plan::AxisX;
			const double PanelIn = Plan::PanelInner;
			const double PanelOut = Plan::PanelOuter;
			const double Allee0 = Plan::CourWestX + K;
			const double Allee1 = Axis - PanelOut - K;
			Walk(Out, TEXT("side-walk.west"), Rectangle({Allee0, Plan::SalonStripEdgeY}, {Plan::ParvisWestX - K, Plan::ParvisSouthY - K}));
			Walk(Out, TEXT("side-walk.east"), Rectangle({Plan::ParvisEastX + K, Plan::SalonStripEdgeY}, {Plan::CourEastX - K, Plan::ParvisSouthY - K}));
			Walk(Out, TEXT("allee.west"), Rectangle({Allee0, Plan::ParvisSouthY + K}, {Allee1, Plan::CourSouthY - K}));
			Walk(Out, TEXT("allee.east"), Rectangle({Axis + PanelOut + K, Plan::ParvisSouthY + K}, {Plan::CourEastX - K, Plan::CourSouthY - K}));

			// North of the panels and the canal, to the panels' kerbs and the canal's copings and walks at y 27.
			const double PanelNorth = Plan::PanelNorthY - K;
			Walk(Out, TEXT("cross.north"), Polygon({FVector2D(Allee1, Plan::ParvisSouthY + K), FVector2D(Axis + PanelOut + K, Plan::ParvisSouthY + K),
				FVector2D(Axis + PanelOut + K, PanelNorth), FVector2D(Axis + PanelIn - K, PanelNorth), FVector2D(Axis + PanelIn - K, Plan::CanalNorthY),
				FVector2D(Axis - PanelIn + K, Plan::CanalNorthY), FVector2D(Axis - PanelIn + K, PanelNorth), FVector2D(Allee1, PanelNorth)}));

			// South of them: from the panels' kerbs (y 55) and the canal (y 57) to the cour's kerb, open to the avenue.
			const double PanelSouth = Plan::PanelSouthY + K;
			Walk(Out, TEXT("cross.south"), Polygon({FVector2D(Allee1, PanelSouth), FVector2D(Axis - PanelIn, PanelSouth),
				FVector2D(Axis - PanelIn, Plan::CanalSouthY), FVector2D(Axis + PanelIn, Plan::CanalSouthY), FVector2D(Axis + PanelIn, PanelSouth),
				FVector2D(Axis + PanelOut + K, PanelSouth), FVector2D(Axis + PanelOut + K, Plan::CourSouthY - K), FVector2D(Allee1, Plan::CourSouthY - K)}));

			// The canal walks |x + 44| 4.50 ... 7.50, y 27 ... 57, beside the panels' kerbs.
			for (const double Side : {-1.0, 1.0})
			{
				const double Coping = Side < 0.0 ? Plan::CanalWestX : Plan::CanalEastX;
				const double Kerb = Axis + Side * (PanelIn - K);
				const double Line = Axis + Side * PanelIn;
				TArray<FVector2D> Corners = {FVector2D(Kerb, Plan::CanalNorthY), FVector2D(Coping, Plan::CanalNorthY), FVector2D(Coping, Plan::CanalSouthY),
					FVector2D(Line, Plan::CanalSouthY), FVector2D(Line, PanelSouth), FVector2D(Kerb, PanelSouth)};
				const FBox2D Region(FVector2D(FMath::Min(Kerb, Coping), Plan::CanalNorthY), FVector2D(FMath::Max(Kerb, Coping), Plan::CanalSouthY));
				Walk(Out, Side < 0.0 ? TEXT("canal-walk.west") : TEXT("canal-walk.east"), Polygon(Corners), LampBasesWithin(Region),
					TEXT("gravel.canal-walks"));
			}
			Walk(Out, TEXT("avenue"), Rectangle({Plan::AvenueWestX + K, Plan::CourSouthY - K}, {Plan::AvenueEastX - K, Plan::AvenueSouthY}));
		}

		/**
		 * The walk east: the band between its outer and inner kerbs' faces, from the cour round Albion's south end to the
		 * rond-point, its end the arc of the ring's kerb face (r 6.94) in the ring's gap.
		 */
		void WalkEast(TArray<FFormDrawing>& Out)
		{
			const FVector2D Ring = At(Plan::RondPoint);
			const double Face = Plan::RingRadius - K;
			const FVector2D OuterEnd = RingWestAt(Plan::WalkOuterY - K, Face);
			const FVector2D InnerEnd = RingWestAt(Plan::WalkInnerY + K, Face);
			FFormProfile Outline = FFormProfile::From(FVector2D(Plan::CourEastX - K, Plan::WalkOuterY - K));
			for (const FVector2D& Corner : {FVector2D(-Plan::WalkBendOuterX + K, Plan::WalkOuterY - K), FVector2D(-Plan::WalkBendOuterX + K, Plan::WalkSouthOuterY - K),
				FVector2D(Plan::WalkBendOuterX - K, Plan::WalkSouthOuterY - K), FVector2D(Plan::WalkBendOuterX - K, Plan::WalkOuterY - K), OuterEnd})
			{
				Outline.LineTo(Corner, EFormJoint::Crisp);
			}
			// Round the ring's face by the west, from the outer kerb's end to the inner's.
			Outline.ArcAbout(Ring, Face, Face, AngleOf(Ring, OuterEnd), AngleOf(Ring, InnerEnd) + 360.0, EFormJoint::Crisp);
			for (const FVector2D& Corner : {FVector2D(Plan::WalkBendInnerX + K, Plan::WalkInnerY + K), FVector2D(Plan::WalkBendInnerX + K, Plan::WalkSouthInnerY + K),
				FVector2D(-Plan::WalkBendInnerX - K, Plan::WalkSouthInnerY + K), FVector2D(-Plan::WalkBendInnerX - K, Plan::WalkInnerY + K),
				FVector2D(Plan::CourEastX - K, Plan::WalkInnerY + K)})
			{
				Outline.LineTo(Corner, EFormJoint::Crisp);
			}
			Walk(Out, TEXT("walk-east"), Outline.Close());
		}

		/** The rond-point's ring, the path north and the Élan's terrace, each between its kerbs' faces and the basin's coping. */
		void Rounds(TArray<FFormDrawing>& Out)
		{
			const FVector2D Ring = At(Plan::RondPoint);
			const FVector2D Elan = At(Plan::ElanAxis);
			const double RingFace = Plan::RingRadius - K;
			const double TerraceFace = Plan::TerraceOuterRadius - K;
			Walk(Out, TEXT("ring"), Circle(Ring, RingFace), {Circle(Ring, Plan::BasinOuterRadius)});

			// The path north, from the terrace kerb's face to the ring's, each end an arc of its circle.
			const FVector2D NorthWest = TerraceSouthAt(Plan::PathWestX + K, TerraceFace);
			const FVector2D NorthEast = TerraceSouthAt(Plan::PathEastX - K, TerraceFace);
			const FVector2D SouthWest = RingNorthAt(Plan::PathWestX + K, RingFace);
			const FVector2D SouthEast = RingNorthAt(Plan::PathEastX - K, RingFace);
			FFormProfile Path = FFormProfile::From(NorthWest);
			Path.LineTo(SouthWest, EFormJoint::Crisp);
			Path.ArcAbout(Ring, RingFace, RingFace, AngleOf(Ring, SouthWest) + 360.0, AngleOf(Ring, SouthEast) + 360.0, EFormJoint::Crisp);
			Path.LineTo(NorthEast, EFormJoint::Crisp);
			Path.ArcAbout(Elan, TerraceFace, TerraceFace, AngleOf(Elan, NorthEast), AngleOf(Elan, NorthWest), EFormJoint::Crisp);
			Walk(Out, TEXT("path-north"), Path.Close());

			// The terrace, from 0.02 off the socle to the outer kerb's face, its ends on the radial end kerbs' faces.
			auto EndAngle = [](double Radius, double Sense) { return Sense * (Plan::TerraceEndDeg - FMath::RadiansToDegrees(FMath::Asin(K / Radius))); };
			const double Inner = Plan::TerraceInnerRadius;
			FFormProfile Terrace = FFormProfile::From(OnCircle(Elan, Inner, EndAngle(Inner, -1.0)));
			Terrace.ArcAbout(Elan, Inner, Inner, EndAngle(Inner, -1.0), EndAngle(Inner, 1.0), EFormJoint::Crisp);
			Terrace.LineTo(OnCircle(Elan, TerraceFace, EndAngle(TerraceFace, 1.0)), EFormJoint::Crisp);
			Terrace.ArcAbout(Elan, TerraceFace, TerraceFace, EndAngle(TerraceFace, 1.0), EndAngle(TerraceFace, -1.0), EFormJoint::Crisp);
			Walk(Out, TEXT("terrace"), Terrace.Close());
		}
	}

	void DrawGravel(TArray<FFormDrawing>& Out)
	{
		Front(Out);
		WalkEast(Out);
		Rounds(Out);
	}
}
