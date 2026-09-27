#include "AlbionParts.h"

/**
 * The arcades' iron (§3.5): on every column and half-column a clustered shaft on its shoe to its cast capital and top
 * plate; the box girders on the arcade lines carrying the lead valley gutters; the pointed arches between the capitals
 * with their bosses; and in each arch's spandrels a wrought ring, an inner ring and a quatrefoil, tied by lugs to the arch
 * and the girder. The iron is out of reach: nothing collides with it; it casts shadows. The capitals' cast leaves wait
 * for polish (§12).
 */
namespace Musee::Albion
{
	namespace
	{
		/**
		 * The clustered shaft's plan: a core Ø 0.15 and four colonnettes Ø 0.09 on the diagonals at r 0.095, cast as one,
		 * its outline the colonnettes' outer arcs joined by the core's arcs between them, run anticlockwise.
		 */
		FFormProfile ClusterPlan()
		{
			const double Core = Plan::CoreRadius;
			const double Colonnette = Plan::ColonnetteRadius;
			const double Reach = Plan::ColonnetteReach;
			// Where the circles cross: the angle at the axis, and at the colonnette's centre, of the triangle of the three radii.
			const double AtAxis = FMath::RadiansToDegrees(FMath::Acos((Reach * Reach + Core * Core - Colonnette * Colonnette) / (2.0 * Reach * Core)));
			const double AtColonnette =
				FMath::RadiansToDegrees(FMath::Acos((Reach * Reach + Colonnette * Colonnette - Core * Core) / (2.0 * Reach * Colonnette)));
			const double OuterArc = 180.0 - AtColonnette;
			auto Diagonal = [](int32 Index) { return 45.0 + 90.0 * Index; };
			const double Start = FMath::DegreesToRadians(Diagonal(0) + AtAxis);
			FFormProfile P = FFormProfile::From(FVector2D(Core * FMath::Cos(Start), Core * FMath::Sin(Start)));
			for (int32 Index = 1; Index <= 4; ++Index)
			{
				const double Angle = Diagonal(Index);
				const double Radians = FMath::DegreesToRadians(Angle);
				P.ArcAbout(FVector2D::ZeroVector, Core, Core, Diagonal(Index - 1) + AtAxis, Angle - AtAxis, EFormJoint::Crisp);
				P.ArcAbout(FVector2D(Reach * FMath::Cos(Radians), Reach * FMath::Sin(Radians)), Colonnette, Colonnette, Angle - OuterArc, Angle + OuterArc,
					EFormJoint::Crisp);
			}
			return P.Close();
		}

