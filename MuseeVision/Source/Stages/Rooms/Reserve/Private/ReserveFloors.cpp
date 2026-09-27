#include "ReserveParts.h"
#include "ReservePlan.h"

/**
 * The floor (§3.2): honed Belgian bluestone flags 0.750 square on the 0.75 grid, laid in lime on the brick bed, run to
 * the walls and cut round the piers' and responds' bases and the racks' floor tracks; and the 36 flush tracks, bronze
 * channels lining the 30 mm cuts, closed at their ends and bottoms, their lips flush with the flags.
 */
namespace Musee::Reserve
{
	namespace
	{
		const Musee::Collision::FClass* const Walkable = &Musee::Collision::Walkable;

		/** A plan rectangle between two x and two y. */
		struct FRect
		{
			double X0, X1, Y0, Y1;
		};

		/**
		 * An outline between y From and To over the hall's length, anticlockwise, with rectangular notches let into it from
		 * its edges: the bases standing against the walls, and the tracks' cuts running out across the nave's seam.
		 */
		FFormProfile Notched(double From, double To, TConstArrayView<FRect> Notches)
		{
			const double X = Plan::InnerX;
			TArray<FRect> Low;
			TArray<FRect> High;
			TArray<FRect> West;
			TArray<FRect> East;
			for (const FRect& N : Notches)
			{
				TArray<FRect>& Edge = N.X0 <= -X ? West : N.X1 >= X ? East : N.Y0 <= From ? Low : High;
				Edge.Add(N);
			}
			Low.Sort([](const FRect& A, const FRect& B) { return A.X0 < B.X0; });
			High.Sort([](const FRect& A, const FRect& B) { return A.X0 > B.X0; });
			East.Sort([](const FRect& A, const FRect& B) { return A.Y0 < B.Y0; });
			West.Sort([](const FRect& A, const FRect& B) { return A.Y0 > B.Y0; });
			TArray<FVector2D> P = {FVector2D(-X, From)};
			for (const FRect& N : Low)
			{
				P.Append({FVector2D(N.X0, From), FVector2D(N.X0, N.Y1), FVector2D(N.X1, N.Y1), FVector2D(N.X1, From)});
			}
			P.Add(FVector2D(X, From));
			for (const FRect& N : East)
			{
				P.Append({FVector2D(X, N.Y0), FVector2D(N.X0, N.Y0), FVector2D(N.X0, N.Y1), FVector2D(X, N.Y1)});
			}
			P.Add(FVector2D(X, To));
			for (const FRect& N : High)
			{
				P.Append({FVector2D(N.X1, To), FVector2D(N.X1, N.Y0), FVector2D(N.X0, N.Y0), FVector2D(N.X0, To)});
			}
			P.Add(FVector2D(-X, To));
			for (const FRect& N : West)
			{
				P.Append({FVector2D(-X, N.Y1), FVector2D(N.X1, N.Y1), FVector2D(N.X1, N.Y0), FVector2D(-X, N.Y0)});
			}
			return FFormProfile::Polygon(P);
		}

		/** What the flags are cut round, as plan rectangles: the piers' and responds' bases and the tracks' cuts. */
		TArray<FRect> Cutouts()
		{
			TArray<FRect> Out;
			const double Base = Plan::RespondBaseHalf;
			for (int32 Line = 0; Line < Plan::PierLines; ++Line)
			{
				const double X = PierX(Line);
				for (const double Side : {-1.0, 1.0})
				{
					const double Y = Side * Plan::ArcadeY;
					Out.Add({X - Base, X + Base, Y - Base, Y + Base});
					const double Wall = Side * Plan::InnerY;
					Out.Add({X - Base, X + Base, FMath::Min(Wall, Wall - Side * Plan::RespondBaseD), FMath::Max(Wall, Wall - Side * Plan::RespondBaseD)});
				}
			}
			for (const double End : {-1.0, 1.0})
			{
				const double Wall = End * Plan::InnerX;
				for (const double Side : {-1.0, 1.0})
				{
					const double Y = Side * Plan::ArcadeY;
					Out.Add({FMath::Min(Wall, Wall - End * Plan::RespondBaseD), FMath::Max(Wall, Wall - End * Plan::RespondBaseD), Y - Base, Y + Base});
				}
			}
			for (const FRack& Rack : Racks())
			{
				const double Near = Rack.Side * Plan::SlotFrom;
				const double Far = Rack.Side * Plan::SlotTo;
				Out.Add({Rack.X - 0.5 * Plan::SlotCut, Rack.X + 0.5 * Plan::SlotCut, FMath::Min(Near, Far), FMath::Max(Near, Far)});
			}
			return Out;
		}

