#include "SalonParts.h"
#include "SalonPlan.h"

/**
 * The fittings (§3.7): the eight backless velvet benches on their fumed oak plinths, the frameless glass stele for
 * Impression, Sunrise on its bronze foot, the 1874 case in the Manet cabinet (a panelled oak carcase under a glass hood
 * bound in bronze angles, its felt floor and Le Charivari's felt board), and the Little Dancer's travertine plinth and
 * glass vitrine.
 */
namespace Musee::Salon
{
	namespace
	{
		/** A rectangle's section (A across, B up) standing on B = 0, its two top arrises eased to a radius. */
		FFormProfile EasedTop(double Half, double Height, double Radius)
		{
			FFormProfile S = FFormProfile::From(FVector2D(-Half, 0.0));
			S.LineTo(FVector2D(Half, 0.0), EFormJoint::Crisp).LineTo(FVector2D(Half, Height - Radius), EFormJoint::Crisp)
				.ArcAbout(FVector2D(Half - Radius, Height - Radius), Radius, Radius, 0.0, 90.0, EFormJoint::Smooth)
				.ArcAbout(FVector2D(-Half + Radius, Height - Radius), Radius, Radius, 90.0, 180.0, EFormJoint::Smooth);
			return S.Close();
		}

		/** A round bar's section of a diameter about a point of (A, B). */
		FFormProfile Cord(const FVector2D& Centre, double Diameter)
		{
			return FFormProfile::Ellipse(Centre, 0.5 * Diameter, 0.5 * Diameter, 0.0, true);
		}

		/** A plan's rounded rectangle about a centre, walked anticlockwise (so what is swept on it stands outside). */
		FFormProfile RoundedLoop(const FVector2D& Centre, double HalfX, double HalfY, double Radius)
		{
			const double X = HalfX - Radius;
			const double Y = HalfY - Radius;
			FFormProfile Loop = FFormProfile::From(Centre + FVector2D(X, -HalfY));
			Loop.LineTo(Centre + FVector2D(-X, -HalfY), EFormJoint::Smooth)
				.ArcAbout(Centre + FVector2D(-X, -Y), Radius, Radius, 270.0, 180.0, EFormJoint::Smooth)
				.LineTo(Centre + FVector2D(-HalfX, Y), EFormJoint::Smooth)
				.ArcAbout(Centre + FVector2D(-X, Y), Radius, Radius, 180.0, 90.0, EFormJoint::Smooth)
				.LineTo(Centre + FVector2D(X, HalfY), EFormJoint::Smooth)
				.ArcAbout(Centre + FVector2D(X, Y), Radius, Radius, 90.0, 0.0, EFormJoint::Smooth)
				.LineTo(Centre + FVector2D(HalfX, -Y), EFormJoint::Smooth)
				.ArcAbout(Centre + FVector2D(X, -Y), Radius, Radius, 0.0, -90.0, EFormJoint::Smooth);
			return Loop.Close();
		}

