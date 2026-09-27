#include "GroundsParts.h"
#include "GroundsRun.h"

/**
 * The strip round the stone complex (§3.5; as built the podium's foot, MuseeFacadeStructure.cpp:217-230): gravel from
 * each building's wall face to d 1.44, top +0.012, and the sawn edge d 1.44 ... 1.56 along its outer boundary, at
 * y +/-9.16 along the Salon, round the Manet cabinet, r 12.76 on the Rotunda drum's free arcs, and round the Nymphéas
 * oval. Where the band runs into another building (the cabinet and Chenghuai) the gravel fills to that outline and there
 * is no edge.
 *
 * Where two faces' bands meet square (the Salon and its cabinet) they are one band with one edge, mitred round its
 * corners. The sheet asks for the union of every face's band, so that the Salon's joins the oval's at its west end and
 * the drum's at its east; Form has no union of plan regions yet, so those two junctions are left unlaid, each band
 * stopping on the building face where it ends (the Salon's corners, the passage block's sides and the cut lines of the
 * drum's free arcs), and nothing overlaps.
 */
namespace Musee::Grounds
{
	namespace
	{
		const Musee::Collision::FClass* const Walkable = &Musee::Collision::Walkable;

		constexpr double Gravel = Plan::StripGravel;

		void Band(TArray<FFormDrawing>& Out, const TCHAR* Name, FFormProfile Outline)
		{
			Out.Add(Laid(FString::Printf(TEXT("strip.%s"), Name), TEXT("gravel"), MoveTemp(Outline), {}, Plan::StripBottom, Plan::StripTop, Walkable));
		}

		/** The edge's section: A across from its inner face (d 1.44) outwards, B up. */
		FFormProfile EdgeSection()
		{
			TArray<FVector2D> Points;
			for (const Plan::FPlanPoint& Point : Plan::StripEdgeSection)
			{
				Points.Add(FVector2D(Point.X - Gravel, Point.Y));
			}
			return Polygon(Points);
		}

		/** The edge along a run of its inner face, the run's right outward, its ends cut on the given planes (zero: square or mitred). */
		void Edge(TArray<FFormDrawing>& Out, const TCHAR* Name, const FRun& Run, const FVector& StartCut = FVector::ZeroVector,
			const FVector& EndCut = FVector::ZeroVector)
		{
			TArray<FRunStone> Stretches = Run.Stones(Run.Length());
			for (int32 Index = 0; Index < Stretches.Num(); ++Index)
			{
				FFormSweep Shape;
				Shape.Section = EdgeSection();
				Shape.Path = MoveTemp(Stretches[Index].Path);
				Shape.StartCut = Index == 0 && !StartCut.IsZero() ? StartCut : Stretches[Index].StartCut;
				Shape.EndCut = Index + 1 == Stretches.Num() && !EndCut.IsZero() ? EndCut : Stretches[Index].EndCut;
				const FString Piece = Stretches.Num() == 1 ? FString::Printf(TEXT("strip.edge.%s"), Name)
					: FString::Printf(TEXT("strip.edge.%s.%d"), Name, Index + 1);
				Out.Add(Drawn(Piece, TEXT("strip.edge"), MoveTemp(Shape), Walkable));
			}
		}

		/** The Salon's south face: y 7.60 to the edge, x -74.60 ... -13.40, the parvis's and walks' north side. */
		void Salon(TArray<FFormDrawing>& Out)
		{
			const double Face = Plan::SalonHalf;
			Band(Out, TEXT("south"), Rectangle({Plan::SalonWestX, Face}, {Plan::SalonEastX, Face + Gravel}));
			Edge(Out, TEXT("south"), FRun::Through({FVector2D(Plan::SalonWestX, Face + Gravel), FVector2D(Plan::SalonEastX, Face + Gravel)}, false));
		}

