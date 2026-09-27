#include "ReserveParts.h"
#include "ReservePlan.h"

/**
 * The vaults (§3.4): 24 groin vaults over 8 bays and 3 rows, the cross bands over every pier line, the arcade bands along
 * the arcade lines and the springing blocks over the piers, all springing at 4.000 and filled solid in brick to the
 * box's top.
 *
 * A groin vault is two semi-elliptical barrels of one rise crossing; each web is the part of one barrel inside the
 * triangle between the bay's diagonals, where it stands higher than the other, so the groins lie over the diagonals.
 * Each quarter is drawn as its barrel's section (the intrados at the 40 shared stations, up to the top) pushed along
 * the barrel's axis and cut on the two diagonal planes: the two barrels meet the diagonal at the same points, so the
 * groin is one shared edge. The half-brick shell (0.105) and the bands' 0.165 are laid solid with the haunches' fill
 * above them, one brick mass; what shows is the soffit: the webs 0.06 above the bands, the bands' 0.06 sides, and the
 * lunettes on the arcade bands' nave faces.
 */
namespace Musee::Reserve
{
	namespace
	{
		const Musee::Collision::FClass* const Solid = &Musee::Collision::Solid;

		/**
		 * An arch's masonry in section (across its span, up): its intrados, a semi-ellipse of Half and Rise springing at Z
		 * about Middle at the shared stations, and the fill over it to Top, drawn anticlockwise.
		 */
		FFormProfile ArchSection(double Middle, double Half, double Z, double Rise, double Top)
		{
			FFormProfile Section = FFormProfile::From(FVector2D(Middle - Half, Z));
			for (int32 Station = 1; Station <= Plan::ArchStations; ++Station)
			{
				const double Angle = UE_DOUBLE_PI * (1.0 - double(Station) / Plan::ArchStations);
				Section.LineTo(FVector2D(Middle + Half * FMath::Cos(Angle), Z + Rise * FMath::Sin(Angle)), EFormJoint::Smooth);
			}
			Section.LineTo(FVector2D(Middle + Half, Top), EFormJoint::Crisp).LineTo(FVector2D(Middle - Half, Top), EFormJoint::Crisp);
			return Section.Close();
		}

		/** The frame of a barrel running east, its section in (y, z), pushed from x = From. */
		FMuseeFrame RunningEast(double From) { return FrameOfAxes(FVector(From, 0.0, 0.0), FVector(0.0, 1.0, 0.0), FVector::UpVector); }

		/** The frame of a barrel running north, its section in (x, z), pushed from y = From towards -y. */
		FMuseeFrame RunningNorth(double From) { return FrameOfAxes(FVector(0.0, From, 0.0), FVector(1.0, 0.0, 0.0), FVector::UpVector); }

		/** A drawing's cut on a vertical plane given in the stage's frame, through a plan point, cutting away the side Normal points to. */
		FFormCut CutIn(const FMuseeFrame& Frame, const FVector2D& Point, const FVector2D& Normal)
		{
			FFormCut Cut;
			Cut.PointMeters = Frame.PointFromParent(FVector(Point.X, Point.Y, 0.0));
			Cut.Normal = Frame.DirectionFromParent(FVector(Normal.X, Normal.Y, 0.0));
			return Cut;
		}

		double RowTop(const FRow& Row) { return Row.Middle == 0.0 ? Plan::TopHigh : Plan::TopLow; }