		/**
		 * One bench, its long axis along x. The plinth is set back under the velvet; the velvet's edge, rolled at its top
		 * and bottom and round its ends in plan, is swept round a loop inside its outline; the quilted top fills the loop,
		 * and the piping runs round the top and bottom and across the three seams.
		 */
		void Bench(TArray<FFormDrawing>& Out, int32 BenchBay, double X, double Y, const TCHAR* Side)
		{
			const FString Name = FString::Printf(TEXT("bench.bay%d.%s"), BenchBay + 1, Side);
			const FString Velvet = FString::Printf(TEXT("bench.bay%d.velvet"), BenchBay + 1);
			const double HalfL = 0.5 * Plan::BenchLength;
			const double HalfW = 0.5 * Plan::BenchWidth;

			FFormSweep Plinth;
			Plinth.Section = EasedTop(HalfW - Plan::PlinthSetBack, Plan::PlinthHeight, Plan::PlinthArris);
			Plinth.Path = FFormProfile::Polyline({FVector2D(X - HalfL + Plan::PlinthSetBack, Y), FVector2D(X + HalfL - Plan::PlinthSetBack, Y)}, false);
			Out.Add(Drawn(Name + TEXT(".plinth"), TEXT("bench.plinth"), MoveTemp(Plinth), Solid));

			// The edge is swept round a loop inside the outline, whose corners keep the ends' radius less that reach; its
			// section runs from just short of those corners' centres out to the outline, the top roll meeting the flat top.
			const double Reach = Plan::EdgeReach;
			const double Corner = Plan::TopRoll - Reach;
			const double Inside = Corner - Plan::Sink;
			const double Low = Plan::PlinthHeight;
			const double Bottom = Plan::BottomRoll;
			const double Top = Plan::TopRoll;
			const double Crown = FMath::RadiansToDegrees(FMath::Acos((Top - Reach - Inside) / Top));
			FFormProfile Edge = FFormProfile::From(FVector2D(-Inside, Low));
			Edge.LineTo(FVector2D(Reach - Bottom, Low), EFormJoint::Crisp)
				.ArcAbout(FVector2D(Reach - Bottom, Low + Bottom), Bottom, Bottom, -90.0, 0.0, EFormJoint::Smooth)
				.LineTo(FVector2D(Reach, Plan::FlatTop - Top), EFormJoint::Smooth)
				.ArcAbout(FVector2D(Reach - Top, Plan::FlatTop - Top), Top, Top, 0.0, Crown, EFormJoint::Smooth);
			const FVector2D Centre(X, Y);
			FFormSweep Roll;
			Roll.Section = Edge.Close();
			Roll.Path = RoundedLoop(Centre, HalfL - Reach, HalfW - Reach, Corner);
			Out.Add(Drawn(Name + TEXT(".velvet"), *Velvet, MoveTemp(Roll), Solid));

			// The quilted top: four panels along the bench, each domed.
			const double CushionX = HalfL - Reach - Inside + Plan::Sink;
			const double CushionY = HalfW - Reach - Inside + Plan::Sink;
			FFormRelief Cushion;
			Cushion.SizeMeters = FVector2D(2.0 * CushionX, 2.0 * CushionY);
			Cushion.GridMeters = Plan::QuiltGrid;
			Cushion.BackMeters = Plan::FlatTop - Plan::PlinthHeight;
			const double Panel = 2.0 * CushionX / Plan::QuiltPanels;
			Cushion.HeightMeters = [Panel, CushionY](double U, double V)
			{
				const double S = FMath::Fractional(U / Panel);
				const double T = V / (2.0 * CushionY);
				return Plan::QuiltDome * FMath::Max(0.0, 1.0 - FMath::Square(2.0 * S - 1.0)) * FMath::Max(0.0, 1.0 - FMath::Square(2.0 * T - 1.0));
			};
			Out.Add(Drawn(Name + TEXT(".cushion"), *Velvet, MoveTemp(Cushion), Solid,
				FrameOfAxes(FVector(X - CushionX, Y - CushionY, Plan::FlatTop), FMuseeFrame::East(), FMuseeFrame::South())));

			const TPair<const TCHAR*, FVector2D> Pipings[] = {{TEXT("top"), FVector2D(-Inside, Plan::FlatTop)}, {TEXT("bottom"), FVector2D(Reach - Bottom, Low)}};
			for (const TPair<const TCHAR*, FVector2D>& Piping : Pipings)
			{
				FFormSweep Run;
				Run.Section = Cord(Piping.Value, Plan::Piping);
				Run.Path = RoundedLoop(Centre, HalfL - Reach, HalfW - Reach, Corner);
				Out.Add(Drawn(Name + TEXT(".piping.") + Piping.Key, *Velvet, MoveTemp(Run), nullptr));
			}
			for (int32 Seam = 1; Seam < Plan::QuiltPanels; ++Seam)
			{
				const double SeamX = X - CushionX + Panel * Seam;
				FFormSweep Run;
				Run.Section = Cord(FVector2D(0.0, Plan::FlatTop), Plan::Piping);
				Run.Path = FFormProfile::Polyline({FVector2D(SeamX, Y - CushionY), FVector2D(SeamX, Y + CushionY)}, false);
				Out.Add(Drawn(FString::Printf(TEXT("%s.piping.seam.%d"), *Name, Seam), *Velvet, MoveTemp(Run), nullptr));
			}
		}