		/**
		 * The Salon's north face and its Manet cabinet as one band: along the Salon, round the cabinet's west, north and east
		 * faces, the slot between the cabinet and Chenghuai filled wall to wall.
		 */
		void North(TArray<FFormDrawing>& Out)
		{
			const double Face = -Plan::SalonHalf;
			const double Edge0 = Face - Gravel;
			const double West = Plan::CabinetWestX - Gravel;
			const double East = Plan::CabinetEastX + Gravel;
			const double Beyond = Plan::CabinetNorthY - Gravel;
			Band(Out, TEXT("north"), Polygon({FVector2D(Plan::SalonWestX, Face), FVector2D(Plan::CabinetWestX, Face),
				FVector2D(Plan::CabinetWestX, Plan::CabinetNorthY), FVector2D(Plan::CabinetEastX, Plan::CabinetNorthY), FVector2D(Plan::CabinetEastX, Face),
				FVector2D(Plan::SalonEastX, Face), FVector2D(Plan::SalonEastX, Edge0), FVector2D(East, Edge0), FVector2D(East, Plan::ChenghuaiSouthY),
				FVector2D(Plan::ChenghuaiWestX, Plan::ChenghuaiSouthY), FVector2D(Plan::ChenghuaiWestX, Beyond), FVector2D(West, Beyond),
				FVector2D(West, Edge0), FVector2D(Plan::SalonWestX, Edge0)}));
			Edge(Out, TEXT("north.west"), FRun::Through({FVector2D(Plan::ChenghuaiWestX, Beyond), FVector2D(West, Beyond), FVector2D(West, Edge0),
				FVector2D(Plan::SalonWestX, Edge0)}, false));
			Edge(Out, TEXT("north.east"), FRun::Through({FVector2D(Plan::SalonEastX, Edge0), FVector2D(East, Edge0),
				FVector2D(East, Plan::ChenghuaiSouthY)}, false));
		}

		/** A straight line the drum's band is cut on: x = Value (or y = Value), on the side of the drum Sign gives the other coordinate. */
		struct FCut
		{
			bool bAlongX = true;
			double Value = 0.0;
			double Sign = 1.0;

			FVector2D At(double Radius) const
			{
				const double Other = Sign * FMath::Sqrt(FMath::Square(Radius) - FMath::Square(Value));
				return bAlongX ? FVector2D(Value, Other) : FVector2D(Other, Value);
			}

			FVector Normal() const { return bAlongX ? FVector(1.0, 0.0, 0.0) : FVector(0.0, 1.0, 0.0); }
		};

		/** One free arc of the drum's band between two cuts, From at the lower plan angle. */
		void DrumArc(TArray<FFormDrawing>& Out, const TCHAR* Name, const FCut& From, const FCut& To)
		{
			const FVector2D Axis = FVector2D::ZeroVector;
			const double Inner = Plan::DrumRadius;
			const double Outer = Plan::DrumRadius + Gravel;
			FFormProfile Outline = FFormProfile::From(From.At(Inner));
			Outline.ArcAbout(Axis, Inner, Inner, AngleOf(Axis, From.At(Inner)), AngleOf(Axis, To.At(Inner)), EFormJoint::Crisp);
			Outline.LineTo(To.At(Outer), EFormJoint::Crisp);
			Outline.ArcAbout(Axis, Outer, Outer, AngleOf(Axis, To.At(Outer)), AngleOf(Axis, From.At(Outer)), EFormJoint::Crisp);
			Band(Out, *FString::Printf(TEXT("drum.%s"), Name), Outline.Close());
			FRun Run = FRun::From(To.At(Outer));
			Run.ArcTo(Axis, AngleOf(Axis, From.At(Outer)), false);
			Edge(Out, *FString::Printf(TEXT("drum.%s"), Name), Run, To.Normal(), From.Normal());
		}

		/**
		 * The drum's free arcs: from the Salon passage's sides (|y| 1.80) to Chenghuai's porch (|x| 2.95) on the north-west, on
		 * to the Hall of Light's west edge (x 10.90), and from it back round the south-east to Albion's porch (|x| 3.70) and on
		 * to the passage.
		 */
		void Drum(TArray<FFormDrawing>& Out)
		{
			DrumArc(Out, TEXT("north-west"), {false, -Plan::SalonPassageHalf, -1.0}, {true, -Plan::ChenghuaiPorchHalf, -1.0});
			DrumArc(Out, TEXT("north-east"), {true, Plan::ChenghuaiPorchHalf, -1.0}, {true, Plan::HallOfLightWestX, -1.0});
			DrumArc(Out, TEXT("south-east"), {true, Plan::HallOfLightWestX, 1.0}, {true, Plan::AlbionPorchHalf, 1.0});
			DrumArc(Out, TEXT("south-west"), {true, -Plan::AlbionPorchHalf, 1.0}, {false, Plan::SalonPassageHalf, -1.0});
		}