		/**
		 * One groin vault's four webs. In the bay's own fractions u (east) and v (south), the east web is where u ≥ |v|: the
		 * row's barrel running east, cut away where u < v and where u < -v; and so round.
		 */
		void Groin(TArray<FFormDrawing>& Out, int32 Bay, const FRow& Row)
		{
			const double Xb = BayCentre(Bay);
			const double Hb = BayHalf(Bay);
			const double Z = Plan::Springing + Plan::WebLift;
			const double Top = RowTop(Row);
			const FVector2D Centre(Xb, Row.Middle);
			// The gradients of u - v and u + v, in the stage's plan: each diagonal plane's normal.
			const FVector2D Minus(1.0 / Hb, -1.0 / Row.Half);
			const FVector2D Plus(1.0 / Hb, 1.0 / Row.Half);
			struct FQuarter
			{
				const TCHAR* Name;
				bool bEast;
				double From;
				double Depth;
				FVector2D Away[2];
			};
			const FQuarter Quarters[] = {
				{TEXT("e"), true, Xb, Hb, {-Minus, -Plus}},
				{TEXT("w"), true, Xb - Hb, Hb, {Minus, Plus}},
				{TEXT("n"), false, Row.Middle, Row.Half, {Plus, -Minus}},
				{TEXT("s"), false, Row.Middle + Row.Half, Row.Half, {Minus, -Plus}},
			};
			for (const FQuarter& Q : Quarters)
			{
				FFormExtrusion Shape;
				Shape.Outline = Q.bEast ? ArchSection(Row.Middle, Row.Half, Z, Row.Rise, Top) : ArchSection(Xb, Hb, Z, Row.Rise, Top);
				Shape.DepthMeters = Q.Depth;
				const FMuseeFrame Frame = Q.bEast ? RunningEast(Q.From) : RunningNorth(Q.From);
				FFormDrawing Drawing = Drawn(FString::Printf(TEXT("vault.web.%d.%s.%s"), Bay + 1, Row.Name, Q.Name), TEXT("vault.web"), MoveTemp(Shape),
					Solid, Frame);
				for (const FVector2D& Away : Q.Away)
				{
					Drawing.Cuts.Add(CutIn(Frame, Centre, Away));
				}
				Out.Add(MoveTemp(Drawing));
			}
		}

		/** A cross band over a pier line in a row: the row's barrel, 0.06 under the webs, 0.70 wide. */
		FFormDrawing CrossBand(int32 Line, const FRow& Row)
		{
			FFormExtrusion Shape;
			Shape.Outline = ArchSection(Row.Middle, Row.Half, Plan::Springing, Row.Rise, RowTop(Row));
			Shape.DepthMeters = 2.0 * Plan::BandHalf;
			return Drawn(FString::Printf(TEXT("vault.band.%d.%s"), Line + 1, Row.Name), TEXT("vault.band"), MoveTemp(Shape), Solid,
				RunningEast(PierX(Line) - Plan::BandHalf));
		}

		/** An arcade band over a bay on one arcade line, its nave face the lunette under the nave's web. */
		FFormDrawing ArcadeBand(int32 Bay, double Side)
		{
			FFormExtrusion Shape;
			Shape.Outline = ArchSection(BayCentre(Bay), BayHalf(Bay), Plan::Springing, Plan::ArcadeRise, Plan::TopHigh);
			Shape.DepthMeters = 2.0 * Plan::BandHalf;
			return Drawn(FString::Printf(TEXT("vault.arcade.%d.%s"), Bay + 1, Side < 0.0 ? TEXT("n") : TEXT("s")), TEXT("vault.band"), MoveTemp(Shape), Solid,
				RunningNorth(Side * Plan::ArcadeY + Plan::BandHalf));
		}
	}

	void DrawVaults(TArray<FFormDrawing>& Out)
	{
		for (int32 Bay = 0; Bay < Plan::Bays; ++Bay)
		{
			for (const FRow& Row : Rows())
			{
				Groin(Out, Bay, Row);
			}
			for (const double Side : {-1.0, 1.0})
			{
				Out.Add(ArcadeBand(Bay, Side));
			}
		}
		for (int32 Line = 0; Line < Plan::PierLines; ++Line)
		{
			for (const FRow& Row : Rows())
			{
				Out.Add(CrossBand(Line, Row));
			}
			// Over each pier the springing plane closes the vault at 4.00, the bands meeting on its square.
			for (const double Side : {-1.0, 1.0})
			{
				const double X = PierX(Line);
				const double Y = Side * Plan::ArcadeY;
				Out.Add(Drawn(FString::Printf(TEXT("vault.springing.%d%s"), Line + 1, Side < 0.0 ? TEXT("n") : TEXT("s")), TEXT("vault.band"),
					Block(FVector(X - Plan::BandHalf, Y - Plan::BandHalf, Plan::Springing), FVector(X + Plan::BandHalf, Y + Plan::BandHalf, Plan::TopHigh)),
					Solid));
			}
		}
	}

	TArray<FName> Pieces::Vaults()
	{
		TArray<FFormDrawing> Drawings;
		DrawVaults(Drawings);
		TArray<FName> Names;
		for (const FFormDrawing& Drawing : Drawings)
		{
			Names.Add(Drawing.Name);
		}
		return Names;
	}
}