		/** One cluster: shoe, moulded base, the clustered shaft, two annulets, the cast capital and the top plate. */
		void Cluster(const FString& Name, const FVector2D& At, TArray<FFormDrawing>& Out)
		{
			const FVector Foot(At.X, At.Y, 0.0);
			const FVector Shoe(Plan::ShoeHalf, Plan::ShoeHalf, 0.0);
			Out.Add(Box(Name + TEXT(".shoe"), TEXT("iron.cast"), Foot - Shoe + FVector(0.0, 0.0, Plan::ShoeFoot), Foot + Shoe + FVector(0.0, 0.0, Plan::ShoeTop),
				Overhead));
			Out.Add(Lathe(Name + TEXT(".base"), TEXT("iron.cast"), Foot, Turned(Plan::ClusterBasePoints), Overhead));
			FFormExtrusion Shaft;
			Shaft.Outline = ClusterPlan();
			Shaft.DepthMeters = Plan::ClusterShaftTop - Plan::ClusterShaftFoot;
			Out.Add(Drawn(Name + TEXT(".shaft"), TEXT("iron.cast"), MoveTemp(Shaft), Overhead, FMuseeFrame(FVector(At.X, At.Y, Plan::ClusterShaftFoot))));
			for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(Plan::AnnuletZ)); ++Index)
			{
				Out.Add(Lathe(FString::Printf(TEXT("%s.annulet.%d"), *Name, Index + 1), TEXT("iron.cast"), Foot, Turned(Plan::AnnuletPoints, Plan::AnnuletZ[Index]),
					Overhead));
			}
			Out.Add(Lathe(Name + TEXT(".capital"), TEXT("iron.cast"), Foot, Turned(Plan::ClusterCapitalPoints), Overhead));
			const FVector Plate(Plan::TopPlateHalf, Plan::TopPlateHalf, 0.0);
			Out.Add(Box(Name + TEXT(".plate"), TEXT("iron.cast"), Foot - Plate + FVector(0.0, 0.0, Plan::TopPlateFoot),
				Foot + Plate + FVector(0.0, 0.0, Plan::TopPlateTop), Overhead));
		}

		void Clusters(TArray<FFormDrawing>& Out)
		{
			for (int32 Line = 2; Line <= Plan::Lines - 1; ++Line)
			{
				Cluster(FString::Printf(TEXT("cluster.%s"), Plan::WestStones[Line - 2]), FVector2D(-Plan::ArcadeX, Plan::LineY(Line)), Out);
				Cluster(FString::Printf(TEXT("cluster.%s"), Plan::EastStones[Line - 2]), FVector2D(Plan::ArcadeX, Plan::LineY(Line)), Out);
			}
			Cluster(TEXT("cluster.nw"), FVector2D(-Plan::ArcadeX, -Plan::HalfClusterY), Out);
			Cluster(TEXT("cluster.ne"), FVector2D(Plan::ArcadeX, -Plan::HalfClusterY), Out);
			Cluster(TEXT("cluster.sw"), FVector2D(-Plan::ArcadeX, Plan::HalfClusterY), Out);
			Cluster(TEXT("cluster.se"), FVector2D(Plan::ArcadeX, Plan::HalfClusterY), Out);
		}

		/**
		 * The box girders on the arcade lines, 9.70 to 10.00, into the end walls: two flanges and two webs round the box's
		 * hollow, in section (x, z) along the court; and on each the lead valley gutter between the vaults' glass.
		 */
		void Girders(TArray<FFormDrawing>& Out)
		{
			const double Low = Plan::GirderFoot;
			const double High = Plan::GirderTop;
			const double F = Plan::FlangeMeters;
			const double WebOut = Plan::GirderWebX + 0.5 * Plan::GirderWebMeters;
			const double WebIn = Plan::GirderWebX - 0.5 * Plan::GirderWebMeters;
			const double Bottom = Plan::BottomFlangeHalf;
			const double Top = Plan::TopFlangeHalf;
			for (const bool bWest : {true, false})
			{
				const double X = bWest ? -Plan::ArcadeX : Plan::ArcadeX;
				const FString Name = bWest ? TEXT("girder.west") : TEXT("girder.east");
				FFormExtrusion Shape;
				Shape.Outline = Polygon({FVector2D(X - Bottom, Low), FVector2D(X + Bottom, Low), FVector2D(X + Bottom, Low + F), FVector2D(X + WebOut, Low + F),
					FVector2D(X + WebOut, High - F), FVector2D(X + Top, High - F), FVector2D(X + Top, High), FVector2D(X - Top, High), FVector2D(X - Top, High - F),
					FVector2D(X - WebOut, High - F), FVector2D(X - WebOut, Low + F), FVector2D(X - Bottom, Low + F)});
				Shape.Holes.Add(FFormProfile::Rectangle(FVector2D(X - WebIn, Low + F), FVector2D(X + WebIn, High - F)));
				Shape.DepthMeters = 2.0 * Plan::GirderEndY;
				Out.Add(Drawn(Name, TEXT("iron.cast"), MoveTemp(Shape), Overhead, AcrossY(Plan::GirderEndY)));
				Out.Add(Box(Name + TEXT(".gutter"), TEXT("lead"), FVector(X - Plan::ValleyHalf, -Plan::GirderEndY, High),
					FVector(X + Plan::ValleyHalf, Plan::GirderEndY, Plan::ValleyTop), Overhead));
			}
		}

		/**
		 * The arcade rib's section, (in from the extrados, across): the outer flange, the web, the inner flange and the roll
		 * whose foot is the soffit, run anticlockwise.
		 */
		FFormProfile ArcadeRib()
		{
			const double Depth = Plan::ArcadeRibDepth;
			const double F = Plan::FlangeMeters;
			const double Roll = Plan::ArcadeRollRadius;
			const double RollAt = Depth - Roll;
			const double InnerLow = Depth - 2.0 * Roll;
			const double InnerHigh = InnerLow - F;
			const double Web = Plan::ArcadeWebHalf;
			const double Outer = Plan::ArcadeOuterFlangeHalf;
			const double Inner = Plan::ArcadeInnerFlangeHalf;
			// Where the web's faces run into the roll.
			const double Meet = RollAt - FMath::Sqrt(Roll * Roll - Web * Web);
			const double Turn = FMath::RadiansToDegrees(FMath::Atan2(Web, Meet - RollAt));
			FFormProfile P = FFormProfile::From(FVector2D(0.0, -Outer));
			P.LineTo(FVector2D(F, -Outer), EFormJoint::Crisp).LineTo(FVector2D(F, -Web), EFormJoint::Crisp).LineTo(FVector2D(InnerHigh, -Web), EFormJoint::Crisp)
				.LineTo(FVector2D(InnerHigh, -Inner), EFormJoint::Crisp).LineTo(FVector2D(InnerLow, -Inner), EFormJoint::Crisp)
				.LineTo(FVector2D(InnerLow, -Web), EFormJoint::Crisp).LineTo(FVector2D(Meet, -Web), EFormJoint::Crisp);
			P.ArcAbout(FVector2D(RollAt, 0.0), Roll, Roll, -Turn, Turn, EFormJoint::Crisp);
			P.LineTo(FVector2D(InnerLow, Web), EFormJoint::Crisp).LineTo(FVector2D(InnerLow, Inner), EFormJoint::Crisp)
				.LineTo(FVector2D(InnerHigh, Inner), EFormJoint::Crisp).LineTo(FVector2D(InnerHigh, Web), EFormJoint::Crisp)
				.LineTo(FVector2D(F, Web), EFormJoint::Crisp).LineTo(FVector2D(F, Outer), EFormJoint::Crisp).LineTo(FVector2D(0.0, Outer), EFormJoint::Crisp);
			return P.Close();
		}

		/**
		 * A spandrel over an arch's haunch: its ring, inner ring and quatrefoil in the arcade's plane, and the two lugs,
		 * one down to the arch's extrados along the line from the arch's centre, one up to the girder's soffit.
		 */
		void Spandrel(const FString& Name, double X, const FVector2D& Hub, const FVector2D& ArchCentre, double ArchRadius, TArray<FFormDrawing>& Out)
		{
			auto InPlane = [X](const FVector2D& P) { return FVector(X, P.X, P.Y); };
			const FVector Across(1.0, 0.0, 0.0);
			auto Frame = [&](const FVector2D& Centre) { return FrameOfAxes(InPlane(Centre), FVector(0.0, 1.0, 0.0), FVector::UpVector); };
			Out.Add(Ring(Name + TEXT(".ring"), TEXT("iron.wrought"), Frame(Hub), Plan::SpandrelRing, Plan::SpandrelTube, Overhead));
			Out.Add(Ring(Name + TEXT(".inner-ring"), TEXT("iron.wrought"), Frame(Hub), Plan::SpandrelInnerRing, Plan::SpandrelInnerTube, Overhead));
			for (int32 Leaf = 0; Leaf < 4; ++Leaf)
			{
				const double Angle = FMath::DegreesToRadians(45.0 + 90.0 * Leaf);
				const FVector2D Centre = Hub + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Plan::QuatrefoilReach;
				Out.Add(Ring(FString::Printf(TEXT("%s.quatrefoil.%d"), *Name, Leaf + 1), TEXT("iron.wrought"), Frame(Centre), Plan::QuatrefoilRing,
					Plan::QuatrefoilTube, Overhead));
			}
			const double Outside = Plan::SpandrelRing + 0.5 * Plan::SpandrelTube;
			const FVector2D Towards = (Hub - ArchCentre).GetSafeNormal();
			Out.Add(Beam(Name + TEXT(".lug.arch"), TEXT("iron.wrought"), InPlane(ArchCentre + Towards * ArchRadius), InPlane(Hub - Towards * Outside),
				Plan::LugWidth, Plan::LugThickness, Overhead, Across));
			Out.Add(Beam(Name + TEXT(".lug.girder"), TEXT("iron.wrought"), InPlane(Hub + FVector2D(0.0, Outside)), InPlane(FVector2D(Hub.X, Plan::GirderFoot)),
				Plan::LugWidth, Plan::LugThickness, Overhead, Across));
		}

		/** The arcades: a pointed arch in each bay on each arcade line, its boss at the crown, its two spandrels. */
		void Arches(TArray<FFormDrawing>& Out)
		{
			for (const bool bWest : {true, false})
			{
				const double X = bWest ? -Plan::ArcadeX : Plan::ArcadeX;
				const TCHAR* Side = bWest ? TEXT("west") : TEXT("east");
				for (int32 Bay = 1; Bay <= Plan::Lines - 1; ++Bay)
				{
					const double Y = Plan::BayY(Bay);
					const FArch Arch = FArch::Pointed(Y, Plan::ArcadeHalfSpan, Plan::ArcadeSpringing, Plan::ArcadeSpringing, Plan::ArcadeCrown);
					const FString Name = FString::Printf(TEXT("arcade.%s.%d"), Side, Bay);
					FFormSweep Rib;
					Rib.Section = ArcadeRib();
					Rib.Path = Arch.Head(Arch.Springing);
					Out.Add(Drawn(Name, TEXT("iron.cast"), MoveTemp(Rib), Overhead, AcrossX(X)));
					Out.Add(Ball(Name + TEXT(".boss"), TEXT("iron.cast"), FVector(X, Y, Plan::ArcadeCrown - Plan::ArcadeRibDepth), Plan::ArcadeBossRadius, Overhead));
					// The spandrels over the haunches: the north one over the arch's left (north) arc, the south over its right.
					const FVector2D North(Y - Plan::SpandrelReach, Plan::SpandrelZ);
					const FVector2D South(Y + Plan::SpandrelReach, Plan::SpandrelZ);
					Spandrel(Name + TEXT(".spandrel.north"), X, North, FVector2D(Y + Arch.Offset, Arch.Springing), Arch.Radius, Out);
					Spandrel(Name + TEXT(".spandrel.south"), X, South, FVector2D(Y - Arch.Offset, Arch.Springing), Arch.Radius, Out);
				}
			}
		}
	}

	void DrawArcade(TArray<FFormDrawing>& Out)
	{
		Clusters(Out);
		Girders(Out);
		Arches(Out);
	}
}