		/** The oval's parallel at D off its inner ellipse (the wall's outer face at 0.60), at the ellipse's parameter T. */
		FVector2D OvalParallel(double T, double D)
		{
			const double A = Plan::OvalA;
			const double B = Plan::OvalB;
			const FVector2D Normal = FVector2D(B * FMath::Cos(T), A * FMath::Sin(T)).GetSafeNormal();
			return At(Plan::OvalCentre) + FVector2D(A * FMath::Cos(T), B * FMath::Sin(T)) + D * Normal;
		}

		/** The parameter in [0, π/2] where the parallel at D reaches a value of x (or of y), by bisection. */
		double OvalWhere(double D, double Value, bool bX)
		{
			double Low = 0.0;
			double High = 0.5 * UE_DOUBLE_PI;
			for (int32 Step = 0; Step < 60; ++Step)
			{
				const double Mid = 0.5 * (Low + High);
				const FVector2D P = OvalParallel(Mid, D);
				// x falls and y rises through the first quarter.
				const bool bPast = bX ? P.X < Value : P.Y > Value;
				(bPast ? High : Low) = Mid;
			}
			return 0.5 * (Low + High);
		}

		/** The parallel at D from T0 to T1, at the oval's even stations between them, smooth throughout. */
		void AlongOval(FFormProfile& Path, double D, double T0, double T1)
		{
			const double Step = 2.0 * UE_DOUBLE_PI / Plan::OvalStations;
			const int32 Count = FMath::Max(1, FMath::CeilToInt32(FMath::Abs(T1 - T0) / Step));
			for (int32 Index = 1; Index <= Count; ++Index)
			{
				Path.LineTo(OvalParallel(FMath::Lerp(T0, T1, double(Index) / Count), D), EFormJoint::Smooth);
			}
		}

		/**
		 * The oval's band, round it by the west from the passage block's north side to its south side (|y| 2.10) and out to
		 * the Salon's west wall (x -74.60); its edge round the west between the Salon's wall on either side.
		 */
		void Oval(TArray<FFormDrawing>& Out)
		{
			const double Face = Plan::OvalWall;
			const double Line = Plan::OvalWall + Gravel;
			const double TBlock = OvalWhere(Face, Plan::PassageBlockHalf, false);
			const double TSalon = OvalWhere(Line, Plan::SalonWestX, true);
			FFormProfile Outline = FFormProfile::From(OvalParallel(-TBlock, Face));
			Outline.LineTo(FVector2D(Plan::SalonWestX, -Plan::PassageBlockHalf), EFormJoint::Crisp);
			Outline.LineTo(OvalParallel(-TSalon, Line), EFormJoint::Crisp);
			AlongOval(Outline, Line, -TSalon, TSalon - 2.0 * UE_DOUBLE_PI);
			Outline.LineTo(FVector2D(Plan::SalonWestX, Plan::PassageBlockHalf), EFormJoint::Crisp);
			Outline.LineTo(OvalParallel(TBlock, Face), EFormJoint::Crisp);
			AlongOval(Outline, Face, TBlock, 2.0 * UE_DOUBLE_PI - TBlock);
			Band(Out, TEXT("oval"), Outline.Close());

			FFormProfile Path = FFormProfile::From(OvalParallel(-TSalon, Line));
			AlongOval(Path, Line, -TSalon, TSalon - 2.0 * UE_DOUBLE_PI);
			FFormSweep Shape;
			Shape.Section = EdgeSection();
			Shape.Path = MoveTemp(Path);
			Shape.StartCut = FVector(1.0, 0.0, 0.0);
			Shape.EndCut = FVector(1.0, 0.0, 0.0);
			Out.Add(Drawn(TEXT("strip.edge.oval"), TEXT("strip.edge"), MoveTemp(Shape), Walkable));
		}
	}

	void DrawStrip(TArray<FFormDrawing>& Out)
	{
		Salon(Out);
		North(Out);
		Drum(Out);
		Oval(Out);
	}
}