		/** The stele: the glass on its bronze foot, whose long top arrises are eased. */
		void Stele(TArray<FFormDrawing>& Out)
		{
			const double Half = 0.5 * Plan::SteleThickness;
			FFormDrawing Glass = Box(TEXT("stele.glass"), TEXT("stele.glass"), FVector(Plan::SteleX - Half, Plan::SteleNorth, Plan::SteleBottom),
				FVector(Plan::SteleX + Half, Plan::SteleSouth, Plan::SteleTop), Solid);
			Glass.bNanite = false;
			Out.Add(MoveTemp(Glass));
			FFormSweep Foot;
			Foot.Section = EasedTop(0.5 * (Plan::FootEast - Plan::FootWest), Plan::SteleBottom, Plan::FootEase);
			const double X = 0.5 * (Plan::FootEast + Plan::FootWest);
			Foot.Path = FFormProfile::Polyline({FVector2D(X, Plan::FootNorth), FVector2D(X, Plan::FootSouth)}, false);
			Out.Add(Drawn(TEXT("stele.foot"), TEXT("stele.foot"), MoveTemp(Foot), Solid));
		}

		/**
		 * A 12 mm bronze angle along one edge of the hood: its legs lie on the two faces that meet there, drawn across the
		 * edge (x out of one face, y out of the other) and pushed its length.
		 */
		void HoodAngle(TArray<FFormDrawing>& Out, const FString& Name, const FVector& Corner, const FVector& Edge, const FVector& N1, const FVector& N2)
		{
			const double T = Plan::AngleThickness;
			const double W = Plan::AngleLeg;
			const FVector Z = FVector::CrossProduct(N1, N2);
			const bool bForward = FVector::DotProduct(Z, Edge) > 0.0;
			const FFormProfile L = FFormProfile::Polygon({{T - W, 0.0}, {0.0, 0.0}, {0.0, T - W}, {T, T - W}, {T, T}, {T - W, T}});
			Out.Add(FaceSlab(Name, TEXT("case.angles"), FrameOfAxes(bForward ? Corner : Corner + Edge, N1, N2), L, {}, Edge.Size(), Solid));
		}

