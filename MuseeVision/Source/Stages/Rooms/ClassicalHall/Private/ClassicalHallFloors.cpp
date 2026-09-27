#include "ClassicalHallParts.h"
#include "ClassicalHallPlan.h"

/**
 * The opus sectile floors (§3.2): 30 mm of stone on the slab, finished level at z 0, each figure one piece of its
 * stone. The gallery's is after the Pantheon's: a white border round a carpet of four bay squares (a verde frame, a
 * giallo field, a white ring round a porphyry disc) with panels of three white cells and red lozenges between them.
 * The tribune's is polar about O: discs, rings and sectors bounded at φ 15° + 30°k, out to a white border that follows
 * the wall into each niche's bay.
 */
namespace Musee::ClassicalHall
{
	namespace
	{
		const FVector2D O = FVector2D::ZeroVector;

		void Stone(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* Kind, FFormProfile Outline, TArray<FFormProfile> Holes = {})
		{
			Out.Add(PlanPrism(TEXT("floor.") + Name, Kind, MoveTemp(Outline), MoveTemp(Holes), -Plan::FloorStone, 0.0, Walkable));
		}

		/** The gallery's white border and the throat's floor on to the tribune's rim, round the carpet. */
		void GalleryBorder(TArray<FFormDrawing>& Out)
		{
			const double Half = Plan::GalleryHalf;
			const double Jamb = FMath::RadiansToDegrees(FMath::Acos(Half / Plan::TribuneRadius));
			FPen Border(-Half, Plan::SouthFaceY);
			Border.To(Half, Plan::SouthFaceY).To(Half, FMath::Sqrt(FMath::Square(Plan::TribuneRadius) - Half * Half));
			Border.Arc(O, Plan::TribuneRadius, Jamb, 180.0 - Jamb);
			Stone(Out, TEXT("gallery.white"), TEXT("floor.white"), Border.Closed(),
				{Rect(-Plan::CarpetHalf, Plan::CarpetNorthY, Plan::CarpetHalf, Plan::CarpetSouthY)});
			Stone(Out, TEXT("gallery.end.south"), TEXT("floor.verde"),
				Rect(-Plan::CarpetHalf, Plan::CarpetSouthY - Plan::CarpetEndPanel, Plan::CarpetHalf, Plan::CarpetSouthY));
			Stone(Out, TEXT("gallery.end.north"), TEXT("floor.verde"),
				Rect(-Plan::CarpetHalf, Plan::CarpetNorthY, Plan::CarpetHalf, Plan::CarpetNorthY + Plan::CarpetEndPanel));
		}

		/** A bay's square on its centre: the verde frame, the giallo field, the white ring and the porphyry disc. */
		void BaySquare(TArray<FFormDrawing>& Out, int32 Bay)
		{
			const FVector2D C(0.0, Plan::BayCentres[Bay]);
			const double Half = Plan::CarpetHalf;
			const double Inner = Half - Plan::FloorFrame;
			const FString Name = FString::Printf(TEXT("bay%d"), Bay + 1);
			Stone(Out, Name + TEXT(".frame"), TEXT("floor.verde"), Rect(-Half, C.Y - Half, Half, C.Y + Half), {Rect(-Inner, C.Y - Inner, Inner, C.Y + Inner)});
			Stone(Out, Name + TEXT(".field"), TEXT("floor.giallo"), Rect(-Inner, C.Y - Inner, Inner, C.Y + Inner), {Circle(C, Plan::BayRingRadius)});
			Stone(Out, Name + TEXT(".ring"), TEXT("floor.white"), Circle(C, Plan::BayRingRadius), {Circle(C, Plan::BayDiscRadius)});
			Stone(Out, Name + TEXT(".disc"), TEXT("floor.porphyry"), Circle(C, Plan::BayDiscRadius));
		}

		/** Between two bays: a verde frame round three white cells, each with a red lozenge on its centre. */
		void BetweenBays(TArray<FFormDrawing>& Out, int32 Pair)
		{
			const double Centre = Plan::PairCentres[Pair];
			const double Deep = 0.5 * Plan::BayLength - Plan::CarpetHalf;
			const double Inner = Deep - Plan::FloorFrame;
			const FString Name = FString::Printf(TEXT("between%d"), Pair + 1);
			const double CellsHalf = Plan::CellEdges[UE_ARRAY_COUNT(Plan::CellEdges) - 1];
			Stone(Out, Name + TEXT(".frame"), TEXT("floor.verde"), Rect(-Plan::CarpetHalf, Centre - Deep, Plan::CarpetHalf, Centre + Deep),
				{Rect(-CellsHalf, Centre - Inner, CellsHalf, Centre + Inner)});
			for (int32 Cell = 0; Cell + 1 < int32(UE_ARRAY_COUNT(Plan::CellEdges)); ++Cell)
			{
				const double X = 0.5 * (Plan::CellEdges[Cell] + Plan::CellEdges[Cell + 1]);
				const double L = Plan::LozengeHalf;
				const FVector2D Lozenge[] = {{X, Centre - L}, {X + L, Centre}, {X, Centre + L}, {X - L, Centre}};
				Stone(Out, FString::Printf(TEXT("%s.cell%d"), *Name, Cell + 1), TEXT("floor.white"),
					Rect(Plan::CellEdges[Cell], Centre - Inner, Plan::CellEdges[Cell + 1], Centre + Inner), {Polygon(Lozenge)});
				Stone(Out, FString::Printf(TEXT("%s.lozenge%d"), *Name, Cell + 1), TEXT("floor.rosso"), Polygon(Lozenge));
			}
		}

		/** A full ring between two radii. */
		void Ring(TArray<FFormDrawing>& Out, const FString& Name, const TCHAR* Kind, double Inner, double Outer)
		{
			Stone(Out, Name, Kind, Circle(O, Outer), {Circle(O, Inner)});
		}