		/**
		 * One field of flags between y From and To: the cutouts wholly inside it are holes, those crossing or touching its
		 * edges are notches cut back to them.
		 */
		FFormDrawing Flags(const TCHAR* Name, const TCHAR* LocalSurface, double From, double To)
		{
			TArray<FRect> Notches;
			TArray<FFormProfile> Holes;
			constexpr double Touch = 1e-9;
			for (FRect R : Cutouts())
			{
				if (R.Y1 <= From + Touch || R.Y0 >= To - Touch)
				{
					continue;
				}
				R.Y0 = FMath::Max(R.Y0, From);
				R.Y1 = FMath::Min(R.Y1, To);
				const bool bInside = R.Y0 > From + Touch && R.Y1 < To - Touch && R.X0 > -Plan::InnerX + Touch && R.X1 < Plan::InnerX - Touch;
				if (bInside)
				{
					Holes.Add(FFormProfile::Rectangle(FVector2D(R.X0, R.Y0), FVector2D(R.X1, R.Y1)));
				}
				else
				{
					Notches.Add(R);
				}
			}
			FFormLaid Shape;
			Shape.Outline = Notched(From, To, Notches);
			Shape.Holes = MoveTemp(Holes);
			Shape.ThicknessMeters = Plan::FlagThickness;
			Shape.JointMeters = Plan::FlagJoint;
			Shape.FillMeters = 0.0;
			Shape.Module.Add(FFormProfile::Rectangle(FVector2D::ZeroVector, FVector2D(Plan::FlagSize, Plan::FlagSize)));
			Shape.LatticeU = FVector2D(Plan::FlagSize, 0.0);
			Shape.LatticeV = FVector2D(0.0, Plan::FlagSize);
			FFormDrawing Drawing = Drawn(Name, LocalSurface, MoveTemp(Shape), Walkable, FMuseeFrame(FVector(0.0, 0.0, -Plan::FlagThickness)));
			// The lime in the joints takes the flags' own finish: the palette has no lime mortar yet (the README's gaps).
			Drawing.PartSurfaces.Add(FFormLaid::JointPart, Surface(LocalSurface));
			return Drawing;
		}

		/** A rack's flush track: the channel filling the cut through the flags, its slot sunk from the floor, closed at its ends. */
		FFormDrawing Slot(const FRack& Rack)
		{
			const double Near = Rack.Side * Plan::SlotFrom;
			const double Far = Rack.Side * Plan::SlotTo;
			const double Y0 = FMath::Min(Near, Far);
			const double Y1 = FMath::Max(Near, Far);
			const double Lip = 0.5 * (Plan::SlotCut - Plan::SlotClear);
			FBlock Cut = Block(FVector(Rack.X - 0.5 * Plan::SlotCut, Y0, -Plan::FlagThickness), FVector(Rack.X + 0.5 * Plan::SlotCut, Y1, 0.0));
			FFormPocket& Channel = Cut.Shape.Pockets.AddDefaulted_GetRef();
			Channel.Outline = FFormProfile::Rectangle(FVector2D(Rack.X - 0.5 * Plan::SlotClear, Y0 + Lip), FVector2D(Rack.X + 0.5 * Plan::SlotClear, Y1 - Lip));
			Channel.DepthMeters = Plan::SlotDepth;
			return Drawn(Rack.Piece(TEXT("floor-track")), TEXT("floor.slot"), MoveTemp(Cut), Walkable);
		}
	}

	void DrawFloors(TArray<FFormDrawing>& Out)
	{
		Out.Add(Flags(TEXT("floor.nave"), TEXT("floor"), -Plan::NaveFlagsHalf, Plan::NaveFlagsHalf));
		Out.Add(Flags(TEXT("floor.aisle.north"), TEXT("floor.aisle"), -Plan::InnerY, -Plan::NaveFlagsHalf));
		Out.Add(Flags(TEXT("floor.aisle.south"), TEXT("floor.aisle"), Plan::NaveFlagsHalf, Plan::InnerY));
		for (const FRack& Rack : Racks())
		{
			Out.Add(Slot(Rack));
		}
	}
}