		/** The case: plinth, carcase with its framed long faces, top rail, felt and board, the glass hood and its angles. */
		void Case(TArray<FFormDrawing>& Out)
		{
			const double X0 = Plan::CaseX - Plan::CaseHalfLength;
			const double X1 = Plan::CaseX + Plan::CaseHalfLength;
			const double Y0 = Plan::CaseY - Plan::CaseHalfWidth;
			const double Y1 = Plan::CaseY + Plan::CaseHalfWidth;
			const double R = Plan::CasePlinthRecess;
			Out.Add(Box(TEXT("case.plinth"), TEXT("case.carcase"), FVector(X0 + R, Y0 + R, 0.0), FVector(X1 - R, Y1 - R, Plan::CasePlinthTop), Solid));
			Out.Add(Box(TEXT("case.body"), TEXT("case.carcase"), FVector(X0, Y0, Plan::CasePlinthTop), FVector(X1, Y1, Plan::CaseBodyTop), Solid));
			const double P = Plan::FrameProud;
			for (const double Side : {-1.0, 1.0})
			{
				const TCHAR* Face = Side < 0.0 ? TEXT("north") : TEXT("south");
				const double In = Side < 0.0 ? Y0 : Y1;
				const double Outer = In + Side * P;
				const double YMin = FMath::Min(In, Outer);
				const double YMax = FMath::Max(In, Outer);
				Out.Add(Box(FString::Printf(TEXT("case.rail.%s.bottom"), Face), TEXT("case.carcase"), FVector(X0, YMin, Plan::BottomRail[0]),
					FVector(X1, YMax, Plan::BottomRail[1]), Solid));
				Out.Add(Box(FString::Printf(TEXT("case.rail.%s.top"), Face), TEXT("case.carcase"), FVector(X0, YMin, Plan::TopRail[0]),
					FVector(X1, YMax, Plan::TopRail[1]), Solid));
				for (int32 Stile = 0; Stile < int32(UE_ARRAY_COUNT(Plan::StileAt)); ++Stile)
				{
					const double SX = Plan::CaseX + Plan::StileAt[Stile];
					Out.Add(Box(FString::Printf(TEXT("case.stile.%s.%d"), Face, Stile + 1), TEXT("case.carcase"), FVector(SX, YMin, Plan::BottomRail[1]),
						FVector(SX + Plan::StileWidth, YMax, Plan::TopRail[0]), Solid));
				}
			}
			const double C = Plan::CapRailProud;
			Out.Add(Box(TEXT("case.cap"), TEXT("case.carcase"), FVector(X0 - C, Y0 - C, Plan::CapRail[0]), FVector(X1 + C, Y1 + C, Plan::CapRail[1]), Solid));
			// The escutcheon on the reader's (north) rail, at the case's middle.
			const double EW = 0.5 * Plan::EscutcheonWidth;
			const double EZ = 0.5 * (Plan::TopRail[0] + Plan::TopRail[1]);
			const double EH = 0.5 * Plan::EscutcheonHeight;
			Out.Add(Box(TEXT("case.escutcheon"), TEXT("case.angles"), FVector(Plan::CaseX - EW, Y0 - P - Plan::EscutcheonThickness, EZ - EH),
				FVector(Plan::CaseX + EW, Y0 - P, EZ + EH), Solid));

			const double G = Plan::HoodPane;
			const double Z0 = Plan::FeltTop;
			const double Z1 = Plan::HoodTop;
			Out.Add(Box(TEXT("case.felt"), TEXT("case.felt"), FVector(X0 + G, Y0 + G, Plan::CapRail[1]), FVector(X1 - G, Y1 - G, Z0), Solid));
			const double BX = Plan::CaseX + Plan::BoardAt;
			Out.Add(Box(TEXT("case.board"), TEXT("case.felt"), FVector(BX - 0.5 * Plan::BoardLength, Plan::CaseY - 0.5 * Plan::BoardWidth, Z0),
				FVector(BX + 0.5 * Plan::BoardLength, Plan::CaseY + 0.5 * Plan::BoardWidth, Z0 + Plan::BoardThickness), Solid));
			const TPair<const TCHAR*, FBox> Panes[] = {
				{TEXT("top"), FBox(FVector(X0, Y0, Z1 - G), FVector(X1, Y1, Z1))},
				{TEXT("north"), FBox(FVector(X0, Y0, Z0), FVector(X1, Y0 + G, Z1 - G))},
				{TEXT("south"), FBox(FVector(X0, Y1 - G, Z0), FVector(X1, Y1, Z1 - G))},
				{TEXT("west"), FBox(FVector(X0, Y0 + G, Z0), FVector(X0 + G, Y1 - G, Z1 - G))},
				{TEXT("east"), FBox(FVector(X1 - G, Y0 + G, Z0), FVector(X1, Y1 - G, Z1 - G))}};
			for (const TPair<const TCHAR*, FBox>& Pane : Panes)
			{
				FFormDrawing Glass = Box(FString::Printf(TEXT("case.hood.%s"), Pane.Key), TEXT("case.hood"), Pane.Value.Min, Pane.Value.Max, Solid);
				Glass.bNanite = false;
				Out.Add(MoveTemp(Glass));
			}

			// The twelve angles: four along x, four along y, four upright.
			const FVector Ex(X1 - X0, 0.0, 0.0);
			const FVector Ey(0.0, Y1 - Y0, 0.0);
			const FVector Ez(0.0, 0.0, Z1 - Z0);
			const FVector N = FMuseeFrame::North();
			const FVector S = FMuseeFrame::South();
			const FVector W = FMuseeFrame::West();
			const FVector E = FMuseeFrame::East();
			const FVector Up = FMuseeFrame::Up();
			const FVector Down = -Up;
			HoodAngle(Out, TEXT("case.angle.north.top"), FVector(X0, Y0, Z1), Ex, N, Up);
			HoodAngle(Out, TEXT("case.angle.north.bottom"), FVector(X0, Y0, Z0), Ex, N, Down);
			HoodAngle(Out, TEXT("case.angle.south.top"), FVector(X0, Y1, Z1), Ex, S, Up);
			HoodAngle(Out, TEXT("case.angle.south.bottom"), FVector(X0, Y1, Z0), Ex, S, Down);
			HoodAngle(Out, TEXT("case.angle.west.top"), FVector(X0, Y0, Z1), Ey, W, Up);
			HoodAngle(Out, TEXT("case.angle.west.bottom"), FVector(X0, Y0, Z0), Ey, W, Down);
			HoodAngle(Out, TEXT("case.angle.east.top"), FVector(X1, Y0, Z1), Ey, E, Up);
			HoodAngle(Out, TEXT("case.angle.east.bottom"), FVector(X1, Y0, Z0), Ey, E, Down);
			HoodAngle(Out, TEXT("case.angle.northwest"), FVector(X0, Y0, Z0), Ez, N, W);
			HoodAngle(Out, TEXT("case.angle.northeast"), FVector(X1, Y0, Z0), Ez, N, E);
			HoodAngle(Out, TEXT("case.angle.southwest"), FVector(X0, Y1, Z0), Ez, S, W);
			HoodAngle(Out, TEXT("case.angle.southeast"), FVector(X1, Y1, Z0), Ez, S, E);
		}