		/** A sector's field inset by the border's width from its arcs and parallel to its two radii. */
		FFormProfile InsetSector(double Inner, double Outer, double FromDeg, double ToDeg, double Border)
		{
			const double In = Inner + Border;
			const double OutR = Outer - Border;
			auto Turn = [Border](double Radius) { return FMath::RadiansToDegrees(FMath::Asin(Border / Radius)); };
			FPen Pen(Polar(In, FromDeg + Turn(In)));
			Pen.Arc(O, In, FromDeg + Turn(In), ToDeg - Turn(In)).Arc(O, OutR, ToDeg - Turn(OutR), FromDeg + Turn(OutR));
			return Pen.Closed();
		}

		/** The tribune's figures from the centre to the verde band before the border (§3.2's table). */
		void TribuneRings(TArray<FFormDrawing>& Out)
		{
			Stone(Out, TEXT("tribune.disc"), TEXT("floor.porphyry"), Circle(O, Plan::TribuneDisc));
			Ring(Out, TEXT("tribune.ring1"), TEXT("floor.giallo"), Plan::TribuneDisc, Plan::TribuneRing1);
			const double Pitch = 360.0 / Plan::FloorSectors;
			for (int32 Sector = 0; Sector < Plan::FloorSectors; ++Sector)
			{
				const double From = Plan::FloorFirstSectorDeg + Pitch * Sector;
				const double Mid = From + 0.5 * Pitch;
				const FFormProfile Dot = Circle(Polar(Plan::InnerDiscAt, Mid), Plan::InnerDiscRadius);
				Stone(Out, FString::Printf(TEXT("tribune.sector%02d"), Sector + 1), TEXT("floor.white"),
					AnnularSector(O, Plan::TribuneRing1, Plan::TribuneRing2, From, From + Pitch), {Dot});
				Stone(Out, FString::Printf(TEXT("tribune.dot%02d"), Sector + 1), TEXT("floor.porphyry"), Dot);

				const FFormProfile Field = InsetSector(Plan::TribuneRing5, Plan::TribuneRing6, From, From + Pitch, Plan::SectorBorder);
				const FFormProfile Disc = Circle(Polar(Plan::OuterDiscAt, Mid), Plan::OuterDiscRadius);
				const bool bPorphyry = FMath::RoundToInt32(Mid / Pitch) % 2 == 1;
				Stone(Out, FString::Printf(TEXT("tribune.panel%02d.border"), Sector + 1), TEXT("floor.white"),
					AnnularSector(O, Plan::TribuneRing5, Plan::TribuneRing6, From, From + Pitch), {Field});
				Stone(Out, FString::Printf(TEXT("tribune.panel%02d.field"), Sector + 1), TEXT("floor.giallo"), Field, {Disc});
				Stone(Out, FString::Printf(TEXT("tribune.panel%02d.disc"), Sector + 1), bPorphyry ? TEXT("floor.porphyry") : TEXT("floor.verde"), Disc);
			}
			Ring(Out, TEXT("tribune.ring2"), TEXT("floor.giallo"), Plan::TribuneRing2, Plan::TribuneRing3);
			Ring(Out, TEXT("tribune.ring3"), TEXT("floor.white"), Plan::TribuneRing3, Plan::TribuneRing4);
			Ring(Out, TEXT("tribune.ring4"), TEXT("floor.verde"), Plan::TribuneRing4, Plan::TribuneRing5);
			Ring(Out, TEXT("tribune.ring5"), TEXT("floor.verde"), Plan::TribuneRing6, Plan::TribuneRing7);
		}

		/** The white border to the wall, r 6.30 to 6.60, and in each niche's bay out to its mouth plane. */
		void TribuneBorder(TArray<FFormDrawing>& Out)
		{
			const double Face = Plan::TribuneRadius;
			TArray<const FNiche*> Tribune;
			for (const FNiche& Niche : Niches())
			{
				if (Niche.bTribune)
				{
					Tribune.Add(&Niche);
				}
			}
			auto DegOf = [](const FNiche* Niche) { return FMath::Fmod(FMath::RadiansToDegrees(FMath::Atan2(Niche->In.Y, Niche->In.X)) + 360.0, 360.0); };
			Tribune.Sort([&DegOf](const FNiche& A, const FNiche& B) { return DegOf(&A) < DegOf(&B); });
			FPen Pen(Polar(Face, 0.0));
			double Deg = 0.0;
			for (const FNiche* Niche : Tribune)
			{
				const double Half = Niche->BayHalf();
				const double Turn = FMath::RadiansToDegrees(FMath::Asin(Half / Face));
				Pen.Arc(O, Face, Deg, DegOf(Niche) - Turn);
				const FVector Near = Niche->Origin - Niche->Lateral() * Half;
				const FVector Far = Niche->Origin + Niche->Lateral() * Half;
				Pen.To(Near.X, Near.Y).To(Far.X, Far.Y);
				Deg = DegOf(Niche) + Turn;
			}
			Pen.Arc(O, Face, Deg, 360.0);
			Stone(Out, TEXT("tribune.border"), TEXT("floor.white"), Pen.Closed(), {Circle(O, Plan::TribuneRing7)});
		}
	}

	void DrawFloors(TArray<FFormDrawing>& Out)
	{
		GalleryBorder(Out);
		for (int32 Bay = 0; Bay < Plan::Bays; ++Bay)
		{
			BaySquare(Out, Bay);
		}
		for (int32 Pair = 0; Pair < int32(UE_ARRAY_COUNT(Plan::PairCentres)); ++Pair)
		{
			BetweenBays(Out, Pair);
		}
		TribuneRings(Out);
		TribuneBorder(Out);
	}
}