		/** The Little Dancer's plinth and her vitrine of five panes. */
		void Dancer(TArray<FFormDrawing>& Out)
		{
			const double X = Plan::DancerX;
			const double Y = Plan::DancerY;
			const double P = Plan::DancerPlinthHalf;
			Out.Add(Box(TEXT("dancer.plinth"), TEXT("dancer.plinth"), FVector(X - P, Y - P, 0.0), FVector(X + P, Y + P, Plan::DancerPlinthTop), Solid));
			const double V = Plan::VitrineHalf;
			const double G = Plan::VitrinePane;
			const double Z0 = Plan::DancerPlinthTop;
			const double Z1 = Plan::VitrineTop;
			const TPair<const TCHAR*, FBox> Panes[] = {
				{TEXT("top"), FBox(FVector(X - V, Y - V, Z1 - G), FVector(X + V, Y + V, Z1))},
				{TEXT("north"), FBox(FVector(X - V, Y - V, Z0), FVector(X + V, Y - V + G, Z1 - G))},
				{TEXT("south"), FBox(FVector(X - V, Y + V - G, Z0), FVector(X + V, Y + V, Z1 - G))},
				{TEXT("west"), FBox(FVector(X - V, Y - V + G, Z0), FVector(X - V + G, Y + V - G, Z1 - G))},
				{TEXT("east"), FBox(FVector(X + V - G, Y - V + G, Z0), FVector(X + V, Y + V - G, Z1 - G))}};
			for (const TPair<const TCHAR*, FBox>& Pane : Panes)
			{
				FFormDrawing Glass = Box(FString::Printf(TEXT("dancer.vitrine.%s"), Pane.Key), TEXT("dancer.vitrine"), Pane.Value.Min, Pane.Value.Max, Solid);
				Glass.bNanite = false;
				Out.Add(MoveTemp(Glass));
			}
		}
	}

	void DrawFittings(TArray<FFormDrawing>& Out)
	{
		for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::BenchX)); ++Index)
		{
			Bench(Out, Index + 1, Plan::BenchX[Index], -Plan::BenchY, TEXT("n"));
			Bench(Out, Index + 1, Plan::BenchX[Index], Plan::BenchY, TEXT("s"));
		}
		Stele(Out);
		Case(Out);
		Dancer(Out);
	}
}
